//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/PassManager.h"

#include "../include/Expr.h"
#include "../include/BasicBlock.h"
#include "../include/Module.h"
#include "../include/Agent.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir{
	

	auto PassManager::run() -> bool {
		if(this->max_threads == 0){ return this->runSingleThreaded(); }
		return this->run_multi_threaded();
	}

	auto PassManager::runSingleThreaded() -> bool {
		for(const PassGroupVariant& pass_group_variant : this->pass_groups){
			const bool pass_result = pass_group_variant.visit([&](const auto& pass_group) -> bool {
				return this->run_single_threaded_pass_group(pass_group);
			});

			if(pass_result == false){ return false; }
		}

		return true;
	}

	auto PassManager::run_multi_threaded() -> bool {
		evo::debugAssert(this->max_threads != 0, "This pass manager is not allowed to run multi-threaded");

		if(this->pool.isRunning() == false){
			this->pool.startup(this->max_threads);
		}

		for(const PassGroupVariant& pass_group_variant : this->pass_groups){
			const bool pass_result = pass_group_variant.visit([&](const auto& pass_group) -> bool {
				return this->run_multi_threaded_pass_group(pass_group);
			});

			if(pass_result == false){ return false; }
		}

		return true;
	}




	auto PassManager::run_single_threaded_pass_group(const StmtPassGroup& stmt_pass_group) -> bool {
		auto agent = Agent(this->module);

		for(Function& func : this->module.getFunctionIter()){
			for(BasicBlock::ID basic_block_id : func){
				BasicBlock& basic_block = agent.getBasicBlock(basic_block_id);

				if(this->run_pass_group(stmt_pass_group, StmtPassGroupItem(basic_block, func)) == false){
					return false;
				}
			}
		}

		return true;
	}

	auto PassManager::run_multi_threaded_pass_group(const StmtPassGroup& stmt_pass_group) -> bool {
		auto agent = Agent(this->module);

		auto items = evo::SmallVector<ThreadPoolItem>();
		for(Function& func : this->module.getFunctionIter()){
			for(BasicBlock::ID basic_block_id : func){
				BasicBlock& basic_block = agent.getBasicBlock(basic_block_id);

				items.emplace_back(StmtPassGroupItem(basic_block, func));
			}
		}

		this->pool.work(std::move(items), [&](ThreadPoolItem& item) -> bool {
			return this->run_pass_group(stmt_pass_group, item.value.as<StmtPassGroupItem>());
		});

		return this->pool.waitUntilDoneWorking();
	}



	auto PassManager::run_pass_group(const StmtPassGroup& stmt_pass_group, const StmtPassGroupItem& item) -> bool {
		size_t basic_block_current_size = item.basic_block.size();

		auto agent = Agent(this->module, item.func, item.basic_block);

		auto iter = item.basic_block.begin(); 
		while(iter != item.basic_block.end()){
			for(const StmtPass& stmt_pass : stmt_pass_group.passes){
				if(stmt_pass.func(*iter, agent) == false){ return false; }

				if(item.basic_block.size() != basic_block_current_size){ break; }
			}

			if(item.basic_block.size() == basic_block_current_size){
				++iter;
			}else{
				// Note: don't have to worry about if was at the last elem of the block as
				// 		 it's illegal to not have a single terminator that's at the end
				basic_block_current_size = item.basic_block.size();
			}
		}

		return true;
	}


}