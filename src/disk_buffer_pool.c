#include "disk_buffer_pool.h"
#include "zmalloc/zmalloc.h"
#include "dict/dict_plugins.h"
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
#include <unistd.h>
#include "vector/vector.h"

// map <sds, diskBufferPool>
dict_func_t  bufferPoolDict = {
    dict_char_hash,
    NULL,
    NULL,
    dict_char_key_compare,
    dict_sds_destructor,
    NULL,
    NULL
};

dict_func_t idToBufferPoolsDict = {
    dict_char_hash,
    NULL,
    NULL,
    dict_char_key_compare,
    dict_sds_destructor,
    NULL,
    NULL
};

buffer_pool_manager_t*  buffer_pool_manager_new() {
    buffer_pool_manager_t* manager = zmalloc(sizeof(buffer_pool_manager_t));
    manager->lock = latte_mutex_new();
    manager->buffer_pools = dict_new(&bufferPoolDict);
    manager->id_to_buffer_pools = dict_new(&idToBufferPoolsDict);
    manager->next_buffer_pool_id = 0;
    manager->dblwr_buffer = NULL;
    manager->frame_manager = buffer_pool_frame_manager_new();
    return manager;
}

int buffer_pool_manager_init(buffer_pool_manager_t* manager , double_write_buffer_t* dblwr_buffer) {
    manager->dblwr_buffer = dblwr_buffer;
    return SUCCESS;
}

int disk_buffer_pool_allocate_frame_purger(disk_buffer_pool_t* pool, frame_t* frame) {
    int rc = SUCCESS;
    if (!frame->dirty) {
        return rc;
    }
    if (frame->frame_id->buffer_pool_id == pool->buffer_pool_id) {
        rc = disk_buffer_pool_flush_page_internal(pool, frame);
    } else {
        rc = buffer_pool_manager_flush_page(pool->bp_manger, frame);
    }
    if (is_rc_fail(rc)) {
        miniDBServerLog(LOG_ERROR ,"Failed to aclloc block due to failed to flush old block. rc=%d", rc);
    }
    return rc;
}

frame_t* disk_buffer_pool_allocate_frame(disk_buffer_pool_t* pool, PageNum page_num) {
    while(true) {
        frame_t* frame = buffer_pool_frame_manager_alloc(pool->frame_manager, pool->buffer_pool_id, page_num);
        if (frame != NULL) {
            miniDBServerLog(LOG_DEBUG, "allocate frame %p, page num %d", frame, page_num);
            return frame;
        }
        miniDBServerLog(LOG_TRACE, "frames are all allocated, so we should purge some frames to get one free frame");
        buffer_pool_frame_manager_purge_frames(pool->frame_manager, 1, disk_buffer_pool_allocate_frame_purger, pool);
    }
    return NULL;
}

