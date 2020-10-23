#pragma once
#include "include.h"

namespace net
{
	// Templated header for a typical message. Will need to be passed in a tape of the message
	// Contains the ID and size in bytes
	template <typename T>
	struct sMessageHeader
	{
		T id;
		uint16_t size = 0;
	};

	// Templated message comprised of a Message header and a body
	template <typename T>
	struct sMessage
	{
		sMessageHeader<T> header{};
		std::vector<uint8_t> body;

		// returns size of a message including header and body.
		size_t size() const
		{
			return sizeof(sMessageHeader<T> +body.size());
		}

		// Overloaded operator used to put Trivial data types into the message buffer
		template <typename DataType>
		friend sMessage<T>& operator <<(sMessage<T>& msg, const DataType& data)
		{
			size_t i = msg.body.size();

			msg.body.resize(msg.body.size() + sizeof(DataType));

			std::memcpy(msg.body.data() + i, &data, sizeof(DataType));

			msg.header.size() = msg.size();

			return msg;
		}

		// Overloaded operator used to retrieve trivial data types form the message buffer
		template <typename DataType>
		friend sMessage<T>& operator >> (sMessage<T>& msg, const DataType& data)
		{
			size_t i = msg.body.size();

			std::memcpy(&data, msg.body.data() + i - sizeof(DataType), sizeof(DataType));

			msg.body.resize(i - sizeof(DataType));

			msg.header.size() = msg.size();

			return msg;
		}
	};

	// Owned message type containing a pointer to a connection object associated with this Message
	// Used to determine a client that sent this message
	template <typename T>
	struct sOwnedMessage
	{
		std::unique_ptr<connection<T>> remote = nullptr;
		sMessage<T> message;
	};
}
