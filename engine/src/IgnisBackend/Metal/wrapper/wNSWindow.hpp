#pragma once


#include "wNSPrivate.hpp"
#include "wNSView.hpp"
#include <Foundation/NSObject.hpp>

#include <CoreGraphics/CGGeometry.h>

_NS_OPTIONS( NS::UInteger, wNSWindowStyleMask )
{
    WindowStyleMaskBorderless       = 0,
    WindowStyleMaskTitled           = ( 1 << 0 ),
    WindowStyleMaskClosable         = ( 1 << 1 ),
    WindowStyleMaskMiniaturizable   = ( 1 << 2 ),
    WindowStyleMaskResizable        = ( 1 << 3 ),
    WindowStyleMaskTexturedBackground = ( 1 << 8 ),
    WindowStyleMaskUnifiedTitleAndToolbar = ( 1 << 12 ),
    WindowStyleMaskFullScreen       = ( 1 << 14 ),
    WindowStyleMaskFullSizeContentView = ( 1 << 15 ),
    WindowStyleMaskUtilityWindow    = ( 1 << 4 ),
    WindowStyleMaskDocModalWindow   = ( 1 << 6 ),
    WindowStyleMaskNonactivatingPanel   = ( 1 << 7 ),
    WindowStyleMaskHUDWindow        = ( 1 << 13 )
};

_NS_ENUM( NS::UInteger, wNSBackingStoreType )
{
    BackingStoreRetained = 0,
    BackingStoreNonretained = 1,
    BackingStoreBuffered = 2
};

class wNSWindow : public NS::Referencing<wNSWindow>
{
public:
    static wNSWindow* alloc();
    wNSWindow* init(CGRect content_rect, wNSWindowStyleMask style_mask, wNSBackingStoreType backing, bool defer );
    wNSView* content_view() const;
    void set_content_view(const wNSView* content_view);
    void make_key_and_order_front(const NS::Object* sender);
    void set_title(const NS::String* title);
    void close();
};


_NS_INLINE wNSWindow* wNSWindow::alloc()
{
    return NS::Object::sendMessage<wNSWindow*>( _NSWRAPPER_PRIVATE_CLS( NSWindow ), _NSWRAPPER_PRIVATE_SEL( alloc_ ) );
}

_NS_INLINE wNSWindow* wNSWindow::init( CGRect content_rect, wNSWindowStyleMask style_mask, wNSBackingStoreType backing, bool defer )
{
    return NS::Object::sendMessage<wNSWindow*>( this, _NSWRAPPER_PRIVATE_SEL( init_with_content_rect_style_mask_backing_defer_ ), content_rect, style_mask, backing, defer );
}

_NS_INLINE wNSView* wNSWindow::content_view() const
{
    return NS::Object::sendMessage< wNSView* >( this, _NSWRAPPER_PRIVATE_SEL( content_view_ ));
}

_NS_INLINE void wNSWindow::set_content_view( const wNSView* content_view )
{
    NS::Object::sendMessage< void >( this, _NSWRAPPER_PRIVATE_SEL( set_content_view_ ), content_view );
}

_NS_INLINE void wNSWindow::make_key_and_order_front( const NS::Object* sender )
{
    NS::Object::sendMessage< void >( this, _NSWRAPPER_PRIVATE_SEL( make_key_and_order_front_ ), sender );
}

_NS_INLINE void wNSWindow::set_title( const NS::String* title )
{
    NS::Object::sendMessage< void >( this, _NSWRAPPER_PRIVATE_SEL( set_title_), title );
}

_NS_INLINE void wNSWindow::close()
{
    NS::Object::sendMessage< void >( this, _NSWRAPPER_PRIVATE_SEL( close_ ) );
}
