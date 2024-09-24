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
#include "buffer_pool_frame_manager.h"

typedef struct disk_buffer_pool_t disk_buffer_pool_t; 
typedef struct bpFileHeader
{
  int32_t buffer_pool_id;   //! buffer pool id
  int32_t page_count;       //! 当前文件一共有多少个页面
  int32_t allocated_pages;  //! 已经分配了多少个页面
  char    bitmap[0];        //! 页面分配位图, 第0个页面(就是当前页面)，总是1
} bpFileHeader;
// typedef struct buffer_pool_frame_manager_t {
//     latte_mutex_t* lock;
//     lruCache* frames;            //LruCache<FrameId, frame_t*,BPFrameIdHasher>  
//     memPoolSimple* allocator;    //memPoolSimple<Frame>
// } buffer_pool_frame_manager_t;
// buffer_pool_frame_manager_t* buffer_pool_frame_manager_new(char* name);
// frame_t* buffer_pool_frame_manager_get_internal(buffer_pool_frame_manager_t* bp, frame_id_t* frameId);
// frame_t* buffer_pool_frame_manager_alloc(buffer_pool_frame_manager_t* bp_manager, int buffer_pool_id, PageNum page_num);
// list_t* buffer_pool_frame_manager_find_list(buffer_pool_frame_manager_t* manager, int buffer_pool_id);
// int buffer_pool_frame_manager_delete(buffer_pool_frame_manager_t* manager, int buffer_pool_id, PageNum page_num, frame_t* frame);
// typedef int (*frame_purge)(void* self, frame_t* frame); 
// int buffer_pool_frame_manager_purge_frames(buffer_pool_frame_manager_t* manager, int count, frame_purge fun, void* self);

typedef struct buffer_pool_log_handler_t {
    disk_buffer_pool_t* buffer_pool;
    log_handler_t* log_handler;
} buffer_pool_log_handler_t;

buffer_pool_log_handler_t* buffer_pool_log_handler_new(disk_buffer_pool_t* pool, log_handler_t* log_handler);
int buffer_pool_log_handler_flush_page(buffer_pool_log_handler_t* handler, page_t* page);


typedef struct buffer_pool_manager_t {
    buffer_pool_frame_manager_t* frame_manager;
    double_write_buffer_t* dblwr_buffer;
    latte_mutex_t* lock;                          // 锁
    dict_t* buffer_pools;                         // Map<string, diskBufferPool>
    dict_t* id_to_buffer_pools;                   // Map<int, diskBufferPool>
    latteAtomic int32_t next_buffer_pool_id;    // 系统启动时，会打开所有的表，这样就可以知道当前系统最大的ID是多少了
} buffer_pool_manager_t;

buffer_pool_manager_t*  buffer_pool_manager_new();
int buffer_pool_manager_init(buffer_pool_manager_t* manager , double_write_buffer_t* dblwr_buffer);
int buffer_pool_manager_open_file(buffer_pool_manager_t* manager, log_handler_t* handler, char* file_name, disk_buffer_pool_t** pool);
disk_buffer_pool_t* buffer_pool_manager_get_buffer_pool(buffer_pool_manager_t* manager, int32_t id);

int buffer_pool_manager_flush_page(buffer_pool_manager_t* manager, frame_t* frame);
    


typedef struct disk_buffer_pool_t {
    buffer_pool_manager_t* bp_manger;
    buffer_pool_frame_manager_t* frame_manager;
    double_write_buffer_t* dblwr_manager;
    buffer_pool_log_handler_t* log_handler;
    int file_desc;
    int32_t buffer_pool_id;
    frame_t *hdr_frame;
    bpFileHeader *file_header;
    set_t* disposed_pages;
    sds file_name;
    latte_mutex_t* lock;
    latte_mutex_t* wr_lock;
} disk_buffer_pool_t;
disk_buffer_pool_t* disk_buffer_pool_empty();
disk_buffer_pool_t* disk_buffer_pool_new(buffer_pool_manager_t* manager, buffer_pool_frame_manager_t* frame_manager, double_write_buffer_t* dblwr_manager,
    log_handler_t* log_handler);
void disk_buffer_pool_delete(disk_buffer_pool_t* pool);
int disk_buffer_pool_unpin_page(disk_buffer_pool_t* pool, frame_t* frame);
int disk_buffer_pool_write_page(disk_buffer_pool_t* pool, PageNum page_num, page_t* page);
frame_t* disk_buffer_pool_get_this_page(disk_buffer_pool_t* pool, PageNum page_num);
int disk_buffer_pool_close_file(disk_buffer_pool_t* pool);
int disk_buffer_pool_flush_page(disk_buffer_pool_t* pool, frame_t* frame);
int disk_buffer_pool_purge_frame(disk_buffer_pool_t* pool, PageNum page_num, frame_t* buf);

#endif