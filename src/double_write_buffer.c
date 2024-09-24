#include "double_write_buffer.h"
#include "mutex/mutex.h"
#include "log.h"
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "crc/crc.h"
#include "fs/fs.h"
#include <unistd.h>
#include <fcntl.h>
#include "disk_buffer_pool.h"
#include "utils.h"


uint64_t dictPageKeyHash(const void *key) {
    struct doubleWritePageKey* k = (doubleWritePageKey*)key;
    return k->buffer_pool_id | k->page_num;
}

int dictPageKeyCompare(void* privdata, const void *key1,
    const void *key2) {
    struct doubleWritePageKey* k1 = (doubleWritePageKey*)key1;
    struct doubleWritePageKey* k2 = (doubleWritePageKey*)key2;
    return k2->buffer_pool_id == k1->buffer_pool_id && 
        k2->page_num == k1->page_num;
}

void dictPageKeyDestructor(void *privdata, void *val)
{
    // doubleWritePageKeyFree(val);
}

struct dictType pagesDict = {
    dictPageKeyHash,
    NULL,
    NULL,
    dictPageKeyCompare,
    dictPageKeyDestructor,
    NULL,
    NULL
};

struct diskDoubleWriteBuffer* diskDoubleWriteBufferCreate(bufferPoolManager* manager) {
    diskDoubleWriteBuffer* buffer = zmalloc(sizeof(diskDoubleWriteBuffer));
    buffer->lock = mutexCreate();
    buffer->dblwr_pages = dictCreate(&pagesDict);
    buffer->file_desc = -1;
    buffer->max_pages = 16;
    buffer->header = doubleWriteBufferHeaderCreate(0);
    buffer->bp_manager = manager;
    return buffer;
}

doubleWriteBufferHeader* doubleWriteBufferHeaderCreate(int page_cnt) {
    doubleWriteBufferHeader* header = zmalloc(sizeof(doubleWriteBuffer));
    header->page_cnt = page_cnt;
    return header;
}

int diskDoubleWriteBufferOpenFile(diskDoubleWriteBuffer* buffer, const char* file_path) {
    if (buffer->file_desc >= 0) {
        miniDBServerLog(LOG_ERROR, "Double write buffer has already opened. file desc=%d", buffer->file_desc);
        return BUFFERPOOL_OPEN;
    }

    int fd = open(file_path, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        miniDBServerLog(LOG_ERROR, "Failed to open or creat %s, due to %s.", file_path, strerror(errno));
        return SCHEMA_DB_EXIST;
    }
    buffer->file_desc = fd;

    return disk_double_write_buffer_load_pages(buffer);
}

int doubleWriteBufferWritePageInternal(diskDoubleWriteBuffer* buffer, doubleWritePage* page) {
    int32_t page_index = page->page_index;
    int64_t offset = page_index * doubleWritePage_SIZE + doubleWriteBufferHeader_SIZE;
    if (lseek(buffer->file_desc, offset, SEEK_SET) == -1) {
        miniDBServerLog(LOG_ERROR, "Failed to add page %lld of %d due to failed to seek %s.", offset, buffer->file_desc, strerror(errno));
        return IOERR_SEEK;
    }

    if (writen(buffer->file_desc, page, doubleWritePage_SIZE) != 0) {
        miniDBServerLog(LOG_ERROR, "Failed to add page %lld of %d due to %s.", offset, buffer->file_desc, strerror(errno));
        return IOERR_WRITE;
    }
    return SUCCESS;
}

