#include "mpnw/datagram_server.h"
#include "mpmt/thread.h"

#include <string.h>
#include <assert.h>

struct DatagramServer
{
	size_t receiveBufferSize;
	DatagramServerReceive receiveFunction;
	void* handle;
	uint8_t* receiveBuffer;
	Socket* receiveSocket;
	Thread* receiveThread;
	volatile bool threadRunning;
};

static void datagramServerReceiveHandler(
	void* argument)
{
	DatagramServer* server =
		(DatagramServer*)argument;
	DatagramServerReceive receiveFunction =
		server->receiveFunction;
	size_t receiveBufferSize =
		server->receiveBufferSize;
	uint8_t* receiveBuffer =
		server->receiveBuffer;
	Socket* receiveSocket =
		server->receiveSocket;

	bool result;
	size_t byteCount;

	SocketAddress* remoteAddress =
		createEmptySocketAddress();

	if (remoteAddress == NULL)
		return;

	while (server->threadRunning == true)
	{
		result = socketReceiveFrom(
			receiveSocket,
			receiveBuffer,
			receiveBufferSize,
			remoteAddress,
			&byteCount);

		if (result == false)
		{
			sleepThread(0.001);
			continue;
		}

		receiveFunction(
			server,
			remoteAddress,
			receiveBuffer,
			byteCount);
	}

	destroySocketAddress(remoteAddress);
	server->threadRunning = false;
}

DatagramServer* createDatagramServer(
	uint8_t addressFamily,
	const char* port,
	size_t receiveBufferSize,
	DatagramServerReceive receiveFunction,
	void* handle,
	SslContext* sslContext)
{
	assert(addressFamily < ADDRESS_FAMILY_COUNT);
	assert(port != NULL);
	assert(receiveBufferSize != 0);
	assert(receiveFunction != NULL);
	assert(isNetworkInitialized() == true);

	DatagramServer* server = malloc(
		sizeof(DatagramServer));

	if (server == NULL)
		return NULL;

	uint8_t* receiveBuffer = malloc(
		receiveBufferSize * sizeof(uint8_t));

	if (receiveBuffer == NULL)
	{
		free(server);
		return NULL;
	}

	SocketAddress* localAddress;

	if (addressFamily == IP_V4_ADDRESS_FAMILY)
	{
		localAddress = createSocketAddress(
			ANY_IP_ADDRESS_V4,
			port);
	}
	else if (addressFamily == IP_V6_ADDRESS_FAMILY)
	{
		localAddress = createSocketAddress(
			ANY_IP_ADDRESS_V6,
			port);
	}
	else
	{
		free(receiveBuffer);
		free(server);
		return NULL;
	}

	if (localAddress == NULL)
	{
		free(receiveBuffer);
		free(server);
		return NULL;
	}

	Socket* receiveSocket = createSocket(
		DATAGRAM_SOCKET_TYPE,
		addressFamily,
		localAddress,
		false,
		false,
		sslContext);

	destroySocketAddress(localAddress);

	if (receiveSocket == NULL)
	{
		free(receiveBuffer);
		free(server);
		return NULL;
	}

	server->receiveBufferSize = receiveBufferSize;
	server->receiveFunction = receiveFunction;
	server->handle = handle;
	server->receiveBuffer = receiveBuffer;
	server->receiveSocket = receiveSocket;
	server->threadRunning = true;

	Thread* receiveThread = createThread(
		datagramServerReceiveHandler,
		server);

	if (receiveThread == NULL)
	{
		destroySocket(receiveSocket);
		free(receiveBuffer);
		free(server);
		return NULL;
	}

	server->receiveThread = receiveThread;
	return server;
}

void destroyDatagramServer(DatagramServer* server)
{
	assert(isNetworkInitialized() == true);

	if (server == NULL)
		return;

	server->threadRunning = false;

	joinThread(server->receiveThread);
	destroyThread(server->receiveThread);

	shutdownSocket(
		server->receiveSocket,
		RECEIVE_SEND_SOCKET_SHUTDOWN);
	destroySocket(server->receiveSocket);

	free(server->receiveBuffer);
	free(server);
}

size_t getDatagramServerReceiveBufferSize(
	const DatagramServer* server)
{
	assert(server != NULL);
	return server->receiveBufferSize;
}

DatagramServerReceive getDatagramServerReceiveFunction(
	const DatagramServer* server)
{
	assert(server != NULL);
	return server->receiveFunction;
}

void* getDatagramServerHandle(
	const DatagramServer* server)
{
	assert(server != NULL);
	return server->handle;
}

Socket* getDatagramServerSocket(
	const DatagramServer* server)
{
	assert(server != NULL);
	return server->receiveSocket;
}

bool isDatagramServerRunning(
	const DatagramServer* server)
{
	assert(server != NULL);
	return server->threadRunning;
}

bool datagramServerSend(
	DatagramServer* server,
	const void* buffer,
	size_t count,
	const SocketAddress* address)
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
