////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/PassManager.h"

#include "../include/Expr.h"
#include "../include/BasicBlock.h"
#include "../include/Module.h"
#include "../include/Agent.h"

#include <ranges>


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir{
	

	auto PassManager::run() -> evo::Result<> {
		if(this->max_threads == 0){
			this->runSingleThreaded();
			return evo::Result<>();
		}
		return this->run_multi_threaded();
	}

	auto PassManager::runSingleThreaded() -> void {
		for(const PassGroupVariant& pass_group_variant : this->pass_groups){
			pass_group_variant.visit([&](const auto& pass_group) -> void {
				this->run_single_threaded_pass_group(pass_group);
			});
		}
	}

	auto PassManager::run_multi_threaded() -> evo::Result<> {
		evo::debugAssert(this->max_threads != 0, "This pass manager is not allowed to run multi-threaded");

		if(this->pool.isRunning() == false){
			this->pool.startup(this->max_threads);
		}

		for(const PassGroupVariant& pass_group_variant : this->pass_groups){
			const evo::Result<> pass_result = pass_group_variant.visit([&](const auto& pass_group) -> evo::Result<> {
				return this->run_multi_threaded_pass_group(pass_group);
			});

			if(pass_result.isError()){ return evo::resultError; }
		}

		return evo::Result<>();
	}



	//////////////////////////////////////////////////////////////////////
	// stmt pass

	auto PassManager::run_single_threaded_pass_group(const StmtPassGroup& stmt_pass_group) -> void {
		auto agent = Agent(this->module);

		for(Function& func : this->module.getFunctionIter()){
			this->run_pass_group(stmt_pass_group, StmtPassGroupItem(func));
		}
	}

	auto PassManager::run_multi_threaded_pass_group(const StmtPassGroup& stmt_pass_group) -> evo::Result<> {
		auto agent = Agent(this->module);

		auto items = evo::SmallVector<ThreadPoolItem>();
		for(Function& func : this->module.getFunctionIter()){
			items.emplace_back(StmtPassGroupItem(func));
		}

		this->pool.work(std::move(items), [&](ThreadPoolItem& item) -> evo::Result<> {
			this->run_pass_group(stmt_pass_group, item.value.as<StmtPassGroupItem>());
			return evo::Result<>();
		});

		return this->pool.waitUntilDoneWorking();
	}


	auto PassManager::run_pass_group(const StmtPassGroup& stmt_pass_group, const StmtPassGroupItem& item) -> void {
		auto agent = Agent(this->module, item.func);

		for(auto iter = item.func.getAllocasRange().begin(); iter != item.func.getAllocasRange().end(); ++iter){
			for(const StmtPass& stmt_pass : stmt_pass_group.passes){
				if(stmt_pass.func(Expr(Expr::Kind::ALLOCA, iter.getID()), agent)){ break; }
			}
		}


		for(BasicBlock::ID basic_block_id : item.func){
			BasicBlock& basic_block = agent.getBasicBlock(basic_block_id);
			agent.setTargetBasicBlock(basic_block);

			size_t i = 0;
			auto iter = basic_block.begin();
			size_t basic_block_saved_size = basic_block.size();
			while(i < basic_block_saved_size){
				agent.setInsertIndex(i);

				bool made_transformation = false;
				for(const StmtPass& stmt_pass : stmt_pass_group.passes){
					if(stmt_pass.func(*iter, agent)){
						made_transformation = true;
						break;
					}
				}


				if(made_transformation){
					const size_t new_size = basic_block.size();

					if(new_size < basic_block_saved_size){ // removal
						basic_block_saved_size = new_size;

					}else if(new_size == basic_block_saved_size){ // replacement
						i += 1;

					}else{ // addition
						i += basic_block_saved_size - new_size + 1;
						basic_block_saved_size = new_size;
					}

					iter = basic_block.begin();
					std::advance(iter, i);

				}else{
					i += 1;
					std::advance(iter, 1);
				}
			}
		}
	}


	//////////////////////////////////////////////////////////////////////
	// reverse stmt pass

	auto PassManager::run_single_threaded_pass_group(const ReverseStmtPassGroup& stmt_pass_group) -> void {
		auto agent = Agent(this->module);

		for(Function& func : this->module.getFunctionIter()){
			this->run_pass_group(stmt_pass_group, ReverseStmtPassGroupItem(func));
		}
	}

	auto PassManager::run_multi_threaded_pass_group(const ReverseStmtPassGroup& stmt_pass_group) -> evo::Result<> {
		auto agent = Agent(this->module);

		auto items = evo::SmallVector<ThreadPoolItem>();
		for(Function& func : this->module.getFunctionIter()){
			items.emplace_back(ReverseStmtPassGroupItem(func));
		}

		this->pool.work(std::move(items), [&](ThreadPoolItem& item) -> evo::Result<> {
			this->run_pass_group(stmt_pass_group, item.value.as<ReverseStmtPassGroupItem>());
			return evo::Result<>();
		});

		return this->pool.waitUntilDoneWorking();
	}


	auto PassManager::run_pass_group(
		const ReverseStmtPassGroup& stmt_pass_group, const ReverseStmtPassGroupItem& item
	) -> void {
		auto agent = Agent(this->module, item.func);

		for(BasicBlock::ID basic_block_id : item.func | std::views::reverse){
			BasicBlock& basic_block = agent.getBasicBlock(basic_block_id);
			agent.setTargetBasicBlock(basic_block);

			for(ptrdiff_t i = basic_block.size() - 1; i >= 0; i-=1){
				agent.setInsertIndex(i);

				for(const ReverseStmtPass& stmt_pass : stmt_pass_group.passes){
					if(stmt_pass.func(basic_block[i], agent)){ break; }
				}
			}
		}


		{
			size_t current_allocas_range_size = item.func.getAllocasRange().size();

			auto iter = item.func.getAllocasRange().begin();
			while(iter != item.func.getAllocasRange().end()){
				for(const ReverseStmtPass& stmt_pass : stmt_pass_group.passes){
					if(stmt_pass.func(Expr(Expr::Kind::ALLOCA, iter.getID()), agent)){ break; }
				}

				if(item.func.getAllocasRange().size() != current_allocas_range_size){
					if(iter == item.func.getAllocasRange().end()){ break; }
					current_allocas_range_size = item.func.getAllocasRange().size();
				}
				++iter;
			}
		}
	}


}