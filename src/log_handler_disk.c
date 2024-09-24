#include "log_handler_disk.h"
#include "code.h"
#include "log_file.h"

int disk_log_handler_init(disk_log_handler_t* handler,const char* path) {
    return logFileManagerInit(handler->file_manager, path, 1000);
}

disk_log_handler_t* disk_log_handler_new() {
    disk_log_handler_t* handler = zmalloc(sizeof(disk_log_handler_t));
    handler->file_manager = logFileManagerCreate();  
    handler-> supper.init = disk_log_handler_init;
    return handler;
}