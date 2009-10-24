-- A solution contains projects, and defines the available configurations
solution "sample"
configurations { "Debug", "Release" }

-- A project defines one build target
project "sample"
--kind "WindowedApp"
kind "ConsoleApp"
--kind "SharedLib"
--kind "StaticLib"
language "C++"
files { "**.h", "**.cpp" }

-- png
defines { "USE_PNG" }
links { "png" }

configuration "Debug"
defines { "DEBUG" }
flags { "Symbols" }
targetdir "debug"

configuration "Release"
defines { "NDEBUG" }
flags { "Optimize" }  
targetdir "release"


