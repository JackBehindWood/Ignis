#pragma once

#include <Ignis/Foundation/Foundation.h>
#include <Ignis/Core/Log.h>
#include <spdlog/sinks/base_sink.h>
#include <memory>

namespace Ignis
{

struct LogEntry
{
    spdlog::level::level_enum level;
    String                    message;
};

// TODO: We should make sure our logger uses the console log, when we have it!

class ConsoleSink : public spdlog::sinks::base_sink<Mutex>
{
public:
    static constexpr uint32_t k_capacity = 512;

    const Deque<LogEntry>& entries() const
    {
        return m_entries;
    }
    void clear()
    {
        m_entries.clear();
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override
    {
    }

private:
    Deque<LogEntry> m_entries;
};

class ConsolePanel
{
public:
    ConsolePanel();

    void draw();

    std::shared_ptr<ConsoleSink> get_sink() const
    {
        return m_sink;
    }

private:
    std::shared_ptr<ConsoleSink> m_sink;
    bool                         m_filter[6]        = {true, true, true, true, true, true};
    bool                         m_auto_scroll      = true;
    bool                         m_scroll_to_bottom = false;
};

} // namespace Ignis
