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
		switch(stmt.kind()){
			case Expr::Kind::None:            evo::debugFatalBreak("Not valid expr");
			case Expr::Kind::GlobalValue:     return false;
			case Expr::Kind::FunctionPointer: return false;
			case Expr::Kind::Number:          return false;
			case Expr::Kind::Boolean:         return false;
			case Expr::Kind::ParamExpr:       return false;
			case Expr::Kind::Call:            return false;
			case Expr::Kind::CallVoid:        return false;
			case Expr::Kind::Breakpoint:      return false;
			case Expr::Kind::Ret:             return false;
			case Expr::Kind::Branch:          return false;
			case Expr::Kind::CondBranch:      return false;
			case Expr::Kind::Unreachable:     return false;
			case Expr::Kind::Alloca:          return false;
			case Expr::Kind::Load:            return false;
			case Expr::Kind::Store:           return false;
			case Expr::Kind::CalcPtr:         return false;
			case Expr::Kind::Memcpy:          return false;
			case Expr::Kind::Memset:          return false;
			case Expr::Kind::BitCast:         return false;
			case Expr::Kind::Trunc:           return false;
			case Expr::Kind::FTrunc:          return false;
			case Expr::Kind::SExt:            return false;
			case Expr::Kind::ZExt:            return false;
			case Expr::Kind::FExt:            return false;
			case Expr::Kind::IToF:            return false;
			case Expr::Kind::UIToF:           return false;
			case Expr::Kind::FToI:            return false;
			case Expr::Kind::FToUI:           return false;

			case Expr::Kind::Add: {
				const Add& add = agent.getAdd(stmt);
				if(add.lhs.kind() != Expr::Kind::Number || add.rhs.kind() != Expr::Kind::Number){ return false; }
				const Number& lhs = agent.getNumber(add.lhs);
				const Number& rhs = agent.getNumber(add.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().sadd(rhs.getInt());
				const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SAddWrap: {
				const SAddWrap& sadd_wrap = agent.getSAddWrap(stmt);
				if(sadd_wrap.lhs.kind() != Expr::Kind::Number || sadd_wrap.rhs.kind() != Expr::Kind::Number){
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
				if(uadd_wrap.lhs.kind() != Expr::Kind::Number || uadd_wrap.rhs.kind() != Expr::Kind::Number){
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

			case Expr::Kind::SAddSat: {
				const SAddSat& sadd_sat = agent.getSAddSat(stmt);
				if(sadd_sat.lhs.kind() != Expr::Kind::Number || sadd_sat.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(sadd_sat.lhs);
				const Number& rhs = agent.getNumber(sadd_sat.rhs);

				core::GenericInt result = lhs.getInt().saddSat(rhs.getInt());
				const Expr result_expr = agent.createNumber(lhs.type, std::move(result));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::UAddSat: {
				const UAddSat& uadd_sat = agent.getUAddSat(stmt);
				if(uadd_sat.lhs.kind() != Expr::Kind::Number || uadd_sat.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(uadd_sat.lhs);
				const Number& rhs = agent.getNumber(uadd_sat.rhs);

				core::GenericInt result = lhs.getInt().uaddSat(rhs.getInt());
				const Expr result_expr = agent.createNumber(lhs.type, std::move(result));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FAdd: {
				const FAdd& fadd = agent.getFAdd(stmt);
				if(fadd.lhs.kind() != Expr::Kind::Number || fadd.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(fadd.lhs);
				const Number& rhs = agent.getNumber(fadd.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getFloat().add(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;


			case Expr::Kind::Sub: {
				const Sub& sub = agent.getSub(stmt);
				if(sub.lhs.kind() != Expr::Kind::Number || sub.rhs.kind() != Expr::Kind::Number){ return false; }
				const Number& lhs = agent.getNumber(sub.lhs);
				const Number& rhs = agent.getNumber(sub.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().ssub(rhs.getInt());
				const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SSubWrap: {
				const SSubWrap& ssub_wrap = agent.getSSubWrap(stmt);
				if(ssub_wrap.lhs.kind() != Expr::Kind::Number || ssub_wrap.rhs.kind() != Expr::Kind::Number){
					return false;
				}

				const Expr result_original_expr = agent.extractSSubWrapResult(stmt);
				const Expr wrapped_original_expr = agent.extractSSubWrapWrapped(stmt);

				const Number& lhs = agent.getNumber(ssub_wrap.lhs);
				const Number& rhs = agent.getNumber(ssub_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().ssub(rhs.getInt());

				const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
				agent.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = agent.createBoolean(result.wrapped);
				agent.replaceExpr(wrapped_original_expr, wrapped_expr);

				agent.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::SSubWrapResult:  return false;
			case Expr::Kind::SSubWrapWrapped: return false;

			case Expr::Kind::USubWrap: {
				const USubWrap& usub_wrap = agent.getUSubWrap(stmt);
				if(usub_wrap.lhs.kind() != Expr::Kind::Number || usub_wrap.rhs.kind() != Expr::Kind::Number){
					return false;
				}

				const Expr result_original_expr = agent.extractUSubWrapResult(stmt);
				const Expr wrapped_original_expr = agent.extractUSubWrapWrapped(stmt);

				const Number& lhs = agent.getNumber(usub_wrap.lhs);
				const Number& rhs = agent.getNumber(usub_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().usub(rhs.getInt());

				const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
				agent.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = agent.createBoolean(result.wrapped);
				agent.replaceExpr(wrapped_original_expr, wrapped_expr);

				agent.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::USubWrapResult:  return false;
			case Expr::Kind::USubWrapWrapped: return false;

			case Expr::Kind::SSubSat: {
				const SSubSat& ssub_sat = agent.getSSubSat(stmt);
				if(ssub_sat.lhs.kind() != Expr::Kind::Number || ssub_sat.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(ssub_sat.lhs);
				const Number& rhs = agent.getNumber(ssub_sat.rhs);

				core::GenericInt result = lhs.getInt().ssubSat(rhs.getInt());
				const Expr result_expr = agent.createNumber(lhs.type, std::move(result));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::USubSat: {
				const USubSat& usub_sat = agent.getUSubSat(stmt);
				if(usub_sat.lhs.kind() != Expr::Kind::Number || usub_sat.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(usub_sat.lhs);
				const Number& rhs = agent.getNumber(usub_sat.rhs);

				core::GenericInt result = lhs.getInt().usubSat(rhs.getInt());
				const Expr result_expr = agent.createNumber(lhs.type, std::move(result));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FSub: {
				const FSub& fsub = agent.getFSub(stmt);
				if(fsub.lhs.kind() != Expr::Kind::Number || fsub.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(fsub.lhs);
				const Number& rhs = agent.getNumber(fsub.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getFloat().sub(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::Mul: {
				const Mul& mul = agent.getMul(stmt);
				if(mul.lhs.kind() != Expr::Kind::Number || mul.rhs.kind() != Expr::Kind::Number){ return false; }
				const Number& lhs = agent.getNumber(mul.lhs);
				const Number& rhs = agent.getNumber(mul.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().smul(rhs.getInt());
				const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SMulWrap: {
				const SMulWrap& smul_wrap = agent.getSMulWrap(stmt);
				if(smul_wrap.lhs.kind() != Expr::Kind::Number || smul_wrap.rhs.kind() != Expr::Kind::Number){
					return false;
				}

				const Expr result_original_expr = agent.extractSMulWrapResult(stmt);
				const Expr wrapped_original_expr = agent.extractSMulWrapWrapped(stmt);

				const Number& lhs = agent.getNumber(smul_wrap.lhs);
				const Number& rhs = agent.getNumber(smul_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().smul(rhs.getInt());

				const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
				agent.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = agent.createBoolean(result.wrapped);
				agent.replaceExpr(wrapped_original_expr, wrapped_expr);

				agent.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::SMulWrapResult:  return false;
			case Expr::Kind::SMulWrapWrapped: return false;

			case Expr::Kind::UMulWrap: {
				const UMulWrap& umul_wrap = agent.getUMulWrap(stmt);
				if(umul_wrap.lhs.kind() != Expr::Kind::Number || umul_wrap.rhs.kind() != Expr::Kind::Number){
					return false;
				}

				const Expr result_original_expr = agent.extractUMulWrapResult(stmt);
				const Expr wrapped_original_expr = agent.extractUMulWrapWrapped(stmt);

				const Number& lhs = agent.getNumber(umul_wrap.lhs);
				const Number& rhs = agent.getNumber(umul_wrap.rhs);

				core::GenericInt::WrapResult result = lhs.getInt().umul(rhs.getInt());

				const Expr result_expr = agent.createNumber(lhs.type, std::move(result.result));
				agent.replaceExpr(result_original_expr, result_expr);

				const Expr wrapped_expr = agent.createBoolean(result.wrapped);
				agent.replaceExpr(wrapped_original_expr, wrapped_expr);

				agent.removeStmt(stmt);

				return true;
			} break;

			case Expr::Kind::UMulWrapResult:  return false;
			case Expr::Kind::UMulWrapWrapped: return false;

			case Expr::Kind::SMulSat: {
				const SMulSat& smul_sat = agent.getSMulSat(stmt);
				if(smul_sat.lhs.kind() != Expr::Kind::Number || smul_sat.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(smul_sat.lhs);
				const Number& rhs = agent.getNumber(smul_sat.rhs);

				core::GenericInt result = lhs.getInt().smulSat(rhs.getInt());
				const Expr result_expr = agent.createNumber(lhs.type, std::move(result));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::UMulSat: {
				const UMulSat& umul_sat = agent.getUMulSat(stmt);
				if(umul_sat.lhs.kind() != Expr::Kind::Number || umul_sat.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(umul_sat.lhs);
				const Number& rhs = agent.getNumber(umul_sat.rhs);

				core::GenericInt result = lhs.getInt().umulSat(rhs.getInt());
				const Expr result_expr = agent.createNumber(lhs.type, std::move(result));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FMul: {
				const FMul& fmul = agent.getFMul(stmt);
				if(fmul.lhs.kind() != Expr::Kind::Number || fmul.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(fmul.lhs);
				const Number& rhs = agent.getNumber(fmul.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getFloat().mul(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SDiv: {
				const SDiv& sdiv = agent.getSDiv(stmt);
				if(sdiv.lhs.kind() != Expr::Kind::Number || sdiv.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(sdiv.lhs);
				const Number& rhs = agent.getNumber(sdiv.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().sdiv(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::UDiv: {
				const UDiv& udiv = agent.getUDiv(stmt);
				if(udiv.lhs.kind() != Expr::Kind::Number || udiv.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(udiv.lhs);
				const Number& rhs = agent.getNumber(udiv.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().sdiv(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FDiv: {
				const FDiv& fdiv = agent.getFDiv(stmt);
				if(fdiv.lhs.kind() != Expr::Kind::Number || fdiv.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(fdiv.lhs);
				const Number& rhs = agent.getNumber(fdiv.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getFloat().div(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SRem: {
				const SRem& srem = agent.getSRem(stmt);
				if(srem.lhs.kind() != Expr::Kind::Number || srem.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(srem.lhs);
				const Number& rhs = agent.getNumber(srem.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().srem(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::URem: {
				const URem& urem = agent.getURem(stmt);
				if(urem.lhs.kind() != Expr::Kind::Number || urem.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(urem.lhs);
				const Number& rhs = agent.getNumber(urem.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().srem(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FRem: {
				const FRem& frem = agent.getFRem(stmt);
				if(frem.lhs.kind() != Expr::Kind::Number || frem.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(frem.lhs);
				const Number& rhs = agent.getNumber(frem.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getFloat().rem(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FNeg: {
				const FNeg& fneg = agent.getFNeg(stmt);
				if(fneg.rhs.kind() != Expr::Kind::Number){
					return false;
				}

				const Number& rhs = agent.getNumber(fneg.rhs);

				const core::GenericFloat zero = [&](){
					if(rhs.type.kind() == Type::Kind::BFloat){ return core::GenericFloat::createBF16(0); }

					switch(rhs.type.getWidth()){
						case 16: return core::GenericFloat::createF16(0);
						case 32: return core::GenericFloat::createF32(0);
						case 64: return core::GenericFloat::createF64(0);
						case 80: return core::GenericFloat::createF80(0);
						case 128: return core::GenericFloat::createF128(0);
					}
					evo::unreachable();
				}();

				const Expr result_expr = agent.createNumber(rhs.type, zero.sub(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::IEq: {
				const IEq& ieq = agent.getIEq(stmt);
				if(ieq.lhs.kind() != Expr::Kind::Number || ieq.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(ieq.lhs);
				const Number& rhs = agent.getNumber(ieq.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getInt().eq(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FEq: {
				const FEq& feq = agent.getFEq(stmt);
				if(feq.lhs.kind() != Expr::Kind::Number || feq.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(feq.lhs);
				const Number& rhs = agent.getNumber(feq.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getFloat().eq(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::INeq: {
				const INeq& ineq = agent.getINeq(stmt);
				if(ineq.lhs.kind() != Expr::Kind::Number || ineq.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(ineq.lhs);
				const Number& rhs = agent.getNumber(ineq.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getInt().neq(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FNeq: {
				const FNeq& fneq = agent.getFNeq(stmt);
				if(fneq.lhs.kind() != Expr::Kind::Number || fneq.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(fneq.lhs);
				const Number& rhs = agent.getNumber(fneq.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getFloat().neq(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SLT: {
				const SLT& slt = agent.getSLT(stmt);
				if(slt.lhs.kind() != Expr::Kind::Number || slt.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(slt.lhs);
				const Number& rhs = agent.getNumber(slt.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getInt().slt(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::ULT: {
				const ULT& ult = agent.getULT(stmt);
				if(ult.lhs.kind() != Expr::Kind::Number || ult.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(ult.lhs);
				const Number& rhs = agent.getNumber(ult.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getInt().ult(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FLT: {
				const FLT& flt = agent.getFLT(stmt);
				if(flt.lhs.kind() != Expr::Kind::Number || flt.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(flt.lhs);
				const Number& rhs = agent.getNumber(flt.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getFloat().lt(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SLTE: {
				const SLTE& slte = agent.getSLTE(stmt);
				if(slte.lhs.kind() != Expr::Kind::Number || slte.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(slte.lhs);
				const Number& rhs = agent.getNumber(slte.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getInt().sle(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::ULTE: {
				const ULTE& ulte = agent.getULTE(stmt);
				if(ulte.lhs.kind() != Expr::Kind::Number || ulte.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(ulte.lhs);
				const Number& rhs = agent.getNumber(ulte.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getInt().ule(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FLTE: {
				const FLTE& flte = agent.getFLTE(stmt);
				if(flte.lhs.kind() != Expr::Kind::Number || flte.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(flte.lhs);
				const Number& rhs = agent.getNumber(flte.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getFloat().le(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SGT: {
				const SGT& sgt = agent.getSGT(stmt);
				if(sgt.lhs.kind() != Expr::Kind::Number || sgt.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(sgt.lhs);
				const Number& rhs = agent.getNumber(sgt.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getInt().sgt(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::UGT: {
				const UGT& ugt = agent.getUGT(stmt);
				if(ugt.lhs.kind() != Expr::Kind::Number || ugt.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(ugt.lhs);
				const Number& rhs = agent.getNumber(ugt.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getInt().ugt(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FGT: {
				const FGT& fgt = agent.getFGT(stmt);
				if(fgt.lhs.kind() != Expr::Kind::Number || fgt.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(fgt.lhs);
				const Number& rhs = agent.getNumber(fgt.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getFloat().gt(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SGTE: {
				const SGTE& sgte = agent.getSGTE(stmt);
				if(sgte.lhs.kind() != Expr::Kind::Number || sgte.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(sgte.lhs);
				const Number& rhs = agent.getNumber(sgte.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getInt().sge(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::UGTE: {
				const UGTE& ugte = agent.getUGTE(stmt);
				if(ugte.lhs.kind() != Expr::Kind::Number || ugte.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(ugte.lhs);
				const Number& rhs = agent.getNumber(ugte.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getInt().uge(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::FGTE: {
				const FGTE& fgte = agent.getFGTE(stmt);
				if(fgte.lhs.kind() != Expr::Kind::Number || fgte.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(fgte.lhs);
				const Number& rhs = agent.getNumber(fgte.rhs);

				const Expr result_expr = agent.createBoolean(lhs.getFloat().ge(rhs.getFloat()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::And: {
				const And& and_stmt = agent.getAnd(stmt);
				if(and_stmt.lhs.kind() != Expr::Kind::Number || and_stmt.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(and_stmt.lhs);
				const Number& rhs = agent.getNumber(and_stmt.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().bitwiseAnd(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::Or: {
				const Or& or_stmt = agent.getOr(stmt);
				if(or_stmt.lhs.kind() != Expr::Kind::Number || or_stmt.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(or_stmt.lhs);
				const Number& rhs = agent.getNumber(or_stmt.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().bitwiseOr(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::Xor: {
				const Xor& xor_stmt = agent.getXor(stmt);
				if(xor_stmt.lhs.kind() != Expr::Kind::Number || xor_stmt.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(xor_stmt.lhs);
				const Number& rhs = agent.getNumber(xor_stmt.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().bitwiseXor(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SHL: {
				const SHL& shl = agent.getSHL(stmt);
				if(shl.lhs.kind() != Expr::Kind::Number || shl.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(shl.lhs);
				const Number& rhs = agent.getNumber(shl.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().ushl(rhs.getInt()).result);
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SSHLSat: {
				const SSHLSat& sshlsat = agent.getSSHLSat(stmt);
				if(sshlsat.lhs.kind() != Expr::Kind::Number || sshlsat.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(sshlsat.lhs);
				const Number& rhs = agent.getNumber(sshlsat.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().sshlSat(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::USHLSat: {
				const USHLSat& ushlsat = agent.getUSHLSat(stmt);
				if(ushlsat.lhs.kind() != Expr::Kind::Number || ushlsat.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(ushlsat.lhs);
				const Number& rhs = agent.getNumber(ushlsat.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().ushlSat(rhs.getInt()));
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::SSHR: {
				const SSHR& sshr = agent.getSSHR(stmt);
				if(sshr.lhs.kind() != Expr::Kind::Number || sshr.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(sshr.lhs);
				const Number& rhs = agent.getNumber(sshr.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().sshr(rhs.getInt()).result);
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

			case Expr::Kind::USHR: {
				const USHR& ushr = agent.getUSHR(stmt);
				if(ushr.lhs.kind() != Expr::Kind::Number || ushr.rhs.kind() != Expr::Kind::Number){
					return false;
				}
				const Number& lhs = agent.getNumber(ushr.lhs);
				const Number& rhs = agent.getNumber(ushr.rhs);

				const Expr result_expr = agent.createNumber(lhs.type, lhs.getInt().ushr(rhs.getInt()).result);
				agent.replaceExpr(stmt, result_expr);
				return true;
			} break;

		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto inst_simplify_impl(Expr stmt, const Agent& agent) -> PassManager::MadeTransformation {
		switch(stmt.kind()){
			case Expr::Kind::None:            evo::debugFatalBreak("Not valid expr");
			case Expr::Kind::GlobalValue:     return false;
			case Expr::Kind::FunctionPointer: return false;
			case Expr::Kind::Number:          return false;
			case Expr::Kind::Boolean:         return false;
			case Expr::Kind::ParamExpr:       return false;
			case Expr::Kind::Call:            return false;
			case Expr::Kind::CallVoid:        return false;
			case Expr::Kind::Breakpoint:      return false;
			case Expr::Kind::Ret:             return false;
			case Expr::Kind::Branch:          return false;

			case Expr::Kind::CondBranch: {
				const CondBranch& cond_branch = agent.getCondBranch(stmt);

				if(cond_branch.cond.kind() != Expr::Kind::Boolean){ return false; }

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

						if(expr.kind() != Expr::Kind::Number){ return false; }
						if(static_cast<uint64_t>(agent.getNumber(expr).getInt()) != 0){ return false; }
					}
				}

				agent.replaceExpr(stmt, calc_ptr.basePtr);
				return true;
			} break;

			// TODO: 
			case Expr::Kind::Memcpy: return false;
			case Expr::Kind::Memset: return false;

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

				if(add.lhs.kind() == Expr::Kind::Number){
					const Number& lhs = agent.getNumber(add.lhs);
					const core::GenericInt& lhs_number = lhs.getInt();
					if(lhs_number == core::GenericInt(lhs_number.getBitWidth(), 0)){
						agent.replaceExpr(stmt, add.rhs);
						return true;
					}

				}else if(add.rhs.kind() == Expr::Kind::Number){
					const Number& rhs = agent.getNumber(add.rhs);
					const core::GenericInt& rhs_number = rhs.getInt();
					if(rhs_number == core::GenericInt(rhs_number.getBitWidth(), 0)){
						agent.replaceExpr(stmt, add.lhs);
						return true;
					}
				}

				return false;
			} break;


			case Expr::Kind::SAddWrap: {
				const SAddWrap& sadd_wrap = agent.getSAddWrap(stmt);

				if(sadd_wrap.lhs.kind() == Expr::Kind::Number){
					const Number& lhs = agent.getNumber(sadd_wrap.lhs);
					const core::GenericInt& number = lhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						agent.replaceExpr(agent.extractSAddWrapResult(stmt), sadd_wrap.rhs);
						agent.replaceExpr(agent.extractSAddWrapWrapped(stmt), agent.createBoolean(false));
						agent.removeStmt(stmt);
						return true;
					}

				}else if(sadd_wrap.rhs.kind() == Expr::Kind::Number){
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

				if(uadd_wrap.lhs.kind() == Expr::Kind::Number){
					const Number& lhs = agent.getNumber(uadd_wrap.lhs);
					const core::GenericInt& number = lhs.getInt();
					if(number == core::GenericInt(number.getBitWidth(), 0)){
						agent.replaceExpr(agent.extractUAddWrapResult(stmt), uadd_wrap.rhs);
						agent.replaceExpr(agent.extractUAddWrapWrapped(stmt), agent.createBoolean(false));
						agent.removeStmt(stmt);
						return true;
					}

				}else if(uadd_wrap.rhs.kind() == Expr::Kind::Number){
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

			case Expr::Kind::SAddSat: {
				const SAddSat& sadd_sat = agent.getSAddSat(stmt);

				if(sadd_sat.lhs.kind() == Expr::Kind::Number){
					const Number& lhs = agent.getNumber(sadd_sat.lhs);
					const core::GenericInt& lhs_number = lhs.getInt();
					if(lhs_number == core::GenericInt(lhs_number.getBitWidth(), 0)){
						agent.replaceExpr(stmt, sadd_sat.rhs);
						return true;
					}

				}else if(sadd_sat.rhs.kind() == Expr::Kind::Number){
					const Number& rhs = agent.getNumber(sadd_sat.rhs);
					const core::GenericInt& rhs_number = rhs.getInt();
					if(rhs_number == core::GenericInt(rhs_number.getBitWidth(), 0)){
						agent.replaceExpr(stmt, sadd_sat.lhs);
						return true;
					}
				}

				return false;
			} break;

			case Expr::Kind::UAddSat: {
				const UAddSat& uadd_sat = agent.getUAddSat(stmt);

				if(uadd_sat.lhs.kind() == Expr::Kind::Number){
					const Number& lhs = agent.getNumber(uadd_sat.lhs);
					const core::GenericInt& lhs_number = lhs.getInt();
					if(lhs_number == core::GenericInt(lhs_number.getBitWidth(), 0)){
						agent.replaceExpr(stmt, uadd_sat.rhs);
						return true;
					}

				}else if(uadd_sat.rhs.kind() == Expr::Kind::Number){
					const Number& rhs = agent.getNumber(uadd_sat.rhs);
					const core::GenericInt& rhs_number = rhs.getInt();
					if(rhs_number == core::GenericInt(rhs_number.getBitWidth(), 0)){
						agent.replaceExpr(stmt, uadd_sat.lhs);
						return true;
					}
				}

				return false;
			} break;

			case Expr::Kind::FAdd: return false;

			// TODO: 
			case Expr::Kind::Sub:             return false;
			case Expr::Kind::SSubWrap:        return false;
			case Expr::Kind::SSubWrapResult:  return false;
			case Expr::Kind::SSubWrapWrapped: return false;
			case Expr::Kind::USubWrap:        return false;
			case Expr::Kind::USubWrapResult:  return false;
			case Expr::Kind::USubWrapWrapped: return false;
			case Expr::Kind::SSubSat:         return false;
			case Expr::Kind::USubSat:         return false;
			case Expr::Kind::FSub:            return false;
			case Expr::Kind::Mul:             return false;
			case Expr::Kind::SMulWrap:        return false;
			case Expr::Kind::SMulWrapResult:  return false;
			case Expr::Kind::SMulWrapWrapped: return false;
			case Expr::Kind::UMulWrap:        return false;
			case Expr::Kind::UMulWrapResult:  return false;
			case Expr::Kind::UMulWrapWrapped: return false;
			case Expr::Kind::SMulSat:         return false;
			case Expr::Kind::UMulSat:         return false;
			case Expr::Kind::FMul:            return false;
			case Expr::Kind::SDiv:            return false;
			case Expr::Kind::UDiv:            return false;
			case Expr::Kind::FDiv:            return false;
			case Expr::Kind::SRem:            return false;
			case Expr::Kind::URem:            return false;
			case Expr::Kind::FRem:            return false;
			case Expr::Kind::FNeg:            return false;

			// TODO:
			case Expr::Kind::IEq:  return false;
			case Expr::Kind::FEq:  return false;
			case Expr::Kind::INeq: return false;
			case Expr::Kind::FNeq: return false;
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

			// TODO:
			case Expr::Kind::And:     return false;
			case Expr::Kind::Or:      return false;
			case Expr::Kind::Xor:     return false;
			case Expr::Kind::SHL:     return false;
			case Expr::Kind::SSHLSat: return false;
			case Expr::Kind::USHLSat: return false;
			case Expr::Kind::SSHR:    return false;
			case Expr::Kind::USHR:    return false;
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto inst_combine_impl(Expr, const Agent&) -> PassManager::MadeTransformation {
		return false;
	}


}