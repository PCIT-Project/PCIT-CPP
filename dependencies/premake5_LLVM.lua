
local LLVM_libs_loc = (config.location .. "/dependencies/LLVM_build/lib/")


------------------------------------------------------------------------------
-- clang

clang = {
	libs = {
		Analysis                    = (LLVM_libs_loc .. "clangAnalysis"),
		AnalysisFlowSensitive       = (LLVM_libs_loc .. "clangAnalysisFlowSensitive"),
		AnalysisFlowSensitiveModels = (LLVM_libs_loc .. "clangAnalysisFlowSensitiveModels"),
		APINotes                    = (LLVM_libs_loc .. "clangAPINotes"),
		ARCMigrate                  = (LLVM_libs_loc .. "clangARCMigrate"),
		AST                         = (LLVM_libs_loc .. "clangAST"),
		ASTMatchers                 = (LLVM_libs_loc .. "clangASTMatchers"),
		Basic                       = (LLVM_libs_loc .. "clangBasic"),
		CodeGen                     = (LLVM_libs_loc .. "clangCodeGen"),
		CrossTU                     = (LLVM_libs_loc .. "clangCrossTU"),
		DependencyScanning          = (LLVM_libs_loc .. "clangDependencyScanning"),
		DirectoryWatcher            = (LLVM_libs_loc .. "clangDirectoryWatcher"),
		Driver                      = (LLVM_libs_loc .. "clangDriver"),
		DynamicASTMatchers          = (LLVM_libs_loc .. "clangDynamicASTMatchers"),
		Edit                        = (LLVM_libs_loc .. "clangEdit"),
		ExtractAPI                  = (LLVM_libs_loc .. "clangExtractAPI"),
		Format                      = (LLVM_libs_loc .. "clangFormat"),
		Frontend                    = (LLVM_libs_loc .. "clangFrontend"),
		FrontendTool                = (LLVM_libs_loc .. "clangFrontendTool"),
		HandleCXX                   = (LLVM_libs_loc .. "clangHandleCXX"),
		HandleLLVM                  = (LLVM_libs_loc .. "clangHandleLLVM"),
		Index                       = (LLVM_libs_loc .. "clangIndex"),
		IndexSerialization          = (LLVM_libs_loc .. "clangIndexSerialization"),
		InstallAPI                  = (LLVM_libs_loc .. "clangInstallAPI"),
		Interpreter                 = (LLVM_libs_loc .. "clangInterpreter"),
		Lex                         = (LLVM_libs_loc .. "clangLex"),
		Parse                       = (LLVM_libs_loc .. "clangParse"),
		Rewrite                     = (LLVM_libs_loc .. "clangRewrite"),
		RewriteFrontend             = (LLVM_libs_loc .. "clangRewriteFrontend"),
		Sema                        = (LLVM_libs_loc .. "clangSema"),
		Serialization               = (LLVM_libs_loc .. "clangSerialization"),
		StaticAnalyzerCheckers      = (LLVM_libs_loc .. "clangStaticAnalyzerCheckers"),
		StaticAnalyzerCore          = (LLVM_libs_loc .. "clangStaticAnalyzerCore"),
		StaticAnalyzerFrontend      = (LLVM_libs_loc .. "clangStaticAnalyzerFrontend"),
		Support                     = (LLVM_libs_loc .. "clangSupport"),
		Tooling                     = (LLVM_libs_loc .. "clangTooling"),
		ToolingASTDiff              = (LLVM_libs_loc .. "clangToolingASTDiff"),
		ToolingCore                 = (LLVM_libs_loc .. "clangToolingCore"),
		ToolingInclusions           = (LLVM_libs_loc .. "clangToolingInclusions"),
		ToolingInclusionsStdlib     = (LLVM_libs_loc .. "clangToolingInclusionsStdlib"),
		ToolingRefactoring          = (LLVM_libs_loc .. "clangToolingRefactoring"),
		ToolingSyntax               = (LLVM_libs_loc .. "clangToolingSyntax"),
		Transformer                 = (LLVM_libs_loc .. "clangTransformer"),
	},
}


-- TODO: remove

function link_clang()
	links {
		clang.libs.Analysis,
		-- clang.libs.AnalysisFlowSensitive,
		-- clang.libs.AnalysisFlowSensitiveModels,
		clang.libs.APINotes,
		-- clang.libs.ARCMigrate,
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


		LLVM.libs.FrontendOffloading,
		LLVM.libs.FrontendOpenMP,
		LLVM.libs.ipo,
		LLVM.libs.Analysis,
		LLVM.libs.FrontendHLSL,
	}


	filter "system:Windows"
		links {
			"Version.lib"
		}
	filter {}
end


------------------------------------------------------------------------------
-- LLD

