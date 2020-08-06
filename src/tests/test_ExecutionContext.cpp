#include "test.hpp"
#include <adl/execution_context.h>

namespace
{
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

	template<size_t value, size_t errValue>
	size_t cancel_if_equal(adl::ExecutionContext& context, size_t prevValue)
	{
		if (value == prevValue)
		{
			context.cancel();
			return errValue;
		}

		return prevValue;
	}

	template<size_t value, size_t errValue>
	size_t cancel_if_equal_ref(adl::ExecutionContext& context, const size_t& prevValue)
	{
		if (value == prevValue)
		{
			context.cancel();
			return errValue;
		}

		return prevValue;
	}

	template<size_t value, size_t errValue>
	size_t cancel_if_equal_refref(adl::ExecutionContext& context, size_t&& prevValue)
	{
		if (value == prevValue)
		{
			context.cancel();
			return errValue;
		}

		return prevValue;
	}

	template<size_t ID>
	size_t set_and_return(size_t value)
	{
		ValueHolder::value = value;
		return ValueHolder::value;
	}

	constexpr size_t ID = __LINE__;
	constexpr size_t VALUE = __LINE__;
	constexpr size_t ERRVALUE = __LINE__;
}

void test_invoke_with_context()
{
	{
		auto result = adl::details::try_invoke_with_context(&generate<VALUE>);

		using return_type = std::invoke_result_t<decltype(&generate<VALUE>)>;
		using result_unwrap_type = adl::details::result_of_unwrap_execution_result_t<decltype(result)>;
		static_assert(std::is_same_v<return_type, result_unwrap_type>);

		assert(result == VALUE);

		auto result2 = adl::details::try_invoke_with_context(&cancel_if_equal<VALUE, ERRVALUE>, std::move(result));

		using result2_unwrap_type = adl::details::result_of_unwrap_execution_result_t<decltype(result2)>;
		static_assert(std::is_same_v<return_type, result2_unwrap_type>);

		assert(result2.result == ERRVALUE);
		assert(adl::details::is_execution_canceled(result2));
	}

	{
		auto result = adl::details::try_invoke_with_context(&generate<VALUE>);

		assert(result == VALUE);

		auto result2 = adl::details::try_invoke_with_context(&cancel_if_equal_ref<VALUE, ERRVALUE>, std::move(result));

		assert(result2.result == ERRVALUE);
		assert(adl::details::is_execution_canceled(result2));
	}

	{
		auto result = adl::details::try_invoke_with_context(&generate<VALUE>);

		assert(result == VALUE);

		auto result2 = adl::details::try_invoke_with_context(&cancel_if_equal_refref<VALUE, ERRVALUE>, std::move(result));

		assert(result2.result == ERRVALUE);
		assert(adl::details::is_execution_canceled(result2));
	}

	{
		auto result = adl::details::try_invoke_with_context(&generate<VALUE>);

		assert(result == VALUE);

		auto result2 = adl::details::try_invoke_with_context(&generate_cancel<ERRVALUE>, result);

		assert(result2.result == ERRVALUE);
		assert(adl::details::is_execution_canceled(result2));

		auto result3 = adl::details::try_invoke_with_context(&generate_cancel<ERRVALUE>);

		assert(result3.result == ERRVALUE);
		assert(adl::details::is_execution_canceled(result3));
	}

	{
		auto result2 = adl::details::try_invoke_with_context(&void_cancel);

		assert(adl::details::is_execution_canceled(result2));
	}

	{
		//auto result = adl::details::try_invoke_with_context(&generate<VALUE>);

		// #TODO: this not compile!
		// Since function do not accept any arguments, is result should be discarded and function invoked? Or this should be an error? 
		// if this is an error do we need to provide a better error msg with static_assert ?
		// if this is not an error, should we provide some warning that result is discarded? 
		// So many questions =(
		// auto result2 = adl::details::invoke_with_context(&void_fn, result);
	}

	{
		reset_value<ID>();

		auto result = adl::details::try_invoke_with_context(&generate<VALUE>);
		auto result2 = adl::details::try_invoke_with_context(&set_a<ID>, result);

		assert(get_value<ID>() == VALUE);

		static_assert(!adl::details::is_execution_canceled(result2));

		if (adl::details::is_execution_canceled(result2))
		{
			// unreachable code, should not be generated since type of result2 is not ExecutionResult and is_execution_canceled always return false
			assert(false);
		}
	}
}

void test_ExecutionContext()
{
	test_invoke_with_context();
}