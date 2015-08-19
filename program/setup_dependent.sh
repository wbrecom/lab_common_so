#/bin/sh
cd depend/tools/

# 判断是32位系统还是64位系统
function Is_x64()
{
    if [ $(getconf WORD_BIT) = '32' ] && [ $(getconf LONG_BIT) = '64' ]; then
    	return 1
	else
	    return 0
	fi  
}

echo "========== setup curl begin =========="
Is_x64
if [ $? == 1 ]; then
	rpm -i c-ares-1.6.0-5.el5.x86_64.rpm
	rpm -i c-ares-devel-1.6.0-5.el5.x86_64.rpm
else
    rpm -i c-ares-1.6.0-5.i386.rpm
    rpm -i c-ares-devel-1.6.0-5.el5.i386.rpm
fi
tar -zxvf curl-7.28.1.tar.gz
cd curl-7.28.1
./configure --enable-ares
./configure && make && make install
cd -
rm curl-7.28.1 -rf
echo "========== setup curl end =========="

echo "========== setup hiredis begin =========="
unzip hiredis-master.zip
cd hiredis-master
make && make install
cd -
rm hiredis-master -rf
echo "========== setup hiredis finish =========="

echo "========== setup json-c begin =========="
rm /usr/local/include/json -rf
rm /usr/local/include/json-c -rf
unzip json-c-master.zip
cd json-c-master
./autogen.sh && ./configure && make && make install
cd -
rm json-c-master -rf
echo "========== setup json-c finish =========="

echo "========== setup libevent begin =========="
tar -zxvf libevent-2.0.10-stable.tar.gz
cd libevent-2.0.10-stable
./configure && make && make install
cd -
rm libevent-2.0.10-stable -rf
echo "========== setup libevent finish =========="

echo "========== setup memcached begin =========="
tar -zxvf memcached-1.4.5.tar.gz
cd memcached-1.4.5
./configure && make && make install
cd -
rm memcached-1.4.5 -rf
echo "========== setup memcached finish =========="

echo "========== setup libmemcached begin =========="
tar -zxvf libmemcached-0.53.tar.gz
cd libmemcached-0.53
./configure --with-memcached && make && make install
cd -
rm libmemcached-0.53 -rf
echo "========== setup libmemcached finish =========="
