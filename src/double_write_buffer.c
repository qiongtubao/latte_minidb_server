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
#include "vector/vector.h"


uint64_t dictPageKeyHash(const void *key) {
    struct double_write_page_key_t* k = (double_write_page_key_t*)key;
    return k->buffer_pool_id | k->page_num;
}

int dictPageKeyCompare(void* privdata, const void *key1,
    const void *key2) {
    struct double_write_page_key_t* k1 = (double_write_page_key_t*)key1;
    struct double_write_page_key_t* k2 = (double_write_page_key_t*)key2;
    return k2->buffer_pool_id == k1->buffer_pool_id && 
        k2->page_num == k1->page_num;
}

void dictPageKeyDestructor(void *privdata, void *val)
{
    // doubleWritePageKeyFree(val);
}

dict_func_t pagesDict = {
    dictPageKeyHash,
    NULL,
    NULL,
    dictPageKeyCompare,
    dictPageKeyDestructor,
    NULL,
    NULL
};

struct disk_double_write_buffer_t* disk_double_write_buffer_new(buffer_pool_manager_t* manager) {
    disk_double_write_buffer_t* buffer = zmalloc(sizeof(disk_double_write_buffer_t));
    buffer->lock = latte_mutex_new();
    buffer->dblwr_pages = dict_new(&pagesDict);
    buffer->file_desc = -1;
    buffer->max_pages = 16;
    buffer->header = double_write_buffer_header_new(0);
    buffer->bp_manager = manager;
    return buffer;
}

double_write_buffer_header_t* double_write_buffer_header_new(int page_cnt) {
    double_write_buffer_header_t* header = zmalloc(sizeof(double_write_buffer_header_t));
    header->page_cnt = page_cnt;
    return header;
}

