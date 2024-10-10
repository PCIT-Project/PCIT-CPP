//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


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