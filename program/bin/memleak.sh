export LD_LIBRARY_PATH=/usr/local/lib/:$LD_LIBRARY_PATH
valgrind --tool=memcheck --leak-check=full -v --log-file=./log --show-reachable=yes ./lab_common_main ../conf/lab_common_main.conf
