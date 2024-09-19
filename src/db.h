#ifndef __LATTE_DB_H
#define __LATTE_DB_H
#include "sds/sds.h"
#include "dict/dict.h"


typedef struct db {
    sds name;
    sds path;
    dict* opened_tables;
    struct bufferPoolManager* buffer_pool_manager;
    struct logHandler* log_handler;
    struct trxKit* trx_kit;
    int32_t next_table_id;
    struct LSN* check_point_lsn;
} db;

typedef struct table {
    struct db* db;
    sds base_dir;
    struct tableMeta* table_meta;
    struct diskBufferPool* diskBufferPool;
    struct recordFileHandler* record_handler;
    struct list* indexes;
} table;



#endif
