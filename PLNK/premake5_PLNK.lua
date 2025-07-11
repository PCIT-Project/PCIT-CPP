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
		(config.location .. "/dependencies/LLVM_build/include"),
		(config.location .. "/dependencies"),
	}

	links{
		"Evo",
		"PCIT_core",
	}

	LLVM.link.all_platforms()
	LLD.link_all()


project "*"