int disk_double_write_buffer_open_file(disk_double_write_buffer_t* buffer, const char* file_path) {
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

int disk_double_write_buffer_write_page_internal(disk_double_write_buffer_t* buffer, double_write_page_t* page) {
    int32_t page_index = page->page_index;
    int64_t offset = page_index * DOUBLE_WRITE_PAGE_SIZE + DOUBLE_WRITE_BUFFER_HEADER_SIZE;
    if (lseek(buffer->file_desc, offset, SEEK_SET) == -1) {
        miniDBServerLog(LOG_ERROR, "Failed to add page %lld of %d due to failed to seek %s.", offset, buffer->file_desc, strerror(errno));
        return IOERR_SEEK;
    }

    if (writen(buffer->file_desc, page, DOUBLE_WRITE_PAGE_SIZE) != 0) {
        miniDBServerLog(LOG_ERROR, "Failed to add page %lld of %d due to %s.", offset, buffer->file_desc, strerror(errno));
        return IOERR_WRITE;
    }
    return SUCCESS;
}

int disk_double_write_buffer_add_page(disk_double_write_buffer_t* buffer, disk_buffer_pool_t* bp, PageNum page_num, page_t* page) {
    latte_mutex_lock(buffer->lock);
    double_write_page_key_t* key = double_write_page_key_new(
        bp->buffer_pool_id,
        page_num
    );
    dict_entry_t* entry = dict_find(buffer->dblwr_pages, &key);
    if (entry != NULL) {
        double_write_page_t* p = dict_get_val(entry);
        p->page = page;
        miniDBServerLog(LOG_TRACE, "[cache hit]add page into double write buffer. buffer_pool_id:%d,page_num:%d,lsn=%d, dwb size=%d",
              bp->buffer_pool_id, page_num, page->lsn, dict_size(buffer->dblwr_pages));
        return disk_double_write_buffer_write_page_internal(buffer, p);
    }
    int64_t page_cnt = dict_size(buffer->dblwr_pages);
    double_write_page_t* age = double_write_page_new(bp->buffer_pool_id, page_num, page_cnt, page);
    minidb_assert(DICT_OK == dict_add(buffer->dblwr_pages, key, age), "insert dblwr_pages key:%d:%d fail", key->buffer_pool_id, key->page_num);
    miniDBServerLog(LOG_TRACE, "insert page into double write buffer. buffer_pool_id:%d,page_num:%d,lsn=%d, dwb size:%d",
            bp->buffer_pool_id, page_num, page->lsn, dict_size(buffer->dblwr_pages));
    
    int rc = disk_double_write_buffer_write_page_internal(buffer, age);
    if (is_rc_fail(rc)) {
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

    if ( buffer->max_pages <= dict_size(buffer->dblwr_pages)) {
        rc = doubleWriteBufferflushPage(buffer);
        if (is_rc_fail(rc)) {
            miniDBServerLog(LOG_ERROR, "Failed to flush pages in double write buffer");
            return rc;
        }
    }

    latte_mutex_unlock(buffer->lock);
    return SUCCESS;
}

int disk_double_write_buffer_load_pages(disk_double_write_buffer_t* buffer) {
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
        int64_t offset = ((int64_t)page_num) * DOUBLE_WRITE_PAGE_SIZE+  DOUBLE_WRITE_BUFFER_HEADER_SIZE;
        if (lseek(buffer->file_desc, offset, SEEK_SET) == -1) {
            miniDBServerLog(LOG_ERROR,"Failed to load page %d, offset=%ld, due to failed to lseek:%s.", page_num, offset, strerror(errno));
            return IOERR_SEEK;
        }
        double_write_page_t* dblwr_page = double_write_page_new();
        dblwr_page->page->check_sum = (CheckSum) -1;
        ret = readn(buffer->file_desc, dblwr_page->page->data, DOUBLE_WRITE_PAGE_SIZE, NULL);
        if (ret != 0) {
            miniDBServerLog(LOG_ERROR, "Failed to load page, file_desc:%d, page num:%d, due to failed to read data:%s, ret=%d, page count=%d",
                buffer->file_desc, page_num, strerror(errno), ret, page_num);
            return IOERR_READ;
        }

        const CheckSum check_sum = crc32jamcrc(dblwr_page->page->data, BP_PAGE_DATA_SIZE);
        if (check_sum == dblwr_page->page->check_sum) {
            minidb_assert(DICT_OK == dict_add(buffer->dblwr_pages, &dblwr_page->key, dblwr_page), "insert diskDoubleWriteBuffer dblwr_page fail");
        } else {
            miniDBServerLog(LOG_TRACE, "got a page with an invalid checksum. on disk:%d, in memory:%d", 
                dblwr_page->page->check_sum, check_sum);
        }
    } 
    miniDBServerLog(LOG_INFO, "double write buffer load pages done. page num=%d", dict_size(buffer->dblwr_pages));
    return SUCCESS;
}

int doubleWriteBufferWritePage(disk_double_write_buffer_t* buffer, double_write_page_t* page) {
    disk_buffer_pool_t* disk_buffer = NULL;
    if (!page->valid) {
        miniDBServerLog(LOG_TRACE, "double write buffer write page invalid. buffer_pool_id:%d,page_num:%d,lsn=%d",
              page->key.buffer_pool_id, page->key.page_num, page->page->lsn);
        return SUCCESS;
    }
    disk_buffer = buffer_pool_manager_get_buffer_pool(buffer->bp_manager, page->key.buffer_pool_id);
    minidb_assert(disk_buffer != NULL, "failed to get disk buffer pool of %d", page->key.buffer_pool_id);
    miniDBServerLog(LOG_TRACE, "double write buffer write page. buffer_pool_id:%d,page_num:%d,lsn=%d",
            page->key.buffer_pool_id, page->key.page_num, page->page->lsn);
    return disk_buffer_pool_write_page(disk_buffer, page->key.page_num, page->page);
}

int doubleWriteBufferflushPage(disk_double_write_buffer_t* buffer) {
    sync();

    latte_iterator_t* iterator = dict_get_latte_iterator(buffer->dblwr_pages);
    // dict_entry_t* entry = NULL;
    while (latte_iterator_has_next(iterator)) {
        latte_pair_t* pair = latte_iterator_next(iterator);
        double_write_page_t* dwp =  latte_pair_value(pair);
        int rc = doubleWriteBufferWritePage(buffer, dwp);
        if (rc != SUCCESS) {
            return rc;
        }
        dwp->valid = false;
        disk_double_write_buffer_write_page_internal(buffer, dwp);
        // doubleWritePageRelease(dwp);
    }
    latte_iterator_delete(iterator);
    dict_destroy(buffer->dblwr_pages);
    buffer->header->page_cnt = 0;
    return SUCCESS;

    
}
int dblwr_buffer_recover(disk_double_write_buffer_t* buffer) {
    return doubleWriteBufferflushPage(buffer);
}



// ===== double_write_page_key_t*
double_write_page_key_t* double_write_page_key_new(int32_t buffer_pool_id, int32_t page_num) {
    double_write_page_key_t* page_key = zmalloc(sizeof(double_write_page_key_t));
    page_key->page_num = page_num;
    page_key->buffer_pool_id = buffer_pool_id;
    return page_key;
}

void double_write_page_key_delete(double_write_page_key_t* key) {
    zfree(key);
}


//========= double_write_page_t*

double_write_page_t* double_write_page_new() {
    double_write_page_t* page = zmalloc(sizeof(double_write_page_t));
    return page;
}

void double_write_page_delete(double_write_page_t* page) {
    zfree(page);
}


int page_cmp(void* a, void* b) {
    double_write_page_t* a1 = (double_write_page_t*)a;
    double_write_page_t* b1 = (double_write_page_t*)b;
    return a1->key.page_num - b1->key.page_num;
}
int disk_double_write_buffer_clear_pages(disk_double_write_buffer_t* buffer, disk_buffer_pool_t* buffer_pool) {

    vector_t* spec_pages = vector_new();
    latte_mutex_lock(buffer->lock);

    latte_iterator_t* iterator =  dict_get_latte_iterator(buffer->dblwr_pages);
    while(latte_iterator_has_next(iterator)) {
        latte_pair_t* pair = latte_iterator_next(iterator);
        double_write_page_t* dbl_page = latte_pair_value(pair);
        if (buffer_pool->buffer_pool_id == dbl_page->key.buffer_pool_id) {
            vector_push(spec_pages, dbl_page);
        }
    }
    latte_iterator_delete(iterator);

    latte_mutex_unlock(buffer->lock);

    LATTE_LIB_LOG(LOG_INFO, "clear pages in double write buffer. file name=%s, page count=%d",
           buffer_pool->file_name, vector_size(spec_pages));
    // 页面从小到大排序，防止出现小页面还没有写入，而页面编号更大的seek失败的情况
    vector_sort(spec_pages, page_cmp);

    int rc = SUCCESS;
    iterator = vector_get_iterator(spec_pages);
    while(latte_iterator_has_next(iterator)) {
        double_write_page_t* page = latte_iterator_next(iterator);
        rc = disk_buffer_pool_write_page(buffer_pool, page->key.page_num, page->page);
        if (is_rc_fail(rc)) {
            LATTE_LIB_LOG(LOG_WARN, "Failed to write page %s:%d to disk buffer pool. rc=%d",
                buffer_pool->file_name, page->key.page_num, rc);
            break;
        }
        page->valid = false;
        disk_double_write_buffer_write_page_internal(buffer_pool, page);
    }   
    latte_iterator_delete(iterator);


    //还不太懂 为什么不直接处理完了就删除
    iterator = vector_get_iterator(spec_pages);
    while(latte_iterator_has_next(iterator)) {
        double_write_page_t* page = latte_iterator_next(iterator);
        double_write_page_delete(page);
    }
    latte_iterator_delete(iterator);
    return SUCCESS;
}