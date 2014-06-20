/*
 * sctp_socket.h
 *
 *  Created on: 8/05/2014
 *      Author: michael
 */

#ifndef SCTP_SOCKET_H_
#define SCTP_SOCKET_H_

#include "tb_epoll.h"
#include "sctp_multi_common.h"

#include <glib.h>
#include <pthread.h>
#include <sys/socket.h>

typedef enum
{
	SM_SK_CREATED = 0,
	SM_SK_CONNECTED = 1,
	SM_SK_BOUND = 2,
	SM_SK_LISTENING = 4,
	SM_SK_ACCEPTING = 8,
	SM_SK_CLOSED = 16,
	SM_SK_ERROR = 32
}
sm_sock_status_t;

typedef enum
{
	SK_CM_CONTINUE = 0,
	SK_CM_EXIT = 1
}
sm_sock_command_t;

typedef struct
{
	sock_t sd;
	sm_sock_status_t status;
	int ht_key;
	pthread_mutex_t lock;
	int epoll_d;
	pthread_t l_thread;
	gboolean listening;
	sm_sock_command_t command;
	GHashTable *reg_str;
	ms_epoll_data e_data;
}
sm_sock_t;

#define SM_PIPE_READ_FROM 0
#define SM_PIPE_WRITE_TO  1

typedef enum
{
	SM_SERVER, SM_CONNECT
}
sm_desc_type;

struct descriptor
{
	sm_sock_t *sock;
	stream_t s_num;
	event_t in_event;
	event_t out_event;
	pipe_t in_pipe[2];
	pipe_t out_pipe[2];
	sm_desc_type type;
	GQueue *desc_queue;
	struct descriptor *parent;
};

typedef struct descriptor sm_desc_t;

sm_desc_t
*sm_desc_create();

sm_sock_t
*sm_sock_create(int sd);

void
sm_sock_destroy(sm_sock_t *sock);

void
sm_sock_destroy_key(gpointer data);


int
sm_sock_events_subscribe(sm_sock_t *as);

int
sm_sock_reg_stream(sm_sock_t *sock, sm_desc_t *desc);

int
sm_sock_connect(sm_sock_t *sock, const struct sockaddr *addr,
		socklen_t len);

int
sm_sock_bind(sm_sock_t *as, const struct sockaddr *addr,
		socklen_t len);

int
sm_sock_listen(sm_sock_t *sock, const int backlog, int epoll_d);

void
sm_sock_accept(int events, void *data);

int
sm_sock_create_as(sm_sock_t *sock, int sd, epoll_funct e_funct);

int
sm_sock_close();

#endif /* SCTP_SOCKET_H_ */
