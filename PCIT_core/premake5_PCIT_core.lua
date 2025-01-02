-- premake5

project "PCIT_core"
	kind "StaticLib"
	-- staticruntime "On"
	

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
