include Makefile.in

INC_PATH=-I../depend/xbson/include -I../depend/feature_pool_bin/include/ -I../depend/uthash/ -I../depend/mapdb/include/ -I../depend/woo/include -I/usr/local/include/json -I/usr/local/include/hiredis/ -I../include/ -I../include/db/ -I../include/utility -I../include/work -I../include/algorithm
LIB_LINK=-L../depend/xbson/lib -L../depend/feature_pool_bin/lib/ -L../depend/mapdb/lib/ -L../depend/woo/lib -L/usr/local/lib -lxbson -lmemcached -lapifp -lmapdb -lcurl -lssl -ljson -lwoo -lstdc++ -lz -lhiredis -lpthread
ALL_BIN=../lib/InterAlgorithmInterface.so

all:$(ALL_BIN)

../lib/InterAlgorithmInterface.so : ./src/inter_algorithm_interface.o
	test -d ../lib || mkdir ../lib
	$(CC) $(CFLAGS) -g -o $@ $^ $(LIB_LINK)

%.o:%.cpp
	$(CC) $(CFLAGS) -c -o $@ $^ $(INC_PATH) -pg
%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $^ $(INC_PATH) -pg

clean:
	rm $(ALL_BIN) ./src/*.o
