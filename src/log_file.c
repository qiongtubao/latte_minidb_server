#include "log_file.h"
#include "zmalloc/zmalloc.h"
#include "code.h"

logFileManager* logFileManagerCreate() {
    logFileManager* manager = zmalloc(sizeof(logFileManager));
    return manager;
}

int logFileManagerInit(logFileManager* manager, char* path, int max_entry_number) {
    manager->directory = sdsnew(path);
    manager->max_entry_number_per_file = max_entry_number;
    return SUCCESS;
}