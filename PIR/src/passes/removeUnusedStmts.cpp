////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/passes/removeUnusedStmts.h"

// #include "../../include/Type.h"
#include "../../include/Expr.h"
// #include "../../include/BasicBlock.h"
#include "../../include/Function.h"
// #include "../../include/Module.h"
#include "../../include/Agent.h"

#include <unordered_set>

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir::passes{
	

	auto removeUnusedStmts() -> PassManager::ReverseStmtPass {
		using FuncMetadata = std::unordered_set<Expr>;
		auto metadata = std::unordered_map<Function*, FuncMetadata>();

		auto impl = [metadata](Expr stmt, const Agent& agent) mutable -> PassManager::MadeTransformation {
			FuncMetadata& func_metadata = [&]() -> FuncMetadata& {
				auto func_metadata_iter = metadata.find(&agent.getTargetFunction());

				if(func_metadata_iter == metadata.end()){
					return metadata.emplace(&agent.getTargetFunction(), FuncMetadata()).first->second;
				}else{
					return func_metadata_iter->second;
				}
			}();

			auto see_expr = [&](const Expr& expr) -> void {
				switch(expr.getKind()){
					break; case Expr::Kind::None:            evo::debugFatalBreak("Invalid expr");
					break; case Expr::Kind::GlobalValue:     break;
					break; case Expr::Kind::Number:          break;
					break; case Expr::Kind::Boolean:         break;
					break; case Expr::Kind::ParamExpr:       break;
					break; case Expr::Kind::Call:            func_metadata.emplace(expr);
					break; case Expr::Kind::CallVoid:        evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::Breakpoint:      evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::Ret:             evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::Branch:          evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::CondBranch:      evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::Unreachable:     evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::Alloca:          func_metadata.emplace(expr);
					break; case Expr::Kind::Load:            func_metadata.emplace(expr);
					break; case Expr::Kind::Store:           evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::CalcPtr:         func_metadata.emplace(expr);
					break; case Expr::Kind::BitCast:         func_metadata.emplace(expr);
					break; case Expr::Kind::Trunc:           func_metadata.emplace(expr);
					break; case Expr::Kind::FTrunc:          func_metadata.emplace(expr);
					break; case Expr::Kind::SExt:            func_metadata.emplace(expr);
					break; case Expr::Kind::ZExt:            func_metadata.emplace(expr);
					break; case Expr::Kind::FExt:            func_metadata.emplace(expr);
					break; case Expr::Kind::IToF:            func_metadata.emplace(expr);
					break; case Expr::Kind::UIToF:           func_metadata.emplace(expr);
					break; case Expr::Kind::FToI:            func_metadata.emplace(expr);
					break; case Expr::Kind::FToUI:           func_metadata.emplace(expr);
					break; case Expr::Kind::Add:             func_metadata.emplace(expr);
					break; case Expr::Kind::SAddWrap:        evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::SAddWrapResult:  func_metadata.emplace(expr);
					break; case Expr::Kind::SAddWrapWrapped: func_metadata.emplace(expr);
					break; case Expr::Kind::UAddWrap:        evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::UAddWrapResult:  func_metadata.emplace(expr);
					break; case Expr::Kind::UAddWrapWrapped: func_metadata.emplace(expr);
					break; case Expr::Kind::SAddSat:         func_metadata.emplace(expr);
					break; case Expr::Kind::UAddSat:         func_metadata.emplace(expr);
					break; case Expr::Kind::FAdd:            func_metadata.emplace(expr);
					break; case Expr::Kind::Sub:             func_metadata.emplace(expr);
					break; case Expr::Kind::SSubWrap:        evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::SSubWrapResult:  func_metadata.emplace(expr);
					break; case Expr::Kind::SSubWrapWrapped: func_metadata.emplace(expr);
					break; case Expr::Kind::USubWrap:        evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::USubWrapResult:  func_metadata.emplace(expr);
					break; case Expr::Kind::USubWrapWrapped: func_metadata.emplace(expr);
					break; case Expr::Kind::SSubSat:         func_metadata.emplace(expr);
					break; case Expr::Kind::USubSat:         func_metadata.emplace(expr);
					break; case Expr::Kind::FSub:            func_metadata.emplace(expr);
					break; case Expr::Kind::Mul:             func_metadata.emplace(expr);
					break; case Expr::Kind::SMulWrap:        evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::SMulWrapResult:  func_metadata.emplace(expr);
					break; case Expr::Kind::SMulWrapWrapped: func_metadata.emplace(expr);
					break; case Expr::Kind::UMulWrap:        evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::UMulWrapResult:  func_metadata.emplace(expr);
					break; case Expr::Kind::UMulWrapWrapped: func_metadata.emplace(expr);
					break; case Expr::Kind::SMulSat:         func_metadata.emplace(expr);
					break; case Expr::Kind::UMulSat:         func_metadata.emplace(expr);
					break; case Expr::Kind::FMul:            func_metadata.emplace(expr);
					break; case Expr::Kind::SDiv:            func_metadata.emplace(expr);
					break; case Expr::Kind::UDiv:            func_metadata.emplace(expr);
					break; case Expr::Kind::FDiv:            func_metadata.emplace(expr);
					break; case Expr::Kind::SRem:            func_metadata.emplace(expr);
					break; case Expr::Kind::URem:            func_metadata.emplace(expr);
					break; case Expr::Kind::FRem:            func_metadata.emplace(expr);
					break; case Expr::Kind::IEq:             func_metadata.emplace(expr);
					break; case Expr::Kind::FEq:             func_metadata.emplace(expr);
					break; case Expr::Kind::INeq:            func_metadata.emplace(expr);
					break; case Expr::Kind::FNeq:            func_metadata.emplace(expr);
					break; case Expr::Kind::SLT:             func_metadata.emplace(expr);
					break; case Expr::Kind::ULT:             func_metadata.emplace(expr);
					break; case Expr::Kind::FLT:             func_metadata.emplace(expr);
					break; case Expr::Kind::SLTE:            func_metadata.emplace(expr);
					break; case Expr::Kind::ULTE:            func_metadata.emplace(expr);
					break; case Expr::Kind::FLTE:            func_metadata.emplace(expr);
					break; case Expr::Kind::SGT:             func_metadata.emplace(expr);
					break; case Expr::Kind::UGT:             func_metadata.emplace(expr);
					break; case Expr::Kind::FGT:             func_metadata.emplace(expr);
					break; case Expr::Kind::SGTE:            func_metadata.emplace(expr);
					break; case Expr::Kind::UGTE:            func_metadata.emplace(expr);
					break; case Expr::Kind::FGTE:            func_metadata.emplace(expr);
				}
			};

			const auto remove_unused_stmt = [&]() -> bool {
				if(func_metadata.contains(stmt) == false){
					agent.removeStmt(stmt);
					return true;
				}

				return false;
			};

			switch(stmt.getKind()){
				case Expr::Kind::None:        evo::debugFatalBreak("Invalid expr");
				case Expr::Kind::GlobalValue: return false;
				case Expr::Kind::Number:      return false;
				case Expr::Kind::Boolean:     return false;
				case Expr::Kind::ParamExpr:   return false;

				case Expr::Kind::Call: {
					// TODO: remove if func has no side-effects
					// if(remove_unused_stmt(stmt)){ return true; }

					const Call& call_inst = agent.getCall(stmt);

					if(call_inst.target.is<PtrCall>()){
						see_expr(call_inst.target.as<PtrCall>().location);
					}

					for(const Expr& arg : call_inst.args){
						see_expr(arg);
					}

					return false;
				} break;

				case Expr::Kind::CallVoid: {
					const CallVoid& call_void_inst = agent.getCallVoid(stmt);

					if(call_void_inst.target.is<PtrCall>()){
						see_expr(call_void_inst.target.as<PtrCall>().location);
					}

					for(const Expr& arg : call_void_inst.args){
						see_expr(arg);
					}

					return false;
				} break;

				case Expr::Kind::Breakpoint: return false;

				case Expr::Kind::Ret: {
					const Ret& ret_inst = agent.getRet(stmt);

					if(ret_inst.value.has_value()){
						see_expr(*ret_inst.value);
					}

					return false;
				} break;

				case Expr::Kind::Branch:      return false;
				case Expr::Kind::CondBranch:  return false;
				case Expr::Kind::Unreachable: return false;

				case Expr::Kind::Alloca: {
					if(remove_unused_stmt()){ return true; }

					return false;
				} break;


				case Expr::Kind::Load: {
					if(remove_unused_stmt()){ return true; }

					const Load& load = agent.getLoad(stmt);
					see_expr(load.source);

					return false;
				} break;

				case Expr::Kind::Store: {
					const Store& store = agent.getStore(stmt);
					see_expr(store.destination);
					see_expr(store.value);

					return false;
				} break;

				case Expr::Kind::CalcPtr: {
					if(remove_unused_stmt()){ return true; }

					const CalcPtr& calc_ptr = agent.getCalcPtr(stmt);
					see_expr(calc_ptr.basePtr);

					for(const CalcPtr::Index& index : calc_ptr.indices){
						if(index.is<Expr>()){ see_expr(index.as<Expr>()); }
					}

					return false;
				} break;


				case Expr::Kind::BitCast: {
					if(remove_unused_stmt()){ return true; }

					const BitCast& bitcast = agent.getBitCast(stmt);
					see_expr(bitcast.fromValue);
				} break;

				case Expr::Kind::Trunc: {
					if(remove_unused_stmt()){ return true; }

					const Trunc& trunc = agent.getTrunc(stmt);
					see_expr(trunc.fromValue);
				} break;

				case Expr::Kind::FTrunc: {
					if(remove_unused_stmt()){ return true; }

					const FTrunc& ftrunc = agent.getFTrunc(stmt);
					see_expr(ftrunc.fromValue);
				} break;

				case Expr::Kind::SExt: {
					if(remove_unused_stmt()){ return true; }

					const SExt& sext = agent.getSExt(stmt);
					see_expr(sext.fromValue);
				} break;

				case Expr::Kind::ZExt: {
					if(remove_unused_stmt()){ return true; }

					const ZExt& zext = agent.getZExt(stmt);
					see_expr(zext.fromValue);
				} break;

				case Expr::Kind::FExt: {
					if(remove_unused_stmt()){ return true; }

					const FExt& fext = agent.getFExt(stmt);
					see_expr(fext.fromValue);
				} break;

				case Expr::Kind::IToF: {
					if(remove_unused_stmt()){ return true; }

					const IToF& itof = agent.getIToF(stmt);
					see_expr(itof.fromValue);
				} break;

				case Expr::Kind::UIToF: {
					if(remove_unused_stmt()){ return true; }

					const UIToF& uitof = agent.getUIToF(stmt);
					see_expr(uitof.fromValue);
				} break;

				case Expr::Kind::FToI: {
					if(remove_unused_stmt()){ return true; }

					const FToI& ftoi = agent.getFToI(stmt);
					see_expr(ftoi.fromValue);
				} break;

				case Expr::Kind::FToUI: {
					if(remove_unused_stmt()){ return true; }

					const FToUI& ftoui = agent.getFToUI(stmt);
					see_expr(ftoui.fromValue);
				} break;


				case Expr::Kind::Add: {
					if(remove_unused_stmt()){ return true; }

					const Add& add = agent.getAdd(stmt);
					see_expr(add.lhs);
					see_expr(add.rhs);

					return false;
				} break;

				case Expr::Kind::SAddWrap: {
					if(func_metadata.contains(agent.extractSAddWrapWrapped(stmt)) == false){
						if(func_metadata.contains(agent.extractSAddWrapResult(stmt)) == false){
							agent.removeStmt(stmt);
							return true;		
						}

						// if wrapped value is never used, replace addWrap with just an add
						const SAddWrap& sadd_wrap = agent.getSAddWrap(stmt);
						see_expr(sadd_wrap.lhs);
						see_expr(sadd_wrap.rhs);

						const Expr new_add = agent.createAdd(
							sadd_wrap.lhs, sadd_wrap.rhs, true, std::string(sadd_wrap.resultName)
						);
						agent.replaceExpr(agent.extractSAddWrapResult(stmt), new_add);
						agent.removeStmt(stmt);
						return true;
					}

					const SAddWrap& sadd_wrap = agent.getSAddWrap(stmt);
					see_expr(sadd_wrap.lhs);
					see_expr(sadd_wrap.rhs);

					return false;
				} break;

				case Expr::Kind::SAddWrapResult:  return false;
				case Expr::Kind::SAddWrapWrapped: return false;

				case Expr::Kind::UAddWrap: {
					if(func_metadata.contains(agent.extractUAddWrapWrapped(stmt)) == false){
						if(func_metadata.contains(agent.extractUAddWrapResult(stmt)) == false){
							agent.removeStmt(stmt);
							return true;		
						}

						// if wrapped value is never used, replace addWrap with just an add
						const UAddWrap& uadd_wrap = agent.getUAddWrap(stmt);
						see_expr(uadd_wrap.lhs);
						see_expr(uadd_wrap.rhs);

						const Expr new_add = agent.createAdd(
							uadd_wrap.lhs, uadd_wrap.rhs, true, std::string(uadd_wrap.resultName)
						);
						agent.replaceExpr(agent.extractUAddWrapResult(stmt), new_add);
						agent.removeStmt(stmt);
						return true;
					}

					const UAddWrap& uadd_wrap = agent.getUAddWrap(stmt);
					see_expr(uadd_wrap.lhs);
					see_expr(uadd_wrap.rhs);

					return false;
				} break;

				case Expr::Kind::UAddWrapResult:  return false;
				case Expr::Kind::UAddWrapWrapped: return false;

				case Expr::Kind::SAddSat: {
					if(remove_unused_stmt()){ return true; }

					const SAddSat& sadd_sat = agent.getSAddSat(stmt);
					see_expr(sadd_sat.lhs);
					see_expr(sadd_sat.rhs);

					return false;
				} break;

				case Expr::Kind::UAddSat: {
					if(remove_unused_stmt()){ return true; }

					const UAddSat& uadd_sat = agent.getUAddSat(stmt);
					see_expr(uadd_sat.lhs);
					see_expr(uadd_sat.rhs);

					return false;
				} break;

				case Expr::Kind::FAdd: {
					if(remove_unused_stmt()){ return true; }

					const FAdd& fadd = agent.getFAdd(stmt);
					see_expr(fadd.lhs);
					see_expr(fadd.rhs);

					return false;
				} break;


				case Expr::Kind::Sub: {
					if(remove_unused_stmt()){ return true; }

					const Sub& sub = agent.getSub(stmt);
					see_expr(sub.lhs);
					see_expr(sub.rhs);

					return false;
				} break;

				case Expr::Kind::SSubWrap: {
					if(func_metadata.contains(agent.extractSSubWrapWrapped(stmt)) == false){
						if(func_metadata.contains(agent.extractSSubWrapResult(stmt)) == false){
							agent.removeStmt(stmt);
							return true;		
						}

						// if wrapped value is never used, replace subWrap with just an sub
						const SSubWrap& ssub_wrap = agent.getSSubWrap(stmt);
						see_expr(ssub_wrap.lhs);
						see_expr(ssub_wrap.rhs);

						const Expr new_sub = agent.createSub(
							ssub_wrap.lhs, ssub_wrap.rhs, true, std::string(ssub_wrap.resultName)
						);
						agent.replaceExpr(agent.extractSSubWrapResult(stmt), new_sub);
						agent.removeStmt(stmt);
						return true;
					}

					const SSubWrap& ssub_wrap = agent.getSSubWrap(stmt);
					see_expr(ssub_wrap.lhs);
					see_expr(ssub_wrap.rhs);

					return false;
				} break;

				case Expr::Kind::SSubWrapResult:  return false;
				case Expr::Kind::SSubWrapWrapped: return false;

				case Expr::Kind::USubWrap: {
					if(func_metadata.contains(agent.extractUSubWrapWrapped(stmt)) == false){
						if(func_metadata.contains(agent.extractUSubWrapResult(stmt)) == false){
							agent.removeStmt(stmt);
							return true;		
						}

						// if wrapped value is never used, replace subWrap with just an sub
						const USubWrap& usub_wrap = agent.getUSubWrap(stmt);
						see_expr(usub_wrap.lhs);
						see_expr(usub_wrap.rhs);

						const Expr new_sub = agent.createSub(
							usub_wrap.lhs, usub_wrap.rhs, true, std::string(usub_wrap.resultName)
						);
						agent.replaceExpr(agent.extractUSubWrapResult(stmt), new_sub);
						agent.removeStmt(stmt);
						return true;
					}

					const USubWrap& usub_wrap = agent.getUSubWrap(stmt);
					see_expr(usub_wrap.lhs);
					see_expr(usub_wrap.rhs);

					return false;
				} break;

				case Expr::Kind::USubWrapResult:  return false;
				case Expr::Kind::USubWrapWrapped: return false;

				case Expr::Kind::SSubSat: {
					if(remove_unused_stmt()){ return true; }

					const SSubSat& ssub_sat = agent.getSSubSat(stmt);
					see_expr(ssub_sat.lhs);
					see_expr(ssub_sat.rhs);

					return false;
				} break;

				case Expr::Kind::USubSat: {
					if(remove_unused_stmt()){ return true; }

					const USubSat& usub_sat = agent.getUSubSat(stmt);
					see_expr(usub_sat.lhs);
					see_expr(usub_sat.rhs);

					return false;
				} break;

				case Expr::Kind::FSub: {
					if(remove_unused_stmt()){ return true; }

					const FSub& fsub = agent.getFSub(stmt);
					see_expr(fsub.lhs);
					see_expr(fsub.rhs);

					return false;
				} break;


				case Expr::Kind::Mul: {
					if(remove_unused_stmt()){ return true; }

					const Mul& mul = agent.getMul(stmt);
					see_expr(mul.lhs);
					see_expr(mul.rhs);

					return false;
				} break;

				case Expr::Kind::SMulWrap: {
					if(func_metadata.contains(agent.extractSMulWrapWrapped(stmt)) == false){
						if(func_metadata.contains(agent.extractSMulWrapResult(stmt)) == false){
							agent.removeStmt(stmt);
							return true;		
						}

						// if wrapped value is never used, replace mulWrap with just an mul
						const SMulWrap& smul_wrap = agent.getSMulWrap(stmt);
						see_expr(smul_wrap.lhs);
						see_expr(smul_wrap.rhs);

						const Expr new_mul = agent.createMul(
							smul_wrap.lhs, smul_wrap.rhs, true, std::string(smul_wrap.resultName)
						);
						agent.replaceExpr(agent.extractSMulWrapResult(stmt), new_mul);
						agent.removeStmt(stmt);
						return true;
					}

					const SMulWrap& smul_wrap = agent.getSMulWrap(stmt);
					see_expr(smul_wrap.lhs);
					see_expr(smul_wrap.rhs);

					return false;
				} break;

				case Expr::Kind::SMulWrapResult:  return false;
				case Expr::Kind::SMulWrapWrapped: return false;

				case Expr::Kind::UMulWrap: {
					if(func_metadata.contains(agent.extractUMulWrapWrapped(stmt)) == false){
						if(func_metadata.contains(agent.extractUMulWrapResult(stmt)) == false){
							agent.removeStmt(stmt);
							return true;		
						}

						// if wrapped value is never used, replace mulWrap with just an mul
						const UMulWrap& umul_wrap = agent.getUMulWrap(stmt);
						see_expr(umul_wrap.lhs);
						see_expr(umul_wrap.rhs);

						const Expr new_mul = agent.createMul(
							umul_wrap.lhs, umul_wrap.rhs, true, std::string(umul_wrap.resultName)
						);
						agent.replaceExpr(agent.extractUMulWrapResult(stmt), new_mul);
						agent.removeStmt(stmt);
						return true;
					}

					const UMulWrap& umul_wrap = agent.getUMulWrap(stmt);
					see_expr(umul_wrap.lhs);
					see_expr(umul_wrap.rhs);

					return false;
				} break;

				case Expr::Kind::UMulWrapResult:  return false;
				case Expr::Kind::UMulWrapWrapped: return false;

				case Expr::Kind::SMulSat: {
					if(remove_unused_stmt()){ return true; }

					const SMulSat& smul_sat = agent.getSMulSat(stmt);
					see_expr(smul_sat.lhs);
					see_expr(smul_sat.rhs);

					return false;
				} break;

				case Expr::Kind::UMulSat: {
					if(remove_unused_stmt()){ return true; }

					const UMulSat& umul_sat = agent.getUMulSat(stmt);
					see_expr(umul_sat.lhs);
					see_expr(umul_sat.rhs);

					return false;
				} break;

				case Expr::Kind::FMul: {
					if(remove_unused_stmt()){ return true; }

					const FMul& fmul = agent.getFMul(stmt);
					see_expr(fmul.lhs);
					see_expr(fmul.rhs);

					return false;
				} break;

				case Expr::Kind::SDiv: {
					if(remove_unused_stmt()){ return true; }

					const SDiv& sdiv = agent.getSDiv(stmt);
					see_expr(sdiv.lhs);
					see_expr(sdiv.rhs);

					return false;
				} break;

				case Expr::Kind::UDiv: {
					if(remove_unused_stmt()){ return true; }

					const UDiv udiv = agent.getUDiv(stmt);
					see_expr(udiv.lhs);
					see_expr(udiv.rhs);

					return false;
				} break;

				case Expr::Kind::FDiv: {
					if(remove_unused_stmt()){ return true; }

					const FDiv& fdiv = agent.getFDiv(stmt);
					see_expr(fdiv.lhs);
					see_expr(fdiv.rhs);

					return false;
				} break;


				case Expr::Kind::SRem: {
					if(remove_unused_stmt()){ return true; }

					const SRem& srem = agent.getSRem(stmt);
					see_expr(srem.lhs);
					see_expr(srem.rhs);

					return false;
				} break;

				case Expr::Kind::URem: {
					if(remove_unused_stmt()){ return true; }

					const URem urem = agent.getURem(stmt);
					see_expr(urem.lhs);
					see_expr(urem.rhs);

					return false;
				} break;

				case Expr::Kind::FRem: {
					if(remove_unused_stmt()){ return true; }

					const FRem& frem = agent.getFRem(stmt);
					see_expr(frem.lhs);
					see_expr(frem.rhs);

					return false;
				} break;

				case Expr::Kind::IEq: {
					if(remove_unused_stmt()){ return true; }

					const IEq& ieq = agent.getIEq(stmt);
					see_expr(ieq.lhs);
					see_expr(ieq.rhs);

					return false;
				} break;

				case Expr::Kind::FEq: {
					if(remove_unused_stmt()){ return true; }

					const FEq& feq = agent.getFEq(stmt);
					see_expr(feq.lhs);
					see_expr(feq.rhs);

					return false;
				} break;

				case Expr::Kind::INeq: {
					if(remove_unused_stmt()){ return true; }

					const INeq& ineq = agent.getINeq(stmt);
					see_expr(ineq.lhs);
					see_expr(ineq.rhs);

					return false;
				} break;

				case Expr::Kind::FNeq: {
					if(remove_unused_stmt()){ return true; }

					const FNeq& fneq = agent.getFNeq(stmt);
					see_expr(fneq.lhs);
					see_expr(fneq.rhs);

					return false;
				} break;

				case Expr::Kind::SLT: {
					if(remove_unused_stmt()){ return true; }

					const SLT& slt = agent.getSLT(stmt);
					see_expr(slt.lhs);
					see_expr(slt.rhs);

					return false;
				} break;

				case Expr::Kind::ULT: {
					if(remove_unused_stmt()){ return true; }

					const ULT& ult = agent.getULT(stmt);
					see_expr(ult.lhs);
					see_expr(ult.rhs);

					return false;
				} break;

				case Expr::Kind::FLT: {
					if(remove_unused_stmt()){ return true; }

					const FLT& flt = agent.getFLT(stmt);
					see_expr(flt.lhs);
					see_expr(flt.rhs);

					return false;
				} break;

				case Expr::Kind::SLTE: {
					if(remove_unused_stmt()){ return true; }

					const SLTE& slte = agent.getSLTE(stmt);
					see_expr(slte.lhs);
					see_expr(slte.rhs);

					return false;
				} break;

				case Expr::Kind::ULTE: {
					if(remove_unused_stmt()){ return true; }

					const ULTE& ulte = agent.getULTE(stmt);
					see_expr(ulte.lhs);
					see_expr(ulte.rhs);

					return false;
				} break;

				case Expr::Kind::FLTE: {
					if(remove_unused_stmt()){ return true; }

					const FLTE& flte = agent.getFLTE(stmt);
					see_expr(flte.lhs);
					see_expr(flte.rhs);

					return false;
				} break;

				case Expr::Kind::SGT: {
					if(remove_unused_stmt()){ return true; }

					const SGT& sgt = agent.getSGT(stmt);
					see_expr(sgt.lhs);
					see_expr(sgt.rhs);

					return false;
				} break;

				case Expr::Kind::UGT: {
					if(remove_unused_stmt()){ return true; }

					const UGT& ugt = agent.getUGT(stmt);
					see_expr(ugt.lhs);
					see_expr(ugt.rhs);

					return false;
				} break;

				case Expr::Kind::FGT: {
					if(remove_unused_stmt()){ return true; }

					const FGT& fgt = agent.getFGT(stmt);
					see_expr(fgt.lhs);
					see_expr(fgt.rhs);

					return false;
				} break;

				case Expr::Kind::SGTE: {
					if(remove_unused_stmt()){ return true; }

					const SGTE& sgte = agent.getSGTE(stmt);
					see_expr(sgte.lhs);
					see_expr(sgte.rhs);

					return false;
				} break;

				case Expr::Kind::UGTE: {
					if(remove_unused_stmt()){ return true; }

					const UGTE& ugte = agent.getUGTE(stmt);
					see_expr(ugte.lhs);
					see_expr(ugte.rhs);

					return false;
				} break;

				case Expr::Kind::FGTE: {
					if(remove_unused_stmt()){ return true; }

					const FGTE& fgte = agent.getFGTE(stmt);
					see_expr(fgte.lhs);
					see_expr(fgte.rhs);

					return false;
				} break;

			}

			evo::debugFatalBreak("Unknown or unsupported Expr::Kind ({})", evo::to_underlying(stmt.getKind()));
		};

		return PassManager::ReverseStmtPass(impl);
	}


}