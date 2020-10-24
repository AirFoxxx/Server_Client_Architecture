#include <stdint.h>
#include "include.h"

// Test of includes

#include "connection.h"
#include "client.h"
#include "server.h"

// Defines a common message types as an enum, both client and server implementations can use this
// in lieu of a template argument and send messages specified by this enum class.

namespace net
{
	enum class eMsgTypes : uint16_t
	{
		ping,
		broadcast,
		empty
	};
}

// new type of message

// Our new derived server form the framework server.
template <typename T>
class MyServer : public net::server_interface<T>
{
public:
	MyServer(uint16_t port) : net::server_interface<T>(port)
	{
		if (!this->Start())
			this->Stop();
	}
public:
	bool OnClientConnect(std::shared_ptr<net::connection<T>> client) override
	{
		// We let all clients connect
		return true;
	}
	// Do something when a client disconnects
	void OnClientDisconnect(std::shared_ptr<net::connection<T>> client) override
	{
	}

	// Do something for specific message when Update() is called on all of them.
	void OnMessage(std::shared_ptr<net::connection<T>> client, net::sMessage<T>& message) override
	{
		// Handle message management
		switch (message.header.id)
			case net::eMsgTypes::ping:
		{
			std::cout << client->GetId() << " sent a message! " << std::endl;

			// send new message back
			net::sMessage<T> messageBack;
			uint32_t i = 15;
			messageBack << i;
			client->Send(messageBack);
		}
	}
private:
	// Place for custom containers for game server logic
};

int main()
{
	// create a server
	auto server = MyServer<net::eMsgTypes>(60000);
	// It is running now
	// hopefully it wont close until all threads join == we close the server.
	while (1)
	{
		server.Update();
	}
	return 0;
}
