#include "utils_mem_pool.h"
#include <assert.h>
#include "log.h"

memPoolSimple* memPoolSimpleCreate() {
    memPoolSimple* pool = zmalloc(sizeof(memPoolSimple));
    return pool;
}

int memPoolSimple_extend(memPoolSimple* simple) {
    if (simple->dynamic == false) {
        miniDBServerLog(LOG_ERROR, "Disable dynamic extend memory pool, but begin to extend, this->name:%s", simple->name);
        return -1;
    }

    latte_mutex_lock(simple->mutex);
    void* pool = zmalloc(simple->item_num_per_pool * simple->class_size);
    if (pool == NULL) {
        latte_mutex_unlock(simple->mutex);
        miniDBServerLog(LOG_ERROR, "Failed to extend memory pool, this->size:%d, item_num_per_pool:%d, this->name:%s.",
              simple->size, simple->item_num_per_pool, simple->name);
        return -1;
    }
    list_add_node_tail(simple->pools, pool);
    simple->size += simple->item_num_per_pool;
    for(int i =0 ; i< simple->item_num_per_pool;i++) {
        list_add_node_tail(simple->frees, pool + i * simple->class_size);
    }

    latte_mutex_unlock(simple->mutex);
    miniDBServerLog(LOG_INFO, "Extend one pool, this->size:%d, item_num_per_pool:%d, this->name:%s.",
           simple->size, simple->item_num_per_pool, simple->name);
    return 0;
}

void* memPoolSimple_alloc(memPoolSimple* simple) {
    void* result = NULL;
    latte_mutex_lock(simple->mutex);
    if (list_length(simple->frees) == 0) {
        if (simple->dynamic == false) {
            goto end;
        }
        if (memPoolSimple_extend(simple) < 0) {
            goto end;
        }
    }
    result = list_rpop(simple->frees);
    assert(set_add(simple->used, result) == 1);
end:
    latte_mutex_unlock(simple->mutex);
    return result;
}

void memPoolSimple_free(memPoolSimple* simple, void* result) {
    latte_mutex_lock(simple->mutex);
    int num = set_remove(simple->used, result); //0 删除失败 1表示删除成功
    if (num == 0) {
        //为了打印消息后结束提前解锁    这里可以写的优雅一点 以后改改
        latte_mutex_unlock(simple->mutex);
        miniDBServerLog(LOG_WARN, "No entry of %p in %s.", result, simple->name);
        // print_stacktrace();
        return;
    }
    list_add_node_tail(simple->frees,result);
    latte_mutex_unlock(simple->mutex);
    return;
}



memPoolItem* memPoolItemCreate(const char* type) {
    memPoolItem* item = zmalloc(sizeof(memPoolItem));
    item->name = sds_new(type);
    return item;
}

int memPoolItemExtend(memPoolItem* item) {
    if (item->dynamic == false) {
        miniDBServerLog(LOG_WARN,"Disable dynamic extend memory pool, but begin to extend, this->name:%s", item->name);
        return -1;
    }
    latte_mutex_lock(item->mutex);
    void* pool = zmalloc(item->item_size * item->item_num_per_pool);
    if (pool == NULL) {
        latte_mutex_unlock(item->mutex);
        miniDBServerLog(LOG_ERROR, "Failed to extend memory pool, this->size:%d, item_num_per_pool:%d, this->name:%s.",
            item->size,
            item->item_num_per_pool,
            item->name);
        return -1;
    }
    list_add_node_tail(item->pools, pool);
    item->size += item->item_num_per_pool;
    for (int i = 0; i < item->item_num_per_pool; i++) {
        char *point = (char *)pool + i * item->item_size;
        list_add_node_tail(item->frees, point);
    }
    latte_mutex_unlock(item->mutex);
    miniDBServerLog(LOG_ERROR, "Extend one pool, this->size:%d, item_size:%d, item_num_per_pool:%d, this->name:%s.",
      item->size,
      item->item_size,
      item->item_num_per_pool,
      item->name);
    return 0;
} 

void memPoolItemCleanup(memPoolItem* item) {
    if (list_length(item->pools) == 0) { //正常情况下应该有数据
        miniDBServerLog(LOG_WARN,"Begin to do cleanup, but there is no memory pool, this->name:%s!", item->name);
        return 0;
    }
    latte_mutex_lock(item->mutex);

    list_delete(item->used);
    list_delete(item->frees);
    item->size = 0;

    latte_iterator_t* it= list_get_latte_iterator(item->pools, 0);
    while(latte_iterator_has_next(it)) {
        void* pool = latte_iterator_next(it);
        zfree(pool);
    }
    latte_iterator_delete(it);

    list_delete(item->pools);
    latte_mutex_unlock(item->mutex);
    miniDBServerLog(LOG_INFO,"Successfully do cleanup, this->name:%s.", item->name);
}

int memPoolItemInit(memPoolItem* item, int item_size, bool dynamic, int pool_num, int item_num_per_pool) {
    if (list_length(item->pools) != 0) { //初始化时 应该没有数据
        miniDBServerLog(LOG_WARN,"Memory pool has been initialized, but still begin to be initialized, this->name:%s.", item->name);
        return 0;
    }
    if (item_size <= 0 || pool_num <= 0 || item_num_per_pool <= 0) {
        miniDBServerLog(LOG_ERROR,"Invalid arguments, item_size:%d, pool_num:%d, item_num_per_pool:%d, this->name:%s.",
        item_size, pool_num, item_num_per_pool, item->name);
        return -1;
    }
    item->item_size = item_size;
    item->item_num_per_pool = item_num_per_pool;
    item->dynamic = true;
    for (int i = 0; i < pool_num; i++) {
        if (memPoolItemExtend(item) < 0) {
            memPoolItemCleanup(item);
            return -1;
        }
    }
    item->dynamic  = dynamic;
    miniDBServerLog(LOG_INFO, "Extend one pool, this->size:%d, item_size:%d, item_num_per_pool:%d, this->name:%s.",
      item->size, item_size, item_num_per_pool, item->name);
    return 0;
}

