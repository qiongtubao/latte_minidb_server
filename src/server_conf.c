#include "server.h"

config_rule_t unix_socket_path_rule = SDS_CONFIG_INIT(NULL);
#define DEFAULT_SERVER_PORT 6789
config_rule_t server_port_rule = LL_CONFIG_INIT(DEFAULT_SERVER_PORT);
config_rule_t protocol_rule = SDS_CONFIG_INIT(NULL);
config_rule_t std_out_rule = SDS_CONFIG_INIT(NULL);
config_rule_t std_err_rule = SDS_CONFIG_INIT(NULL);
config_rule_t trx_kit_name_rule = SDS_CONFIG_INIT(NULL);
config_rule_t thread_handling_name_rule = SDS_CONFIG_INIT(NULL);
config_rule_t buffer_pool_memory_size_rule = LL_CONFIG_INIT(NULL);
config_rule_t durability_mode_rule = SDS_CONFIG_INIT(NULL);

config_rule_t demon_rule = LL_CONFIG_INIT(NULL);

config_rule_t log_file_name_rule = SDS_CONFIG_INIT("minidb.log");
config_rule_t log_file_level_rule = LL_CONFIG_INIT(NULL);
config_rule_t log_console_level_rule = LL_CONFIG_INIT(NULL);
config_rule_t process_name_rule = SDS_CONFIG_INIT(NULL);

config_manager_t* createServerConfig() {
    config_manager_t* c = config_manager_new();
    config_register_rule(c, sds_new("process_name"), &process_name_rule);
    config_register_rule(c, sds_new("unix_socket_path"), &unix_socket_path_rule);
    config_register_rule(c, sds_new("server_port"),  &server_port_rule);
    config_register_rule(c, sds_new("protocol"),  &protocol_rule);
    config_register_rule(c, sds_new("std_out"), &std_out_rule);
    config_register_rule(c, sds_new("std_err"), &std_err_rule);
    config_register_rule(c, sds_new("trx_kit_name"), &trx_kit_name_rule);
    config_register_rule(c, sds_new("thread_handling_name"), &thread_handling_name_rule);
    config_register_rule(c, sds_new("buffer_pool_memory_size"), &buffer_pool_memory_size_rule);
    config_register_rule(c, sds_new("durability_mode"), &durability_mode_rule);
    
    config_register_rule(c, sds_new("demon"), &demon_rule);

    config_register_rule(c, sds_new("log_file_name"), &log_file_name_rule);
    config_register_rule(c, sds_new("log_file_level"), &log_file_level_rule);
    config_register_rule(c, sds_new("log_console_level"), &log_console_level_rule);
    
    
    return c;
}