int disk_buffer_pool_open_file(disk_buffer_pool_t* pool, const char* file_name) {
    int fd = open(file_name, O_RDWR);
    if (fd < 0) {
        miniDBServerLog(LOG_ERROR, "Failed to open file %s, because %s.", file_name, strerror(errno));
        return IOERR_ACCESS;
    }
    miniDBServerLog(LOG_INFO, "Successfully open buffer pool file %s.", file_name);

    pool->file_name = sds_new(file_name);
    pool->file_desc = fd;

    page_t header_page;
    int ret = readn(fd, &header_page, sizeof(header_page), NULL);
    if (ret != 0) {
        miniDBServerLog(LOG_ERROR, "Failed to read first page of %s, due to %s.", file_name, strerror(errno));
        close(fd);
        pool->file_desc = -1;
        return IOERR_READ;
    }

    bpFileHeader* tmp_file_header =  (bpFileHeader*)header_page.data;
    pool->buffer_pool_id = tmp_file_header->buffer_pool_id;

    int rc =SUCCESS;
    pool->hdr_frame = disk_buffer_pool_allocate_frame(pool, BP_HEADER_PAGE);
    if (pool->hdr_frame == NULL) {
        miniDBServerLog(LOG_ERROR, "failed to allocate frame for header. file name %s", file_name);
        close(fd);
        pool->file_desc = -1;
        return BUFFERPOOL_NOBUF;
    }

    frame_set_buffer_pool_id(pool->hdr_frame, pool->buffer_pool_id);
    frame_access(pool->hdr_frame);

    if (is_rc_fail(rc = disk_buffer_pool_load_page(pool, BP_HEADER_PAGE, pool->hdr_frame))) {
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

int buffer_pool_manager_open_file(buffer_pool_manager_t* manager, log_handler_t* handler, char* file_name, disk_buffer_pool_t** pool) {
    int rc = SUCCESS;
    latte_mutex_lock(manager->lock);
    dict_entry_t* node = dict_find(manager->buffer_pools, file_name);
    if (node != NULL) {
        miniDBServerLog(LOG_WARN, "file already opened. file name=%s", file_name);
        return BUFFERPOOL_OPEN;
    }

    disk_buffer_pool_t* bp = disk_buffer_pool_new(manager, manager->frame_manager, manager->dblwr_buffer, handler);
    rc = disk_buffer_pool_open_file(bp, file_name);
    if (is_rc_fail(SUCCESS)) {
        miniDBServerLog(LOG_WARN, "failed to open file name %s", file_name);
        disk_buffer_pool_delete(bp);
        goto end;
    }
    int32_t next_buffer_pool_id = 0;
    latte_atomic_get(manager->next_buffer_pool_id, next_buffer_pool_id);
    if (bp->buffer_pool_id >= next_buffer_pool_id) {
        latte_atomic_set(manager->next_buffer_pool_id, bp->buffer_pool_id + 1);
    }

    assert(DICT_OK == dict_add(manager->buffer_pools, sds_new(file_name), bp));
    assert(DICT_OK == dict_add(manager->id_to_buffer_pools, bp->buffer_pool_id, bp));
    char buffer[BT_BUFFER_SIZE];
    miniDBServerLog(LOG_DEBUG, "insert buffer pool into fd buffer pools. fd=%d, bp=%p, lbt=%s", bp->file_desc, bp, lbt(&buffer));
    *pool = bp;
end:
    latte_mutex_unlock(manager->lock);
    return rc;
}

buffer_pool_log_handler_t* buffer_pool_log_handler_new(disk_buffer_pool_t* pool, log_handler_t* log_handler) {
    buffer_pool_log_handler_t* handler = zmalloc(sizeof(buffer_pool_log_handler_t));
    handler->buffer_pool = pool;
    handler->log_handler = log_handler;
    return handler;
}

int buffer_pool_log_handler_flush_page(buffer_pool_log_handler_t* handler, page_t* page) {
    return handler->log_handler->waitLsn(handler->log_handler, page->lsn);
}

disk_buffer_pool_t* disk_buffer_pool_empty() {
    disk_buffer_pool_t* pool = zmalloc(sizeof(disk_buffer_pool_t));
    return pool;
}

disk_buffer_pool_t* disk_buffer_pool_new(buffer_pool_manager_t* manager, buffer_pool_frame_manager_t* frame_manager, double_write_buffer_t* dblwr_manager,
    log_handler_t* log_handler) {
    disk_buffer_pool_t* pool = disk_buffer_pool_empty();
    pool->bp_manger = manager;
    pool->frame_manager = frame_manager;
    pool->dblwr_manager = dblwr_manager;
    pool->log_handler = buffer_pool_log_handler_new(pool, log_handler);
    return pool;
}

void disk_buffer_pool_delete(disk_buffer_pool_t* pool) {
    zfree(pool);
}

int disk_buffer_pool_write_page(disk_buffer_pool_t* pool, PageNum page_num, page_t* page) {
    int result = SUCCESS;
    latte_mutex_lock(pool->wr_lock);
    int64_t offset = ((int64_t)page_num) * sizeof(page_t);
    if (lseek(pool->file_desc, offset, SEEK_SET) == -1) {
        latte_mutex_unlock(pool->wr_lock);
        miniDBServerLog(LOG_ERROR, "Failed to write page %lld of %d due to failed to seek %s.", offset, pool->file_desc, strerror(errno));
        return IOERR_SEEK;
    }
    if (writen(pool->file_desc, page, sizeof(page_t)) != 0) {
        miniDBServerLog(LOG_ERROR, "Failed to write page %lld of %d due to %s.", offset, pool->file_desc, strerror(errno));
        return IOERR_WRITE;
    }
    miniDBServerLog(LOG_TRACE, "write_page: buffer_pool_id:%d, page_num:%d, lsn=%d, check_sum=%d", pool->buffer_pool_id, page_num, page->lsn, page->check_sum);

    latte_mutex_unlock(pool->wr_lock);
    return SUCCESS;
}

int disk_buffer_pool_flush_all_pages(disk_buffer_pool_t* pool) {
    list_t* frames = buffer_pool_frame_manager_find_list(pool->frame_manager, pool->buffer_pool_id);
    latte_iterator_t* it = list_get_latte_iterator(frames, 0);
    while(latte_iterator_has_next(it)) {
        frame_t* frame = latte_iterator_next(it);
        int rc = disk_buffer_pool_flush_page(pool, frame);
        frame_unpin(frame);
        if (is_rc_fail(rc)) {
            miniDBServerLog(LOG_WARN, "failed to flush all pages");
            return rc;
        }
    }
    return SUCCESS;

}

int disk_buffer_pool_load_page(disk_buffer_pool_t* pool, PageNum page_num, frame_t* frame) {
    page_t* page = frame->page;
    int rc = pool->dblwr_manager->read_page(pool->dblwr_manager, pool, page_num, page);
    if (!is_rc_fail(rc)) {
        return rc;
    }
    latte_mutex_lock(pool->wr_lock);
    int64_t offset = ((int64_t)page_num) * BP_PAGE_SIZE;
    if (lseek(pool->file_desc, offset, SEEK_SET) == -1) {
        LATTE_LIB_LOG(LOG_ERROR, "Failed to load page %s:%d, due to failed to lseek:%s.", pool->file_name, page_num, strerror(errno));

        return IOERR_SEEK;
    }
    int len = 0;
    int ret = readn(pool->file_desc, page, BP_PAGE_SIZE, &len);
    if (ret != 0) {
        LATTE_LIB_LOG(LOG_ERROR, "Failed to load page %s, file_desc:%d, page num:%d, due to failed to read data:%s, ret=%d, page count=%d", 
        pool->file_name, pool->file_desc ,page_num, strerror(errno), ret, pool->file_header->allocated_pages);

        return IOERR_READ;
    }
    latte_mutex_unlock(pool->wr_lock);
    frame_set_page_num(frame, page_num);
    sds_t frame_str = frame_to_sds(frame);
    LATTE_LIB_LOG(LOG_DEBUG, "Load page %s:%d, file_desc:%d, frame=%s",
            pool->file_name, page_num, pool->file_desc, frame_str );
    sds_delete(frame_str);
    return SUCCESS;
}


// int disk_buffer_pool_purge_frame(disk_buffer_pool_t* p, PageNum page_num, frame_t* frame) {
//     int rc = SUCCESS;
//     if (frame->pin_count != 1) {
//         miniDBServerLog(LOG_INFO, "Begin to free page %d frame_id=%d, but it's pin count > 1:%d.",
//         frame->frame_id->page_num, frame->frame_id->buffer_pool_id, frame->pin_count);
//         return LOCKED_UNLOCK;
//     }
//     if (frame->dirty) {
//         int rc = disk_buffer_pool_flush_page_internal(p, frame);
//         if (is_rc_fail(rc)) {
//             miniDBServerLog(LOG_WARN, "Failed to flush page %d frame_id=%d during purge page.", frame->frame_id->page_num, frame->frame_id->buffer_pool_id);
//             return rc;
//         }
//     }
//     miniDBServerLog(LOG_DEBUG, "Successfully purge frame =%p, page %d frame_id=%s", frame, frame->frame_id->page_num, frame->frame_id->buffer_pool_id);
//     buffer_pool_frame_manager_delete(p->bp_manger,p->buffer_pool_id, page_num, frame);
//     return rc;
// }



frame_t* disk_buffer_pool_get_this_page(disk_buffer_pool_t* pool, PageNum page_num) {
    frame_t* used_match_frame = buffer_pool_frame_manager_get(pool->frame_manager, pool->buffer_pool_id, page_num);
    if (used_match_frame != NULL) {
        frame_access(used_match_frame);
        return used_match_frame;
    }
    frame_t* frame = NULL;
    latte_mutex_lock(pool->lock);
    frame = disk_buffer_pool_allocate_frame(pool, page_num);
    if (frame == NULL) {
        latte_mutex_unlock(pool->lock);
        miniDBServerLog(LOG_ERROR, "Failed to alloc frame %s:%d, due to failed to alloc page.", pool->file_name, page_num);
        return NULL;
    }
    frame->frame_id->buffer_pool_id = pool->buffer_pool_id;
    frame_access(frame);
    
    if (is_rc_fail(disk_buffer_pool_load_page(pool, page_num, frame))) {
        miniDBServerLog(LOG_ERROR, "Failed to load page %s:%d", pool->file_name, page_num);
        disk_buffer_pool_purge_frame(pool, page_num, frame);
        return NULL;
    } 
    latte_mutex_unlock(pool->lock);
    return frame;
}

int disk_buffer_pool_unpin_page(disk_buffer_pool_t* pool, frame_t* frame) {
    frame_unpin(frame);
    return SUCCESS;
}

// int disk_buffer_pool_allocate_frame_purger(disk_buffer_pool_t* pool, frame_t* frame) {
//     if (!frame->dirty) {
//         return SUCCESS;
//     }
//     int rc = SUCCESS;
//     if (frame->frame_id->buffer_pool_id == pool->buffer_pool_id) {
//         rc = disk_buffer_pool_flush_page_internal(pool, frame);
//     } else {
//         rc = buffer_pool_manager_flush_page(pool->bp_manger, frame);
//     }

//     if (is_rc_fail(rc)) {
//         miniDBServerLog(LOG_ERROR, "Failed to aclloc block due to failed to flush old block. rc=%d", rc);
//     }
//     return rc;
// }

// int disk_buffer_pool_allocate_frame(disk_buffer_pool_t* pool, PageNum page_num, frame_t** buffer) {
//     while(true) {
//         frame_t* frame = buffer_pool_frame_manager_alloc(pool->frame_manager,pool->buffer_pool_id, page_num);
//         if (frame != NULL) {
//             *buffer = frame;
//             miniDBServerLog(LOG_DEBUG, "allocate frame %p, page num %d", frame, page_num);
//             return SUCCESS;
//         }

//         miniDBServerLog(LOG_TRACE, "frames are all allocated, so we should purge some frames to get one free frame");
//         bp_frame_manager_purage_frames(pool->frame_manager, 1, disk_buffer_pool_allocate_frame_purger, pool);
//     }
//     return BUFFERPOOL_NOBUF;
// }



int disk_buffer_pool_flush_page_internal(disk_buffer_pool_t* pool, frame_t* frame) {
    int rc = buffer_pool_log_handler_flush_page(pool->log_handler, frame->page);
    if (is_rc_fail(rc)) {
        miniDBServerLog(LOG_WARN, "Failed to log flush frame= id:%d,page_num:%d,, dirty=%d,pin=%d,lsn=%d, rc=%d", 
            frame->frame_id->buffer_pool_id, 
            frame->frame_id->page_num,
            frame->dirty,
            frame->pin_count,
            frame->page->lsn,
            rc);
        // ignore error handle
    }
    frame_set_check_sum(frame,crc32jamcrc(frame->page->data, BP_PAGE_DATA_SIZE));
    rc = disk_double_write_buffer_add_page(pool->dblwr_manager, pool, frame->frame_id->page_num, frame->page);
    if (is_rc_fail(rc)) {
        return rc;
    }
    frame_clear_dirty(frame);
    miniDBServerLog(LOG_DEBUG, "Flush block. file desc=%d, frame=%s", pool->file_desc, frame->frame_id);

    return SUCCESS;
}

int disk_buffer_pool_purge_frame(disk_buffer_pool_t* pool, PageNum page_num, frame_t* buf) {
    int pin_count;
    latte_atomic_get(buf->pin_count, pin_count);
    if (pin_count != 1) {
        miniDBServerLog(LOG_INFO, "Begin to free page %d frame_id=buffer_pool_id:%d, but it's pin count > 1:%d.",
            buf->frame_id->page_num, buf->frame_id->buffer_pool_id, pin_count);
        return LOCKED_UNLOCK;
    }
    if (buf->dirty) {
        int rc = disk_buffer_pool_flush_page_internal(pool,buf);
        if (is_rc_fail(rc)) {
            miniDBServerLog(LOG_WARN,"Failed to flush page %d frame_id=%d during purge page.", buf->frame_id->page_num, buf->frame_id->buffer_pool_id);
            return rc;
        }
    }
    miniDBServerLog(LOG_DEBUG, "Successfully purge frame =%p, page %d buffer_pool_id=%d", buf, buf->frame_id->page_num, buf->frame_id->buffer_pool_id);
    buffer_pool_frame_manager_delete(pool->frame_manager, pool->buffer_pool_id, page_num, buf);
    return SUCCESS;
}

int disk_buffer_pool_purge_all_pages(disk_buffer_pool_t* pool) {
    list_t* used = buffer_pool_frame_manager_find_list(pool->frame_manager, pool->buffer_pool_id);
    latte_mutex_lock(pool->lock);
    latte_iterator_t* itor = list_get_latte_iterator(used, 0);
    while(latte_iterator_has_next(itor)) {
        frame_t* frame = latte_iterator_next(itor);
        disk_buffer_pool_purge_frame(pool, frame->frame_id->page_num, frame);
    }
    latte_mutex_unlock(pool->lock);
    return SUCCESS;
}

int disk_buffer_pool_close_file(disk_buffer_pool_t* pool) {
    int rc = SUCCESS;
    if (pool->file_desc < 0) {
        return rc;
    }

    frame_unpin(pool->hdr_frame);
    // TODO: 理论上是在回放时回滚未提交事务，但目前没有undo log，因此不下刷数据page，只通过redo log回放
    rc = disk_buffer_pool_purge_all_pages(pool);
    if (is_rc_fail(rc)) {
        miniDBServerLog(LOG_ERROR, "failed to close %s, due to failed to purge pages. rc=%d", pool->file_name, rc);
        return rc;
    }
    rc = disk_double_write_buffer_clear_pages(pool->dblwr_manager,pool);
    // rc = doubleWriteBufferClearPages(pool->dblwr_manager, pool);
    if (is_rc_fail(rc)) {
        miniDBServerLog(LOG_WARN,"failed to clear pages in double write buffer. filename=%s, rc=%s", pool->file_name, rc);
        return rc;
    }
    set_clear(pool->disposed_pages);
    // diskBufferPoolClear(pool->disposed_pages);

    if (close(pool->file_desc) < 0) {
        miniDBServerLog(LOG_ERROR, "Failed to close fileId:%d, fileName:%s, error:%s", pool->file_desc, pool->file_name, strerror(errno));
        return IOERR_CLOSE;
    }
    miniDBServerLog(LOG_INFO,"Successfully close file %d:%s.", pool->file_desc, pool->file_name);
    pool->file_desc = -1;
    return SUCCESS;
}

// buffer_pool_frame_manager_t* buffer_pool_frame_manager_new(char* tag) {
//     buffer_pool_frame_manager_t* manager = zmalloc(sizeof(buffer_pool_frame_manager_t));
//     manager->lock = latte_recursive_mutex_new();
//     manager->frames = lruCacheCreate(0);
//     manager->allocator = memPoolItemCreate(tag);
//     return manager;
// }

frame_t* buffer_pool_frame_manager_get_internal(buffer_pool_frame_manager_t* bp, frame_id_t* frameId) {
    frame_t* frame = NULL;
    frame = lruCache_get(bp->frames, frameId);
    if (frame != NULL) {
        frame_pin(frame);
    }
    return frame;
}

frame_t* buffer_pool_frame_manager_get(buffer_pool_frame_manager_t* bp, int buffer_pool_id, PageNum page_num) {
    frame_id_t frame_id = {
        .buffer_pool_id = buffer_pool_id,
        .page_num = page_num
    };
    latte_mutex_lock(bp->lock);
    frame_t* frame = buffer_pool_frame_manager_get_internal(bp, &frame_id);
    latte_mutex_unlock(bp->lock);
    return frame;
}

buffer_pool_frame_manager_deleteInternal(buffer_pool_frame_manager_t* bp_frame_manager, frame_id_t* frame_id, frame_t* frame) {
    frame_t* frame_source = lruCache_get(bp_frame_manager->frames, frame_id);
    minidb_assert(frame_source != NULL && frame == frame_source && frame->pin_count == 1,
        "failed to free frame. frameId=%d:%d, frame_source=%p, frame=%p, pinCount=%d",
            frame_id->buffer_pool_id, frame_id->page_num, frame_source, frame, frame->pin_count);
    frame->frame_id->page_num = -1;
    frame_unpin(frame);
    lruCache_remove(bp_frame_manager->frames, frame_id);
    memPoolSimple_free(bp_frame_manager->allocator, frame);
    return SUCCESS;
}


int buffer_pool_frame_manager_delete(buffer_pool_frame_manager_t* bp_frame_manager, int buffer_pool_id, PageNum page_num, frame_t* frame) {
    frame_id_t id = {
        .buffer_pool_id = buffer_pool_id,
        .page_num = page_num
    };
    int rc;
    latte_mutex_lock(bp_frame_manager->lock);
    rc = buffer_pool_frame_manager_deleteInternal(bp_frame_manager, &id, frame);
    latte_mutex_unlock(bp_frame_manager->lock);
    return rc;
}

int buffer_pool_frame_manager_purge_frames(buffer_pool_frame_manager_t* manager, int count, frame_purge fun, void* self) {
    int result;
    latte_mutex_lock(manager->lock);
    list_t* frames_can_purge = list_new();
    
    latte_iterator_t* it = list_get_latte_iterator(&manager->frames->list, 1);
    while(latte_iterator_has_next(it)) {
        keyValue* node = latte_iterator_next(it);
        frame_id_t* id = node->key;
        frame_t* frame = node->value;
        if (frame_can_purge(frame)) {
            frame_pin(frame);
            list_add_node_tail(frames_can_purge, frame);
            if (list_length(frames_can_purge) >= count) {
                break;
            }
        }
    }
    latte_iterator_delete(it);
    miniDBServerLog(LOG_INFO, "purge frames find %ld pages total", list_length(frames_can_purge));
    /// 当前还在frameManager的锁内，而 purger 是一个非常耗时的操作
    /// 他需要把脏页数据刷新到磁盘上去，所以这里会极大地降低并发度
    int freed_count = 0;
    it = list_get_latte_iterator(frames_can_purge, 0);
    while(latte_iterator_has_next(it)) {
        frame_t* frame = latte_iterator_next(it);
        int rc = fun(self, frame);
        if (!is_rc_fail(rc)) {
            buffer_pool_frame_manager_deleteInternal(manager, frame->frame_id, frame);
            freed_count++;
        } else {
            frame_unpin(frame);
            miniDBServerLog(LOG_WARN, "failed to purge frame. frame_id=%s, rc=%d", 
               frame->frame_id->buffer_pool_id, rc);
        }
    }

    latte_mutex_unlock(manager->lock);
    miniDBServerLog(LOG_INFO,"purge frame done. number=%d", freed_count);
    return freed_count;
}


list_t* buffer_pool_frame_manager_find_list(buffer_pool_frame_manager_t* manager, int buffer_pool_id) {
    list_t* frames = list_new();
    latte_mutex_lock(manager->lock);
    latte_iterator_t* it = list_get_latte_iterator(&manager->frames->list, 1);
    while(latte_iterator_has_next(it)) {
        keyValue* node = latte_iterator_next(it);
        frame_id_t* id = node->key;
        frame_t* frame = node->value;
        if (buffer_pool_id == frame->frame_id->buffer_pool_id) {
            frame_pin(frame);
            list_add_node_tail(frames, frame);
        }
    }
    latte_iterator_delete(it);
    latte_mutex_unlock(manager->lock);
    return frames;
}

frame_t* buffer_pool_frame_manager_alloc(buffer_pool_frame_manager_t* bp_manager, int buffer_pool_id, PageNum page_num) {
    frame_id_t* frame_id = frame_id_new(buffer_pool_id, page_num);
    frame_t *frame = NULL;
    latte_mutex_lock(bp_manager->lock);
    frame = buffer_pool_frame_manager_get_internal(bp_manager, frame_id);
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
            frame_id_delete(frame_id);
        }
    }
end:
    latte_mutex_unlock(bp_manager->lock);
    return frame;
}

