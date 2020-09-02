// Copyright (c) 2020 Ivan Miasnikov | mailto:ivaneotg@gmail.com
// MIT License | https://opensource.org/licenses/MIT

#pragma once
#include "dispatcher.h"
#include "execution_context.h"

namespace adl {

	template<typename ChannelType, typename CallableType, typename ContinuationType>
	class TaskWrapper;

	template<typename ChannelType, typename CallableType>
	constexpr auto task(CallableType&& callable);

	template<typename ChannelType, typename CallableType, typename ContinuationType>
	constexpr auto continuationTask(CallableType&& callable, ContinuationType&& continuation);

	namespace details
	{
		template<typename T>
		struct task_wrapper_traits : std::false_type
		{};

		template<typename ChannelType, typename CallableType, typename ContinuationType>
		struct task_wrapper_traits<TaskWrapper<ChannelType, CallableType, ContinuationType>> : std::true_type
		{};

		template<typename T>
		inline constexpr bool is_task_wrapper_v = task_wrapper_traits<std::decay_t<T>>::value;

		template<typename T>
		constexpr auto unwrap(T&& callable)
		{
			if constexpr (is_task_wrapper_v<T>)
			{
				return std::forward<T>(callable).unwrap();
			}
			else
			{
				return std::forward<T>(callable);
			}
		}

	} // namespace detail

	// Task without continuation
	template<typename ChannelType, typename CallableType>
	class TaskWrapper<ChannelType, CallableType, void>
	{
	public:

		using channel_t = ChannelType;
		using callable_t = CallableType;
		using continuation_t = void;

		TaskWrapper() = delete;
		TaskWrapper(TaskWrapper&&) = default;
		TaskWrapper(const TaskWrapper&) = default;
		TaskWrapper& operator=(TaskWrapper&&) = default;
		TaskWrapper& operator=(const TaskWrapper&) = default;

		constexpr TaskWrapper(callable_t&& callable)
			: m_callable{ std::forward<callable_t>(callable) }
		{}

		// No channel, execute continuation directly. Result of the previous execution agent will be ignored and passed to the next then continuation.
		template<typename F>
		constexpr auto post(F&& postExecutionAgent)
		{
			// Create a new strand node where execution agents invoked in sequence
			return task<channel_t>([callable = std::move(m_callable), executionAgent = details::unwrap(std::forward<F>(postExecutionAgent))]()
			{
				// Variables here:
				// 'callable' - can be a first execution agent or a 'strand' node with execution agents
				// 'postFunction' - is always execution agent

				auto result = replace_void_result_with_placeholder(callable);

				// Since there is no channel specified, execute agent directly after callable
				executionAgent();

				return result;
			});
		}

		// Post continuation to channel's executor. Result of the previous execution agent will be ignored and passed to the next then continuation.
		template<typename ContinuationChannel, typename F>
		constexpr auto post(F&& postExecutionAgent)
		{
			// Create a new strand node where execution agent is posted to the channel executor
			return task<channel_t>([callable = std::move(m_callable), executionAgent = details::unwrap(std::forward<F>(postExecutionAgent))]()
			{
				// Variables here:
				// 'callable' - can be a first execution agent or a 'strand' node with execution agents
				// 'executionAgent' - is always execution agent

				auto result = replace_void_result_with_placeholder(callable);

				// Post agent to the specified channel so it will be invoked by channel executor
				adl::post<ContinuationChannel>(std::move(executionAgent));

				return result;
			});
		}

		// No channel, execute continuations directly. The order of calls correspond to the arguments indexes. 
		// Result of the previous execution agent will be ignored and passed to the next then continuation.
		template<typename... Args>
		constexpr auto post_bulk(Args&&... postExecutionAgents)
		{
			return task<channel_t>([callable = std::move(m_callable), executionAgents = std::make_tuple(details::unwrap(std::forward<Args>(postExecutionAgents))...)]()
			{
				// Variables here:
				// 'callable' - can be a first execution agent or a 'strand' node with execution agents
				// 'executionAgents' - are always execution agents

				auto result = replace_void_result_with_placeholder(callable);

				// Since there is no channel specified, execute agents directly after callable
				apply_ignoring_placeholder([](auto&&... callables)
					{
						(..., std::forward<decltype(callables)>(callables)());
					}, std::move(executionAgents));

				return result;
			});
		}

