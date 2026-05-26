workspace "Ignis"
   configurations { "Debug", "Release", "Distribution" }
   architecture "ARM64"
   multiprocessorcompile "on"
   startproject "IgnisEditor"

if _ACTION == "export-compile-commands" then
    require "export-compile-commands"
end

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories
include_dirs = {}
include_dirs["GLFW"]        = "%{wks.location}/engine/vendor/GLFW/include"
include_dirs["spdlog"]      = "%{wks.location}/engine/vendor/spdlog/include"
include_dirs["metal"]       = "%{wks.location}/engine/vendor/metal-cpp"
include_dirs["SPIRV_Cross"] = "%{wks.location}/engine/vendor/SPIRV-Cross"
include_dirs["DXC"]         = "%{wks.location}/engine/vendor/dxc/include"
include_dirs["yaml_cpp"]    = "%{wks.location}/engine/vendor/yaml-cpp/include"

group "Dependencies"
   include "engine/vendor/GLFW"
   include "engine/vendor/SPIRV-Cross"
   include "engine/vendor/yaml-cpp"
group ""

-- Include engine and editor premake files
include "engine"
include "editor"
