#pragma once
#include <assert.h>

template<size_t ID>
struct ValueHolder
{
	inline static size_t value = 0;
};

template<size_t ID, size_t value>
size_t set_value()
{
	ValueHolder<ID>::value = value;
	return ValueHolder<ID>::value;
}

template<size_t ID>
size_t get_value()
{
	return ValueHolder<ID>::value;
}

template<size_t ID>
void reset_value()
{
	ValueHolder<ID>::value = 0;
}

template<size_t... IDS>
void reset_values()
{
	(..., reset_value<IDS>());
}

inline void void_fn()
{
	ValueHolder<0>::value = 0;
}

template<size_t value>
constexpr size_t add(size_t prevValue)
{
	return prevValue + value;
}

template<size_t ID>
void set_a(size_t value)
{
	ValueHolder<ID>::value = value;
}

template<size_t value>
constexpr size_t generate()
{
	return value;
}

void test_Placeholder();
void test_ExecutionContext();
void test_QueueEecutor();
void test_AsyncExecutor();
void test_InlineExecutor();
void test_Task();
void test_Task_Channel();
void test_Task_ExecutionContext();

inline void run_tests()
{
	test_Placeholder();
	test_ExecutionContext();
	test_QueueEecutor();
	test_AsyncExecutor();
	test_InlineExecutor();
	test_Task();
	test_Task_Channel();	
	test_Task_ExecutionContext();
}