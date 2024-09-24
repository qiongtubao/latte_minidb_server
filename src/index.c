#include "index.h"

void index_init(Index* index, indexMeta* index_meta, fieldMeta* field_meta) {
    index->index_meta = index_meta;
    index->field_meta = field_meta;
}