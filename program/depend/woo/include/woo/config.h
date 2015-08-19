/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_config_H__
#define __woo_config_H__

#include <map>
#include <vector>
#include <string>

namespace woo {
typedef std::map<std::string, std::string> Conf;

typedef struct _inet_socket_addr_t {
   char ip[32];
   int port;
} inet_socket_addr_t;

char *rtrim(char *s);

typedef std::vector<inet_socket_addr_t> ServerArray;

Conf *
conf_load(const char *path);

void 
conf_destroy( Conf *conf);

const char *
conf_get_str(Conf *conf, const char *name, const char *def_value);

int
conf_get_int(Conf *conf, const char *name, int def_value);

long
conf_get_long(Conf *conf, const char *name, int def_value);

int 
conf_get_server_array(Conf *conf, const char *server_name, ServerArray &server_arr);

}

#endif
