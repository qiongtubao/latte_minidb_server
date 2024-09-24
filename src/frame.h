#ifndef __LATTE_MINIDB_FRAME_H
#define __LATTE_MINIDB_FRAME_H


#include "utils/atomic.h"
#include "dict/dict.h"
#include <stdbool.h>
#include "page.h"
#include "mutex/mutex.h"

typedef struct frameId {
    int buffer_pool_id;
    PageNum page_num;
} frameId;

frameId* frameIdCreate(int buffer_pool_id, int32_t page_num);
void frameIdRelease(frameId* id);


/**
 * @brief 页帧
 * @ingroup BufferPool
 * @details 页帧是磁盘文件在内存中的表示。磁盘文件按照页面来操作，操作之前先映射到内存中，
 * 将磁盘数据读取到内存中，也就是页帧。
 *
 * 当某个页面被淘汰时，如果有些内容曾经变更过，那么就需要将这些内容刷新到磁盘上。这里有
 * 一个dirty标识，用来标识页面是否被修改过。
 *
 * 为了防止在使用过程中页面被淘汰，这里使用了pin count，当页面被使用时，pin count会增加，
 * 当页面不再使用时，pin count会减少。当pin count为0时，页面可以被淘汰。
 */
typedef struct Frame {
    bool dirty;
    latteAtomic int pin_count;
    unsigned long acc_time;
    frameId* frame_id;
    Page* page;
    // recursiveSharedMutex* lock;
    latte_mutex* debug_lock;
    intptr_t write_locker;
    int write_recursive_count;
    dict* read_lockers; //map<intptr_t, int>
} Frame;

Frame* frameCreate();
void frame_pin(Frame* frame) ;
int frame_unpin(Frame* frame);
void frameRelease(Frame* frame);
void frameSetCheckSum(Frame* frame, CheckSum checksum);
void frameClearDirty(Frame* frame);


#endif