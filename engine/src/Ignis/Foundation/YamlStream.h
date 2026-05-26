#pragma once

#include "Filesystem.h"
#include "String.h"
#include "Vector.h"
#include "Optional.h"

#include <yaml-cpp/yaml.h>

namespace Ignis
{

// ---------------------------------------------------------------------------
// YamlWriter — builds a YAML document and flushes it to a file.
//
// Usage:
//   YamlWriter w;
//   w.set("key", value);
//   w.begin_sequence("items");
//   w.push_sequence_item(item);
//   w.end_sequence();
//   bool ok = w.write(path);
// ---------------------------------------------------------------------------
class YamlWriter
{
public:
    YamlWriter();

    YamlWriter& set(const String& key, const String& value);
    YamlWriter& set(const String& key, int32_t value);
    YamlWriter& set(const String& key, uint32_t value);
    YamlWriter& set(const String& key, float value);
    YamlWriter& set(const String& key, bool value);

    YamlWriter& begin_map(const String& key);
    YamlWriter& end_map();

    YamlWriter& begin_sequence(const String& key);
    YamlWriter& push_sequence_item(const String& value);
    YamlWriter& end_sequence();

    bool write(const Path& path);

private:
    YAML::Emitter m_emitter;
    bool          m_in_sequence = false;
    uint32_t      m_map_depth   = 0;
};

// ---------------------------------------------------------------------------
// YamlReader — wraps a parsed YAML::Node, provides typed get<T> with defaults.
//
// Usage:
//   YamlReader r(path);
//   if (!r.is_open()) { ... }
//   auto val   = r.get<uint32_t>("key", default_val);
//   auto items = r.get_sequence("list");
// ---------------------------------------------------------------------------
class YamlReader
{
public:
    explicit YamlReader(const Path& path);

    bool is_open() const
    {
        return m_open;
    }

    template <typename T>
    T get(const String& key, T default_value) const
    {
        if (!m_open)
        {
            return default_value;
        }
        const YAML::Node node = m_root[key];
        if (!node.IsDefined() || node.IsNull())
        {
            return default_value;
        }
        try
        {
            return node.as<T>();
        }
        catch (...)
        {
            return default_value;
        }
    }

    template <typename T>
    T get_nested(const String& map_key, const String& field_key, T default_value) const
    {
        if (!m_open)
        {
            return default_value;
        }
        const YAML::Node map = m_root[map_key];
        if (!map.IsDefined() || !map.IsMap())
        {
            return default_value;
        }
        const YAML::Node node = map[field_key];
        if (!node.IsDefined() || node.IsNull())
        {
            return default_value;
        }
        try
        {
            return node.as<T>();
        }
        catch (...)
        {
            return default_value;
        }
    }

    Vector<String> get_sequence(const String& key) const;

private:
    YAML::Node m_root;
    bool       m_open = false;
};

} // namespace Ignis
