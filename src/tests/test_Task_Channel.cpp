#include "test.hpp"
#include <adl/task.h>
#include <adl/executors/async_executor.h>
#include <adl/executors/queue_executor.h>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace
{
	enum class ChannelType : int
	{
		T1 = 1,
		T2 = 2,
		A = 3,
		Q1 = 4,
		Q2 = 5,
		Q3 = 6,
		Q4 = 7,
	};

	// Require manual dispatch
	using Channel_Q1 = adl::Channel<ChannelType, ChannelType::Q1, adl::QueueExecutor>;
	using Channel_Q2 = adl::Channel<ChannelType, ChannelType::Q2, adl::QueueExecutor>;
	using Channel_Q3 = adl::Channel<ChannelType, ChannelType::Q3, adl::QueueExecutor>;
	using Channel_Q4 = adl::Channel<ChannelType, ChannelType::Q4, adl::QueueExecutor>;

	// Require manual dispatch to clean outdated futures
	using Channel_A = adl::Channel<ChannelType, ChannelType::A, adl::AsyncExecutor>;

	// Dispatched in separate threads
	using Channel_T1 = adl::Channel<ChannelType, ChannelType::T1, adl::QueueExecutor>; 
	using Channel_T2 = adl::Channel<ChannelType, ChannelType::T2, adl::QueueExecutor>;	

	template<typename ChannelType>
	class jthread
	{
	public:

		jthread()
			: m_done{ false }
			, m_thread{ &dispatch_channel, std::ref(m_done) }
		{}

		~jthread()
		{
			m_done = true;
			m_thread.join();
		}

	private:

		std::atomic_bool m_done;
		std::thread m_thread;

		static void dispatch_channel(std::atomic_bool& done)
		{
			while (!done)
			{
				adl::dispatch<ChannelType>();
				std::this_thread::sleep_for(std::chrono::milliseconds{ 10 });
			}
		}
	};	

	constexpr size_t PID1 = __LINE__;
	constexpr size_t PID2 = __LINE__;
	constexpr size_t PID3 = __LINE__;
	constexpr size_t PID4 = __LINE__;

	constexpr size_t GEN1 = __LINE__;
	constexpr size_t GEN2 = __LINE__;
	constexpr size_t GEN3 = __LINE__;
	constexpr size_t GEN4 = __LINE__;

	constexpr size_t ADD1 = __LINE__;
	constexpr size_t ADD2 = __LINE__;
	constexpr size_t ADD3 = __LINE__;
	constexpr size_t ADD4 = __LINE__;

	template<size_t ID, size_t value>
	void wait_for_value(std::condition_variable& cv)
	{
		std::mutex mutex;
		std::unique_lock<std::mutex> lock{ mutex };
		cv.wait(lock, [] { return get_value<ID>() == value; });
	}

	template<size_t ID, size_t value>
	size_t set_and_notify(std::condition_variable& cv)
	{
		size_t result = set_value<ID, value>();
		cv.notify_one();
		return result;
	}
}

void test_Task_Post_Async()
{
	// CVs to notify current threads that tasks is ready.
	std::condition_variable cv_a;
	std::condition_variable cv_t1;
	std::condition_variable cv_t2;

	reset_values<PID1, PID2, PID3, PID4>();

	adl::task<Channel_Q1>(&set_value<PID1, GEN1>)
		.post<Channel_T1>([&cv_t1] { set_and_notify<PID2, GEN2>(cv_t1); })
		.post<Channel_A>([&cv_a]   { set_and_notify<PID3, GEN3>(cv_a); })
		.post<Channel_T2>([&cv_t2] { set_and_notify<PID4, GEN4>(cv_t2); })
		.post<Channel_Q2>([&cv_t2] { wait_for_value<PID4, GEN4>(cv_t2); })
		.submit();

	// Values should be updated after dispatches, not before
	assert(get_value<PID1>() == 0);
	assert(get_value<PID2>() == 0);
	assert(get_value<PID3>() == 0);
	assert(get_value<PID4>() == 0);

	{
		adl::dispatch<Channel_Q1>();
		assert(get_value<PID1>() == GEN1);
	}

	{
		// Channel_T1 dispatched in separate thread, just wait for value
		wait_for_value<PID2, GEN2>(cv_t1);
		assert(get_value<PID2>() == GEN2);
	}

	{
		wait_for_value<PID3, GEN3>(cv_a);
		assert(get_value<PID3>() == GEN3);
		// Dispatch to release outdated futures in async_executor
		adl::dispatch<Channel_A>();
	}

	adl::dispatch<Channel_Q2>();

	assert(get_value<PID4>() == GEN4);
}

