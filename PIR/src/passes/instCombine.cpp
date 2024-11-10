//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../../include/passes/instCombine.h"

#include "../../include/Type.h"
#include "../../include/Expr.h"
#include "../../include/BasicBlock.h"
#include "../../include/Function.h"
#include "../../include/Module.h"
#include "../../include/Agent.h"

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir::passes{
	

	auto constant_folding_impl(Expr& stmt, Agent& agent) -> bool {
		switch(stmt.getKind()){
			case Expr::Kind::None:         evo::debugFatalBreak("Not valid expr");
			case Expr::Kind::GlobalValue:  return true;
			case Expr::Kind::Number:       return true;
			case Expr::Kind::ParamExpr:    return true;
			case Expr::Kind::CallInst:     return true;
			case Expr::Kind::CallVoidInst: return true;
			case Expr::Kind::RetInst:      return true;
			case Expr::Kind::BrInst:       return true;

			case Expr::Kind::Add: {
				const Add& add = agent.getAdd(stmt);
				if(add.lhs.getKind() != Expr::Kind::Number || add.rhs.getKind() != Expr::Kind::Number){ return true; }
				const Number& lhs = agent.getNumber(add.lhs);
				const Number& rhs = agent.getNumber(add.rhs);

				if(lhs.type.isIntegral()){
					core::GenericInt::WrapResult result = lhs.type.getKind() == Type::Kind::Unsigned
						? lhs.getInt().uadd(rhs.getInt())
						: lhs.getInt().sadd(rhs.getInt());

					if(result.wrapped){ return false; }

					const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
					agent.replaceExpr(stmt, result_expr);

				}else{
					const Expr result_expr = agent.createNumber(lhs.type, lhs.getFloat().add(rhs.getFloat()));
					agent.replaceExpr(stmt, result_expr);
				}

				return true;
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto inst_simplify_impl(Expr& stmt, Agent& agent) -> bool {
		switch(stmt.getKind()){
			case Expr::Kind::None:         evo::debugFatalBreak("Not valid expr");
			case Expr::Kind::GlobalValue:  return true;
			case Expr::Kind::Number:       return true;
			case Expr::Kind::ParamExpr:    return true;
			case Expr::Kind::CallInst:     return true;
			case Expr::Kind::CallVoidInst: return true;
			case Expr::Kind::RetInst:      return true;
			case Expr::Kind::BrInst:       return true;

			case Expr::Kind::Add: {
				const Add& add = agent.getAdd(stmt);

				if(add.lhs.getKind() == Expr::Kind::Number){
					const Number& lhs = agent.getNumber(add.lhs);
					if(lhs.type.isIntegral()){
						const core::GenericInt& number = lhs.getInt();
						if(number == core::GenericInt(number.getBitWidth(), 0)){
							agent.replaceExpr(stmt, add.rhs);
						}
					}

				}else if(add.rhs.getKind() == Expr::Kind::Number){
					const Number& rhs = agent.getNumber(add.rhs);
					if(rhs.type.isIntegral()){
						const core::GenericInt& number = rhs.getInt();
						if(number == core::GenericInt(number.getBitWidth(), 0)){
							agent.replaceExpr(stmt, add.lhs);
						}
					}
				}

				return true;
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto inst_combine_impl(Expr&, Agent&) -> bool {
		return true;
	}


}