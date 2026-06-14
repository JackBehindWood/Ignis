#include "igpch.h"
#include "MetalShaderLibrary.h"
#include "MetalDevice.h"

#include <Foundation/Foundation.hpp>
#include <Foundation/NSError.hpp>
#include <Metal/Metal.hpp>
#include <dispatch/dispatch.h>

namespace Ignis
{

static uint64_t fnv1a_bytes(const uint8_t* data, size_t size)
{
    constexpr uint64_t k_basis = 14695981039346656037ULL;
    constexpr uint64_t k_prime = 1099511628211ULL;
    uint64_t           h       = k_basis;
    for (size_t i = 0; i < size; ++i)
    {
        h ^= static_cast<uint64_t>(data[i]);
        h *= k_prime;
    }
    return h ? h : 1;
}

void MetalShaderLibrary::init(MetalDevice* device)
{
    m_device = device;
}

void MetalShaderLibrary::reset()
{
    for (auto& [key, fn] : m_function_cache)
    {
        fn->release();
    }
    m_function_cache.clear();
}

MTL::Function* MetalShaderLibrary::load_hardware_function(const uint8_t* data, size_t size, const String& entry_point)
{
    const uint64_t hash   = fnv1a_bytes(data, size);
    const String   fn_key = to_string(hash) + ':' + entry_point;

    auto fn_it = m_function_cache.find(fn_key);
    if (fn_it != m_function_cache.end())
    {
        fn_it->second->retain();
        return fn_it->second;
    }

    MTL_AUTORELEASE_POOL;

    dispatch_data_t ddata =
        dispatch_data_create(data, size, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    NS::Error*    error   = nullptr;
    MTL::Library* library = m_device->get_device()->newLibrary(ddata, &error);
    dispatch_release(ddata);

    if (!library)
    {
        IG_CORE_ERROR("MetalShaderLibrary: failed to create library from memory (entry '{}'): {}", entry_point,
                      error->localizedDescription()->utf8String());
        return nullptr;
    }

    NS::String*    ns_entry = NS::String::string(entry_point.c_str(), NS::StringEncoding::UTF8StringEncoding);
    MTL::Function* function = library->newFunction(ns_entry);
    library->release();

    if (!function)
    {
        IG_CORE_ERROR("MetalShaderLibrary: entry point '{}' not found in library", entry_point);
        return nullptr;
    }

    m_function_cache.emplace(fn_key, function);
    function->retain();
    return function;
}

void MetalShaderLibrary::invalidate(uint64_t bytecode_hash)
{
    const String prefix = to_string(bytecode_hash) + ':';
    for (auto it = m_function_cache.begin(); it != m_function_cache.end();)
    {
        if (it->first.compare(0, prefix.size(), prefix) == 0)
        {
            it->second->release();
            it = m_function_cache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

MetalShaderLibrary::~MetalShaderLibrary()
{
    for (auto& [key, fn] : m_function_cache)
    {
        fn->release();
    }
}

} // namespace Ignis
