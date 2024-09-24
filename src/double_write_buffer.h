#ifndef __LATTE_DOUBLE_WRITE_BUFFER_H
#define __LATTE_DOUBLE_WRITE_BUFFER_H

#include "mutex/mutex.h"
#include "dict/dict.h"
#include "code.h"
#include "page.h"
#include "bitmap/bitmap.h"

typedef struct  disk_buffer_pool_t disk_buffer_pool_t;


typedef struct double_write_buffer_header_t {
    int32_t page_cnt;
} double_write_buffer_header_t;
struct double_write_buffer_header_t* double_write_buffer_header_new();

typedef struct double_write_buffer_t double_write_buffer_t;
typedef struct double_write_buffer_t {
    int (*add_page)(double_write_buffer_t* dwb, struct disk_buffer_pool_t* bp, PageNum page_num, page_t* age);
    int (*read_page)(double_write_buffer_t* dwb, struct disk_buffer_pool_t* bp, PageNum page_num, page_t* age);
    int (*clear_pages)(double_write_buffer_t* dwb, struct disk_buffer_pool_t* bp);
} double_write_buffer_t;

typedef struct disk_double_write_buffer_t {
    double_write_buffer_t supper;
    int file_desc;
    int max_pages;
    latte_mutex_t* lock;
    struct buffer_pool_manager_t* bp_manager;
    double_write_buffer_header_t* header;
    dict_t* dblwr_pages;
} disk_double_write_buffer_t;


typedef struct double_write_page_key_t {
    int32_t buffer_pool_id;
    PageNum page_num;
} double_write_page_key_t;
double_write_page_key_t* double_write_page_key_new(int32_t buffer_pool_id, int32_t page_num);
void double_write_page_key_delete(double_write_page_key_t* key);

typedef struct double_write_page_t {
    double_write_page_key_t key;
    int32_t page_index;
    bool valid;
    page_t* page;
} double_write_page_t;
double_write_page_t* double_write_page_new();
void double_write_page_delete(double_write_page_t* page);

static const int32_t DOUBLE_WRITE_BUFFER_HEADER_SIZE = sizeof(double_write_buffer_header_t);
static const int32_t DOUBLE_WRITE_PAGE_SIZE = sizeof(double_write_page_t);



struct disk_double_write_buffer_t* disk_double_write_buffer_new(struct buffer_pool_manager_t* manager);
int disk_double_write_buffer_open_file(disk_double_write_buffer_t* buffer,const char* file_path);
int disk_double_write_buffer_add_page(disk_double_write_buffer_t* buffer, disk_buffer_pool_t* bp, PageNum page_num, page_t* page);
int disk_double_write_buffer_write_page_internal(disk_double_write_buffer_t* buffer, double_write_page_t* page);
int disk_double_write_buffer_clear_pages(disk_double_write_buffer_t* buffer, disk_buffer_pool_t* buffer_pool);
typedef struct buffer_pool_iterator_t {
    bitmap_t bit_map;
    PageNum current_page_num;
} buffer_pool_iterator_t;
#endif