#ifndef __LATTE_LOG_FIELD_META_H
#define __LATTE_LOG_FIELD_META_H

#include "sds/sds.h"
#include "attr_type.h"
#include <stdbool.h>
#include "value/value.h"
typedef struct fieldMeta {
    sds name;
    attr_type_enum attr_type;
    int attr_offset;
    int attr_len;
    bool visible;
    int field_id;
} fieldMeta;

fieldMeta* fieldMetaCreate(const char *name, attr_type_enum attr_type, int attr_offset, int attr_len, bool visible, int field_id);
fieldMeta* fieldMetaFromJson(value_t* v);

#endif