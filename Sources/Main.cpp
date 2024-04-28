#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

int g_num = 0;

// ������
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
		m_lockFlag�� bExpected�� ������ m_lockFlag�� bDesire�� �����ϰ� true ��ȯ
		m_lockFlag�� bExpected�� �ٸ��� bExpected�� m_lockFlag�� �����ϰ� false ��ȯ
		�̷��� ������ ���ɶ��� ������ ���� ������ false�� ��ȯ�ϸ� bExpected�� false�� �־����
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
#pragma region ��
	auto lockStartTime = std::chrono::steady_clock::now();

	std::mutex mutex;
	std::thread lockThread1(LockThreadFunc, &mutex);
	std::thread lockThread2(LockThreadFunc, &mutex);

	lockThread1.join();
	lockThread2.join();

	auto diffLockTime = (std::chrono::steady_clock::now() - lockStartTime);
	std::cout << "�� ���: " << g_num << std::endl;
	std::cout << "��� �ð�: " << std::chrono::duration<float>(diffLockTime).count() << "��" << std::endl << std::endl;
#pragma endregion

	g_num = 0;

#pragma region ���ɶ�
	auto spinLockStartTime = std::chrono::steady_clock::now();

	SpinLock spinLock;
	std::thread spinLockThread1(SpinLockThreadFunc, &spinLock);
	std::thread spinLockThread2(SpinLockThreadFunc, &spinLock);

	spinLockThread1.join();
	spinLockThread2.join();

	auto diffSpinLockTime = (std::chrono::steady_clock::now() - spinLockStartTime);
	std::cout << "���ɶ� ���: " << g_num << std::endl;
	std::cout << "��� �ð�: " << std::chrono::duration<float>(diffSpinLockTime).count() << "��" << std::endl;
#pragma endregion

	return 0;
}