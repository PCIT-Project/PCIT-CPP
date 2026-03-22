-- premake5

project "PCIT_llvm_interface"
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

	-- AArch64
	llvm_link("LLVMAggressiveInstCombine")
	llvm_link("LLVMAnalysis")
	llvm_link("LLVMAsmParser")
	llvm_link("LLVMAsmPrinter")
	llvm_link("LLVMBinaryFormat")
	llvm_link("LLVMBitReader")
	llvm_link("LLVMBitstreamReader")
	llvm_link("LLVMBitWriter")
	llvm_link("LLVMCFGuard")
	-- llvm_link("LLVMCFIVerify")
	llvm_link("LLVMCGData")
	llvm_link("LLVMCodeGen")
	llvm_link("LLVMCodeGenTypes")
	llvm_link("LLVMCore")
	llvm_link("LLVMCoroutines")
	llvm_link("LLVMCoverage")
	-- llvm_link("LLVMDebugInfoBTF")
	llvm_link("LLVMDebugInfoCodeView")
	-- llvm_link("LLVMDebuginfod")
	llvm_link("LLVMDebugInfoDWARF")
	llvm_link("LLVMDebugInfoDWARFLowLevel")
	-- llvm_link("LLVMDebugInfoGSYM")
	-- llvm_link("LLVMDebugInfoLogicalView")
	llvm_link("LLVMDebugInfoMSF")
	llvm_link("LLVMDebugInfoPDB")
	llvm_link("LLVMDemangle")
	-- llvm_link("LLVMDiff")
	-- llvm_link("LLVMDlltoolDriver")
	-- llvm_link("LLVMDWARFCFIChecker")
	-- llvm_link("LLVMDWARFLinker")
	-- llvm_link("LLVMDWARFLinkerClassic")
	-- llvm_link("LLVMDWARFLinkerParallel")
	-- llvm_link("LLVMDWP")
	llvm_link("LLVMExecutionEngine")
	-- llvm_link("LLVMExegesis")
	-- llvm_link("LLVMExegesisAArch64")
	-- llvm_link("LLVMExegesisRISCV")
	-- llvm_link("LLVMExegesisX86")
	-- llvm_link("LLVMExtensions")
	-- llvm_link("LLVMFileCheck")
	llvm_link("LLVMFrontendAtomic")
	llvm_link("LLVMFrontendDirective")
	llvm_link("LLVMFrontendDriver")
	llvm_link("LLVMFrontendHLSL")
	llvm_link("LLVMFrontendOffloading")
	-- llvm_link("LLVMFrontendOpenACC")
	llvm_link("LLVMFrontendOpenMP")
	-- llvm_link("LLVMFuzzerCLI")
	-- llvm_link("LLVMFuzzMutate")
	llvm_link("LLVMGlobalISel")
	llvm_link("LLVMHipStdPar")
	llvm_link("LLVMInstCombine")
	llvm_link("LLVMInstrumentation")
	-- llvm_link("LLVMInterfaceStub")
	-- llvm_link("LLVMInterpreter")
	llvm_link("LLVMipo")
	llvm_link("LLVMIRPrinter")
	llvm_link("LLVMIRReader")
	llvm_link("LLVMJITLink")
	llvm_link("LLVMLibDriver")
	-- llvm_link("LLVMLineEditor")
	llvm_link("LLVMLinker")
	llvm_link("LLVMLTO")
	llvm_link("LLVMMC")
	-- llvm_link("LLVMMCA")
	llvm_link("LLVMMCDisassembler")
	-- llvm_link("LLVMMCJIT")
	llvm_link("LLVMMCParser")
	-- llvm_link("LLVMMIRParser")
	llvm_link("LLVMObjCARCOpts")
	-- llvm_link("LLVMObjCopy")
	llvm_link("LLVMObject")
	llvm_link("LLVMObjectYAML")
	-- llvm_link("LLVMOptDriver")
	llvm_link("LLVMOption")
	-- llvm_link("LLVMOrcDebugging")
	llvm_link("LLVMOrcJIT")
	llvm_link("LLVMOrcShared")
	llvm_link("LLVMOrcTargetProcess")
	llvm_link("LLVMPasses")
	llvm_link("LLVMProfileData")
	llvm_link("LLVMRemarks")
	-- RISCV
	llvm_link("LLVMRuntimeDyld")
	llvm_link("LLVMSandboxIR")
	llvm_link("LLVMScalarOpts")
	llvm_link("LLVMSelectionDAG")
	llvm_link("LLVMSupport")
	-- llvm_link("LLVMSymbolize")
	-- llvm_link("LLVMTableGen")
	-- llvm_link("LLVMTableGenBasic")
	-- llvm_link("LLVMTableGenCommon")
	llvm_link("LLVMTarget")
	llvm_link("LLVMTargetParser")
	-- llvm_link("LLVMTelemetry")
	llvm_link("LLVMTextAPI")
	-- llvm_link("LLVMTextAPIBinaryReader")
	llvm_link("LLVMTransformUtils")
	llvm_link("LLVMVectorize")
	-- WebAssembly
	llvm_link("LLVMWindowsDriver")
	llvm_link("LLVMWindowsManifest")
	-- x86
	-- llvm_link("LLVMXRay")




	-- llvm_link_X86()
	llvm_link_all_platforms()


	filter "system:Windows"
		links{
			"ntdll"
		}
	filter {}



project "*"


