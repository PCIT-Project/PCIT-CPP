////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


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
	

	auto constant_folding_impl(Expr stmt, const Agent& agent) -> bool {
		switch(stmt.getKind()){
			case Expr::Kind::None:        evo::debugFatalBreak("Not valid expr");
			case Expr::Kind::GlobalValue: return false;
			case Expr::Kind::Number:      return false;
			case Expr::Kind::Boolean:     return false;
			case Expr::Kind::ParamExpr:   return false;
			case Expr::Kind::Call:        return false;
			case Expr::Kind::CallVoid:    return false;
			case Expr::Kind::Ret:         return false;
			case Expr::Kind::Branch:      return false;
			case Expr::Kind::Alloca:      return false;
			case Expr::Kind::Load:        return false;
			case Expr::Kind::Store:       return false;


			case Expr::Kind::Add: {
				const Add& add = agent.getAdd(stmt);
				if(add.lhs.getKind() != Expr::Kind::Number || add.rhs.getKind() != Expr::Kind::Number){ return false; }
				const Number& lhs = agent.getNumber(add.lhs);
				const Number& rhs = agent.getNumber(add.rhs);

				if(lhs.type.isIntegral()){
					core::GenericInt::WrapResult result = lhs.type.getKind() == Type::Kind::Unsigned
						? lhs.getInt().uadd(rhs.getInt())
						: lhs.getInt().sadd(rhs.getInt());

					if(result.wrapped){ return false; }

					const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
					agent.replaceExpr(stmt, result_expr);
					return true;

				}else{
					const Expr result_expr = agent.createNumber(lhs.type, lhs.getFloat().add(rhs.getFloat()));
					agent.replaceExpr(stmt, result_expr);
					return true;
				}
			} break;

			case Expr::Kind::AddWrap: {
				const AddWrap& add_wrap = agent.getAddWrap(stmt);
				if(add_wrap.lhs.getKind() != Expr::Kind::Number || add_wrap.rhs.getKind() != Expr::Kind::Number){
					return false;
				}

				const Expr result_original_expr = agent.extractAddWrapResult(stmt);
				const Expr wrapped_original_expr = agent.extractAddWrapWrapped(stmt);

				const Number& lhs = agent.getNumber(add_wrap.lhs);
				const Number& rhs = agent.getNumber(add_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.type.getKind() == Type::Kind::Unsigned
					? lhs.getInt().uadd(rhs.getInt())
					: lhs.getInt().sadd(rhs.getInt());

				const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
				agent.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = agent.createBoolean(result.wrapped);
				agent.replaceExpr(wrapped_original_expr, wrapped_expr);

				agent.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::AddWrapResult:  return false;
			case Expr::Kind::AddWrapWrapped: return false;
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto inst_simplify_impl(Expr stmt, const Agent& agent) -> bool {
		switch(stmt.getKind()){
			case Expr::Kind::None:        evo::debugFatalBreak("Not valid expr");
			case Expr::Kind::GlobalValue: return false;
			case Expr::Kind::Number:      return false;
			case Expr::Kind::Boolean:     return false;
			case Expr::Kind::ParamExpr:   return false;
			case Expr::Kind::Call:        return false;
			case Expr::Kind::CallVoid:    return false;
			case Expr::Kind::Ret:         return false;
			case Expr::Kind::Branch:      return false;
			case Expr::Kind::Alloca:      return false;
			case Expr::Kind::Load:        return false;
			case Expr::Kind::Store:       return false;

			case Expr::Kind::Add: {
				const Add& add = agent.getAdd(stmt);

				if(add.lhs.getKind() == Expr::Kind::Number){
					const Number& lhs = agent.getNumber(add.lhs);
					if(lhs.type.isIntegral()){
						const core::GenericInt& number = lhs.getInt();
						if(number == core::GenericInt(number.getBitWidth(), 0)){
							agent.replaceExpr(stmt, add.rhs);
							return true;
						}
					}

				}else if(add.rhs.getKind() == Expr::Kind::Number){
					const Number& rhs = agent.getNumber(add.rhs);
					if(rhs.type.isIntegral()){
						const core::GenericInt& number = rhs.getInt();
						if(number == core::GenericInt(number.getBitWidth(), 0)){
							agent.replaceExpr(stmt, add.lhs);
							return true;
						}
					}
				}

				return false;
			} break;

			case Expr::Kind::AddWrap: {
				const AddWrap& add_wrap = agent.getAddWrap(stmt);

				if(add_wrap.lhs.getKind() == Expr::Kind::Number){
					const Number& lhs = agent.getNumber(add_wrap.lhs);
					const core::GenericInt& number = lhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						agent.replaceExpr(agent.extractAddWrapResult(stmt), add_wrap.rhs);
						agent.replaceExpr(agent.extractAddWrapWrapped(stmt), agent.createBoolean(false));
						agent.removeStmt(stmt);
						return true;
					}

				}else if(add_wrap.rhs.getKind() == Expr::Kind::Number){
					const Number& rhs = agent.getNumber(add_wrap.rhs);
					const core::GenericInt& number = rhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						agent.replaceExpr(agent.extractAddWrapResult(stmt), add_wrap.lhs);
						agent.replaceExpr(agent.extractAddWrapWrapped(stmt), agent.createBoolean(false));
						agent.removeStmt(stmt);
						return true;
					}
				}

				return false;
			} break;

			case Expr::Kind::AddWrapResult:  return false;
			case Expr::Kind::AddWrapWrapped: return false;
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto inst_combine_impl(Expr, const Agent&) -> bool {
		return false;
	}


}