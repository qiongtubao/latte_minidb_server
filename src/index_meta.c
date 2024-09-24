#include "index_meta.h"

indexMeta* indexMetaCreate() {
    indexMeta* meta = zmalloc(sizeof(indexMeta));
    return meta;
}

//index meta to json
value_t* index_meta_to_json() {
    (void)FIELD_FIELD_NAME;
    (void)FIELD_NAME;
    return NULL;
}

