
#include "value.h"

Value* ValueCreate() {
    Value* value = zmalloc(sizeof(Value));
    value->attr_type = UNDEFINED;
    value->own_data = false;
    return value;
}

void value_reset(Value* v) {
    switch (v->attr_type)
    {
    case CHARS:
        if (v->own_data && v->value.pointer_value != NULL) {
            sdsfree(v->value.pointer_value);
            v->value.pointer_value = NULL;
        }
        break;
    
    default:
        break;
    }

    v->attr_type = UNDEFINED;
    v->own_data = false;
}

void value_set_sds(Value* v, char* data, int len) {
    value_reset(v);
    v->attr_type = CHARS;
    if (data == NULL) {
        v->value.pointer_value = sdsempty();
    } else {
        v->own_data = true;
        if (len > 0) {
            len = strnlen(data, len);
        } else {
            len = strlen(data);
        }
        v->value.pointer_value = sdsnewlen(data, len);
        v->length = len;
    }
}

void value_set_data(Value* v, char* data, int length) {
    switch (v->attr_type)
    {
    case CHARS: {
        /* code */
        value_set_sds(v, data, length);
    } break;
    case INTS: {
        v->value.int_value = *(int*)data;
        v->length = length;
    } break;
    case FLOATS: {
       v->value.float_value = *(float*)data;
       v->length = length;
    } break;
    case BOOLEANS: {
        v->value.bool_value = *(int*)data != 0;
        v->length = length;
    } break;
    default:
        break;
    }
}