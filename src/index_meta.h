#ifndef __LATTE_INDEX_META_H
#define __LATTE_INDEX_META_H

#include "sds/sds.h"

typedef struct indexMeta {
    sds name;
    sds field;
} indexMeta;

indexMeta* indexMetaCreate();

#endif