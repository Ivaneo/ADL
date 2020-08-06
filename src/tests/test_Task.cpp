#include "test.hpp"
#include <adl/task.h>

namespace
{
	constexpr size_t PID = __LINE__;
	constexpr size_t PID2 = __LINE__;
	constexpr size_t PID3 = __LINE__;
	constexpr size_t PID4 = __LINE__;
	constexpr size_t PVALUE = __LINE__;

	template<size_t ID, size_t value>
	void set_v()
	{
		ValueHolder<ID>::value = value;
	}

	template<size_t ID, size_t value>
	size_t set_r()
	{
		ValueHolder<ID>::value = value;
		return ValueHolder<ID>::value;
	}

	template<size_t ID, size_t value>
	constexpr void add_v()
	{
		ValueHolder<ID>::value += value;
	}
}

void test_Task_submit()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t ID2 = __LINE__;
	constexpr size_t VALUE = __LINE__;

	// One execution agent, rvalue submit
	{
		reset_value<ID>();

		adl::task(&set_value<ID, VALUE>)
			.submit();

		assert(get_value<ID>() == VALUE);
	}

	// One execution agent, lvalue submit
	{
		reset_value<ID>();

		auto task = adl::task(&set_value<ID, VALUE>);
		task.submit();

		assert(get_value<ID>() == VALUE);
	}

	// Two execution agents task|post, rvalue submit
	{
		reset_values<ID, ID2>();

		adl::task(&set_value<ID, VALUE>)
			.post(&set_value<ID2, VALUE>)
			.submit();

		assert(get_value<ID>() == VALUE);
		assert(get_value<ID2>() == VALUE);
	}

	// Two execution agents task|post, lvalue submit
	{
		reset_values<ID, ID2>();

		auto task = adl::task(&set_value<ID, VALUE>)
			.post(&set_value<ID2, VALUE>);
		task.submit();

		assert(get_value<ID>() == VALUE);
		assert(get_value<ID2>() == VALUE);
	}
}

#define TEST_POST(T1, T2, T3, T4) \
	reset_values<PID, PID2, PID3, PID4>(); \
	adl::task(set_ ## T1 ## <PID, PVALUE>) \
		.post(set_ ## T2 ## <PID2, PVALUE>) \
		.post(set_ ## T3 ## <PID3, PVALUE>) \
		.post(set_ ## T4 ## <PID4, PVALUE>) \
		.submit(); \
	test_values()

void test_Task_post()
{
	auto test_values = []
	{
		assert(get_value<PID>() == PVALUE);
		assert(get_value<PID2>() == PVALUE);
		assert(get_value<PID3>() == PVALUE);
		assert(get_value<PID4>() == PVALUE);
	};

	// Test various combinations of functions with (r) or without (v) return value
	{
		TEST_POST(v, v, v, v);
		TEST_POST(r, r, r, r);
	}
	{
		TEST_POST(v, r, r, r);
		TEST_POST(v, v, r, r);
		TEST_POST(v, v, r, r);
		TEST_POST(v, v, v, r);
	}
	{
		TEST_POST(r, r, r, v);
		TEST_POST(r, r, v, v);
		TEST_POST(r, v, v, v);
	}
	{
		TEST_POST(r, v, v, r);
		TEST_POST(r, r, v, r);
	}
	{
		TEST_POST(v, r, r, v);
		TEST_POST(v, v, r, v);
	}
}

#undef TEST_POST

void test_Task_then()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t GEN = __LINE__;
	constexpr size_t ADD1 = __LINE__;
	constexpr size_t ADD2 = __LINE__;
	constexpr size_t ADD3 = __LINE__;

	{
		reset_value<ID>();

		adl::task(&generate<GEN>)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>)
			.then(&set_a<ID>)
			.submit();

		assert(get_value<ID>() == GEN + ADD1 + ADD2 + ADD3);
	}

	{
		reset_value<ID>();

		adl::task(&generate<GEN>)
			.post(&void_fn) // post should ignore previous return value and pass it to the next 'then' continuation
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>)
			.then(&set_a<ID>)
			.submit();

		assert(get_value<ID>() == GEN + ADD1 + ADD2 + ADD3);
	}

	{
		reset_value<ID>();

		adl::task(&generate<GEN>)
			.post(&void_fn) // post should ignore previous return value and pass it to the next 'then' continuation
			.post(&void_fn) 
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>)
			.then(&set_a<ID>)
			.submit();

		assert(get_value<ID>() == GEN + ADD1 + ADD2 + ADD3);
	}

	{
		reset_value<ID>();

		adl::task(&generate<GEN>)
			.post(&void_fn) // post should ignore previous return value and pass it to the next 'then' continuation
			.post(&void_fn)
			.post(&void_fn)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>)
			.then(&set_a<ID>)
			.submit();

		assert(get_value<ID>() == GEN + ADD1 + ADD2 + ADD3);
	}

	{
		reset_value<ID>();

		adl::task(&generate<GEN>)
			.post(&void_fn) // post should ignore previous return value and pass it to the next 'then' continuation
			.post(&void_fn)
			.then(&add<ADD1>)
			.post(&void_fn)
			.then(&add<ADD2>)
			.post(&void_fn)
			.then(&add<ADD3>)
			.post(&void_fn)
			.then(&set_a<ID>)
			.submit();

		assert(get_value<ID>() == GEN + ADD1 + ADD2 + ADD3);
	}
	
	{
		reset_value<ID>();

		adl::task(&void_fn)
			.then(&generate<GEN>)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>)
			.then(&set_a<ID>)
			.submit();

		assert(get_value<ID>() == GEN + ADD1 + ADD2 + ADD3);
	}

	{
		reset_value<ID>();

		adl::task(&void_fn)
			.then(&void_fn)
			.then(&generate<GEN>)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>)
			.then(&set_a<ID>)
			.submit();

		assert(get_value<ID>() == GEN + ADD1 + ADD2 + ADD3);
	}

	{
		reset_value<ID>();

		adl::task(&void_fn)
			.then(&void_fn)			
			.then(&generate<GEN>)
			.then(&add<ADD1>)
			.then(&add<ADD2>)
			.then(&add<ADD3>)
			.then(&set_a<ID>)
			.then(&void_fn)
			.submit();

		assert(get_value<ID>() == GEN + ADD1 + ADD2 + ADD3);
	}
}

