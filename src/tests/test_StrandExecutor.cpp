#include "test.hpp"
#include <adl/dispatcher.h>
#include <adl/executors/strand_executor.h>

namespace
{
	enum class StrandChannelType : int
	{
		S1 = 1,
		S2 = 2,
		S3 = 3,
		S4 = 4,
	};

	template<StrandChannelType type>
	using StrandChannel = adl::Channel<StrandChannelType, type, adl::StrandExecutor>;

	using Channel_S1 = StrandChannel<StrandChannelType::S1>;
	using Channel_S2 = StrandChannel<StrandChannelType::S2>;
	using Channel_S3 = StrandChannel<StrandChannelType::S3>;
	using Channel_S4 = StrandChannel<StrandChannelType::S4>;
}

void test_StrandEecutor_post()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t VALUE = __LINE__;

	// Set new initial value
	reset_value<ID>();
	// Post function to channel
	adl::post<Channel_S1>(&set_value<ID, VALUE>);
	// Still 0, value should be updated after dispatch
	assert(get_value<ID>() == 0);
	// Dispatch channel
	adl::dispatch<Channel_S1>();
	// Value should be updated
	assert(get_value<ID>() == VALUE);
}

void test_StrandEecutor_post_bulk()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t ID2 = __LINE__;
	constexpr size_t ID3 = __LINE__;
	constexpr size_t VALUE = __LINE__;

	reset_values<ID, ID2, ID3>();
	// Post functions to channel
	adl::post_bulk<Channel_S2>(&set_value<ID, VALUE>, &set_value<ID2, VALUE>, &set_value<ID3, VALUE>);
	// Values should be updated after dispatch
	assert(get_value<ID>() == 0);
	assert(get_value<ID2>() == 0);
	assert(get_value<ID3>() == 0);
	// Dispatch channel
	adl::dispatch<Channel_S2>();
	// Check for updated values
	assert(get_value<ID>() == VALUE);
	assert(get_value<ID2>() == VALUE);
	assert(get_value<ID3>() == VALUE);
}

void test_StrandEecutor_post_future()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t VALUE = __LINE__;

	// Set new initial value
	set_value<ID, VALUE>();
	// Post function to channel and get a future
	auto future = adl::post_future<Channel_S3>(&get_value<ID>);
	// Future should have a shared state
	assert(future.valid());
	// Dispatch channel
	adl::dispatch<Channel_S3>();
	// Value should be available
	assert(future.get() == VALUE);
}

void test_StrandEecutor_post_future_bulk()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t ID2 = __LINE__;
	constexpr size_t ID3 = __LINE__;
	constexpr size_t VALUE = __LINE__;

	set_value<ID, VALUE>();
	set_value<ID2, VALUE>();
	set_value<ID3, VALUE>();
	// Post functions to channel and get a futures
	auto futures = adl::post_future_bulk<Channel_S4>(&get_value<ID>, &get_value<ID2>, &get_value<ID3>);
	// Futures should have a shared state
	assert(std::get<0>(futures).valid());
	assert(std::get<1>(futures).valid());
	assert(std::get<2>(futures).valid());
	// Dispatch channel
	adl::dispatch<Channel_S4>();
	// Check for updated values
	assert(std::get<0>(futures).get() == VALUE);
	assert(std::get<1>(futures).get() == VALUE);
	assert(std::get<2>(futures).get() == VALUE);
}

void test_StrandEecutor()
{
	test_StrandEecutor_post();
	test_StrandEecutor_post_bulk();
	test_StrandEecutor_post_future();
	test_StrandEecutor_post_future_bulk();
}