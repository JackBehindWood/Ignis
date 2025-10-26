
#pragma once


#include <objc/runtime.h>

#define _NSWRAPPER_PRIVATE_CLS( symbol )                   ( wNSPrivate::Class::s_k ## symbol )
#define _NSWRAPPER_PRIVATE_SEL( accessor )                 ( wNSPrivate::Selector::s_k ## accessor )

#if defined( NS_PRIVATE_IMPLEMENTATION )

#define _NSWRAPPER_PRIVATE_VISIBILITY                        __attribute__( ( visibility( "default" ) ) )
#define _NSWRAPPER_PRIVATE_IMPORT                          __attribute__( ( weak_import ) )

#if __OBJC__
#define  _NSWRAPPER_PRIVATE_OBJC_LOOKUP_CLASS( symbol  )   ( ( __bridge void* ) objc_lookUpClass( # symbol ) )
#else
#define  _NSWRAPPER_PRIVATE_OBJC_LOOKUP_CLASS( symbol  )   objc_lookUpClass( # symbol )
#endif // __OBJC__

#define _NSWRAPPER_PRIVATE_DEF_CLS( symbol )                void* s_k ## symbol _NS_PRIVATE_VISIBILITY = _NS_PRIVATE_OBJC_LOOKUP_CLASS( symbol );
#define _NSWRAPPER_PRIVATE_DEF_SEL( accessor, symbol )     SEL s_k ## accessor _NS_PRIVATE_VISIBILITY = sel_registerName( symbol );
#define _NSWRAPPER_PRIVATE_DEF_CONST( type, symbol )       _NS_EXTERN type const   NS ## symbol   _NS_PRIVATE_IMPORT; \
                                                    type const NS::symbol = ( nullptr != &NS ## symbol ) ? NS ## symbol : nullptr;
#else

#define _NSWRAPPER_PRIVATE_DEF_CLS( symbol )                extern void* s_k ## symbol;
#define _NSWRAPPER_PRIVATE_DEF_SEL( accessor, symbol )     extern SEL s_k ## accessor;
#define _NSWRAPPER_PRIVATE_DEF_CONST( type, symbol )


#endif // NS_PRIVATE_IMPLEMENTATION

namespace wNSPrivate::Class
{
    _NSWRAPPER_PRIVATE_DEF_CLS(NSView);
    _NSWRAPPER_PRIVATE_DEF_CLS(NSWindow);
}

namespace wNSPrivate::Selector
{
    _NSWRAPPER_PRIVATE_DEF_SEL(init_with_frame_, "initWithFrame:");
    _NSWRAPPER_PRIVATE_DEF_SEL(set_layer_, "setLayer:");
    _NSWRAPPER_PRIVATE_DEF_SEL(set_opaque_, "setOpaque:");
    _NSWRAPPER_PRIVATE_DEF_SEL(set_wants_layer_, "setWantsLayer:");
    
    _NSWRAPPER_PRIVATE_DEF_SEL(alloc_, "alloc");
    _NSWRAPPER_PRIVATE_DEF_SEL(init_with_content_rect_style_mask_backing_defer_, "initWithContentRect:styleMask:backing:defer:");
    _NSWRAPPER_PRIVATE_DEF_SEL(content_view_, "contentView");
    _NSWRAPPER_PRIVATE_DEF_SEL(set_content_view_, "setContentView:");
    _NSWRAPPER_PRIVATE_DEF_SEL(make_key_and_order_front_, "makeKeyAndOrderFront:");
    _NSWRAPPER_PRIVATE_DEF_SEL(set_title_, "setTitle:");
    _NSWRAPPER_PRIVATE_DEF_SEL(close_, "close");
}

/*

namespace NS::Private::Class {

_NSWRAPPER_PRIVATE_DEF_CLS( NSApplication );
_NSWRAPPER_PRIVATE_DEF_CLS( NSRunningApplication );
_NSWRAPPER_PRIVATE_DEF_CLS( NSView );
_NSWRAPPER_PRIVATE_DEF_CLS( NSWindow );
_NSWRAPPER_PRIVATE_DEF_CLS( NSMenu );
_NSWRAPPER_PRIVATE_DEF_CLS( NSMenuItem );

} // Class


namespace NS::Private::Selector
{

_NSWRAPPER_PRIVATE_DEF_SEL( addItem_,
                        "addItem:" );

_NSWRAPPER_PRIVATE_DEF_SEL( addItemWithTitle_action_keyEquivalent_,
                        "addItemWithTitle:action:keyEquivalent:" );

_NSWRAPPER_PRIVATE_DEF_SEL( applicationDidFinishLaunching_,
                        "applicationDidFinishLaunching:" );

_NSWRAPPER_PRIVATE_DEF_SEL( applicationShouldTerminateAfterLastWindowClosed_,
                        "applicationShouldTerminateAfterLastWindowClosed:" );

_NSWRAPPER_PRIVATE_DEF_SEL( applicationWillFinishLaunching_,
                        "applicationWillFinishLaunching:" );

_NSWRAPPER_PRIVATE_DEF_SEL( close,
                        "close" );

_NSWRAPPER_PRIVATE_DEF_SEL( currentApplication,
                        "currentApplication" );

_NSWRAPPER_PRIVATE_DEF_SEL( keyEquivalentModifierMask,
                         "keyEquivalentModifierMask" );

_NSWRAPPER_PRIVATE_DEF_SEL( localizedName,
                        "localizedName" );

_NSWRAPPER_PRIVATE_DEF_SEL( sharedApplication,
                        "sharedApplication" );

_NSWRAPPER_PRIVATE_DEF_SEL( setDelegate_,
                        "setDelegate:" );

_NSWRAPPER_PRIVATE_DEF_SEL( setActivationPolicy_,
                        "setActivationPolicy:" );

_NSWRAPPER_PRIVATE_DEF_SEL( activateIgnoringOtherApps_,
                        "activateIgnoringOtherApps:" );

_NSWRAPPER_PRIVATE_DEF_SEL( run,
                        "run" );

_NSWRAPPER_PRIVATE_DEF_SEL( terminate_,
                        "terminate:" );

_NSWRAPPER_PRIVATE_DEF_SEL( initWithContentRect_styleMask_backing_defer_,
                        "initWithContentRect:styleMask:backing:defer:" );

_NSWRAPPER_PRIVATE_DEF_SEL( initWithFrame_,
                        "initWithFrame:" );

_NSWRAPPER_PRIVATE_DEF_SEL( initWithTitle_,
                        "initWithTitle:" );

_NSWRAPPER_PRIVATE_DEF_SEL( setLayer_,
                        "setLayer:" );
    
_NSWRAPPER_PRIVATE_DEF_SEL( setOpaque_,
                        "setOpaque:" );

_NSWRAPPER_PRIVATE_DEF_SEL( setWantsLayer_,
                        "setWantsLayer:" );
    
_NSWRAPPER_PRIVATE_DEF_SEL( contentView,
                        "contentView" );
    
_NSWRAPPER_PRIVATE_DEF_SEL( setContentView_,
                        "setContentView:" );

_NSWRAPPER_PRIVATE_DEF_SEL( makeKeyAndOrderFront_,
                        "makeKeyAndOrderFront:" );

_NSWRAPPER_PRIVATE_DEF_SEL( setKeyEquivalentModifierMask_,
                        "setKeyEquivalentModifierMask:" );

_NSWRAPPER_PRIVATE_DEF_SEL( setMainMenu_,
                        "setMainMenu:" );

_NSWRAPPER_PRIVATE_DEF_SEL( setSubmenu_,
                        "setSubmenu:" );

_NSWRAPPER_PRIVATE_DEF_SEL( setTitle_,
                        "setTitle:" );

_NSWRAPPER_PRIVATE_DEF_SEL( windows,
                        "windows" );

}

*/
