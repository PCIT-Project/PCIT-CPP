-- premake5

project "PCIT_core"
	kind "StaticLib"
	
	filter "configurations:Optimize or Release"
		staticruntime "On"
	filter{}
	

	targetdir(target.lib)
	objdir(target.obj)

	files {
		"./src/**.cpp",
	}

	
	includedirs{
		(config.location .. "/dependencies"),
	}

	links{
		"Evo",
	}


project "*"
