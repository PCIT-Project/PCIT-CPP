////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Agent.h"

#include "../include/ReaderAgent.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir{

	//////////////////////////////////////////////////////////////////////
	// targets

	auto Agent::setTargetFunction(Function::ID id) -> void {
		this->target_func = &this->module.getFunction(id);
		this->target_basic_block = nullptr;
	}

	auto Agent::setTargetFunction(Function& func) -> void {
		this->target_func = &func;
		this->target_basic_block = nullptr;
	}

	auto Agent::removeTargetFunction() -> void {
		this->target_func = nullptr;
		this->target_basic_block = nullptr;
	}

	auto Agent::getTargetFunction() const -> Function& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		return *this->target_func;
	}


	auto Agent::setTargetBasicBlock(BasicBlock::ID id) -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		// TODO: check that block is in function
		this->target_basic_block = &this->getBasicBlock(id);
	}

	auto Agent::setTargetBasicBlock(BasicBlock& basic_block) -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		// TODO: check that block is in function
		this->target_basic_block = &basic_block;
	}

	auto Agent::removeTargetBasicBlock() -> void {
		this->target_basic_block = nullptr;
	}

	auto Agent::getTargetBasicBlock() const -> BasicBlock& {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		return *this->target_basic_block;
	}


	//////////////////////////////////////////////////////////////////////
	// misc expr stuff

	auto Agent::getExprType(const Expr& expr) const -> Type {
		if(this->hasTargetFunction()){
			return ReaderAgent(this->module, this->getTargetFunction()).getExprType(expr);
		}else{
			return ReaderAgent(this->module).getExprType(expr);
		}
	}


	auto Agent::replaceExpr(Expr original, const Expr& replacement) const -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");

		struct OriginalLocation{
			BasicBlock::ID basic_block_id;
			size_t index;
		};
		auto original_location = std::optional<OriginalLocation>();

		for(const BasicBlock::ID& basic_block_id : *this->target_func){
			BasicBlock& basic_block = this->getBasicBlock(basic_block_id);
			for(size_t i = 0; Expr& stmt : basic_block){
				EVO_DEFER([&](){ i += 1; });

				if(stmt == original){
					if(replacement.isStmt()){
						stmt = replacement;
					}else{
						original_location.emplace(basic_block_id, i);
					}
					continue;
				}

				switch(stmt.getKind()){
					case Expr::Kind::None: evo::debugFatalBreak("Invalid stmt");
					
					case Expr::Kind::GlobalValue: continue;
					
					case Expr::Kind::Number: continue;

					case Expr::Kind::ParamExpr: continue;
					
					case Expr::Kind::CallInst: {
						CallInst& call_inst = this->target_func->calls[stmt.index];

						if(call_inst.target.is<PtrCall>() && call_inst.target.as<PtrCall>().location == original){
							call_inst.target.as<PtrCall>().location = replacement;
						}

						for(Expr& arg : call_inst.args){
							if(arg == original){ arg = replacement; }
						}
					} break;
					
					case Expr::Kind::CallVoidInst: {
						CallVoidInst& call_void_inst = this->target_func->call_voids[stmt.index];

						if(
							call_void_inst.target.is<PtrCall>() &&
							call_void_inst.target.as<PtrCall>().location == original
						){
							call_void_inst.target.as<PtrCall>().location = replacement;
						}

						for(Expr& arg : call_void_inst.args){
							if(arg == original){ arg = replacement; }
						}
					} break;
					
					case Expr::Kind::RetInst: {
						RetInst& ret_inst = this->target_func->rets[stmt.index];

						if(ret_inst.value.has_value() && *ret_inst.value == original){
							ret_inst.value.emplace(replacement);
						}
					} break;
					
					case Expr::Kind::BrInst: continue;
					
					case Expr::Kind::Add: {
						Add& add = this->target_func->adds[stmt.index];

						if(add.lhs == original){ add.lhs = replacement; }
						if(add.rhs == original){ add.rhs = replacement; }
					} break;
				}
			}
		}

		if(original_location.has_value()){
			BasicBlock& basic_block = this->getBasicBlock(original_location->basic_block_id);
			basic_block.remove(original_location->index);
		}

		this->delete_expr(original);
	}

	
	//////////////////////////////////////////////////////////////////////
	// basic blocks

	auto Agent::createBasicBlock(std::string&& name, Function::ID func) const -> BasicBlock::ID {
		return this->createBasicBlock(std::move(name), this->module.getFunction(func));
	}

	auto Agent::createBasicBlock(std::string&& name, Function& func) const -> BasicBlock::ID {
		const pcit::pir::BasicBlock::ID new_block_id = this->module.basic_blocks.emplace_back(std::move(name));
		func.append_basic_block(new_block_id);
		return new_block_id;
	}

	auto Agent::createBasicBlock(std::string&& name) const -> BasicBlock::ID {
		evo::debugAssert(this->hasTargetFunction(), "Cannot use this function as there is no function target set");
		return this->createBasicBlock(std::move(name), *this->target_func);
	}

	auto Agent::getBasicBlock(BasicBlock::ID id) const -> BasicBlock& {
		return this->module.basic_blocks[id];
	}


	//////////////////////////////////////////////////////////////////////
	// numbers

	auto Agent::createNumber(const Type& type, core::GenericInt&& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.isIntegral(), "Type and value must both be integral or both be floating");

		return Expr(Expr::Kind::Number, this->module.numbers.emplace_back(type, std::move(value)));
	}

	auto Agent::createNumber(const Type& type, const core::GenericInt& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.isIntegral(), "Type and value must both be integral or both be floating");

		return Expr(Expr::Kind::Number, this->module.numbers.emplace_back(type, value));
	}

	auto Agent::createNumber(const Type& type, core::GenericFloat&& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.isFloat(), "Type and value must both be integral or both be floating");

		return Expr(Expr::Kind::Number, this->module.numbers.emplace_back(type, std::move(value)));
	}

	auto Agent::createNumber(const Type& type, const core::GenericFloat& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.isFloat(), "Type and value must both be integral or both be floating");

		return Expr(Expr::Kind::Number, this->module.numbers.emplace_back(type, value));
	}

	auto Agent::getNumber(const Expr& expr) const -> const Number& {
		return ReaderAgent(this->module).getNumber(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// param exprs

	auto Agent::createParamExpr(uint32_t index) -> Expr {
		return Expr(Expr::Kind::ParamExpr, index);
	}

	auto Agent::getParamExpr(const Expr& expr) -> ParamExpr {
		return ReaderAgent::getParamExpr(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// calls

	auto Agent::createCallInst(std::string&& name, Function::ID func, evo::SmallVector<Expr>&& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallInst, this->target_func->calls.emplace_back(std::move(name), func, std::move(args))
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCallInst(std::string&& name, Function::ID func, const evo::SmallVector<Expr>& args) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallInst, this->target_func->calls.emplace_back(std::move(name), func, args)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	auto Agent::createCallInst(std::string&& name, FunctionDecl::ID func, evo::SmallVector<Expr>&& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallInst, this->target_func->calls.emplace_back(std::move(name), func, std::move(args))
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCallInst(std::string&& name, FunctionDecl::ID func, const evo::SmallVector<Expr>& args) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallInst, this->target_func->calls.emplace_back(std::move(name), func, args)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	auto Agent::createCallInst(
		std::string&& name, const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args
	) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getTypeFunction(func_type).returnType.getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallInst,
			this->target_func->calls.emplace_back(std::move(name), PtrCall(func, func_type), std::move(args))
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCallInst(
		std::string&& name, const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args
	) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getTypeFunction(func_type).returnType.getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallInst, this->target_func->calls.emplace_back(std::move(name), PtrCall(func, func_type), args)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	//////////////////////////////////////////////////////////////////////
	// call voids

	auto Agent::createCallVoidInst(Function::ID func, evo::SmallVector<Expr>&& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoidInst, this->target_func->call_voids.emplace_back(func, std::move(args))
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoidInst(Function::ID func, const evo::SmallVector<Expr>& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(Expr::Kind::CallVoidInst, this->target_func->call_voids.emplace_back(func, args));
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	auto Agent::createCallVoidInst(FunctionDecl::ID func, evo::SmallVector<Expr>&& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoidInst, this->target_func->call_voids.emplace_back(func, std::move(args))
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoidInst(FunctionDecl::ID func, const evo::SmallVector<Expr>& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(Expr::Kind::CallVoidInst, this->target_func->call_voids.emplace_back(func, args));
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	auto Agent::createCallVoidInst(const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getTypeFunction(func_type).returnType.getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoidInst,
			this->target_func->call_voids.emplace_back(PtrCall(func, func_type), std::move(args))
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoidInst(const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getTypeFunction(func_type).returnType.getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoidInst, this->target_func->call_voids.emplace_back(PtrCall(func, func_type), args)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	//////////////////////////////////////////////////////////////////////
	// ret instructions

	auto Agent::createRetInst(const Expr& expr) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(expr.isValue(), "Must return value");
		evo::debugAssert(
			this->getExprType(expr) == this->target_func->getReturnType(), "Return type must match function"
		);

		return Expr(Expr::Kind::RetInst, this->target_func->rets.emplace_back(expr));
	}

	auto Agent::createRetInst() const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->target_func->getReturnType().getKind() == Type::Kind::Void, "Return type must match"
		);

		return Expr(Expr::Kind::RetInst, this->target_func->rets.emplace_back(std::nullopt));
	}

	auto Agent::getRetInst(const Expr& expr) const -> const RetInst& {
		return ReaderAgent(this->module, this->getTargetFunction()).getRetInst(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// br instructions

	auto Agent::createBrInst(BasicBlock::ID basic_block_id) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");

		const auto new_expr = Expr(Expr::Kind::BrInst, basic_block_id.get());
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::getBrInst(const Expr& expr) -> BrInst {
		return ReaderAgent::getBrInst(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// add

	auto Agent::createAdd(std::string&& name, const Expr& lhs, const Expr& rhs, bool may_wrap) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");

		const auto new_expr = Expr(
			Expr::Kind::Add, this->target_func->adds.emplace_back(std::move(name), lhs, rhs, may_wrap)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::getAdd(const Expr& expr) const -> const Add& {
		return ReaderAgent(this->module, this->getTargetFunction()).getAdd(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// internal

	auto Agent::delete_expr(const Expr& expr) const -> void {
		evo::debugAssert(this->hasTargetFunction(), "Not target function is set");

		switch(expr.getKind()){
			break; case Expr::Kind::None:         evo::debugFatalBreak("Invalid expr");
			break; case Expr::Kind::GlobalValue:  return;
			break; case Expr::Kind::Number:       this->module.numbers.erase(expr.index);
			break; case Expr::Kind::ParamExpr:    return;
			break; case Expr::Kind::CallInst:     this->target_func->calls.erase(expr.index);
			break; case Expr::Kind::CallVoidInst: this->target_func->call_voids.erase(expr.index);
			break; case Expr::Kind::RetInst:      this->target_func->rets.erase(expr.index);
			break; case Expr::Kind::BrInst:       return;
			break; case Expr::Kind::Add:          this->target_func->adds.erase(expr.index);
		}
	}


}