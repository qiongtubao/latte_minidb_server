#include "disk_buffer_pool.h"
#include "zmalloc/zmalloc.h"
#include "dict_plugins/dict_plugins.h"
#include "code.h"
#include "log.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include "fs/fs.h"
#include "frame.h"
#include "double_write_buffer.h"
#include "utils/atomic.h"
#include "crc/crc.h"
#include "utils.h"

// map <sds, diskBufferPool>
struct dictType  bufferPoolDict = {
    dictCharHash,
    NULL,
    NULL,
    dictCharKeyCompare,
    dictSdsDestructor,
    NULL,
    NULL
};

struct dictType idToBufferPoolsDict = {
    dictCharHash,
    NULL,
    NULL,
    dictCharKeyCompare,
    dictSdsDestructor,
    NULL,
    NULL
};

bufferPoolManager*  bufferPoolManagerCreate() {
    bufferPoolManager* manager = zmalloc(sizeof(bufferPoolManager));
    manager->lock = mutexCreate();
    manager->buffer_pools = dictCreate(&bufferPoolDict);
    manager->id_to_buffer_pools = dictCreate(&idToBufferPoolsDict);
    manager->next_buffer_pool_id = 0;
    manager->dblwr_buffer = NULL;
    manager->frame_manager = frameManagerCreate();
    return manager;
}

int bufferPoolManagerInit(bufferPoolManager* manager , doubleWriteBuffer* dblwr_buffer) {
    manager->dblwr_buffer = dblwr_buffer;
    return SUCCESS;
}


int bufferPoolManagerOpenFile(bufferPoolManager* manager, logHandler* handler, char* file_name, diskBufferPool** pool) {
    int rc = SUCCESS;
    mutexLock(manager->lock);
    dictEntry* node = dictFind(manager->buffer_pools, file_name);
    if (node != NULL) {
        miniDBServerLog(LOG_WARN, "file already opened. file name=%s", file_name);
        return BUFFERPOOL_OPEN;
    }

    diskBufferPool* bp = diskBufferPoolCreate(manager, manager->frame_manager, manager->dblwr_buffer, handler);
    rc = diskBufferPoolOpenFile(bp, file_name);
    if (isRcFail(SUCCESS)) {
        miniDBServerLog(LOG_WARN, "failed to open file name %s", file_name);
        diskBufferPoolRelease(bp);
        goto end;
    }

    if (bp->buffer_pool_id >= atomic_load(&manager->next_buffer_pool_id)) {
        atomic_store(&manager->next_buffer_pool_id, bp->buffer_pool_id + 1);
    }

    assert(DICT_OK == dictAdd(manager->buffer_pools, sdsnew(file_name), bp));
    assert(DICT_OK == dictAdd(manager->id_to_buffer_pools, bp->buffer_pool_id, bp));
    char buffer[BT_BUFFER_SIZE];
    miniDBServerLog(LOG_DEBUG, "insert buffer pool into fd buffer pools. fd=%d, bp=%p, lbt=%s", bp->file_desc, bp, lbt(&buffer));
    *pool = bp;
end:
    mutexUnlock(manager->lock);
    return rc;
}

bufferPoolLogHandler* bufferPoolLogHandlerCreate(diskBufferPool* pool, logHandler* log_handler) {
    bufferPoolLogHandler* handler = zmalloc(sizeof(bufferPoolLogHandler));
    handler->buffer_pool = pool;
    handler->log_handler = log_handler;
    return handler;
}

int bufferPoolLogHandlerFlushPage(bufferPoolLogHandler* handler, Page* page) {
    return handler->log_handler->waitLsn(handler->log_handler, page->lsn);
}

diskBufferPool* diskBufferPoolEmpty() {
    diskBufferPool* pool = zmalloc(sizeof(diskBufferPool));
    return pool;
}

diskBufferPool* diskBufferPoolCreate(bufferPoolManager* manager, bpFrameManager* frame_manager, doubleWriteBuffer* dblwr_manager,
    logHandler* log_handler) {
    diskBufferPool* pool = diskBufferPoolEmpty();
    pool->bp_manger = manager;
    pool->frame_manager = frame_manager;
    pool->dblwr_manager = dblwr_manager;
    pool->log_handler = bufferPoolLogHandlerCreate(pool, log_handler);
    return pool;
}

void diskBufferPoolRelease(diskBufferPool* pool) {
    zfree(pool);
}