void test_Task_Then_Async()
{
	{
		// CVs to notify current threads that tasks is ready.
		std::condition_variable cv;

		reset_value<PID1>();

		adl::task<Channel_Q1>(&generate<GEN1>)
			.then<Channel_T1>(&add<ADD1>)
			.then<Channel_A>(&add<ADD2>)
			.then<Channel_T2>([&cv](size_t value) { set_a<PID1>(value); cv.notify_one(); })
			.submit();

		adl::dispatch<Channel_Q1>();

		wait_for_value<PID1, GEN1 + ADD1 + ADD2>(cv);

		assert(get_value<PID1>() == GEN1 + ADD1 + ADD2);
	}

	{
		// CVs to notify current threads that tasks is ready.
		std::condition_variable cv;

		reset_values<PID1, PID2, PID3, PID4>();

		adl::task<Channel_Q1>(&generate<GEN1>)
			.then<Channel_T1>(&add<ADD1>)
			.then<Channel_A>(&add<ADD2>)
			.then(&set_a<PID1>)
			.then<Channel_T2>(&generate<GEN2>)
			.then<Channel_T1>(&add<ADD1>)
			.then<Channel_A>(&add<ADD2>)
			.then(&set_a<PID2>)
			.then<Channel_T2>(&generate<GEN3>)
			.then<Channel_T1>(&add<ADD1>)
			.then<Channel_A>(&add<ADD2>)
			.then(&set_a<PID3>)
			.then<Channel_T1>(&generate<GEN4>)
			.then<Channel_T2>([&cv](size_t value) { set_a<PID4>(value); cv.notify_one(); })
			.submit();

		adl::dispatch<Channel_Q1>();

		wait_for_value<PID4, GEN4>(cv);

		assert(get_value<PID1>() == GEN1 + ADD1 + ADD2);
		assert(get_value<PID2>() == GEN2 + ADD1 + ADD2);
		assert(get_value<PID3>() == GEN3 + ADD1 + ADD2);
		assert(get_value<PID4>() == GEN4);
	}
}

void test_Task_Then()
{
	{
		reset_values<PID1, PID2>();

		adl::task<Channel_Q1>(&generate<GEN1>)
			.then(&set_a<PID1>)
			.then(&generate<GEN2>)
			.then(&set_a<PID2>)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
	}

	{
		reset_values<PID1, PID2>();

		adl::task<Channel_Q1>(&generate<GEN1>)
			.then(&set_a<PID1>)
			.then(&void_fn)
			.then(&generate<GEN2>)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&set_a<PID2>)
			.then(&void_fn)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2 + ADD1 + ADD2);
	}

	{
		reset_values<PID1, PID2>();

		adl::task<Channel_Q1>(&generate<GEN1>)
			.then(&set_a<PID1>)
			.then<Channel_Q2>(&generate<GEN2>)
			.then<Channel_Q3>(&set_a<PID2>)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == 0); // should be updated after Q2 dispatch

		adl::dispatch<Channel_Q2>();
		adl::dispatch<Channel_Q3>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
	}

	{
		reset_values<PID1, PID2>();

		adl::task<Channel_Q1>(&generate<GEN1>)
			.then(&set_a<PID1>)
			.then<Channel_Q2>(&generate<GEN2>)
			.then(&set_a<PID2>)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == 0); // should be updated after Q2 dispatch

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
	}

	{
		reset_values<PID1, PID2>();

		adl::task<Channel_Q1>(&generate<GEN1>)
			.then<Channel_Q2>(&set_a<PID1>)
			.then<Channel_Q3>(&generate<GEN2>)
			.then<Channel_Q4>(&set_a<PID2>)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == 0); // should be updated after Q2 dispatch
		assert(get_value<PID2>() == 0); // should be updated after Q4 dispatch

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == 0); // should be updated after Q4 dispatch

		adl::dispatch<Channel_Q3>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == 0); // should be updated after Q4 dispatch

		adl::dispatch<Channel_Q4>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
	}
	
	{
		reset_values<PID1, PID2>();

		adl::task<Channel_Q1>(&generate<GEN1>)
			.then<Channel_Q2>(&set_a<PID1>)
			.then<Channel_Q3>(&generate<GEN2>)
			.then<Channel_Q4>(&set_a<PID2>)
			.then<Channel_T1>(&void_fn) // with void ending
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == 0); // should be updated after Q2 dispatch
		assert(get_value<PID2>() == 0); // should be updated after Q4 dispatch

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == 0); // should be updated after Q4 dispatch

		adl::dispatch<Channel_Q3>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == 0); // should be updated after Q4 dispatch

		adl::dispatch<Channel_Q4>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
	}	

}

