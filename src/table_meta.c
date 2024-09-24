#include "table_meta.h"
#include "value/value.h"
#include "json/latte_json.h"
#include "fs/fs.h"
#include "log.h"
#include "json/latte_json.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

indexMeta* table_meta_get_index_by_index(tableMeta* t, int index) {
    list_node_t* node = list_index(t->indexes, index);
    if (node == NULL) return NULL;
    return (indexMeta*)node->value;
}

fieldMeta* table_meta_get_field_by_name(tableMeta* t, sds name) {
    if (name == NULL) {
        return NULL;
    }
    list_iterator_t* iter =  list_get_iterator(t->fields,AL_START_HEAD);
    list_node_t* node = NULL;
    fieldMeta* meta = NULL;
    while ((node = list_next(iter)) != NULL) {
        fieldMeta* field = list_node_value(node);
        if (0 == strcmp(field->name, name)) {
            meta  = field;
            goto end;
        }
    }
end: 
    list_iterator_delete(iter);
    return meta;

}

//json写入文件
int table_meta_serialize(tableMeta* table, int fd) {
    //TODO
}

const char* FIELD_TABLE_ID = "table_id";
const char* FIELD_TABLE_NAME = "table_name";
const char* FIELD_STORAGE_FORMAT = "storage_format";
const char* FIELD_FIELDS = "fields";
const char* FIELD_INDEXES = "indexes";


int fieldMetaComparator(void* f1, void* f2) {
    fieldMeta* fm1 = (fieldMeta*)f1;
    fieldMeta* fm2 = (fieldMeta*)f2;
    return fm1->attr_offset < fm2->attr_offset;
}

//读取json
int table_meta_deserialize(tableMeta* table, int fd) {
    value_t* table_value;
    sds file = readall(fd);
    int success = sds_to_json(file, &table_value);
    if (!success) {
        miniDBServerLog(LOG_ERROR,"Failed to deserialize table meta. error");
        return -1;
    }

    //TODO 细节处理一下 valueIsInt64 ...
    int64_t tableid = value_get_int64(json_map_get_value(table_value, FIELD_TABLE_ID));
    sds table_name = value_get_sds(json_map_get_value(table_value, FIELD_TABLE_NAME));
    vector_t* fields_value = value_get_array(json_map_get_value(table_value, FIELD_FIELDS));
    storageFormat storage_format = (storageFormat)value_get_int64(json_map_get_value(table_value, FIELD_STORAGE_FORMAT));
    
    vector_t* fields = vector_new();
    vector_resize(fields, vector_size(fields_value));
    latte_iterator_t* fieldv_iter = vector_get_iterator(fields_value);
    while(latte_iterator_has_next(fieldv_iter)) {
        value_t* field_value = latte_iterator_next(fieldv_iter);
        fieldMeta* field = fieldMetaFromJson(field_value);
        if (field == NULL) {
            miniDBServerLog(LOG_ERROR, "Failed to deserialize table meta. table name =%s",  table_name);
            return -1;
        }
        vector_push(fields, field);
    }
    latte_iterator_delete(fieldv_iter);
    vector_sort(fields, fieldMetaComparator);
    table->name = sds_dup(table_name);
    table->fields = fields;
    table->storage_format = storage_format;
    fieldMeta* field_last = ((fieldMeta*)vector_get(fields, fields->count - 1));
    fieldMeta* field_frist = ((fieldMeta*)vector_get(fields, 0));
    table->record_size = field_last->attr_offset + field_last->attr_len - field_frist->attr_offset;
    latte_iterator_t* field_iter =vector_get_iterator(fields);
    while(latte_iterator_has_next(field_iter)) {
        fieldMeta* field_meta = latte_iterator_next(field_iter);
        if (!field_meta->visible) {
            vector_push(table->trx_fields, field_meta);
        }
    }
    latte_iterator_delete(field_iter);
    
    value_t* indexes_value = json_map_get_value(table_value, FIELD_INDEXES);
    if (indexes_value != NULL) {
        if (!value_is_array(indexes_value)) {
            miniDBServerLog(LOG_ERROR,"Invalid table meta. indexes is not array, json :%s", file);
            return -1;
        }
        vector_t* indexes_array = value_get_array(indexes_value);
        vector_t* indexes = vector_new();
        vector_resize(indexes, vector_size(indexes_array));
        
        latte_iterator_t* indexIt = vector_get_iterator(indexes_array);
        while(latte_iterator_has_next(indexIt)) {
            value_t* index_value = latte_iterator_next(indexIt);
            indexMeta* index = indexMetaFromJson(table, index_value);
            if (index == NULL) {
                miniDBServerLog(LOG_ERROR, "Failed to deserialize table meta. table name=%s", table_name);
                return -1;
            }
            vector_push(indexes, index);
        }
        latte_iterator_delete(indexIt);
        table->indexes = indexes;
    }
    int filesize = sds_len(file);
    sds_delete(file);
    return filesize;
}



//about index meta 

indexMeta* indexMetaFromJson(tableMeta* table, value_t* json_value) {
    sds json_str = NULL;
    value_t* name_value  = json_map_get_value(json_value, FIELD_NAME);
    if (!value_is_sds(name_value)) {
        miniDBServerLog(LOG_ERROR, "Index name is not a string.");
        goto error;
    }
    sds name = value_get_sds(name_value);
    value_t* field_value  = json_map_get_value(json_value, FIELD_FIELD_NAME);
    if (!value_is_sds(field_value)) {
        miniDBServerLog(LOG_ERROR, "Field name of index [%s] is not a string", name);
        goto error;
    }
    sds field_str = value_get_sds(field_value);
    list_node_t* node = list_search_key(table->fields, field_str);
    if (NULL == node) {
        miniDBServerLog("Deserialize index [%s]: no such field: %s", name, field_str);
        goto error;
    }
    fieldMeta* field = node->value;

    indexMeta* index = indexMetaCreate();
    index->name = sds_dup(name);
    index->field = sds_dup(field->name); //还无法确定是否能共用或者智能指针等等  先拷贝
    return index;
error:
    json_str = json_to_sds(json_value);
    miniDBServerLog(LOG_ERROR, "[indexMetaFromJson] fail json: %s", json_str);
    sds_delete(json_str);
    return NULL;
}