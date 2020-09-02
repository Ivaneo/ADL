#pragma once
// Copyright (c) 2020 Ivan Miasnikov | mailto:ivaneotg@gmail.com
// MIT License | https://opensource.org/licenses/MIT

#pragma once
#include "executor.h"
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
		auto promise = details::make_promise<F>();
		auto future = details::get_future(promise);
		auto task = details::make_task(std::move(promise), std::forward<F>(callable));

		{
			std::unique_lock lock{ m_tasksMutex };
			m_tasks.emplace(std::move(task));
		}

		return std::move(future);
    }

    template<typename... Args>
    auto future_bulk_execute(Args&&... callables)
    {
		auto promises = details::make_promises<Args...>();
		auto futures = details::get_futures(promises);
		auto tasks = details::make_tasks(std::move(promises), std::forward<Args>(callables)...);

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

    std::mutex m_tasksMutex;
	std::mutex m_deferredTasksMutex;
    std::queue<std::function<void()>> m_tasks;
	std::queue<std::function<void()>> m_deferredTasks;
};

}