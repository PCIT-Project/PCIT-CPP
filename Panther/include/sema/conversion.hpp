////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>
#include <PCIT_core.hpp>


#include "./Expr.hpp"


namespace pcit::panther{
	
	class Context;

}


namespace pcit::panther::sema{


	[[nodiscard]] auto exprToGenericValue(Expr expr, const class panther::Context& context) -> core::GenericValue;

	[[nodiscard]] auto extractStringFromExpr(Expr expr, const class panther::Context& context) -> std::string_view;


}
