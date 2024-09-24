#ifndef __LATTE_TABLE_META_H
#define __LATTE_TABLE_META_H


#include <stdint.h>
#include "sds/sds.h"
#include "list/list.h"
#include "index_meta.h"
#include "field_meta.h"
#include "types.h"

typedef struct tableMeta {
    int32_t table_id;
    sds name;
    list_t* trx_fields;
    list_t* fields;    //list<FieldMeta>
    list_t* indexes;   //list<IndexMeta>
    storageFormat storage_format;
    int record_size;
} tableMeta;

indexMeta* table_meta_get_index_by_index(tableMeta* t, int index);
fieldMeta* table_meta_get_field_by_name(tableMeta* t, sds name);

//
indexMeta* indexMetaFromJson(tableMeta* table, value_t* json_value);
#endif