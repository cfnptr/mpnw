#include "mpnw/datagram_server.h"
#include "mpmt/thread.h"

#include <assert.h>
#include <string.h>

struct DatagramServer
{
	DatagramServerReceive* receiveFunctions;
	size_t receiveFunctionCount;
	void* functionArgument;
	size_t receiveBufferSize;
	uint8_t* receiveBuffer;
	volatile bool threadRunning;
	struct Socket* receiveSocket;
	struct Thread* receiveThread;
};

static void datagramServerReceiveHandler(
	void* argument)
{
	assert(argument != NULL);

	struct DatagramServer* server =
		(struct DatagramServer*)argument;
	DatagramServerReceive* receiveFunctions =
		server->receiveFunctions;
	size_t receiveFunctionCount =
		server->receiveFunctionCount;
	void* functionArgument =
		server->functionArgument;
	size_t receiveBufferSize =
		server->receiveBufferSize;
	uint8_t* receiveBuffer =
		server->receiveBuffer;
	struct Socket* receiveSocket =
		server->receiveSocket;

	size_t byteCount;
	struct SocketAddress* remoteAddress;

	while (server->threadRunning == true)
	{
		bool result = socketReceiveFrom(
			receiveSocket,
			receiveBuffer,
			receiveBufferSize,
			&remoteAddress,
			&byteCount);

		if (result == false || byteCount == 0)
		{
			sleepThread(1);
			continue;
		}

		size_t functionIndex =
			(size_t)receiveBuffer[0];

		if (functionIndex < receiveFunctionCount)
		{
			DatagramServerReceive receiveFunction =
				receiveFunctions[functionIndex];

			receiveFunction(
				server,
				remoteAddress,
				receiveBuffer,
				byteCount,
				functionArgument);
		}

		destroySocketAddress(
			remoteAddress);
	}
}

struct DatagramServer* createDatagramServer(
	const struct SocketAddress* localAddress,
	const DatagramServerReceive* _receiveFunctions,
	size_t receiveFunctionCount,
	void* functionArgument,
	size_t receiveBufferSize)
{
	assert(localAddress != NULL);
	assert(_receiveFunctions != NULL);
	assert(receiveFunctionCount > 0);
	assert(receiveFunctionCount <= 256);
	assert(receiveBufferSize > 0);

	struct DatagramServer* server =
		malloc(sizeof(struct DatagramServer));

	if (server == NULL)
		return NULL;

	size_t receiveFunctionSize =
		receiveFunctionCount * sizeof(DatagramServerReceive);
	DatagramServerReceive* receiveFunctions = malloc(
		receiveFunctionSize);

	if (receiveFunctions == NULL)
	{
		free(server);
		return NULL;
	}

	memcpy(
		receiveFunctions,
		_receiveFunctions,
		receiveFunctionSize);

	uint8_t* receiveBuffer = malloc(
		receiveBufferSize * sizeof(uint8_t));

	if (receiveBuffer == NULL)
	{
		free(receiveFunctions);
		free(server);
		return NULL;
	}

	enum AddressFamily addressFamily;

	bool result = getSocketAddressFamily(
		localAddress,
		&addressFamily);

	if (result == false)
	{
		free(receiveBuffer);
		free(receiveFunctions);
		free(server);
		return NULL;
	}

	struct Socket* receiveSocket = createSocket(
		DATAGRAM_SOCKET_TYPE,
		addressFamily);

	if (receiveSocket == NULL)
	{
		free(receiveBuffer);
		free(receiveFunctions);
		free(server);
		return NULL;
	}

	result = bindSocket(
		receiveSocket,
		localAddress);

	if (result == false)
	{
		destroySocket(receiveSocket);
		free(receiveBuffer);
		free(receiveFunctions);
		free(server);
		return NULL;
	}

	server->receiveFunctions = receiveFunctions;
	server->receiveFunctionCount = receiveFunctionCount;
	server->functionArgument = functionArgument;
	server->receiveFunctionCount = receiveFunctionCount;
	server->receiveBufferSize = receiveBufferSize;
	server->receiveBuffer = receiveBuffer;
	server->threadRunning = true;
	server->receiveSocket = receiveSocket;

	struct Thread* receiveThread = createThread(
		datagramServerReceiveHandler,
		server);

	if (receiveThread == NULL)
	{
		destroySocket(receiveSocket);
		free(receiveBuffer);
		free(receiveFunctions);
		free(server);
		return NULL;
	}

	server->receiveThread = receiveThread;
	return server;
}

void destroyDatagramServer(
	struct DatagramServer* server)
{
	if (server == NULL)
		return;

	server->threadRunning = false;

	destroySocket(server->receiveSocket);
	joinThread(server->receiveThread);
	destroyThread(server->receiveThread);

	free(server->receiveBuffer);
	free(server->receiveFunctions);
	free(server);
}

bool datagramServerSend(
	struct DatagramServer* server,
	const void* buffer,
	size_t count,
	const struct SocketAddress* address)
{
	assert(server != NULL);
	assert(buffer != NULL);
	assert(count != 0);
	assert(address != NULL);

	return socketSendTo(
		server->receiveSocket,
		buffer,
		count,
		address);
}
