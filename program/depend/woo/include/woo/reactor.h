/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_reactor_H__
#define __woo_reactor_H__

namespace woo {

typedef struct _reactor_t  reactor_t;

const int REACTOR_EVENT_FLAG_MAIN_THREAD_EXEC = 0x01;
const int REACTOR_EVENT_FLAG_FAST_LOOP = 0x02;
const int REACTOR_EVENT_FLAG_QUEUE_LONG = 0x04;
const int REACTOR_EVENT_FLAG_QUEUE_FULL = 0x08;

typedef int (* reactor_event_handle_t) (reactor_t *reactor, void *handle_data, int handle_fd, int *flag);

reactor_t* reactor_create(int pool_thread_num);
int reactor_add(reactor_t *reactor, int events, reactor_event_handle_t handle, void *handle_data, int handle_fd, int flag, size_t timeout);
int reactor_remove(reactor_t *reactor, int fd);
int reactor_run(reactor_t *reactor, bool new_thread = false);
void reactor_quit(reactor_t *reactor);
void reactor_wait(reactor_t *reactor);
void reactor_destroy(reactor_t *reactor);

}

#endif
