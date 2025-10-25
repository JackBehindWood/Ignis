#pragma once

#include "Ignis/Core/Base.h"
#include "Ignis/Core/Log.h"
#include <filesystem>

#ifdef IG_ENABLE_ASSERTS

	// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define IG_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { IG##type##ERROR(msg, __VA_ARGS__); IG_DEBUGBREAK(); } }
	#define IG_INTERNAL_ASSERT_WITH_MSG(type, check, ...) IG_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define IG_INTERNAL_ASSERT_NO_MSG(type, check) IG_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", IG_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define IG_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define IG_INTERNAL_ASSERT_GET_MACRO(...) IG_EXPAND_MACRO( IG_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, IG_INTERNAL_ASSERT_WITH_MSG, IG_INTERNAL_ASSERT_NO_MSG) )

	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define IG_ASSERT(...) IG_EXPAND_MACRO( IG_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define IG_CORE_ASSERT(...) IG_EXPAND_MACRO( IG_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define IG_ASSERT(...)
	#define IG_CORE_ASSERT(...)
#endif