#include "log_handler_vacuous.h"
#include "code.h"

int vacuous_log_handler_init(vacuous_log_handler_t* handler, const char* path) {
    return SUCCESS;
}
vacuous_log_handler_t* vacuous_log_handler_new() {
    vacuous_log_handler_t* handler = zmalloc(sizeof(vacuous_log_handler_t)); 
    handler->supper.init =  vacuous_log_handler_init; 
    return handler;
}