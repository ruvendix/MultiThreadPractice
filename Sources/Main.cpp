#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <future>
#include <type_traits>
#include <functional>

using namespace std::chrono_literals;

// ������Ǯ
class RxThreadPool
{
public:
	RxThreadPool();
	~RxThreadPool();

	// �� ���Դ�!
	template <typename TTaskFunc, typename... Args>
	std::future<std::invoke_result_t<TTaskFunc, Args...>> AddTask(int taskIdx, TTaskFunc&& taskFunc, Args... args)
	{
		using ReturnType = std::invoke_result_t<TTaskFunc, Args...>;
		using PackagedTask = std::packaged_task<ReturnType(void)>;

		/*
		std::bind()�� �Լ� ��ü�� ���� �� ���ڵ��� ������ �Լ��� bind�ϹǷ�
		bind�� �Լ� ��ü�� �����ϴ� �ʿ����� �Ű������� void�� ���� �� ����.
		��ȯ���� �ʿ��ϴٸ� �����ϴ� �ʿ��� ��ȯ ������ ����ϸ��		
		*/
		auto bindTask = std::bind(taskFunc, std::forward<Args>(args)...);
		std::shared_ptr<PackagedTask> spTask = std::make_shared<PackagedTask>(bindTask);
		
		// task�� local���� �����ϹǷ� queue�� ������Ű���� heap���� �Ѱܾ���
		TaskInfo taskInfo;
		taskInfo.taskFunc = [spTask]() { (*spTask)(); };
		{
			std::scoped_lock lock(m_mutex);
			taskInfo.taskIdx = taskIdx;
			m_taskQueue.push(taskInfo);
		}
		
		m_conditionVar.notify_one();
		return (spTask->get_future());
	}

	// ������ ���� �Լ�
	void DoWork();

private:
	int m_hardwareConcurrencyThreadCount = 0;
	std::vector<std::thread> m_threads;

	std::mutex m_mutex; // Lock-based
	std::condition_variable m_conditionVar;

	using TaskFuncType = std::function<void(void)>;

	struct TaskInfo
	{
		TaskFuncType taskFunc;
		int taskIdx = 0;
	};

	std::queue<TaskInfo> m_taskQueue;
	bool m_bAllStop = false;
};
///////////////////////////////////////////////////////////////////////////////////
RxThreadPool::RxThreadPool()
{
	m_hardwareConcurrencyThreadCount = std::thread::hardware_concurrency();

	for (int i = 0; i < m_hardwareConcurrencyThreadCount; ++i)
	{
		m_threads.push_back(std::thread(&RxThreadPool::DoWork, this));
	}
}

RxThreadPool::~RxThreadPool()
{
	m_bAllStop = true;
	m_conditionVar.notify_all();

	for (std::thread& refThread : m_threads)
	{
		refThread.join();
	}
}

void RxThreadPool::DoWork()
{
	while (true)
	{
		TaskInfo taskInfo;

		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_conditionVar.wait(lock, [this]() { return ((m_taskQueue.empty() == false) || (m_bAllStop == true)); });

			// �۾��� �� ���µ� �ߴ� �ñ׳��� ������ Ż�� (���� �ڵ� ����)
			if ((m_bAllStop == true) &&
				(m_taskQueue.empty() == true))
			{
				return;
			}

			taskInfo = std::move(m_taskQueue.front());
			m_taskQueue.pop();

			// ThreadId�� �ϳ��� ��ü�� �̷��� ����ϴ� �� �ùٸ��� ����
			printf("Thread Id(%d)�� (%d)�� �� ó��!\n", std::this_thread::get_id(), taskInfo.taskIdx);
		}

		//std::this_thread::sleep_for(std::chrono_literals::operator""ms(1ull)); // 1ms ����
		std::this_thread::sleep_for(1s); // 1ms ����
		taskInfo.taskFunc();
	}
}

int work1(int num1, int num2)
{
	printf("Thread Id(%d)�� task ó��!\n", std::this_thread::get_id());
	return (num1 + num2);
}

float work2(float num1, float num2)
{
	printf("Thread Id(%d)�� task ó��!\n", std::this_thread::get_id());
	return (num1 + num2);
}

class GreatWork
{
public:
	GreatWork() = default;
};

void work3(GreatWork greatWork)
{
	printf("Thread Id(%d)�� task ó��!\n", std::this_thread::get_id());
}

int main()
{
	RxThreadPool rxThreadPool;

	// ���� �������� ����� �������� ���� �����嵵 ��ٷ�����...
	//std::cout << "work1: " << rxThreadPool.AddTask(0, work1, 10, 20).get() << std::endl << std::endl;
	//std::cout << "work2: " << rxThreadPool.AddTask(1, work2, 24.02f, 12.62f).get() << std::endl << std::endl;

	//std::cout << "work3: ";
	//rxThreadPool.AddTask(2, work3, GreatWork()); // ��ȯ

	std::vector<std::future<int>> m_futures;
	for (int i = 0; i < 5; ++i)
	{
		m_futures.push_back(rxThreadPool.AddTask(i + 3, work1, 10, 20));
	}

	// ���� ������� ���� �������� �۾� ����� ��ٸ��� �ʰ� �ٸ� ���� ó���ϴ� �� ����
	std::cout << "���� �������� ����!\n";

	for (int i = 0; i < 5; ++i)
	{
		int ret = m_futures[i].get();
		printf("work1 ���: %d\n", ret);
	}

	return 0;
}