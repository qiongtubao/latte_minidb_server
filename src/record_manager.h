#ifndef __LATTE_RECORD_MANAGER_H
#define __LATTE_RECORD_MANAGER_H

#include "disk_buffer_pool.h"
#include "log_handler.h"
#include "types.h"
#include "table_meta.h"

typedef struct recordFileHandler {
    diskBufferPool* disk_buffer_pool;
    logHandler* log_handler;
    set* free_pages;
    latte_mutex* lock;
    storageFormat storage_format;
    tableMeta*     table_meta;
} recordFileHandler;

recordFileHandler* recordFileHandlerCreate();



#endif