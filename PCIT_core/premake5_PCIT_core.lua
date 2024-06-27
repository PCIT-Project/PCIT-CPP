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
		(config.location .. "/libs"),
	}

	links{
		"Evo",
	}


project "*"
