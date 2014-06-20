/*
 * sctp_association.h
 *
 *  Created on: 5/05/2014
 *      Author: michael
 */

#ifndef SCTP_ASSOCIATION_H_
#define SCTP_ASSOCIATION_H_

#include "sctp_multi_common.h"
#include "tb_epoll.h"

#include <glib.h>
#include <sys/socket.h>
#include <pthread.h>

#define DEFAULT_MAX_ST 32

/**
 * @enum
 *
 * @brief Specifies the current status of the Association.
 */
typedef enum
{
	SMA_CREATED = 0,
	SMA_CONNECTED = 1,
	SMA_BOUND = 2,
	SMA_LISTENING = 4,
	SMA_ACCEPT = 8,
	SMA_CLOSED = 16,
	SMA_ERROR = 32
}
sm_association_status_t;

/**
 * @enum
 *
 * @brief Specifies if any threads used by the Association should terminate.
 */
typedef enum
{
	LIS_CONTINUE,
	LIS_EXIT
}
sm_command_t;

/**
 * @struct <sctp_as_t> [sctp_association.h]
 *
 * This struct outlines the data required for an association.
 */
typedef struct
{
	GHashTable *child_streams; 	///< A hash table mapping stream to descriptor.
								///< these are used for actual communication over
								///< streams and are derived when a connection is
								///< made.
	GHashTable *parent_streams; ///< A hash table mapping stream to parent descriptor.
								///< Used on the server side, they are used when
								///< a new stream connection is made, and are
								///< used to derive child stream descriptors.
	int ht_key; 				///< Key for association map
	int sd;						///< The actual linux generated socket descriptor
								///< For this association.
	sm_association_status_t status; //Current status of association.
	int epoll_d;				///< The epoll descriptor for the association.
	sm_command_t command;		///< Command to stop any threads used by an association.
	ms_epoll_data e_data;		///< The epoll data used for this association.
}
sm_as_t;

/**
 * @brief Create a new association, with the provided streams.
 *
 * Creates a new sm_as_t struct, and allocates the necessary memory.
 *
 * @param streams The streams hash table to use with this association.
 */
sm_as_t
*sm_as_create(GHashTable *streams);

/**
 * @brief Destroy an association.
 *
 * Destroys an association, freeing memory.
 */
void
sm_as_destroy();

/**
 * @brief Destroy a key used in the hash table.
 *
 * Used by both child_streams and parent_streams to destroy keys.
 *
 * @param data The key to destroy.
 */
void
sm_as_destroy_key(gpointer data);

/**
 * @brief Called when an incoming event is caught by epoll.
 */
void
sm_as_in_event(int events, void *data);

void
sm_as_out_event(int events, void *data);

void
sm_as_create_md(sm_desc_t *desc);

#endif /* SCTP_ASSOCIATION_H_ */
