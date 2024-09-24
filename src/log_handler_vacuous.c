#include "log_handler_vacuous.h"
#include "code.h"

int vacuousLogHandlerInit(vacuousLogHandler* handler, const char* path) {
    return SUCCESS;
}
vacuousLogHandler* vacuousLogHandlerCreate() {
    vacuousLogHandler* handler = zmalloc(sizeof(vacuousLogHandler)); 
    handler->supper.init =  vacuousLogHandlerInit; 
    return handler;
}