-- premake5



project "PLNK_lib"
	kind "StaticLib"
	
	filter "configurations:Optimize or Release or ReleaseDist"
		staticruntime "On"
	filter{}
	

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

	llvm_link_all_platforms()
	-- LLD.link_all()
	llvm_link("lldCOFF")
	llvm_link("lldCommon")
	llvm_link("lldELF")
	llvm_link("lldMachO")
	llvm_link("lldMinGW")
	llvm_link("lldWasm")


project "*"


