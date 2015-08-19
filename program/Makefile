include Makefile.in

INC_PATH=-I./depend/xbson/include -I./depend/feature_pool_bin/include/ -I./depend/uthash/ -I./depend/mapdb/include/ -I./depend/woo/include -I/usr/local/include/json -I/usr/local/include/hiredis/ -I./include/ -I./include/db/ -I./include/utility -I./include/work -I./include/algorithm
LIB_LINK=-L./depend/xbson/lib -L./depend/feature_pool_bin/lib/ -L./depend/mapdb/lib/ -L./depend/woo/lib -L/usr/local/lib -lxbson -lmemcached -lapifp -lmapdb -lcurl -lssl -ljson -lwoo -lstdc++ -lz -lhiredis -lpthread
ALL_BIN = ./bin/lab_common_svr ./bin/lab_common_main
all:$(ALL_BIN)

./bin/lab_common_svr : ./src/lab_common_svr.o ./src/ini_file.o ./src/encode_convert.o
	test -d ./bin || mkdir ./bin
	$(CC) $(CFLAGS) -g -o  $@ $^ $(LIB_LINK)

./bin/lab_common_main : ./src/lab_common_main.o ./src/ini_file.o ./src/encode_convert.o
	test -d ./bin || mkdir ./bin
	$(CC) $(CFLAGS) -g -o $@ $^ $(LIB_LINK)

%.o:%.cpp
	$(CC) $(CFLAGS) -c -o $@ $^ $(INC_PATH) -pg
%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $^ $(INC_PATH) -pg

clean:
	rm $(ALL_BIN) ./src/*.o
