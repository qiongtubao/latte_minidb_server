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
sds* parseArgv(int argc, char** argv, int* len) {
    char** result = zmalloc(sizeof(sds)*(argc + 1));
    result[argc] = NULL;
    for(int j = 0; j < argc; j++) {
        result[j] = sdsnewlen(argv[j], strlen(argv[j]));
    }
    *len = argc;
    return result;
}


int initHandler(latteMiniDBServer* server) {
    server->hander = createDefaultHandler();
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
            if (loadConfigFromFile(server->config, server->configfile) == 0) {
                goto fail;
            }
            attribute_index++;
        }
    }
    //add config attribute property
    if (loadConfigFromArgv(server->config, argv + attribute_index, argc - attribute_index) == 0) {
        goto fail;
    }

    int rc = 0;
    if (configGetInt(server->config, "demon")) {
        //启动守护进程
        int rc = daemonize_service(configGetSds(server->config, "std_out"), configGetSds(server->config, "std_err"));
        if (rc != 0) {
            miniDBServerLog(LOG_INFO, "Shutdown due to failed to daemon current process!");
            return rc;
        }
    } 
    sds process_name = configGetSds(server->config, "process_name");
    if (process_name != NULL) {
      writePidFile(process_name);
    }
    initHandler(server);

    // startLatteServer(&server->server);
    return 1;
fail:
    miniDBServerLog(LOG_ERROR, "start latte server fail");
    return 0;

}

int startMiniDBServer(struct latteMiniDBServer* redisServer) {
    return 1;
}