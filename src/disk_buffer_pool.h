#ifndef __LATTE_DISK_BUFFER_POOL_H
#define __LATTE_DISK_BUFFER_POOL_H

#include "mutex/mutex.h"
#include "dict/dict.h"
#include "utils/atomic.h"
#include "utils/atomic.h"
#include "frame.h"
#include "sds/sds.h"
#include "set/set.h"
#include "log_handler.h"
#include "utils_lru_cache.h"
#include "utils_mem_pool.h"
#include "double_write_buffer.h"

typedef struct diskBufferPool diskBufferPool; 
typedef struct bpFileHeader
{
  int32_t buffer_pool_id;   //! buffer pool id
  int32_t page_count;       //! 当前文件一共有多少个页面
  int32_t allocated_pages;  //! 已经分配了多少个页面
  char    bitmap[0];        //! 页面分配位图, 第0个页面(就是当前页面)，总是1
} bpFileHeader;
typedef struct bpFrameManager {
    latte_mutex* lock;
    lruCache* frames;            //LruCache<FrameId, Frame*,BPFrameIdHasher>  
    memPoolSimple* allocator;    //memPoolSimple<Frame>
} bpFrameManager;
Frame* bp_frame_manager_get_internal(bpFrameManager* bp, frameId* frameId);
Frame* bp_frame_manager_alloc(bpFrameManager* bp_manager, int buffer_pool_id, PageNum page_num);
list* bpFrameManagerFindList(bpFrameManager* manager, int buffer_pool_id);
int bpFrameManagerFree(bpFrameManager* manager, int buffer_pool_id, PageNum page_num, Frame* frame);
typedef int (*frame_purge)(void* self, Frame * frame); 
int bpFrameManagerPurgeFrames(bpFrameManager* manager, int count, frame_purge fun, void* self);

typedef struct bufferPoolLogHandler {
    diskBufferPool* buffer_pool;
    logHandler* log_handler;
} bufferPoolLogHandler;

bufferPoolLogHandler* bufferPoolLogHandlerCreate(diskBufferPool* pool, logHandler* log_handler);
int bufferPoolLogHandlerFlushPage(bufferPoolLogHandler* handler, Page* page);


typedef struct bufferPoolManager {
    bpFrameManager* frame_manager;
    doubleWriteBuffer* dblwr_buffer;
    latte_mutex* lock;                          // 锁
    dict* buffer_pools;                         // Map<string, diskBufferPool>
    dict* id_to_buffer_pools;                   // Map<int, diskBufferPool>
    latteAtomic int32_t next_buffer_pool_id;    // 系统启动时，会打开所有的表，这样就可以知道当前系统最大的ID是多少了
} bufferPoolManager;

bufferPoolManager*  bufferPoolManagerCreate();
int bufferPoolManagerInit(bufferPoolManager* manager , doubleWriteBuffer* dblwr_buffer);
int bufferPoolManagerOpenFile(bufferPoolManager* manager, logHandler* handler, char* file_name, diskBufferPool** pool);
diskBufferPool * bufferPoolManagerGetBufferPool(bufferPoolManager* manager, int32_t id);

int bufferPoolManagerFlushPage(bufferPoolManager* manager, Frame* frame);
    


typedef struct diskBufferPool {
    bufferPoolManager* bp_manger;
    bpFrameManager* frame_manager;
    doubleWriteBuffer* dblwr_manager;
    bufferPoolLogHandler* log_handler;
    int file_desc;
    int32_t buffer_pool_id;
    Frame *hdr_frame;
    bpFileHeader *file_header;
    set* disposed_pages;
    sds file_name;
    latte_mutex* lock;
    latte_mutex* wr_lock;
} diskBufferPool;
diskBufferPool* diskBufferPoolEmpty();
diskBufferPool* diskBufferPoolCreate(bufferPoolManager* manager, bpFrameManager* frame_manager, doubleWriteBuffer* dblwr_manager,
    logHandler* log_handler);
void diskBufferPoolRelease(diskBufferPool* pool);
int disBufferPoolUnpinPage(diskBufferPool* pool, Frame* frame);
int diskBufferPoolWritePage(diskBufferPool* pool, PageNum page_num, Page* page);
Frame* diskBufferPoolGetThisPage(diskBufferPool* pool, PageNum page_num);
int diskBufferPoolCloseFile(diskBufferPool* pool);
int diskBufferPoolFlushPage(diskBufferPool* pool, Frame* frame);
int diskBufferPoolPurgeFrame(diskBufferPool* pool, PageNum page_num, Frame* buf);
#endif