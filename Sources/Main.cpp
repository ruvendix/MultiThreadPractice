#include <cstdio>

#include <iostream>
#include <thread>

int g_sharedNum = 0;

void ThreadFunc()
{
	for (int i = 0; i < 100000; ++i)
	{
		++g_sharedNum;
	}
}

int main()
{
	std::cout << "Thread hardware_concurrency(" << std::thread::hardware_concurrency() << ")\n";

	std::thread t1(ThreadFunc);
	std::thread t2(ThreadFunc);

	t1.join();
	t2.join();

	std::cout << "shared num: " << g_sharedNum << std::endl;
	return 0;
}