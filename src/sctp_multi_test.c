/*
 * sctp_multi_test.c
 *
 *  Created on: 5/05/2014
 *      Author: michael
 */
#include <netinet/in.h>

#include "sctp_association.h"
#include "sctp_multi.h"
#include "sctp_association.h"
#include "tb_common.h"
#include "sctp_socket.h"

#include <glib.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MS_ASSERT(b, m) if(!(b)){ fprintf(stderr, "Failed: %s\n", m); return FALSE; }
#define MS_M_ASSERT(b) \
			if(!b) { \
				fprintf(stderr, "Failed test:" #b "\n"); \
				num_fail++; \
			} \
			else{ \
				fprintf(stderr, "Passed test:" #b "\n"); \
				num_pass++; \
			}

#define MS_CLOSE() \
		fprintf(stdout, "Number Passed: %d\n", num_pass); \
		fprintf(stderr, "Number Failed: %d\n", num_fail)

#define MS_SETUP() int num_fail = 0; int num_pass = 0

#define TEST(N) gboolean N(); \
	\
		gboolean N() \
		{ \
			fprintf(stdout, #N "\n");

#define MS_ADDR(A, P) \
		struct addrinfo hints, *info; \
		memset(&hints, 0, sizeof(struct addrinfo)); \
		hints.ai_family = AF_INET; \
		hints.ai_socktype = SOCK_STREAM; \
		hints.ai_protocol = IPPROTO_SCTP; \
		rc = getaddrinfo(A, P, &hints, &info); \
		if(rc != 0) { \
			fprintf(stderr, "Error: getaddrinfo: %s\n", \
					gai_strerror(rc)); \
		} \
		MS_ASSERT(rc == 0, "getaddrinfo = 0") \

typedef struct
{
	pthread_t thread;
	void *info;
}
test_thread_t;

typedef struct
{
	sm_multi_t *multi;
	int s;
}
thread_info_t;

void
*t_accept(void *data);

TEST(test_sm_multi_create)
	sm_multi_t *multi = sm_multi_create();
	assert(multi != NULL);
	sm_multi_destroy(multi);

	return TRUE;
}

TEST(test_sm_sock_subscribe)
	PRT_INFO_B("Creating socket");
	sm_sock_t *sock = sm_sock_create(-1);
	MS_ASSERT(sock != NULL, "sock == NULL");
	MS_ASSERT(sock->sd != -1, "sock->sd == -1");
	PRT_INFO_B("Testing subscribe");
	MS_ASSERT(sm_sock_events_subscribe(sock) == 0, "event subscribe fail");
	PRT_INFO_B("Destroying socket");
	sm_sock_destroy(sock);

	return TRUE;
}

TEST(test_sm_socket)
	sm_multi_t *multi = sm_multi_create();
	md_t md = sm_socket(multi);

	sm_desc_t *desc = g_hash_table_lookup(multi->desc, &md);

	MS_ASSERT(g_hash_table_size(multi->desc) == 1, "size of hashtable != 1");
	MS_ASSERT(desc != NULL, "desc != NULL");
	MS_ASSERT(desc->in_event == md, "desc->event != to file descriptor");

	sm_multi_destroy(multi);

	return TRUE;
}

TEST(test_sm_insert_as_addr)
	sm_multi_t *multi = sm_multi_create();
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, "192.168.0.10", &addr.sin_addr);
	sm_sock_t *sock = sm_sock_create(-1);
	sm_as_t *as = sm_as_create(sock->reg_str);
	sm_insert_in_socket(multi, as, (struct sockaddr*)&addr, sizeof(addr));

	assert(g_hash_table_size(multi->in_sockets) == 1);

	sm_multi_destroy(multi);

	return TRUE;
}

TEST(test_sm_get_as_addr)
	sm_multi_t *multi = sm_multi_create();
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, "192.168.0.10", &addr.sin_addr);
	sm_sock_t *soc = sm_sock_create(-1);
	sm_as_t *as = sm_as_create(soc->reg_str);
	sm_insert_in_socket(multi, as, (struct sockaddr*)&addr, sizeof(addr));

	assert(g_hash_table_size(multi->in_sockets) == 1);

	sm_as_t *as2 = sm_get_in_sock(multi, (struct sockaddr*)&addr,
			sizeof(addr));

	assert(as2 != NULL);

	sm_multi_destroy(multi);

	return TRUE;
}

TEST(test_sm_bind)
	int rc;
	PRT_INFO_B("Creating multi");
	sm_multi_t *multi = sm_multi_create();
	assert(multi != NULL);
	PRT_INFO_B("Getting sockets");
	sock_t s = sm_socket(multi);
	sock_t s2 = sm_socket(multi);

	assert(s != -1);
	assert(s2 != -1);
	assert(s != s2);

	struct addrinfo hints, *info;

	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_SCTP;

	rc = getaddrinfo("192.168.0.10", "5000", &hints, &info);

	if(rc != 0) {
		fprintf(stderr, "Error: getaddrinfo: %s\n",
				gai_strerror(rc));
	}

	assert(rc == 0);

	PRT_INFO_B("Binding socket");
	rc = sm_bind(multi, s, (struct sockaddr*)info->ai_addr,
			info->ai_addrlen, 0);

	if(rc != 0) {
		perror("bind");
	}

	assert(rc == 0);


	PRT_INFO_B("Getting descriptors");
	sm_desc_t *desc = sm_get_desc(multi, s);
	MS_ASSERT(desc != NULL, "desc != NULL");
	MS_ASSERT(desc->sock != NULL, "desc->sock stream != NULL");
	MS_ASSERT(desc->sock->status == SM_SK_BOUND, "sock status != BOUND");
	PRT_INFO_B("Destroying multi");
	sm_multi_destroy(multi);

	return TRUE;
}

