export LD_LIBRARY_PATH=/usr/local/lib/:$LD_LIBRARY_PATH
./saferun ./lab_common_svr ../conf/lab_common_svr.conf >> ../log/work_interface.log 2>&1 &
