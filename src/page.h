#ifndef __LATTE_PAGE_H

#define __LATTE_PAGE_H

#include "types.h"

static const PageNum BP_INVALID_PAGE_NUM = -1;

static const PageNum BP_HEADER_PAGE = 0;
// 页面大小定义
static const int BP_PAGE_SIZE = (1 << 13);   // 页面大小为 8192 字节


// 已知具体的类型大小
static const int PageNumSize = sizeof(PageNum);
static const int LSNSize = sizeof(LSN);
static const int CheckSumSize = sizeof(CheckSum);

// 使用 const 定义常量
#define BP_PAGE_DATA_SIZE  (BP_PAGE_SIZE - sizeof(PageNum) - sizeof(LSN) - sizeof(CheckSum))

typedef struct Page {
    LSN lsn;
    CheckSum check_sum;
    char data[];
} Page;

Page* pageCreate();

#endif /* __LATTE_PAGE_H */