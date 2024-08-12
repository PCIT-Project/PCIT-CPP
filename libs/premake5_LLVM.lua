
local LLVM_libs_loc = (config.location .. "/libs/LLVM_build/lib/")


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
		AMDGPUAsmParser         = (LLVM_libs_loc .. "LLVMAMDGPUAsmParser"),
		AMDGPUCodeGen           = (LLVM_libs_loc .. "LLVMAMDGPUCodeGen"),
		AMDGPUDesc              = (LLVM_libs_loc .. "LLVMAMDGPUDesc"),
		AMDGPUDisassembler      = (LLVM_libs_loc .. "LLVMAMDGPUDisassembler"),
		AMDGPUInfo              = (LLVM_libs_loc .. "LLVMAMDGPUInfo"),
		AMDGPUTargetMCA         = (LLVM_libs_loc .. "LLVMAMDGPUTargetMCA"),
		AMDGPUUtils             = (LLVM_libs_loc .. "LLVMAMDGPUUtils"),
		Analysis                = (LLVM_libs_loc .. "LLVMAnalysis"),
		ARMAsmParser            = (LLVM_libs_loc .. "LLVMARMAsmParser"),
		ARMCodeGen              = (LLVM_libs_loc .. "LLVMARMCodeGen"),
		ARMDesc                 = (LLVM_libs_loc .. "LLVMARMDesc"),
		ARMDisassembler         = (LLVM_libs_loc .. "LLVMARMDisassembler"),
		ARMInfo                 = (LLVM_libs_loc .. "LLVMARMInfo"),
		ARMUtils                = (LLVM_libs_loc .. "LLVMARMUtils"),
		AsmParser               = (LLVM_libs_loc .. "LLVMAsmParser"),
		AsmPrinter              = (LLVM_libs_loc .. "LLVMAsmPrinter"),
		AVRAsmParser            = (LLVM_libs_loc .. "LLVMAVRAsmParser"),
		AVRCodeGen              = (LLVM_libs_loc .. "LLVMAVRCodeGen"),
		AVRDesc                 = (LLVM_libs_loc .. "LLVMAVRDesc"),
		AVRDisassembler         = (LLVM_libs_loc .. "LLVMAVRDisassembler"),
		AVRInfo                 = (LLVM_libs_loc .. "LLVMAVRInfo"),
		BinaryFormat            = (LLVM_libs_loc .. "LLVMBinaryFormat"),
		BitReader               = (LLVM_libs_loc .. "LLVMBitReader"),
		BitstreamReader         = (LLVM_libs_loc .. "LLVMBitstreamReader"),
		BitWriter               = (LLVM_libs_loc .. "LLVMBitWriter"),
		BPFAsmParser            = (LLVM_libs_loc .. "LLVMBPFAsmParser"),
		BPFCodeGen              = (LLVM_libs_loc .. "LLVMBPFCodeGen"),
		BPFDesc                 = (LLVM_libs_loc .. "LLVMBPFDesc"),
		BPFDisassembler         = (LLVM_libs_loc .. "LLVMBPFDisassembler"),
		BPFInfo                 = (LLVM_libs_loc .. "LLVMBPFInfo"),
		CFGuard                 = (LLVM_libs_loc .. "LLVMCFGuard"),
		CFIVerify               = (LLVM_libs_loc .. "LLVMCFIVerify"),
		CodeGen                 = (LLVM_libs_loc .. "LLVMCodeGen"),
		CodeGenTypes            = (LLVM_libs_loc .. "LLVMCodeGenTypes"),
		Core                    = (LLVM_libs_loc .. "LLVMCore"),
		Coroutines              = (LLVM_libs_loc .. "LLVMCoroutines"),
		Coverage                = (LLVM_libs_loc .. "LLVMCoverage"),
		DebugInfoBTF            = (LLVM_libs_loc .. "LLVMDebugInfoBTF"),
		DebugInfoCodeView       = (LLVM_libs_loc .. "LLVMDebugInfoCodeView"),
		Debuginfod              = (LLVM_libs_loc .. "LLVMDebuginfod"),
		DebugInfoDWARF          = (LLVM_libs_loc .. "LLVMDebugInfoDWARF"),
		DebugInfoGSYM           = (LLVM_libs_loc .. "LLVMDebugInfoGSYM"),
		DebugInfoLogicalView    = (LLVM_libs_loc .. "LLVMDebugInfoLogicalView"),
		DebugInfoMSF            = (LLVM_libs_loc .. "LLVMDebugInfoMSF"),
		DebugInfoPDB            = (LLVM_libs_loc .. "LLVMDebugInfoPDB"),
		Demangle                = (LLVM_libs_loc .. "LLVMDemangle"),
		Diff                    = (LLVM_libs_loc .. "LLVMDiff"),
		DlltoolDriver           = (LLVM_libs_loc .. "LLVMDlltoolDriver"),
		DWARFLinker             = (LLVM_libs_loc .. "LLVMDWARFLinker"),
		DWARFLinkerClassic      = (LLVM_libs_loc .. "LLVMDWARFLinkerClassic"),
		DWARFLinkerParallel     = (LLVM_libs_loc .. "LLVMDWARFLinkerParallel"),
		DWP                     = (LLVM_libs_loc .. "LLVMDWP"),
		ExecutionEngine         = (LLVM_libs_loc .. "LLVMExecutionEngine"),
		Exegesis                = (LLVM_libs_loc .. "LLVMExegesis"),
		ExegesisAArch64         = (LLVM_libs_loc .. "LLVMExegesisAArch64"),
		ExegesisMips            = (LLVM_libs_loc .. "LLVMExegesisMips"),
		ExegesisPowerPC         = (LLVM_libs_loc .. "LLVMExegesisPowerPC"),
		ExegesisX86             = (LLVM_libs_loc .. "LLVMExegesisX86"),
		Extensions              = (LLVM_libs_loc .. "LLVMExtensions"),
		FileCheck               = (LLVM_libs_loc .. "LLVMFileCheck"),
		FrontendDriver          = (LLVM_libs_loc .. "LLVMFrontendDriver"),
		FrontendHLSL            = (LLVM_libs_loc .. "LLVMFrontendHLSL"),
		FrontendOffloading      = (LLVM_libs_loc .. "LLVMFrontendOffloading"),
		FrontendOpenACC         = (LLVM_libs_loc .. "LLVMFrontendOpenACC"),
		FrontendOpenMP          = (LLVM_libs_loc .. "LLVMFrontendOpenMP"),
		FuzzerCLI               = (LLVM_libs_loc .. "LLVMFuzzerCLI"),
		FuzzMutate              = (LLVM_libs_loc .. "LLVMFuzzMutate"),
		GlobalISel              = (LLVM_libs_loc .. "LLVMGlobalISel"),
		HexagonAsmParser        = (LLVM_libs_loc .. "LLVMHexagonAsmParser"),
		HexagonCodeGen          = (LLVM_libs_loc .. "LLVMHexagonCodeGen"),
		HexagonDesc             = (LLVM_libs_loc .. "LLVMHexagonDesc"),
		HexagonDisassembler     = (LLVM_libs_loc .. "LLVMHexagonDisassembler"),
		HexagonInfo             = (LLVM_libs_loc .. "LLVMHexagonInfo"),
		HipStdPar               = (LLVM_libs_loc .. "LLVMHipStdPar"),
		InstCombine             = (LLVM_libs_loc .. "LLVMInstCombine"),
		Instrumentation         = (LLVM_libs_loc .. "LLVMInstrumentation"),
		InterfaceStub           = (LLVM_libs_loc .. "LLVMInterfaceStub"),
		Interpreter             = (LLVM_libs_loc .. "LLVMInterpreter"),
		ipo                     = (LLVM_libs_loc .. "LLVMipo"),
		IRPrinter               = (LLVM_libs_loc .. "LLVMIRPrinter"),
		IRReader                = (LLVM_libs_loc .. "LLVMIRReader"),
		JITLink                 = (LLVM_libs_loc .. "LLVMJITLink"),
		LanaiAsmParser          = (LLVM_libs_loc .. "LLVMLanaiAsmParser"),
		LanaiCodeGen            = (LLVM_libs_loc .. "LLVMLanaiCodeGen"),
		LanaiDesc               = (LLVM_libs_loc .. "LLVMLanaiDesc"),
		LanaiDisassembler       = (LLVM_libs_loc .. "LLVMLanaiDisassembler"),
		LanaiInfo               = (LLVM_libs_loc .. "LLVMLanaiInfo"),
		LibDriver               = (LLVM_libs_loc .. "LLVMLibDriver"),
		LineEditor              = (LLVM_libs_loc .. "LLVMLineEditor"),
		Linker                  = (LLVM_libs_loc .. "LLVMLinker"),
		LoongArchAsmParser      = (LLVM_libs_loc .. "LLVMLoongArchAsmParser"),
		LoongArchCodeGen        = (LLVM_libs_loc .. "LLVMLoongArchCodeGen"),
		LoongArchDesc           = (LLVM_libs_loc .. "LLVMLoongArchDesc"),
		LoongArchDisassembler   = (LLVM_libs_loc .. "LLVMLoongArchDisassembler"),
		LoongArchInfo           = (LLVM_libs_loc .. "LLVMLoongArchInfo"),
		LTO                     = (LLVM_libs_loc .. "LLVMLTO"),
		MC                      = (LLVM_libs_loc .. "LLVMMC"),
		MCA                     = (LLVM_libs_loc .. "LLVMMCA"),
		MCDisassembler          = (LLVM_libs_loc .. "LLVMMCDisassembler"),
		MCJIT                   = (LLVM_libs_loc .. "LLVMMCJIT"),
		MCParser                = (LLVM_libs_loc .. "LLVMMCParser"),
		MipsAsmParser           = (LLVM_libs_loc .. "LLVMMipsAsmParser"),
		MipsCodeGen             = (LLVM_libs_loc .. "LLVMMipsCodeGen"),
		MipsDesc                = (LLVM_libs_loc .. "LLVMMipsDesc"),
		MipsDisassembler        = (LLVM_libs_loc .. "LLVMMipsDisassembler"),
		MipsInfo                = (LLVM_libs_loc .. "LLVMMipsInfo"),
		MIRParser               = (LLVM_libs_loc .. "LLVMMIRParser"),
		MSP430AsmParser         = (LLVM_libs_loc .. "LLVMMSP430AsmParser"),
		MSP430CodeGen           = (LLVM_libs_loc .. "LLVMMSP430CodeGen"),
		MSP430Desc              = (LLVM_libs_loc .. "LLVMMSP430Desc"),
		MSP430Disassembler      = (LLVM_libs_loc .. "LLVMMSP430Disassembler"),
		MSP430Info              = (LLVM_libs_loc .. "LLVMMSP430Info"),
		NVPTXCodeGen            = (LLVM_libs_loc .. "LLVMNVPTXCodeGen"),
		NVPTXDesc               = (LLVM_libs_loc .. "LLVMNVPTXDesc"),
		NVPTXInfo               = (LLVM_libs_loc .. "LLVMNVPTXInfo"),
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
		PowerPCAsmParser        = (LLVM_libs_loc .. "LLVMPowerPCAsmParser"),
		PowerPCCodeGen          = (LLVM_libs_loc .. "LLVMPowerPCCodeGen"),
		PowerPCDesc             = (LLVM_libs_loc .. "LLVMPowerPCDesc"),
		PowerPCDisassembler     = (LLVM_libs_loc .. "LLVMPowerPCDisassembler"),
		PowerPCInfo             = (LLVM_libs_loc .. "LLVMPowerPCInfo"),
		ProfileData             = (LLVM_libs_loc .. "LLVMProfileData"),
		Remarks                 = (LLVM_libs_loc .. "LLVMRemarks"),
		RISCVAsmParser          = (LLVM_libs_loc .. "LLVMRISCVAsmParser"),
		RISCVCodeGen            = (LLVM_libs_loc .. "LLVMRISCVCodeGen"),
		RISCVDesc               = (LLVM_libs_loc .. "LLVMRISCVDesc"),
		RISCVDisassembler       = (LLVM_libs_loc .. "LLVMRISCVDisassembler"),
		RISCVInfo               = (LLVM_libs_loc .. "LLVMRISCVInfo"),
		RISCVTargetMCA          = (LLVM_libs_loc .. "LLVMRISCVTargetMCA"),
		RuntimeDyld             = (LLVM_libs_loc .. "LLVMRuntimeDyld"),
		ScalarOpts              = (LLVM_libs_loc .. "LLVMScalarOpts"),
		SelectionDAG            = (LLVM_libs_loc .. "LLVMSelectionDAG"),
		SparcAsmParser          = (LLVM_libs_loc .. "LLVMSparcAsmParser"),
		SparcCodeGen            = (LLVM_libs_loc .. "LLVMSparcCodeGen"),
		SparcDesc               = (LLVM_libs_loc .. "LLVMSparcDesc"),
		SparcDisassembler       = (LLVM_libs_loc .. "LLVMSparcDisassembler"),
		SparcInfo               = (LLVM_libs_loc .. "LLVMSparcInfo"),
		Support                 = (LLVM_libs_loc .. "LLVMSupport"),
		Symbolize               = (LLVM_libs_loc .. "LLVMSymbolize"),
		SystemZAsmParser        = (LLVM_libs_loc .. "LLVMSystemZAsmParser"),
		SystemZCodeGen          = (LLVM_libs_loc .. "LLVMSystemZCodeGen"),
		SystemZDesc             = (LLVM_libs_loc .. "LLVMSystemZDesc"),
		SystemZDisassembler     = (LLVM_libs_loc .. "LLVMSystemZDisassembler"),
		SystemZInfo             = (LLVM_libs_loc .. "LLVMSystemZInfo"),
		TableGen                = (LLVM_libs_loc .. "LLVMTableGen"),
		TableGenCommon          = (LLVM_libs_loc .. "LLVMTableGenCommon"),
		TableGenGlobalISel      = (LLVM_libs_loc .. "LLVMTableGenGlobalISel"),
		Target                  = (LLVM_libs_loc .. "LLVMTarget"),
		TargetParser            = (LLVM_libs_loc .. "LLVMTargetParser"),
		TextAPI                 = (LLVM_libs_loc .. "LLVMTextAPI"),
		TextAPIBinaryReader     = (LLVM_libs_loc .. "LLVMTextAPIBinaryReader"),
		TransformUtils          = (LLVM_libs_loc .. "LLVMTransformUtils"),
		VEAsmParser             = (LLVM_libs_loc .. "LLVMVEAsmParser"),
		VECodeGen               = (LLVM_libs_loc .. "LLVMVECodeGen"),
		Vectorize               = (LLVM_libs_loc .. "LLVMVectorize"),
		VEDesc                  = (LLVM_libs_loc .. "LLVMVEDesc"),
		VEDisassembler          = (LLVM_libs_loc .. "LLVMVEDisassembler"),
		VEInfo                  = (LLVM_libs_loc .. "LLVMVEInfo"),
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
		XCoreCodeGen            = (LLVM_libs_loc .. "LLVMXCoreCodeGen"),
		XCoreDesc               = (LLVM_libs_loc .. "LLVMXCoreDesc"),
		XCoreDisassembler       = (LLVM_libs_loc .. "LLVMXCoreDisassembler"),
		XCoreInfo               = (LLVM_libs_loc .. "LLVMXCoreInfo"),
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


