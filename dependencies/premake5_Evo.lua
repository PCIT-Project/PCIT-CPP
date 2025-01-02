-- premake5

project "Evo"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	exceptionhandling "Off"
	allmodulespublic "Off"

	
	targetdir(target.lib)
	objdir(target.obj)


	files {
		(config.location .. "/extern/Evo/tools/**.cpp"),
		(config.location .. "/extern/Evo/tools/**.h"),
	}

	includedirs{
		(config.location .. "/extern/Evo"),
	}



	------------------------------------------
	-- build

	filter "configurations:Debug"
		defines{
			"EVO_CONFIG_DEBUG",
		}
	filter {}

	filter "configurations:Dev"
		defines{
			"EVO_CONFIG_DEBUG",
		}
	filter {}


	filter "configurations:Optimize"
		defines{
			"EVO_CONFIG_DEBUG",
		}
	filter {}

	filter "configurations:Release"
		defines{
			-- none...
		}
	filter {}


	filter "configurations:ReleaseDist"
		defines{
			-- none...
		}
	filter {}



project "*"