int diskBufferPoolWritePage(diskBufferPool* pool, PageNum page_num, Page* page) {
    int result = SUCCESS;
    mutexLock(pool->wr_lock);
    int64_t offset = ((int64_t)page_num) * sizeof(Page);
    if (lseek(pool->file_desc, offset, SEEK_SET) == -1) {
        mutexUnlock(pool->wr_lock);
        miniDBServerLog(LOG_ERROR, "Failed to write page %lld of %d due to failed to seek %s.", offset, pool->file_desc, strerror(errno));
        return IOERR_SEEK;
    }
    if (writen(pool->file_desc, page, sizeof(Page)) != 0) {
        miniDBServerLog(LOG_ERROR, "Failed to write page %lld of %d due to %s.", offset, pool->file_desc, strerror(errno));
        return IOERR_WRITE;
    }
    miniDBServerLog(LOG_TRACE, "write_page: buffer_pool_id:%d, page_num:%d, lsn=%d, check_sum=%d", pool->buffer_pool_id, page_num, page->lsn, page->check_sum);

    mutexUnlock(pool->wr_lock);
    return SUCCESS;
}

int diskBufferPoolFlushAllPages(diskBufferPool* pool) {
    list* frames = bpFrameManagerFindList(pool->frame_manager, pool->buffer_pool_id);
    Iterator* it = listGetLatteIterator(frames, 0);
    while(iteratorHasNext(it)) {
        Frame* frame = iteratorNext(it);
        int rc = diskBufferPoolFlushPage(pool, frame);
        frame_unpin(frame);
        if (isRcFail(rc)) {
            miniDBServerLog(LOG_WARN, "failed to flush all pages");
            return rc;
        }
    }
    return SUCCESS;

}


// int diskBufferPoolPurgeFrame(diskBufferPool* p, PageNum page_num, Frame* frame) {
//     int rc = SUCCESS;
//     if (frame->pin_count != 1) {
//         miniDBServerLog(LOG_INFO, "Begin to free page %d frame_id=%d, but it's pin count > 1:%d.",
//         frame->frame_id->page_num, frame->frame_id->buffer_pool_id, frame->pin_count);
//         return LOCKED_UNLOCK;
//     }
//     if (frame->dirty) {
//         int rc = diskBufferPoolFlushPageInternal(p, frame);
//         if (isRcFail(rc)) {
//             miniDBServerLog(LOG_WARN, "Failed to flush page %d frame_id=%d during purge page.", frame->frame_id->page_num, frame->frame_id->buffer_pool_id);
//             return rc;
//         }
//     }
//     miniDBServerLog(LOG_DEBUG, "Successfully purge frame =%p, page %d frame_id=%s", frame, frame->frame_id->page_num, frame->frame_id->buffer_pool_id);
//     bpFrameManagerFree(p->bp_manger,p->buffer_pool_id, page_num, frame);
//     return rc;
// }
int diskBufferPoolAllocateFramePurger(diskBufferPool* pool, Frame* frame) {
    int rc = SUCCESS;
    if (!frame->dirty) {
        return rc;
    }
    if (frame->frame_id->buffer_pool_id == pool->buffer_pool_id) {
        rc = diskBufferPoolFlushPageInternal(pool, frame);
    } else {
        rc = bufferPoolManagerFlushPage(pool->bp_manger, frame);
    }
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_ERROR ,"Failed to aclloc block due to failed to flush old block. rc=%d", rc);
    }
    return rc;
}
Frame* diskBufferPoolAllocateFrame(diskBufferPool* pool, PageNum page_num) {
    while(true) {
        Frame* frame = bpFrameManagerAlloc(pool->frame_manager, pool->buffer_pool_id, page_num);
        if (frame != NULL) {
            miniDBServerLog(LOG_DEBUG, "allocate frame %p, page num %d", frame, page_num);
            return frame;
        }
        miniDBServerLog(LOG_TRACE, "frames are all allocated, so we should purge some frames to get one free frame");
        bpFrameManagerPurgeFrames(pool->frame_manager, 1, diskBufferPoolAllocateFramePurger, pool);
    }
    return NULL;
}

Frame* diskBufferPoolGetThisPage(diskBufferPool* pool, PageNum page_num) {
    Frame* used_match_frame = bpFrameManagerGet(pool->frame_manager, pool->buffer_pool_id, page_num);
    if (used_match_frame != NULL) {
        frameAccess(used_match_frame);
        return used_match_frame;
    }
    Frame* frame = NULL;
    mutexLock(pool->lock);
    frame = diskBufferPoolAllocateFrame(pool, page_num);
    if (frame == NULL) {
        mutexUnlock(pool->lock);
        miniDBServerLog(LOG_ERROR, "Failed to alloc frame %s:%d, due to failed to alloc page.", pool->file_name, page_num);
        return NULL;
    }
    frame->frame_id->buffer_pool_id = pool->buffer_pool_id;
    
    frameAccess(frame);
    if (isRcFail(diskBufferPoolLoadPage(pool, page_num, frame))) {
        miniDBServerLog(LOG_ERROR, "Failed to load page %s:%d", pool->file_name, page_num);
        diskBufferPoolPurgeFrame(pool, page_num, frame);
        return NULL;
    } 
    mutexUnlock(pool->lock);
    return frame;
}

