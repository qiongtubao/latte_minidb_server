
#ifndef __LATTE_RECORD_H
#define __LATTE_RECORD_H

#include "types.h"

typedef struct RID {
    PageNum page_num;
    SlotNum slot_num;
} RID;


#endif