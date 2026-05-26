#include "igpch.h"
#include "YamlStream.h"

#include <fstream>

namespace Ignis
{

// ---------------------------------------------------------------------------
// YamlWriter
// ---------------------------------------------------------------------------

YamlWriter::YamlWriter()
{
    m_emitter << YAML::BeginMap;
}

YamlWriter& YamlWriter::set(const String& key, const String& value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
    return *this;
}

YamlWriter& YamlWriter::set(const String& key, int32_t value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
    return *this;
}

YamlWriter& YamlWriter::set(const String& key, uint32_t value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
    return *this;
}

YamlWriter& YamlWriter::set(const String& key, float value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
    return *this;
}

YamlWriter& YamlWriter::set(const String& key, bool value)
{
    m_emitter << YAML::Key << key << YAML::Value << value;
    return *this;
}

YamlWriter& YamlWriter::begin_map(const String& key)
{
    m_emitter << YAML::Key << key << YAML::Value << YAML::BeginMap;
    ++m_map_depth;
    return *this;
}

YamlWriter& YamlWriter::end_map()
{
    IG_CORE_ASSERT(m_map_depth > 0, "YamlWriter: end_map without matching begin_map");
    m_emitter << YAML::EndMap;
    --m_map_depth;
    return *this;
}

YamlWriter& YamlWriter::begin_sequence(const String& key)
{
    m_emitter << YAML::Key << key << YAML::Value << YAML::BeginSeq;
    m_in_sequence = true;
    return *this;
}

YamlWriter& YamlWriter::push_sequence_item(const String& value)
{
    m_emitter << value;
    return *this;
}

YamlWriter& YamlWriter::end_sequence()
{
    m_emitter << YAML::EndSeq;
    m_in_sequence = false;
    return *this;
}

bool YamlWriter::write(const Path& path)
{
    IG_CORE_ASSERT(m_map_depth == 0, "YamlWriter: unclosed begin_map");
    m_emitter << YAML::EndMap;

    const Path tmp = path.parent_path() / (path.filename().string() + ".tmp");
    {
        std::ofstream file(tmp);
        if (!file.is_open())
        {
            return false;
        }
        file << m_emitter.c_str();
        if (!file.good())
        {
            Filesystem::remove(tmp);
            return false;
        }
    }

    std::error_code ec;
    Filesystem::rename(tmp, path, ec);
    if (ec)
    {
        Filesystem::remove(tmp);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// YamlReader
// ---------------------------------------------------------------------------

YamlReader::YamlReader(const Path& path)
{
    try
    {
        m_root = YAML::LoadFile(path.string());
        m_open = m_root.IsDefined();
    }
    catch (...)
    {
    }
}

Vector<String> YamlReader::get_sequence(const String& key) const
{
    Vector<String> result;
    if (!m_open)
    {
        return result;
    }

    const YAML::Node node = m_root[key];
    if (!node.IsDefined() || !node.IsSequence())
    {
        return result;
    }

    for (const auto& item : node)
    {
        try
        {
            result.push_back(item.as<String>());
        }
        catch (...)
        {
        }
    }
    return result;
}

} // namespace Ignis
