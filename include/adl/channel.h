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

// You can specialize this class for your channel type
template<typename ChannelType>
struct channel_info
{
    constexpr auto static get_name() { return static_cast<int>(ChannelType::ID); }
};

template<>
struct channel_info<void>
{
    constexpr auto static get_name() { return "void"; }
};

template<typename ChannelType>
constexpr static auto get_channel_name() { return channel_info<ChannelType>::get_name(); }

} // namespace adl