#include "test.hpp"
#include <adl/dispatcher.h>
#include <adl/executors/queue_executor.h>

namespace
{
	enum class QueueChannelType : int
	{
		Q1 = 1,
		Q2 = 2,
		Q3 = 3,
		Q4 = 4,
	};

	template<QueueChannelType type>
	using QueueChannel = adl::Channel<QueueChannelType, type, adl::QueueExecutor>;

	using Channel_Q1 = QueueChannel<QueueChannelType::Q1>;
	using Channel_Q2 = QueueChannel<QueueChannelType::Q2>;
	using Channel_Q3 = QueueChannel<QueueChannelType::Q3>;
	using Channel_Q4 = QueueChannel<QueueChannelType::Q4>;
}

void test_QueueEecutor_post()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t VALUE = __LINE__;

	// Set new initial value
	reset_value<ID>();
	// Post function to channel
	adl::post<Channel_Q1>(&set_value<ID, VALUE>);
	// Still 0, value should be updated after dispatch
	assert(get_value<ID>() == 0); 
	// Dispatch channel
	adl::dispatch<Channel_Q1>();
	// Value should be updated
	assert(get_value<ID>() == VALUE);
}

void test_QueueEecutor_post_bulk()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t ID2 = __LINE__;
	constexpr size_t ID3 = __LINE__;
	constexpr size_t VALUE = __LINE__;

	reset_values<ID, ID2, ID3>();
	// Post functions to channel
	adl::post_bulk<Channel_Q2>(&set_value<ID, VALUE>, &set_value<ID2, VALUE>, &set_value<ID3, VALUE>);
	// Values should be updated after dispatch
	assert(get_value<ID>() == 0);
	assert(get_value<ID2>() == 0);
	assert(get_value<ID3>() == 0);
	// Dispatch channel
	adl::dispatch<Channel_Q2>();
	// Check for updated values
	assert(get_value<ID>() == VALUE);
	assert(get_value<ID2>() == VALUE);
	assert(get_value<ID3>() == VALUE);
}

void test_QueueEecutor_post_future()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t VALUE = __LINE__;

	// Set new initial value
	set_value<ID, VALUE>();
	// Post function to channel and get a future
	auto future = adl::post_future<Channel_Q3>(&get_value<ID>);
	// Future should have a shared state
	assert(future.valid());
	// Dispatch channel
	adl::dispatch<Channel_Q3>();
	// Value should be available
	assert(future.get() == VALUE);
}

void test_QueueEecutor_post_future_bulk()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t ID2 = __LINE__;
	constexpr size_t ID3 = __LINE__;
	constexpr size_t VALUE = __LINE__;

	set_value<ID, VALUE>();
	set_value<ID2, VALUE>();
	set_value<ID3, VALUE>();
	// Post functions to channel and get a futures
	auto futures = adl::post_future_bulk<Channel_Q4>(&get_value<ID>, &get_value<ID2>, &get_value<ID3>);
	// Futures should have a shared state
	assert(std::get<0>(futures).valid());
	assert(std::get<1>(futures).valid());
	assert(std::get<2>(futures).valid());
	// Dispatch channel
	adl::dispatch<Channel_Q4>();
	// Check for updated values
	assert(std::get<0>(futures).get() == VALUE);
	assert(std::get<1>(futures).get() == VALUE);
	assert(std::get<2>(futures).get() == VALUE);
}

void test_QueueEecutor()
{
	test_QueueEecutor_post();
	test_QueueEecutor_post_bulk();
	test_QueueEecutor_post_future();
	test_QueueEecutor_post_future_bulk();
}