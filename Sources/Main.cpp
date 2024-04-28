#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

int g_num = 0;

// 락프리
class SpinLock
{
public:
	SpinLock() = default;
	~SpinLock() = default;
	
	void Lock()
	{
		bool bExpected = false;
		bool bDesire = true;

		/*
		m_lockFlag와 bExpected가 같으면 m_lockFlag를 bDesire로 설정하고 true 반환
		m_lockFlag와 bExpected가 다르면 bExpected를 m_lockFlag로 설정하고 false 반환
		이러한 구조로 스핀락을 구현할 때는 스핀이 false를 반환하면 bExpected를 false로 넣어야함
		*/
		while (m_lockFlag.compare_exchange_strong(bExpected, bDesire) == false)
		{
			bExpected = false;
		}
	}

	void Unlock()
	{
		m_lockFlag.store(false);
	}

private:
	std::atomic<bool> m_lockFlag = false;
};

void LockThreadFunc(std::mutex* pMutex)
{
	std::lock_guard<std::mutex> lock(*pMutex);
	for (int i = 0; i < 10000000; ++i)
	{
		++g_num;
	}
}

void SpinLockThreadFunc(SpinLock* pSpinLock)
{	
	pSpinLock->Lock();
	for (int i = 0; i < 10000000; ++i)
	{
		++g_num;
	}
	pSpinLock->Unlock();
}

int main()
{
#pragma region 락
	auto lockStartTime = std::chrono::steady_clock::now();

	std::mutex mutex;
	std::thread lockThread1(LockThreadFunc, &mutex);
	std::thread lockThread2(LockThreadFunc, &mutex);

	lockThread1.join();
	lockThread2.join();

	auto diffLockTime = (std::chrono::steady_clock::now() - lockStartTime);
	std::cout << "락 결과: " << g_num << std::endl;
	std::cout << "경과 시간: " << std::chrono::duration<float>(diffLockTime).count() << "초" << std::endl << std::endl;
#pragma endregion

	g_num = 0;

#pragma region 스핀락
	auto spinLockStartTime = std::chrono::steady_clock::now();

	SpinLock spinLock;
	std::thread spinLockThread1(SpinLockThreadFunc, &spinLock);
	std::thread spinLockThread2(SpinLockThreadFunc, &spinLock);

	spinLockThread1.join();
	spinLockThread2.join();

	auto diffSpinLockTime = (std::chrono::steady_clock::now() - spinLockStartTime);
	std::cout << "스핀락 결과: " << g_num << std::endl;
	std::cout << "경과 시간: " << std::chrono::duration<float>(diffSpinLockTime).count() << "초" << std::endl;
#pragma endregion

	return 0;
}