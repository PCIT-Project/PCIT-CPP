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
	

	auto constant_folding_impl(Expr stmt, const Agent& agent) -> PassManager::MadeTransformation {
		switch(stmt.getKind()){
			case Expr::Kind::None:        evo::debugFatalBreak("Not valid expr");
			case Expr::Kind::GlobalValue: return false;
			case Expr::Kind::Number:      return false;
			case Expr::Kind::Boolean:     return false;
			case Expr::Kind::ParamExpr:   return false;
			case Expr::Kind::Call:        return false;
			case Expr::Kind::CallVoid:    return false;
			case Expr::Kind::Breakpoint:  return false;
			case Expr::Kind::Ret:         return false;
			case Expr::Kind::Branch:      return false;
			case Expr::Kind::CondBranch:  return false;
			case Expr::Kind::Unreachable: return false;
			case Expr::Kind::Alloca:      return false;
			case Expr::Kind::Load:        return false;
			case Expr::Kind::Store:       return false;
			case Expr::Kind::CalcPtr:     return false;
			case Expr::Kind::BitCast:     return false;
			case Expr::Kind::Trunc:       return false;
			case Expr::Kind::FTrunc:      return false;
			case Expr::Kind::SExt:        return false;
			case Expr::Kind::ZExt:        return false;
			case Expr::Kind::FExt:        return false;
			case Expr::Kind::IToF:        return false;
			case Expr::Kind::UIToF:       return false;
			case Expr::Kind::FToI:        return false;
			case Expr::Kind::FToUI:       return false;

			case Expr::Kind::Add: {
				const Add& add = agent.getAdd(stmt);
				if(add.lhs.getKind() != Expr::Kind::Number || add.rhs.getKind() != Expr::Kind::Number){ return false; }
				const Number& lhs = agent.getNumber(add.lhs);
				const Number& rhs = agent.getNumber(add.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().sadd(rhs.getInt());
				const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FAdd: {
				const FAdd& fadd = agent.getFAdd(stmt);
				if(fadd.lhs.getKind() != Expr::Kind::Number || fadd.rhs.getKind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(fadd.lhs);
				const Number& rhs = agent.getNumber(fadd.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getFloat().add(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SAddWrap: {
				const SAddWrap& sadd_wrap = agent.getSAddWrap(stmt);
				if(sadd_wrap.lhs.getKind() != Expr::Kind::Number || sadd_wrap.rhs.getKind() != Expr::Kind::Number){
					return false;
				}

				const Expr result_original_expr = agent.extractSAddWrapResult(stmt);
				const Expr wrapped_original_expr = agent.extractSAddWrapWrapped(stmt);

				const Number& lhs = agent.getNumber(sadd_wrap.lhs);
				const Number& rhs = agent.getNumber(sadd_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().sadd(rhs.getInt());

				const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
				agent.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = agent.createBoolean(result.wrapped);
				agent.replaceExpr(wrapped_original_expr, wrapped_expr);

				agent.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::SAddWrapResult:  return false;
			case Expr::Kind::SAddWrapWrapped: return false;

			case Expr::Kind::UAddWrap: {
				const UAddWrap& uadd_wrap = agent.getUAddWrap(stmt);
				if(uadd_wrap.lhs.getKind() != Expr::Kind::Number || uadd_wrap.rhs.getKind() != Expr::Kind::Number){
					return false;
				}

				const Expr result_original_expr = agent.extractUAddWrapResult(stmt);
				const Expr wrapped_original_expr = agent.extractUAddWrapWrapped(stmt);

				const Number& lhs = agent.getNumber(uadd_wrap.lhs);
				const Number& rhs = agent.getNumber(uadd_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().uadd(rhs.getInt());

				const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
				agent.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = agent.createBoolean(result.wrapped);
				agent.replaceExpr(wrapped_original_expr, wrapped_expr);

				agent.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::UAddWrapResult:  return false;
			case Expr::Kind::UAddWrapWrapped: return false;
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto inst_simplify_impl(Expr stmt, const Agent& agent) -> PassManager::MadeTransformation {
		switch(stmt.getKind()){
			case Expr::Kind::None:        evo::debugFatalBreak("Not valid expr");
			case Expr::Kind::GlobalValue: return false;
			case Expr::Kind::Number:      return false;
			case Expr::Kind::Boolean:     return false;
			case Expr::Kind::ParamExpr:   return false;
			case Expr::Kind::Call:        return false;
			case Expr::Kind::CallVoid:    return false;
			case Expr::Kind::Breakpoint:  return false;
			case Expr::Kind::Ret:         return false;
			case Expr::Kind::Branch:      return false;

			case Expr::Kind::CondBranch: {
				const CondBranch& cond_branch = agent.getCondBranch(stmt);

				if(cond_branch.cond.getKind() != Expr::Kind::Boolean){ return false; }

				if(agent.getBoolean(cond_branch.cond)){
					const BasicBlock::ID then_block = cond_branch.thenBlock;
					agent.removeStmt(stmt);
					agent.createBranch(then_block);
				}else{
					const BasicBlock::ID else_block = cond_branch.elseBlock;
					agent.removeStmt(stmt);
					agent.createBranch(else_block);
				}

				return true;
			} break;

			case Expr::Kind::Unreachable: return false;
			case Expr::Kind::Alloca:      return false;
			case Expr::Kind::Load:        return false;
			case Expr::Kind::Store:       return false;

			case Expr::Kind::CalcPtr: {
				const CalcPtr& calc_ptr = agent.getCalcPtr(stmt);

				for(const CalcPtr::Index& index : calc_ptr.indices){
					if(index.is<int64_t>()){
						if(index.as<int64_t>() != 0){ return false; }
					}else{
						const Expr& expr = index.as<Expr>();

						if(expr.getKind() != Expr::Kind::Number){ return false; }
						if(static_cast<uint64_t>(agent.getNumber(expr).getInt()) != 0){ return false; }
					}
				}

				agent.replaceExpr(stmt, calc_ptr.basePtr);
				return true;
			} break;

			case Expr::Kind::BitCast: {
				const BitCast& bitcast = agent.getBitCast(stmt);
				
				if(agent.getExprType(bitcast.fromValue) == bitcast.toType){
					agent.replaceExpr(stmt, bitcast.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::Trunc: {
				const Trunc& trunc = agent.getTrunc(stmt);
				
				if(agent.getExprType(trunc.fromValue) == trunc.toType){
					agent.replaceExpr(stmt, trunc.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::FTrunc: {
				const FTrunc& ftrunc = agent.getFTrunc(stmt);
				
				if(agent.getExprType(ftrunc.fromValue) == ftrunc.toType){
					agent.replaceExpr(stmt, ftrunc.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::SExt: {
				const SExt& sext = agent.getSExt(stmt);
				
				if(agent.getExprType(sext.fromValue) == sext.toType){
					agent.replaceExpr(stmt, sext.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::ZExt: {
				const ZExt& zext = agent.getZExt(stmt);
				
				if(agent.getExprType(zext.fromValue) == zext.toType){
					agent.replaceExpr(stmt, zext.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::FExt: {
				const FExt& fext = agent.getFExt(stmt);
				
				if(agent.getExprType(fext.fromValue) == fext.toType){
					agent.replaceExpr(stmt, fext.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::IToF: {
				const IToF& itof = agent.getIToF(stmt);
				
				if(agent.getExprType(itof.fromValue) == itof.toType){
					agent.replaceExpr(stmt, itof.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::UIToF: {
				const UIToF& uitof = agent.getUIToF(stmt);
				
				if(agent.getExprType(uitof.fromValue) == uitof.toType){
					agent.replaceExpr(stmt, uitof.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::FToI: {
				const FToI& ftoi = agent.getFToI(stmt);
				
				if(agent.getExprType(ftoi.fromValue) == ftoi.toType){
					agent.replaceExpr(stmt, ftoi.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::FToUI: {
				const FToUI& ftoui = agent.getFToUI(stmt);
				
				if(agent.getExprType(ftoui.fromValue) == ftoui.toType){
					agent.replaceExpr(stmt, ftoui.fromValue);
					return true;
				}
			} break;


			case Expr::Kind::Add: {
				const Add& add = agent.getAdd(stmt);

				if(add.lhs.getKind() == Expr::Kind::Number){
					const Number& lhs = agent.getNumber(add.lhs);
					const core::GenericInt& lhs_number = lhs.getInt();
					if(lhs_number == core::GenericInt(lhs_number.getBitWidth(), 0)){
						agent.replaceExpr(stmt, add.rhs);
						return true;
					}

				}else if(add.rhs.getKind() == Expr::Kind::Number){
					const Number& rhs = agent.getNumber(add.rhs);
					const core::GenericInt& rhs_number = rhs.getInt();
					if(rhs_number == core::GenericInt(rhs_number.getBitWidth(), 0)){
						agent.replaceExpr(stmt, add.lhs);
						return true;
					}
				}

				return false;
			} break;

			case Expr::Kind::FAdd: return true;

			case Expr::Kind::SAddWrap: {
				const SAddWrap& sadd_wrap = agent.getSAddWrap(stmt);

				if(sadd_wrap.lhs.getKind() == Expr::Kind::Number){
					const Number& lhs = agent.getNumber(sadd_wrap.lhs);
					const core::GenericInt& number = lhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						agent.replaceExpr(agent.extractSAddWrapResult(stmt), sadd_wrap.rhs);
						agent.replaceExpr(agent.extractSAddWrapWrapped(stmt), agent.createBoolean(false));
						agent.removeStmt(stmt);
						return true;
					}

				}else if(sadd_wrap.rhs.getKind() == Expr::Kind::Number){
					const Number& rhs = agent.getNumber(sadd_wrap.rhs);
					const core::GenericInt& number = rhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						agent.replaceExpr(agent.extractSAddWrapResult(stmt), sadd_wrap.lhs);
						agent.replaceExpr(agent.extractSAddWrapWrapped(stmt), agent.createBoolean(false));
						agent.removeStmt(stmt);
						return true;
					}
				}

				return false;
			} break;

			case Expr::Kind::SAddWrapResult:  return false;
			case Expr::Kind::SAddWrapWrapped: return false;

			case Expr::Kind::UAddWrap: {
				const UAddWrap& uadd_wrap = agent.getUAddWrap(stmt);

				if(uadd_wrap.lhs.getKind() == Expr::Kind::Number){
					const Number& lhs = agent.getNumber(uadd_wrap.lhs);
					const core::GenericInt& number = lhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						agent.replaceExpr(agent.extractUAddWrapResult(stmt), uadd_wrap.rhs);
						agent.replaceExpr(agent.extractUAddWrapWrapped(stmt), agent.createBoolean(false));
						agent.removeStmt(stmt);
						return true;
					}

				}else if(uadd_wrap.rhs.getKind() == Expr::Kind::Number){
					const Number& rhs = agent.getNumber(uadd_wrap.rhs);
					const core::GenericInt& number = rhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						agent.replaceExpr(agent.extractUAddWrapResult(stmt), uadd_wrap.lhs);
						agent.replaceExpr(agent.extractUAddWrapWrapped(stmt), agent.createBoolean(false));
						agent.removeStmt(stmt);
						return true;
					}
				}

				return false;
			} break;

			case Expr::Kind::UAddWrapResult:  return false;
			case Expr::Kind::UAddWrapWrapped: return false;
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto inst_combine_impl(Expr, const Agent&) -> PassManager::MadeTransformation {
		return false;
	}


}