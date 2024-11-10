//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <PCIT_core.h>

#include "./BasicBlock.h"

namespace pcit::pir{


	class PassManager{
		public:
			struct StmtPass{
				using Func = std::function<bool(class Expr&, const class Agent&)>;
				Func func;
			};

			struct StmtPassGroup{
				evo::SmallVector<StmtPass> passes{};

				StmtPassGroup(evo::SmallVector<StmtPass>&& _passes) : passes(std::move(_passes)) {}
				StmtPassGroup(const evo::SmallVector<StmtPass>& _passes) : passes(_passes) {}
				StmtPassGroup(StmtPass&& pass) : passes{std::move(pass)} {}
				StmtPassGroup(const StmtPass& pass) : passes{pass} {}
			};


		public:
			PassManager(class Module& _module, unsigned max_num_threads)
				: module(_module), max_threads(max_num_threads) {}
			~PassManager() {
				if(this->pool.isRunning()){
					this->pool.shutdown();
				}
			}

			EVO_NODISCARD static auto optimalNumThreads() -> unsigned {
				return unsigned(core::ThreadPool<ThreadPoolItem>::optimalNumThreads());
			}


			auto addPass(const StmtPassGroup& pass) -> void { this->pass_groups.emplace_back(pass); }
			auto addPass(StmtPassGroup&& pass) -> void { this->pass_groups.emplace_back(std::move(pass)); }

			EVO_NODISCARD auto run() -> bool;
			EVO_NODISCARD auto runSingleThreaded() -> bool;

		private:
			EVO_NODISCARD auto run_multi_threaded() -> bool;

			struct StmtPassGroupItem{
				Function& func;
			};
			EVO_NODISCARD auto run_single_threaded_pass_group(const StmtPassGroup& stmt_pass_group) -> bool;
			EVO_NODISCARD auto run_multi_threaded_pass_group(const StmtPassGroup& stmt_pass_group) -> bool;
			EVO_NODISCARD auto run_pass_group(const StmtPassGroup& stmt_pass_group, const StmtPassGroupItem& item)
				-> bool;



		private:
			class Module& module;
			unsigned max_threads;

			using PassGroupVariant = evo::Variant<StmtPassGroup>;
			evo::SmallVector<PassGroupVariant> pass_groups{};

			struct ThreadPoolItem{
				evo::Variant<StmtPassGroupItem> value;
			};
			core::ThreadPool<ThreadPoolItem> pool{};
	};


}


