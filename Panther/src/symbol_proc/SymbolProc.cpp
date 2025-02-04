////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SymbolProc.h"

#include <queue>

#include "../../include/Context.h"



#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther{

	// Is there a faster way of doing this than just locking the whole thing?
	static core::SpinLock wait_on_if_needed_lock{};
	

	EVO_NODISCARD auto SymbolProc::waitOnDeclIfNeeded(ID id, Context& context, ID self_id) -> WaitOnResult {
		const auto wait_on_lock = std::scoped_lock(wait_on_if_needed_lock);

		const auto lock = std::scoped_lock(this->waiting_lock);
		
		if(this->decl_done){ return WaitOnResult::NotNeeded; }
		if(this->passed_on_by_when_cond){ return WaitOnResult::WasPassedOnByWhenCond; }
		if(this->errored){ return WaitOnResult::WasErrored; }


		if(this->detect_circular_dependency(id, context) == false){ return  WaitOnResult::CircularDepDetected;; }
		this->decl_waited_on_by.emplace_back(id);
		context.symbol_proc_manager.getSymbolProc(id).waiting_for.emplace_back(self_id);

		return WaitOnResult::Waiting;
	}

	EVO_NODISCARD auto SymbolProc::waitOnDefIfNeeded(ID id, Context& context, ID self_id) -> WaitOnResult {
		const auto wait_on_lock = std::scoped_lock(wait_on_if_needed_lock);

		const auto lock = std::scoped_lock(this->waiting_lock);
		
		if(this->def_done){ return WaitOnResult::NotNeeded; }
		if(this->passed_on_by_when_cond){ return WaitOnResult::WasPassedOnByWhenCond; }
		if(this->errored){ return WaitOnResult::WasErrored; }


		if(this->detect_circular_dependency(id, context) == false){ return WaitOnResult::CircularDepDetected; }
		this->def_waited_on_by.emplace_back(id);
		context.symbol_proc_manager.getSymbolProc(id).waiting_for.emplace_back(self_id);

		return WaitOnResult::Waiting;
	}


	auto SymbolProc::detect_circular_dependency(ID id, Context& context) const -> bool {
		auto visited_queue = std::queue<ID>();

		{
			for(const ID& waited_for_id : this->waiting_for){
				visited_queue.push(waited_for_id);
			}
		}
		
		while(visited_queue.empty() == false){
			const ID visited_id = visited_queue.front();
			visited_queue.pop();

			const SymbolProc& visited = context.symbol_proc_manager.getSymbolProc(visited_id);

			if(visited_id == id){
				context.emitError(
					Diagnostic::Code::SymbolProcCircularDep,
					Diagnostic::Location::get(this->ast_node, context.getSourceManager()[this->source_id]),
					"Detected a circular dependency when analyzing this symbol:",
					Diagnostic::Info(
						"Requires this symbol:",
						Diagnostic::Location::get(visited.ast_node, context.getSourceManager()[visited.source_id])
					)
				);
				return false;
			}


			{
				const auto lock = std::scoped_lock(visited.waiting_lock);
				for(const ID& waited_for_id : visited.waiting_for){
					visited_queue.push(waited_for_id);
				}
			}
		}

		return true;
	}

}