function LLVM.link.AMDGPU()
	links{
		LLVM.libs.AMDGPUAsmParser,
		LLVM.libs.AMDGPUCodeGen,
		LLVM.libs.AMDGPUDesc,
		LLVM.libs.AMDGPUDisassembler,
		LLVM.libs.AMDGPUInfo,
		LLVM.libs.AMDGPUTargetMCA,
		LLVM.libs.AMDGPUUtils,
	}
end

function LLVM.link.ARM()
	links{
		LLVM.libs.ARMAsmParser,
		LLVM.libs.ARMCodeGen,
		LLVM.libs.ARMDesc,
		LLVM.libs.ARMDisassembler,
		LLVM.libs.ARMInfo,
		LLVM.libs.ARMUtils,
	}
end

function LLVM.link.AVR()
	links{
		LLVM.libs.AVRAsmParser,
		LLVM.libs.AVRCodeGen,
		LLVM.libs.AVRDesc,
		LLVM.libs.AVRDisassembler,
		LLVM.libs.AVRInfo,
	}
end

function LLVM.link.BPF()
	links{
		LLVM.libs.BPFAsmParser,
		LLVM.libs.BPFCodeGen,
		LLVM.libs.BPFDesc,
		LLVM.libs.BPFDisassembler,
		LLVM.libs.BPFInfo,
	}
