#pragma once

#include "Ignis/Core/Base.h"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace Ignis 
{

	class Log
	{
	public:
		static void init(bool log_to_file = true);

		static SharedPtr<spdlog::logger>& get_core_logger() { return s_core_logger; }
		static SharedPtr<spdlog::logger>& get_client_logger() { return s_client_logger; }
	private:
		static SharedPtr<spdlog::logger> s_core_logger;
		static SharedPtr<spdlog::logger> s_client_logger;
	};

}

// Core log macros
#define IG_CORE_TRACE(...)    ::Ignis::Log::get_core_logger()->trace(__VA_ARGS__)
#define IG_CORE_INFO(...)     ::Ignis::Log::get_core_logger()->info(__VA_ARGS__)
#define IG_CORE_WARN(...)     ::Ignis::Log::get_core_logger()->warn(__VA_ARGS__)
#define IG_CORE_ERROR(...)    ::Ignis::Log::get_core_logger()->error(__VA_ARGS__)
#define IG_CORE_CRITICAL(...) ::Ignis::Log::get_core_logger()->critical(__VA_ARGS__)

// Client log macros
#define IG_TRACE(...)         ::Ignis::Log::get_client_logger()->trace(__VA_ARGS__)
#define IG_INFO(...)          ::Ignis::Log::get_client_logger()->info(__VA_ARGS__)
#define IG_WARN(...)          ::Ignis::Log::get_client_logger()->warn(__VA_ARGS__)
#define IG_ERROR(...)         ::Ignis::Log::get_client_logger()->error(__VA_ARGS__)
#define IG_CRITICAL(...)      ::Ignis::Log::get_client_logger()->critical(__VA_ARGS__)