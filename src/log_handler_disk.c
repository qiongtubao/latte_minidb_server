#include "log_handler_disk.h"
#include "code.h"
#include "log_file.h"

int diskLogHandlerInit(diskLogHandler* handler,const char* path) {
    return logFileManagerInit(handler->file_manager, path, 1000);
}

diskLogHandler* diskLogHandlerCreate() {
    diskLogHandler* handler = zmalloc(sizeof(diskLogHandler));
    handler->file_manager = logFileManagerCreate();   
    return handler;
}