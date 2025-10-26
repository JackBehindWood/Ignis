#pragma once

#include "Ignis/Rendering/GRI/GRIViewport.h"

namespace MTL { class Device; }
namespace CA { class MetalLayer; }

namespace Ignis
{
    class MetalViewport : public GRIViewport
    {
    private:
        uint32_t m_width, m_height;
        CA::MetalLayer* m_metal_layer;
    public:
        MetalViewport(MTL::Device* device, uint32_t width, uint32_t height);
        ~MetalViewport() override;

        void resize(uint32_t width, uint32_t height);
        virtual inline uint32_t get_width() const override { return m_width; };
        virtual inline uint32_t get_height() const override { return m_height; };

        virtual inline void* get_native_handle() const override {return static_cast<void*>(m_metal_layer); };
    };
}