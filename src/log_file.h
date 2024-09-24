#ifndef __LATTE_LOG_FILE_H
#define __LATTE_LOG_FILE_H

#include "sds/sds.h"

typedef struct logFileManager {
    sds directory;
    int max_entry_number_per_file;
} logFileManager;

logFileManager* logFileManagerCreate();
int logFileManagerInit(logFileManager* manager, char* path, int max_entry_number);


#endif