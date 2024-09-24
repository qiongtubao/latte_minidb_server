#include "buffer_pool_frame_manager.h"

buffer_pool_frame_manager_t* buffer_pool_frame_manager_new() {
    buffer_pool_frame_manager_t* manager = zmalloc(sizeof(buffer_pool_frame_manager_t));
    manager->lock = latte_recursive_mutex_new();
    manager->frames = lruCacheCreate(0);
    manager->allocator = memPoolSimpleCreate();
    return manager;
}