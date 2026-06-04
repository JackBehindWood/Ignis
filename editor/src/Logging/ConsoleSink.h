#pragma once

#include <Ignis/Foundation/Foundation.h>
#include <Ignis/Core/Log.h>

#pragma warning(push, 0)
#include <spdlog/sinks/base_sink.h>
#pragma warning(pop)

namespace Ignis
{

struct LogEntry
{
    spdlog::level::level_enum level;
    String                    timestamp;
    String                    message;
};

class ConsoleSink : public spdlog::sinks::base_sink<Mutex>
{
public:
    static constexpr uint32_t k_capacity = 512;

    static ConsoleSink& get();

    const Deque<LogEntry>& entries() const
    {
        return m_entries;
    }
    void clear()
    {
        m_entries.clear();
    }

    void push_user_input(const String& message);

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override
    {
    }

private:
    Deque<LogEntry> m_entries;
};

} // namespace Ignis