void test_Task_Post()
{
	{
		reset_values<PID1, PID2>();

		adl::task<Channel_Q1>(&void_fn)
			.post(&set_value<PID1, GEN1>)
			.post(&set_value<PID2, GEN2>)
			.post(&void_fn)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
	}

	{
		reset_values<PID1, PID2>();

		adl::task<Channel_Q1>(&void_fn)
			.post(&set_value<PID1, GEN1>)
			.post<Channel_Q2>(&set_value<PID2, GEN2>)
			.post(&void_fn)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == 0); // should be updated after Q2 dispatch

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
	}

	{
		reset_values<PID1, PID2, PID3, PID4>();

		adl::task(&void_fn)
			.post<Channel_Q1>(&set_value<PID1, GEN1>)
			.post<Channel_Q1>(&set_value<PID2, GEN2>)
			.post<Channel_Q2>(&set_value<PID3, GEN3>)
			.post<Channel_Q2>(&set_value<PID4, GEN4>)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
		assert(get_value<PID3>() == 0); // should be updated after Q2 dispatch
		assert(get_value<PID4>() == 0); // should be updated after Q2 dispatch

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
		assert(get_value<PID3>() == GEN3);
		assert(get_value<PID4>() == GEN4);
	}

	{
		reset_values<PID1, PID2, PID3, PID4>();

		adl::task<Channel_Q1>(&set_value<PID1, GEN1>)
			.post<Channel_Q2>(&set_value<PID2, GEN2>)
			.post<Channel_Q3>(&set_value<PID3, GEN3>)
			.post<Channel_Q4>(&set_value<PID4, GEN4>)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == 0); // should be updated after Q2 dispatch
		assert(get_value<PID3>() == 0); // should be updated after Q3 dispatch
		assert(get_value<PID4>() == 0); // should be updated after Q4 dispatch

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID2>() == GEN2); 
		assert(get_value<PID3>() == 0); // should be updated after Q3 dispatch
		assert(get_value<PID4>() == 0); // should be updated after Q4 dispatch

		adl::dispatch<Channel_Q3>();

		assert(get_value<PID3>() == GEN3);
		assert(get_value<PID4>() == 0); // should be updated after Q4 dispatch

		adl::dispatch<Channel_Q4>();

		assert(get_value<PID4>() == GEN4);
	}
}

void test_Task_Post_Bulk()
{
	{
		reset_values<PID1, PID2, PID3, PID4>();

		adl::task<Channel_Q1>(&void_fn)
			.post_bulk(&set_value<PID1, GEN1>, &set_value<PID2, GEN2>)
			.post_bulk<Channel_Q2>(&set_value<PID3, GEN3>, &set_value<PID4, GEN4>)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
		assert(get_value<PID3>() == 0); // should be updated after Q2 dispatch
		assert(get_value<PID4>() == 0); // should be updated after Q2 dispatch

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID3>() == GEN3);
		assert(get_value<PID4>() == GEN4);
	}

	{
		reset_values<PID1, PID2, PID3, PID4>();

		adl::task(&void_fn)
			.post_bulk<Channel_Q1>(&set_value<PID1, GEN1>
				, &set_value<PID2, GEN2>
				, &set_value<PID3, GEN3>
				, &set_value<PID4, GEN4>)
			.post_bulk<Channel_Q2>(&set_value<PID1, GEN1 + ADD1>
				, &set_value<PID2, GEN2 + ADD2>
				, &set_value<PID3, GEN3 + ADD3>
				, &set_value<PID4, GEN4 + ADD4>)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
		assert(get_value<PID3>() == GEN3);
		assert(get_value<PID4>() == GEN4);

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID1>() == GEN1 + ADD1);
		assert(get_value<PID2>() == GEN2 + ADD2);
		assert(get_value<PID3>() == GEN3 + ADD3);
		assert(get_value<PID4>() == GEN4 + ADD4);
	}
}

