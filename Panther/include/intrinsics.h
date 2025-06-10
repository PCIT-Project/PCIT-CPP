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

			// build system
			BUILD_SET_NUM_THREADS,
			BUILD_SET_OUTPUT,
			BUILD_SET_USE_STD_LIB,

			_MAX_,
		};

		EVO_NODISCARD auto lookupKind(std::string_view name) -> std::optional<Kind>;
		auto initLookupTableIfNeeded() -> void;
	};



	namespace TemplateIntrinsicFunc{
		enum class Kind{
			SIZE_OF,
			BIT_WIDTH,

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
			BSWAP,
			CTPOP,
			CTLZ,
			CTTZ,

			_MAX_,
		};
		

		EVO_NODISCARD auto lookupKind(std::string_view name) -> std::optional<Kind>;
		auto initLookupTableIfNeeded() -> void;
	}


}