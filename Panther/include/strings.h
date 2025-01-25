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

namespace pcit::panther::strings{

	enum class StringCode : uint32_t {
		Num,   // num
		LHS,   // lhs
		RHS,   // rhs
		Value, // value
	};

	EVO_NODISCARD constexpr auto toStringView(StringCode str) -> std::string_view {
		switch(str){
			case StringCode::Num:   return "num";
			case StringCode::LHS:   return "lhs";
			case StringCode::RHS:   return "rhs";
			case StringCode::Value: return "value";
		}

		evo::debugFatalBreak("Unknown string code");
	}


}