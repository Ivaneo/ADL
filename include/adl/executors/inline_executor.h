// Copyright (c) 2020 Ivan Miasnikov | mailto:ivaneotg@gmail.com
// MIT License | https://opensource.org/licenses/MIT

#pragma once
#include <type_traits>
#include <tuple>
#include <future>

namespace adl {

class InlineExecutor
{
public:

    template<typename F>
    inline constexpr void execute(F&& callable)
    {
		callable();
    }

    template<typename... Args>
    inline constexpr void bulk_execute(Args&&... callables)
    {
        (..., std::forward<Args>(callables)());
    }

	template<typename F>
	inline constexpr void defer_execute(F&& callable)
	{
		// Be careful, deferring tasks for inline executor is a recursion!
		callable();
	}	

    template<typename F>
    inline auto future_execute(F&& callable)
    {
		//#TODO: C++2x return std::make_ready_future(...);
		return make_ready_future(std::forward<F>(callable));
    }

    template<typename... Args>
    inline auto future_bulk_execute(Args&&... callables)
    {
		return std::make_tuple(future_execute(std::forward<Args>(callables))...);
    }

private:

	template<typename F>
	static constexpr auto make_ready_future(F&& callable)
	{		
		std::promise<std::invoke_result_t<F>> promise;

		if constexpr (std::is_void_v<std::invoke_result_t<F>>)
		{
			std::invoke(std::forward<F>(callable));
			promise.set_value();
		}
		else
		{
			promise.set_value(std::invoke(std::forward<F>(callable)));
		}

		return promise.get_future();
	}
};

} // namespace adl {