// Copyright (c) 2020 Ivan Miasnikov | mailto:ivaneotg@gmail.com
// MIT License | https://opensource.org/licenses/MIT

#pragma once
#include "channel.h"
#include "executors/inline_executor.h"

namespace adl
{

// Return executor instance for provided channel
template<typename ChannelType>
static auto& get_executor()
{
	static ChannelType::executor_t executor;
	return executor;
}

// Return inline executor if channel is not provided
template<>
static auto& get_executor<void>()
{
	static InlineExecutor executor;
	return executor;
}

// Submit execution agent for one-way execution in provided channel
template<typename ChannelType, typename CallableType>
static void post(CallableType&& callable)
{ 
    get_executor<ChannelType>().execute(std::forward<CallableType>(callable));
}

// Submit execution agent for one-way deferred execution in provided channel
template<typename ChannelType, typename CallableType>
static void post_defer(CallableType&& callable)
{
	get_executor<ChannelType>().defer_execute(std::forward<CallableType>(callable));
}

// Submit a group of execution agents for one-way execution in provided channel
template<typename ChannelType, typename... CallableTypes>
static void post_bulk(CallableTypes&&... callables)
{
    get_executor<ChannelType>().bulk_execute(std::forward<CallableTypes>(callables)...);
}

// Submit execution agent for two-way execution in provided channel
template<typename ChannelType, typename CallableType>
static auto post_future(CallableType&& callable)
{
    return get_executor<ChannelType>().future_execute(std::forward<CallableType>(callable));
}

// Submit a group of execution agents for two-way execution in provided channel
template<typename ChannelType, typename... CallableTypes>
static auto post_future_bulk(CallableTypes&&... callables)
{ 
    return get_executor<ChannelType>().future_bulk_execute(std::forward<CallableTypes>(callables)...);
}

// Dispatch execution agents for provided channel
template<typename ChannelType>
static void dispatch()
{
    get_executor<ChannelType>().dispatch();
}

} // namespace adl