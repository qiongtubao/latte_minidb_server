#ifndef __LATTE_MINIDB_INDEX_H
#define __LATTE_MINIDB_INDEX_H
#include "index_meta.h"
#include "field_meta.h"

typedef struct Index {
    indexMeta* index_meta;   ///< 索引的元数据
    fieldMeta* field_meta;   ///< 当前实现仅考虑一个字段的索引
} Index;

void index_init(Index* index, indexMeta* index_meta, fieldMeta* field_meta);

#endif
