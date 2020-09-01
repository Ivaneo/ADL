#include "test.hpp"
#include <adl/task.h>
#include <adl/executors/queue_executor.h>

namespace
{
	enum class ChannelType : int
	{
		Q1 = 1,
		Q2 = 2,
		Q3 = 3,
		Q4 = 4,
	};

	using Channel_Q1 = adl::Channel<ChannelType, ChannelType::Q1, adl::QueueExecutor>;
	using Channel_Q2 = adl::Channel<ChannelType, ChannelType::Q2, adl::QueueExecutor>;
	using Channel_Q3 = adl::Channel<ChannelType, ChannelType::Q3, adl::QueueExecutor>;
	using Channel_Q4 = adl::Channel<ChannelType, ChannelType::Q4, adl::QueueExecutor>;

	template<size_t value>
	size_t generate_cancel(adl::ExecutionContext& context)
	{
		context.cancel();
		return generate<value>();
	}

	void void_cancel(adl::ExecutionContext& context)
	{
		context.cancel();
	}

	void void_no_cancel(adl::ExecutionContext& context)
	{
		void_fn();
	}

	template<size_t value>
	size_t cancel_if_equal(adl::ExecutionContext& context, size_t prevValue)
	{
		if (value == prevValue)
		{
			context.cancel();
		}

		return prevValue;
	}

	template<size_t ID>
	size_t set_and_return(size_t value)
	{
		ValueHolder::value = value;
		return ValueHolder::value;
	}

	template<size_t value>
	struct GenerateDeferOnce
	{
		size_t operator()(adl::ExecutionContext& context)
		{
			if (defer)
			{
				context.defer();
				defer = false;
			}

			return value;
		}

		bool defer = true;
	};

	template<size_t addValue>
	struct AddDeferOnce
	{
		size_t operator()(adl::ExecutionContext& context, size_t prevValue)
		{
			if (defer)
			{
				context.defer();
				defer = false;
			}

			return prevValue + addValue;
		}

		bool defer = true;
	};
}

void test_Task_ExecutionCancel()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t GEN = __LINE__;
	constexpr size_t ADD = __LINE__;

	{
		// Should not compile since its a task with no continuations
		//adl::task(&void_cancel)
		//	.submit();
	}

	{
		reset_value<ID>();

		auto t = adl::task<Channel_Q1>(&generate_cancel<GEN>) // all further continuation should not be invoked
			.then<Channel_Q2>(&add<ADD>)
			.then(&set_a<ID>);

		t.submit();

		adl::dispatch<Channel_Q1>();
		adl::dispatch<Channel_Q2>();

		assert(get_value<ID>() == 0);
	}

	{
		reset_value<ID>();

		auto t = adl::task<Channel_Q1>(&void_cancel) // all further continuation should not be invoked			
			.then<Channel_Q2>(&generate<GEN>)
			.then(&add<ADD>)
			.then(&set_a<ID>);

		t.submit();

		adl::dispatch<Channel_Q1>();
		adl::dispatch<Channel_Q2>();

		assert(get_value<ID>() == 0);
	}

	{
		reset_value<ID>();

		auto t = adl::task<Channel_Q1>(&generate<GEN>) 
			.then<Channel_Q2>(&cancel_if_equal<GEN>) // all further continuation should not be invoked
			.then<Channel_Q3>(&add<ADD>)
			.then(&set_a<ID>);

		t.submit();

		adl::dispatch<Channel_Q1>();
		adl::dispatch<Channel_Q2>();
		adl::dispatch<Channel_Q3>();

		assert(get_value<ID>() == 0);
	}

	{
		reset_value<ID>();

		auto t = adl::task<Channel_Q1>(&generate<GEN>)
			.then<Channel_Q2>(&cancel_if_equal<GEN + 1>)
			.then<Channel_Q3>(&add<ADD>)
			.then(&set_a<ID>);

		t.submit();

		adl::dispatch<Channel_Q1>();
		adl::dispatch<Channel_Q2>();
		adl::dispatch<Channel_Q3>();

		assert(get_value<ID>() == GEN + ADD);
	}

	{
		reset_value<ID>();

		auto t = adl::task<Channel_Q1>(&generate<GEN>)
			.then<Channel_Q2>(&cancel_if_equal<GEN + 1>)
			.then<Channel_Q2>(&cancel_if_equal<GEN>) // all further continuation should not be invoked
			.then<Channel_Q3>(&add<ADD>)
			.then(&set_a<ID>);

		t.submit();

		adl::dispatch<Channel_Q1>();
		adl::dispatch<Channel_Q2>();
		adl::dispatch<Channel_Q3>();

		assert(get_value<ID>() == 0);
	}

	{
		reset_value<ID>();

		auto t = adl::task<Channel_Q1>(&generate<GEN>)
			.then<Channel_Q2>(&cancel_if_equal<GEN>) // all further continuation should not be invoked
			.then(&add<ADD>)
			.then(&set_a<ID>);

		t.submit();

		adl::dispatch<Channel_Q1>();
		adl::dispatch<Channel_Q2>();

		assert(get_value<ID>() == 0);
	}

	{
		reset_value<ID>();

		auto t = adl::task<Channel_Q1>(&generate<GEN>)			
			.then<Channel_Q2>(&add<ADD>)
			.then(&cancel_if_equal<GEN + ADD>) // all further continuation should not be invoked
			.then(&set_a<ID>);

		t.submit();

		adl::dispatch<Channel_Q1>();
		adl::dispatch<Channel_Q2>();

		assert(get_value<ID>() == 0);
	}


	{
		reset_value<ID>();

		adl::task<Channel_Q1>([] { return 10; })
			.then<Channel_Q2>([](adl::ExecutionContext& context, size_t value) { if (value % 2 == 0) context.cancel(); return value; })
			.then(&set_a<ID>) // should not be called
			.submit();

		adl::dispatch<Channel_Q1>();
		adl::dispatch<Channel_Q2>();

		assert(get_value<ID>() == 0);
	}
}

void test_Task_ExecutionDefer()
{
	constexpr size_t ID = __LINE__;
	constexpr size_t GEN = __LINE__;
	constexpr size_t ADD = __LINE__;

	{
		reset_value<ID>();

		auto t = adl::task<Channel_Q1>(GenerateDeferOnce<GEN>{})
			.then<Channel_Q2>(&add<ADD>)
			.then(&set_a<ID>);

		t.submit();

		adl::dispatch<Channel_Q1>();
		adl::dispatch<Channel_Q2>();

		// On the first dispatch task should be deferred
		assert(get_value<ID>() == 0);

		adl::dispatch<Channel_Q1>();
		adl::dispatch<Channel_Q2>();

		assert(get_value<ID>() == GEN + ADD);
	}

	{
		reset_value<ID>();

		auto t = adl::task<Channel_Q1>(&generate<GEN>)
			.then<Channel_Q2>(AddDeferOnce<ADD>{})
			.then(&set_a<ID>);

		t.submit();

		adl::dispatch<Channel_Q1>();
		adl::dispatch<Channel_Q2>();

		// On the first dispatch task should be deferred
		assert(get_value<ID>() == 0);

		adl::dispatch<Channel_Q2>();

		assert(get_value<ID>() == GEN + ADD);
	}
}

void test_Task_ExecutionContext()
{
	test_Task_ExecutionCancel();
	test_Task_ExecutionDefer();
}