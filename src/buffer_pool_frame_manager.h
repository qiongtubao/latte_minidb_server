#ifndef __LATTE_BUFFER_POOL_FRAME_MANAGER_H
#define __LATTE_BUFFER_POOL_FRAME_MANAGER_H
#include "mutex/mutex.h"
#include "utils_lru_cache.h"
#include "utils_mem_pool.h"

#include "frame.h"

/**
 *  lock 
 *  lru 
 *  mem_pool_simple => 
 */
typedef struct buffer_pool_frame_manager_t {
    latte_mutex_t* lock;
    lruCache* frames;            //LruCache<FrameId, frame_t*,BPFrameIdHasher>  
    memPoolSimple* allocator;    //memPoolSimple<Frame>
} buffer_pool_frame_manager_t;
buffer_pool_frame_manager_t* buffer_pool_frame_manager_new();


frame_t* buffer_pool_frame_manager_get(buffer_pool_frame_manager_t* bp, int buffer_pool_id, PageNum page_num);
frame_t* buffer_pool_frame_manager_get_internal(buffer_pool_frame_manager_t* bp, frame_id_t* frameId);;
frame_t* buffer_pool_frame_manager_alloc(buffer_pool_frame_manager_t* bp_manager, int buffer_pool_id, PageNum page_num);
list_t* buffer_pool_frame_manager_find_list(buffer_pool_frame_manager_t* manager, int buffer_pool_id);
int buffer_pool_frame_manager_delete(buffer_pool_frame_manager_t* manager, int buffer_pool_id, PageNum page_num, frame_t* frame);
typedef int (*frame_purge)(void* self, frame_t * frame); 
int buffer_pool_frame_manager_purge_frames(buffer_pool_frame_manager_t* manager, int count, frame_purge fun, void* self);


#endif