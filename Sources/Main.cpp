#include <cstdio>

#include <iostream>
#include <thread>

void ThreadFunc()
{
	std::cout << "Thread Id( " << std::this_thread::get_id() << " )";
}

int main()
{
	std::cout << "Thread hardware_concurrency(" << std::thread::hardware_concurrency() << ")\n";

	std::thread t1(ThreadFunc);

	for (int i = 0; i < 60; ++i)
	{
		printf("%d\n", i);
	}

	t1.join();

	return 0;
}