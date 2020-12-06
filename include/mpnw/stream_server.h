#pragma once
#include "mpnw/socket.h"

/* Stream server instance handle */
struct StreamServer;
/* Stream session instance handle */
struct StreamSession;

/* Stream session message receive function */
typedef bool(*StreamSessionReceive)(
	struct StreamSession* session,
	const uint8_t* buffer,
	size_t count,
	void* argument);

/*
 * Creates a new stream server.
 * Returns stream server on success, otherwise null.
 *
 * TODO:
 * localAddress - pointer to the valid socket address.
 * acceptFunctions - pointer to the valid accept functions.
 * functionArgument - pointer to the server function argument.
 * receiveBufferSize - socket datagram receive buffer size.
 */
struct StreamServer* createStreamServer(
	const struct SocketAddress* localAddress,
	size_t sessionBufferSize,
	StreamSessionReceive* receiveFunctions,
	size_t receiveFunctionCount,
	size_t receiveTimeoutTime,
	void* functionArgument,
	size_t receiveBufferSize);

/*
 * Destroys specified stream server.
 * server - pointer to the valid stream server.
 */
void destroyStreamServer(
	struct StreamServer* server);

/*
 * Sends datagram to the specified session.
 * Returns true on success.
 *
 * session - pointer to the valid stream session.
 * buffer - pointer to the valid data buffer.
 * count - data buffer send byte count.
 */
bool streamSessionSend(
	struct StreamSession* session,
	const void* buffer,
	size_t count);
