#include <iostream>
#include <future>
#include <thread>
#include <chrono>

int ThreadFunc()
{
	// 해당 쓰레드에서 4초 쉼
	std::this_thread::sleep_for(std::chrono_literals::operator""s(4ull));
	return 10;
}

int main()
{
	std::future<int> future = std::async(std::launch::async, &ThreadFunc);

	auto timePoint = std::chrono::steady_clock::now();

	// 4초 동안 딴 짓 가능함
	for (int i = 0; i < 100000; ++i)
	{
		for (int j = 0; j < 200000; ++j)
		{

		}
	}

	auto diffTimePoint = std::chrono::steady_clock::now() - timePoint;

	std::cout << "걸린 시간: " << std::chrono::duration<float>(diffTimePoint).count() << std::endl;
	std::cout << future.get(); // 비동기로 실행중
	
	return 0;
}