#include "log_handler.h"
#include "utils.h"
#include <string.h>
#include <strings.h>

/**
 * 暂时实现上分2个实现 一个disk 和 vacuous  其实可忽略vacuous
 */
log_handler_t* log_handler_new(const char* name) {
    if (name == NULL || is_blank(name)) {
        name = "vacuous";
    }

    if (strcasecmp(name, "disk") == 0) {
        return disk_log_handler_new();
    } else if (strcasecmp(name, "vacuous") == 0) {
        return vacuous_log_handler_new();
    } 
    return NULL;
}