int doubleWriteBufferAddPage(diskDoubleWriteBuffer* buffer, diskBufferPool* bp, PageNum page_num, Page* page) {
    mutexLock(buffer->lock);
    doubleWritePageKey* key = doubleWritePageKeyCreate(
        bp->buffer_pool_id,
        page_num
    );
    dictEntry* entry = dictFind(buffer->dblwr_pages, &key);
    if (entry != NULL) {
        doubleWritePage* p = dictGetVal(entry);
        p->page = page;
        miniDBServerLog(LOG_TRACE, "[cache hit]add page into double write buffer. buffer_pool_id:%d,page_num:%d,lsn=%d, dwb size=%d",
              bp->buffer_pool_id, page_num, page->lsn, dictSize(buffer->dblwr_pages));
        return doubleWriteBufferWritePageInternal(buffer, p);
    }
    int64_t page_cnt = dictSize(buffer->dblwr_pages);
    doubleWritePage* age = doubleWritePageCreate(bp->buffer_pool_id, page_num, page_cnt, page);
    minidb_assert(DICT_OK == dictAdd(buffer->dblwr_pages, key, age), "insert dblwr_pages key:%d:%d fail", key->buffer_pool_id, key->page_num);
    miniDBServerLog(LOG_TRACE, "insert page into double write buffer. buffer_pool_id:%d,page_num:%d,lsn=%d, dwb size:%d",
            bp->buffer_pool_id, page_num, page->lsn, dictSize(buffer->dblwr_pages));
    
    int rc = doubleWriteBufferWritePageInternal(buffer, age);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN, "failed to write page into double write buffer. rc=%d buffer_pool_id:%d,page_num:%d,lsn=%d.",
            rc, bp->buffer_pool_id, page_num, page->lsn);
        return rc;
    }

    if (page_cnt + 1 > buffer->header->page_cnt) {
        buffer->header->page_cnt = page_cnt + 1;
        if (-1 == lseek(buffer->file_desc, 0, SEEK_SET)) {
            miniDBServerLog(LOG_ERROR,"Failed to add page header due to failed to seek %s.", strerror(errno));
            return IOERR_SEEK;
        }
        if (0 != writen(buffer->file_desc, buffer->header, sizeof(buffer->header))) {
            miniDBServerLog(LOG_ERROR,"Failed to add page header due to %s.", strerror(errno));
            return IOERR_WRITE;
        }
    }

    if ( buffer->max_pages <= dictSize(buffer->dblwr_pages)) {
        rc = doubleWriteBufferFlushPage(buffer);
        if (isRcFail(rc)) {
            miniDBServerLog(LOG_ERROR, "Failed to flush pages in double write buffer");
            return rc;
        }
    }

    mutexUnlock(buffer->lock);
    return SUCCESS;
}

int disk_double_write_buffer_load_pages(diskDoubleWriteBuffer* buffer) {
    if (buffer->file_desc < 0) {
        miniDBServerLog(LOG_ERROR, "Failed to load pages, due to file desc is invalid.");
        return BUFFERPOOL_OPEN;
    }

    if (!buffer->dblwr_pages == NULL) {
        miniDBServerLog(LOG_ERROR, "Failed to load pages, due to double write buffer is not empty. opened?");
        return BUFFERPOOL_OPEN;
    }

    if (lseek(buffer->file_desc, 0, SEEK_SET) == -1) {
        miniDBServerLog(LOG_ERROR, "Failed to load page header, due to failed to lseek:%s.", strerror(errno));
        return IOERR_SEEK;
    }
    //一次读取
    int ret = readn(buffer->file_desc, buffer->header, sizeof(buffer->header), NULL);
    if (ret != 0 && ret != -1) {
        miniDBServerLog(LOG_ERROR, "Failed to load page header, file_desc:%d, due to failed to read data:%s, ret=%d", 
            buffer->file_desc, strerror(errno), ret);
        return IOERR_READ;
    }

    for(int page_num = 0; page_num < buffer->header->page_cnt; page_num++) {
        int64_t offset = ((int64_t)page_num) * doubleWritePage_SIZE+  doubleWriteBufferHeader_SIZE;
        if (lseek(buffer->file_desc, offset, SEEK_SET) == -1) {
            miniDBServerLog(LOG_ERROR,"Failed to load page %d, offset=%ld, due to failed to lseek:%s.", page_num, offset, strerror(errno));
            return IOERR_SEEK;
        }
        doubleWritePage* dblwr_page = doubleWritePageCreate();
        dblwr_page->page->check_sum = (CheckSum) -1;
        ret = readn(buffer->file_desc, dblwr_page->page->data, doubleWritePage_SIZE, NULL);
        if (ret != 0) {
            miniDBServerLog(LOG_ERROR, "Failed to load page, file_desc:%d, page num:%d, due to failed to read data:%s, ret=%d, page count=%d",
                buffer->file_desc, page_num, strerror(errno), ret, page_num);
            return IOERR_READ;
        }

        const CheckSum check_sum = crc32jamcrc(dblwr_page->page->data, BP_PAGE_DATA_SIZE);
        if (check_sum == dblwr_page->page->check_sum) {
            minidb_assert(DICT_OK == dictAdd(buffer->dblwr_pages, &dblwr_page->key, dblwr_page), "insert diskDoubleWriteBuffer dblwr_page fail");
        } else {
            miniDBServerLog(LOG_TRACE, "got a page with an invalid checksum. on disk:%d, in memory:%d", 
                dblwr_page->page->check_sum, check_sum);
        }
    } 
    miniDBServerLog(LOG_INFO, "double write buffer load pages done. page num=%d", dictSize(buffer->dblwr_pages));
    return SUCCESS;
}