end

function LLVM.link.Exegesis()
	links{
		LLVM.libs.Exegesis,
		LLVM.libs.ExegesisAArch64,
		LLVM.libs.ExegesisMips,
		LLVM.libs.ExegesisPowerPC,
		LLVM.libs.ExegesisX86,
	}
end

function LLVM.link.Hexagon()
	links{
		LLVM.libs.HexagonAsmParser,
		LLVM.libs.HexagonCodeGen,
		LLVM.libs.HexagonDesc,
		LLVM.libs.HexagonDisassembler,
		LLVM.libs.HexagonInfo,
	}
end

function LLVM.link.Lanai()
	links{
		LLVM.libs.LanaiAsmParser,
		LLVM.libs.LanaiCodeGen,
		LLVM.libs.LanaiDesc,
		LLVM.libs.LanaiDisassembler,
		LLVM.libs.LanaiInfo,
	}
end

function LLVM.link.LoongArch()
	links{
		LLVM.libs.LoongArchAsmParser,
		LLVM.libs.LoongArchCodeGen,
		LLVM.libs.LoongArchDesc,
		LLVM.libs.LoongArchDisassembler,
		LLVM.libs.LoongArchInfo,
	}
end

function LLVM.link.Mips()
	links{
		LLVM.libs.MipsAsmParser,
		LLVM.libs.MipsCodeGen,
		LLVM.libs.MipsDesc,
		LLVM.libs.MipsDisassembler,
		LLVM.libs.MipsInfo,
	}
