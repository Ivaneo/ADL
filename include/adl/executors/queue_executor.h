#pragma once
#include <functional>
#include <queue>
#include <mutex>
#include <future>

namespace adl {

class QueueExecutor
{
public:

    template<typename F>
    void execute(F&& callable)
    {
        std::unique_lock lock{ m_tasksMutex };
        m_tasks.emplace(std::forward<F>(callable));
    }

	template<typename... Args>
	void bulk_execute(Args&&... callables)
	{
		std::unique_lock lock{ m_tasksMutex };
        (..., m_tasks.emplace(std::forward<Args>(callables)));
    }

	template<typename F>
	void defer_execute(F&& callable)
	{
		std::unique_lock lock{ m_deferredTasksMutex };
		m_deferredTasks.emplace(std::forward<F>(callable));
	}

    template<typename F>
    auto future_execute(F&& callable)
    {
		auto promise = make_promise<F>();
		auto future = get_future(promise);
		auto task = make_task(std::move(promise), std::forward<F>(callable));

		{
			std::unique_lock lock{ m_tasksMutex };
			m_tasks.emplace(std::move(task));
		}

		return std::move(future);
    }

    template<typename... Args>
    auto future_bulk_execute(Args&&... callables)
    {
		auto promises = make_promises<Args...>();
		auto futures = get_futures(promises);
		auto tasks = make_tasks(std::move(promises), std::forward<Args>(callables)...);

		{
			std::unique_lock lock{ m_tasksMutex };
			std::apply([this](auto&&... args) { (..., m_tasks.emplace(std::forward<decltype(args)>(args))); }, std::move(tasks));
		}

		return std::move(futures);
    }

    void dispatch()
    {   
        while (!m_tasks.empty())
        {
            std::unique_lock lock{ m_tasksMutex };
            auto task = std::move(m_tasks.front());
            m_tasks.pop();
            lock.unlock();

            std::invoke(task);
        }

		std::scoped_lock lock{ m_deferredTasksMutex, m_tasksMutex };
		while (!m_deferredTasks.empty())
		{
			m_tasks.emplace(std::move(m_deferredTasks.front()));
			m_deferredTasks.pop();
		}
    }

private:

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
			return [promise = std::move(promise), callable = std::forward<F>(callable)]()
			{
				std::invoke(callable);
				promise->set_value();
			};
		}
		else
		{
			return [promise = std::move(promise), callable = std::forward<F>(callable)]()
			{
				promise->set_value(std::invoke(callable));
			};
		}
	}

	template<typename T, typename... Args>
	static auto make_tasks(T&& promises, Args&&... callables)
	{
		return make_tasks_impl(std::move(promises), std::make_tuple(std::forward<Args>(callables)...), std::index_sequence_for<Args...>{});
	}

	template<typename T, typename F, std::size_t... Is>
	static auto make_tasks_impl(T&& promises, F&& callables, std::index_sequence<Is...>)
	{
		return std::make_tuple(make_task(std::move(std::get<Is>(promises)), std::move(std::get<Is>(callables)))...);
	}

private:

    std::mutex m_tasksMutex;
	std::mutex m_deferredTasksMutex;
    std::queue<std::function<void()>> m_tasks;
	std::queue<std::function<void()>> m_deferredTasks;
};

}