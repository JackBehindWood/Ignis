#pragma once

#include "Ignis/Rendering/GRI/GRIViewport.h"

namespace Ignis
{
    class MetalViewport : public GRIViewport
    {
    private:
        uint32_t m_width, m_height;
    public:
        MetalViewport(uint32_t width, uint32_t height);
        ~MetalViewport() override;

        void resize(uint32_t width, uint32_t height);
        virtual uint32_t get_width() const override;
        virtual uint32_t get_height() const override;
    };
}