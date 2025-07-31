-- premake5

project "PCIT_clang_interface"
	kind "StaticLib"
	-- staticruntime "On"

	
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

		clang.libs.Analysis,
		-- clang.libs.AnalysisFlowSensitive,
		-- clang.libs.AnalysisFlowSensitiveModels,
		clang.libs.APINotes,
		clang.libs.AST,
		clang.libs.ASTMatchers,
		clang.libs.Basic,
		clang.libs.CodeGen,
		-- clang.libs.CrossTU,
		-- clang.libs.DependencyScanning,
		-- clang.libs.DirectoryWatcher,
		clang.libs.Driver,
		-- clang.libs.DynamicASTMatchers,
		clang.libs.Edit,
		-- clang.libs.ExtractAPI,
		-- clang.libs.Format,
		clang.libs.Frontend,
		-- clang.libs.FrontendTool,
		-- clang.libs.HandleCXX,
		-- clang.libs.HandleLLVM,
		-- clang.libs.Index,
		-- clang.libs.IndexSerialization,
		-- clang.libs.InstallAPI,
		-- clang.libs.Interpreter,
		clang.libs.Lex,
		clang.libs.Parse,
		-- clang.libs.Rewrite,
		-- clang.libs.RewriteFrontend,
		clang.libs.Sema,
		clang.libs.Serialization,
		-- clang.libs.StaticAnalyzerCheckers,
		-- clang.libs.StaticAnalyzerCore,
		-- clang.libs.StaticAnalyzerFrontend,
		clang.libs.Support,
		-- clang.libs.Tooling,
		-- clang.libs.ToolingASTDiff,
		-- clang.libs.ToolingCore,
		-- clang.libs.ToolingInclusions,
		-- clang.libs.ToolingInclusionsStdlib,
		-- clang.libs.ToolingRefactoring,
		-- clang.libs.ToolingSyntax,
		-- clang.libs.Transformer,
	}


	filter "system:Windows"
		links{
			"Version"
		}
	filter {}


project "*"