		// Post continuations to channel's executor after callable. The order of calls is depends on channel executor
		// Result of the previous execution agent will be ignored and passed to the next then continuation.
		template<typename ContinuationChannel, typename... Args>
		constexpr auto post_bulk(Args&&... postExecutionAgents)
		{
			return task<channel_t>([callable = std::move(m_callable), executionAgents = std::make_tuple(details::unwrap(std::forward<Args>(postExecutionAgents))...)]()
			{
				// Variables here:
				// 'callable' - can be a first execution agent or a 'strand' node with execution agents
				// 'executionAgents' - are always execution agents

				auto result = replace_void_result_with_placeholder(callable);

				// Post execution agents to the specified channel so they will be invoked by channel executor
				apply_ignoring_placeholder([](auto&&... callables)
					{
						adl::post_bulk<ContinuationChannel>(std::forward<decltype(callables)>(callables)...);
					}, std::move(executionAgents));

				return result;
			});
		}

		// No channel, execute continuation directly 
		template<typename F>
		constexpr auto then(F&& thenExecutionAgent)
		{
			// Create a new strand node where execution agents invoked in sequence
			return task<channel_t>([callable = std::move(m_callable), executionAgent = details::unwrap(std::forward<F>(thenExecutionAgent))]()
			{
				// Variables here:
				// 'callable' - can be a first execution agent or a 'strand' node with execution agents
				// 'executionAgent' - is always execution agent

				auto result = replace_void_result_with_placeholder(callable);

				// Since its a no channel then, execute continuation directly after callable
				return invoke_ignoring_placeholder(executionAgent, std::move(result));
			});
		}

		// Execute continuation in specified channel
		template<typename ContinuationChannel, typename F>
		constexpr auto then(F&& thenExecutionAgent)
		{
			// Create a new node with continuation
			return continuationTask<ContinuationChannel>([callable = std::move(m_callable)](auto&& inputContinuation)
			{
				// Variables here:
				// 'callable' - can be a first execution agent or a 'strand' node with execution agents
				// 'inputContinuation' - can be a next continuation node or last execution agent passed to then

				using ExCallableRef = decltype(callable);
				using ExContinuationRef = decltype(inputContinuation);
				using ExContinuationChannel = ContinuationChannel;
				using ExDeferChannel = channel_t;

				struct ExecutionWrapper
				{
					std::remove_reference_t<ExCallableRef> callable;
					std::remove_reference_t<ExContinuationRef> continuation;

					inline constexpr void operator()()
					{
						auto result = details::try_invoke_with_context(callable);

						// if result with context this code will check if continuation was canceled.
						// if result without context this code will be thrown away by compiler since in this case is_execution_canceled always return false
						if (details::is_execution_canceled(result))
						{
							return;
						}

						// if result with context this code will check if continuation was deferred.
						// if result without context this code will be thrown away by compiler since in this case is_execution_deferred always return false
						if (details::is_execution_deferred(result))
						{
							// Defer current task
							adl::post_defer<ExDeferChannel>(ExecutionWrapper{ std::move(callable), std::move(continuation) });

							return;
						}

						// Post continuation to the specified channel so it will be invoked by channel executor
						adl::post<ExContinuationChannel>([result = details::unwrap_execution_result(std::move(result)), continuation = std::move(continuation)]()
						{
							// Captured variables here:
							// 'result' - result of the previous execution agent or a placeholder (if previous execution returned void)
							// 'continuation' - can be a node or last execution agent passed to the task

							// If continuation is a node, we should always pass the result to it, since it can't be ignored.
							// If this is an execution agent, we should pass the result only if it can be invoked with it.
							// For example last leaf can be a function with no arguments, and in this case result is discarded. 
							// #TODO: need a better way to find out if continuation is a node or execution agent
							try_invoke_with_arg(continuation, std::move(result));
						});
					}
				};

				adl::post<channel_t>(ExecutionWrapper{ std::move(callable), std::forward<ExContinuationRef>(inputContinuation) });

			},
				details::unwrap(std::forward<F>(thenExecutionAgent)));
		}

		constexpr void submit() &&
		{
			adl::post<channel_t>(std::move(m_callable));
		}

		constexpr void submit() &
		{
			adl::post<channel_t>(m_callable);
		}

		constexpr auto unwrap() &
		{
			// If you got this assert, make sure that all execution agents in nested task is copy constructible, or try to use std::move when passing nested task
			static_assert(std::is_copy_constructible_v<decltype(m_callable)>);

			if constexpr (std::is_void_v<channel_t>)
			{
				return m_callable;
			}
			else
			{
				return [callable = m_callable]()
				{
					adl::post<channel_t>(std::move(callable));
				};
			}
		}

