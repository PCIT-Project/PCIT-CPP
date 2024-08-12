//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include "./class_impls/native_ptr_decls.h"

namespace pcit::llvmint{

	enum class LinkageType{
		External = 0,
		AvailableExternally,
		LinkOnceAny,
		LinkOnceODR,
		WeakAny,
		WeakODR,
		Appending,
		Internal,
		Private,
		ExternalWeak,
		Common,
	};
	

	// https://llvm.org/doxygen/namespacellvm_1_1CallingConv.html
	// #a93deac163dfd0548fe0a161bb95b2603afd841a49aec1539bc88abc8ff9e170fb
	enum class CallingConv{
		C = 0,
		Fast = 8,
		Cold = 9,
		GHC = 10,
		HiPE = 11,
		AnyReg = 13,
		PreserveMost = 14,
		PreserveAll = 15,
		Swift = 16,
		CXX_FAST_TLS = 17,
		Tail = 18,
		CFGuard_Check = 19,
		SwiftTail = 20,
		PreserveNone = 21,
		FirstTargetCC = 64,
		X86_StdCall = 64,
		X86_FastCall = 65,
		ARM_APCS = 66,
		ARM_AAPCS = 67,
		ARM_AAPCS_VFP = 68,
		MSP430_INTR = 69,
		X86_ThisCall = 70,
		PTX_Kernel = 71,
		PTX_Device = 72,
		SPIR_FUNC = 75,
		SPIR_KERNEL = 76,
		Intel_OCL_BI = 77,
		X86_64_SysV = 78,
		Win64 = 79,
		X86_VectorCall = 80,
		DUMMY_HHVM = 81,
		DUMMY_HHVM_C = 82,
		X86_INTR = 83,
		AVR_INTR = 84,
		AVR_SIGNAL = 85,
		AVR_BUILTIN = 86,
		AMDGPU_VS = 87,
		AMDGPU_GS = 88,
		AMDGPU_PS = 89,
		AMDGPU_CS = 90,
		AMDGPU_KERNEL = 91,
		X86_RegCall = 92,
		AMDGPU_HS = 93,
		MSP430_BUILTIN = 94,
		AMDGPU_LS = 95,
		AMDGPU_ES = 96,
		AArch64_VectorCall = 97,
		AArch64_SVE_VectorCall = 98,
		WASM_EmscriptenInvoke = 99,
		AMDGPU_Gfx = 100,
		M68k_INTR = 101,
		AArch64_SME_ABI_Support_Routines_PreserveMost_From_X0 = 102,
		AArch64_SME_ABI_Support_Routines_PreserveMost_From_X2 = 103,
		AMDGPU_CS_Chain = 104,
		AMDGPU_CS_ChainPreserve = 105,
		M68k_RTD = 106,
		GRAAL = 107,
		ARM64EC_Thunk_X64 = 108,
		ARM64EC_Thunk_Native = 109,
		RISCV_VectorCall = 110,
		AArch64_SME_ABI_Support_Routines_PreserveMost_From_X1 = 111,

		MaxID = 1023,
	};

	
}