disk_buffer_pool_t* buffer_pool_manager_get_buffer_pool(buffer_pool_manager_t* manager, int32_t id) {
    disk_buffer_pool_t* result = NULL;
    latte_mutex_lock(manager->lock);
    dict_entry_t* node = dict_find(manager->buffer_pools, id);
    if (node == NULL) {
        goto end;
    }
    result = dict_get_entry_val(node);
end:
    latte_mutex_unlock(manager->lock);
    return result;
}

int buffer_pool_manager_flush_page(buffer_pool_manager_t* manager, frame_t* frame) {
    int buffer_pool_id = frame->frame_id->buffer_pool_id;
    int rc = SUCCESS;
    latte_mutex_lock(manager->lock);
    dict_entry_t* entry = dict_find(manager->id_to_buffer_pools, buffer_pool_id);
    if (entry == NULL) {
        miniDBServerLog(LOG_WARN, "unknown buffer pool of id %d", buffer_pool_id);
        return INTERNAL;
    }

    disk_buffer_pool_t* bp = dict_get_val(entry);
    rc = disk_buffer_pool_flush_page(bp, frame);
    latte_mutex_unlock(manager->lock);
    return rc;
}

int disk_buffer_pool_flush_page(disk_buffer_pool_t* pool, frame_t* frame){
    int result;
    latte_mutex_lock(pool->lock);
    result = disk_buffer_pool_flush_page_internal(pool, frame);
    latte_mutex_unlock(pool->lock);
    return result;
}