LLD = {
	libs = {
		COFF   = (LLVM_libs_loc .. "lldCOFF"),
		Common = (LLVM_libs_loc .. "lldCommon"),
		ELF    = (LLVM_libs_loc .. "lldELF"),
		MachO  = (LLVM_libs_loc .. "lldMachO"),
		MinGW  = (LLVM_libs_loc .. "lldMinGW"),
		Wasm   = (LLVM_libs_loc .. "lldWasm"),
	},
}


function LLD.link_all()
	links{
		LLD.libs.COFF,
		LLD.libs.Common,
		LLD.libs.ELF,
		LLD.libs.MachO,
		LLD.libs.MinGW,
		LLD.libs.Wasm,
	}
end





------------------------------------------------------------------------------
-- LLVM

LLVM = {
	libs = {
		AArch64AsmParser        = (LLVM_libs_loc .. "LLVMAArch64AsmParser"),
		AArch64CodeGen          = (LLVM_libs_loc .. "LLVMAArch64CodeGen"),
		AArch64Desc             = (LLVM_libs_loc .. "LLVMAArch64Desc"),
		AArch64Disassembler     = (LLVM_libs_loc .. "LLVMAArch64Disassembler"),
		AArch64Info             = (LLVM_libs_loc .. "LLVMAArch64Info"),
		AArch64Utils            = (LLVM_libs_loc .. "LLVMAArch64Utils"),
		AggressiveInstCombine   = (LLVM_libs_loc .. "LLVMAggressiveInstCombine"),
		Analysis                = (LLVM_libs_loc .. "LLVMAnalysis"),
		AsmParser               = (LLVM_libs_loc .. "LLVMAsmParser"),
		AsmPrinter              = (LLVM_libs_loc .. "LLVMAsmPrinter"),
		BinaryFormat            = (LLVM_libs_loc .. "LLVMBinaryFormat"),
		BitReader               = (LLVM_libs_loc .. "LLVMBitReader"),
		BitstreamReader         = (LLVM_libs_loc .. "LLVMBitstreamReader"),
		BitWriter               = (LLVM_libs_loc .. "LLVMBitWriter"),
		CFGuard                 = (LLVM_libs_loc .. "LLVMCFGuard"),
		CFIVerify               = (LLVM_libs_loc .. "LLVMCFIVerify"),
		CGData                  = (LLVM_libs_loc .. "LLVMCGData"),
		CodeGen                 = (LLVM_libs_loc .. "LLVMCodeGen"),
		CodeGenTypes            = (LLVM_libs_loc .. "LLVMCodeGenTypes"),
		Core                    = (LLVM_libs_loc .. "LLVMCore"),
		Coroutines              = (LLVM_libs_loc .. "LLVMCoroutines"),
		Coverage                = (LLVM_libs_loc .. "LLVMCoverage"),
		DebugInfoBTF            = (LLVM_libs_loc .. "LLVMDebugInfoBTF"),
		DebugInfoCodeView       = (LLVM_libs_loc .. "LLVMDebugInfoCodeView"),
		Debuginfod              = (LLVM_libs_loc .. "LLVMDebuginfod"),
		DebugInfoDWARF          = (LLVM_libs_loc .. "LLVMDebugInfoDWARF"),
		DebugInfoDWARFLowLevel  = (LLVM_libs_loc .. "LLVMDebugInfoDWARFLowLevel"),
		DebugInfoGSYM           = (LLVM_libs_loc .. "LLVMDebugInfoGSYM"),
		DebugInfoLogicalView    = (LLVM_libs_loc .. "LLVMDebugInfoLogicalView"),
		DebugInfoMSF            = (LLVM_libs_loc .. "LLVMDebugInfoMSF"),
		DebugInfoPDB            = (LLVM_libs_loc .. "LLVMDebugInfoPDB"),
		Demangle                = (LLVM_libs_loc .. "LLVMDemangle"),
		Diff                    = (LLVM_libs_loc .. "LLVMDiff"),
		DlltoolDriver           = (LLVM_libs_loc .. "LLVMDlltoolDriver"),
		DWARFCFIChecker         = (LLVM_libs_loc .. "LLVMDWARFCFIChecker"),
		DWARFLinker             = (LLVM_libs_loc .. "LLVMDWARFLinker"),
		DWARFLinkerClassic      = (LLVM_libs_loc .. "LLVMDWARFLinkerClassic"),
		DWARFLinkerParallel     = (LLVM_libs_loc .. "LLVMDWARFLinkerParallel"),
		DWP                     = (LLVM_libs_loc .. "LLVMDWP"),
		ExecutionEngine         = (LLVM_libs_loc .. "LLVMExecutionEngine"),
		Exegesis                = (LLVM_libs_loc .. "LLVMExegesis"),
		ExegesisAArch64         = (LLVM_libs_loc .. "LLVMExegesisAArch64"),
		ExegesisRISCV           = (LLVM_libs_loc .. "LLVMExegesisRISCV"),
		ExegesisX86             = (LLVM_libs_loc .. "LLVMExegesisX86"),
		Extensions              = (LLVM_libs_loc .. "LLVMExtensions"),
		FileCheck               = (LLVM_libs_loc .. "LLVMFileCheck"),
		FrontendAtomic          = (LLVM_libs_loc .. "LLVMFrontendAtomic"),
		FrontendDirective       = (LLVM_libs_loc .. "LLVMFrontendDirective"),
		FrontendDriver          = (LLVM_libs_loc .. "LLVMFrontendDriver"),
		FrontendHLSL            = (LLVM_libs_loc .. "LLVMFrontendHLSL"),
		FrontendOffloading      = (LLVM_libs_loc .. "LLVMFrontendOffloading"),
		FrontendOpenACC         = (LLVM_libs_loc .. "LLVMFrontendOpenACC"),
		FrontendOpenMP          = (LLVM_libs_loc .. "LLVMFrontendOpenMP"),
		FuzzerCLI               = (LLVM_libs_loc .. "LLVMFuzzerCLI"),
		FuzzMutate              = (LLVM_libs_loc .. "LLVMFuzzMutate"),
		GlobalISel              = (LLVM_libs_loc .. "LLVMGlobalISel"),
		HipStdPar               = (LLVM_libs_loc .. "LLVMHipStdPar"),
		InstCombine             = (LLVM_libs_loc .. "LLVMInstCombine"),
		Instrumentation         = (LLVM_libs_loc .. "LLVMInstrumentation"),
		InterfaceStub           = (LLVM_libs_loc .. "LLVMInterfaceStub"),
		Interpreter             = (LLVM_libs_loc .. "LLVMInterpreter"),
		ipo                     = (LLVM_libs_loc .. "LLVMipo"),
		IRPrinter               = (LLVM_libs_loc .. "LLVMIRPrinter"),
		IRReader                = (LLVM_libs_loc .. "LLVMIRReader"),
		JITLink                 = (LLVM_libs_loc .. "LLVMJITLink"),
		LibDriver               = (LLVM_libs_loc .. "LLVMLibDriver"),
		LineEditor              = (LLVM_libs_loc .. "LLVMLineEditor"),
		Linker                  = (LLVM_libs_loc .. "LLVMLinker"),
		LTO                     = (LLVM_libs_loc .. "LLVMLTO"),
		MC                      = (LLVM_libs_loc .. "LLVMMC"),
		MCA                     = (LLVM_libs_loc .. "LLVMMCA"),
		MCDisassembler          = (LLVM_libs_loc .. "LLVMMCDisassembler"),
		MCJIT                   = (LLVM_libs_loc .. "LLVMMCJIT"),
		MCParser                = (LLVM_libs_loc .. "LLVMMCParser"),
		MIRParser               = (LLVM_libs_loc .. "LLVMMIRParser"),
		ObjCARCOpts             = (LLVM_libs_loc .. "LLVMObjCARCOpts"),
		ObjCopy                 = (LLVM_libs_loc .. "LLVMObjCopy"),
		Object                  = (LLVM_libs_loc .. "LLVMObject"),
		ObjectYAML              = (LLVM_libs_loc .. "LLVMObjectYAML"),
		OptDriver               = (LLVM_libs_loc .. "LLVMOptDriver"),
		Option                  = (LLVM_libs_loc .. "LLVMOption"),
		OrcDebugging            = (LLVM_libs_loc .. "LLVMOrcDebugging"),
		OrcJIT                  = (LLVM_libs_loc .. "LLVMOrcJIT"),
		OrcShared               = (LLVM_libs_loc .. "LLVMOrcShared"),
		OrcTargetProcess        = (LLVM_libs_loc .. "LLVMOrcTargetProcess"),
		Passes                  = (LLVM_libs_loc .. "LLVMPasses"),
		ProfileData             = (LLVM_libs_loc .. "LLVMProfileData"),
		Remarks                 = (LLVM_libs_loc .. "LLVMRemarks"),
		RISCVAsmParser          = (LLVM_libs_loc .. "LLVMRISCVAsmParser"),
		RISCVCodeGen            = (LLVM_libs_loc .. "LLVMRISCVCodeGen"),
		RISCVDesc               = (LLVM_libs_loc .. "LLVMRISCVDesc"),
		RISCVDisassembler       = (LLVM_libs_loc .. "LLVMRISCVDisassembler"),
		RISCVInfo               = (LLVM_libs_loc .. "LLVMRISCVInfo"),
		RISCVTargetMCA          = (LLVM_libs_loc .. "LLVMRISCVTargetMCA"),
		RuntimeDyld             = (LLVM_libs_loc .. "LLVMRuntimeDyld"),
		SandboxIR               = (LLVM_libs_loc .. "LLVMSandboxIR"),
		ScalarOpts              = (LLVM_libs_loc .. "LLVMScalarOpts"),
		SelectionDAG            = (LLVM_libs_loc .. "LLVMSelectionDAG"),
		Support                 = (LLVM_libs_loc .. "LLVMSupport"),
		Symbolize               = (LLVM_libs_loc .. "LLVMSymbolize"),
		TableGen                = (LLVM_libs_loc .. "LLVMTableGen"),
		TableGenBasic           = (LLVM_libs_loc .. "LLVMTableGenBasic"),
		TableGenCommon          = (LLVM_libs_loc .. "LLVMTableGenCommon"),
		Target                  = (LLVM_libs_loc .. "LLVMTarget"),
		TargetParser            = (LLVM_libs_loc .. "LLVMTargetParser"),
		Telemetry               = (LLVM_libs_loc .. "LLVMTelemetry"),
		TextAPI                 = (LLVM_libs_loc .. "LLVMTextAPI"),
		TextAPIBinaryReader     = (LLVM_libs_loc .. "LLVMTextAPIBinaryReader"),
		TransformUtils          = (LLVM_libs_loc .. "LLVMTransformUtils"),
		Vectorize               = (LLVM_libs_loc .. "LLVMVectorize"),
		WebAssemblyAsmParser    = (LLVM_libs_loc .. "LLVMWebAssemblyAsmParser"),
		WebAssemblyCodeGen      = (LLVM_libs_loc .. "LLVMWebAssemblyCodeGen"),
		WebAssemblyDesc         = (LLVM_libs_loc .. "LLVMWebAssemblyDesc"),
		WebAssemblyDisassembler = (LLVM_libs_loc .. "LLVMWebAssemblyDisassembler"),
		WebAssemblyInfo         = (LLVM_libs_loc .. "LLVMWebAssemblyInfo"),
		WebAssemblyUtils        = (LLVM_libs_loc .. "LLVMWebAssemblyUtils"),
		WindowsDriver           = (LLVM_libs_loc .. "LLVMWindowsDriver"),
		WindowsManifest         = (LLVM_libs_loc .. "LLVMWindowsManifest"),
		X86AsmParser            = (LLVM_libs_loc .. "LLVMX86AsmParser"),
		X86CodeGen              = (LLVM_libs_loc .. "LLVMX86CodeGen"),
		X86Desc                 = (LLVM_libs_loc .. "LLVMX86Desc"),
		X86Disassembler         = (LLVM_libs_loc .. "LLVMX86Disassembler"),
		X86Info                 = (LLVM_libs_loc .. "LLVMX86Info"),
		X86TargetMCA            = (LLVM_libs_loc .. "LLVMX86TargetMCA"),
		XRay                    = (LLVM_libs_loc .. "LLVMXRay"),
	},

	link = {},
}



