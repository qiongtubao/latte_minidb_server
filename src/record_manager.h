#ifndef __LATTE_RECORD_MANAGER_H
#define __LATTE_RECORD_MANAGER_H

#include "disk_buffer_pool.h"
#include "log_handler.h"
#include "types.h"
#include "table_meta.h"

typedef struct record_file_handler_t {
    disk_buffer_pool_t* disk_buffer_pool;
    log_handler_t* log_handler;
    set_t* free_pages;
    latte_mutex_t* lock;
    storageFormat storage_format;
    tableMeta*     table_meta;
} record_file_handler_t;

record_file_handler_t* record_file_handler_new();
void record_file_handler_delete(record_file_handler_t * handler);



#endif