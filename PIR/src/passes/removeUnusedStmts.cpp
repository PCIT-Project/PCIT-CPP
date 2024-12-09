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
					break; case Expr::Kind::FAdd:            func_metadata.emplace(expr);
					break; case Expr::Kind::SAddWrap:        evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::SAddWrapResult:  func_metadata.emplace(expr);
					break; case Expr::Kind::SAddWrapWrapped: func_metadata.emplace(expr);
					break; case Expr::Kind::UAddWrap:        evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::UAddWrapResult:  func_metadata.emplace(expr);
					break; case Expr::Kind::UAddWrapWrapped: func_metadata.emplace(expr);
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

				case Expr::Kind::FAdd: {
					if(remove_unused_stmt()){ return true; }

					const FAdd& fadd = agent.getFAdd(stmt);
					see_expr(fadd.lhs);
					see_expr(fadd.rhs);

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
			}

			evo::debugFatalBreak("Unknown or unsupported Expr::Kind ({})", evo::to_underlying(stmt.getKind()));
		};

		return PassManager::ReverseStmtPass(impl);
	}


}