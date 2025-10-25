#pragma once

#include "Ignis/Core/PlatformDetection.h"

#ifdef IG_DEBUG
	#if defined(IG_PLATFORM_WINDOWS)
		#define IG_DEBUGBREAK() __debugbreak()
	#elif defined(IG_PLATFORM_LINUX)
		#include <signal.h>
		#define IG_DEBUGBREAK() raise(SIGTRAP)
    #elif defined(IG_PLATFORM_MACOS)
        #include <csignal>
        #define IG_DEBUGBREAK() __builtin_trap()
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define IG_ENABLE_ASSERTS
#else
	#define IG_DEBUGBREAK()
#endif

#define IG_EXPAND_MACRO(x) x
#define IG_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define IG_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#include "Ignis/Core/Log.h"
#include "Ignis/Core/Assert.h"