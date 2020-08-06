#include "test.hpp"
#include <adl/dispatcher.h>

void test_InlineExecutor_post()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t VALUE = __LINE__;

	// Set new initial value
	reset_value<ID>();
	// Post function to channel
	adl::post<void>(&set_value<ID, VALUE>);
	// Value should be updated
	assert(get_value<ID>() == VALUE);
}

void test_InlineExecutor_post_bulk()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t ID2 = __LINE__;
	constexpr size_t ID3 = __LINE__;
	constexpr size_t VALUE = __LINE__;

	// Set new initial values
	reset_values<ID, ID2, ID3>();
	// Post functions to channel
	adl::post_bulk<void>(&set_value<ID, VALUE>, &set_value<ID2, VALUE>, &set_value<ID3, VALUE>);
	// Check for updated values
	assert(get_value<ID>() == VALUE);
	assert(get_value<ID2>() == VALUE);
	assert(get_value<ID3>() == VALUE);
}

void test_InlineExecutor_post_future()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t VALUE = __LINE__;

	// Set new initial value
	set_value<ID, VALUE>();
	// Post function to channel and get a future
	auto future = adl::post_future<void>(&get_value<ID>);
	// Future should have a shared state
	assert(future.valid());
	// Value should be available
	assert(future.get() == VALUE);
}

void test_InlineExecutor_post_future_bulk()
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
	auto futures = adl::post_future_bulk<void>(&get_value<ID>, &get_value<ID2>, &get_value<ID3>);
	// Futures should have a shared state
	assert(std::get<0>(futures).valid());
	assert(std::get<1>(futures).valid());
	assert(std::get<2>(futures).valid());
	// Check for updated values
	assert(std::get<0>(futures).get() == VALUE);
	assert(std::get<1>(futures).get() == VALUE);
	assert(std::get<2>(futures).get() == VALUE);
}

void test_InlineExecutor()
{
	test_InlineExecutor_post();
	test_InlineExecutor_post_bulk();
	test_InlineExecutor_post_future();
	test_InlineExecutor_post_future_bulk();
}