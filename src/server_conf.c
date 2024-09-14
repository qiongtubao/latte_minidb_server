#include "server.h"

configRule unix_socket_path_rule = SDS_CONFIG_INIT(NULL);
#define DEFAULT_SERVER_PORT 6789
configRule server_port_rule = LL_CONFIG_INIT(DEFAULT_SERVER_PORT);
configRule protocol_rule = SDS_CONFIG_INIT(NULL);
configRule std_out_rule = SDS_CONFIG_INIT(NULL);
configRule std_err_rule = SDS_CONFIG_INIT(NULL);
configRule trx_kit_name_rule = SDS_CONFIG_INIT(NULL);
configRule thread_handling_name_rule = SDS_CONFIG_INIT(NULL);
configRule buffer_pool_memory_size_rule = LL_CONFIG_INIT(NULL);
configRule durability_mode_rule = SDS_CONFIG_INIT(NULL);

config* createServerConfig() {
    config* c = createConfig();
    registerConfig(c, sdsnew("unix_socket_path"), &unix_socket_path_rule);
    registerConfig(c, sdsnew("server_port"),  &server_port_rule);
    registerConfig(c, sdsnew("protocol"),  &protocol_rule);
    registerConfig(c, sdsnew("std_out"), &std_out_rule);
    registerConfig(c, sdsnew("std_err"), &std_err_rule);
    registerConfig(c, sdsnew("trx_kit_name"), &trx_kit_name_rule);
    registerConfig(c, sdsnew("thread_handling_name"), &thread_handling_name_rule);
    registerConfig(c, sdsnew("buffer_pool_memory_size"), &buffer_pool_memory_size_rule);
    registerConfig(c, sdsnew("durability_mode"), &durability_mode_rule);
    return c;
}

