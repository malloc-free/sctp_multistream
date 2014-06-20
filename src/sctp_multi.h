/*
 * sctp_multi.h
 *
 *  Created on: 5/05/2014
 *      Author: michael
 */

#ifndef SCTP_MULTI_H_
#define SCTP_MULTI_H_

#include "sctp_multi_common.h"
#include "sctp_socket.h"

#include <netinet/sctp.h>
#include <glib.h>
#include <pthread.h>


typedef enum
{
	SM_M_CONTINUE = 0,
	SM_M_STOP = 1
}
sm_multi_command_t;

typedef struct
{
	//Data Structures
	GHashTable *in_sockets;
	GHashTable *desc;
	GHashTable *out_sockets;

	int max_fd;

	//Threading
	pthread_mutex_t mutex;
	pthread_t m_thread;
	int epoll_d;
	int polling;
	sm_multi_command_t command;

}
sm_multi_t;

sm_multi_t
*sm_multi_create();

void
sm_multi_destroy(sm_multi_t *multi);

md_t
sm_socket(sm_multi_t *multi);

int
sm_connect(sm_multi_t *multi, const md_t md,
		const struct sockaddr *addr, socklen_t len, stream_t s_num);

int
sm_bind(sm_multi_t *multi, const md_t md,
		const struct sockaddr *addr, socklen_t len, stream_t s_num);

int
sm_listen(sm_multi_t *multi, const md_t md, const int backlog);

int
sm_start_poll(sm_multi_t *multi);

int
sm_accept(sm_multi_t *multi, const md_t md);

int
sm_close(sm_multi_t *multi, const md_t md);

sm_desc_t
*sm_get_desc(sm_multi_t *multi, const md_t md);

void
sm_destroy_key(gpointer data);

sm_sock_t
*sm_get_in_sock(sm_multi_t *multi, const struct sockaddr* addr,
		socklen_t len);

void
sm_insert_in_socket(sm_multi_t *multi, sm_sock_t *sock,
		const struct sockaddr *addr, socklen_t len);

void
sm_insert_out_socket(sm_multi_t *multi, sm_sock_t *sock,
		const struct sockaddr *addr, socklen_t len);

sm_sock_t
*sm_get_out_sock(sm_multi_t *multi, const struct sockaddr *addr,
		socklen_t len);

void
*sm_begin_polling(void *data);

#endif /* SCTP_MULTI_H_ */
