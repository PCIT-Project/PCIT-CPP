//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>
#include <PCIT_core.h>

namespace pcit::panther::strings{

	enum class StringCode{
		Num, // num
		LHS, // lhs
		RHS, // rhs
	};

	EVO_NODISCARD constexpr auto toStringView(StringCode str) -> std::string_view {
		switch(str){
			case StringCode::Num: return "num";
			case StringCode::LHS: return "lhs";
			case StringCode::RHS: return "rhs";
		}

		evo::debugFatalBreak("Unknown string code");
	}


}