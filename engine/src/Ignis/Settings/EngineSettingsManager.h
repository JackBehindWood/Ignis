#pragma once

#include "EngineSettings.h"
#include "Ignis/Foundation/Vector.h"

#include <algorithm>

namespace Ignis
{

class ISettingsObserver
{
public:
    virtual void on_settings_changed(const EngineSettings& s) = 0;

protected:
    ~ISettingsObserver() = default;
};

class EngineSettingsManager
{
public:
    static EngineSettingsManager& get()
    {
        static EngineSettingsManager instance;
        return instance;
    }

    const EngineSettings& settings() const
    {
        return m_settings;
    }

    void apply(const EngineSettings& s)
    {
        m_settings = s;
        for (auto* obs : m_observers)
        {
            obs->on_settings_changed(m_settings);
        }
    }

    void add_observer(ISettingsObserver* obs)
    {
        m_observers.push_back(obs);
    }

    void remove_observer(ISettingsObserver* obs)
    {
        m_observers.erase(std::remove(m_observers.begin(), m_observers.end(), obs), m_observers.end());
    }

private:
    EngineSettings             m_settings;
    Vector<ISettingsObserver*> m_observers;
};

} // namespace Ignis
