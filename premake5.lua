-- premake5

workspace "Panthera-Project-CPP"
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
		optimize "On"

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
	-- debugdir(config.location .. "/testing")

	defines{
		"PANTHERA_BUILD_DEBUG",
		"PANTHERA_CONFIG_DEBUG",
		"PANTHERA_CONFIG_TRACE",
	}

filter {}


filter "configurations:Dev"
	warnings "High"
	-- debugdir (config.location .. "/testing")

	defines{
		"PANTHERA_BUILD_DEV",
		"PANTHERA_CONFIG_DEBUG",
	}

filter {}


filter "configurations:Optimize"
	-- debugdir (config.location .. "/testing")

	defines{
		"PANTHERA_BUILD_OPTIMIZE",
		"PANTHERA_CONFIG_DEBUG",
	}

filter {}


filter "configurations:Release"
	-- debugdir (config.location .. "/testing")

	defines{
		"PANTHERA_BUILD_RELEASE",
		"PANTHERA_CONFIG_RELEASE",
	}
filter {}


filter "configurations:ReleaseDist"
	defines{
		"PANTHERA_BUILD_DIST",
		"PANTHERA_CONFIG_RELEASE",
	}
filter {}




------------------------------------------------------------------------------
-- projects

include "./Panther/premake5_panther.lua"


------------------------------------------------------------------------------
-- grouping

project("Evo").group = "External Libs"

project("Panther").group = "Libs"

project("pthr").group = "Executables"



