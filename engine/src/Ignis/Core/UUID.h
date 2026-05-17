#pragma once

namespace Ignis {

class UUID
{
private:
    static uint64_t generate();

    uint64_t m_uuid;
public:
    UUID() : m_uuid(generate()) {}
    explicit UUID(uint64_t uuid) : m_uuid(uuid) {}

    operator uint64_t() const { return m_uuid; }

    bool operator==(const UUID& other) const { return m_uuid == other.m_uuid; }
    bool operator!=(const UUID& other) const { return m_uuid != other.m_uuid; }

    static constexpr uint64_t s_invalid = 0;
};

} // namespace Ignis

namespace std {

template<>
struct hash<Ignis::UUID>
{
    size_t operator()(const Ignis::UUID& uuid) const
    {
        return hash<uint64_t>()(static_cast<uint64_t>(uuid));
    }
};

} // namespace std