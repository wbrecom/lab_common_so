import socket
import struct
#import logging
try:
    import simplejson
except Exception, data:
    import json as simplejson

#logger = logging.getLogger('root')

def reqbinserver(host, port, req_str, log_id = 0, timeout_usecs = 1500000) :
    address = (host, port)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try :
        s.settimeout(timeout_usecs / 1000000.0)
        s.connect(address)

        head = struct.pack('IIII', len(req_str), log_id, 0, 0)
        ret = s.send(head)
        if ret != len(head) :
            s.close()
            return False
        send_num = 0
        while send_num < len(req_str) :
            ret = s.send(req_str[send_num:])
            if ret <= 0 :
                s.close()
                return False
            send_num += ret

        head = s.recv(16)
        body_len, log_id, tmp1, tmp2 = struct.unpack('IIII', head)
        if body_len == 0:
            data = ''
        else :
            recved_data = []
            recved_len = 0
            while recved_len < body_len:
                data = s.recv(body_len - recved_len)
                if len(data) <= 0:
                    s.close()
                    return False
                recved_data.append(data)
                recved_len += len(data)
            data = ''.join(recved_data)
        s.close()
        return data
    except Exception, e:
        pass
    s.close()
    return False

def reqbinserver_cluster(cluster, server_idx, req_str,
        log_id = 0, timeout_usecs = 1500000, retry_times = 2) :
    server_num = len(cluster)

    for i in range(0, retry_times):
        server = cluster[(i + server_idx) % server_num]
        resp = reqbinserver(server["host"], server["port"], req_str, log_id, timeout_usecs )
        print i
        if resp != False:
            return resp
    return False

if __name__ == "__main__":
    import time
    import json
    import sys
    import random

    if len(sys.argv) < 2:
        print "usage:%s server_host server_port req\n" % sys.argv[0]
        sys.exit(1)
    host = sys.argv[1]
    port = int(sys.argv[2])
    req_str = sys.argv[3]
    print "send:%u:%s" % (len(req_str), req_str)
    beg = time.time()

    log_id = random.randint(0, 99999999)
    resp_str = reqbinserver_cluster([{"host":host, "port":port}], 0, req_str, log_id)
    #import simplejson
    #import zlib
    #try:
	#    print zlib.decompress(resp_str)
    #except Exception, data:
    print resp_str
    #print simplejson.loads(resp_str)
    end = time.time()
    #print "recv:%u:%s" % (len(resp_str), resp_str)
    print "cost %f secs" % (end - beg)
