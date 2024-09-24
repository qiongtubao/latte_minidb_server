#ifndef __LATTE_INDEX_META_H
#define __LATTE_INDEX_META_H

#include "sds/sds.h"
#include "value/value.h"


const static char* FIELD_NAME = "name";
const static char* FIELD_FIELD_NAME = "field_name";
typedef struct indexMeta {
    sds name;
    sds field;
} indexMeta;

indexMeta* indexMetaCreate();

#endif