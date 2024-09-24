#include "utils_lru_cache.h"
#include <assert.h>
#include "zmalloc/zmalloc.h"

dict_func_t lruCacheDictType = {
    NULL, //dictListNodeHash
    NULL,
    NULL,
    NULL, //dictListNodeCompare
    NULL
};


lruCache* lruCacheCreate(int size) {
    lruCache* cache = zmalloc(sizeof(lruCache));
    cache->list.head = cache->list.tail = NULL;
    cache->list.len = 0;
    cache->list.dup = NULL;
    cache->list.free = NULL;
    cache->list.match = NULL;
    //TODO 需要限制个数
    cache->searcher = dict_new(&lruCacheDictType);
    return cache;
}

size_t lruCache_count(lruCache* cache) {
    return dict_size(cache->searcher);
}



void* lruCache_get(lruCache* cache, void* key) {
    dict_entry_t* node = dict_find(cache->searcher, key);
    if (node == NULL) {
        return NULL;
    }
    list_node_t* ln = dict_get_val(node);
    // lru_touch(cacahe, ln);
    list_move_head(cache, ln);
    return ((keyValue*)(list_node_value(ln)))->value;
}


int lruCache_put(lruCache* cache, void* key, void* value) {
    dict_entry_t* node = dict_find(cache->searcher, key);
    if (node != NULL) {
        list_node_t* ln = dict_get_val(node);
        ((keyValue*)(list_node_value(ln)))->value = value;
        list_move_head(cache, ln);
        return 0;
    }
    list_add_node_head(cache, keyValueCreate(key, value));
    assert(DICT_OK == dict_add(cache->searcher, key, cache->list.head));
    return 1;
}

void lruCache_remove(lruCache* cache, void* key) {
    dict_entry_t* node = dict_find(cache->searcher, key);
    if (node != NULL) {
        list_del_node(cache, dict_get_entry_val(node));
        dict_delete_key(cache->searcher, key);
    }
}

latte_iterator_t* lruCache_getIterator(lruCache* cache) {
    return list_get_latte_iterator(cache, 0);
}

keyValue* keyValueCreate(void* key, void* value) {
    keyValue* kv = zmalloc(sizeof(keyValue));
    kv->key = key;
    kv->value = value;
    return kv;
}
void keyValueRelease(keyValue* kv) {
    zfree(kv);
}