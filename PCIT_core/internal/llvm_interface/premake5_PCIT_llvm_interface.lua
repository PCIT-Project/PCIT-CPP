-- premake5

project "PCIT_llvm_interface"
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

		-- AArch64
		LLVM.libs.AggressiveInstCombine,
		-- AMDGPU
		LLVM.libs.Analysis,
		-- ARM
		LLVM.libs.AsmParser,
		LLVM.libs.AsmPrinter,
		-- AVR
		LLVM.libs.BinaryFormat,
		LLVM.libs.BitReader,
		LLVM.libs.BitstreamReader,
		LLVM.libs.BitWriter,
		-- BFP
		LLVM.libs.CFGuard,
		LLVM.libs.CFIVerify,
		LLVM.libs.CGData,
		LLVM.libs.CodeGen,
		LLVM.libs.CodeGenTypes,
		LLVM.libs.Core,
		LLVM.libs.Coroutines,
		LLVM.libs.Coverage,
		LLVM.libs.DebugInfoBTF,
		LLVM.libs.DebugInfoCodeView,
		LLVM.libs.Debuginfod,
		LLVM.libs.DebugInfoDWARF,
		LLVM.libs.DebugInfoGSYM,
		LLVM.libs.DebugInfoLogicalView,
		LLVM.libs.DebugInfoMSF,
		LLVM.libs.DebugInfoPDB,
		LLVM.libs.Demangle,
		LLVM.libs.Diff,
		LLVM.libs.DlltoolDriver,
		LLVM.libs.DWARFLinker,
		LLVM.libs.DWARFLinkerClassic,
		LLVM.libs.DWARFLinkerParallel,
		LLVM.libs.DWP,
		LLVM.libs.ExecutionEngine,
		-- Exegesis
		LLVM.libs.Extensions,
		LLVM.libs.FileCheck,
		LLVM.libs.FrontendAtomic,
		LLVM.libs.FrontendDriver,
		LLVM.libs.FrontendHLSL,
		LLVM.libs.FrontendOffloading,
		LLVM.libs.FrontendOpenACC,
		LLVM.libs.FrontendOpenMP,
		LLVM.libs.FuzzerCLI,
		LLVM.libs.FuzzMutate,
		LLVM.libs.GlobalISel,
		-- Hexagon
		LLVM.libs.HipStdPar,
		LLVM.libs.InstCombine,
		LLVM.libs.Instrumentation,
		LLVM.libs.InterfaceStub,
		LLVM.libs.Interpreter,
		LLVM.libs.ipo,
		LLVM.libs.IRPrinter,
		LLVM.libs.IRReader,
		LLVM.libs.JITLink,
		-- Lanai
		LLVM.libs.LibDriver,
		LLVM.libs.LineEditor,
		LLVM.libs.Linker,
		-- LoongArch
		LLVM.libs.LTO,
		LLVM.libs.MC,
		LLVM.libs.MCA,
		LLVM.libs.MCDisassembler,
		LLVM.libs.MCJIT,
		LLVM.libs.MCParser,
		-- Mips
		LLVM.libs.MIRParser,
		-- MSP430
		-- NVPTX
		LLVM.libs.ObjCARCOpts,
		LLVM.libs.ObjCopy,
		LLVM.libs.Object,
		LLVM.libs.ObjectYAML,
		LLVM.libs.OptDriver,
		LLVM.libs.Option,
		LLVM.libs.OrcDebugging,
		LLVM.libs.OrcJIT,
		LLVM.libs.OrcShared,
		LLVM.libs.OrcTargetProcess,
		LLVM.libs.Passes,
		-- PowerPC
		LLVM.libs.ProfileData,
		LLVM.libs.Remarks,
		-- RISCV
		LLVM.libs.RuntimeDyld,
		LLVM.libs.SandboxIR,
		LLVM.libs.ScalarOpts,
		LLVM.libs.SelectionDAG,
		-- Sparc
		-- SPIRV
		LLVM.libs.Support,
		LLVM.libs.Symbolize,
		-- SystemZ
		LLVM.libs.TableGen,
		LLVM.libs.TableGenBasic,
		LLVM.libs.TableGenCommon,
		LLVM.libs.Target,
		LLVM.libs.TargetParser,
		LLVM.libs.Telemetry,
		LLVM.libs.TextAPI,
		LLVM.libs.TextAPIBinaryReader,
		LLVM.libs.TransformUtils,
		-- VE
		LLVM.libs.Vectorize,
		-- WebAssembly
		LLVM.libs.WindowsDriver,
		LLVM.libs.WindowsManifest,
		-- X86
		-- XCore
		LLVM.libs.XRay,
	}

	LLVM.link.X86()


	filter "system:Windows"
		links{
			"ntdll"
		}
	filter {}



project "*"


