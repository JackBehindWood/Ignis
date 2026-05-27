#include "igpch.h"
#include "MetalShaderLibrary.h"
#include "MetalDevice.h"

#include <Foundation/Foundation.hpp>
#include <Foundation/NSError.hpp>
#include <Metal/Metal.hpp>
#include <dispatch/dispatch.h>

namespace Ignis
{

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
    const String fn_key = to_string(reinterpret_cast<uintptr_t>(data)) + ':' + entry_point;

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

MetalShaderLibrary::~MetalShaderLibrary()
{
    for (auto& [key, fn] : m_function_cache)
    {
        fn->release();
    }
}

} // namespace Ignis
