////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/passes/instCombine.hpp"

#include "../../include/Type.hpp"
#include "../../include/Expr.hpp"
#include "../../include/BasicBlock.hpp"
#include "../../include/Function.hpp"
#include "../../include/Module.hpp"
#include "../../include/InstrHandler.hpp"

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir::passes{
	

	auto constant_folding_impl(Expr stmt, const InstrHandler& handler) -> PassManager::MadeTransformation {
		switch(stmt.kind()){
			case Expr::Kind::NONE:            evo::debugFatalBreak("Not valid expr");
			case Expr::Kind::GLOBAL_VALUE:     return false;
			case Expr::Kind::FUNCTION_POINTER: return false;
			case Expr::Kind::NUMBER:           return false;
			case Expr::Kind::BOOLEAN:          return false;
			case Expr::Kind::NULLPTR:          return false;
			case Expr::Kind::PARAM_EXPR:       return false;
			case Expr::Kind::CALL:             return false;
			case Expr::Kind::CALL_VOID:        return false;
			case Expr::Kind::CALL_NO_RETURN:   return false;
			case Expr::Kind::ABORT:            return false;
			case Expr::Kind::BREAKPOINT:       return false;
			case Expr::Kind::RET:              return false;
			case Expr::Kind::JUMP:             return false;
			case Expr::Kind::BRANCH:           return false;
			case Expr::Kind::UNREACHABLE:      return false;
			case Expr::Kind::PHI:              return false;
			case Expr::Kind::SWITCH:           return false;
			case Expr::Kind::ALLOCA:           return false;
			case Expr::Kind::LOAD:             return false;
			case Expr::Kind::STORE:            return false;
			case Expr::Kind::CALC_PTR:         return false;
			case Expr::Kind::MEMCPY:           return false;
			case Expr::Kind::MEMSET:           return false;
			case Expr::Kind::BIT_CAST:         return false;
			case Expr::Kind::TRUNC:            return false;
			case Expr::Kind::FTRUNC:           return false;
			case Expr::Kind::SEXT:             return false;
			case Expr::Kind::ZEXT:             return false;
			case Expr::Kind::FEXT:             return false;
			case Expr::Kind::ITOF:             return false;
			case Expr::Kind::UITOF:            return false;
			case Expr::Kind::FTOI:             return false;
			case Expr::Kind::FTOUI:            return false;

			case Expr::Kind::ADD: {
				const Add& add = handler.getAdd(stmt);
				if(add.lhs.kind() != Expr::Kind::NUMBER || add.rhs.kind() != Expr::Kind::NUMBER){ return false; }
				const Number& lhs = handler.getNumber(add.lhs);
				const Number& rhs = handler.getNumber(add.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().sadd(rhs.getInt());
				const Expr result_expr = handler.createNumber(lhs.type, std::move(result.result));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SADD_WRAP: {
				const SAddWrap& sadd_wrap = handler.getSAddWrap(stmt);
				if(sadd_wrap.lhs.kind() != Expr::Kind::NUMBER || sadd_wrap.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}

				const Expr result_original_expr = handler.extractSAddWrapResult(stmt);
				const Expr wrapped_original_expr = handler.extractSAddWrapWrapped(stmt);

				const Number& lhs = handler.getNumber(sadd_wrap.lhs);
				const Number& rhs = handler.getNumber(sadd_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().sadd(rhs.getInt());

				const Expr result_expr = handler.createNumber(lhs.type, std::move(result.result));
				handler.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = handler.createBoolean(result.wrapped);
				handler.replaceExpr(wrapped_original_expr, wrapped_expr);

				handler.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::SADD_WRAP_RESULT:  return false;
			case Expr::Kind::SADD_WRAP_WRAPPED: return false;

			case Expr::Kind::UADD_WRAP: {
				const UAddWrap& uadd_wrap = handler.getUAddWrap(stmt);
				if(uadd_wrap.lhs.kind() != Expr::Kind::NUMBER || uadd_wrap.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}

				const Expr result_original_expr = handler.extractUAddWrapResult(stmt);
				const Expr wrapped_original_expr = handler.extractUAddWrapWrapped(stmt);

				const Number& lhs = handler.getNumber(uadd_wrap.lhs);
				const Number& rhs = handler.getNumber(uadd_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().uadd(rhs.getInt());

				const Expr result_expr = handler.createNumber(lhs.type, std::move(result.result));
				handler.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = handler.createBoolean(result.wrapped);
				handler.replaceExpr(wrapped_original_expr, wrapped_expr);

				handler.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::UADD_WRAP_RESULT:  return false;
			case Expr::Kind::UADD_WRAP_WRAPPED: return false;

			case Expr::Kind::SADD_SAT: {
				const SAddSat& sadd_sat = handler.getSAddSat(stmt);
				if(sadd_sat.lhs.kind() != Expr::Kind::NUMBER || sadd_sat.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(sadd_sat.lhs);
				const Number& rhs = handler.getNumber(sadd_sat.rhs);

				core::GenericInt result = lhs.getInt().saddSat(rhs.getInt());
				const Expr result_expr = handler.createNumber(lhs.type, std::move(result));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::UADD_SAT: {
				const UAddSat& uadd_sat = handler.getUAddSat(stmt);
				if(uadd_sat.lhs.kind() != Expr::Kind::NUMBER || uadd_sat.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(uadd_sat.lhs);
				const Number& rhs = handler.getNumber(uadd_sat.rhs);

				core::GenericInt result = lhs.getInt().uaddSat(rhs.getInt());
				const Expr result_expr = handler.createNumber(lhs.type, std::move(result));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FADD: {
				const FAdd& fadd = handler.getFAdd(stmt);
				if(fadd.lhs.kind() != Expr::Kind::NUMBER || fadd.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(fadd.lhs);
				const Number& rhs = handler.getNumber(fadd.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getFloat().add(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;


			case Expr::Kind::SUB: {
				const Sub& sub = handler.getSub(stmt);
				if(sub.lhs.kind() != Expr::Kind::NUMBER || sub.rhs.kind() != Expr::Kind::NUMBER){ return false; }
				const Number& lhs = handler.getNumber(sub.lhs);
				const Number& rhs = handler.getNumber(sub.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().ssub(rhs.getInt());
				const Expr result_expr = handler.createNumber(lhs.type, std::move(result.result));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SSUB_WRAP: {
				const SSubWrap& ssub_wrap = handler.getSSubWrap(stmt);
				if(ssub_wrap.lhs.kind() != Expr::Kind::NUMBER || ssub_wrap.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}

				const Expr result_original_expr = handler.extractSSubWrapResult(stmt);
				const Expr wrapped_original_expr = handler.extractSSubWrapWrapped(stmt);

				const Number& lhs = handler.getNumber(ssub_wrap.lhs);
				const Number& rhs = handler.getNumber(ssub_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().ssub(rhs.getInt());

				const Expr result_expr = handler.createNumber(lhs.type, std::move(result.result));
				handler.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = handler.createBoolean(result.wrapped);
				handler.replaceExpr(wrapped_original_expr, wrapped_expr);

				handler.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::SSUB_WRAP_RESULT:  return false;
			case Expr::Kind::SSUB_WRAP_WRAPPED: return false;

			case Expr::Kind::USUB_WRAP: {
				const USubWrap& usub_wrap = handler.getUSubWrap(stmt);
				if(usub_wrap.lhs.kind() != Expr::Kind::NUMBER || usub_wrap.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}

				const Expr result_original_expr = handler.extractUSubWrapResult(stmt);
				const Expr wrapped_original_expr = handler.extractUSubWrapWrapped(stmt);

				const Number& lhs = handler.getNumber(usub_wrap.lhs);
				const Number& rhs = handler.getNumber(usub_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().usub(rhs.getInt());

				const Expr result_expr = handler.createNumber(lhs.type, std::move(result.result));
				handler.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = handler.createBoolean(result.wrapped);
				handler.replaceExpr(wrapped_original_expr, wrapped_expr);

				handler.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::USUB_WRAP_RESULT:  return false;
			case Expr::Kind::USUB_WRAP_WRAPPED: return false;

			case Expr::Kind::SSUB_SAT: {
				const SSubSat& ssub_sat = handler.getSSubSat(stmt);
				if(ssub_sat.lhs.kind() != Expr::Kind::NUMBER || ssub_sat.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(ssub_sat.lhs);
				const Number& rhs = handler.getNumber(ssub_sat.rhs);

				core::GenericInt result = lhs.getInt().ssubSat(rhs.getInt());
				const Expr result_expr = handler.createNumber(lhs.type, std::move(result));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::USUB_SAT: {
				const USubSat& usub_sat = handler.getUSubSat(stmt);
				if(usub_sat.lhs.kind() != Expr::Kind::NUMBER || usub_sat.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(usub_sat.lhs);
				const Number& rhs = handler.getNumber(usub_sat.rhs);

				core::GenericInt result = lhs.getInt().usubSat(rhs.getInt());
				const Expr result_expr = handler.createNumber(lhs.type, std::move(result));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FSUB: {
				const FSub& fsub = handler.getFSub(stmt);
				if(fsub.lhs.kind() != Expr::Kind::NUMBER || fsub.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(fsub.lhs);
				const Number& rhs = handler.getNumber(fsub.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getFloat().sub(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::MUL: {
				const Mul& mul = handler.getMul(stmt);
				if(mul.lhs.kind() != Expr::Kind::NUMBER || mul.rhs.kind() != Expr::Kind::NUMBER){ return false; }
				const Number& lhs = handler.getNumber(mul.lhs);
				const Number& rhs = handler.getNumber(mul.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().smul(rhs.getInt());
				const Expr result_expr = handler.createNumber(lhs.type, std::move(result.result));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SMUL_WRAP: {
				const SMulWrap& smul_wrap = handler.getSMulWrap(stmt);
				if(smul_wrap.lhs.kind() != Expr::Kind::NUMBER || smul_wrap.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}

				const Expr result_original_expr = handler.extractSMulWrapResult(stmt);
				const Expr wrapped_original_expr = handler.extractSMulWrapWrapped(stmt);

				const Number& lhs = handler.getNumber(smul_wrap.lhs);
				const Number& rhs = handler.getNumber(smul_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().smul(rhs.getInt());

				const Expr result_expr = handler.createNumber(lhs.type, std::move(result.result));
				handler.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = handler.createBoolean(result.wrapped);
				handler.replaceExpr(wrapped_original_expr, wrapped_expr);

				handler.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::SMUL_WRAP_RESULT:  return false;
			case Expr::Kind::SMUL_WRAP_WRAPPED: return false;

			case Expr::Kind::UMUL_WRAP: {
				const UMulWrap& umul_wrap = handler.getUMulWrap(stmt);
				if(umul_wrap.lhs.kind() != Expr::Kind::NUMBER || umul_wrap.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}

				const Expr result_original_expr = handler.extractUMulWrapResult(stmt);
				const Expr wrapped_original_expr = handler.extractUMulWrapWrapped(stmt);

				const Number& lhs = handler.getNumber(umul_wrap.lhs);
				const Number& rhs = handler.getNumber(umul_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().umul(rhs.getInt());

				const Expr result_expr = handler.createNumber(lhs.type, std::move(result.result));
				handler.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = handler.createBoolean(result.wrapped);
				handler.replaceExpr(wrapped_original_expr, wrapped_expr);

				handler.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::UMUL_WRAP_RESULT:  return false;
			case Expr::Kind::UMUL_WRAP_WRAPPED: return false;

			case Expr::Kind::SMUL_SAT: {
				const SMulSat& smul_sat = handler.getSMulSat(stmt);
				if(smul_sat.lhs.kind() != Expr::Kind::NUMBER || smul_sat.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(smul_sat.lhs);
				const Number& rhs = handler.getNumber(smul_sat.rhs);

				core::GenericInt result = lhs.getInt().smulSat(rhs.getInt());
				const Expr result_expr = handler.createNumber(lhs.type, std::move(result));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::UMUL_SAT: {
				const UMulSat& umul_sat = handler.getUMulSat(stmt);
				if(umul_sat.lhs.kind() != Expr::Kind::NUMBER || umul_sat.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(umul_sat.lhs);
				const Number& rhs = handler.getNumber(umul_sat.rhs);

				core::GenericInt result = lhs.getInt().umulSat(rhs.getInt());
				const Expr result_expr = handler.createNumber(lhs.type, std::move(result));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FMUL: {
				const FMul& fmul = handler.getFMul(stmt);
				if(fmul.lhs.kind() != Expr::Kind::NUMBER || fmul.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(fmul.lhs);
				const Number& rhs = handler.getNumber(fmul.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getFloat().mul(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SDIV: {
				const SDiv& sdiv = handler.getSDiv(stmt);
				if(sdiv.lhs.kind() != Expr::Kind::NUMBER || sdiv.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(sdiv.lhs);
				const Number& rhs = handler.getNumber(sdiv.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().sdiv(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::UDIV: {
				const UDiv& udiv = handler.getUDiv(stmt);
				if(udiv.lhs.kind() != Expr::Kind::NUMBER || udiv.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(udiv.lhs);
				const Number& rhs = handler.getNumber(udiv.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().sdiv(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FDIV: {
				const FDiv& fdiv = handler.getFDiv(stmt);
				if(fdiv.lhs.kind() != Expr::Kind::NUMBER || fdiv.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(fdiv.lhs);
				const Number& rhs = handler.getNumber(fdiv.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getFloat().div(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SREM: {
				const SRem& srem = handler.getSRem(stmt);
				if(srem.lhs.kind() != Expr::Kind::NUMBER || srem.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(srem.lhs);
				const Number& rhs = handler.getNumber(srem.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().srem(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::UREM: {
				const URem& urem = handler.getURem(stmt);
				if(urem.lhs.kind() != Expr::Kind::NUMBER || urem.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(urem.lhs);
				const Number& rhs = handler.getNumber(urem.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().srem(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FREM: {
				const FRem& frem = handler.getFRem(stmt);
				if(frem.lhs.kind() != Expr::Kind::NUMBER || frem.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(frem.lhs);
				const Number& rhs = handler.getNumber(frem.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getFloat().rem(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FNEG: {
				const FNeg& fneg = handler.getFNeg(stmt);
				if(fneg.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}

				const Number& rhs = handler.getNumber(fneg.rhs);

				const core::GenericFloat zero = [&](){
					switch(rhs.type.getWidth()){
						case 16:  return core::GenericFloat::createF16(0);
						case 32:  return core::GenericFloat::createF32(0);
						case 64:  return core::GenericFloat::createF64(0);
						case 80:  return core::GenericFloat::createF80(0);
						case 128: return core::GenericFloat::createF128(0);
					}
					evo::unreachable();
				}();

				const Expr result_expr = handler.createNumber(rhs.type, zero.sub(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::IEQ: {
				const IEq& ieq = handler.getIEq(stmt);
				if(ieq.lhs.kind() != Expr::Kind::NUMBER || ieq.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(ieq.lhs);
				const Number& rhs = handler.getNumber(ieq.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getInt().eq(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FEQ: {
				const FEq& feq = handler.getFEq(stmt);
				if(feq.lhs.kind() != Expr::Kind::NUMBER || feq.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(feq.lhs);
				const Number& rhs = handler.getNumber(feq.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getFloat().eq(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::INEQ: {
				const INeq& ineq = handler.getINeq(stmt);
				if(ineq.lhs.kind() != Expr::Kind::NUMBER || ineq.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(ineq.lhs);
				const Number& rhs = handler.getNumber(ineq.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getInt().neq(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FNEQ: {
				const FNeq& fneq = handler.getFNeq(stmt);
				if(fneq.lhs.kind() != Expr::Kind::NUMBER || fneq.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(fneq.lhs);
				const Number& rhs = handler.getNumber(fneq.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getFloat().neq(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SLT: {
				const SLT& slt = handler.getSLT(stmt);
				if(slt.lhs.kind() != Expr::Kind::NUMBER || slt.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(slt.lhs);
				const Number& rhs = handler.getNumber(slt.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getInt().slt(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::ULT: {
				const ULT& ult = handler.getULT(stmt);
				if(ult.lhs.kind() != Expr::Kind::NUMBER || ult.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(ult.lhs);
				const Number& rhs = handler.getNumber(ult.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getInt().ult(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FLT: {
				const FLT& flt = handler.getFLT(stmt);
				if(flt.lhs.kind() != Expr::Kind::NUMBER || flt.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(flt.lhs);
				const Number& rhs = handler.getNumber(flt.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getFloat().lt(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SLTE: {
				const SLTE& slte = handler.getSLTE(stmt);
				if(slte.lhs.kind() != Expr::Kind::NUMBER || slte.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(slte.lhs);
				const Number& rhs = handler.getNumber(slte.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getInt().sle(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::ULTE: {
				const ULTE& ulte = handler.getULTE(stmt);
				if(ulte.lhs.kind() != Expr::Kind::NUMBER || ulte.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(ulte.lhs);
				const Number& rhs = handler.getNumber(ulte.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getInt().ule(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FLTE: {
				const FLTE& flte = handler.getFLTE(stmt);
				if(flte.lhs.kind() != Expr::Kind::NUMBER || flte.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(flte.lhs);
				const Number& rhs = handler.getNumber(flte.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getFloat().le(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SGT: {
				const SGT& sgt = handler.getSGT(stmt);
				if(sgt.lhs.kind() != Expr::Kind::NUMBER || sgt.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(sgt.lhs);
				const Number& rhs = handler.getNumber(sgt.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getInt().sgt(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::UGT: {
				const UGT& ugt = handler.getUGT(stmt);
				if(ugt.lhs.kind() != Expr::Kind::NUMBER || ugt.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(ugt.lhs);
				const Number& rhs = handler.getNumber(ugt.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getInt().ugt(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FGT: {
				const FGT& fgt = handler.getFGT(stmt);
				if(fgt.lhs.kind() != Expr::Kind::NUMBER || fgt.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(fgt.lhs);
				const Number& rhs = handler.getNumber(fgt.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getFloat().gt(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SGTE: {
				const SGTE& sgte = handler.getSGTE(stmt);
				if(sgte.lhs.kind() != Expr::Kind::NUMBER || sgte.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(sgte.lhs);
				const Number& rhs = handler.getNumber(sgte.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getInt().sge(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::UGTE: {
				const UGTE& ugte = handler.getUGTE(stmt);
				if(ugte.lhs.kind() != Expr::Kind::NUMBER || ugte.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(ugte.lhs);
				const Number& rhs = handler.getNumber(ugte.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getInt().uge(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FGTE: {
				const FGTE& fgte = handler.getFGTE(stmt);
				if(fgte.lhs.kind() != Expr::Kind::NUMBER || fgte.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(fgte.lhs);
				const Number& rhs = handler.getNumber(fgte.rhs);

				const Expr result_expr = handler.createBoolean(lhs.getFloat().ge(rhs.getFloat()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::AND: {
				const And& and_stmt = handler.getAnd(stmt);
				if(and_stmt.lhs.kind() != Expr::Kind::NUMBER || and_stmt.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(and_stmt.lhs);
				const Number& rhs = handler.getNumber(and_stmt.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().bitwiseAnd(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::OR: {
				const Or& or_stmt = handler.getOr(stmt);
				if(or_stmt.lhs.kind() != Expr::Kind::NUMBER || or_stmt.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(or_stmt.lhs);
				const Number& rhs = handler.getNumber(or_stmt.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().bitwiseOr(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::XOR: {
				const Xor& xor_stmt = handler.getXor(stmt);
				if(xor_stmt.lhs.kind() != Expr::Kind::NUMBER || xor_stmt.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(xor_stmt.lhs);
				const Number& rhs = handler.getNumber(xor_stmt.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().bitwiseXor(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SHL: {
				const SHL& shl = handler.getSHL(stmt);
				if(shl.lhs.kind() != Expr::Kind::NUMBER || shl.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(shl.lhs);
				const Number& rhs = handler.getNumber(shl.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().ushl(rhs.getInt()).result);
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SSHL_SAT: {
				const SSHLSat& sshlsat = handler.getSSHLSat(stmt);
				if(sshlsat.lhs.kind() != Expr::Kind::NUMBER || sshlsat.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(sshlsat.lhs);
				const Number& rhs = handler.getNumber(sshlsat.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().sshlSat(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::USHL_SAT: {
				const USHLSat& ushlsat = handler.getUSHLSat(stmt);
				if(ushlsat.lhs.kind() != Expr::Kind::NUMBER || ushlsat.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(ushlsat.lhs);
				const Number& rhs = handler.getNumber(ushlsat.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().ushlSat(rhs.getInt()));
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SSHR: {
				const SSHR& sshr = handler.getSSHR(stmt);
				if(sshr.lhs.kind() != Expr::Kind::NUMBER || sshr.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(sshr.lhs);
				const Number& rhs = handler.getNumber(sshr.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().sshr(rhs.getInt()).result);
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::USHR: {
				const USHR& ushr = handler.getUSHR(stmt);
				if(ushr.lhs.kind() != Expr::Kind::NUMBER || ushr.rhs.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& lhs = handler.getNumber(ushr.lhs);
				const Number& rhs = handler.getNumber(ushr.rhs);

				const Expr result_expr = handler.createNumber(lhs.type, lhs.getInt().ushr(rhs.getInt()).result);
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::BIT_REVERSE: {
				const BitReverse& bit_reverse = handler.getBitReverse(stmt);
				if(bit_reverse.arg.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& arg = handler.getNumber(bit_reverse.arg);

				const Expr result_expr = handler.createNumber(arg.type, arg.getInt().bitReverse());
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::BYTE_SWAP: {
				const ByteSwap& byte_swap = handler.getByteSwap(stmt);
				if(byte_swap.arg.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& arg = handler.getNumber(byte_swap.arg);

				const Expr result_expr = handler.createNumber(arg.type, arg.getInt().byteSwap());
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::CTPOP: {
				const CtPop& ctpop = handler.getCtPop(stmt);
				if(ctpop.arg.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& arg = handler.getNumber(ctpop.arg);

				const Expr result_expr = handler.createNumber(arg.type, arg.getInt().ctPop());
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::CTLZ: {
				const CTLZ& ctlz = handler.getCTLZ(stmt);
				if(ctlz.arg.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& arg = handler.getNumber(ctlz.arg);

				const Expr result_expr = handler.createNumber(arg.type, arg.getInt().ctlz());
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::CTTZ: {
				const CTTZ& cttz = handler.getCTTZ(stmt);
				if(cttz.arg.kind() != Expr::Kind::NUMBER){
					return false;
				}
				const Number& arg = handler.getNumber(cttz.arg);

				const Expr result_expr = handler.createNumber(arg.type, arg.getInt().cttz());
				handler.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::CMPXCHG:           return false;
			case Expr::Kind::CMPXCHG_LOADED:    return false;
			case Expr::Kind::CMPXCHG_SUCCEEDED: return false;
			case Expr::Kind::ATOMIC_RMW:        return false;
			case Expr::Kind::META_LOCAL_VAR:    return false;
			case Expr::Kind::META_PARAM:        return false;
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto inst_simplify_impl(Expr stmt, const InstrHandler& handler) -> PassManager::MadeTransformation {
		switch(stmt.kind()){
			case Expr::Kind::NONE:             evo::debugFatalBreak("Not valid expr");
			case Expr::Kind::GLOBAL_VALUE:     return false;
			case Expr::Kind::FUNCTION_POINTER: return false;
			case Expr::Kind::NUMBER:           return false;
			case Expr::Kind::BOOLEAN:          return false;
			case Expr::Kind::NULLPTR:          return false;
			case Expr::Kind::PARAM_EXPR:       return false;
			case Expr::Kind::CALL:             return false;
			case Expr::Kind::CALL_VOID:        return false;
			case Expr::Kind::CALL_NO_RETURN:   return false;
			case Expr::Kind::ABORT:            return false;
			case Expr::Kind::BREAKPOINT:       return false;
			case Expr::Kind::RET:              return false;
			case Expr::Kind::JUMP:             return false;

			case Expr::Kind::BRANCH: {
				const Branch& branch = handler.getBranch(stmt);

				if(branch.cond.kind() != Expr::Kind::BOOLEAN){ return false; }

				if(handler.getBoolean(branch.cond)){
					const BasicBlock::ID then_block = branch.thenBlock;
					handler.removeStmt(stmt);
					handler.createJump(then_block);
				}else{
					const BasicBlock::ID else_block = branch.elseBlock;
					handler.removeStmt(stmt);
					handler.createJump(else_block);
				}

				return true;
			} break;

			case Expr::Kind::UNREACHABLE: return false;
			case Expr::Kind::PHI:         return false;
			case Expr::Kind::SWITCH:      return false;
			case Expr::Kind::ALLOCA:      return false;
			case Expr::Kind::LOAD:        return false;
			case Expr::Kind::STORE:       return false;

			case Expr::Kind::CALC_PTR: {
				const CalcPtr& calc_ptr = handler.getCalcPtr(stmt);

				for(const CalcPtr::Index& index : calc_ptr.indices){
					if(index.is<int64_t>()){
						if(index.as<int64_t>() != 0){ return false; }
					}else{
						const Expr& expr = index.as<Expr>();

						if(expr.kind() != Expr::Kind::NUMBER){ return false; }
						if(static_cast<uint64_t>(handler.getNumber(expr).getInt()) != 0){ return false; }
					}
				}

				handler.replaceExpr(stmt, calc_ptr.basePtr);
				return true;
			} break;

			// TODO(FEATURE): 
			case Expr::Kind::MEMCPY: return false;
			case Expr::Kind::MEMSET: return false;

			case Expr::Kind::BIT_CAST: {
				const BitCast& bitcast = handler.getBitCast(stmt);
				
				if(handler.getExprType(bitcast.fromValue) == bitcast.toType){
					handler.replaceExpr(stmt, bitcast.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::TRUNC: {
				const Trunc& trunc = handler.getTrunc(stmt);
				
				if(handler.getExprType(trunc.fromValue) == trunc.toType){
					handler.replaceExpr(stmt, trunc.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::FTRUNC: {
				const FTrunc& ftrunc = handler.getFTrunc(stmt);
				
				if(handler.getExprType(ftrunc.fromValue) == ftrunc.toType){
					handler.replaceExpr(stmt, ftrunc.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::SEXT: {
				const SExt& sext = handler.getSExt(stmt);
				
				if(handler.getExprType(sext.fromValue) == sext.toType){
					handler.replaceExpr(stmt, sext.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::ZEXT: {
				const ZExt& zext = handler.getZExt(stmt);
				
				if(handler.getExprType(zext.fromValue) == zext.toType){
					handler.replaceExpr(stmt, zext.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::FEXT: {
				const FExt& fext = handler.getFExt(stmt);
				
				if(handler.getExprType(fext.fromValue) == fext.toType){
					handler.replaceExpr(stmt, fext.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::ITOF: {
				const IToF& itof = handler.getIToF(stmt);
				
				if(handler.getExprType(itof.fromValue) == itof.toType){
					handler.replaceExpr(stmt, itof.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::UITOF: {
				const UIToF& uitof = handler.getUIToF(stmt);
				
				if(handler.getExprType(uitof.fromValue) == uitof.toType){
					handler.replaceExpr(stmt, uitof.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::FTOI: {
				const FToI& ftoi = handler.getFToI(stmt);
				
				if(handler.getExprType(ftoi.fromValue) == ftoi.toType){
					handler.replaceExpr(stmt, ftoi.fromValue);
					return true;
				}
			} break;

			case Expr::Kind::FTOUI: {
				const FToUI& ftoui = handler.getFToUI(stmt);
				
				if(handler.getExprType(ftoui.fromValue) == ftoui.toType){
					handler.replaceExpr(stmt, ftoui.fromValue);
					return true;
				}
			} break;


			case Expr::Kind::ADD: {
				const Add& add = handler.getAdd(stmt);

				if(add.lhs.kind() == Expr::Kind::NUMBER){
					const Number& lhs = handler.getNumber(add.lhs);
					const core::GenericInt& lhs_number = lhs.getInt();
					if(lhs_number == core::GenericInt(lhs_number.getBitWidth(), 0)){
						handler.replaceExpr(stmt, add.rhs);
						return true;
					}

				}else if(add.rhs.kind() == Expr::Kind::NUMBER){
					const Number& rhs = handler.getNumber(add.rhs);
					const core::GenericInt& rhs_number = rhs.getInt();
					if(rhs_number == core::GenericInt(rhs_number.getBitWidth(), 0)){
						handler.replaceExpr(stmt, add.lhs);
						return true;
					}
				}

				return false;
			} break;


			case Expr::Kind::SADD_WRAP: {
				const SAddWrap& sadd_wrap = handler.getSAddWrap(stmt);

				if(sadd_wrap.lhs.kind() == Expr::Kind::NUMBER){
					const Number& lhs = handler.getNumber(sadd_wrap.lhs);
					const core::GenericInt& number = lhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						handler.replaceExpr(handler.extractSAddWrapResult(stmt), sadd_wrap.rhs);
						handler.replaceExpr(handler.extractSAddWrapWrapped(stmt), handler.createBoolean(false));
						handler.removeStmt(stmt);
						return true;
					}

				}else if(sadd_wrap.rhs.kind() == Expr::Kind::NUMBER){
					const Number& rhs = handler.getNumber(sadd_wrap.rhs);
					const core::GenericInt& number = rhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						handler.replaceExpr(handler.extractSAddWrapResult(stmt), sadd_wrap.lhs);
						handler.replaceExpr(handler.extractSAddWrapWrapped(stmt), handler.createBoolean(false));
						handler.removeStmt(stmt);
						return true;
					}
				}

				return false;
			} break;

			case Expr::Kind::SADD_WRAP_RESULT:  return false;
			case Expr::Kind::SADD_WRAP_WRAPPED: return false;

			case Expr::Kind::UADD_WRAP: {
				const UAddWrap& uadd_wrap = handler.getUAddWrap(stmt);

				if(uadd_wrap.lhs.kind() == Expr::Kind::NUMBER){
					const Number& lhs = handler.getNumber(uadd_wrap.lhs);
					const core::GenericInt& number = lhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						handler.replaceExpr(handler.extractUAddWrapResult(stmt), uadd_wrap.rhs);
						handler.replaceExpr(handler.extractUAddWrapWrapped(stmt), handler.createBoolean(false));
						handler.removeStmt(stmt);
						return true;
					}

				}else if(uadd_wrap.rhs.kind() == Expr::Kind::NUMBER){
					const Number& rhs = handler.getNumber(uadd_wrap.rhs);
					const core::GenericInt& number = rhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						handler.replaceExpr(handler.extractUAddWrapResult(stmt), uadd_wrap.lhs);
						handler.replaceExpr(handler.extractUAddWrapWrapped(stmt), handler.createBoolean(false));
						handler.removeStmt(stmt);
						return true;
					}
				}

				return false;
			} break;

			case Expr::Kind::UADD_WRAP_RESULT:  return false;
			case Expr::Kind::UADD_WRAP_WRAPPED: return false;

			case Expr::Kind::SADD_SAT: {
				const SAddSat& sadd_sat = handler.getSAddSat(stmt);

				if(sadd_sat.lhs.kind() == Expr::Kind::NUMBER){
					const Number& lhs = handler.getNumber(sadd_sat.lhs);
					const core::GenericInt& lhs_number = lhs.getInt();
					if(lhs_number == core::GenericInt(lhs_number.getBitWidth(), 0)){
						handler.replaceExpr(stmt, sadd_sat.rhs);
						return true;
					}

				}else if(sadd_sat.rhs.kind() == Expr::Kind::NUMBER){
					const Number& rhs = handler.getNumber(sadd_sat.rhs);
					const core::GenericInt& rhs_number = rhs.getInt();
					if(rhs_number == core::GenericInt(rhs_number.getBitWidth(), 0)){
						handler.replaceExpr(stmt, sadd_sat.lhs);
						return true;
					}
				}

				return false;
			} break;

			case Expr::Kind::UADD_SAT: {
				const UAddSat& uadd_sat = handler.getUAddSat(stmt);

				if(uadd_sat.lhs.kind() == Expr::Kind::NUMBER){
					const Number& lhs = handler.getNumber(uadd_sat.lhs);
					const core::GenericInt& lhs_number = lhs.getInt();
					if(lhs_number == core::GenericInt(lhs_number.getBitWidth(), 0)){
						handler.replaceExpr(stmt, uadd_sat.rhs);
						return true;
					}

				}else if(uadd_sat.rhs.kind() == Expr::Kind::NUMBER){
					const Number& rhs = handler.getNumber(uadd_sat.rhs);
					const core::GenericInt& rhs_number = rhs.getInt();
					if(rhs_number == core::GenericInt(rhs_number.getBitWidth(), 0)){
						handler.replaceExpr(stmt, uadd_sat.lhs);
						return true;
					}
				}

				return false;
			} break;

			case Expr::Kind::FADD: return false;

			// TODO(FEATURE): 
			case Expr::Kind::SUB:               return false;
			case Expr::Kind::SSUB_WRAP:         return false;
			case Expr::Kind::SSUB_WRAP_RESULT:  return false;
			case Expr::Kind::SSUB_WRAP_WRAPPED: return false;
			case Expr::Kind::USUB_WRAP:         return false;
			case Expr::Kind::USUB_WRAP_RESULT:  return false;
			case Expr::Kind::USUB_WRAP_WRAPPED: return false;
			case Expr::Kind::SSUB_SAT:          return false;
			case Expr::Kind::USUB_SAT:          return false;
			case Expr::Kind::FSUB:              return false;
			case Expr::Kind::MUL:               return false;
			case Expr::Kind::SMUL_WRAP:         return false;
			case Expr::Kind::SMUL_WRAP_RESULT:  return false;
			case Expr::Kind::SMUL_WRAP_WRAPPED: return false;
			case Expr::Kind::UMUL_WRAP:         return false;
			case Expr::Kind::UMUL_WRAP_RESULT:  return false;
			case Expr::Kind::UMUL_WRAP_WRAPPED: return false;
			case Expr::Kind::SMUL_SAT:          return false;
			case Expr::Kind::UMUL_SAT:          return false;
			case Expr::Kind::FMUL:              return false;
			case Expr::Kind::SDIV:              return false;
			case Expr::Kind::UDIV:              return false;
			case Expr::Kind::FDIV:              return false;
			case Expr::Kind::SREM:              return false;
			case Expr::Kind::UREM:              return false;
			case Expr::Kind::FREM:              return false;
			case Expr::Kind::FNEG:              return false;

			// TODO(FEATURE):
			case Expr::Kind::IEQ:  return false;
			case Expr::Kind::FEQ:  return false;
			case Expr::Kind::INEQ: return false;
			case Expr::Kind::FNEQ: return false;
			case Expr::Kind::SLT:  return false;
			case Expr::Kind::ULT:  return false;
			case Expr::Kind::FLT:  return false;
			case Expr::Kind::SLTE: return false;
			case Expr::Kind::ULTE: return false;
			case Expr::Kind::FLTE: return false;
			case Expr::Kind::SGT:  return false;
			case Expr::Kind::UGT:  return false;
			case Expr::Kind::FGT:  return false;
			case Expr::Kind::SGTE: return false;
			case Expr::Kind::UGTE: return false;
			case Expr::Kind::FGTE: return false;

			// TODO(FEATURE):
			case Expr::Kind::AND:      return false;
			case Expr::Kind::OR:       return false;
			case Expr::Kind::XOR:      return false;
			case Expr::Kind::SHL:      return false;
			case Expr::Kind::SSHL_SAT: return false;
			case Expr::Kind::USHL_SAT: return false;
			case Expr::Kind::SSHR:     return false;
			case Expr::Kind::USHR:     return false;

			// TODO(FEATURE): 
			case Expr::Kind::BIT_REVERSE: return false;
			case Expr::Kind::BYTE_SWAP:   return false;
			case Expr::Kind::CTPOP:       return false;
			case Expr::Kind::CTLZ:        return false;
			case Expr::Kind::CTTZ:        return false;

			case Expr::Kind::CMPXCHG:           return false;
			case Expr::Kind::CMPXCHG_LOADED:    return false;
			case Expr::Kind::CMPXCHG_SUCCEEDED: return false;
			case Expr::Kind::ATOMIC_RMW:        return false;

			case Expr::Kind::META_LOCAL_VAR:    return false;
			case Expr::Kind::META_PARAM:        return false;
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto inst_combine_impl(Expr, const InstrHandler&) -> PassManager::MadeTransformation {
		// TODO(FEATURE): 
		return false;
	}


}