////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>
#include <PCIT_core.h>

#include "./type_ids.h"

namespace pcit::panther{

	
	namespace IntrinsicFunc{
		enum class Kind {
			// operational
			ABORT,
			BREAKPOINT,
			PANIC,

			// build system
			BUILD_SET_NUM_THREADS,
			BUILD_SET_OUTPUT,
			BUILD_SET_STD_LIB_PACKAGE,
			BUILD_CREATE_PACKAGE,
			BUILD_ADD_SOURCE_FILE,
			BUILD_ADD_SOURCE_DIRECTORY,
			BUILD_ADD_C_HEADER_FILE,
			BUILD_ADD_CPP_HEADER_FILE,

			_LAST_ = BUILD_ADD_CPP_HEADER_FILE,
		};

		EVO_NODISCARD auto lookupKind(std::string_view name) -> std::optional<Kind>;
		auto initLookupTableIfNeeded() -> void;
	};



	namespace TemplateIntrinsicFunc{
		enum class Kind{
			// type traits
			GET_TYPE_ID,
			ARRAY_ELEMENT_TYPE_ID,
			ARRAY_REF_ELEMENT_TYPE_ID,
			NUM_BYTES,
			NUM_BITS,
			IS_DEFAULT_INITIALIZABLE,
			IS_TRIVIALLY_DEFAULT_INITIALIZABLE,
			IS_COMPTIME_DEFAULT_INITIALIZABLE,
			IS_NO_ERROR_DEFAULT_INITIALIZABLE,
			IS_SAFE_DEFAULT_INITIALIZABLE,
			IS_TRIVIALLY_DELETABLE,
			IS_COMPTIME_DELETABLE,
			IS_COPYABLE,
			IS_TRIVIALLY_COPYABLE,
			IS_COMPTIME_COPYABLE,
			IS_NO_ERROR_COPYABLE,
			IS_SAFE_COPYABLE,
			IS_MOVABLE,
			IS_TRIVIALLY_MOVABLE,
			IS_COMPTIME_MOVABLE,
			IS_NO_ERROR_MOVABLE,
			IS_SAFE_MOVABLE,
			IS_COMPARABLE,
			IS_TRIVIALLY_COMPARABLE,
			IS_COMPTIME_COMPARABLE,
			IS_NO_ERROR_COMPARABLE,
			IS_SAFE_COMPARABLE,

			// type conversion
			BIT_CAST,
			TRUNC,
			FTRUNC,
			SEXT,
			ZEXT,
			FEXT,
			I_TO_F,
			F_TO_I,

			// arithmetic
			ADD,
			ADD_WRAP,
			ADD_SAT,
			FADD,
			SUB,
			SUB_WRAP,
			SUB_SAT,
			FSUB,
			MUL,
			MUL_WRAP,
			MUL_SAT,
			FMUL,
			DIV,
			FDIV,
			REM,
			FNEG,

			// comparison
			EQ,
			NEQ,
			LT,
			LTE,
			GT,
			GTE,

			// bitwise
			AND,
			OR,
			XOR,
			SHL,
			SHL_SAT,
			SHR,
			BIT_REVERSE,
			BYTE_SWAP,
			CTPOP,
			CTLZ,
			CTTZ,

			// atomics
			ATOMIC_LOAD,
			ATOMIC_STORE,
			CMPXCHG,
			ATOMIC_RMW,

			_LAST_ = ATOMIC_RMW,
		};
		

		EVO_NODISCARD auto lookupKind(std::string_view name) -> std::optional<Kind>;
		auto initLookupTableIfNeeded() -> void;


		enum class AtomicOrdering : uint32_t {
			MONOTONIC,
			ACQUIRE,
			RELEASE,
			ACQ_REL,
			SEQ_CST,

			// _LAST_ = SEQ_CST,
		};


		enum class AtomicRMWOp : uint32_t {
			XCHG,
			ADD,
			SUB,
			AND,
			NAND,
			OR,
			XOR,
			MIN,
			MAX,

			_LAST_ = MAX,
		};

	}


}