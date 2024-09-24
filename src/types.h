#ifndef __LATTE_TYPES_H
#define __LATTE_TYPES_H
#include <inttypes.h>
#include <stdint.h>
/// 磁盘文件，包括存放数据的文件和索引(B+-Tree)文件，都按照页来组织
/// 每一页都有一个编号，称为PageNum
#define PageNum int32_t

/// 数据文件中按照页来组织，每一页会存放一些行数据(row)，或称为记录(record)
/// 每一行(row/record)，都占用一个槽位(slot)，这些槽有一个编号，称为SlotNum
#define SlotNum int32_t

/// LSN for log sequence number
#define LSN int64_t


#define LSN_FORMAT PRId64


#define CheckSum unsigned int
/**
 * @brief 存储格式
 * @details 当前仅支持行存格式（ROW_FORMAT）以及 PAX 存储格式(PAX_FORMAT)。
 */
typedef enum storageFormat {
    UNKNOWN_FORMAT = 0,
    ROW_FORMAT,
    PAX_FORMAT
} storageFormat;

#endif