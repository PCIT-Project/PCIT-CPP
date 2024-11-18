////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Agent.h"

#include "../include/misc.h"
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




	auto Agent::replaceStmt(Expr original, const Expr& replacement) const -> void {
		this->replace_stmt_impl<false>(original, replacement);
	}

	auto Agent::replaceStmtWithValue(Expr original, const Expr& replacement) const -> void {
		this->replace_stmt_impl<true>(original, replacement);
	}


	template<bool REPLACE_WITH_VALUE>
	auto Agent::replace_stmt_impl(Expr original, const Expr& replacement) const -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		if constexpr(REPLACE_WITH_VALUE){
			evo::debugAssert(replacement.isValue(), "replacement is not a value");
		}else{
			evo::debugAssert(replacement.isStmt(), "replacement is not a statement");
		}

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
					if constexpr(REPLACE_WITH_VALUE){
						original_location.emplace(basic_block_id, i);
					}else{
						stmt = replacement;
					}
					continue;
				}

				switch(stmt.getKind()){
					case Expr::Kind::None: evo::debugFatalBreak("Invalid stmt");
					
					case Expr::Kind::GlobalValue: continue;
					
					case Expr::Kind::Number: continue;

					case Expr::Kind::ParamExpr: continue;
					
					case Expr::Kind::Call: {
						Call& call_inst = this->module.calls[stmt.index];

						if(call_inst.target.is<PtrCall>() && call_inst.target.as<PtrCall>().location == original){
							call_inst.target.as<PtrCall>().location = replacement;
						}

						for(Expr& arg : call_inst.args){
							if(arg == original){ arg = replacement; }
						}
					} break;
					
					case Expr::Kind::CallVoid: {
						CallVoid& call_void_inst = this->module.call_voids[stmt.index];

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
					
					case Expr::Kind::Ret: {
						Ret& ret_inst = this->module.rets[stmt.index];

						if(ret_inst.value.has_value() && *ret_inst.value == original){
							ret_inst.value.emplace(replacement);
						}
					} break;
					
					case Expr::Kind::Branch: continue;
					
					case Expr::Kind::Add: {
						Add& add = this->module.adds[stmt.index];

						if(add.lhs == original){ add.lhs = replacement; }
						if(add.rhs == original){ add.rhs = replacement; }
					} break;

					case Expr::Kind::AddWrap: {
						AddWrap& add_wrap = this->module.add_wraps[stmt.index];

						if(add_wrap.lhs == original){ add_wrap.lhs = replacement; }
						if(add_wrap.rhs == original){ add_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::AddWrapResult: continue;
					case Expr::Kind::AddWrapWrapped: continue;
				}
			}
		}

		if(original_location.has_value()){
			BasicBlock& basic_block = this->getBasicBlock(original_location->basic_block_id);
			basic_block.remove(original_location->index);
		}

		this->delete_expr(original);
	}


	auto Agent::removeStmt(Expr stmt_to_remove) const -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		evo::debugAssert(stmt_to_remove.isStmt(), "not a statement");

		if(stmt_to_remove.getKind() != Expr::Kind::Alloca){
			for(const BasicBlock::ID& basic_block_id : *this->target_func){
				BasicBlock& basic_block = this->getBasicBlock(basic_block_id);
	
				for(size_t i = 0; const Expr& stmt : basic_block){
					if(stmt == stmt_to_remove){
						basic_block.remove(i);
						this->delete_expr(stmt_to_remove);
						return;
					}

					i += 1;
				}

			}
		}

		this->delete_expr(stmt_to_remove);
	}

	
	//////////////////////////////////////////////////////////////////////
	// basic blocks

	auto Agent::createBasicBlock(Function::ID func, std::string&& name) const -> BasicBlock::ID {
		return this->createBasicBlock(this->module.getFunction(func), std::move(name));
	}

	auto Agent::createBasicBlock(Function& func, std::string&& name) const -> BasicBlock::ID {
		const pcit::pir::BasicBlock::ID new_block_id =
			this->module.basic_blocks.emplace_back(this->get_stmt_name(std::move(name)));
		func.append_basic_block(new_block_id);
		return new_block_id;
	}

	auto Agent::createBasicBlock(std::string&& name) const -> BasicBlock::ID {
		evo::debugAssert(this->hasTargetFunction(), "Cannot use this function as there is no function target set");
		return this->createBasicBlock(*this->target_func, std::move(name));
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
	// booleans

	auto Agent::createBoolean(bool value) -> Expr {
		return Expr(Expr::Kind::Boolean, uint32_t(value));
	}

	auto Agent::getBoolean(const Expr& expr) -> bool {
		return ReaderAgent::getBoolean(expr);
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
	// global values (expr)

	auto Agent::createGlobalValue(const GlobalVar::ID& global_id) -> Expr {
		return Expr(Expr::Kind::GlobalValue, global_id.get());
	}

	auto Agent::getGlobalValue(const Expr& expr) const -> const GlobalVar& {
		return ReaderAgent(this->module).getGlobalValue(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// calls

	auto Agent::createCall(Function::ID func, evo::SmallVector<Expr>&& args, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), func, std::move(args))
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCall(Function::ID func, const evo::SmallVector<Expr>& args, std::string&& name) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), func, args)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	auto Agent::createCall(FunctionDecl::ID func, evo::SmallVector<Expr>&& args, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), func, std::move(args))
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCall(FunctionDecl::ID func, const evo::SmallVector<Expr>& args, std::string&& name) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), func, args)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	auto Agent::createCall(
		const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args, std::string&& name
	) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionType(func_type).returnType.getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
			this->module.calls.emplace_back(
				this->get_stmt_name(std::move(name)), PtrCall(func, func_type), std::move(args)
			)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCall(
		const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args, std::string&& name
	) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionType(func_type).returnType.getKind() != Type::Kind::Void,
			"Call cannot return `Void` (did you mean CallVoid?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::Call,
			this->module.calls.emplace_back(this->get_stmt_name(std::move(name)), PtrCall(func, func_type), args)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	auto Agent::getCall(const Expr& expr) const -> const Call& {
		return ReaderAgent(this->module, this->getTargetFunction()).getCall(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// call voids

	auto Agent::createCallVoid(Function::ID func, evo::SmallVector<Expr>&& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoid, this->module.call_voids.emplace_back(func, std::move(args))
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoid(Function::ID func, const evo::SmallVector<Expr>& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunction(func).getReturnType().getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(Expr::Kind::CallVoid, this->module.call_voids.emplace_back(func, args));
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	auto Agent::createCallVoid(FunctionDecl::ID func, evo::SmallVector<Expr>&& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoid, this->module.call_voids.emplace_back(func, std::move(args))
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoid(FunctionDecl::ID func, const evo::SmallVector<Expr>& args) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionDecl(func).returnType.getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func, args), "Func call args don't match");

		const auto new_expr = Expr(Expr::Kind::CallVoid, this->module.call_voids.emplace_back(func, args));
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	auto Agent::createCallVoid(const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionType(func_type).returnType.getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoid,
			this->module.call_voids.emplace_back(PtrCall(func, func_type), std::move(args))
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createCallVoid(const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getFunctionType(func_type).returnType.getKind() == Type::Kind::Void,
			"CallVoid must return `Void` (did you mean Call?)"
		);
		evo::debugAssert(this->target_func->check_func_call_args(func_type, args), "Func call args don't match");

		const auto new_expr = Expr(
			Expr::Kind::CallVoid, this->module.call_voids.emplace_back(PtrCall(func, func_type), args)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}


	auto Agent::getCallVoid(const Expr& expr) const -> const CallVoid& {
		return ReaderAgent(this->module, this->getTargetFunction()).getCallVoid(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// ret instructions

	auto Agent::createRet(const Expr& expr) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(expr.isValue(), "Must return value");
		evo::debugAssert(
			this->getExprType(expr) == this->target_func->getReturnType(), "Return type must match function"
		);

		const auto new_expr = Expr(Expr::Kind::Ret, this->module.rets.emplace_back(expr));
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::createRet() const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->target_func->getReturnType().getKind() == Type::Kind::Void, "Return type must match"
		);

		const auto new_expr = Expr(Expr::Kind::Ret, this->module.rets.emplace_back(std::nullopt));
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::getRet(const Expr& expr) const -> const Ret& {
		return ReaderAgent(this->module, this->getTargetFunction()).getRet(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// branch instructions

	auto Agent::createBranch(BasicBlock::ID basic_block_id) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");

		const auto new_expr = Expr(Expr::Kind::Branch, basic_block_id.get());
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::getBranch(const Expr& expr) -> Branch {
		return ReaderAgent::getBranch(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// alloca

	auto Agent::createAlloca(const Type& type, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetFunction(), "No target functions set");

		return Expr(
			Expr::Kind::Alloca, this->target_func->allocas.emplace_back(this->get_stmt_name(std::move(name)), type)
		);
	}

	auto Agent::getAlloca(const Expr& expr) const -> const Alloca& {
		return ReaderAgent(this->module, this->getTargetFunction()).getAlloca(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// add

	auto Agent::createAdd(const Expr& lhs, const Expr& rhs, bool may_wrap, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isNumeric(), "Can only add numerics");

		const auto new_expr = Expr(
			Expr::Kind::Add,
			this->module.adds.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, may_wrap)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::getAdd(const Expr& expr) const -> const Add& {
		return ReaderAgent(this->module, this->getTargetFunction()).getAdd(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// add wrap

	auto Agent::createAddWrap(const Expr& lhs, const Expr& rhs, std::string&& result_name, std::string&& wrapped_name)
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isIntegral(), "Can only add wrap integrals");

		const auto new_expr = Expr(
			Expr::Kind::AddWrap,
			this->module.add_wraps.emplace_back(
				this->get_stmt_name(std::move(result_name)), this->get_stmt_name(std::move(wrapped_name)), lhs, rhs
			)
		);
		this->target_basic_block->append(new_expr);
		return new_expr;
	}

	auto Agent::getAddWrap(const Expr& expr) const -> const AddWrap& {
		return ReaderAgent(this->module, this->getTargetFunction()).getAddWrap(expr);
	}


	auto Agent::extractAddWrapResult(const Expr& expr) -> Expr {
		return ReaderAgent::extractAddWrapResult(expr);
	}

	auto Agent::extractAddWrapWrapped(const Expr& expr) -> Expr {
		return ReaderAgent::extractAddWrapWrapped(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// internal

	auto Agent::delete_expr(const Expr& expr) const -> void {
		evo::debugAssert(this->hasTargetFunction(), "Not target function is set");

		switch(expr.getKind()){
			break; case Expr::Kind::None:           evo::debugFatalBreak("Invalid expr");
			break; case Expr::Kind::GlobalValue:    return;
			break; case Expr::Kind::Number:         this->module.numbers.erase(expr.index);
			break; case Expr::Kind::Boolean:        return;
			break; case Expr::Kind::ParamExpr:      return;
			break; case Expr::Kind::Call:           this->module.calls.erase(expr.index);
			break; case Expr::Kind::CallVoid:       this->module.call_voids.erase(expr.index);
			break; case Expr::Kind::Ret:            this->module.rets.erase(expr.index);
			break; case Expr::Kind::Branch:         return;
			break; case Expr::Kind::Alloca:         this->target_func->allocas.erase(expr.index);
			break; case Expr::Kind::Add:            this->module.adds.erase(expr.index);
			break; case Expr::Kind::AddWrap:        this->module.add_wraps.erase(expr.index);
			break; case Expr::Kind::AddWrapResult:  return;
			break; case Expr::Kind::AddWrapWrapped: return;
		}
	}



	auto Agent::name_exists_in_func(std::string_view name) const -> bool {
		evo::debugAssert(this->hasTargetFunction(), "Not target function is set");

		for(const Parameter& param : this->target_func->getParameters()){
			if(param.getName() == name){ return true; }
		}

		for(BasicBlock::ID basic_block_id : *this->target_func){
			const BasicBlock& basic_block = this->getBasicBlock(basic_block_id);
			if(basic_block.getName() == name){ return true; }

			for(const Expr& stmt : basic_block){
				switch(stmt.getKind()){
					break; case Expr::Kind::None:        evo::debugFatalBreak("Invalid expr");
					break; case Expr::Kind::GlobalValue: continue;
					break; case Expr::Kind::Number:      continue;
					break; case Expr::Kind::Boolean:     continue;
					break; case Expr::Kind::ParamExpr:   continue;
					break; case Expr::Kind::Call:
						if(this->getCall(stmt).name == name){ return true; }
					break; case Expr::Kind::CallVoid: continue;
					break; case Expr::Kind::Ret:      continue;
					break; case Expr::Kind::Branch:       continue;
					break; case Expr::Kind::Alloca:
						if(this->getAlloca(stmt).name == name){ return true; }
					break; case Expr::Kind::Add:
						if(this->getAdd(stmt).name == name){ return true; }
					break; case Expr::Kind::AddWrap: {
						const AddWrap& add_wrap = this->getAddWrap(stmt);
						if(add_wrap.resultName == name){ return true; }
						if(add_wrap.wrappedName == name){ return true; }
					} break;
					break; case Expr::Kind::AddWrapResult:  continue;
					break; case Expr::Kind::AddWrapWrapped: continue;
				}
			}
		}

		return false;
	}


	auto Agent::get_stmt_name(std::string&& name) const -> std::string {
		evo::debugAssert(this->hasTargetFunction(), "Not target function is set");
		evo::debugAssert(name.empty() || isStandardName(name), "Not a valid stmt name ({})", name);

		unsigned suffix_num = 1;
		std::string converted_name = [&](){
			if(name.empty()){
				suffix_num += 1;
				return std::format("{}.1", name);
			}else{
				return name;
			}
		}();


		while(this->name_exists_in_func(converted_name)){
			converted_name = std::format("{}.{}", name, suffix_num);
			suffix_num += 1;
		}

		return converted_name;
	}


}