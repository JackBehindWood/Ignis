#pragma once

#include "Ignis/Foundation/Foundation.h"
#include "Ignis/Math/Math.h"
#include "Ignis/Core/UUID.h"
#include "Ignis/Asset/Asset.h"
#include "yaml-cpp/yaml.h"

// Forward-declare SceneCamera codec helpers defined in SceneCamera.cpp
namespace Ignis
{
class SceneCamera;
YAML::Node  serialize_scene_camera(const SceneCamera& cam);
SceneCamera deserialize_scene_camera(const YAML::Node& node);
} // namespace Ignis

namespace YAML
{

template <>
struct convert<Ignis::Math::Vec2f>
{
    static Node encode(const Ignis::Math::Vec2f& v)
    {
        Node node;
        node.push_back(v.x);
        node.push_back(v.y);
        node.SetStyle(EmitterStyle::Flow);
        return node;
    }
    static bool decode(const Node& node, Ignis::Math::Vec2f& v)
    {
        if (!node.IsSequence() || node.size() != 2)
        {
            return false;
        }
        v.x = node[0].as<float>();
        v.y = node[1].as<float>();
        return true;
    }
};

template <>
struct convert<Ignis::Math::Vec3f>
{
    static Node encode(const Ignis::Math::Vec3f& v)
    {
        Node node;
        node.push_back(v.x);
        node.push_back(v.y);
        node.push_back(v.z);
        node.SetStyle(EmitterStyle::Flow);
        return node;
    }
    static bool decode(const Node& node, Ignis::Math::Vec3f& v)
    {
        if (!node.IsSequence() || node.size() != 3)
        {
            return false;
        }
        v.x = node[0].as<float>();
        v.y = node[1].as<float>();
        v.z = node[2].as<float>();
        return true;
    }
};

template <>
struct convert<Ignis::Math::Vec4f>
{
    static Node encode(const Ignis::Math::Vec4f& v)
    {
        Node node;
        node.push_back(v.x);
        node.push_back(v.y);
        node.push_back(v.z);
        node.push_back(v.w);
        node.SetStyle(EmitterStyle::Flow);
        return node;
    }
    static bool decode(const Node& node, Ignis::Math::Vec4f& v)
    {
        if (!node.IsSequence() || node.size() != 4)
        {
            return false;
        }
        v.x = node[0].as<float>();
        v.y = node[1].as<float>();
        v.z = node[2].as<float>();
        v.w = node[3].as<float>();
        return true;
    }
};

template <>
struct convert<Ignis::Math::Quatf>
{
    static Node encode(const Ignis::Math::Quatf& q)
    {
        Node node;
        node.push_back(q.x);
        node.push_back(q.y);
        node.push_back(q.z);
        node.push_back(q.w);
        node.SetStyle(EmitterStyle::Flow);
        return node;
    }
    static bool decode(const Node& node, Ignis::Math::Quatf& q)
    {
        if (!node.IsSequence() || node.size() != 4)
        {
            return false;
        }
        q.x = node[0].as<float>();
        q.y = node[1].as<float>();
        q.z = node[2].as<float>();
        q.w = node[3].as<float>();
        return true;
    }
};

} // namespace YAML

namespace Ignis::Math
{

inline YAML::Emitter& operator<<(YAML::Emitter& out, const Vec2f& v)
{
    return out << YAML::convert<Vec2f>::encode(v);
}
inline YAML::Emitter& operator<<(YAML::Emitter& out, const Vec3f& v)
{
    return out << YAML::convert<Vec3f>::encode(v);
}
inline YAML::Emitter& operator<<(YAML::Emitter& out, const Vec4f& v)
{
    return out << YAML::convert<Vec4f>::encode(v);
}
inline YAML::Emitter& operator<<(YAML::Emitter& out, const Quatf& v)
{
    return out << YAML::convert<Quatf>::encode(v);
}

} // namespace Ignis::Math

namespace Ignis
{

template <typename T>
struct YamlCodec
{
    static YAML::Node Encode(const T& val);
    static T          Decode(const YAML::Node& node);
};

// Primitives
template <>
struct YamlCodec<float>
{
    static YAML::Node Encode(const float& val)
    {
        return YAML::Node(val);
    }
    static float Decode(const YAML::Node& node)
    {
        return node.as<float>();
    }
};

template <>
struct YamlCodec<double>
{
    static YAML::Node Encode(const double& val)
    {
        return YAML::Node(val);
    }
    static double Decode(const YAML::Node& node)
    {
        return node.as<double>();
    }
};

template <>
struct YamlCodec<bool>
{
    static YAML::Node Encode(const bool& val)
    {
        return YAML::Node(val);
    }
    static bool Decode(const YAML::Node& node)
    {
        return node.as<bool>();
    }
};

template <>
struct YamlCodec<int32_t>
{
    static YAML::Node Encode(const int32_t& val)
    {
        return YAML::Node(val);
    }
    static int32_t Decode(const YAML::Node& node)
    {
        return node.as<int32_t>();
    }
};

template <>
struct YamlCodec<uint32_t>
{
    static YAML::Node Encode(const uint32_t& val)
    {
        return YAML::Node(val);
    }
    static uint32_t Decode(const YAML::Node& node)
    {
        return node.as<uint32_t>();
    }
};

// String (= std::string)
template <>
struct YamlCodec<String>
{
    static YAML::Node Encode(const String& val)
    {
        return YAML::Node(val);
    }
    static String Decode(const YAML::Node& node)
    {
        return node.as<std::string>();
    }
};

// Math types
template <>
struct YamlCodec<Math::Vec2f>
{
    static YAML::Node Encode(const Math::Vec2f& val)
    {
        return YAML::convert<Math::Vec2f>::encode(val);
    }
    static Math::Vec2f Decode(const YAML::Node& node)
    {
        return node.as<Math::Vec2f>();
    }
};

template <>
struct YamlCodec<Math::Vec3f>
{
    static YAML::Node Encode(const Math::Vec3f& val)
    {
        return YAML::convert<Math::Vec3f>::encode(val);
    }
    static Math::Vec3f Decode(const YAML::Node& node)
    {
        return node.as<Math::Vec3f>();
    }
};

template <>
struct YamlCodec<Math::Vec4f>
{
    static YAML::Node Encode(const Math::Vec4f& val)
    {
        return YAML::convert<Math::Vec4f>::encode(val);
    }
    static Math::Vec4f Decode(const YAML::Node& node)
    {
        return node.as<Math::Vec4f>();
    }
};

template <>
struct YamlCodec<Math::Quatf>
{
    static YAML::Node Encode(const Math::Quatf& val)
    {
        return YAML::convert<Math::Quatf>::encode(val);
    }
    static Math::Quatf Decode(const YAML::Node& node)
    {
        return node.as<Math::Quatf>();
    }
};

// UUID / AssetID (AssetID = UUID via typedef, no separate specialization needed)
template <>
struct YamlCodec<UUID>
{
    static YAML::Node Encode(const UUID& val)
    {
        return YAML::Node(static_cast<uint64_t>(val));
    }
    static UUID Decode(const YAML::Node& node)
    {
        return UUID(node.as<uint64_t>());
    }
};

// SceneCamera — delegates to the free functions in SceneCamera.cpp
template <>
struct YamlCodec<SceneCamera>
{
    static YAML::Node Encode(const SceneCamera& val)
    {
        return serialize_scene_camera(val);
    }
    static SceneCamera Decode(const YAML::Node& node)
    {
        return deserialize_scene_camera(node);
    }
};

} // namespace Ignis