int doubleWriteBufferWritePage(diskDoubleWriteBuffer* buffer, doubleWritePage* page) {
    diskBufferPool* disk_buffer = NULL;
    if (!page->valid) {
        miniDBServerLog(LOG_TRACE, "double write buffer write page invalid. buffer_pool_id:%d,page_num:%d,lsn=%d",
              page->key.buffer_pool_id, page->key.page_num, page->page->lsn);
        return SUCCESS;
    }
    disk_buffer = bufferPoolManagerGetBufferPool(buffer->bp_manager, page->key.buffer_pool_id);
    minidb_assert(disk_buffer != NULL, "failed to get disk buffer pool of %d", page->key.buffer_pool_id);
    miniDBServerLog(LOG_TRACE, "double write buffer write page. buffer_pool_id:%d,page_num:%d,lsn=%d",
            page->key.buffer_pool_id, page->key.page_num, page->page->lsn);
    return diskBufferPoolWritePage(disk_buffer, page->key.page_num, page->page);
}

int doubleWriteBufferflushPage(diskDoubleWriteBuffer* buffer) {
    sync();

    dictIterator* iterator = dictGetIterator(buffer->dblwr_pages);
    dictEntry* entry = NULL;
    while ((entry = dictNext(iterator)) == NULL) {
        doubleWritePage * dwp =  dictGetVal(entry);
        int rc = doubleWriteBufferWritePage(buffer, dwp);
        if (rc != SUCCESS) {
            return rc;
        }
        dwp->valid = false;
        doubleWriteBufferWritePageInternal(buffer, dwp);
        // doubleWritePageRelease(dwp);
    }
    dictReleaseIterator(iterator);
    dictDestroy(buffer->dblwr_pages);
    buffer->header->page_cnt = 0;
    return SUCCESS;

    
}
int dblwr_buffer_recover(diskDoubleWriteBuffer* buffer) {
    return doubleWriteBufferflushPage(buffer);
}



// ===== doubleWritePageKey*
doubleWritePageKey* doubleWritePageKeyCreate(int32_t buffer_pool_id, int32_t page_num) {
    doubleWritePageKey* page_key = zmalloc(sizeof(doubleWritePageKey));
    page_key->page_num = page_num;
    page_key->buffer_pool_id = buffer_pool_id;
    return page_key;
}

void doubleWritePageKeyRelease(doubleWritePageKey* key) {
    zfree(key);
}


//========= doubleWritePage*

doubleWritePage* doubleWritePageCreate() {
    doubleWritePage* page = zmalloc(sizeof(doubleWritePage));
    return page;
}