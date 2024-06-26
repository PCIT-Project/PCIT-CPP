-- premake5

project "Panther"
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




project "pthr"
	kind "ConsoleApp"
	-- staticruntime "On"
	

	targetdir(target.bin)
	objdir(target.obj)

	files {
		"./exec/**.cpp",
	}

	

	includedirs{
		(config.location .. "/libs"),

		"./include/",
	}

	links{
		"Evo",
		"Panther",
	}


project "*"