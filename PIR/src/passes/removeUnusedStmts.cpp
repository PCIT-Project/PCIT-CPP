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

		auto impl = [metadata](Expr stmt, const Agent& agent) mutable -> bool {
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
					break; case Expr::Kind::None:           evo::debugFatalBreak("Invalid expr");
					break; case Expr::Kind::GlobalValue:    break;
					break; case Expr::Kind::Number:         break;
					break; case Expr::Kind::Boolean:        break;
					break; case Expr::Kind::ParamExpr:      break;
					break; case Expr::Kind::Call:           func_metadata.emplace(expr);
					break; case Expr::Kind::CallVoid:       evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::Ret:            evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::Branch:         evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::Alloca:         func_metadata.emplace(expr);
					break; case Expr::Kind::Load:           func_metadata.emplace(expr);
					break; case Expr::Kind::Store:          evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::CalcPtr:        func_metadata.emplace(expr);
					break; case Expr::Kind::Add:            func_metadata.emplace(expr);
					break; case Expr::Kind::AddWrap:        evo::debugFatalBreak("Should never see this expr kind");
					break; case Expr::Kind::AddWrapResult:  func_metadata.emplace(expr);
					break; case Expr::Kind::AddWrapWrapped: func_metadata.emplace(expr);
				}
			};

			switch(stmt.getKind()){
				case Expr::Kind::None:        evo::debugFatalBreak("Invalid expr");
				case Expr::Kind::GlobalValue: return false;
				case Expr::Kind::Number:      return false;
				case Expr::Kind::Boolean:     return false;
				case Expr::Kind::ParamExpr:   return false;

				case Expr::Kind::Call: {
					// TODO: remove if func has no side-effects
					// if(func_metadata.contains(stmt) == false){
					// 	agent.removeStmt(stmt);
					// 	return false;
					// }

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

				case Expr::Kind::Ret: {
					const Ret& ret_inst = agent.getRet(stmt);

					if(ret_inst.value.has_value()){
						see_expr(*ret_inst.value);
					}

					return false;
				} break;

				case Expr::Kind::Branch: return false;

				case Expr::Kind::Alloca: {
					if(func_metadata.contains(stmt) == false){
						agent.removeStmt(stmt);
						return true;
					}

					return false;
				} break;


				case Expr::Kind::Load: {
					if(func_metadata.contains(stmt) == false){
						agent.removeStmt(stmt);
						return true;
					}

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
					if(func_metadata.contains(stmt) == false){
						agent.removeStmt(stmt);
						return true;
					}

					const CalcPtr& calc_ptr = agent.getCalcPtr(stmt);
					see_expr(calc_ptr.basePtr);

					for(const CalcPtr::Index& index : calc_ptr.indices){
						if(index.is<Expr>()){ see_expr(index.as<Expr>()); }
					}

					return false;
				} break;

				case Expr::Kind::Add: {
					if(func_metadata.contains(stmt) == false){
						agent.removeStmt(stmt);
						return true;
					}

					const Add& add = agent.getAdd(stmt);
					see_expr(add.lhs);
					see_expr(add.rhs);

					return false;
				} break;

				case Expr::Kind::AddWrap: {
					if(func_metadata.contains(agent.extractAddWrapWrapped(stmt)) == false){
						if(func_metadata.contains(agent.extractAddWrapResult(stmt)) == false){
							agent.removeStmt(stmt);
							return true;		
						}

						// if wrapped value is never used, replace addWrap with just an add
						const AddWrap& add_wrap = agent.getAddWrap(stmt);
						see_expr(add_wrap.lhs);
						see_expr(add_wrap.rhs);

						const Expr new_add = agent.createAdd(
							add_wrap.lhs, add_wrap.rhs, true, std::string(add_wrap.resultName)
						);
						agent.replaceExpr(agent.extractAddWrapResult(stmt), new_add);
						agent.removeStmt(stmt);
						return true;
					}

					const AddWrap& add_wrap = agent.getAddWrap(stmt);
					see_expr(add_wrap.lhs);
					see_expr(add_wrap.rhs);

					return false;
				} break;

				case Expr::Kind::AddWrapResult:  return false;
				case Expr::Kind::AddWrapWrapped: return false;
			}

			evo::debugFatalBreak("Unknown or unsupported Expr::Kind ({})", evo::to_underlying(stmt.getKind()));
		};

		return PassManager::ReverseStmtPass(impl);
	}


}