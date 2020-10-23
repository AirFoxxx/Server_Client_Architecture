#pragma once
#include "include.h"
#include "connection.h"
#include "tsQueue.h"
#include "Message.h"

namespace net
{
	// A client interface implementation that owns one connection object, its own socket,
	// a thread to read messages continuously and a thread safe queue of incoming ownedMessages.
	// This class only takes care of connecting / disconnecting the client to / from a server
	// and then leaves it all to the Connection object.
	template <typename T>
	class client_interface
	{
		client_interface() : m_socket(m_context)
		{
		}

		~client_interface()
		{
			Disconnect();
		}

	public:
		bool Connect(const std::string& host, const uint16_t port)
		{
			try
			{
				// Create the connection.
				m_connection = std::make_unique<connection<T>>(
					connection<T>::owner::client,
					m_context,
					asio::ip::tcp::socket(m_context), m_qMessagesIn);

				// resolve the address passed in.
				asio::ip::tcp::resolver resolver(m_context);
				asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));
				// connect to server
				m_connection->ConnectToServer(endpoints);

				// Create the thread that will continuosly execute operations on the stack.
				thrContext = std::thread([this]() { m_context.run(); });
			}
			catch (std::exception& e)
			{
				std::cerr << "Client exception: " << e.what() << "\n"
					return false;
			}
			return true;
		}

		// Disconnects from the server if connected
		void Disconnect()
		{
			if (IsConnected())
			{
				m_connection->Disconnect();
			}

			m_context.stop();

			// Close the running thread
			if (thrContext.joinable())
				thrContext.join();
		}

		bool IsConnected()
		{
			if (m_connection)
				return m_connection->IsConnected();
			else
				return false;
		}

		// returns the thread safe queue of incoming messages.
		TsQueue<sOwnedMessage<T>>& Incoming()
		{
			return m_qMessagesIn;
		}

	protected:
		// asio context handles the data transfer
		asio::io_context m_context;
		// but it needs a thread to run in
		std::thread thrContext;
		// This is the hardware socket that is connected to the server.
		// It gets passed to connection object via constructor
		asio::ip::tcp::socket m_socket;
		// the client has a single instance of a connection object (this class), which handles the data transfer
		std::unique_ptr<connection<T>> m_connection;
	private:
		// Thread safe queue for all message Objects. These are Owned messages, for they can come form the server and other clients?
		// Also this is different from the queues that are inside connection object. So is this even used?
		TsQueue<sOwnedMessage<T>> m_qMessagesIn;
	};
}
