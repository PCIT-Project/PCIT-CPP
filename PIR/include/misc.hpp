////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>


namespace pcit::pir{


	[[nodiscard]] inline auto isStandardName(std::string_view name) -> bool {
		if(name.empty()){ return false; }

		for(char c : name){
			if(!evo::isAlphaNumeric(c) && c != '.' && c != '_' && c != '-'){ return false; }
		}

		return true;
	}


	[[nodiscard]] inline auto isStandardExprName(std::string_view name) -> bool {
		if(name.empty()){ return false; }

		for(char c : name){
			if(!evo::isAlphaNumeric(c) && c != '.' && c != '_'){ return false; }
		}

		return true;
	}


}


