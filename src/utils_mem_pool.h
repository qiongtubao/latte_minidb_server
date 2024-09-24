#ifndef __LATTE_UTILS_MEM_POOL_H
#define __LATTE_UTILS_MEM_POOL_H

#include "mutex/mutex.h"
#include "sds/sds.h"
#include "list/list.h"
#include "set/set.h"



typedef struct memPoolSimple {
    latte_mutex* mutex;
    int             size;
    bool            dynamic;
    sds          name;
    list* pools;
    set*  used;
    list* frees;
    int   item_num_per_pool;
    int   class_size;
} memPoolSimple;

memPoolSimple* memPoolSimpleCreate();
void* memPoolSimple_alloc(memPoolSimple* simple);
void memPoolSimple_free(memPoolSimple* simple, void* result);

typedef struct memPoolItem {
    latte_mutex* mutex;
    sds name;
    bool dynamic;
    int size;
    int item_size;
    int item_num_per_pool;

    list* pools; //void*
    set* used;   //void*
    list* frees; //void*

} memPoolItem;
memPoolItem* memPoolItemCreate(const char* type);




#endif