-- premake5

project "PIR"
	kind "StaticLib"
	-- staticruntime "On"
	

	targetdir(target.lib)
	objdir(target.obj)

	files {
		"./src/**.cpp",
	}

	filter "action:vs*"
		buildoptions { "/bigobj" }
	filter {}

	
	includedirs{
		(config.location .. "/PCIT_core/include"),
		(config.location .. "/PCIT_core/internal/llvm_interface/include"),
		(config.location .. "/libs"),
	}

	links{
		"Evo",
		"PCIT_core",
		"PCIT_llvm_interface",
	}


project "*"





project "pirc"
	kind "ConsoleApp"
	-- staticruntime "On"
	

	targetdir(target.bin)
	objdir(target.obj)

	files{
		"./exec/**.cpp",
	}

	

	includedirs{
		(config.location .. "/libs"),
		(config.location .. "/PCIT_core/include"),		

		"./include/",
	}

	links{
		"Evo",
		"PCIT_core",
		"PIR",
	}


project "*"

