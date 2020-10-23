#pragma once
#include "include.h"

namespace net
{
	// A templated thread safe queue. Used to store messages.
	template <typename T>
	class TsQueue
	{
	public:
		TsQueue() = default;
		TsQueue(const TsQueue<T>&) = delete;
		virtual ~TsQueue()
		{
			clear();
		}

	public:
		// Access the first element by const reference.
		const T& front()
		{
			std::scoped_lock lock(myMutex);
			return myDeque.front();
		}
		// Access the last element by const reference.
		const T& back()
		{
			std::scoped_lock lock(myMutex);
			return myDeque.back();
		}
		// Construct the element at the front of the queue.
		void push_front(const T& input)
		{
			std::scoped_lock lock(myMutex);
			myDeque.emplace_front(std::move(input));
		}
		// Construct the element at the back of the queue.
		void push_back(const T& input)
		{
			std::scoped_lock lock(myMutex);
			myDeque.emplace_back(std::move(input));
		}
		// Is the queue empty?
		bool empty()
		{
			std::scoped_lock lock(myMutex);
			return myDeque.empty();
		}
		// Gets the size of the queue (not sure if in bytes or as message count)?
		size_t count()
		{
			std::scoped_lock lock(myMutex);
			return myDeque.size();
		}
		// Clear the contents of the queue
		void clear()
		{
			std::scoped_lock lock(myMutex);
			return myDeque.clear();
		}
		// Remove and return the item from the front of the queue
		T pop_front()
		{
			std::scoped_lock lock(myMutex);
			auto item = std::move(myDeque.front());
			myDeque.pop_front();
			return item;
		}
		// Remove and return the item at the back of the queue.
		T pop_back()
		{
			std::scoped_lock lock(myMutex);
			auto item = std::move(myDeque.back());
			myDeque.pop_back();
			return item;
		}

	protected:
		// Mutex used for this queue
		std::mutex myMutex;
		// The underlying container
		std::deque<T> myDeque;
	};
}
