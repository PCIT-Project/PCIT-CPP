//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/Module.h"

#include "../include/ReaderAgent.h"

namespace pcit::pir{
	

	auto Module::getExprType(const Expr& expr) const -> Type {
		evo::debugAssert(
			expr.isConstant(),
			"Module can only get value of expr that is a constant. Use Function::getExprType() instead "
			"(where Function is the function the expr is from"
		);

		switch(expr.getKind()){
			case Expr::Kind::GlobalValue: return this->createTypePtr();
			case Expr::Kind::Number:      return ReaderAgent(*this).getNumber(expr).type;
		}

		evo::debugFatalBreak("Unknown or unsupported constant expr kind");
	}


}