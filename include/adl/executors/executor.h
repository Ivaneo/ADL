#pragma once
#include <future>
#include <memory>

namespace adl
{
	namespace details
	{
		template<typename F>
		static auto make_promise()
		{
			// Future task captures the promise and then stored inside the std::function,
			// but there exists a fundamental requirement, that the wrapped functor should be copy-constructible to perform type erasure.
			// so we can't use raw std::promise as it is movable only, that's why shared_ptr is used.
			return std::make_shared<std::promise<std::invoke_result_t<F>>>();
		}

		template<typename... Args>
		static auto make_promises()
		{
			return std::make_tuple(make_promise<Args>()...);
		}

		template<typename T>
		static auto get_future(std::shared_ptr<std::promise<T>>& promise)
		{
			return promise->get_future();
		}

		template<typename T>
		static auto get_futures(T&& promises)
		{
			return std::apply([](auto&&... args) { return std::make_tuple(get_future(std::forward<decltype(args)>(args))...); }, std::forward<T>(promises));
		}

		template<typename T, typename F>
		static auto make_task(std::shared_ptr<std::promise<T>>&& promise, F&& callable)
		{
			if constexpr (std::is_void_v<std::invoke_result_t<F>>)
			{
				return[promise = std::move(promise), callable = std::forward<F>(callable)]()
				{
					std::invoke(callable);
					promise->set_value();
				};
			}
			else
			{
				return[promise = std::move(promise), callable = std::forward<F>(callable)]()
				{
					promise->set_value(std::invoke(callable));
				};
			}
		}

		template<typename T, typename F, std::size_t... Is>
		static auto make_tasks_impl(T&& promises, F&& callables, std::index_sequence<Is...>)
		{
			return std::make_tuple(make_task(std::move(std::get<Is>(promises)), std::move(std::get<Is>(callables)))...);
		}

		template<typename T, typename... Args>
		static auto make_tasks(T&& promises, Args&&... callables)
		{
			return make_tasks_impl(std::move(promises), std::make_tuple(std::forward<Args>(callables)...), std::index_sequence_for<Args...>{});
		}
	}
}