#ifndef __LATTE_DB_H
#define __LATTE_DB_H

#include "sds/sds.h"
#include "dict/dict.h"
#include "list/list.h"
#include "trx.h"

typedef struct db_t {
    sds name;
    sds path;
    dict_t* opened_tables;
    struct buffer_pool_manager_t* buffer_pool_manager;
    struct log_handler_t* log_handler;
    trxKit* trx_kit;
    int32_t next_table_id;
    struct LSN* check_point_lsn;
} db_t;



db_t* db_new();
int db_init(db_t* d,char* dbname, sds dbpath, 
        sds trx_kit_name, sds log_handler_name);
void db_delete(db_t* d);



#endif
