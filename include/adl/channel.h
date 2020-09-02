// Copyright (c) 2020 Ivan Miasnikov | mailto:ivaneotg@gmail.com
// MIT License | https://opensource.org/licenses/MIT

#pragma once

namespace adl
{

template<typename ChannelType>
struct channel_info;

template<typename ChannelIDType, ChannelIDType ID, typename ChannelExecutorType>
struct Channel;

template<typename ChannelIDType, ChannelIDType ChannelID, typename ChannelExecutorType>
struct Channel
{
    using id_t = ChannelIDType;
    using executor_t = ChannelExecutorType;
    static constexpr ChannelIDType ID = ChannelID;
};

} // namespace adl