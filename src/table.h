#ifndef __LATTE_TABLE_H
#define __LATTE_TABLE_H

#include <stdint.h>
#include "sds/sds.h"
#include "list/list.h"
#include "table_meta.h"
#include "disk_buffer_pool.h"
#include "record_manager.h"
#include "db.h"

typedef struct table {
    db_t* db;
    sds base_dir;
    tableMeta* table_meta;
    disk_buffer_pool_t* data_buffer_pool;
    record_file_handler_t* record_handler;
    list_t* indexes;
} table;

table* tableCreate();
int tableOpen(table* t, db_t* d,char* dir, char* filename);
void tableRelease(table* t);

#endif