function LLVM.link.AArch64()
	links{
		LLVM.libs.AArch64AsmParser,
		LLVM.libs.AArch64CodeGen,
		LLVM.libs.AArch64Desc,
		LLVM.libs.AArch64Disassembler,
		LLVM.libs.AArch64Info,
		LLVM.libs.AArch64Utils,
	}
end



function LLVM.link.RISCV()
	links{
		LLVM.libs.RISCVAsmParser,
		LLVM.libs.RISCVCodeGen,
		LLVM.libs.RISCVDesc,
		LLVM.libs.RISCVDisassembler,
		LLVM.libs.RISCVInfo,
		LLVM.libs.RISCVTargetMCA,
	}
end

function LLVM.link.WebAssembly()
	links{
		LLVM.libs.WebAssemblyAsmParser,
		LLVM.libs.WebAssemblyCodeGen,
		LLVM.libs.WebAssemblyDesc,
		LLVM.libs.WebAssemblyDisassembler,
		LLVM.libs.WebAssemblyInfo,
		LLVM.libs.WebAssemblyUtils,
	}
end

function LLVM.link.X86()
	links{
		LLVM.libs.X86AsmParser,
		LLVM.libs.X86CodeGen,
		LLVM.libs.X86Desc,
		LLVM.libs.X86Disassembler,
		LLVM.libs.X86Info,
		LLVM.libs.X86TargetMCA,
	}
end



function LLVM.link.all_platforms()
	LLVM.link.AArch64()
	LLVM.link.RISCV()
	LLVM.link.WebAssembly()
	LLVM.link.X86()
end
