#include "field_meta.h"
#include "zmalloc/zmalloc.h"

fieldMeta* fieldMetaCreate(const char *name, AttrType attr_type, int attr_offset, int attr_len, bool visible, int field_id) {
    fieldMeta* meta = zmalloc(sizeof(fieldMeta));
    meta->name = sdsnew(name);
    meta->attr_type = attr_type;
    meta->attr_offset = attr_offset;
    meta->attr_len = attr_len;
    meta->visible = visible;
    meta->field_id = field_id;
    return meta;
}