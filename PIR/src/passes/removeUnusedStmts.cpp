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
					case Expr::Kind::None:         evo::debugFatalBreak("Invalid expr");
					case Expr::Kind::GlobalValue:  break;
					case Expr::Kind::Number:       break;
					case Expr::Kind::ParamExpr:    break;
					case Expr::Kind::CallInst:     func_metadata.emplace(expr);
					case Expr::Kind::CallVoidInst: break;
					case Expr::Kind::RetInst:      break;
					case Expr::Kind::BrInst:       break;
					case Expr::Kind::Alloca:       func_metadata.emplace(expr);
					case Expr::Kind::Add:          func_metadata.emplace(expr);
				}
			};

			switch(stmt.getKind()){
				case Expr::Kind::None:        evo::debugFatalBreak("Invalid expr");
				case Expr::Kind::GlobalValue: return true;
				case Expr::Kind::Number:      return true;
				case Expr::Kind::ParamExpr:   return true;

				case Expr::Kind::CallInst: {
					// TODO: remove if func has no side-effects
					// if(func_metadata.contains(stmt) == false){
					// 	agent.removeStmt(stmt);
					// 	return true;
					// }

					const CallInst& call_inst = agent.getCallInst(stmt);

					if(call_inst.target.is<PtrCall>()){
						see_expr(call_inst.target.as<PtrCall>().location);
					}

					for(const Expr& arg : call_inst.args){
						see_expr(arg);
					}
				} break;

				case Expr::Kind::CallVoidInst: {
					const CallVoidInst& call_void_inst = agent.getCallVoidInst(stmt);

					if(call_void_inst.target.is<PtrCall>()){
						see_expr(call_void_inst.target.as<PtrCall>().location);
					}

					for(const Expr& arg : call_void_inst.args){
						see_expr(arg);
					}
				} break;

				case Expr::Kind::RetInst: {
					const RetInst& ret_inst = agent.getRetInst(stmt);

					if(ret_inst.value.has_value()){
						see_expr(*ret_inst.value);
					}
				} break;

				case Expr::Kind::BrInst: return true;

				case Expr::Kind::Alloca: {
					if(func_metadata.contains(stmt) == false){
						agent.removeStmt(stmt);
						return true;
					}
				} break;

				case Expr::Kind::Add: {
					if(func_metadata.contains(stmt) == false){
						agent.removeStmt(stmt);
						return true;
					}

					const Add& add = agent.getAdd(stmt);
					see_expr(add.lhs);
					see_expr(add.rhs);
				} break;
			}

			return true;
		};

		return PassManager::ReverseStmtPass(impl);
	}


}