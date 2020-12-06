#pragma once
#include "mpnw/socket.h"

/* Datagram server instance handle */
struct DatagramServer;

/* Datagram server datagram receive function */
typedef void(*DatagramServerReceive)(
	struct DatagramServer* server,
	const struct SocketAddress* address,
	const uint8_t* buffer,
	size_t count,
	void* argument);

/*
 * Creates a new datagram server.
 * Returns datagram server on success, otherwise null.
 *
 * addressFamily - local datagram socket address family.
 * port - pointer to the valid local address port string.
 * receiveFunctions - pointer to the valid receive functions.
 * receiveFunctionCount - receive function array item count.
 * functionArgument - pointer to the server function argument.
 * receiveBufferSize - socket datagram receive buffer size.
 */
struct DatagramServer* createDatagramServer(
	enum AddressFamily addressFamily,
	const char* port,
	const DatagramServerReceive* receiveFunctions,
	size_t receiveFunctionCount,
	void* functionArgument,
	size_t receiveBufferSize);

/*
 * Destroys specified datagram server.
 * server - pointer to the valid datagram server.
 */
void destroyDatagramServer(
	struct DatagramServer* server);

/*
 * Sends datagram to the specified address.
 * Returns true on success.
 *
 * server - pointer to the valid datagram server.
 * buffer - pointer to the valid data buffer.
 * count - data buffer send byte count.
 * address - destination datagram address.
 */
bool datagramServerSend(
	struct DatagramServer* server,
	const void* buffer,
	size_t count,
	const struct SocketAddress* address);