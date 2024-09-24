#include "bplus_tree.h"
#include "log.h"
#include "code.h"
#include <string.h>
#include "value.h"
#include <assert.h>

int bplus_tree_index_handler_close(bplusTreeHandler* handler) {
    if (handler->disk_buffer_pool != NULL) {
        diskBufferPoolCloseFile(handler->disk_buffer_pool);
    }
    handler->disk_buffer_pool  = NULL;
    return SUCCESS;
}
#define FIRST_INDEX_PAGE 1
int bplus_tree_index_handler_open_dbp(bplusTreeHandler* handler, logHandler* log_handler, diskBufferPool* disk_buffer_pool) {
    if (handler->disk_buffer_pool != NULL) {
        miniDBServerLog(LOG_WARN, "b+ tree has been opened before index.open.");
        return RECORD_OPENNED;
    }
    int rc = SUCCESS;

    Frame *frame= diskBufferPoolGetThisPage(disk_buffer_pool,FIRST_INDEX_PAGE);
    if (frame == NULL) {
        miniDBServerLog(LOG_WARN,"Failed to get first page, rc=%d", rc);
        return rc;
    }

    char* pdata = frame->page->data;
    //?
    memcpy(handler->file_header, pdata, sizeof(indexFileHeader));
    handler->header_dirty = false;
    handler->disk_buffer_pool = disk_buffer_pool;
    handler->log_handler = log_handler;
    
    handler->mem_pool_item = memPoolItemCreate("b+tree");
    if (memPoolItemInit(handler->mem_pool_item, handler->file_header->key_length) < 0) {
        miniDBServerLog(LOG_WARN, "Failed to init memory pool for index");
        bplus_tree_index_handler_close(handler);
        return NOMEM;
    }
    disBufferPoolUnpinPage(handler->disk_buffer_pool, frame);
    handler->key_comparator.attr_comparator.attr_type = handler->file_header->attr_type;
    handler->key_comparator.attr_comparator.attr_length = handler->file_header->attr_length;

    handler->key_printer.attr_type = handler->file_header->attr_type;
    handler->key_printer.attr_length = handler->file_header->attr_length;

    miniDBServerLog(LOG_INFO, "Successfully open index");
    return rc;
}
int bplus_tree_index_handler_open(bplusTreeHandler* handler, logHandler* log_handler, bufferPoolManager*  bpm, const char* file_name) {
    if (handler->disk_buffer_pool != NULL) {
        miniDBServerLog(LOG_WARN, "%s has been opened before index.open.", file_name);
        return RECORD_OPENNED;
    }

    diskBufferPool* disk_buffer_pool;
    int rc = bufferPoolManagerOpenFile(bpm, log_handler, file_name, &disk_buffer_pool);
    if (isRcFail(rc)) {
        diskBufferPoolRelease(disk_buffer_pool);
        miniDBServerLog(LOG_WARN, "Failed to open file name=%s, rc=%d", file_name, rc);
        return rc;
    }

    rc = bplus_tree_index_handler_open_dbp(handler, log_handler, disk_buffer_pool);
    if (!isRcFail(rc)) {
        miniDBServerLog(LOG_INFO, "open b+tree success. filename=%s", file_name);
    }
    return rc;
}

attrComparator* attrComparatorCreate(AttrType attr_type, int attr_length) {
    attrComparator* comparator =  zmalloc(sizeof(attrComparator));
    comparator->attr_type = attr_type;
    comparator->attr_length = attr_length;
    return comparator;
}



typedef struct DataType {
    AttrType attr_type;
    /**
     * @return
     *  -1 表示 left < right
     *  0 表示 left = right
     *  1 表示 left > right
     *  INT32_MAX 表示未实现的比较
     */
    int (*compare)(Value left, Value right);
} DataType;


int undefined_compare(Value left, Value right) {
    assert(left.attr_type == UNDEFINED
        && right.attr_type == UNDEFINED);
    return INT32_MAX; 
}

int charType_compare(Value left, Value right) {
    assert(left.attr_type == CHARS && 
        right.attr_type == CHARS);
        //这里小心点 如果未来出现问题的话 可能是 sds 使用的memcmp比较的   而原代码是使用strncmp比较的
    return sdscmp(left.value.pointer_value, right.value.pointer_value);
}

#define EPSILON (1E-6)
int compare_float(float v1, float v2) {
    float cmp = v1 - v2;
    return cmp > EPSILON? 1: (cmp < -EPSILON? -1: 0);
}
int intsType_compare(Value left, Value right) {
    assert(left.attr_type == INTS && 
        (right.attr_type == INTS || right.attr_type == FLOATS) );
    
    if (right.attr_type == INTS) {
        int v1 =  left.value.int_value;
        int v2 =  right.value.int_value;
        return v1 > v2 ? 1: (v1 < v2? -1 :0); 
    } else if (right.attr_type == FLOATS) {
        //误差在1E-6以内都算相等
        float v1 = (float)left.value.int_value;
        float v2 = right.value.float_value;
        return compare_float(v1, v2);
    }
    return INT32_MAX;
}


int floatsType_compare(Value left, Value right) {
    assert(left.attr_type == FLOATS && 
        right.attr_type == FLOATS || right.attr_type == INTS);

    float v1 = left.value.float_value;
    if (right.attr_type == FLOATS) {
        float v2 = right.value.float_value;
        return compare_float(v1, v2);
    } else if (right.attr_type == INTS) {
        float v2 = (float)right.value.int_value;
        return compare_float(v1, v2);
    }
    return INT32_MAX;
}


DataType type_instances[5] = {
    {
        .attr_type = UNDEFINED, //DataType(UNDEFINED)
        .compare = undefined_compare
    },
    {
        .attr_type = CHARS,  //CharType
        .compare = charType_compare
    },
    {
        .attr_type = INTS,  //IntegerType
        .compare = intsType_compare
    },
    {
        .attr_type = FLOATS,  //IntegerType
        .compare = floatsType_compare
    },
    {
        .attr_type = BOOLEANS,  //DataType(BOOLEANS)
        .compare = undefined_compare    //返回未实现的比较  （暂时还不好确定  true和flase 谁大谁小）
    }

};

int attrComparator_operator(attrComparator* c,const char* v1, const char* v2) {
    Value left;
    left.attr_type = c->attr_type;
    value_set_data(&left, v1, c->attr_length);
    Value right;
    right.attr_type = c->attr_type;
    value_set_data(&right, v2, c->attr_length);

    DataType type = type_instances[c->attr_type];
    return type.compare(left, right);
}

