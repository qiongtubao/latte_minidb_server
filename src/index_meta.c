#include "index_meta.h"

indexMeta* indexMetaCreate() {
    indexMeta* meta = zmalloc(sizeof(indexMeta));
    return meta;
}
