#ifndef __LATTE_DOUBLE_WRITE_BUFFER_H
#define __LATTE_DOUBLE_WRITE_BUFFER_H

#include "mutex/mutex.h"
#include "dict/dict.h"
#include "code.h"
#include "page.h"

typedef struct  diskBufferPool diskBufferPool;


typedef struct doubleWriteBufferHeader {
    int32_t page_cnt;
} doubleWriteBufferHeader;

typedef struct doubleWriteBuffer doubleWriteBuffer;
typedef struct doubleWriteBuffer {
    int (*add_page)(doubleWriteBuffer* dwb, struct diskBufferPool* bp, PageNum page_num, Page* age);
    int (*read_page)(doubleWriteBuffer* dwb, struct diskBufferPool* bp, PageNum page_num, Page* age);
    int (*clear_pages)(doubleWriteBuffer* dwb, struct diskBufferPool* bp);
} doubleWriteBuffer;

typedef struct diskDoubleWriteBuffer {
    doubleWriteBuffer supper;
    int file_desc;
    int max_pages;
    latte_mutex* lock;
    struct bufferPoolManager* bp_manager;
    doubleWriteBufferHeader* header;
    dict* dblwr_pages;
} diskDoubleWriteBuffer;


typedef struct doubleWritePageKey {
    int32_t buffer_pool_id;
    PageNum page_num;
} doubleWritePageKey;
doubleWritePageKey* doubleWritePageKeyCreate(int32_t buffer_pool_id, int32_t page_num);
void doubleWritePageKeyRelease(doubleWritePageKey* key);

typedef struct doubleWritePage {
    doubleWritePageKey key;
    int32_t page_index;
    bool valid;
    Page* page;
} doubleWritePage;
doubleWritePage* doubleWritePageCreate();

static const int32_t doubleWriteBufferHeader_SIZE = sizeof(doubleWriteBufferHeader);
static const int32_t doubleWritePage_SIZE = sizeof(doubleWritePage);

struct doubleWriteBufferHeader* doubleWriteBufferHeaderCreate();
struct diskDoubleWriteBuffer* diskDoubleWriteBufferCreate(struct bufferPoolManager* manager);
int diskDoubleWriteBufferOpenFile(diskDoubleWriteBuffer* buffer,const char* file_path);
int doubleWriteBufferAddPage(diskDoubleWriteBuffer* buffer, diskBufferPool* bp, PageNum page_num, Page* page);
int doubleWriteBufferWritePageInternal(diskDoubleWriteBuffer* buffer, doubleWritePage* page);

#endif