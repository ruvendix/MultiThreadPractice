#include <iostream>
#include <future>
#include <thread>
#include <chrono>

int ThreadFunc()
{
	// �ش� �����忡�� 4�� ��
	std::this_thread::sleep_for(std::chrono_literals::operator""s(4ull));
	return 10;
}

int main()
{
	std::future<int> future = std::async(std::launch::async, &ThreadFunc);

	auto timePoint = std::chrono::steady_clock::now();

	// 4�� ���� �� �� ������
	for (int i = 0; i < 100000; ++i)
	{
		for (int j = 0; j < 200000; ++j)
		{

		}
	}

	auto diffTimePoint = std::chrono::steady_clock::now() - timePoint;

	std::cout << "�ɸ� �ð�: " << std::chrono::duration<float>(diffTimePoint).count() << std::endl;
	std::cout << future.get(); // �񵿱�� ������
	
	return 0;
}