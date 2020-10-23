#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <algorithm>
#include <functional>
#include <chrono>

int function1(int x, int y)
{
	int z = x / y;
	return z;
}

std::mutex mutex;

int main()
{
	//create shared variable count
	static int count = 0;
	//create a context for threads
	auto lambda = [&]()
	{
		for (int i = 1; i <= 1000000; ++i)
		{
			//take scoped control of a global mutex
			std::scoped_lock lock(mutex);

			++count;
		}
	};

	// bind function test;
	auto bindFunc = std::bind(function1, std::placeholders::_2, std::placeholders::_1);

	std::cout << bindFunc(5, 10) << std::endl;

	//new empty vector of threads
	std::vector<std::thread> threadGroup;

	//start timer
	auto start = std::chrono::steady_clock::now();

	//create threads in vector
	for (int i = 0; i < 50; ++i)
	{
		threadGroup.push_back(std::thread(lambda));
	}
	//do some operations in main program while threads are running.

	// wait for all threads to finish
	std::for_each(threadGroup.begin(), threadGroup.end(), [](std::thread& thread) {thread.join(); });

	//end timer
	auto end = std::chrono::steady_clock::now();

	// save result of first timer
	auto time1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

	//second attempt in this thread
	auto start1 = std::chrono::steady_clock::now();

	// test loop
	for (int i = 1; i <= 50000000; ++i)
	{
		++count;
	}

	//end timer
	auto end1 = std::chrono::steady_clock::now();

	// save result of first timer
	auto time2 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();

	// honestly, result should be better since there is no waiting for mutex and associated work.
	// Best method would be to have thread local storage for the million, and then add them up when they are done
	// to a global.
	std::cout << time1 << std::endl;
	std::cout << time2 << std::endl;

	std::cout << "count is: " << count << std::endl;
	std::cout << "which is: " << count / 1000000 << " Million!" << std::endl;
	std::cin.get();
	return 0;
}
