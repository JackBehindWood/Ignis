#pragma once

#include "wNSPrivate.hpp"
#include <Foundation/NSObject.hpp>
#include <CoreGraphics/CGGeometry.h>

namespace CA
{
    class MetalLayer;
}

class wNSView : public NS::Referencing<wNSView>
{
public:
    wNSView* init(CGRect frame);
    void set_layer (CA::MetalLayer* layer);
    void set_opaque (bool opaque);
    void set_wants_layer (bool wants_layer);
};

_NS_INLINE wNSView* wNSView::init( CGRect frame )
{
    return NS::Object::sendMessage< wNSView* >( _NSWRAPPER_PRIVATE_CLS( NSView ), _NSWRAPPER_PRIVATE_SEL( init_with_frame_ ), frame );
}

_NS_INLINE void wNSView::set_layer( CA::MetalLayer* layer )
{
    return NS::Object::sendMessage< void >( this, _NSWRAPPER_PRIVATE_SEL( set_layer_ ), layer );
}

_NS_INLINE void wNSView::set_opaque( bool opaque )
{
    return NS::Object::sendMessage< void >( this, _NSWRAPPER_PRIVATE_SEL( set_opaque_ ), opaque );
}

_NS_INLINE void wNSView::set_wants_layer( bool wants_layer )
{
    return NS::Object::sendMessage< void >( this, _NSWRAPPER_PRIVATE_SEL( set_wants_layer_ ), wants_layer );
}
