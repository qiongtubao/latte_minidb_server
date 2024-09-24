#include "field_meta.h"
#include "zmalloc/zmalloc.h"
#include "log.h"
#include "json/latte_json.h"

fieldMeta* fieldMetaCreate(const char *name, attr_type_enum attr_type, int attr_offset, int attr_len, bool visible, int field_id) {
    fieldMeta* meta = zmalloc(sizeof(fieldMeta));
    meta->name = sds_new(name);
    meta->attr_type = attr_type;
    meta->attr_offset = attr_offset;
    meta->attr_len = attr_len;
    meta->visible = visible;
    meta->field_id = field_id;
    return meta;
}
const char*  FIELD_NAME     =  "name";
const char*  FIELD_TYPE     =  "type";
const char*  FIELD_OFFSET   =  "offset";
const char*  FIELD_LEN      =  "len";
const char*  FIELD_VISIBLE  =  "visible";
const char*  FIELD_FIELD_ID =  "FIELD_id";

fieldMeta* fieldMetaFromJson(value_t* json_value) {
    sds json_str  = NULL;
    if (!value_is_map(json_value)) {
        miniDBServerLog(LOG_ERROR, "[fieldMetaFromJson] json_value is not object");
        goto error;
    }
    
    sds name_value = json_map_get_sds(json_value, FIELD_NAME);
    sds type_value = json_map_get_sds(json_value, FIELD_TYPE);
    int64_t offset_value = json_map_get_int64(json_value, FIELD_OFFSET);
    int64_t len_value = json_map_get_int64(json_value, FIELD_LEN);
    bool visible_value = json_map_get_bool(json_value, FIELD_VISIBLE);
    int64_t field_id_value = json_map_get_int64(json_value, FIELD_FIELD_ID);
    attr_type_enum type = attr_type_from_str(type_value);
    if (UNDEFINED == type) {
        miniDBServerLog(LOG_ERROR, "Got invalid field type. type=%s", type_value);
        goto error;
    }
    fieldMeta* meta = fieldMetaCreate(sds_dup(name_value), type, offset_value, len_value, visible_value, field_id_value);
    return meta;

error:
    json_str = json_to_sds(json_value);
    miniDBServerLog(LOG_ERROR, "[fieldMetaFromJson] Failed to deserialize field. json value=%s", json_str);
    sds_delete(json_str);
    return NULL;
}