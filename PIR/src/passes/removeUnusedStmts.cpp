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
				switch(expr.kind()){
					break; case Expr::Kind::NONE:              evo::debugFatalBreak("Invalid expr");
					break; case Expr::Kind::GLOBAL_VALUE:      break;
					break; case Expr::Kind::FUNCTION_POINTER:  break;
					break; case Expr::Kind::NUMBER:            break;
					break; case Expr::Kind::BOOLEAN:           break;
					break; case Expr::Kind::NULLPTR:           break;
					break; case Expr::Kind::PARAM_EXPR:        break;
					break; case Expr::Kind::CALL:              func_metadata.emplace(expr);
					break; case Expr::Kind::CALL_VOID:         evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::ABORT:             evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::BREAKPOINT:        evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::RET:               evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::JUMP:              evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::BRANCH:            evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::UNREACHABLE:       evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::PHI:               evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::SWITCH:            evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::ALLOCA:            func_metadata.emplace(expr);
					break; case Expr::Kind::LOAD:              func_metadata.emplace(expr);
					break; case Expr::Kind::STORE:             evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::CALC_PTR:          func_metadata.emplace(expr);
					break; case Expr::Kind::MEMCPY:            func_metadata.emplace(expr);
					break; case Expr::Kind::MEMSET:            func_metadata.emplace(expr);
					break; case Expr::Kind::BIT_CAST:          func_metadata.emplace(expr);
					break; case Expr::Kind::TRUNC:             func_metadata.emplace(expr);
					break; case Expr::Kind::FTRUNC:            func_metadata.emplace(expr);
					break; case Expr::Kind::SEXT:              func_metadata.emplace(expr);
					break; case Expr::Kind::ZEXT:              func_metadata.emplace(expr);
					break; case Expr::Kind::FEXT:              func_metadata.emplace(expr);
					break; case Expr::Kind::ITOF:              func_metadata.emplace(expr);
					break; case Expr::Kind::UITOF:             func_metadata.emplace(expr);
					break; case Expr::Kind::FTOI:              func_metadata.emplace(expr);
					break; case Expr::Kind::FTOUI:             func_metadata.emplace(expr);
					break; case Expr::Kind::ADD:               func_metadata.emplace(expr);
					break; case Expr::Kind::SADD_WRAP:         evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::SADD_WRAP_RESULT:  func_metadata.emplace(expr);
					break; case Expr::Kind::SADD_WRAP_WRAPPED: func_metadata.emplace(expr);
					break; case Expr::Kind::UADD_WRAP:         evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::UADD_WRAP_RESULT:  func_metadata.emplace(expr);
					break; case Expr::Kind::UADD_WRAP_WRAPPED: func_metadata.emplace(expr);
					break; case Expr::Kind::SADD_SAT:          func_metadata.emplace(expr);
					break; case Expr::Kind::UADD_SAT:          func_metadata.emplace(expr);
					break; case Expr::Kind::FADD:              func_metadata.emplace(expr);
					break; case Expr::Kind::SUB:               func_metadata.emplace(expr);
					break; case Expr::Kind::SSUB_WRAP:         evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::SSUB_WRAP_RESULT:  func_metadata.emplace(expr);
					break; case Expr::Kind::SSUB_WRAP_WRAPPED: func_metadata.emplace(expr);
					break; case Expr::Kind::USUB_WRAP:         evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::USUB_WRAP_RESULT:  func_metadata.emplace(expr);
					break; case Expr::Kind::USUB_WRAP_WRAPPED: func_metadata.emplace(expr);
					break; case Expr::Kind::SSUB_SAT:          func_metadata.emplace(expr);
					break; case Expr::Kind::USUB_SAT:          func_metadata.emplace(expr);
					break; case Expr::Kind::FSUB:              func_metadata.emplace(expr);
					break; case Expr::Kind::MUL:               func_metadata.emplace(expr);
					break; case Expr::Kind::SMUL_WRAP:         evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::SMUL_WRAP_RESULT:  func_metadata.emplace(expr);
					break; case Expr::Kind::SMUL_WRAP_WRAPPED: func_metadata.emplace(expr);
					break; case Expr::Kind::UMUL_WRAP:         evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::UMUL_WRAP_RESULT:  func_metadata.emplace(expr);
					break; case Expr::Kind::UMUL_WRAP_WRAPPED: func_metadata.emplace(expr);
					break; case Expr::Kind::SMUL_SAT:          func_metadata.emplace(expr);
					break; case Expr::Kind::UMUL_SAT:          func_metadata.emplace(expr);
					break; case Expr::Kind::FMUL:              func_metadata.emplace(expr);
					break; case Expr::Kind::SDIV:              func_metadata.emplace(expr);
					break; case Expr::Kind::UDIV:              func_metadata.emplace(expr);
					break; case Expr::Kind::FDIV:              func_metadata.emplace(expr);
					break; case Expr::Kind::SREM:              func_metadata.emplace(expr);
					break; case Expr::Kind::UREM:              func_metadata.emplace(expr);
					break; case Expr::Kind::FREM:              func_metadata.emplace(expr);
					break; case Expr::Kind::FNEG:              func_metadata.emplace(expr);
					break; case Expr::Kind::IEQ:               func_metadata.emplace(expr);
					break; case Expr::Kind::FEQ:               func_metadata.emplace(expr);
					break; case Expr::Kind::INEQ:              func_metadata.emplace(expr);
					break; case Expr::Kind::FNEQ:              func_metadata.emplace(expr);
					break; case Expr::Kind::SLT:               func_metadata.emplace(expr);
					break; case Expr::Kind::ULT:               func_metadata.emplace(expr);
					break; case Expr::Kind::FLT:               func_metadata.emplace(expr);
					break; case Expr::Kind::SLTE:              func_metadata.emplace(expr);
					break; case Expr::Kind::ULTE:              func_metadata.emplace(expr);
					break; case Expr::Kind::FLTE:              func_metadata.emplace(expr);
					break; case Expr::Kind::SGT:               func_metadata.emplace(expr);
					break; case Expr::Kind::UGT:               func_metadata.emplace(expr);
					break; case Expr::Kind::FGT:               func_metadata.emplace(expr);
					break; case Expr::Kind::SGTE:              func_metadata.emplace(expr);
					break; case Expr::Kind::UGTE:              func_metadata.emplace(expr);
					break; case Expr::Kind::FGTE:              func_metadata.emplace(expr);
					break; case Expr::Kind::AND:               func_metadata.emplace(expr);
					break; case Expr::Kind::OR:                func_metadata.emplace(expr);
					break; case Expr::Kind::XOR:               func_metadata.emplace(expr);
					break; case Expr::Kind::SHL:               func_metadata.emplace(expr);
					break; case Expr::Kind::SSHL_SAT:          func_metadata.emplace(expr);
					break; case Expr::Kind::USHL_SAT:          func_metadata.emplace(expr);
					break; case Expr::Kind::SSHR:              func_metadata.emplace(expr);
					break; case Expr::Kind::USHR:              func_metadata.emplace(expr);
					break; case Expr::Kind::BIT_REVERSE:       func_metadata.emplace(expr);
					break; case Expr::Kind::BYTE_SWAP:         func_metadata.emplace(expr);
					break; case Expr::Kind::CTPOP:             func_metadata.emplace(expr);
					break; case Expr::Kind::CTLZ:              func_metadata.emplace(expr);
					break; case Expr::Kind::CTTZ:              func_metadata.emplace(expr);
					break; case Expr::Kind::CMPXCHG:           evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::CMPXCHG_LOADED:    func_metadata.emplace(expr);
					break; case Expr::Kind::CMPXCHG_SUCCEEDED: func_metadata.emplace(expr);
					break; case Expr::Kind::ATOMIC_RMW:        func_metadata.emplace(expr);
					break; case Expr::Kind::LIFETIME_START:    evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::LIFETIME_END:      evo::debugFatalBreak("Should never see this expr kind");
				}
			};

			const auto remove_unused_stmt = [&]() -> bool {
				if(func_metadata.contains(stmt) == false){
					agent.removeStmt(stmt);
					return true;
				}

				return false;
			};

			switch(stmt.kind()){
				case Expr::Kind::NONE:            evo::debugFatalBreak("Invalid expr");
				case Expr::Kind::GLOBAL_VALUE:     return false;
				case Expr::Kind::FUNCTION_POINTER: return false;
				case Expr::Kind::NUMBER:           return false;
				case Expr::Kind::BOOLEAN:          return false;
				case Expr::Kind::NULLPTR:          return false;
				case Expr::Kind::PARAM_EXPR:       return false;

				case Expr::Kind::CALL: {
					// TODO(FUTURE): remove if func has no side-effects
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

				case Expr::Kind::CALL_VOID: {
					const CallVoid& call_void_inst = agent.getCallVoid(stmt);

					if(call_void_inst.target.is<PtrCall>()){
						see_expr(call_void_inst.target.as<PtrCall>().location);
					}

					for(const Expr& arg : call_void_inst.args){
						see_expr(arg);
					}

					return false;
				} break;

				case Expr::Kind::ABORT:      return false;
				case Expr::Kind::BREAKPOINT: return false;

				case Expr::Kind::RET: {
					const Ret& ret_inst = agent.getRet(stmt);

					if(ret_inst.value.has_value()){
						see_expr(*ret_inst.value);
					}

					return false;
				} break;

				case Expr::Kind::JUMP:        return false;
				case Expr::Kind::BRANCH:      return false;
				case Expr::Kind::UNREACHABLE: return false;
				case Expr::Kind::PHI:         return false;
				case Expr::Kind::SWITCH:      return false;

				case Expr::Kind::ALLOCA: {
					if(remove_unused_stmt()){ return true; }

					return false;
				} break;


				case Expr::Kind::LOAD: {
					const Load& load = agent.getLoad(stmt);

					if(load.atomicOrdering == AtomicOrdering::NONE || load.atomicOrdering == AtomicOrdering::MONOTONIC){
						if(remove_unused_stmt()){ return true; }
					}

					see_expr(load.source);

					return false;
				} break;

				case Expr::Kind::STORE: {
					const Store& store = agent.getStore(stmt);
					see_expr(store.destination);
					see_expr(store.value);

					return false;
				} break;

				case Expr::Kind::CALC_PTR: {
					if(remove_unused_stmt()){ return true; }

					const CalcPtr& calc_ptr = agent.getCalcPtr(stmt);
					see_expr(calc_ptr.basePtr);

					for(const CalcPtr::Index& index : calc_ptr.indices){
						if(index.is<Expr>()){ see_expr(index.as<Expr>()); }
					}

					return false;
				} break;


				case Expr::Kind::MEMCPY: {
					const Memcpy& memcpy = agent.getMemcpy(stmt);
					see_expr(memcpy.dst);
					see_expr(memcpy.src);
					see_expr(memcpy.numBytes);
				} break;

				case Expr::Kind::MEMSET: {
					const Memset& memset = agent.getMemset(stmt);
					see_expr(memset.dst);
					see_expr(memset.value);
					see_expr(memset.numBytes);
				} break;


				case Expr::Kind::BIT_CAST: {
					if(remove_unused_stmt()){ return true; }

					const BitCast& bitcast = agent.getBitCast(stmt);
					see_expr(bitcast.fromValue);
				} break;

				case Expr::Kind::TRUNC: {
					if(remove_unused_stmt()){ return true; }

					const Trunc& trunc = agent.getTrunc(stmt);
					see_expr(trunc.fromValue);
				} break;

				case Expr::Kind::FTRUNC: {
					if(remove_unused_stmt()){ return true; }

					const FTrunc& ftrunc = agent.getFTrunc(stmt);
					see_expr(ftrunc.fromValue);
				} break;

				case Expr::Kind::SEXT: {
					if(remove_unused_stmt()){ return true; }

					const SExt& sext = agent.getSExt(stmt);
					see_expr(sext.fromValue);
				} break;

				case Expr::Kind::ZEXT: {
					if(remove_unused_stmt()){ return true; }

					const ZExt& zext = agent.getZExt(stmt);
					see_expr(zext.fromValue);
				} break;

				case Expr::Kind::FEXT: {
					if(remove_unused_stmt()){ return true; }

					const FExt& fext = agent.getFExt(stmt);
					see_expr(fext.fromValue);
				} break;

				case Expr::Kind::ITOF: {
					if(remove_unused_stmt()){ return true; }

					const IToF& itof = agent.getIToF(stmt);
					see_expr(itof.fromValue);
				} break;

				case Expr::Kind::UITOF: {
					if(remove_unused_stmt()){ return true; }

					const UIToF& uitof = agent.getUIToF(stmt);
					see_expr(uitof.fromValue);
				} break;

				case Expr::Kind::FTOI: {
					if(remove_unused_stmt()){ return true; }

					const FToI& ftoi = agent.getFToI(stmt);
					see_expr(ftoi.fromValue);
				} break;

				case Expr::Kind::FTOUI: {
					if(remove_unused_stmt()){ return true; }

					const FToUI& ftoui = agent.getFToUI(stmt);
					see_expr(ftoui.fromValue);
				} break;


				case Expr::Kind::ADD: {
					if(remove_unused_stmt()){ return true; }

					const Add& add = agent.getAdd(stmt);
					see_expr(add.lhs);
					see_expr(add.rhs);

					return false;
				} break;

				case Expr::Kind::SADD_WRAP: {
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
							sadd_wrap.lhs, sadd_wrap.rhs, false, false, std::string(sadd_wrap.resultName)
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

				case Expr::Kind::SADD_WRAP_RESULT:  return false;
				case Expr::Kind::SADD_WRAP_WRAPPED: return false;

				case Expr::Kind::UADD_WRAP: {
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
							uadd_wrap.lhs, uadd_wrap.rhs, false, false, std::string(uadd_wrap.resultName)
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

				case Expr::Kind::UADD_WRAP_RESULT:  return false;
				case Expr::Kind::UADD_WRAP_WRAPPED: return false;

				case Expr::Kind::SADD_SAT: {
					if(remove_unused_stmt()){ return true; }

					const SAddSat& sadd_sat = agent.getSAddSat(stmt);
					see_expr(sadd_sat.lhs);
					see_expr(sadd_sat.rhs);

					return false;
				} break;

				case Expr::Kind::UADD_SAT: {
					if(remove_unused_stmt()){ return true; }

					const UAddSat& uadd_sat = agent.getUAddSat(stmt);
					see_expr(uadd_sat.lhs);
					see_expr(uadd_sat.rhs);

					return false;
				} break;

				case Expr::Kind::FADD: {
					if(remove_unused_stmt()){ return true; }

					const FAdd& fadd = agent.getFAdd(stmt);
					see_expr(fadd.lhs);
					see_expr(fadd.rhs);

					return false;
				} break;


				case Expr::Kind::SUB: {
					if(remove_unused_stmt()){ return true; }

					const Sub& sub = agent.getSub(stmt);
					see_expr(sub.lhs);
					see_expr(sub.rhs);

					return false;
				} break;

				case Expr::Kind::SSUB_WRAP: {
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
							ssub_wrap.lhs, ssub_wrap.rhs, false, false, std::string(ssub_wrap.resultName)
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

				case Expr::Kind::SSUB_WRAP_RESULT:  return false;
				case Expr::Kind::SSUB_WRAP_WRAPPED: return false;

				case Expr::Kind::USUB_WRAP: {
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
							usub_wrap.lhs, usub_wrap.rhs, false, false, std::string(usub_wrap.resultName)
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

				case Expr::Kind::USUB_WRAP_RESULT:  return false;
				case Expr::Kind::USUB_WRAP_WRAPPED: return false;

				case Expr::Kind::SSUB_SAT: {
					if(remove_unused_stmt()){ return true; }

					const SSubSat& ssub_sat = agent.getSSubSat(stmt);
					see_expr(ssub_sat.lhs);
					see_expr(ssub_sat.rhs);

					return false;
				} break;

				case Expr::Kind::USUB_SAT: {
					if(remove_unused_stmt()){ return true; }

					const USubSat& usub_sat = agent.getUSubSat(stmt);
					see_expr(usub_sat.lhs);
					see_expr(usub_sat.rhs);

					return false;
				} break;

				case Expr::Kind::FSUB: {
					if(remove_unused_stmt()){ return true; }

					const FSub& fsub = agent.getFSub(stmt);
					see_expr(fsub.lhs);
					see_expr(fsub.rhs);

					return false;
				} break;


				case Expr::Kind::MUL: {
					if(remove_unused_stmt()){ return true; }

					const Mul& mul = agent.getMul(stmt);
					see_expr(mul.lhs);
					see_expr(mul.rhs);

					return false;
				} break;

				case Expr::Kind::SMUL_WRAP: {
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
							smul_wrap.lhs, smul_wrap.rhs, false, false, std::string(smul_wrap.resultName)
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

				case Expr::Kind::SMUL_WRAP_RESULT:  return false;
				case Expr::Kind::SMUL_WRAP_WRAPPED: return false;

				case Expr::Kind::UMUL_WRAP: {
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
							umul_wrap.lhs, umul_wrap.rhs, false, false, std::string(umul_wrap.resultName)
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

				case Expr::Kind::UMUL_WRAP_RESULT:  return false;
				case Expr::Kind::UMUL_WRAP_WRAPPED: return false;

				case Expr::Kind::SMUL_SAT: {
					if(remove_unused_stmt()){ return true; }

					const SMulSat& smul_sat = agent.getSMulSat(stmt);
					see_expr(smul_sat.lhs);
					see_expr(smul_sat.rhs);

					return false;
				} break;

				case Expr::Kind::UMUL_SAT: {
					if(remove_unused_stmt()){ return true; }

					const UMulSat& umul_sat = agent.getUMulSat(stmt);
					see_expr(umul_sat.lhs);
					see_expr(umul_sat.rhs);

					return false;
				} break;

				case Expr::Kind::FMUL: {
					if(remove_unused_stmt()){ return true; }

					const FMul& fmul = agent.getFMul(stmt);
					see_expr(fmul.lhs);
					see_expr(fmul.rhs);

					return false;
				} break;

				case Expr::Kind::SDIV: {
					if(remove_unused_stmt()){ return true; }

					const SDiv& sdiv = agent.getSDiv(stmt);
					see_expr(sdiv.lhs);
					see_expr(sdiv.rhs);

					return false;
				} break;

				case Expr::Kind::UDIV: {
					if(remove_unused_stmt()){ return true; }

					const UDiv udiv = agent.getUDiv(stmt);
					see_expr(udiv.lhs);
					see_expr(udiv.rhs);

					return false;
				} break;

				case Expr::Kind::FDIV: {
					if(remove_unused_stmt()){ return true; }

					const FDiv& fdiv = agent.getFDiv(stmt);
					see_expr(fdiv.lhs);
					see_expr(fdiv.rhs);

					return false;
				} break;


				case Expr::Kind::SREM: {
					if(remove_unused_stmt()){ return true; }

					const SRem& srem = agent.getSRem(stmt);
					see_expr(srem.lhs);
					see_expr(srem.rhs);

					return false;
				} break;

				case Expr::Kind::UREM: {
					if(remove_unused_stmt()){ return true; }

					const URem urem = agent.getURem(stmt);
					see_expr(urem.lhs);
					see_expr(urem.rhs);

					return false;
				} break;

				case Expr::Kind::FREM: {
					if(remove_unused_stmt()){ return true; }

					const FRem& frem = agent.getFRem(stmt);
					see_expr(frem.lhs);
					see_expr(frem.rhs);

					return false;
				} break;

				case Expr::Kind::FNEG: {
					if(remove_unused_stmt()){ return true; }

					const FNeg& fneg = agent.getFNeg(stmt);
					see_expr(fneg.rhs);

					return false;
				} break;

				case Expr::Kind::IEQ: {
					if(remove_unused_stmt()){ return true; }

					const IEq& ieq = agent.getIEq(stmt);
					see_expr(ieq.lhs);
					see_expr(ieq.rhs);

					return false;
				} break;

				case Expr::Kind::FEQ: {
					if(remove_unused_stmt()){ return true; }

					const FEq& feq = agent.getFEq(stmt);
					see_expr(feq.lhs);
					see_expr(feq.rhs);

					return false;
				} break;

				case Expr::Kind::INEQ: {
					if(remove_unused_stmt()){ return true; }

					const INeq& ineq = agent.getINeq(stmt);
					see_expr(ineq.lhs);
					see_expr(ineq.rhs);

					return false;
				} break;

				case Expr::Kind::FNEQ: {
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

				case Expr::Kind::AND: {
					if(remove_unused_stmt()){ return true; }

					const And& and_stmt = agent.getAnd(stmt);
					see_expr(and_stmt.lhs);
					see_expr(and_stmt.rhs);

					return false;
				} break;

				case Expr::Kind::OR: {
					if(remove_unused_stmt()){ return true; }

					const Or& or_stmt = agent.getOr(stmt);
					see_expr(or_stmt.lhs);
					see_expr(or_stmt.rhs);

					return false;
				} break;

				case Expr::Kind::XOR: {
					if(remove_unused_stmt()){ return true; }

					const Xor& xor_stmt = agent.getXor(stmt);
					see_expr(xor_stmt.lhs);
					see_expr(xor_stmt.rhs);

					return false;
				} break;

				case Expr::Kind::SHL: {
					if(remove_unused_stmt()){ return true; }

					const SHL& shl = agent.getSHL(stmt);
					see_expr(shl.lhs);
					see_expr(shl.rhs);

					return false;
				} break;

				case Expr::Kind::SSHL_SAT: {
					if(remove_unused_stmt()){ return true; }

					const SSHLSat& sshlsat = agent.getSSHLSat(stmt);
					see_expr(sshlsat.lhs);
					see_expr(sshlsat.rhs);

					return false;
				} break;

				case Expr::Kind::USHL_SAT: {
					if(remove_unused_stmt()){ return true; }

					const USHLSat& ushlsat = agent.getUSHLSat(stmt);
					see_expr(ushlsat.lhs);
					see_expr(ushlsat.rhs);

					return false;
				} break;

				case Expr::Kind::SSHR: {
					if(remove_unused_stmt()){ return true; }

					const SSHR& sshr = agent.getSSHR(stmt);
					see_expr(sshr.lhs);
					see_expr(sshr.rhs);

					return false;
				} break;

				case Expr::Kind::USHR: {
					if(remove_unused_stmt()){ return true; }

					const USHR& ushr = agent.getUSHR(stmt);
					see_expr(ushr.lhs);
					see_expr(ushr.rhs);

					return false;
				} break;

				case Expr::Kind::BIT_REVERSE: {
					if(remove_unused_stmt()){ return true; }

					const BitReverse& bit_reverse = agent.getBitReverse(stmt);
					see_expr(bit_reverse.arg);

					return false;
				} break;

				case Expr::Kind::BYTE_SWAP: {
					if(remove_unused_stmt()){ return true; }

					const ByteSwap& byte_swap = agent.getByteSwap(stmt);
					see_expr(byte_swap.arg);

					return false;
				} break;

				case Expr::Kind::CTPOP: {
					if(remove_unused_stmt()){ return true; }

					const CtPop& ctpop = agent.getCtPop(stmt);
					see_expr(ctpop.arg);

					return false;
				} break;


				case Expr::Kind::CTLZ: {
					if(remove_unused_stmt()){ return true; }

					const CTLZ& ctlz = agent.getCTLZ(stmt);
					see_expr(ctlz.arg);

					return false;
				} break;

				case Expr::Kind::CTTZ: {
					if(remove_unused_stmt()){ return true; }

					const CTTZ& cttz = agent.getCTTZ(stmt);
					see_expr(cttz.arg);

					return false;
				} break;

				case Expr::Kind::CMPXCHG: {
					const CmpXchg& cmpxchg = agent.getCmpXchg(stmt);
					see_expr(cmpxchg.target);
					see_expr(cmpxchg.expected);
					see_expr(cmpxchg.desired);

					return false;
				} break;

				case Expr::Kind::CMPXCHG_LOADED:    return false;
				case Expr::Kind::CMPXCHG_SUCCEEDED: return false;

				case Expr::Kind::ATOMIC_RMW: {
					const AtomicRMW& atomic_rmw = agent.getAtomicRMW(stmt);
					see_expr(atomic_rmw.target);
					see_expr(atomic_rmw.value);

					return false;
				} break;

				case Expr::Kind::LIFETIME_START:    return false;
				case Expr::Kind::LIFETIME_END:      return false;
			}

			evo::debugFatalBreak("Unknown or unsupported Expr::Kind ({})", evo::to_underlying(stmt.kind()));
		};

		return PassManager::ReverseStmtPass(impl);
	}


}