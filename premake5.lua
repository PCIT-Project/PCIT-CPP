-- premake5

workspace "PCIT-CPP"
	architecture "x64"

	configurations{
		"Debug",
		"Dev",
		"Optimize",
		"Release",
		"ReleaseDist",
	}

	platforms {
		"Windows",
		"Linux",
	}
	defaultplatform "Windows"


	flags{
		"MultiProcessorCompile",
		-- "NoPCH",
	}


	startproject "pthr"


	---------------------------------------
	-- platform

	filter "system:Windows"
		system "windows"
	filter {}


	filter "system:Linux"
		system "linux"
	filter {}




	------------------------------------------------------------------------------
	-- configs

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"
		optimize "Off"

		defines{
			"_DEBUG",
		}
	filter {}


	filter "configurations:Dev"
		runtime "Debug"
		symbols "On"
		optimize "Off"

		defines{
			"_DEBUG",
		}
	filter {}


	filter "configurations:Optimize"
		runtime "Debug" -- TODO: figure out how to have LLVM build with release runtime
		symbols "On"
		optimize "Full"

		defines{
			"NDEBUG",
		}

		flags{
			"LinkTimeOptimization",
		}
	filter {}


	filter "configurations:Release"
		runtime "Debug" -- TODO: figure out how to have LLVM build with release runtime
		symbols "Off"
		optimize "Full"

		defines{
			"NDEBUG",
		}

		flags{
			"LinkTimeOptimization",
		}
	filter {}



	filter "configurations:ReleaseDist"
		runtime "Debug" -- TODO: figure out how to have LLVM build with release runtime
		symbols "Off"
		optimize "Full"

		defines{
			"NDEBUG",
		}

		flags{
			"LinkTimeOptimization",
		}
	filter {}



------------------------------------------------------------------------------
-- global variables
	
config = {
	location = ("%{wks.location}"),
	platform = ("%{cfg.platform}"),
	build    = ("%{cfg.buildcfg}"),
	project  = ("%{prj.name}"),
}


target = {
	bin = string.format("%s/build/%s/%s/bin/",   config.location, config.platform, config.build),
	lib = string.format("%s/build/%s/%s/lib/%s", config.location, config.platform, config.build, config.project),
	obj = string.format("%s/build/%s/%s/obj/%s", config.location, config.platform, config.build, config.project),
}



------------------------------------------------------------------------------
-- extern lib projects


include "libs/premake5_Evo.lua"


------------------------------------------------------------------------------
-- project settings

language "C++"
cppdialect "C++20"
exceptionhandling "Off"
allmodulespublic "Off"



---------------------------------------
-- build

filter "configurations:Debug"
	warnings "High"
	debugdir(config.location .. "/testing")

	defines{
		"PCIT_BUILD_DEBUG",
		"PCIT_CONFIG_DEBUG",
		"PCIT_CONFIG_TRACE",
	}

filter {}


filter "configurations:Dev"
	warnings "High"
	debugdir (config.location .. "/testing")

	defines{
		"PCIT_BUILD_DEV",
		"PCIT_CONFIG_DEBUG",
	}

filter {}


filter "configurations:Optimize"
	debugdir (config.location .. "/testing")

	defines{
		"PCIT_BUILD_OPTIMIZE",
		"PCIT_CONFIG_RELEASE",
	}

filter {}


filter "configurations:Release"
	debugdir (config.location .. "/testing")

	defines{
		"PCIT_BUILD_RELEASE",
		"PCIT_CONFIG_RELEASE",
	}
filter {}


filter "configurations:ReleaseDist"
	defines{
		"PCIT_BUILD_DIST",
		"PCIT_CONFIG_RELEASE",
	}
filter {}




------------------------------------------------------------------------------
-- projects

include "./libs/premake5_LLVM.lua"

include "./PCIT_core/premake5_PCIT_core.lua"
include "./Panther/premake5_panther.lua"
include "./PCIT_core/internal/llvm_interface/premake5_PCIT_llvm_interface.lua"


------------------------------------------------------------------------------
-- grouping

project("Evo").group = "External Libs"

project("PCIT_core").group = "Libs"
project("Panther").group = "Libs"

project("PCIT_llvm_interface").group = "DynLibs"

project("pthr").group = "Executables"



