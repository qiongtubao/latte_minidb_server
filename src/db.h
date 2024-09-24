#ifndef __LATTE_DB_H
#define __LATTE_DB_H

#include "sds/sds.h"
#include "dict/dict.h"
#include "list/list.h"
#include "trx.h"

typedef struct db {
    sds name;
    sds path;
    dict* opened_tables;
    struct bufferPoolManager* buffer_pool_manager;
    struct logHandler* log_handler;
    trxKit* trx_kit;
    int32_t next_table_id;
    struct LSN* check_point_lsn;
} db;



db* dbCreate();
int dbInit(db* d,char* dbname, sds dbpath, 
        sds trx_kit_name, sds log_handler_name);
void dbRelease(db* d);



#endif