TEST(test_sm_listen)
	int rc;

	sm_multi_t *multi = sm_multi_create();
	assert(multi != NULL);

	sock_t s = sm_socket(multi);

	assert(s != -1);

	MS_ADDR("192.168.0.10", "5001");

	rc = sm_bind(multi, s, (struct sockaddr*)info->ai_addr,
			info->ai_addrlen, 0);

	if(rc != 0) {
		perror("bind");
	}

	MS_ASSERT(rc == 0, "sm_bind not returning 0");
	sm_desc_t *desc = sm_get_desc(multi, s);
	MS_ASSERT(desc != NULL, "s == NULL");
	MS_ASSERT(desc->sock != NULL, "desc->sock == NULL");
	MS_ASSERT(desc->sock->status == SM_SK_BOUND, "sock status != BOUND");

	rc = sm_listen(multi, s, 10);

	if(rc != 0) {
		perror("listen");
	}

	MS_ASSERT(rc == 0, "sm_listen not returning 0");
	MS_ASSERT(desc->sock->status == SM_SK_LISTENING, "sock status != LISTENING");

	sm_multi_destroy(multi);

	return TRUE;
}

TEST(test_sm_accept)
	int rc;

	sm_multi_t *multi = sm_multi_create();
	assert(multi != NULL);

	sock_t s = sm_socket(multi);

	assert(s != -1);

	MS_ADDR("192.168.0.10", "5002");

	rc = sm_bind(multi, s, (struct sockaddr*)info->ai_addr,
			info->ai_addrlen, 0);

	if(rc != 0) {
		perror("bind");
	}

	MS_ASSERT(rc == 0, "sm_bind not returning 0");
	sm_desc_t *desc = sm_get_desc(multi, s);
	MS_ASSERT(desc != NULL, "s == NULL");
	MS_ASSERT(desc->sock != NULL, "desc->sock == NULL");
	MS_ASSERT(desc->sock->status == SM_SK_BOUND, "sock status != BOUND");

	rc = sm_listen(multi, s, 10);

	if(rc != 0) {
		perror("listen");
	}

	MS_ASSERT(rc == 0, "sm_listen not returning 0");
	MS_ASSERT(desc->sock->status == SM_SK_LISTENING, "sock status != LISTENING");

	PRT_INFO_B("Creating accept thread");
	thread_info_t i_t = { multi, s };
	pthread_t p;
	pthread_create(&p, NULL, &t_accept, (void*)&i_t);

	PRT_INFO_B("Creating second multi");
	sm_multi_t *multi2 = sm_multi_create();
	MS_ASSERT(multi2 != NULL, "multi2 == NULL");

	sock_t s2 = sm_socket(multi2);
	MS_ASSERT(s != -1, "s2 == -1");

	sm_as_t *as1 = sm_get_in_sock(multi2, info->ai_addr, info->ai_addrlen);
	MS_ASSERT(as1 == NULL, "as1 != NULL");

	PRT_INFO_B("Attempting to connect");
	rc = sm_connect(multi2, s2, (struct sockaddr*)info->ai_addr,
			info->ai_addrlen, 0);

	if(rc == -1) {
		perror("connect");
	}

	if(rc == -1) {
		perror("Error on connect: ");
	}

	MS_ASSERT(rc != -1, "connect returning -1");
	PRT_INFO_B("Connected");

	int *val;
	pthread_join(p, (void**)&val);
	MS_ASSERT(*val == TRUE, "thread not returning true");
	free(val);
	sm_multi_destroy(multi);

	return TRUE;
}

void
*t_accept(void *data)
{
	PRT_INFO_B("Waiting to accept");
	thread_info_t *info = (thread_info_t*)data;
	int rc;
	gboolean *val = malloc(sizeof(gboolean));
	*val = FALSE;

	rc = sm_accept(info->multi, info->s);
	PRT_INFO_B("Something accepted");
	*val = (rc != -1);

	pthread_exit(val);
}

//
//gboolean
//test_sm_recv()
//{
//	MS_ASSERT(0, "test_sm_recv");
//
//	return TRUE;
//}
//
//gboolean
//test_sm_send()
//{
//	MS_ASSERT(0, "test_sm_send");
//
//	return TRUE;
//}
//
//gboolean
//test_sm_connect()
//{
//	MS_ASSERT(0, "test_sm_connect");
//
//	return TRUE;
//}
//
//gboolean
//test_sm_close()
//{
//	MS_ASSERT(0, "test_sm_close");
//
//	return TRUE;
//}

int
main(void)
{
	MS_SETUP();
	MS_M_ASSERT(test_sm_multi_create());
	MS_M_ASSERT(test_sm_sock_subscribe());
	MS_M_ASSERT(test_sm_socket());
	MS_M_ASSERT(test_sm_insert_as_addr());
	MS_M_ASSERT(test_sm_get_as_addr());
	MS_M_ASSERT(test_sm_bind());
	MS_M_ASSERT(test_sm_listen());
	MS_M_ASSERT(test_sm_accept());
//
	MS_CLOSE();
	return 0;
}






