#pragma once
#include <tuple>
#include <type_traits>

#define ADL_DEBUG true

namespace adl {

    struct placeholder
    {
    };

    inline constexpr placeholder placeholder_v{};

    template <typename T>
    using void_to_placeholder_t = std::conditional_t<std::is_void_v<T>, placeholder, T>;

    template <typename T>
    using is_placeholder_t = std::is_same<std::decay_t<T>, placeholder>;

    template <typename T>
    inline constexpr bool is_placeholder_v = is_placeholder_t<T>::value;

    // If the invocation would return void then placeholder is returned
    template <typename F, typename... Args>
    constexpr auto replace_void_result_with_placeholder(F&& f, Args&&... args)
    {
        if constexpr (std::is_void_v<std::invoke_result_t<F, Args...>>)
        {
			#if ADL_DEBUG
            std::forward<F>(f)(std::forward<Args>(args)...);
			#else
            std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
			#endif
			return placeholder{};
        }
        else
        {            
			#if ADL_DEBUG
			return std::forward<F>(f)(std::forward<Args>(args)...);
			#else
			return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
			#endif
        }
    }

	template <typename F, typename T>
	constexpr auto try_invoke_with_arg(F&& f, T&& arg)
	{
		if constexpr (std::is_invocable_v<F, T>)
		{
			#if ADL_DEBUG
			return std::forward<F>(f)(std::forward<T>(arg));
			#else
			return std::invoke(std::forward<F>(f), std::forward<T>(arg));
			#endif			
		}
		else
		{
			#if ADL_DEBUG
			return std::forward<F>(f)();
			#else
			return std::invoke(std::forward<F>(f));
			#endif
		}
	}

    template <typename F>
    constexpr auto invoke_ignoring_placeholder(F&& f)
    {
        return replace_void_result_with_placeholder(std::forward<F>(f));
    }

	template <typename F, typename T, typename... Args>
    constexpr auto invoke_ignoring_placeholder(F&& f, T&& first, Args&&... args)
    {
        if constexpr (is_placeholder_v<T>)
        {
			return replace_void_result_with_placeholder(std::forward<F>(f), std::forward<Args>(args)...);
        }
        else
        {
            return replace_void_result_with_placeholder(std::forward<F>(f), std::forward<T>(first), std::forward<Args>(args)...);
        }
    }

    // Invoke callable with elements of tuple ignoring placeholders during the expansion.
    template <typename F, typename T>
    constexpr decltype(auto) apply_ignoring_placeholder(F&& f, T&& tuple)
    {
        return std::apply([&f](auto&&... args) -> decltype(auto) 
        {
            return invoke_ignoring_placeholder(std::forward<F>(f), std::forward<decltype(args)>(args)...);
        },
            std::forward<T>(tuple));
    }

    // Evaluates to the result of invoke ignoring placeholder
    template <typename F, typename... Args>
    using result_of_ignoring_placeholder_t = decltype(invoke_ignoring_placeholder(std::declval<F>(), std::declval<Args>()...));

} // namespace adl