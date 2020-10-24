#pragma once
#include "include.h"

namespace net
{
	// The server interface that can be started and ended, it accepts an acceptor through a constructor,
	// and then waits for connections of clients, and for all these connections creates a
	// connection object endpoint owned by the server, used to communicate with the other side.
	template <typename T>
	class server_interface
	{
	public:
		server_interface(uint16_t port)
			: m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
		{
		}

		virtual ~server_interface()
		{
			Stop();
		}

		// Start the server
		bool Start()
		{
			try
			{
				// Issue a task to do before starting the worker
				WaitForClientConnection();

				m_threadContext = std::thread([this]() {m_asioContext.run(); });
			}
			catch (std::exception e)
			{
				std::cerr << "Server exception: " << e.what() << "\n";
				return false;
			}

			std::cout << "Server started!\n";
			return true;
		}
		// Stop the server, drops all clients and its thread.
		void Stop()
		{
			m_asioContext.stop();

			if (m_threadContext.joinable())
				m_threadContext.join();

			std::cout << "Server stopped!\n";
		}

		/// <summary>
		/// This behemoth primes the context with a function to { wait for client connections, and then
		/// creates a new shared pointer to this incoming connection, creates an ID for it and
		/// puts it in the deque of all held connections to clients.
		/// Once this is done, it reprimes the context with the same function}.
		/// </summary>
		void WaitForClientConnection()
		{
			m_asioAcceptor.async_accept(
				[this](std::error_code ec, asio::ip::tcp::socket socket)
				{
					if (!ec)
					{
						std::cout << "Server - New connection: " << socket.remote_endpoint() << "\n";
						// Initialize new connection object, set its parent to server
						std::shared_ptr<connection<T>> newconn =
							std::make_shared<connection<T>>(connection<T>::owner::server,
								m_asioContext, std::move(socket), m_qMessagesIn);

						// deny a connection happens here
						if (OnClientConnect(newconn))
						{
							// add it to the deque of connection objects;
							m_deqConnections.push_back(std::move(newconn));
							// Call the connectToClient function on this connection.
							// It assigns a new unique ID to this connection, and primes the context
							// of this socket associated with this connection with the ReadHeader()
							// function, so it starts reading incoming messages on this socket.
							m_deqConnections.back()->ConnectToClient(nIDCounter++);

							std::cout << "ID: " << m_deqConnections.back()->GetId() << " Connection Approved!\n";
						}
						else
						{
							std::cout << " Connection denied!\n";
						}
					}
					else
					{
						std::cout << "Server - New connection error: " << ec.message() << "\n";
					}

					// this puts another work to do for the worker of our server on the stack, so we can anticipate another connection
					WaitForClientConnection();
				});
		}

		/// <summary>
		/// Sends a message to the supplied client Connection object.
		/// </summary>
		/// <param name="client">The client connection object Unique_ptr</param>
		/// <param name="message">The message to send</param>
		void MessageClient(std::shared_ptr<connection<T>> client, const sMessage<T>& message)
		{
			if (client && client->IsConnected())
			{
				client->Send(message);
			}
			else
			{
				// If we cannot send for some reason, client is considered AWOL and cleanup ensues.
				OnClientDisconnect(client);
				client.reset();
				m_deqConnections.erase(
					std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
			}
		}

		/// <summary>
		/// Sends a global message to all clients, can ignore supplied client and will ignore all nullptr clients.
		/// </summary>
		/// <param name="message">The message to send to everyone</param>
		/// <param name="pIgnoreClient">The ignored client.</param>
		void MessageAllClients(const sMessage<T>& message, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
		{
			bool bInvalidClientExists = false;

			for (auto& client : m_deqConnections)
			{
				if (client && client->IsConnected())
				{
					if (client != pIgnoreClient)
						client->Send(message);
				}
				else
				{
					// Invalid client gets recognized, we do not send a message to this one
					OnClientDisconnect(client);
					client.reset();
					bInvalidClientExists = true;
				}
			}

			if (bInvalidClientExists)
				// and cleanup of invalid client
				m_deqConnections.erase(
					std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
		}

	protected:
		// Do something when a client connects. Filters the IP address or similar things
		virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
		{
			return false;
		}
		// Do something when a client disconnects
		virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
		{
		}
	public:
		/// <summary>
		/// This function is called in a loop from your Main() function to keep server running.
		/// Goes through all the obtained messages, calls their respective handler.
		/// Meant to be used from the outside.
		/// </summary>
		/// <param name="nMaxMessages">The n maximum messages.</param>
		void Update(size_t nMaxMessages = -1)
		{
			size_t nMessagecount = 0;
			while (nMessagecount < nMaxMessages && !m_qMessagesIn.empty())
			{
				auto msg = m_qMessagesIn.pop_front();

				OnMessage(msg.remote, msg.message);

				nMessagecount++;
			}
		}
	protected:
		// Do something for specific message when Update() is called on all of them.
		virtual void OnMessage(std::shared_ptr<connection<T>> client, sMessage<T>& message)
		{
		}

	protected:
		// Thread safe queue for incoming message packets
		TsQueue<sOwnedMessage<T>> m_qMessagesIn;
		// deque of connections that came in
		std::deque<std::shared_ptr<connection<T>>> m_deqConnections;
		// Server specific context
		asio::io_context m_asioContext;
		// Server owned thread for the context.
		std::thread m_threadContext;
		// The acceptor object that will be filled with a function to handle incoming connections form clients.
		asio::ip::tcp::acceptor m_asioAcceptor;

		// clients will be represented by a unique ID
		uint32_t nIDCounter = 10000;
	};
}
