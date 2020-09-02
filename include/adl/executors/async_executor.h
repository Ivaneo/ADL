// Copyright (c) 2020 Ivan Miasnikov | mailto:ivaneotg@gmail.com
// MIT License | https://opensource.org/licenses/MIT

#pragma once
#include <future>
#include <mutex>
#include <list>

namespace adl
{
    class AsyncExecutor
    {
    public:

        template<typename F>
        void execute(F&& callable)
        {
            auto&& future = future_execute(std::forward<F>(callable));
            // future can't be discarded since it will block the thread until the result is ready, so deferring future deletion
            std::unique_lock lock{ m_mutex };
            m_futures.emplace_back([sharedFuture = future.share()]() -> bool
            {
                return !sharedFuture.valid() || sharedFuture._Is_ready();
            });
        }

        template<typename... Args>
        void bulk_execute(Args&&... callables)
        {
            auto futures = std::make_tuple(future_execute(std::forward<Args>(callables))...);

            std::unique_lock lock{ m_mutex };
            std::apply([this](auto&&... future)
            {
                (..., m_futures.emplace_back([sharedFuture = future.share()]() -> bool
                {
                    return !sharedFuture.valid() || sharedFuture._Is_ready();
                }));
            }, std::move(futures));
        }

		template<typename F>
		void defer_execute(F&& callable)
		{
			execute(std::forward<F>(callable));
		}

        template<typename F>
        auto future_execute(F&& callable)
        {
            return std::async(std::launch::async, std::forward<F>(callable));
        }

        template<typename... Args>
        auto future_bulk_execute(Args&&... callables)
        {
            return std::make_tuple(future_execute(std::forward<Args>(callables))...);
        }

        void dispatch()
        {
            if (!m_futures.empty())
            {
                std::unique_lock lock{ m_mutex };
                for (auto it = m_futures.begin(); it != m_futures.end();)
                {
                    if (std::invoke(*it))
                    {
                        it = m_futures.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
            }
        }

    private:

        std::mutex m_mutex;
        std::list<std::function<bool()>> m_futures;
    };

} // namespace adl