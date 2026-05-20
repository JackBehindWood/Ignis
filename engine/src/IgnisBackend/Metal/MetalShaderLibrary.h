#pragma once

#include "MetalResource.h"

#include <unordered_map>
#include <string>

namespace Ignis
{
    class MetalDevice;

    class MetalShaderLibrary
    {
    private:
        MetalDevice* m_device = nullptr;
        UnorderedMap<std::string, MTL::Function*> m_function_cache;

        MetalShaderLibrary()  = default;
        ~MetalShaderLibrary();
    public:
        void init(MetalDevice* device);
        void reset();

        // Loads a MTL::Function from in-memory .metallib bytecode.
        // Caller receives an owned +1 retain; MetalVertexShader/MetalPixelShader destructors release it.
        // Cache key: (bytecode pointer address + entry_point) — stable for the lifetime of the owning AssetShader.
        MTL::Function* load_hardware_function(const uint8_t* data, size_t size,
                                              const String& entry_point);

        MetalShaderLibrary(const MetalShaderLibrary&)            = delete;
        MetalShaderLibrary& operator=(const MetalShaderLibrary&) = delete;
        MetalShaderLibrary(MetalShaderLibrary&&)                 = delete;
        MetalShaderLibrary& operator=(MetalShaderLibrary&&)      = delete;

        static MetalShaderLibrary& get()
        {
            static MetalShaderLibrary instance;
            return instance;
        }
    };

} // namespace Ignis
