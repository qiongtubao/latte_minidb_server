#ifndef __LATTE_VALUE_H
#define __LATTE_VALUE_H

#include "attr_type.h"
#include "sds/sds.h"
#include "stdbool.h"

typedef struct Value {
    AttrType attr_type;
    int length;
    union  {
        int32_t int_value;
        float float_value;
        bool bool_value;
        sds pointer_value;
    } value;
    /// 是否申请并占有内存, 目前对于 CHARS 类型 own_data_ 为true, 其余类型 own_data_ 为false
    bool own_data;
} Value;

Value* ValueCreate();
void value_reset(Value* v);
void value_set_sds(Value* v, char* data, int len);
void value_set_data(Value* v, char* data, int length);
#endif