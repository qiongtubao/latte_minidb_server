#ifndef __LATTE_UTILS_H
#define __LATTE_UTILS_H
#include "sds/sds.h"
#include <stdbool.h>
sds getAbsolutePath(char *filename);
bool is_blank(const char* str);
#endif