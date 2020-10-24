#pragma once
#include "commonMessageTypes.h"

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
