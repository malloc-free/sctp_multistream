/*
 * sctp_multi_common.h
 *
 *  Created on: 5/05/2014
 *      Author: michael
 */

#ifndef SCTP_MULTI_COMMON_H_
#define SCTP_MULTI_COMMON_H_

#define PIPE_READ 0
#define PIPE_WRITE 1
#define sock_t int
#define md_t int
#define stream_t int
#define pipe_t int
#define event_t int

typedef void (*epoll_funct)(int events, void *data);

typedef struct {
	epoll_funct event_funct;
	void *data;
}
ms_epoll_data;

#endif /* SCTP_MULTI_COMMON_H_ */
