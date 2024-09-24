#ifndef __LATTE_BPLUS_TREE_INDEX_H
#define __LATTE_BPLUS_TREE_INDEX_H

#include <stdbool.h>
#include "table.h"
#include "bplus_tree.h"

typedef struct bplusTreeIndex {
    bool inited;
    table* table;
    bplusTreeHandler* index_handler;
} bplusTreeIndex;

bplusTreeIndex* bplusTreeIndexCreate();
void bplusTreeIndexRelese(bplusTreeIndex* index);

int bplusTreeIndexOpen(bplusTreeIndex* index, table* t, const char* file_name, indexMeta* index_meta, fieldMeta* field_meta);

#endif