void test_Task_post_bulk()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t ID2 = __LINE__;
	constexpr size_t ID3 = __LINE__;
	constexpr size_t GEN = __LINE__;
	constexpr size_t ADD1 = __LINE__;
	constexpr size_t ADD2 = __LINE__;
	constexpr size_t ADD3 = __LINE__;

	{
		reset_values<ID, ID2, ID3>();

		adl::task(&void_fn)
			.post_bulk(&set_v<ID, ADD1>, &set_v<ID2, ADD2>, &set_v<ID3, ADD3>)
			.submit();

		assert(get_value<ID>() == ADD1);
		assert(get_value<ID2>() == ADD2);
		assert(get_value<ID3>() == ADD3);
	}

	{
		reset_values<ID, ID2, ID3>();

		adl::task(&generate<GEN>)
			.post_bulk(&set_v<ID, ADD1>, &set_v<ID2, ADD2>)
			.then(&add<ADD3>)
			.then(&set_a<ID3>)
			.submit();

		assert(get_value<ID>() == ADD1);
		assert(get_value<ID2>() == ADD2);
		assert(get_value<ID3>() == GEN + ADD3);
	}

	{
		reset_values<ID, ID2, ID3>();

		adl::task(&void_fn)
			.then(&generate<GEN>)
			.post_bulk(&set_v<ID, ADD1>, &set_v<ID2, ADD2>)
			.then(&add<ADD3>)
			.post_bulk(&add_v<ID, ADD1>, &add_v<ID2, ADD2>)
			.then(&set_a<ID3>)
			.submit();

		assert(get_value<ID>() == ADD1 + ADD1);
		assert(get_value<ID2>() == ADD2 + ADD2);
		assert(get_value<ID3>() == GEN + ADD3);
	}

	{
		reset_values<ID, ID2, ID3>();

		adl::task(&void_fn)
			.then(&void_fn)
			.then(&generate<GEN>)
			.post_bulk(&set_v<ID, ADD1>, &set_v<ID2, ADD2>)
			.then(&add<ADD3>)
			.post_bulk(&add_v<ID, ADD1>, &add_v<ID2, ADD2>)
			.then(&set_a<ID3>)
			.submit();

		assert(get_value<ID>() == ADD1 + ADD1);
		assert(get_value<ID2>() == ADD2 + ADD2);
		assert(get_value<ID3>() == GEN + ADD3);
	}
}

void test_Task_nested()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t ID2 = __LINE__;
	constexpr size_t ID3 = __LINE__;
	constexpr size_t GEN = __LINE__;
	constexpr size_t ADD1 = __LINE__;
	constexpr size_t ADD2 = __LINE__;
	constexpr size_t ADD3 = __LINE__;

	auto t1 = adl::task(&generate<GEN>)
		.then(&add<ADD1>)
		.then(&add<ADD2>)
		.then(&add<ADD3>)
		.then(&set_a<ID>);

	auto t2 = adl::task(&generate<GEN>)
		.then(&add<ADD1>)
		.then(&add<ADD2>)
		.then(&add<ADD3>)
		.then(&set_a<ID2>);

	auto t3 = adl::task(&generate<GEN>)
		.then(&add<ADD1>)
		.then(&add<ADD2>)
		.then(&add<ADD3>)
		.then(&set_a<ID3>);

	{
		reset_values<ID, ID2, ID3>();

		auto tn = adl::task(t1)
			.then(t2)
			.then(t3);
			
		tn.submit();

		assert(get_value<ID>() == GEN + ADD1 + ADD2 + ADD3);
		assert(get_value<ID2>() == GEN + ADD1 + ADD2 + ADD3);
		assert(get_value<ID3>() == GEN + ADD1 + ADD2 + ADD3);
	}

	{
		reset_values<ID, ID2, ID3>();

		adl::task(&void_fn)
			.post_bulk(t1, t2, t3)
			.submit();

		assert(get_value<ID>() == GEN + ADD1 + ADD2 + ADD3);
		assert(get_value<ID2>() == GEN + ADD1 + ADD2 + ADD3);
		assert(get_value<ID3>() == GEN + ADD1 + ADD2 + ADD3);
	}
}

void test_Task()
{
	test_Task_submit();
	test_Task_post();
	test_Task_then();
	test_Task_post_bulk();
	test_Task_nested();
}