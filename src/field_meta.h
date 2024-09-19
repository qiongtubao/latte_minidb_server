#ifndef __LATTE_LOG_FIELD_META_H
#define __LATTE_LOG_FIELD_META_H

#include "sds/sds.h"
#include "attr_type.h"
#include <stdbool.h>
typedef struct fieldMeta {
    sds name;
    AttrType attr_type;
    int attr_offset;
    int attr_len;
    bool visible;
    int field_id;
} fieldMeta;

fieldMeta* fieldMetaCreate(const char *name, AttrType attr_type, int attr_offset, int attr_len, bool visible, int field_id);

#endif