#include "edpch.h"
#include "Logging/ConsoleSink.h"

#include <memory>
#include "ConsoleSink.h"

namespace Ignis
{

static std::shared_ptr<ConsoleSink> s_sink = std::make_shared<ConsoleSink>();

namespace
{
struct AutoRegister
{
    AutoRegister()
    {
        s_sink->set_level(spdlog::level::trace);
        Log::add_pending_sink(s_sink);
    }
} s_auto_register;
} // namespace

ConsoleSink& ConsoleSink::get()
{
    return *s_sink;
}

void ConsoleSink::push_user_input(const String& message)
{
    LockGuard<Mutex> lock(mutex_);

    auto    tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm_buf{};
    localtime_r(&tt, &tm_buf);
    char ts[9];
    std::strftime(ts, sizeof(ts), "%H:%M:%S", &tm_buf);

    LogEntry& entry = m_entries.emplace_back();
    entry.level     = static_cast<spdlog::level::level_enum>(10); // Use 10 as a unique "User" flag
    entry.timestamp = ts;
    entry.message   = message;

    if (m_entries.size() > k_capacity)
    {
        m_entries.pop_front();
    }
}

void ConsoleSink::sink_it_(const spdlog::details::log_msg& msg)
{
    auto    tt = std::chrono::system_clock::to_time_t(msg.time);
    std::tm tm_buf{};
    localtime_r(&tt, &tm_buf);
    char ts[9];
    std::strftime(ts, sizeof(ts), "%H:%M:%S", &tm_buf);

    LogEntry& entry = m_entries.emplace_back();
    entry.level     = msg.level;
    entry.timestamp = ts;
    entry.message   = String(msg.payload.data(), msg.payload.size());

    if (m_entries.size() > k_capacity)
    {
        m_entries.pop_front();
    }
}

} // namespace Ignis
