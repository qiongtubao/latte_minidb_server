#include "attr_type.h"
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
const char *ATTR_TYPE_NAME[] = {"undefined", "chars", "ints", "floats", "booleans"};
attr_type_enum attr_type_from_str(const char *s) {
    for (unsigned int i = 0; i < sizeof(ATTR_TYPE_NAME) / sizeof(ATTR_TYPE_NAME[0]); i++) {
        if (0 == strcasecmp(ATTR_TYPE_NAME[i], s)) {
            return (attr_type_enum)i;
        }
    }
    return UNDEFINED;
}

const char* attr_type_to_str(attr_type_enum type) {
    if (type >= UNDEFINED && type < MAXTYPE) {
        return ATTR_TYPE_NAME[(int)(type)];
    }
    return "unknown";
}

value_type_enum attr_type_to_value_type(attr_type_enum type) {
    switch (type)
    {
    case CHARS:
        return VALUE_SDS;
        break;
    case INTS:
        return VALUE_INT;
    case FLOATS:
        return VALUE_DOUBLE;
    case BOOLEANS:
        return VALUE_BOOLEAN;
    default:
        return VALUE_UNDEFINED;
        break;
    }

}