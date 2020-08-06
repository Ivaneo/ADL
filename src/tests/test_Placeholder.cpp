#include "test.hpp"
#include <adl/placeholder.h>

namespace
{
	constexpr size_t PID0 = __LINE__;
	constexpr size_t PID1 = __LINE__;
	constexpr size_t PID2 = __LINE__;
	constexpr size_t PVALUE0 = __LINE__;
	constexpr size_t PVALUE1 = __LINE__;
	constexpr size_t PVALUE2 = __LINE__;

	void foo_no_arg()
	{
		ValueHolder<PID0>::value = PVALUE0;
	}

	void foo_one_arg(size_t value)
	{
		ValueHolder<PID1>::value = value;
	}

	void foo_two_arg(size_t value1, size_t value2)
	{
		ValueHolder<PID1>::value = value1;
		ValueHolder<PID2>::value = value2;
	}

	size_t foo_with_result()
	{
		return set_value<PID0, PVALUE0>();
	}
}

void test_Placeholder_result()
{
	// These tests should compile!

	using foo_no_arg_t = decltype(&foo_no_arg);
	using foo_one_arg_t = decltype(&foo_one_arg);
	using foo_two_arg_t = decltype(&foo_two_arg);
	using foo_with_result_t = decltype(&foo_with_result);

	static_assert(adl::is_placeholder_v<adl::result_of_ignoring_placeholder_t<foo_no_arg_t>>);
	static_assert(adl::is_placeholder_v<adl::result_of_ignoring_placeholder_t<foo_one_arg_t, size_t>>);
	static_assert(adl::is_placeholder_v<adl::result_of_ignoring_placeholder_t<foo_two_arg_t, size_t, size_t>>);

	static_assert(adl::is_placeholder_v<adl::result_of_ignoring_placeholder_t<foo_no_arg_t, adl::placeholder>>);
	static_assert(adl::is_placeholder_v<adl::result_of_ignoring_placeholder_t<foo_one_arg_t, adl::placeholder, size_t>>);
	static_assert(adl::is_placeholder_v<adl::result_of_ignoring_placeholder_t<foo_two_arg_t, adl::placeholder, size_t, size_t>>);

	static_assert(!adl::is_placeholder_v<adl::result_of_ignoring_placeholder_t<foo_with_result_t>>);
}

void test_Placeholder_replace()
{
	{
		reset_value<PID0>();

		auto result = adl::replace_void_result_with_placeholder(&foo_no_arg);

		static_assert(adl::is_placeholder_v<decltype(result)>);
		assert(get_value<PID0>() == PVALUE0);
	}
	
	{
		reset_value<PID1>();

		auto result = adl::replace_void_result_with_placeholder(&foo_one_arg, PVALUE1);

		static_assert(adl::is_placeholder_v<decltype(result)>);
		assert(get_value<PID1>() == PVALUE1);
	}

	{
		reset_values<PID1, PID2>();

		auto result = adl::replace_void_result_with_placeholder(&foo_two_arg, PVALUE1, PVALUE2);

		static_assert(adl::is_placeholder_v<decltype(result)>);
		assert(get_value<PID1>() == PVALUE1);
		assert(get_value<PID2>() == PVALUE2);
	}

	{
		reset_value<PID0>();		

		auto result = adl::replace_void_result_with_placeholder(&foo_with_result);

		static_assert(std::is_same_v<size_t, decltype(result)>);
		assert(get_value<PID0>() == PVALUE0);
	}
}

void test_Placeholder_ignore()
{
	{
		reset_value<PID0>();

		auto result = adl::invoke_ignoring_placeholder(&foo_no_arg, adl::placeholder_v);

		static_assert(adl::is_placeholder_v<decltype(result)>);
		assert(get_value<PID0>() == PVALUE0);
	}

	{
		reset_value<PID1>();

		auto result = adl::invoke_ignoring_placeholder(&foo_one_arg, adl::placeholder_v, PVALUE1);

		static_assert(adl::is_placeholder_v<decltype(result)>);
		assert(get_value<PID1>() == PVALUE1);
	}

	{
		reset_values<PID1, PID2>();

		auto result = adl::invoke_ignoring_placeholder(&foo_two_arg, adl::placeholder_v, PVALUE1, PVALUE2);

		static_assert(adl::is_placeholder_v<decltype(result)>);
		assert(get_value<PID1>() == PVALUE1);
		assert(get_value<PID2>() == PVALUE2);
	}

	{
		reset_value<PID0>();

		auto result = adl::invoke_ignoring_placeholder(&foo_with_result, adl::placeholder_v);

		static_assert(std::is_same_v<size_t, decltype(result)>);
		assert(get_value<PID0>() == PVALUE0);
	}
}

void test_Placeholder_apply()
{
	{
		reset_value<PID0>();

		auto result = adl::apply_ignoring_placeholder(&foo_no_arg, std::make_tuple(adl::placeholder_v));

		static_assert(adl::is_placeholder_v<decltype(result)>);
		assert(get_value<PID0>() == PVALUE0);
	}

	{
		reset_value<PID1>();

		auto result = adl::apply_ignoring_placeholder(&foo_one_arg, std::make_tuple(adl::placeholder_v, PVALUE1));

		static_assert(adl::is_placeholder_v<decltype(result)>);
		assert(get_value<PID1>() == PVALUE1);
	}

	{
		reset_values<PID1, PID2>();

		auto result = adl::apply_ignoring_placeholder(&foo_two_arg, std::make_tuple(adl::placeholder_v, PVALUE1, PVALUE2));

		static_assert(adl::is_placeholder_v<decltype(result)>);
		assert(get_value<PID1>() == PVALUE1);
		assert(get_value<PID2>() == PVALUE2);
	}

	{
		reset_value<PID0>();

		auto result = adl::apply_ignoring_placeholder(&foo_with_result, std::make_tuple(adl::placeholder_v));

		static_assert(std::is_same_v<size_t, decltype(result)>);
		assert(get_value<PID0>() == PVALUE0);
	}
}

void test_Placeholder()
{
	test_Placeholder_result();
	test_Placeholder_replace();
	test_Placeholder_ignore();
	test_Placeholder_apply();
}