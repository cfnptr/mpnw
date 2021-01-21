#include "mpnw/datagram_client.h"
#include "mpmt/thread.h"

#include <string.h>
#include <assert.h>

struct DatagramClient
{
	DatagramClientReceive receiveFunction;
	void* functionArgument;
	size_t receiveBufferSize;
	uint8_t* receiveBuffer;
	volatile bool threadRunning;
	struct Socket* receiveSocket;
	struct Thread* receiveThread;
};

static void datagramClientReceiveHandler(
	void* argument)
{
	struct DatagramClient* client =
		(struct DatagramClient*)argument;
	DatagramClientReceive receiveFunction =
		client->receiveFunction;
	void* functionArgument =
		client->functionArgument;
	size_t receiveBufferSize =
		client->receiveBufferSize;
	uint8_t* receiveBuffer =
		client->receiveBuffer;
	struct Socket* receiveSocket =
		client->receiveSocket;

	bool result;
	size_t byteCount;

	client->threadRunning = true;

	while (true)
	{
		result = socketReceive(
			receiveSocket,
			receiveBuffer,
			receiveBufferSize,
			&byteCount);

		if (result == false || byteCount == 0)
			break;

		result = receiveFunction(
			client,
			receiveBuffer,
			byteCount,
			functionArgument);

		if (result == false)
			break;
	}

	client->threadRunning = false;
}

struct DatagramClient* createDatagramClient(
	uint8_t addressFamily,
	struct SslContext* sslContext,
	const struct SocketAddress* remoteAddress,
	DatagramClientReceive receiveFunction,
	void* functionArgument,
	size_t receiveBufferSize)
{
	assert(remoteAddress != NULL);
	assert(receiveFunction != NULL);
	assert(receiveBufferSize != 0);

	struct DatagramClient* client = malloc(
		sizeof(struct DatagramClient));

	if (client == NULL)
		return NULL;

	uint8_t* receiveBuffer = malloc(
		receiveBufferSize * sizeof(uint8_t));

	if (receiveBuffer == NULL)
	{
		free(client);
		return NULL;
	}

	struct Socket* receiveSocket = createSocket(
		DATAGRAM_SOCKET_TYPE,
		addressFamily,
		sslContext);

	if (receiveSocket == NULL)
	{
		free(receiveBuffer);
		free(client);
		return NULL;
	}

	struct SocketAddress* localAddress;

	if (addressFamily == IP_V4_ADDRESS_FAMILY)
	{
		localAddress = createSocketAddress(
			ANY_IP_ADDRESS_V4,
			ANY_IP_ADDRESS_PORT);
	}
	else if (addressFamily == IP_V6_ADDRESS_FAMILY)
	{
		localAddress = createSocketAddress(
			ANY_IP_ADDRESS_V6,
			ANY_IP_ADDRESS_PORT);
	}
	else
	{
		abort();
	}

	if (localAddress == NULL)
	{
		destroySocket(receiveSocket);
		free(receiveBuffer);
		free(client);
		return NULL;
	}

	bool result = bindSocket(
		receiveSocket,
		localAddress);

	destroySocketAddress(
		localAddress);

	if (result == false)
	{
		destroySocket(receiveSocket);
		free(receiveBuffer);
		free(client);
		return NULL;
	}

	result = connectSocket(
		receiveSocket,
		remoteAddress);

	if (result == false)
	{
		destroySocket(receiveSocket);
		free(receiveBuffer);
		free(client);
		return NULL;
	}

	client->receiveFunction = receiveFunction;
	client->functionArgument = functionArgument;
	client->receiveBufferSize = receiveBufferSize;
	client->receiveBuffer = receiveBuffer;
	client->threadRunning = false;
	client->receiveSocket = receiveSocket;

	struct Thread* receiveThread = createThread(
		datagramClientReceiveHandler,
		client);

	if (receiveThread == NULL)
	{
		destroySocket(receiveSocket);
		free(receiveBuffer);
		free(client);
		return NULL;
	}

	client->receiveThread = receiveThread;
	return client;
}

void destroyDatagramClient(
	struct DatagramClient* client)
{
	if (client == NULL)
		return;

	destroySocket(client->receiveSocket);
	joinThread(client->receiveThread);
	destroyThread(client->receiveThread);

	free(client->receiveBuffer);
	free(client);
}

bool getDatagramClientRunning(
	const struct DatagramClient* client)
{
	assert(client != NULL);
	return client->threadRunning;
}

bool datagramClientSend(
	struct DatagramClient* client,
	const void* buffer,
	size_t count)
{
	assert(client != NULL);
	assert(buffer != NULL);
	assert(count != 0);

	return socketSend(
		client->receiveSocket,
		buffer,
		count);
}
