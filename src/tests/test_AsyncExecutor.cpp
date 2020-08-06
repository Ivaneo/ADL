#include "test.hpp"
#include <adl/dispatcher.h>
#include <adl/executors/async_executor.h>
#include <mutex>
#include <condition_variable>

namespace
{
	enum class AsyncChannelType : int
	{
		A1 = 1,
		A2 = 2,
		A3 = 3,
		A4 = 4,
	};

	template<AsyncChannelType type>
	using AsyncChannel = adl::Channel<AsyncChannelType, type, adl::AsyncExecutor>;

	using Channel_A1 = AsyncChannel<AsyncChannelType::A1>;
	using Channel_A2 = AsyncChannel<AsyncChannelType::A2>;
	using Channel_A3 = AsyncChannel<AsyncChannelType::A3>;
	using Channel_A4 = AsyncChannel<AsyncChannelType::A4>;

	constexpr size_t AsyncID = __LINE__;
	constexpr size_t AsyncID2 = __LINE__;
	constexpr size_t AsyncID3 = __LINE__;
	constexpr size_t AsyncVALUE = __LINE__;
}

void test_AsyncExecutor_post()
{	
	// Set new initial value
	reset_value<AsyncID>();

	// CVs to notify current thread that task is ready.
	std::condition_variable cv;

	// Post function to channel
	adl::post<Channel_A1>([&]{ set_value<AsyncID, AsyncVALUE>(); cv.notify_one(); });

	// Wait for signal
	{
		std::mutex mutex;
		std::unique_lock<std::mutex> lock{ mutex };
		cv.wait(lock, [] { return get_value<AsyncID>() == AsyncVALUE; });
	}
	
	// Value should be updated
	assert(get_value<AsyncID>() == AsyncVALUE);

	// Dispatch channel to clear outdated futures in async_executor
	adl::dispatch<Channel_A1>();
}

void test_AsyncExecutor_post_bulk()
{
	// Set new initial value
	reset_values<AsyncID, AsyncID2, AsyncID3>();

	// CVs to notify current thread that task is ready.
	std::condition_variable cv[3];

	// Post function to channel
	adl::post_bulk<Channel_A2>([&]{ set_value<AsyncID, AsyncVALUE>();	cv[0].notify_one(); }
							  ,[&]{ set_value<AsyncID2, AsyncVALUE>();	cv[1].notify_one();	}
							  ,[&]{ set_value<AsyncID3, AsyncVALUE>();	cv[2].notify_one();	});

	// Wait for signals
	{
		std::mutex mutex;
		std::unique_lock<std::mutex> lock{ mutex };
		cv[0].wait(lock, [] { return get_value<AsyncID>() == AsyncVALUE; });
	}
	{
		std::mutex mutex;
		std::unique_lock<std::mutex> lock{ mutex };
		cv[1].wait(lock, [] { return get_value<AsyncID2>() == AsyncVALUE; });
	}
	{
		std::mutex mutex;
		std::unique_lock<std::mutex> lock{ mutex };
		cv[2].wait(lock, [] { return get_value<AsyncID3>() == AsyncVALUE; });
	}

	// Value should be updated
	assert(get_value<AsyncID>() == AsyncVALUE);
	assert(get_value<AsyncID2>() == AsyncVALUE);
	assert(get_value<AsyncID3>() == AsyncVALUE);

	// Dispatch channel to clear outdated futures in async_executor
	adl::dispatch<Channel_A2>();
}

void test_AsyncExecutor_post_future()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t VALUE = __LINE__;

	// Set new initial value
	set_value<ID, VALUE>();
	// Post function to channel and get a future
	auto future = adl::post_future<Channel_A3>(&get_value<ID>);
	// Future should have a shared state
	assert(future.valid());	
	// Value should be available
	assert(future.get() == VALUE);

	// Dispatch not needed 
	// adl::dispatch<Channel_A3>();
}

void test_AsyncExecutor_post_future_bulk()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t ID2 = __LINE__;
	constexpr size_t ID3 = __LINE__;
	constexpr size_t VALUE = __LINE__;

	// Set new initial values
	set_value<ID, VALUE>();
	set_value<ID2, VALUE>();
	set_value<ID3, VALUE>();

	// Post functions to channel and get a futures
	auto futures = adl::post_future_bulk<Channel_A4>(&get_value<ID>, &get_value<ID2>, &get_value<ID3>);

	// Futures should have a shared state
	assert(std::get<0>(futures).valid());
	assert(std::get<1>(futures).valid());
	assert(std::get<2>(futures).valid());

	// Wait for updated values
	assert(std::get<0>(futures).get() == VALUE);
	assert(std::get<1>(futures).get() == VALUE);
	assert(std::get<2>(futures).get() == VALUE);

	// Dispatch not needed 
	// adl::dispatch<Channel_A4>();
}

void test_AsyncExecutor()
{
	test_AsyncExecutor_post();
	test_AsyncExecutor_post_bulk();
	test_AsyncExecutor_post_future();
	test_AsyncExecutor_post_future_bulk();
}