		constexpr auto unwrap() &&
		{
			if constexpr (std::is_void_v<channel_t>)
			{
				return std::move(m_callable);
			}
			else
			{
				return[callable = std::move(m_callable)]()
				{
					adl::post<channel_t>(std::move(callable));
				};
			}
		}

	private:

		callable_t m_callable;
	};

	// Task with continuation
	template<typename ChannelType, typename CallableType, typename ContinuationType>
	class TaskWrapper
	{
	public:

		using channel_t = ChannelType;
		using callable_t = CallableType;
		using continuation_t = ContinuationType;

		TaskWrapper() = delete;
		TaskWrapper(const TaskWrapper&) = delete;
		TaskWrapper& operator=(const TaskWrapper&) = delete;
		TaskWrapper& operator=(TaskWrapper&&) = default;
		TaskWrapper(TaskWrapper&&) = default;

		constexpr TaskWrapper(callable_t&& callable, continuation_t&& continuation)
			: m_callable{ std::forward<callable_t>(callable) }
			, m_continuation{ std::forward<continuation_t>(continuation) }
		{}

		// No channel, execute continuation directly. Result of the previous execution agent will be ignored and passed to the next then continuation.
		template<typename F>
		constexpr auto post(F&& postExecutionAgent)
		{
			return post<void>(std::forward<F>(postExecutionAgent));
		}

		// Post continuation to channel's executor. Result of the previous execution agent will be ignored and passed to the next then continuation.
		template<typename ContinuationChannel, typename F>
		constexpr auto post(F&& postExecutionAgent)
		{
			// Create a new continuation node where execution agent is posted to the channel executor
			return then([executionAgent = details::unwrap(std::forward<F>(postExecutionAgent))](auto&& prevResult)
			{
				adl::post<ContinuationChannel>(std::move(executionAgent));
				return prevResult;
			});
		}

		// No channel, execute continuations directly. The order of calls correspond to the arguments indexes
		// Result of the previous execution agent will be ignored and passed to the next then continuation.
		template<typename... Args>
		constexpr auto post_bulk(Args&&... postExecutionAgents)
		{
			return post_bulk<void>(std::forward<Args>(postExecutionAgents)...);
		}

		// Post continuations to channel's executor after callable. The order of calls is depends on channel executor.
		// Result of the previous execution agent will be ignored and passed to the next then continuation.
		template<typename ContinuationChannel, typename... Args>
		constexpr auto post_bulk(Args&&... postExecutionAgents)
		{
			return then([executionAgents = std::make_tuple(details::unwrap(std::forward<Args>(postExecutionAgents))...)](auto&& prevResult)
			{
				// Post execution agents to the specified channel so they will be invoked by channel executor
				apply_ignoring_placeholder([](auto&&... continuations)
					{
						adl::post_bulk<ContinuationChannel>(std::forward<decltype(continuations)>(continuations)...);
					}, std::move(executionAgents));

				return prevResult;
			});
		}

		// No channel, execute directly and pass result to continuation
		template<typename F>
		constexpr auto then(F&& thenExecutionAgent)
		{
			return then<void>(std::forward<F>(thenExecutionAgent));
		}