end

function LLVM.link.MSP430()
	links{
		LLVM.libs.MSP430AsmParser,
		LLVM.libs.MSP430CodeGen,
		LLVM.libs.MSP430Desc,
		LLVM.libs.MSP430Disassembler,
		LLVM.libs.MSP430Info,
	}
end

function LLVM.link.NVPTX()
	links{
		LLVM.libs.NVPTXCodeGen,
		LLVM.libs.NVPTXDesc,
		LLVM.libs.NVPTXInfo,
	}
end

function LLVM.link.PowerPC()
	links{
		LLVM.libs.PowerPCAsmParser,
		LLVM.libs.PowerPCCodeGen,
		LLVM.libs.PowerPCDesc,
		LLVM.libs.PowerPCDisassembler,
		LLVM.libs.PowerPCInfo,
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

function LLVM.link.Sparc()
	links{
		LLVM.libs.SparcAsmParser,
		LLVM.libs.SparcCodeGen,
		LLVM.libs.SparcDesc,
		LLVM.libs.SparcDisassembler,
		LLVM.libs.SparcInfo,
	}
end

function LLVM.link.SystemZ()
	links{
		LLVM.libs.SystemZAsmParser,
		LLVM.libs.SystemZCodeGen,
		LLVM.libs.SystemZDesc,
		LLVM.libs.SystemZDisassembler,
		LLVM.libs.SystemZInfo,
	}
end

function LLVM.link.WASM()
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

function LLVM.link.XCore()
	links{
		LLVM.libs.XCoreCodeGen,
		LLVM.libs.XCoreDesc,
		LLVM.libs.XCoreDisassembler,
		LLVM.libs.XCoreInfo,
	}
end


function LLVM.link.all_platforms()
	LLVM.link.AArch64()
	LLVM.link.AMDGPU()
	LLVM.link.ARM()
	LLVM.link.AVR()
	LLVM.link.BPF()
	LLVM.link.Exegesis()
	LLVM.link.Hexagon()
	LLVM.link.Lanai()
	LLVM.link.LoongArch()
	LLVM.link.Mips()
	LLVM.link.MSP430()
	LLVM.link.NVPTX()
	LLVM.link.PowerPC()
	LLVM.link.RISCV()
	LLVM.link.Sparc()
	LLVM.link.SystemZ()
	LLVM.link.WASM()
	LLVM.link.X86()
	LLVM.link.XCore()
end