int disBufferPoolUnpinPage(diskBufferPool* pool, Frame* frame) {
    frame_unpin(frame);
    return SUCCESS;
}

int disk_buffer_pool_allocate_frame_purger(diskBufferPool* pool, Frame* frame) {
    if (!frame->dirty) {
        return SUCCESS;
    }
    int rc = SUCCESS;
    if (frame->frame_id->buffer_pool_id == pool->buffer_pool_id) {
        rc = disk_buffer_pool_flush_page_internal(pool, frame);
    } else {
        rc = buffer_pool_manager_flush_page(pool->bp_manger, frame);
    }

    if (isRcFail(rc)) {
        miniDBServerLog(LOG_ERROR, "Failed to aclloc block due to failed to flush old block. rc=%d", rc);
    }
    return rc;
}

int disk_buffer_pool_allocate_frame(diskBufferPool* pool, PageNum page_num, Frame** buffer) {
    while(true) {
        Frame* frame = bp_frame_manager_alloc(pool->frame_manager,pool->buffer_pool_id, page_num);
        if (frame != NULL) {
            *buffer = frame;
            miniDBServerLog(LOG_DEBUG, "allocate frame %p, page num %d", frame, page_num);
            return SUCCESS;
        }

        miniDBServerLog(LOG_TRACE, "frames are all allocated, so we should purge some frames to get one free frame");
        bp_frame_manager_purage_frames(pool->frame_manager, 1, disk_buffer_pool_allocate_frame_purger, pool);
    }
    return BUFFERPOOL_NOBUF;
}

int diskBufferPoolOpenFile(diskBufferPool* pool, const char* file_name) {
    int fd = open(file_name, O_RDWR);
    if (fd < 0) {
        miniDBServerLog(LOG_ERROR, "Failed to open file %s, because %s.", file_name, strerror(errno));
        return IOERR_ACCESS;
    }
    miniDBServerLog(LOG_INFO, "Successfully open buffer pool file %s.", file_name);

    pool->file_name = sdsnew(file_name);
    pool->file_desc = fd;

    Page header_page;
    int ret = readn(fd, &header_page, sizeof(header_page), NULL);
    if (ret != 0) {
        miniDBServerLog(LOG_ERROR, "Failed to read first page of %s, due to %s.", file_name, strerror(errno));
        closed(fd);
        pool->file_desc = -1;
        return IOERR_READ;
    }

    bpFileHeader* tmp_file_header =  (bpFileHeader*)header_page.data;
    pool->buffer_pool_id = tmp_file_header->buffer_pool_id;

    int rc = disk_buffer_pool_allocate_frame(pool, BP_HEADER_PAGE, &pool->hdr_frame);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_ERROR, "failed to allocate frame for header. file name %s", file_name);
        close(fd);
        pool->file_desc = -1;
        return rc;
    }

    hdr_frame_set_buffer_pool_id(pool->hdr_frame, pool->buffer_pool_id);
    hdr_frame_access(pool->hdr_frame);

    if (isRcFail(rc = disk_buffer_pool_load_page(pool, BP_HEADER_PAGE, pool->hdr_frame))) {
        miniDBServerLog(LOG_ERROR, "Failed to load first page of %s, due to %s.", file_name, strerror(errno));
        close(fd);
        pool->file_desc = -1;
        return rc;
    }
    pool->file_header = (bpFileHeader*)pool->hdr_frame->page->data;
    miniDBServerLog(LOG_INFO, "Successfully open %s. file_desc=%d, hdr_frame=%p, file header=pageCount:%d,allocatedCount:%d",
           file_name, pool->file_desc, pool->hdr_frame, pool->file_header->page_count, pool->file_header->allocated_pages
           );
    return SUCCESS;
}

