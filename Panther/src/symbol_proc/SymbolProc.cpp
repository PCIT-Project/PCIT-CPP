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

	// TODO(PERF): Is there a faster way of doing this than just locking the whole thing?
	static core::SpinLock wait_on_if_needed_lock{};
	

	auto SymbolProc::waitOnDeclIfNeeded(ID id, Context& context, ID self_id) -> WaitOnResult {
		const auto wait_on_lock = std::scoped_lock(wait_on_if_needed_lock);
		
		// TODO(PERF): is doing these checks before taking locks faster
		if(this->decl_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		if(id == self_id){
			context.emitError(
				Diagnostic::Code::SYMBOL_PROC_CIRCULAR_DEP,
				Diagnostic::Location::get(this->ast_node, context.getSourceManager()[this->source_id]),
				"Detected a circular dependency when analyzing this symbol:",
				Diagnostic::Info("Self dependency")
			);
			return WaitOnResult::CIRCULAR_DEP_DETECTED;
		}

		if(this->detect_circular_dependency(id, context, DependencyKind::DEF) == false){
			return WaitOnResult::CIRCULAR_DEP_DETECTED;
		}

		SymbolProc& waiting_symbol = context.symbol_proc_manager.getSymbolProc(id);

		const auto lock = std::scoped_lock(this->decl_waited_on_lock, waiting_symbol.waiting_for_lock);

		if(this->decl_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		this->decl_waited_on_by.emplace_back(id);
		waiting_symbol.waiting_for.emplace_back(self_id);

		return WaitOnResult::WAITING;
	}

	auto SymbolProc::waitOnPIRDeclIfNeeded(ID id, Context& context, ID self_id) -> WaitOnResult {
		const auto wait_on_lock = std::scoped_lock(wait_on_if_needed_lock);

		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		SymbolProc& waiting_symbol = context.symbol_proc_manager.getSymbolProc(id);

		const auto lock = std::scoped_lock(this->pir_decl_waited_on_lock, waiting_symbol.waiting_for_lock);

		if(this->pir_decl_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		this->pir_decl_waited_on_by.emplace_back(id);
		waiting_symbol.waiting_for.emplace_back(self_id);

		return WaitOnResult::WAITING;
	}

	auto SymbolProc::waitOnDefIfNeeded(ID id, Context& context, ID self_id) -> WaitOnResult {
		const auto wait_on_lock = std::scoped_lock(wait_on_if_needed_lock);

		if(this->def_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		if(id == self_id){
			context.emitError(
				Diagnostic::Code::SYMBOL_PROC_CIRCULAR_DEP,
				Diagnostic::Location::get(this->ast_node, context.getSourceManager()[this->source_id]),
				"Detected a circular dependency when analyzing this symbol:",
				Diagnostic::Info("Self dependency")
			);
			return WaitOnResult::CIRCULAR_DEP_DETECTED;
		}

		if(this->detect_circular_dependency(id, context, DependencyKind::DEF) == false){
			return WaitOnResult::CIRCULAR_DEP_DETECTED;
		}

		SymbolProc& waiting_symbol = context.symbol_proc_manager.getSymbolProc(id);

		const auto lock = std::scoped_lock(this->def_waited_on_lock, waiting_symbol.waiting_for_lock);

		if(this->def_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		this->def_waited_on_by.emplace_back(id);
		waiting_symbol.waiting_for.emplace_back(self_id);

		return WaitOnResult::WAITING;
	}


	auto SymbolProc::waitOnPIRDefIfNeeded(ID id, Context& context, ID self_id) -> WaitOnResult {
		const auto wait_on_lock = std::scoped_lock(wait_on_if_needed_lock);

		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		SymbolProc& waiting_symbol = context.symbol_proc_manager.getSymbolProc(id);

		const auto lock = std::scoped_lock(this->pir_decl_waited_on_lock, waiting_symbol.waiting_for_lock);

		if(this->pir_def_done){ return WaitOnResult::NOT_NEEDED; }
		if(this->status == Status::PASSED_ON_BY_WHEN_COND){ return WaitOnResult::WAS_PASSED_ON_BY_WHEN_COND; }
		if(this->status == Status::ERRORED){ return WaitOnResult::WAS_ERRORED; }

		this->pir_def_waited_on_by.emplace_back(id);
		waiting_symbol.waiting_for.emplace_back(self_id);

		return WaitOnResult::WAITING;
	}



	auto SymbolProc::detect_circular_dependency(ID id, Context& context, DependencyKind initial_dependency_kind) const
	-> bool {
		auto visited_queue = std::queue<ID>();

		{
			const auto lock = std::scoped_lock(this->waiting_for_lock);
			for(const ID& waiting_for_id : this->waiting_for){
				visited_queue.push(waiting_for_id);
			}
		}
		
		while(visited_queue.empty() == false){
			const ID visited_id = visited_queue.front();
			visited_queue.pop();

			const SymbolProc& visited = context.symbol_proc_manager.getSymbolProc(visited_id);

			if(visited_id == id){
				this->emit_diagnostic_on_circular_dependency(id, context, initial_dependency_kind);
				return false;
			}


			{
				const auto lock = std::scoped_lock(visited.waiting_for_lock);
				for(const ID& waiting_for_id : visited.waiting_for){
					visited_queue.push(waiting_for_id);
				}
			}
		}

		return true;
	}




	// This is a separate function from `detect_circular_dependency` since the computation required to have the 
	//     extra information is much slower, so only do the slow case if a circular dependency is known to exist
	auto SymbolProc::emit_diagnostic_on_circular_dependency(
		ID id, Context& context, DependencyKind initial_dependency_kind
	) const -> void {
		struct VisitedInfo{
			ID id;
			DependencyKind kind;
		};


		auto visited_queue = std::queue<VisitedInfo>();

		{
			const auto lock = std::scoped_lock(this->waiting_for_lock);
			for(const ID& waiting_for_id : this->waiting_for){
				visited_queue.emplace(waiting_for_id, initial_dependency_kind);
			}
		}
		
		while(true){
			const VisitedInfo visited_info = visited_queue.front();
			visited_queue.pop();

			const SymbolProc& visited = context.symbol_proc_manager.getSymbolProc(visited_info.id);

			if(visited_info.id == id){
				auto infos = evo::SmallVector<Diagnostic::Info>();

				switch(visited_info.kind){
					case DependencyKind::DECL: {
						infos.emplace_back(
							"Requries declaration of this symbol:",
							Diagnostic::Location::get(this->ast_node, context.getSourceManager()[this->source_id])
						);
					} break;

					case DependencyKind::DEF: {
						infos.emplace_back(
							"Requries definition of this symbol:",
							Diagnostic::Location::get(this->ast_node, context.getSourceManager()[this->source_id])
						);
					} break;
				}

				context.emitError(
					Diagnostic::Code::SYMBOL_PROC_CIRCULAR_DEP,
					Diagnostic::Location::get(visited.ast_node, context.getSourceManager()[visited.source_id]),
					"Detected a circular dependency when analyzing this symbol:",
					std::move(infos)
				);

				return;
			}


			{
				const auto lock = std::scoped_lock(visited.waiting_for_lock);
				for(const ID& waiting_for_id : visited.waiting_for){
					const SymbolProc& waiting_for_symbol = context.symbol_proc_manager.getSymbolProc(waiting_for_id);

					{ // declaration
						const auto decl_lock = std::scoped_lock(waiting_for_symbol.decl_waited_on_lock);

						if(
							std::ranges::find(waiting_for_symbol.decl_waited_on_by, visited_info.id)
							!= waiting_for_symbol.decl_waited_on_by.end()
						){
							visited_queue.emplace(waiting_for_id, DependencyKind::DECL);
							continue;
						}
					}


					{ // definition
						const auto def_lock = std::scoped_lock(waiting_for_symbol.def_waited_on_lock);

						if(
							std::ranges::find(waiting_for_symbol.def_waited_on_by, visited_info.id)
							!= waiting_for_symbol.def_waited_on_by.end()
						){
							visited_queue.emplace(waiting_for_id, DependencyKind::DEF);
							continue;
						}
					}

				}
			}
			
		}

	}



}