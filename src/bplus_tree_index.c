#include "bplus_tree_index.h"
#include "log.h"
#include "code.h"

bplusTreeIndex* bplusTreeIndexCreate() {
    bplusTreeIndex* index = zmalloc(sizeof(bplusTreeIndex));
    return index;
}

void bplusTreeIndexRelese(bplusTreeIndex* index) {
    zfree(index);
}

int bplusTreeIndexOpen(bplusTreeIndex* index, table* t, const char* file_name, indexMeta* index_meta, fieldMeta* field_meta) {
    if (index->inited) {
        miniDBServerLog(LOG_WARN, "Failed to open index due to the index has been initedd before. file_name:%s, index:%s, field:%s",
        file_name, index_meta->name, index_meta->field);
        return RECORD_OPENNED;
    }
    index_init(index, index_meta, field_meta);
    bufferPoolManager* bpm = t->db->buffer_pool_manager;
    int rc = bplus_tree_index_handler_open(index->index_handler, t->db->log_handler, bpm, file_name);
    if (isRcFail(rc)) {
        miniDBServerLog(LOG_WARN ,"Failed to open index_handler, file_name:%s, index:%s, field:%s, rc:%d",
            file_name, index_meta->name, index_meta->field, rc);
        return rc;
    } 
    index->inited = true;
    index->table = t;
    miniDBServerLog(LOG_INFO, "Successfully open index, file_name:%s, index:%s, field:%s",
        file_name, index_meta->name, index_meta->field);
    return SUCCESS;

}