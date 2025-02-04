////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./SymbolProc.h"


namespace pcit::panther{

	
	class SymbolProcManager{
		public:
			SymbolProcManager() = default;
			~SymbolProcManager() = default;


			EVO_NODISCARD auto getSymbolProc(SymbolProc::ID id) const -> const SymbolProc& {
				return this->symbol_procs[id];
			};

			EVO_NODISCARD auto getSymbolProc(SymbolProc::ID id) -> SymbolProc& {
				return this->symbol_procs[id];
			};

			using SymbolProcIter = core::IterRange<core::SyncLinearStepAlloc<SymbolProc, SymbolProc::ID>::ConstIter>;
			EVO_NODISCARD auto iterSymbolProcs() -> SymbolProcIter {
				return core::IterRange(this->symbol_procs.cbegin(), this->symbol_procs.cend());
			}


			EVO_NODISCARD auto allProcsDone() const -> bool { return this->num_procs_not_done == 0; }
			EVO_NODISCARD auto notAllProcsDone() const -> bool { return this->num_procs_not_done != 0; }


		private:
			auto create_symbol_proc(auto&&... args) -> SymbolProc::ID {
				this->num_procs_not_done += 1;
				return this->symbol_procs.emplace_back(std::forward<decltype(args)>(args)...);
			}

			auto symbol_proc_done() -> void {
				this->num_procs_not_done -= 1;
			}

	
		private:
			core::SyncLinearStepAlloc<SymbolProc, SymbolProc::ID> symbol_procs{};

			std::atomic<size_t> num_procs_not_done = 0;

			friend class SymbolProcBuilder;
			friend class SemanticAnalyzer;
			friend class SymbolProc;
	};


}
