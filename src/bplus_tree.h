#ifndef __LATTE_BPLUS_TREE_H
#define __LATTE_BPLUS_TREE_H

#include "log_handler.h"
#include "disk_buffer_pool.h"
#include "attr_type.h"
#include "record.h"
#include "index.h"
#include "utils_mem_pool.h"
#include "attr_type.h"

typedef struct indexFileHeader {
    PageNum root_page;
    int32_t  internal_max_size;  ///< 内部节点最大的键值对数
    int32_t  leaf_max_size;      ///< 叶子节点最大的键值对数
    int32_t  attr_length;        ///< 键值的长度
    int32_t  key_length;         ///< attr length + sizeof(RID)
    AttrType attr_type;          ///< 键值的类型
} indexFileHeader;


typedef struct attrComparator {
    AttrType attr_type;
    int attr_length
} attrComparator;
attrComparator* attrComparatorCreate(AttrType attr_type, int attr_length);
int attrComparator_operator(attrComparator* c,const char* v1, const char* v2);

typedef struct keyComparator {
    attrComparator attr_comparator;
} keyComparator;
int keyComparator_operator(keyComparator* c, const char* v1, const char* v2);


typedef struct keyPrinter {
    AttrType attr_type;
    int attr_length;
} keyPrinter;
sds keyPrinter_operator(keyPrinter* printer, const char* v);

typedef struct bplusTreeHandler {
    Index supper;
    logHandler* log_handler;            //日志处理器
    diskBufferPool* disk_buffer_pool;   //磁盘缓冲池
    bool header_dirty;
    indexFileHeader* file_header;
    // sharedMutex root_lock;
    keyComparator key_comparator;
    keyPrinter key_printer;
    memPoolItem* mem_pool_item;
} bplusTreeHandler;
int bplus_tree_index_handler_open(bplusTreeHandler* handler, logHandler* log_handler, bufferPoolManager*  bpm, const char* file_name);
#endif