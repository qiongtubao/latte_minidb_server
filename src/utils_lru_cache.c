#include "utils_lru_cache.h"
#include <assert.h>
#include "zmalloc/zmalloc.h"

dictType lruCacheDictType = {
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
    cache->searcher = dictCreate(&lruCacheDictType);
    return cache;
}

size_t lruCache_count(lruCache* cache) {
    return dictSize(cache->searcher);
}



void* lruCache_get(lruCache* cache, void* key) {
    dictEntry* node = dictFind(cache->searcher, key);
    if (node == NULL) {
        return NULL;
    }
    listNode* ln = dictGetVal(node);
    // lru_touch(cacahe, ln);
    listMoveHead(cache, ln);
    return ((keyValue*)(listNodeValue(ln)))->value;
}


int lruCache_put(lruCache* cache, void* key, void* value) {
    dictEntry* node = dictFind(cache->searcher, key);
    if (node != NULL) {
        listNode* ln = dictGetVal(node);
        ((keyValue*)(listNodeValue(ln)))->value = value;
        listMoveHead(cache, ln);
        return 0;
    }
    listAddNodeHead(cache, keyValueCreate(key, value));
    assert(DICT_OK == dictAdd(cache->searcher, key, cache->list.head));
    return 1;
}

void lruCache_remove(lruCache* cache, void* key) {
    dictEntry* node = dictFind(cache->searcher, key);
    if (node != NULL) {
        listDelNode(cache, dictGetVal(node));
        dictDelete(cache->searcher, key);
    }
}

Iterator* lruCache_getIterator(lruCache* cache) {
    return listGetLatteIterator(cache, 0);
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