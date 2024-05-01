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

// 쓰레드풀
class RxThreadPool
{
public:
	RxThreadPool();
	~RxThreadPool();

	// 일 들어왔다!
	template <typename TTaskFunc, typename... Args>
	std::future<std::invoke_result_t<TTaskFunc, Args...>> AddTask(int taskIdx, TTaskFunc&& taskFunc, Args... args)
	{
		using ReturnType = std::invoke_result_t<TTaskFunc, Args...>;
		using PackagedTask = std::packaged_task<ReturnType(void)>;

		/*
		std::bind()로 함수 객체를 만들 때 인자들을 전달한 함수에 bind하므로
		bind된 함수 객체를 저장하는 쪽에서는 매개변수를 void로 정할 수 있음.
		반환값이 필요하다면 저장하는 쪽에서 반환 형식을 명시하면됨		
		*/
		auto bindTask = std::bind(taskFunc, std::forward<Args>(args)...);
		std::shared_ptr<PackagedTask> spTask = std::make_shared<PackagedTask>(bindTask);
		
		// task는 local에서 생성하므로 queue에 유지시키려면 heap으로 넘겨야함
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

	// 쓰레드 전용 함수
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

			// 작업할 게 없는데 중단 시그널이 왔으면 탈출 (락은 자동 해제)
			if ((m_bAllStop == true) &&
				(m_taskQueue.empty() == true))
			{
				return;
			}

			taskInfo = std::move(m_taskQueue.front());
			m_taskQueue.pop();

			// ThreadId도 하나의 객체라 이렇게 사용하는 건 올바르지 않음
			printf("Thread Id(%d)는 (%d)번 일 처리!\n", std::this_thread::get_id(), taskInfo.taskIdx);
		}

		//std::this_thread::sleep_for(std::chrono_literals::operator""ms(1ull)); // 1ms 슬립
		std::this_thread::sleep_for(1s); // 1ms 슬립
		taskInfo.taskFunc();
	}
}

int work1(int num1, int num2)
{
	printf("Thread Id(%d)가 task 처리!\n", std::this_thread::get_id());
	return (num1 + num2);
}

float work2(float num1, float num2)
{
	printf("Thread Id(%d)가 task 처리!\n", std::this_thread::get_id());
	return (num1 + num2);
}

class GreatWork
{
public:
	GreatWork() = default;
};

void work3(GreatWork greatWork)
{
	printf("Thread Id(%d)가 task 처리!\n", std::this_thread::get_id());
}

int main()
{
	RxThreadPool rxThreadPool;

	// 서브 쓰레드의 결과를 받으려면 메인 쓰레드도 기다려야함...
	//std::cout << "work1: " << rxThreadPool.AddTask(0, work1, 10, 20).get() << std::endl << std::endl;
	//std::cout << "work2: " << rxThreadPool.AddTask(1, work2, 24.02f, 12.62f).get() << std::endl << std::endl;

	//std::cout << "work3: ";
	//rxThreadPool.AddTask(2, work3, GreatWork()); // 반환

	std::vector<std::future<int>> m_futures;
	for (int i = 0; i < 5; ++i)
	{
		m_futures.push_back(rxThreadPool.AddTask(i + 3, work1, 10, 20));
	}

	// 메인 쓰레드는 서브 쓰레드의 작업 결과를 기다리지 않고 다른 일을 처리하는 게 가능
	std::cout << "메인 쓰레드의 등장!\n";

	for (int i = 0; i < 5; ++i)
	{
		int ret = m_futures[i].get();
		printf("work1 결과: %d\n", ret);
	}

	return 0;
}