int diskBufferPoolFlushPageInternal(diskBufferPool* pool, Frame* frame) {
    int rc = bufferPoolLogHandlerFlushPage(pool->log_handler, frame->page);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN, "Failed to log flush frame= id:%d,page_num:%d,, dirty=%d,pin=%d,lsn=%d, rc=%d", 
            frame->frame_id->buffer_pool_id, 
            frame->frame_id->page_num,
            frame->dirty,
            frame->pin_count,
            frame->page->lsn,
            rc);
        // ignore error handle
    }
    frameSetCheckSum(frame,crc32jamcrc(frame->page->data, BP_PAGE_DATA_SIZE));
    rc = doubleWriteBufferAddPage(pool->dblwr_manager, pool, frame->frame_id->page_num, frame->page);
    if (isRcFail(rc)) {
        return rc;
    }
    frameClearDirty(frame);
    miniDBServerLog(LOG_DEBUG, "Flush block. file desc=%d, frame=%s", pool->file_desc, frame->frame_id);

    return SUCCESS;
}

int diskBufferPoolPurgeFrame(diskBufferPool* pool, PageNum page_num, Frame* buf) {
    int pin_count;
    atomicGet(buf->pin_count, pin_count);
    if (pin_count != 1) {
        miniDBServerLog(LOG_INFO, "Begin to free page %d frame_id=buffer_pool_id:%d, but it's pin count > 1:%d.",
            buf->frame_id->page_num, buf->frame_id->buffer_pool_id, pin_count);
        return LOCKED_UNLOCK;
    }
    if (buf->dirty) {
        int rc = diskBufferPoolFlushPageInternal(pool,buf);
        if (isRcFail(rc)) {
            miniDBServerLog(LOG_WARN,"Failed to flush page %d frame_id=%d during purge page.", buf->frame_id->page_num, buf->frame_id->buffer_pool_id);
            return rc;
        }
    }
    miniDBServerLog(LOG_DEBUG, "Successfully purge frame =%p, page %d buffer_pool_id=%d", buf, buf->frame_id->page_num, buf->frame_id->buffer_pool_id);
    bpFrameManagerFree(pool->frame_manager, pool->buffer_pool_id, page_num, buf);
    return SUCCESS;
}

int diskBufferPoolPurgeAllPages(diskBufferPool* pool) {
    list* used = bpFrameManagerFindList(pool->frame_manager, pool->buffer_pool_id);
    mutexLock(pool->lock);
    Iterator* itor = listGetLatteIterator(used, 0);
    while(iteratorHasNext(itor)) {
        Frame* frame = iteratorNext(itor);
        diskBufferPoolPurgeFrame(pool, frame->frame_id->page_num, frame);
    }
    mutexUnlock(pool->lock);
    return SUCCESS;
}

int diskBufferPoolCloseFile(diskBufferPool* pool) {
    int rc = SUCCESS;
    if (pool->file_desc < 0) {
        return rc;
    }

    frame_unpin(pool->hdr_frame);
    // TODO: 理论上是在回放时回滚未提交事务，但目前没有undo log，因此不下刷数据page，只通过redo log回放
    rc = diskBufferPoolPurgeAllPages(pool);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_ERROR, "failed to close %s, due to failed to purge pages. rc=%d", pool->file_name, rc);
        return rc;
    }

    rc = doubleWriteBufferClearPages(pool->dblwr_manager, pool);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN,"failed to clear pages in double write buffer. filename=%s, rc=%s", pool->file_name, rc);
        return rc;
    }
    diskBufferPoolClear(pool->disposed_pages);

    if (close(pool->file_desc) < 0) {
        miniDBServerLog(LOG_ERROR, "Failed to close fileId:%d, fileName:%s, error:%s", pool->file_desc, pool->file_name, strerror(errno));
        return IOERR_CLOSE;
    }
    miniDBServerLog(LOG_INFO,"Successfully close file %d:%s.", pool->file_desc, pool->file_name);
    pool->file_desc = -1;
    return SUCCESS;
}

Frame* bp_frame_manager_get_internal(bpFrameManager* bp, frameId* frameId) {
    Frame* frame = NULL;
    frame = lruCache_get(bp->frames, frameId);
    if (frame != NULL) {
        frame_pin(frame);
    }
    return frame;
}

bpFrameManagerFreeInternal(bpFrameManager* bp_frame_manager, frameId* frame_id, Frame* frame) {
    Frame* frame_source = lruCache_get(bp_frame_manager->frames, frame_id);
    minidb_assert(frame_source != NULL && frame == frame_source && frame->pin_count == 1,
        "failed to free frame. frameId=%d:%d, frame_source=%p, frame=%p, pinCount=%d",
            frame_id->buffer_pool_id, frame_id->page_num, frame_source, frame, frame->pin_count);
    frame->frame_id->page_num = -1;
    frame_unpin(frame);
    lruCache_remove(bp_frame_manager->frames, frame_id);
    memPoolSimple_free(bp_frame_manager->allocator, frame);
    return SUCCESS;
}


