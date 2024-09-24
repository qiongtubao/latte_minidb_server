#include "record_manager.h"
#include "log.h"

record_file_handler_t* record_file_handler_new() {
    record_file_handler_t* handler = zmalloc(sizeof(record_file_handler_t));
    return handler;
}

void record_file_handler_close(record_file_handler_t* handler) {
    if (handler->disk_buffer_pool != NULL) {
        set_delete(handler->free_pages);
        handler->disk_buffer_pool = NULL;
        handler->log_handler = NULL;
        handler->table_meta = NULL;
    }
}

void record_file_handler_delete(record_file_handler_t* handler) {
    record_file_handler_close(handler);
    zfree(handler);
}

int recordFileHandlerInitFreePages(record_file_handler_t* handler) {
    int rc = SUCCESS;
    buffer_pool_iterator_t* bp_iterator;
    
    return rc;
}

int recordFileHandlerInit(record_file_handler_t* handler,disk_buffer_pool_t* pool, log_handler_t* log_handler, tableMeta* table_meta) {
    if (handler->disk_buffer_pool != NULL) {
        miniDBServerLog(LOG_ERROR, "record file handler has been openned.");
        return RECORD_OPENNED;
    }
    handler->disk_buffer_pool = pool;
    handler->log_handler = log_handler;
    handler->table_meta = table_meta;

    int rc = recordFileHandlerInitFreePages(handler);
    miniDBServerLog(LOG_INFO, "open record file handle done. rc=%d", rc);
    return SUCCESS;
}