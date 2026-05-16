#pragma once

#include "Ignis/Rendering/GRI/GRIResource.h"
#include "MetalAutoReleasePool.h"

struct GLFWwindow;

namespace CA 
{ 
    class MetalLayer; 
    class MetalDrawable;
}

namespace MTL
{
    class Texture;
}

namespace Ignis
{
    class MetalGRI;
    class MetalDevice;

    class MetalTexture2D : public GRITexture2D
    {
    private:
        MetalDevice& m_device;
        MTL::Texture* m_texture;
        uint32_t m_width, m_height;
        uint32_t m_mip_count;
    public:
        MetalTexture2D(MetalDevice* device, const GRITexture2DDesc& desc);
        MetalTexture2D(MetalDevice* device, MTL::Texture* texture, bool retain = false);
        ~MetalTexture2D() override;
        virtual inline uint32_t get_width() const override { return m_width; };
        virtual inline uint32_t get_height() const override { return m_height; };
        virtual inline uint32_t get_mip_count() const override { return m_mip_count; };
        virtual inline void* get_native_handle() const override { return static_cast<void*>(m_texture); }

        inline MTL::Texture* get_texture() { return m_texture; }
    };
    
    class MetalViewport : public GRIViewport
    {
    private:
        MetalDevice& m_device;
        MetalTexture2D* m_backbuffers[2];
        uint32_t m_current_backbuffer_index;
        uint32_t m_width, m_height;

        GLFWwindow* m_window;
        CA::MetalLayer* m_metal_layer;
        CA::MetalDrawable* m_drawable;

        void setup_callbacks();
        void create_backbuffers(uint32_t width, uint32_t height);
        void destroy_backbuffers();
    public:
        MetalViewport(MetalDevice* device, const GRIViewportDesc& desc);
        ~MetalViewport() override;

        inline void swap_buffers() { m_current_backbuffer_index = 1 - m_current_backbuffer_index; }
        inline MetalTexture2D* get_backbuffer(uint32_t index) { return m_backbuffers[index]; }
        inline MetalTexture2D* get_current_backbuffer() { return m_backbuffers[m_current_backbuffer_index]; }

        void resize(uint32_t width, uint32_t height);
        virtual inline uint32_t get_width() const override { return m_width; };
        virtual inline uint32_t get_height() const override { return m_height; };

        CA::MetalDrawable* get_drawable();
        void release_drawable();

        virtual inline void* get_native_handle() const override { return static_cast<void*>(m_window); }

        inline MetalDevice& get_device() {return m_device;}
        inline GLFWwindow* get_window() {return m_window;}
        inline CA::MetalLayer* get_metal_layer() {return m_metal_layer; }
    };

    template<class T>
	struct MetalResourceTraits
	{
	};

    template<>
	struct MetalResourceTraits<GRITexture2D>
	{
		typedef MetalTexture2D ConcreteType;
	};

    template<>
	struct MetalResourceTraits<GRIViewport>
	{
		typedef MetalViewport ConcreteType;
	};

    template <typename T>
    static inline typename MetalResourceTraits<T>::ConcreteType* resource_cast(T* resource)
    {
        return static_cast<typename MetalResourceTraits<T>::ConcreteType*>(resource);
    }
}