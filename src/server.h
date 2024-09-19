#ifndef __LATTE_MINIDB_SERVER_H
#define __LATTE_MINIDB_SERVER_H

#include "server/server.h"
#include "log/log.h"
#include "default_handler.h"
config* createServerConfig();




typedef struct latteMiniDBServer {
    struct latteServer server;
    int exec_argc;
    sds* exec_argv;
    sds executable; /** execut file path **/
    sds configfile;
    struct config* config;
    struct dbHandler* hander;
} latteMiniDBServer;


#define DB_ERR 0
#define DB_OK 1

#define PRIVATE 
PRIVATE int initMiniDBServer(struct latteMiniDBServer* redisServer, int argc, sds* argv);
PRIVATE int startMiniDBServer(struct latteMiniDBServer* redisServer);

#define LATTE_MINIDB_SERVER_LOG_TAG "latte_mini_server"
#define miniDBServerLog(log, ...) log_log(LATTE_MINIDB_SERVER_LOG_TAG, log, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__) 




#endif