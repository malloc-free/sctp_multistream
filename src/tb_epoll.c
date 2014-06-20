/*
 * tb_epoll.c
 *
 *  Created on: 9/01/2014
 *      Author: michael
 */
#include "tb_epoll.h"
#include "tb_common.h"

#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

const int event_defaults = EPOLLIN | EPOLLET;

tb_epoll_t
*tb_create_e_poll(int sock_d, int max_events, int grp_events,
		void *data, func_event handler)
{
	int rc, e_id;

	e_id  = epoll_create1(0);

	if(e_id == -1)
	{
		perror("Cannot create epoll");
		return NULL;
	}

	tb_epoll_t *e_poll = malloc(sizeof(tb_epoll_t));
	e_poll->e_id = e_id;
	e_poll->max_events = max_events;
	e_poll->events = calloc(max_events, sizeof(struct epoll_event));
	e_poll->grp_events = malloc(sizeof(struct epoll_event));

	if(grp_events == 0)
	{
		e_poll->grp_events->events = event_defaults;
	}
	else
	{
		e_poll->grp_events->events = grp_events;
	}

	tb_edata_t *tb_data = malloc(sizeof(tb_edata_t));
	tb_data->data = data;
	tb_data->fd = sock_d;
	tb_data->f_event = handler;

	epoll_data_t e_data = { tb_data };

	e_poll->grp_events->data = e_data;

	rc = tb_add_socket(e_poll, sock_d, data);
	if(rc == -1)
	{
		perror("tb_create_e_poll: tb_add_socket");
		tb_destroy_epoll(e_poll);

		return NULL;
	}

	e_poll->w_time = -1;

	return e_poll;
}

void
tb_destroy_epoll(tb_epoll_t *e_poll)
{
	close(e_poll->e_id);
	free(e_poll->events);
	free(e_poll);
}

int
tb_add_socket(tb_epoll_t *e_poll, int sock_d, void *data)
{
	int rc, flags;

	flags = fcntl(sock_d, F_GETFL, 0);
	if(flags == -1)
	{
		perror("tb_add_socket: fctnl: F_GETFL");
		return -1;
	}

	flags |= O_NONBLOCK;

	rc = fcntl(sock_d, F_SETFL, flags);
	if(rc == -1)
	{
		perror("tb_add_socket: fcntl: F_SETFL");
		return -1;
	}

	struct epoll_event *events = malloc(sizeof(struct epoll_event));
	memcpy(events, e_poll->grp_events, sizeof(struct epoll_event));
	tb_edata_t *d = malloc(sizeof(tb_edata_t));

	d->data = ((tb_edata_t*)e_poll->grp_events->data.ptr)->data;
	d->fd = sock_d;

	if(data != NULL) {
		d->data = data;
	}

	events->data.ptr = d;

	rc = epoll_ctl(e_poll->e_id, EPOLL_CTL_ADD, sock_d,
			events);
	if(rc == -1)
	{
		perror("tb_add_socket: epoll_ctl");
		return -1;
	}

	return 0;
}

int
tb_epoll_for_events(tb_epoll_t *e_poll)
{
#ifdef _DEBUG_EPOLL
	PRT_ACK("Polling for events")
#endif

	int i;

	int num_events = epoll_wait(e_poll->e_id, e_poll->events,
			e_poll->max_events, e_poll->w_time);

#ifdef _DEBUG_EPOLL
	PRT_ACK("Polled!")
#endif

	for(i = 0; i < num_events; i++)
	{
		if(e_poll->events[i].events & EPOLLIN){
			tb_edata_t *data = (tb_edata_t*)e_poll->events[i].data.ptr;
			data->f_event(e_poll->events[i].events, data);
		}
		else if(e_poll->events[1].events & EPOLLHUP)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}

	return 0;
}


