#ifndef __LATTE_LOG_ENTRY_H
#define __LATTE_LOG_ENTRY_H

#include "types.h"
#include "sds/sds.h"

typedef struct logHeader {
    LSN lsn;
    int32_t size;
    int32_t module_id;
} logHeader;

typedef struct logEntry {
    logHeader header;   //日志头
    sds data;           //日志数据 vector<char> 数组
} logEntry;

void logHeaderInit(logHeader* header);
logEntry* logEntryCreate();



#endif