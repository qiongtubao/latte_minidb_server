#include "table_meta.h"

indexMeta* table_meta_get_index_by_index(tableMeta* t, int index) {
    listNode* node = listIndex(t->indexes, index);
    if (node == NULL) return NULL;
    return (indexMeta*)node->value;
}

fieldMeta* table_meta_get_field_by_name(tableMeta* t, sds name) {
    if (name == NULL) {
        return NULL;
    }
    listIter* iter =  listGetIterator(t->fields,AL_START_HEAD);
    listNode* node = NULL;
    fieldMeta* meta = NULL;
    while ((node = listNext(iter)) != NULL) {
        fieldMeta* field = listNodeValue(node);
        if (0 == strcmp(field->name, name)) {
            meta  = field;
            goto end;
        }
    }
end: 
    listReleaseIterator(iter);
    return meta;

}