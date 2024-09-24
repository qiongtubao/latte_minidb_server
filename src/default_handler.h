#ifndef __LATTE_DEFAULT_HANDLER_H
#define __LATTE_DEFAULT_HANDLER_H

#include "dict/dict.h"
#include "sds/sds.h"
#include "db.h"
#include "table.h"
typedef struct dbHandler {
    sds base_dir;           //存储引擎的根目录
    sds db_dir;             //数据库文件的根目录
    sds trx_kit_name;       //事务模型的名称
    sds log_handler_name;   //日志处理器的名称
    dict* opened_dbs;       //打开数据库 <sds, db*>
} dbHandler;
dbHandler* defaultHandlerCreate();
int init_default_handler(dbHandler* handler, const char* base_dir, const char* trx_kit_name, const char *log_handler_name);
void destroy_default_handler();

int handler_create_db(dbHandler* handler, const char* dbname);
int drop_db(dbHandler* handler, const char* dbname);
int handler_open_db(dbHandler* handler, const char* dbname);
int close_db(const char* dbname);

int create_table(const char *dbname, const char *relation_name, dict* attributes);
int drop_table(const char* dbname, const char *relation_name);

db *find_db(dbHandler* handler,const char* dbname);
table *find_table(const char* dbname, const char* table_name);
int table_sync();




#endif