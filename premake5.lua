--nana
workspace "nana"
	configurations { "Debug", "Release" }
	platforms { "Win64" }
	--location "build-test/vs2022"

	filter { "platforms:Win64" }
    	system "Windows"
    	architecture "x64"

	filter "configurations:Debug"
		defines { "_DEBUG", "_LIB" }
		symbols "On"
		targetdir "bin/Debug_x64"
		objdir "bin/Debug_x64_tmp"

	filter "configurations:Release"
		defines { "NDEBUG", "_LIB" }
		optimize "On"
		targetdir "bin/Release_x64"
		objdir "bin/Release_x64_tmp"

project "nana"
	kind "StaticLib"

	language "C++"
	cppdialect "C++17"

	includedirs { 
		"include" 
	}
	files {
		"include/nana/**.hpp",
		"source/**.cpp",
	}
