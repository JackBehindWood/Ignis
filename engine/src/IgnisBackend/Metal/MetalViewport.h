#pragma once

#include "Ignis/Rendering/GRI/GRIViewport.h"

struct GLFWwindow;

namespace CA { class MetalLayer; }

namespace Ignis
{
    class MetalDevice;
    class MetalViewport : public GRIViewport
    {
    private:
        MetalDevice& m_device;
        GLFWwindow* m_window;
        uint32_t m_width, m_height;
        CA::MetalLayer* m_metal_layer;

        void setup_callbacks();
    public:
        MetalViewport(MetalDevice* device, const GRIViewportDesc& desc);
        ~MetalViewport() override;

        void resize(uint32_t width, uint32_t height);
        virtual inline uint32_t get_width() const override { return m_width; };
        virtual inline uint32_t get_height() const override { return m_height; };

        virtual inline void* get_native_handle() const override { return static_cast<void*>(m_window); };

        inline MetalDevice& get_device() {return m_device;}
        inline GLFWwindow* get_window() {return m_window;}
        inline CA::MetalLayer* get_metal_layer() {return m_metal_layer; }
    };
}