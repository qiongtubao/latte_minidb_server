#include "table.h"
#include "zmalloc/zmalloc.h"
#include "code.h"
#include "log.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "list/list.h"
#include "index_meta.h"
#include "field_meta.h"
#include "bplus_tree_index.h"

table* tableCreate() {
    table* t = zmalloc(sizeof(table));
    return t;
}



int table_init_record_handler(table* t, const char *base_dir) {
    sds data_file = sdscatfmt(sdsempty(), "%s/%s.data", base_dir, t->table_meta->name);

    bufferPoolManager* bpm = t->db->buffer_pool_manager;
    int rc = bufferPoolManagerOpenFile(bpm, t->db->log_handler, data_file, &t->data_buffer_pool);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_ERROR, "Failed to open disk buffer pool for file:%s. rc=%d", data_file, rc);
        return rc;
    }
    t->record_handler =  recordFileHandlerCreate(t->table_meta->storage_format);
    rc = recordFileHandlerInit(t->record_handler, t->data_buffer_pool, t->db->log_handler, t->table_meta);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_ERROR, "Failed to init record handler. rc=%d", rc);
        data_buffer_pool_close_file(t->data_buffer_pool);
        t->data_buffer_pool = NULL;
        recordHandlerRelease(t->record_handler);
        t->record_handler = NULL;
        return rc;
    }
    return rc;
}


sds table_index_file(const char* base_dir, const char* table_name, const char *index_name) {
    return sdscatfmt(sdsempty(), "%s/%s-%s.index", base_dir, table_name, index_name);
}
/**
 * 打开一个表
 * @param meta_file 保存表元数据的文件完整路径
 * @param base_dir 表所在的文件夹，表记录数据文件、索引数据文件存放位置
 */
int tableOpen(table* t, db* d, char* base_dir, char* meta_file) {
    sds meta_file_path = sdscatfmt(sdsempty(), "%s/%s", base_dir, meta_file);
    int fd = open(meta_file_path, O_RDONLY);
    if (fd < 0) {
        miniDBServerLog(LOG_ERROR, 
            "Failed to open meta file for read. file name=%s, errmsg=%s", meta_file_path, strerror(errno));
        return IOERR_OPEN;
    }
    int rc = table_meta_deserialize(t, fd);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_ERROR, "Failed to deserialize table meta. file name=%s", meta_file_path);
        close(fd);
        return INTERNAL;
    }
    close(fd);
    t->db = d;
    t->base_dir = sdsnew(base_dir);

    //加载数据文件
    rc = table_init_record_handler(t, base_dir);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_ERROR, "Failed to open table %s due to init record handler failed.", base_dir);
        return rc;
    }

    const int index_num = listLength(t->table_meta->indexes);
    for (int i = 0; i < index_num; i++) {
        const indexMeta* index_meta = table_meta_get_index_by_index(t->table_meta, i);
        const fieldMeta* field_meta = table_meta_get_field_by_name(t->table_meta, index_meta->field);
        if (field_meta == NULL) {
            miniDBServerLog(LOG_ERROR, "Found invalid index meta info which has a non-exists field. table=%s, index=%s, field=%s",
                t->table_meta->name, index_meta->name, index_meta->field);
            return INTERNAL;
        }
        bplusTreeIndex* index = bplusTreeIndexCreate();
        sds index_file = table_index_file(t->base_dir, t->table_meta->name , index_meta->name);
        int rc = bplusTreeIndexOpen(index, t, index_file, index_meta, field_meta);
        if (isRcFail(rc)) {
            bplusTreeIndexRelese(index);
            miniDBServerLog(LOG_ERROR, "Failed to open index. table=%s, index=%s, file=%s, rc=%d",
                t->table_meta->name, index_meta->name, index_file, rc);
            return rc;
        }
        listAddNodeTail(t->indexes, index);
    }
    return rc;
}
void tableRelease(table* t) {
    zfree(t);
}


