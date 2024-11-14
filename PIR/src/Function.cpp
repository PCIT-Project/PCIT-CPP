////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Function.h"

#include "../include/Module.h"

#include "../include/Agent.h"
#include "../include/ReaderAgent.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir{
	
	
	auto Function::append_basic_block(BasicBlock::ID id) -> void {
		evo::debugAssert(!this->basic_block_is_already_in(id), "Basic block is already in this funciton");
		this->basic_blocks.emplace_back(id);
	}


	auto Function::insert_basic_block_before(BasicBlock::ID id, BasicBlock::ID before) -> void {
		evo::debugAssert(!this->basic_block_is_already_in(id), "Basic block is already in this funciton");

		for(auto iter = this->basic_blocks.begin(); iter != this->basic_blocks.end(); ++iter){
			if(*iter == before){
				this->basic_blocks.insert(iter, id);
				return;
			}
		}

		evo::debugFatalBreak("Before basic block is not in this function");
	}

	auto Function::insert_basic_block_after(BasicBlock::ID id, BasicBlock::ID after) -> void {
		evo::debugAssert(!this->basic_block_is_already_in(id), "Basic block is already in this funciton");
		
		for(auto iter = this->basic_blocks.begin(); iter != this->basic_blocks.end(); ++iter){
			if(*iter == after){
				this->basic_blocks.insert(++iter, id);
				return;
			}
		}

		evo::debugFatalBreak("After basic block is not in this function");
	}






	auto Function::basic_block_is_already_in(BasicBlock::ID id) const -> bool {
		for(const BasicBlock::ID& basic_block_id : this->basic_blocks){
			if(basic_block_id == id){ return true; }
		}

		return false;
	}

	
	auto Function::check_func_call_args(Function::ID id, evo::ArrayProxy<Expr> args) const -> bool {
		const Function& func = this->parent_module.getFunction(id);

		auto param_types = evo::SmallVector<Type>();
		for(const Parameter& param : func.getParameters()){
			param_types.emplace_back(param.getType());
		}

		return this->check_func_call_args(param_types, args);
	}

	auto Function::check_func_call_args(FunctionDecl::ID id, evo::ArrayProxy<Expr> args) const -> bool {
		const FunctionDecl& target_func_decl = this->parent_module.getFunctionDecl(id);

		auto param_types = evo::SmallVector<Type>();
		for(const Parameter& param : target_func_decl.parameters){
			param_types.emplace_back(param.getType());
		}

		return this->check_func_call_args(param_types, args);
	}

	auto Function::check_func_call_args(Type func_type, evo::ArrayProxy<Expr> args) const -> bool {
		return this->check_func_call_args(this->parent_module.getFunctionType(func_type).parameters, args);
	}

	auto Function::check_func_call_args(evo::ArrayProxy<Type> param_types, evo::ArrayProxy<Expr> args) const -> bool {
		if(param_types.size() != args.size()){ return false; }

		for(size_t i = 0; i < param_types.size(); i+=1){
			if(param_types[i] != ReaderAgent(this->getParentModule(), *this).getExprType(args[i])){ return false; }
		}

		return true;
	}
	

}