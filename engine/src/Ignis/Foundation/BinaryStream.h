#pragma once

#include "Filesystem.h"
#include "String.h"
#include <fstream>
#include <cstring>

namespace Ignis
{

class BinaryWriter
{
public:
    BinaryWriter() = default; // not-open state; check is_open() before use
    explicit BinaryWriter(const Path& path)
        : m_stream(path, std::ios::binary | std::ios::trunc)
    {}

    BinaryWriter(BinaryWriter&&)            = default;
    BinaryWriter& operator=(BinaryWriter&&) = default;

    bool is_open() const { return m_stream.is_open(); }
    bool good()    const { return m_stream.good(); }

    void write_u8 (uint8_t  v) { m_stream.write(reinterpret_cast<const char*>(&v), 1); }
    void write_u16(uint16_t v) { m_stream.write(reinterpret_cast<const char*>(&v), 2); }
    void write_u32(uint32_t v) { m_stream.write(reinterpret_cast<const char*>(&v), 4); }
    void write_u64(uint64_t v) { m_stream.write(reinterpret_cast<const char*>(&v), 8); }
    void write_bytes(const void* data, size_t size) { m_stream.write(static_cast<const char*>(data), size); }

    using StreamPos = std::streampos;

    StreamPos tell()                { return m_stream.tellp();  }
    void      seekp(StreamPos p)    { m_stream.seekp(p);        }

    template <typename T>
    BinaryWriter& operator<<(const T& value)
    {
        static_assert(IsTriviallyCopyable<T>, "Type must be trivially copyable for binary serialization.");
        write_bytes(&value, sizeof(T));
        return *this;
    }

private:
    std::ofstream m_stream;
};

class BinaryReader
{
public:
    BinaryReader() = default; // not-open state; check is_open() before use
    explicit BinaryReader(const Path& path)
        : m_stream(path, std::ios::binary)
    {}

    void open(const Path& path) { m_stream.open(path, std::ios::binary); }
    void close()                { m_stream.close(); }

    void   seekg(uint64_t offset) { m_stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg); }
    size_t read_chunk(void* dst, size_t n)
    {
        m_stream.read(static_cast<char*>(dst), static_cast<std::streamsize>(n));
        return static_cast<size_t>(m_stream.gcount());
    }

    BinaryReader(BinaryReader&&)            = default;
    BinaryReader& operator=(BinaryReader&&) = default;

    bool is_open() const { return m_stream.is_open(); }
    bool good()    const { return m_stream.good(); }

    uint8_t  read_u8()  { uint8_t  v; m_stream.read(reinterpret_cast<char*>(&v), 1); return v; }
    uint16_t read_u16() { uint16_t v; m_stream.read(reinterpret_cast<char*>(&v), 2); return v; }
    uint32_t read_u32() { uint32_t v; m_stream.read(reinterpret_cast<char*>(&v), 4); return v; }
    uint64_t read_u64() { uint64_t v; m_stream.read(reinterpret_cast<char*>(&v), 8); return v; }
    void read_bytes(void* data, size_t size) { m_stream.read(static_cast<char*>(data), size); }

    template <typename T>
    BinaryReader& operator>>(T& value)
    {
        static_assert(IsTriviallyCopyable<T>, "Type must be trivially copyable for binary deserialization.");
        read_bytes(&value, sizeof(T));
        return *this;
    }

    size_t get_size()
    {
        if (!m_stream.is_open()) 
        {
            return 0;
        }

        std::streampos current = m_stream.tellg(); // Save current pos
        m_stream.seekg(0, std::ios::end);
        std::streampos size = m_stream.tellg();
        m_stream.seekg(current, std::ios::beg);    // Restore original pos
        
        return static_cast<size_t>(size);
    }

    size_t get_remaining_bytes()
    {
        if (!m_stream.is_open()) 
        {
            return 0;
        }

        std::streampos current = m_stream.tellg();
        m_stream.seekg(0, std::ios::end);
        std::streampos size = m_stream.tellg();
        m_stream.seekg(current, std::ios::beg);
        
        return static_cast<size_t>(size - current);
    }

    String read_all_text()
    {
        return String((std::istreambuf_iterator<char>(m_stream)), std::istreambuf_iterator<char>());
    }

private:
    std::ifstream m_stream;
};

// ---------------------------------------------------------------------------
// MemBinaryReader — in-memory byte-buffer reader, same API as BinaryReader
// ---------------------------------------------------------------------------
class MemBinaryReader
{
public:
    MemBinaryReader() = default;
    MemBinaryReader(const uint8_t* data, size_t size)
        : m_data(data), m_size(size) {}

    uint8_t  read_u8()  { return read_pod<uint8_t>();  }
    uint16_t read_u16() { return read_pod<uint16_t>(); }
    uint32_t read_u32() { return read_pod<uint32_t>(); }
    uint64_t read_u64() { return read_pod<uint64_t>(); }

    void read_bytes(void* dst, size_t n)
    {
        if (m_pos + n > m_size) { m_good = false; return; }
        std::memcpy(dst, m_data + m_pos, n);
        m_pos += n;
    }

    bool good() const { return m_good; }

private:
    template<typename T>
    T read_pod()
    {
        T v{};
        if (m_pos + sizeof(T) > m_size) { m_good = false; return v; }
        std::memcpy(&v, m_data + m_pos, sizeof(T));
        m_pos += sizeof(T);
        return v;
    }

    const uint8_t* m_data = nullptr;
    size_t         m_size = 0;
    size_t         m_pos  = 0;
    bool           m_good = true;
};

} // namespace Ignis
