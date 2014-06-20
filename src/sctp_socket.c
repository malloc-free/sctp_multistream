/*
 * sctp_socket.c
 *
 *  Created on: 8/05/2014
 *      Author: michael
 */
#include <netinet/in.h>

#include "sctp_socket.h"
#include "tb_common.h"
#include "sctp_multi_common.h"
#include "sctp_association.h"

#include <pthread.h>
#include <glib.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/sctp.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <sys/epoll.h>
#include <fcntl.h>

sm_sock_t
*sm_sock_create(int sd)
{
	sm_sock_t *sock = malloc(sizeof(sm_sock_t));
	sock->sd = sd;
	if(sock->sd == -1) {
		sock->sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

		if(sock->sd == -1) {
			free(sock);
			return NULL;
		}
	}

	pthread_mutex_init(&sock->lock, NULL);
	sock->status = SM_SK_CREATED;
	sock->listening = FALSE;
	sock->reg_str = g_hash_table_new_full(&g_int_hash, &g_int_equal,
			&sm_sock_destroy_key, NULL);

	return sock;
}

void
sm_sock_destroy(sm_sock_t *sock)
{
	pthread_mutex_destroy(&sock->lock);
	g_hash_table_destroy(sock->reg_str);
	free(sock);
}

void
sm_sock_destroy_key(gpointer data)
{
	free(data);
}

sm_desc_t
*sm_desc_create() {
	sm_desc_t *desc = malloc(sizeof(sm_desc_t));
	desc->in_event = eventfd(0, 0);
	desc->out_event = eventfd(0, 0);
	pipe(desc->in_pipe);
	pipe(desc->out_pipe);
	desc->type = SM_CONNECT;

	return desc;
}

void
sm_desc_destroy(sm_desc_t *desc) {
	free(desc);
}
int
sm_sock_events_subscribe(sm_sock_t *sock)
{
	int rc, flags;

	struct sctp_event_subscribe events;

	rc = setsockopt(sock->sd, SOL_SCTP, SCTP_EVENTS,
			&events, sizeof(events));

	if(rc != 0)
	{
		perror("sm_as_events_subscribe: setsockopt");
		return rc;
	}

	flags = fcntl(sock->sd, F_GETFL, 0);

	if(rc == -1) {
		perror("sm_as_events_subscribe: fctl get flags");
		return rc;
	}

	fprintf(stdout, "Setting to non-blocking mode\n");
	flags |= O_NONBLOCK;

	rc = fcntl(sock->sd, F_SETFL, flags);

	if(rc == -1) {
		perror("sm_as_events_subscribe: fctl set flags");
		return rc;
	}

	return 0;
}

int
sm_sock_reg_stream(sm_sock_t *sock, sm_desc_t *desc) {
	int ret_value = -1;

	if(!g_hash_table_contains(sock->reg_str, &desc->s_num)) {
		int *key = malloc(sizeof(int));
		*key = desc->s_num;
		g_hash_table_insert(sock->reg_str, key, desc);
	}

	return ret_value;
}


int
sm_sock_connect(sm_sock_t *sock, const struct sockaddr *addr,
		socklen_t len)
{
	pthread_mutex_lock(&sock->lock);

	int rc = 0;

	if(sock->status < SM_SK_CONNECTED)
		rc = connect(sock->sd, addr, len);

	if(rc == 0) {
		sock->status = SM_SK_CONNECTED;
		fprintf(stdout, "connect : connected\n");
		sm_sock_create_as(sock, sock->sd, &sm_as_in_event);
	}
	else {
		perror("sm_sock_connect");
		sock->status = SM_SK_ERROR;
	}

	pthread_mutex_unlock(&sock->lock);

	return rc;
}

int
sm_sock_bind(sm_sock_t *sock, const struct sockaddr *addr,
		socklen_t len)
{
	pthread_mutex_lock(&sock->lock);
	fprintf(stdout, "bind : socket = %d\n", sock->sd);
	int rc = 0;

	if(sock->status < SM_SK_BOUND)
		rc = bind(sock->sd, addr, len);

	if(rc == 0)
		sock->status = SM_SK_BOUND;
	else
		sock->status = SM_SK_ERROR;

	pthread_mutex_unlock(&sock->lock);

	return rc;
}

int
sm_sock_listen(sm_sock_t *sock, const int backlog, int epoll_d)
{
	pthread_mutex_lock(&sock->lock);
	fprintf(stdout, "listen : socket = %d\n", sock->sd);
	int rc = 0;

	if(sock->status < SM_SK_LISTENING) {
		rc = listen(sock->sd, backlog);

		if(rc == 0) {
			sock->epoll_d = epoll_d;
			sock->status = SM_SK_LISTENING;
			sock->e_data.data = sock;
			sock->e_data.event_funct = &sm_sock_accept;
			struct epoll_event *events = malloc(sizeof(struct epoll_event));
			events->events = EPOLLIN | EPOLLET;
			events->data.ptr = &sock->e_data;
			rc = epoll_ctl(epoll_d, EPOLL_CTL_ADD, sock->sd, events);
		}

		if(rc == -1) {
			sock->status = SM_SK_ERROR;
		}

	}

	pthread_mutex_unlock(&sock->lock);

	return rc;
}

void
sm_sock_accept(int events, void *data)
{
	fprintf(stdout, "sm_sock_accept called\n");
	sm_sock_t *sock = (sm_sock_t*)data;
	int flags, rc;
	char buffer[128];
	struct sctp_sndrcvinfo info;
	struct sockaddr_storage storage;

	socklen_t addr_len = (socklen_t)sizeof(storage);
	rc = accept(sock->sd, (struct sockaddr*)&storage, &addr_len);
	fprintf(stdout, "connection accepted %d\n", rc);

	if(rc != -1) {
		sm_sock_create_as(sock, rc, &sm_as_in_event);
	}
}

int
sm_sock_create_as(sm_sock_t *sock, int sd, epoll_funct e_funct) {
	sm_as_t *as = sm_as_create(sock->reg_str);
	as->sd = sd;
	as->e_data.data = as;
	as->e_data.event_funct = e_funct;
	struct epoll_event events;
	events.data.ptr = &as->e_data;
	events.events = EPOLLIN | EPOLLET;

	return epoll_ctl(sd, EPOLL_CTL_ADD, sd, events);
}

int
sm_sock_close(sm_sock_t *sock)
{
	int rc;

	if(sock->status != SM_SK_CONNECTED && sock->status != SM_SK_LISTENING)
	{
		rc = -1;
	}
	else
	{
		rc = close(sock->sd);

		if(rc == 0)
			sock->status = SM_SK_CLOSED;
		else
			sock->status = SM_SK_ERROR;
	}

	pthread_mutex_unlock(&sock->lock);

	return rc;
}

