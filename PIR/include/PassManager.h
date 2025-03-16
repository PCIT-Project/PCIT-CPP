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

#include "./BasicBlock.h"
#include "./Expr.h"

namespace pcit::pir{


	class PassManager{
		public:
			using MadeTransformation = bool;

			struct StmtPass{
				using Func = std::function<MadeTransformation(Expr, const class Agent&)>;
				Func func;
			};

			struct StmtPassGroup{
				evo::SmallVector<StmtPass> passes{};

				StmtPassGroup(evo::SmallVector<StmtPass>&& _passes) : passes(std::move(_passes)) {}
				StmtPassGroup(const evo::SmallVector<StmtPass>& _passes) : passes(_passes) {}
				StmtPassGroup(StmtPass&& pass) : passes{std::move(pass)} {}
				StmtPassGroup(const StmtPass& pass) : passes{pass} {}
			};


			struct ReverseStmtPass{
				using Func = std::function<MadeTransformation(Expr, const class Agent&)>;
				Func func;
			};

			struct ReverseStmtPassGroup{
				evo::SmallVector<ReverseStmtPass> passes{};

				ReverseStmtPassGroup(evo::SmallVector<ReverseStmtPass>&& _passes) : passes(std::move(_passes)) {}
				ReverseStmtPassGroup(const evo::SmallVector<ReverseStmtPass>& _passes) : passes(_passes) {}
				ReverseStmtPassGroup(ReverseStmtPass&& pass) : passes{std::move(pass)} {}
				ReverseStmtPassGroup(const ReverseStmtPass& pass) : passes{pass} {}
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

			auto addPass(const ReverseStmtPassGroup& pass) -> void { this->pass_groups.emplace_back(pass); }
			auto addPass(ReverseStmtPassGroup&& pass) -> void { this->pass_groups.emplace_back(std::move(pass)); }

			EVO_NODISCARD auto run() -> bool;
			EVO_NODISCARD auto runSingleThreaded() -> bool;

		private:
			EVO_NODISCARD auto run_multi_threaded() -> bool;

			struct StmtPassGroupItem{
				class Function& func;

				auto operator=(const StmtPassGroupItem& rhs) -> StmtPassGroupItem& {
					std::destroy_at(this); // just in case destruction becomes non-trivial
					std::construct_at(this, rhs);
					return *this;
				}
			};
			EVO_NODISCARD auto run_single_threaded_pass_group(const StmtPassGroup& stmt_pass_group) -> bool;
			EVO_NODISCARD auto run_multi_threaded_pass_group(const StmtPassGroup& stmt_pass_group) -> bool;
			EVO_NODISCARD auto run_pass_group(const StmtPassGroup& stmt_pass_group, const StmtPassGroupItem& item)
				-> bool;


			struct ReverseStmtPassGroupItem{
				class Function& func;

				auto operator=(const ReverseStmtPassGroupItem& rhs) -> ReverseStmtPassGroupItem& {
					std::destroy_at(this); // just in case destruction becomes non-trivial
					std::construct_at(this, rhs);
					return *this;
				}
			};
			EVO_NODISCARD auto run_single_threaded_pass_group(const ReverseStmtPassGroup& stmt_pass_group) -> bool;
			EVO_NODISCARD auto run_multi_threaded_pass_group(const ReverseStmtPassGroup& stmt_pass_group) -> bool;
			EVO_NODISCARD auto run_pass_group(
				const ReverseStmtPassGroup& stmt_pass_group, const ReverseStmtPassGroupItem& item
			) -> bool;



		private:
			class Module& module;
			unsigned max_threads;

			using PassGroupVariant = evo::Variant<StmtPassGroup, ReverseStmtPassGroup>;
			evo::SmallVector<PassGroupVariant> pass_groups{};

			struct ThreadPoolItem{
				evo::Variant<StmtPassGroupItem, ReverseStmtPassGroupItem> value;
			};
			core::ThreadPool<ThreadPoolItem> pool{};
	};


}


