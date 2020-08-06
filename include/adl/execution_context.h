#pragma once
#include "placeholder.h"

namespace adl
{
	class ExecutionContext
	{
	public:

		// Cancel further continuations
		void cancel() { m_canceled = true; }

		bool is_canceled() const { return m_canceled; }

	private:

		bool m_canceled = false;
	};

	namespace details
	{
		// Wrap execution context with result
		template<typename T>
		struct ResultWithContext
		{
			using result_t = T;

			T result;
			adl::ExecutionContext context;
		};

		template<typename T>
		struct result_with_context_traits : std::false_type
		{};

		template<typename T>
		struct result_with_context_traits<ResultWithContext<T>> : std::true_type
		{};

		template<typename T>
		inline constexpr bool is_result_with_context_v = result_with_context_traits<std::decay_t<T>>::value;

		template<typename F, typename... Args>
		inline constexpr bool is_invokable_with_context_v = std::is_invocable_v<F, ExecutionContext&, Args...>;

		template<typename T>
		auto make_result_with_context(T&& result)
		{
			if constexpr (is_result_with_context_v<T>)
			{
				return std::forward<T>(result);
			}
			else
			{
				return ResultWithContext<T>{ std::forward<T>(result) };
			}
		}

		template<typename T>
		auto make_result_with_context(T&& result, const adl::ExecutionContext& context)
		{
			return ResultWithContext<T>{ std::forward<T>(result), context };
		}

		template<typename T>
		constexpr auto unwrap_execution_result(ResultWithContext<T>&& result)
		{
			return std::move(result.result);
		}

		template<typename T>
		constexpr auto unwrap_execution_result(T&& result)
		{
			return std::forward<T>(result);
		}

		template <typename F, typename T>
		constexpr auto try_invoke_with_context_impl(F&& f, T&& result)
		{
			if constexpr (is_invokable_with_context_v<F, T>)
			{
				ExecutionContext context;
				return make_result_with_context(replace_void_result_with_placeholder(std::forward<F>(f), context, std::forward<T>(result)), context);
			}
			else if constexpr (is_invokable_with_context_v<F>)
			{
				ExecutionContext context;
				return make_result_with_context(replace_void_result_with_placeholder(std::forward<F>(f), context), context);
			}
			else
			{
				return invoke_ignoring_placeholder(std::forward<F>(f), std::forward<T>(result));
			}
		}

		template<typename F, typename T>
		constexpr auto try_invoke_with_context(F&& f, T&& result)
		{
			return try_invoke_with_context_impl(std::forward<F>(f), unwrap_execution_result(std::forward<T>(result)));
		}

		template<typename F>
		constexpr auto try_invoke_with_context(F&& f)
		{
			return try_invoke_with_context_impl(std::forward<F>(f), placeholder_v);
		}

		template<typename T>
		constexpr bool is_execution_canceled(ResultWithContext<T>& result)
		{
			return result.context.is_canceled();
		}

		template<typename T>
		constexpr bool is_execution_canceled(T&&)
		{
			return false;
		}

		template<typename T>
		using result_of_unwrap_execution_result_t = decltype(unwrap_execution_result(std::declval<std::decay_t<T>>()));

		template <typename F, typename... Args>
		using result_of_invoke_with_context_t = decltype(try_invoke_with_context(std::declval<F>(), std::declval<Args>()...));
	}
}