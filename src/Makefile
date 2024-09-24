
LIB_MODUELS="sds config server os fs dict_plugins mutex list crc set"

uname_S:= $(shell sh -c 'uname -s 2>/dev/null || echo not')
uname_M:= $(shell sh -c 'uname -s 2>/dev/null || echo not')

OPTIMIZATION?=-O0
STD=-std=c99 -pedantic -DREDIS_STATIC=''
WARN=-Wall -W -Wno-missing-field-initializers
OPT=$(OPTIMIZATION)
DEBUG=-g -ggdb
FINAL_CFLAGS=$(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS) $(LATTE_REDIS_CFLAGS)
LATTE_REDIS_CC=$(QUIET_CC)$(CC) $(FINAL_CFLAGS)
ifeq ($(uname_S),Linux)
	FINAL_SHARD_LIB=-lrt 
endif

latte_lib:
	cd ../deps/latte_c && make build MODULES=$(LIB_MODUELS) 

%.o: %.c 
	$(LATTE_REDIS_CC) -o $@ -c $< -I../deps/latte_c/out/include 

MAC_M1_CFLAGS=-fprofile-arcs -ftest-coverage
OBJS= main.o server.o server_conf.o utils.o default_handler.o session.o field_meta.o db.o  trx.o  trx_mvcc.o trx_vacuous.o log_handler.o log_handler_disk.o log_handler_vacuous.o log_file.o log_buffer.o page.o table.o table_meta.o disk_buffer_pool.o double_write_buffer.o record_manager.o log_replayer_integrated.o bplus_tree_index.o bplus_tree_log.o bplus_tree.o frame.o trx_log_mvcc.o buffer_pool_log.o index.o index_meta.o utils_lru_cache.o utils_mem_pool.o value.o
# -lstdc++
all:  $(OBJS) 
	$(LATTE_REDIS_CC) -o main $(OBJS) -I../deps/latte_c/out/include ../deps/latte_c/out/lib/liblatte.a  ../deps/latte_c/deps/jemalloc/lib/libjemalloc.a $(FINAL_SHARD_LIB) -lm  -ldl -lz -lstdc++ -lpthread $(MAC_M1_CFLAGS)
	

clean:
	rm -rf *.o 
	rm -rf main

distclean:
	cd ../deps/latte_c && make clean_all 
	$(MAKE) clean

test: 
	@(cd ..; ./runtest )