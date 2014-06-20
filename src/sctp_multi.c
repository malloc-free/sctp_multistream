/*
 * sctp_multi.c
 *
 *  Created on: 5/05/2014
 *      Author: michael
 */
#include <netinet/in.h>

#include "sctp_multi.h"
#include "sctp_multi_common.h"
#include "sctp_socket.h"
#include "tb_common.h"

#include <stdlib.h>
#include <glib.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <assert.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <errno.h>


sm_multi_t
*sm_multi_create()
{
	sm_multi_t *multi = malloc(sizeof(sm_multi_t));
	multi->in_sockets = g_hash_table_new_full(&g_int_hash, &g_int_equal,
			&sm_destroy_key, NULL);
	multi->desc = g_hash_table_new_full(&g_int_hash, &g_int_equal,
			&sm_destroy_key, NULL);
	multi->out_sockets = g_hash_table_new_full(&g_int_hash, &g_int_equal,
			&sm_destroy_key, NULL);

	multi->max_fd = 0;
	multi->epoll_d = epoll_create1(0);
	if(multi->epoll_d == -1) {
		fprintf(stderr, "Cannot create epoll descriptor\n");
		exit(1);
	}
	multi->polling = 0;
	multi->command = SM_M_CONTINUE;

	//Threading
	pthread_mutex_init(&multi->mutex, NULL);

	return multi;
}

void
sm_multi_destroy(sm_multi_t *multi)
{
	g_hash_table_destroy(multi->in_sockets);
	g_hash_table_destroy(multi->desc);
	pthread_mutex_destroy(&multi->mutex);

	free(multi);
}

md_t
sm_socket(sm_multi_t *multi)
{
	pthread_mutex_lock(&multi->mutex);

	md_t *md = malloc(sizeof(int));
	sm_desc_t *desc = sm_desc_create();
	*md = desc->in_event;
	g_hash_table_insert(multi->desc, md, desc);

	pthread_mutex_unlock(&multi->mutex);

	return *md;
}

int
sm_connect(sm_multi_t *multi, const md_t md, const struct sockaddr *addr,
		socklen_t len, stream_t s_num)
{
	int rc = 0;
	sm_desc_t *desc= g_hash_table_lookup(multi->desc, &md);
	assert(desc != NULL);
	desc->s_num = s_num;
	desc->sock = sm_get_out_sock(multi, addr, len);

	if(desc->sock == NULL) {
		desc->sock = sm_sock_create(-1);
		sm_sock_events_subscribe(desc->sock);

		sm_start_poll(multi);
		sm_insert_out_socket(multi, desc->sock, addr, len);
	}

	rc = sm_sock_connect(desc->sock, addr, len);

	if(rc == 0) {
		sm_sock_reg_stream(desc->sock, desc);
	}

	return rc;
}

int
sm_bind(sm_multi_t *multi, const md_t md, const struct sockaddr *addr,
		socklen_t len, stream_t s_num)
{
	int rc = 0;
	sm_desc_t *desc = g_hash_table_lookup(multi->desc, &md);
	assert(desc != NULL);
	desc->s_num = s_num;
	desc->sock = sm_get_in_sock(multi, addr, len);
	desc->type = SM_SERVER;
	desc->desc_queue = g_queue_new();

	if(desc->sock == NULL) {
		desc->sock = sm_sock_create(-1);
		sm_sock_events_subscribe(desc->sock);
		sm_insert_in_socket(multi, desc->sock, addr, len);
	}

	rc = sm_sock_bind(desc->sock, addr, len);

	if(rc == 0) {
		sm_sock_reg_stream(desc->sock, desc);
	}

	return rc;
}

int
sm_listen(sm_multi_t *multi, const md_t md, const int backlog)
{
	int rc = -1;
	sm_desc_t *desc = g_hash_table_lookup(multi->desc, &md);

	if(desc != NULL && desc->sock != NULL) {

		rc = sm_sock_listen(desc->sock, backlog, multi->epoll_d);
		sm_start_poll(multi);
	}
	else {
		rc = -1;
	}

	return rc;
}

