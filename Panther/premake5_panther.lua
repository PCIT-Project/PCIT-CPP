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
		(config.location .. "/PIR/include"),
		(config.location .. "/PCIT_core/include"),
		(config.location .. "/PCIT_core/internal/llvm_interface/include"),
		(config.location .. "/dependencies"),
	}

	links{
		"Evo",
		"PCIT_core",
		"PIR",
	}


	filter "action:vs*"
		buildoptions{ "/bigobj" }
	filter {}

project "*"




project "pthr"
	kind "ConsoleApp"
	-- staticruntime "On"
	

	targetdir(target.bin)
	objdir(target.obj)

	files{
		"./exec/**.cpp",
	}

	

	includedirs{
		(config.location .. "/dependencies"),
		(config.location .. "/PCIT_core/include"),
		(config.location .. "/PLNK/include"),
		(config.location .. "/PIR/include"),

		"./include/",
	}

	links{
		"Evo",
		"PCIT_core",
		"PLNK_lib",
		"PIR",
		"Panther",
	}



project "*"