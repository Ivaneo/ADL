#pragma once
#include "executor.h"
#include <functional>
#include <vector>
#include <mutex>
#include <future>

namespace adl {

	class StrandExecutor
	{
	public:

		template<typename F>
		void execute(F&& callable)
		{
			std::unique_lock lock{ m_mutex };
			m_tasks.emplace_back(std::forward<F>(callable));
		}

		template<typename... Args>
		void bulk_execute(Args&&... callables)
		{
			std::unique_lock lock{ m_mutex };
			(..., m_tasks.emplace_back(std::forward<Args>(callables)));
		}

		template<typename F>
		void defer_execute(F&& callable)
		{
			// All executions in strand are deferred
			execute(std::forward<F>(callable));
		}

		template<typename F>
		auto future_execute(F&& callable)
		{
			auto promise = details::make_promise<F>();
			auto future = details::get_future(promise);
			auto task = details::make_task(std::move(promise), std::forward<F>(callable));

			{
				std::unique_lock lock{ m_mutex };
				m_tasks.emplace_back(std::move(task));
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
				std::unique_lock lock{ m_mutex };
				std::apply([this](auto&&... args) { (..., m_tasks.emplace_back(std::forward<decltype(args)>(args))); }, std::move(tasks));
			}

			return std::move(futures);
		}

		void dispatch()
		{
			if (!m_tasks.empty())
			{
				std::unique_lock lock{ m_mutex };
				auto tasks = std::move(m_tasks);
				m_tasks.clear();
				lock.unlock();

				for (auto&& task : tasks)
				{
					task();
				}
			}
		}

	private:

		std::mutex m_mutex;
		std::vector<std::function<void()>> m_tasks;
	};

}