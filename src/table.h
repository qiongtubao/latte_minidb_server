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
    db* db;
    sds base_dir;
    tableMeta* table_meta;
    diskBufferPool* data_buffer_pool;
    recordFileHandler* record_handler;
    list* indexes;
} table;

table* tableCreate();
int tableOpen(table* t, db* d,char* dir, char* filename);
void tableRelease(table* t);

#endif