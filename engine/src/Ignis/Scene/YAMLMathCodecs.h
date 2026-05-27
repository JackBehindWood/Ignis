#pragma once

#include "yaml-cpp/yaml.h"
#include "Ignis/Math/Math.h"

namespace YAML
{

// TODO: this file should probably be a .ccp file and is this the best spot to put it?

// TODO: we should find a way to make it template aware, so we can do any Vec2<T> type for example and not only Vec2f =
// Vec2<float>
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
