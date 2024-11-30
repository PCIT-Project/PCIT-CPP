-- premake5

project "PLNK_lib"
	kind "StaticLib"
	-- staticruntime "On"
	

	targetdir(target.lib)
	objdir(target.obj)

	files {
		"./src/**.cpp",
	}

	
	includedirs{
		(config.location .. "/PCIT_core/include"),
		(config.location .. "/libs/LLVM_build/include"),
		(config.location .. "/libs"),
	}

	links{
		"Evo",
		"PCIT_core",
	}


project "*"


