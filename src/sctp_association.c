/*
 * sctp_association.c
 *
 *  Created on: 5/05/2014
 *      Author: michael
 */

#include "sctp_association.h"
#include "tb_epoll.h"
#include "tb_common.h"
#include "sctp_socket.h"

#include <glib.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <sys/epoll.h>

sm_as_t
*sm_as_create(GHashTable *streams)
{
	sm_as_t *as = malloc(sizeof(sm_as_t));
	as->status = SMA_CREATED;
	as->child_streams = streams;
	as->epoll_d = epoll_create1(0);

	return as;
}

void
sm_as_destroy(sm_as_t *as)
{
	g_hash_table_destroy(as->child_streams);
	free(as);
}

void
sm_as_destroy_key(gpointer data)
{
	free(data);
}

void
sm_as_in_event(int events, void *data) {
	sm_as_t *as = (sm_as_t*)data;
	int rc, flags;
	struct sctp_sndrcvinfo info;
	char buffer[128];
	rc = sctp_recvmsg(as->sd, buffer, sizeof(buffer),
			NULL, 0, &info, &flags);

	if(rc != -1) {
		sm_desc_t *desc = g_hash_table_lookup(as->child_streams,
				&info.sinfo_stream);

		if(desc != NULL) {
			write(desc->in_event, "hello", 6);

			write(desc->in_pipe, buffer, rc);
		}
		else {
			fprintf(stdout, "error, stream not found\n");
		}
	}
}

void
sm_as_out_event(int events, void *data) {
	sm_desc_t *desc = data;
	sm_as_t *as = data + 1;
	int rc;

	char buffer[128];

	read(desc->out_pipe, buffer, sizeof(buffer));

	rc = sctp_sendmsg(as->sd, buffer, sizeof(buffer), NULL, 0, 0, 0,
			desc->s_num, 0, 0);

	if(rc == -1) {
		perror("Error: sm_as_out_event");
	}
}

void
sm_as_create_md(sm_desc_t *desc) {
	sm_desc_t *n_desc = sm_desc_create();
	n_desc->parent = desc;
	g_queue_push_head(desc->desc_queue, n_desc);
}

