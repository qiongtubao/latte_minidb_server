#include "server.h"
#include <string.h>
#include "utils.h"
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include "config/config.h"
#include "code.h"
#include "./log.h"
#include "os/process.h"
#include "os/pidfile.h"

sds* parseArgv(int argc, char** argv, int* len) {
    char** result = zmalloc(sizeof(sds)*(argc + 1));
    result[argc] = NULL;
    for(int j = 0; j < argc; j++) {
        result[j] = sds_new_len(argv[j], strlen(argv[j]));
    }
    *len = argc;
    return result;
}


int initHandler(latteMiniDBServer* server) {
    server->hander = db_handler_new();
    int ret = db_handler_init(server->hander, 
        "miniob",
        config_get_sds(server->config, "trx_kit_name"),
        config_get_sds(server->config, "durability_mode")
    );
    if (is_rc_fail(ret)) {
        miniDBServerLog(LOG_ERROR,"failed to init handler. rc=%d", ret);
        return -1;
    }
    miniDBServerLog(LOG_INFO, "init Handler");
    return SUCCESS;
}

int initLog(latteMiniDBServer* server) {
  log_init();
  log_add_stdout(LATTE_MINIDB_SERVER_LOG_TAG, 
    config_get_int64(server->config, "log_console_level")
  );
  log_add_file(LATTE_MINIDB_SERVER_LOG_TAG, 
    config_get_sds(server->config, "log_file_name"), 
    config_get_int64(server->config, "log_file_level")
  );
  return SUCCESS;
}

//初始化dbserver
int initMiniDBServer(latteMiniDBServer* server, int argc, char** argv) {
    int len = 0;
    server->exec_argv = parseArgv(argc, argv, &len);
    server->exec_argc = len;

    server->executable = getAbsolutePath(argv[0]);
    server->config = createServerConfig();

    int attribute_index = 1;
    if (argc > 1) {
        if (argv[1][0] != '-') {
            server->configfile = getAbsolutePath(argv[1]);
            if (load_config_from_file(server->config, server->configfile) == 0) {
                goto fail;
            }
            attribute_index++;
        }
    }
    //add config attribute property
    if (load_config_from_argv(server->config, argv + attribute_index, argc - attribute_index) == 0) {
        goto fail;
    }

    int rc = 0;
    if (config_get_int64(server->config, "demon")) {
        //启动守护进程
        int rc = daemonize_service(config_get_sds(server->config, "std_out"), config_get_sds(server->config, "std_err"));
        if (rc != 0) {
            miniDBServerLog(LOG_INFO, "Shutdown due to failed to daemon current process!");
            return rc;
        }
    } 
    sds process_name = config_get_sds(server->config, "process_name");
    if (process_name != NULL) {
      write_pid_file(process_name);
    }
    initLog(server);
    initHandler(server);
    miniDBServerLog(LOG_INFO, "Successfully init utility");
    // startLatteServer(&server->server);
    return 1;
fail:
    miniDBServerLog(LOG_ERROR, "start latte server fail");
    return 0;

}

int startMiniDBServer(struct latteMiniDBServer* redisServer) {
    return 1;
}