#include "edpch.h"
#include "Logging/ConsoleSink.h"

#include <memory>

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

void ConsoleSink::sink_it_(const spdlog::details::log_msg& msg)
{
    spdlog::memory_buf_t formatted;
    base_sink<Mutex>::formatter_->format(msg, formatted);

    LogEntry& entry = m_entries.emplace_back();
    entry.level     = msg.level;
    entry.message   = String(formatted.data(), formatted.size());

    if (!entry.message.empty() && entry.message.back() == '\n')
    {
        entry.message.pop_back();
    }

    if (m_entries.size() > k_capacity)
    {
        m_entries.pop_front();
    }
}

} // namespace Ignis