void test_Task_Nested()
{
	auto t1 = adl::task(&generate<GEN1>)
		.then(&add<ADD1>)
		.then(&add<ADD2>)
		.then(&add<ADD3>)
		.then(&set_a<PID1>);

	auto t2 = adl::task(&generate<GEN2>)
		.then(&add<ADD1>)
		.then(&add<ADD2>)
		.then(&add<ADD3>)
		.then(&set_a<PID2>);

	auto t3 = adl::task(&generate<GEN3>)
		.then(&add<ADD1>)
		.then(&add<ADD2>)
		.then(&add<ADD3>)
		.then(&set_a<PID3>);

	{
		reset_values<PID1, PID2, PID3>();

		adl::task<Channel_Q1>(t1)
			.post<Channel_Q2>(t2)
			.post<Channel_Q3>(t3)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1 + ADD1 + ADD2 + ADD3);
		assert(get_value<PID2>() == 0);
		assert(get_value<PID3>() == 0);	

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID2>() == GEN2 + ADD1 + ADD2 + ADD3);
		assert(get_value<PID3>() == 0);

		adl::dispatch<Channel_Q3>();

		assert(get_value<PID3>() == GEN3 + ADD1 + ADD2 + ADD3);
	}

	{
		reset_values<PID1, PID2, PID3>();

		adl::task<Channel_Q1>(t1)
			.then<Channel_Q2>(t2)
			.then<Channel_Q3>(t3)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1 + ADD1 + ADD2 + ADD3);
		assert(get_value<PID2>() == 0);
		assert(get_value<PID3>() == 0);

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID2>() == GEN2 + ADD1 + ADD2 + ADD3);
		assert(get_value<PID3>() == 0);

		adl::dispatch<Channel_Q3>();

		assert(get_value<PID3>() == GEN3 + ADD1 + ADD2 + ADD3);
	}

	{
		auto tt1 = adl::task<Channel_Q1>(&generate<GEN1>)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>)
			.then(&set_a<PID1>);

		auto tt2 = adl::task<Channel_Q2>(&generate<GEN2>)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>)
			.then(&set_a<PID2>);

		auto tt3 = adl::task<Channel_Q3>(&generate<GEN3>)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>)
			.then(&set_a<PID3>);

		reset_values<PID1, PID2, PID3, PID4>();

		adl::task(tt1)
			.then(tt2)
			.then(tt3)
			.then<Channel_Q4>(&set_value<PID4, GEN4>)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1 + ADD1 + ADD2 + ADD3);
		assert(get_value<PID2>() == 0);
		assert(get_value<PID3>() == 0);
		assert(get_value<PID4>() == 0);

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID2>() == GEN2 + ADD1 + ADD2 + ADD3);
		assert(get_value<PID3>() == 0);
		assert(get_value<PID4>() == 0);

		adl::dispatch<Channel_Q3>();

		assert(get_value<PID3>() == GEN3 + ADD1 + ADD2 + ADD3);
		assert(get_value<PID4>() == 0);

		adl::dispatch<Channel_Q4>();

		assert(get_value<PID4>() == GEN4);
	}

	{
		auto tt1 = adl::task(&generate<GEN1>)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>);

		auto tt2 = adl::task(&generate<GEN2>)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>);

		auto tt3 = adl::task(&generate<GEN3>)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>);

		reset_values<PID1, PID2, PID3, PID4>();

		adl::task<Channel_Q1>(tt1)
			.then(&set_a<PID1>)
			.then<Channel_Q2>(tt2)
			.then(&set_a<PID2>)
			.then<Channel_Q3>(tt3)
			.then(&set_a<PID3>)
			.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == GEN1 + ADD1 + ADD2 + ADD3);
		assert(get_value<PID2>() == 0);
		assert(get_value<PID3>() == 0);

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID2>() == GEN2 + ADD1 + ADD2 + ADD3);
		assert(get_value<PID3>() == 0);

		adl::dispatch<Channel_Q3>();

		assert(get_value<PID3>() == GEN3 + ADD1 + ADD2 + ADD3);
	}
}

void test_Task_Mixed()
{
	auto t1 = adl::task(&generate<GEN1>)
		.then(&add<ADD1>)
		.then(&add<ADD2>);

	auto t2 = adl::task(&generate<GEN2>)
		.then(&add<ADD1>)
		.then(&add<ADD2>)
		.then(&add<ADD3>)
		.then(&set_a<PID2>);

	{
		reset_values<PID1, PID2, PID3, PID4>();

		auto t = adl::task<Channel_Q1>(t1)
			.then(&add<ADD3>)
			.post_bulk<Channel_Q2>(t2, &set_value<PID3, GEN3>, &set_value<PID4, GEN4>)
			.then<Channel_Q3>(&set_a<PID1>)
			.post_bulk<Channel_Q4>(&set_value<PID1, GEN1>, &set_value<PID2, GEN2>);

		t.submit();

		adl::dispatch<Channel_Q1>();

		assert(get_value<PID1>() == 0);
		assert(get_value<PID2>() == 0);
		assert(get_value<PID3>() == 0);
		assert(get_value<PID4>() == 0);

		adl::dispatch<Channel_Q2>();

		assert(get_value<PID1>() == 0);
		assert(get_value<PID2>() == GEN2 + ADD1 + ADD2 + ADD3);
		assert(get_value<PID3>() == GEN3);
		assert(get_value<PID4>() == GEN4);

		adl::dispatch<Channel_Q3>();

		assert(get_value<PID1>() == GEN1 + ADD1 + ADD2 + ADD3);

		adl::dispatch<Channel_Q4>();

		assert(get_value<PID1>() == GEN1);
		assert(get_value<PID2>() == GEN2);
	}
}

void test_Task_Channel()
{
	jthread<Channel_T1> t1;
	jthread<Channel_T2> t2;
			
	test_Task_Then();
	test_Task_Post();
	test_Task_Post_Bulk();
	test_Task_Post_Async();
	test_Task_Then_Async();	
	test_Task_Nested();
	test_Task_Mixed();	
}