int bpFrameManagerFree(bpFrameManager* bp_frame_manager, int buffer_pool_id, PageNum page_num, Frame* frame) {
    frameId id = {
        .buffer_pool_id = buffer_pool_id,
        .page_num = page_num
    };
    int rc;
    mutexLock(bp_frame_manager->lock);
    rc = bpFrameManagerFreeInternal(bp_frame_manager, &id, frame);
    mutexUnlock(bp_frame_manager->lock);
    return rc;
}

int bpFrameManagerPurgeFrames(bpFrameManager* manager, int count, frame_purge fun, void* self) {
    int result;
    mutexLock(manager->lock);
    list* frames_can_purge = listCreate();
    
    Iterator* it = listGetLatteIterator(&manager->frames->list, 1);
    while(iteratorHasNext(it)) {
        keyValue* node = iteratorNext(it);
        frameId* id = node->key;
        Frame* frame = node->value;
        if (frameCanPurge(frame)) {
            frame_pin(frame);
            listAddNodeTail(frames_can_purge, frame);
            if (listLength(frames_can_purge) >= count) {
                break;
            }
        }
    }
    iteratorRelease(it);
    miniDBServerLog(LOG_INFO, "purge frames find %ld pages total", listLength(frames_can_purge));
    /// 当前还在frameManager的锁内，而 purger 是一个非常耗时的操作
    /// 他需要把脏页数据刷新到磁盘上去，所以这里会极大地降低并发度
    int freed_count = 0;
    it = listGetLatteIterator(frames_can_purge, 0);
    while(iteratorHasNext(it)) {
        Frame* frame = iteratorNext(it);
        int rc = fun(self, frame);
        if (!isRcFail(rc)) {
            bpFrameManagerFreeInternal(manager, frame->frame_id, frame);
            freed_count++;
        } else {
            frame_unpin(frame);
            miniDBServerLog(LOG_WARN, "failed to purge frame. frame_id=%s, rc=%d", 
               frame->frame_id->buffer_pool_id, rc);
        }
    }

    mutexUnlock(manager->lock);
    miniDBServerLog(LOG_INFO,"purge frame done. number=%d", freed_count);
    return freed_count;
}

Frame* bp_frame_manager_alloc(bpFrameManager* bp_manager, int buffer_pool_id, PageNum page_num) {
    frameId* frame_id = frameIdCreate(buffer_pool_id, page_num);
    Frame *frame = NULL;
    mutexLock(bp_manager->lock);
    frame = bp_frame_manager_get_internal(bp_manager, frame_id);
    if (frame != NULL) {
        goto end;
    }
    frame = memPoolSimple_alloc(bp_manager->allocator);
    if (frame != NULL) {
        assert(frame->pin_count == 0);
        // assert(frame->pin_count == 0, "got an invalid frame that pin count is not 0. frame=%p", frame);
        frame->frame_id->buffer_pool_id = buffer_pool_id;
        frame->frame_id->page_num = page_num;
        frame_pin(frame);
        if (lruCache_put(bp_manager->frames, frame_id, frame) == 0) {
            frameIdRelease(frame_id);
        }
    }
end:
    mutexUnlock(bp_manager->lock);
    return frame;
}

diskBufferPool * bufferPoolManagerGetBufferPool(bufferPoolManager* manager, int32_t id) {
    diskBufferPool* result = NULL;
    mutexLock(manager->lock);
    dictEntry* node = dictFind(manager->buffer_pools, id);
    if (node == NULL) {
        goto end;
    }
    result = dictGetVal(node);
end:
    mutexUnlock(manager->lock);
    return result;
}

int bufferPoolManagerFlushPage(bufferPoolManager* manager, Frame* frame) {
    int buffer_pool_id = frame->frame_id->buffer_pool_id;
    int rc = SUCCESS;
    mutexLock(manager->lock);
    dictEntry* entry = dictFind(manager->id_to_buffer_pools, buffer_pool_id);
    if (entry == NULL) {
        miniDBServerLog(LOG_WARN, "unknown buffer pool of id %d", buffer_pool_id);
        return INTERNAL;
    }

    diskBufferPool* bp = dictGetVal(entry);
    rc = diskBufferPoolFlushPage(bp, frame);
    mutexUnlock(manager->lock);
    return rc;
}

int diskBufferPoolFlushPage(diskBufferPool* pool, Frame* frame){
    int result;
    mutexLock(pool->lock);
    result = diskBufferPoolFlushPageInternal(pool, frame);
    mutexUnlock(pool->lock);
    return result;
}