		// Execute continuation agent in specified channel with result of the previous execution agent
		template<typename ContinuationChannel, typename F>
		constexpr auto then(F&& thenExecutionAgent)
		{
			// Create a new node with continuation
			return continuationTask<ContinuationChannel>([callable = std::move(m_callable), continuation = std::move(m_continuation)](auto&& inputContinuation)
			{
				// Variables here:
				// 'callable' - is always a node that accept input continuation
				// 'continuation' - is always execution agent
				// 'inputContinuation' - can be a next continuation node or last execution agent passed to then

				// Invoke a node and pass input continuation to it
				return callable([callable = std::move(continuation), continuation = std::forward<decltype(inputContinuation)>(inputContinuation)](auto&& prevResult)
				{
					// Variables here:
					// 'callable' - is always execution agent
					// 'continuation' - can be a node or last execution agent passed to the task
					// 'prevResult' - result of the previous execution agent or a placeholder (if previous execution returned void)

					using ExCallableRef = decltype(callable);
					using ExContinuationRef = decltype(inputContinuation);
					using ExContinuation = std::remove_reference_t<ExContinuationRef>;
					using ExResultRef = decltype(prevResult);
					using ExContinuationChannel = ContinuationChannel;
					using ExDeferChannel = channel_t;

					struct ExecutionWrapper
					{
						std::remove_reference_t<ExCallableRef>		callable;
						std::remove_reference_t<ExContinuationRef>	continuation;
						std::remove_reference_t<ExResultRef>		prevResult;

						inline constexpr void operator()()
						{
							invoke(std::move(callable), std::move(continuation), std::move(prevResult));
						}

						static inline constexpr void invoke(ExCallableRef callable, ExContinuation continuation, ExResultRef prevResult)
						{
							// Result shouldn't be moved if callable is invokable with context
							// It is possible that execution will be deferred and prev result will be reused in call of the deferred task
							// This imposes restrictions to the callable signature, - smth like 'void foo(ExecutionContext&, T&&)' is prohibited, second argument can't be rvalue
							auto result = details::try_invoke_with_context(callable, details::move_if_invokable_without_context<ExCallableRef>(prevResult));

							// if result with context this code will check if continuation was canceled.
							// if result without context this code will be thrown away by compiler since in this case is_execution_canceled always return false
							if (details::is_execution_canceled(result))
							{
								return;
							}

							// if result with context this code will check if continuation was deferred and post a new deferred call.
							// if result without context this code will be thrown away by compiler since in this case is_execution_deferred always return false
							if (details::is_execution_deferred(result))
							{
								// Defer current task
								adl::post_defer<ExDeferChannel>(ExecutionWrapper{ std::move(callable), std::move(continuation), std::move(prevResult) });

								return;
							}

							// Post continuation to the specified channel so it will be invoked by channel executor
							adl::post<ExContinuationChannel>([result = details::unwrap_execution_result(std::move(result)), continuation = std::move(continuation)]()
							{
								// Captured variables here:
								// 'result' - result of the previous execution agent or a placeholder (if previous execution returned void)
								// 'continuation' - can be a node or last execution agent passed to the task

								// If continuation is a node, we should always pass the result to it. See argument (auto&& prevResult) in lambda above, it can't be ignored.
								// So if the result is placeholder, it will be ignored during continuation invoke.
								// If this is an execution agent, we should pass the result only if it can be invoked with it.
								// For example last leaf can be a function with no arguments, and in this case result is discarded. 
								// #TODO: need a better way to find out if continuation is a node or execution agent
								try_invoke_with_arg(continuation, std::move(result));
							});
						}
					};

					ExecutionWrapper::invoke(callable, std::move(continuation), std::forward<ExResultRef>(prevResult));
				});
			},
				details::unwrap(std::forward<F>(thenExecutionAgent)));
		}

		constexpr void submit() &&
		{
			std::invoke(m_callable, std::move(m_continuation));
		}

		constexpr void submit() &
		{
			std::invoke(m_callable, m_continuation);
		}

		constexpr auto unwrap() &
		{
			// If you got this assert, make sure that all execution agents in nested task is copy constructible, or try to use std::move when passing nested task
			static_assert(std::is_copy_constructible_v<decltype(m_callable)>);
			static_assert(std::is_copy_constructible_v<decltype(m_continuation)>);

			return [callable = m_callable, continuation = m_continuation]()
			{
				std::invoke(callable, std::move(continuation));
			};
		}

		constexpr auto unwrap() &&
		{
			return[callable = std::move(m_callable), continuation = std::move(m_continuation)]()
			{
				std::invoke(callable, std::move(continuation));
			};
		}

	private:

		callable_t		m_callable;
		continuation_t	m_continuation;
	};

	template<typename CallableType>
	constexpr auto task(CallableType&& callable)
	{
		using UnwrappedCallableType = decltype(details::unwrap(std::forward<CallableType>(callable)));
		return TaskWrapper<void, UnwrappedCallableType, void>{details::unwrap(std::forward<CallableType>(callable))};
	}

	template<typename ChannelType, typename CallableType>
	constexpr auto task(CallableType&& callable)
	{
		using UnwrappedCallableType = decltype(details::unwrap(std::forward<CallableType>(callable)));
		return TaskWrapper<ChannelType, UnwrappedCallableType, void>{details::unwrap(std::forward<CallableType>(callable))};
	}

	template<typename ChannelType, typename CallableType, typename ContinuationType>
	constexpr auto continuationTask(CallableType&& callable, ContinuationType&& continuation)
	{
		using UnwrappedCallableType = decltype(details::unwrap(std::forward<CallableType>(callable)));
		using UnwrappedContinuationType = decltype(details::unwrap(std::forward<ContinuationType>(continuation)));
		return TaskWrapper<ChannelType, UnwrappedCallableType, UnwrappedContinuationType>
		{
			details::unwrap(std::forward<CallableType>(callable)),
				details::unwrap(std::forward<ContinuationType>(continuation))
		};
	}

} // namespace adl