int
sm_start_poll(sm_multi_t *multi) {
	pthread_mutex_lock(&multi->mutex);

	if(!multi->polling) {
		fprintf(stdout, "Starting polling\n");
		pthread_create(&multi->m_thread, NULL, &sm_begin_polling, multi);
		multi->polling = 1;
	}

	pthread_mutex_unlock(&multi->mutex);

	return 0;
}

int
sm_accept(sm_multi_t *multi, const md_t md)
{
	sm_desc_t *desc = g_hash_table_lookup(multi->desc, &md);
	int nd = -1;

	char buff[128];

	read(desc->in_event, buff, sizeof(buff));

	return nd;
}

int
sm_close(sm_multi_t *multi, const md_t md)
{
	int rc;

	sm_desc_t *desc = g_hash_table_lookup(multi->desc, &md);

//	if(desc->as != NULL)
//		sm_as_return_stream(desc->as, desc->stream);

	//rc = sm_stream_close(desc);

	if(rc != 0)
	{
		return rc;
	}

	//sm_stream_destroy(desc);

	return rc;
}

sm_desc_t
*sm_get_desc(sm_multi_t *multi, const md_t md)
{
	return (sm_desc_t*)g_hash_table_lookup(multi->desc, &md);
}

void
sm_destroy_key(gpointer data)
{
	free(data);
}

sm_sock_t
*sm_get_in_sock(sm_multi_t *multi, const struct sockaddr *addr,
		socklen_t len)
{
	sm_sock_t *sock = NULL;

	if(addr->sa_family == AF_INET) {
		struct sockaddr_in *in = (struct sockaddr_in*)addr;
		int val = in->sin_addr.s_addr;
		sock = g_hash_table_lookup(multi->in_sockets, &val);
	}

	return sock;
}

void
sm_insert_in_socket(sm_multi_t *multi, sm_sock_t *sock,
		const struct sockaddr *addr, socklen_t len)
{
	if(addr->sa_family == AF_INET) {
		struct sockaddr_in *in = (struct sockaddr_in*)addr;

		int *val = malloc(sizeof(unsigned long));
		*val = in->sin_addr.s_addr;
		sock->ht_key = in->sin_addr.s_addr;
		g_hash_table_insert(multi->in_sockets, val, sock);
	}
}

void
sm_insert_out_socket(sm_multi_t *multi, sm_sock_t *sock,
		const struct sockaddr *addr, socklen_t len)
{
	if(addr->sa_family == AF_INET) {
		struct sockaddr_in *in = (struct sockaddr_in*)addr;

		int *val = malloc(sizeof(unsigned int));
		*val = in->sin_addr.s_addr;
		sock->ht_key = in->sin_addr.s_addr;
		g_hash_table_insert(multi->out_sockets, val, sock);
	}
}

sm_sock_t
*sm_get_out_sock(sm_multi_t *multi, const struct sockaddr *addr,
		socklen_t len)
{
	sm_sock_t *sock = NULL;

	if(addr->sa_family == AF_INET) {
		struct sockaddr_in *in = (struct sockaddr_in*)addr;
		int val = in->sin_addr.s_addr;
		sock = g_hash_table_lookup(multi->out_sockets, &val);
	}

	return sock;
}

void
*sm_begin_polling(void *data)
{
	fprintf(stdout, "Polling\n");
	sm_multi_t *multi = (sm_multi_t*)data;
	struct epoll_event *events = calloc(64, sizeof(struct epoll_event));

	while(!(multi->command & SM_M_STOP)) {
		int n = epoll_wait(multi->epoll_d, events, 64, -1);
		fprintf(stdout, "Received info\n");
		int i = 0;

		for(; i < n; ++i) {
			ms_epoll_data *data = (ms_epoll_data*)events[i].data.ptr;
			fprintf(stdout, "calling\n");
			data->event_funct(events[i].events, data->data);
		}
	}

	fprintf(stdout, "Exiting polling loop\n");

	free(events);

	pthread_exit(NULL);
}





