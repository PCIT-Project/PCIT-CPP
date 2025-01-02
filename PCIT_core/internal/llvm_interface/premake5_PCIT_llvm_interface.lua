-- premake5

project "PCIT_llvm_interface"
	kind "StaticLib"


	
	targetdir(target.lib)
	objdir(target.obj)

	files {
		"./src/**.cpp",
	}

	
	includedirs{
		(config.location .. "/dependencies/LLVM_build/include"),
		(config.location .. "/dependencies"),
	}



	LLVM.link.X86()

	links{
		"Evo",

		-- platform: AArch64
		LLVM.libs.AggressiveInstCombine,
		-- platform: AMDGPU
		LLVM.libs.Analysis,
		-- platform: ARM
		LLVM.libs.AsmParser,
		LLVM.libs.AsmPrinter,
		-- platform: AVR
		LLVM.libs.BinaryFormat,
		LLVM.libs.BitReader,
		LLVM.libs.BitstreamReader,
		LLVM.libs.BitWriter,
		-- platform: BPF
		LLVM.libs.CFGuard,
		LLVM.libs.CFIVerify,
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
		-- platform: Exegesis
		LLVM.libs.Extensions,
		LLVM.libs.FileCheck,
		LLVM.libs.FrontendDriver,
		LLVM.libs.FrontendHLSL,
		LLVM.libs.FrontendOffloading,
		LLVM.libs.FrontendOpenACC,
		LLVM.libs.FrontendOpenMP,
		LLVM.libs.FuzzerCLI,
		LLVM.libs.FuzzMutate,
		LLVM.libs.GlobalISel,
		-- platform: Hexagon
		LLVM.libs.HipStdPar,
		LLVM.libs.InstCombine,
		LLVM.libs.Instrumentation,
		LLVM.libs.InterfaceStub,
		LLVM.libs.Interpreter,
		LLVM.libs.ipo,
		LLVM.libs.IRPrinter,
		LLVM.libs.IRReader,
		LLVM.libs.JITLink,
		-- platform: Lanai
		LLVM.libs.LibDriver,
		LLVM.libs.LineEditor,
		LLVM.libs.Linker,
		-- platform: LoongArch
		LLVM.libs.LTO,
		LLVM.libs.MC,
		LLVM.libs.MCA,
		LLVM.libs.MCDisassembler,
		LLVM.libs.MCJIT,
		LLVM.libs.MCParser,
		-- platform: Mips
		LLVM.libs.MIRParser,
		-- platform: MSP430
		-- platform: NVPTX
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
		-- platform: PowerPC
		LLVM.libs.ProfileData,
		LLVM.libs.Remarks,
		-- platform: RISCV
		LLVM.libs.RuntimeDyld,
		LLVM.libs.ScalarOpts,
		LLVM.libs.SelectionDAG,
		-- platform: Sparc
		LLVM.libs.Support,
		LLVM.libs.Symbolize,
		-- platform: SystemZ
		LLVM.libs.TableGen,
		LLVM.libs.TableGenCommon,
		LLVM.libs.TableGenGlobalISel,
		LLVM.libs.Target,
		LLVM.libs.TargetParser,
		LLVM.libs.TextAPI,
		LLVM.libs.TextAPIBinaryReader,
		LLVM.libs.TransformUtils,
		LLVM.libs.VEAsmParser,
		LLVM.libs.VECodeGen,
		LLVM.libs.Vectorize,
		LLVM.libs.VEDesc,
		LLVM.libs.VEDisassembler,
		LLVM.libs.VEInfo,
		-- platform: WebAssembly
		LLVM.libs.WindowsDriver,
		LLVM.libs.WindowsManifest,
		-- platform: x86
		LLVM.libs.X86TargetMCA,
		-- platform: XCore
		LLVM.libs.XRay,
	}

	LLVM.link.X86();


project "*"


