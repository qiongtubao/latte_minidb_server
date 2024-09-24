#ifndef __LATTE_UTILS_MEM_POOL_H
#define __LATTE_UTILS_MEM_POOL_H

#include "mutex/mutex.h"
#include "sds/sds.h"
#include "list/list.h"
#include "set/set.h"



typedef struct memPoolSimple {
    latte_mutex_t* mutex;
    int             size;
    bool            dynamic;
    sds          name;
    list_t* pools;
    set_t*  used;
    list_t* frees;
    int   item_num_per_pool;
    int   class_size;
} memPoolSimple;

memPoolSimple* memPoolSimpleCreate();
void* memPoolSimple_alloc(memPoolSimple* simple);
void memPoolSimple_free(memPoolSimple* simple, void* result);

typedef struct memPoolItem {
    latte_mutex_t* mutex;
    sds name;
    bool dynamic;
    int size;
    int item_size;
    int item_num_per_pool;

    list_t* pools; //void*
    set_t* used;   //void*
    list_t* frees; //void*

} memPoolItem;
memPoolItem* memPoolItemCreate(const char* type);




#endif