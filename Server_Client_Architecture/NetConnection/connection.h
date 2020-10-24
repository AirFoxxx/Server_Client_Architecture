#pragma once
#include "include.h"

namespace net
{
	// Templated class used to represent a connection object.
	// Every client has one connection object, every server has a std::deque of all connection objects to it.
	template <typename T>
	class connection : public std::enable_shared_from_this<connection<T>>
	{
	public:
		// Defines an owner of each connection. Different behavior exists for client or server connections.
		enum class owner
		{
			server,
			client
		};
		// A constructor, gets primarily called form server and client implementations.
		connection(owner parent, asio::io_context& asioContext,
			asio::ip::tcp::socket socket, TsQueue<sOwnedMessage<T>>& qIn)
			: m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
		{
			m_nOwnerType = parent;
		}

		virtual ~connection()
		{}

	public:
		// If this is a connection endpoint owned by the server, then we asign an ID to is to differentiate form serverSide,
		// and start waiting for Incoming messages form their client counterparts.
		void ConnectToClient(uint32_t uid = 0)
		{
			if (m_nOwnerType == owner::server)
			{
				if (m_socket.is_open())
				{
					id = uid;
					ReadHeader();
				}
			}
		}

		// Called from the client on a connection.
		// Sends a async connect request with this socket, that gets picked up by the server's acceptor.
		// It then primes this socket's context to start reading possible incoming messages from the server.
		bool ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
		{
			if (m_nOwnerType == owner::client)
			{
				asio::async_connect(m_socket, endpoints,
					[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
					{
						if (!ec)
						{
							ReadHeader();
						}
					});
			}
		}

		bool Disconnect()
		{
			if (IsConnected())
			{
				asio::post(m_asioContext, [this]() { m_socket.close(); });
			}
		}

		bool IsConnected() const
		{
			if (m_socket)
				return m_socket.is_open();
			else
				return false;
		}

		uint32_t GetId() const
		{
			return id;
		}

	public:
		// Posts a function to the context that checks, if we are currently already writing and sending a message,
		// and if not, it starts writing and sending it. Otherwise it saves it for later.
		void Send(const sMessage<T>& message)
		{
			asio::post(m_asioContext,
				[this, message]()
				{
					bool bWritingMessage = !m_qMessagesOut.empty();
					m_qMessagesOut.push_back(message);
					if (!bWritingMessage)
					{
						WriteHeader();
					}
				});
		}

	private:
		// Starts assynchronously reading a Header of a first message in temporary message in queue, if the body of message
		// is bigger than 0, we start reading the body. Otherwise we register another job to read header of the next message.
		void ReadHeader()
		{
			asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(sMessageHeader<T>)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_msgTemporaryIn.header.size > 0)
						{
							m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
							ReadBody();
						}
						// if it has no body...
						else
						{
							AddToIncomingMessageQueue();
						}
					}
					else
					{
						std::cout << " Read header fail!\n";
						m_socket.close();
					}
				});
		}

		// Starts asynchronously reading a body. We just add it to message queue.
		void ReadBody()
		{
			asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						AddToIncomingMessageQueue();
					}
					else
					{
						std::cout << " Read body fail!\n";
						m_socket.close();
					}
				});
		}

		// Starts assynchronously writing a Header of a first message in temporary message out queue, if the body of message
		// is bigger than 0, we start writing a body. Otherwise we register another job to write header of the next message.
		void WriteHeader()
		{
			asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(sMessageHeader<T>)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_qMessagesOut.front().body.size() > 0)
						{
							WriteBody();
						}
						else
						{
							m_qMessagesOut.pop_front();

							if (!m_qMessagesOut.empty())
							{
								WriteHeader();
							}
						}
					}
					else
					{
						std::cout << " Write Header fail!\n";
						m_socket.close();
					}
				});
		}
		// Starts asynchronously writing a body. If the Message out queue is not empty after the body is sent, we
		// register another WriteHeader() to start writing messages left.
		void WriteBody()
		{
			asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						m_qMessagesOut.pop_front();

						if (!m_qMessagesOut.empty())
						{
							WriteHeader();
						}
					}
					else
					{
						std::cout << " Write body fail!\n";
						m_socket.close();
					}
				});
		}
		// Adds the newly read message to appropriate containers.
		void AddToIncomingMessageQueue()
		{
			// If I am a server...
			if (m_nOwnerType == owner::server)
				// Then put this message in a ownedMessage container with a unique ptr to myself.
			{
				//sOwnedMessage<T> temp = { this->shared_from_this(), m_msgTemporaryIn };
				m_qMessagesIn.push_back({ this->shared_from_this(), m_msgTemporaryIn });
			}

			else
				// Do not assign my pointer to this message
			{
				//sOwnedMessage<T> temp = { nullptr, m_msgTemporaryIn };
				m_qMessagesIn.push_back({ nullptr, m_msgTemporaryIn });
			}

			// Prime the context with the next header to read.
			ReadHeader();
		}

	protected:
		// A socket for this connection
		asio::ip::tcp::socket m_socket;
		// A context for this connection
		asio::io_context& m_asioContext;
		// A thread safe queue to store outcoming messages.
		// These do not need to be associated with this connection object.
		TsQueue<sMessage<T>> m_qMessagesOut;
		// A thread safe queue to store outcoming messages.
		// This reference is passed in the constructor by the owner, so owner has this queue on him.
		// server propagates only 1 queue to its connections, so this is shared among them.
		// client propagates only 1 also, but clients are unique and each client owns its connection.
		TsQueue<sOwnedMessage<T>>& m_qMessagesIn;
		sMessage<T> m_msgTemporaryIn;
		// Definition of an owner of this connection object
		owner m_nOwnerType = owner::server;
		// Unique ID for connections created by the server, because there is more than one.
		uint32_t id = 0;
	};
}
