-- premake5

project "PCIT_clang_interface"
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
		(config.location .. "/dependencies/LLVM_build/include"),
		(config.location .. "/dependencies"),
	}


	links{
		"Evo",
	}


	llvm_link("clangAnalysis")
	-- llvm_link("clangAnalysisFlowSensitive")
	-- llvm_link("clangAnalysisFlowSensitiveModels")
	llvm_link("clangAPINotes")
	llvm_link("clangAST")
	llvm_link("clangASTMatchers")
	llvm_link("clangBasic")
	llvm_link("clangCodeGen")
	-- llvm_link("clangCrossTU")
	-- llvm_link("clangDependencyScanning")
	-- llvm_link("clangDirectoryWatcher")
	llvm_link("clangDriver")
	-- llvm_link("clangDynamicASTMatchers")
	llvm_link("clangEdit")
	-- llvm_link("clangExtractAPI")
	-- llvm_link("clangFormat")
	llvm_link("clangFrontend")
	-- llvm_link("clangFrontendTool")
	-- llvm_link("clangHandleCXX")
	-- llvm_link("clangHandleLLVM")
	-- llvm_link("clangIndex")
	-- llvm_link("clangIndexSerialization")
	-- llvm_link("clangInstallAPI")
	-- llvm_link("clangInterpreter")
	llvm_link("clangLex")
	llvm_link("clangParse")
	-- llvm_link("clangRewrite")
	-- llvm_link("clangRewriteFrontend")
	llvm_link("clangSema")
	llvm_link("clangSerialization")
	-- llvm_link("clangStaticAnalyzerCheckers")
	-- llvm_link("clangStaticAnalyzerCore")
	-- llvm_link("clangStaticAnalyzerFrontend")
	llvm_link("clangSupport")
	-- llvm_link("clangTooling")
	-- llvm_link("clangToolingASTDiff")
	-- llvm_link("clangToolingCore")
	-- llvm_link("clangToolingInclusions")
	-- llvm_link("clangToolingInclusionsStdlib")
	-- llvm_link("clangToolingRefactoring")
	-- llvm_link("clangToolingSyntax")
	-- llvm_link("clangTransformer")



	filter "system:Windows"
		links{
			"Version"
		}
	filter {}


project "*"


