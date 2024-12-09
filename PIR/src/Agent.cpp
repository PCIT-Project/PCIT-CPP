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
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::setTargetFunction(Function& func) -> void {
		this->target_func = &func;
		this->target_basic_block = nullptr;
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::removeTargetFunction() -> void {
		this->target_func = nullptr;
		this->target_basic_block = nullptr;
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::getTargetFunction() const -> Function& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		return *this->target_func;
	}


	auto Agent::setTargetBasicBlock(BasicBlock::ID id) -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		// TODO: check that block is in function

		this->target_basic_block = &this->getBasicBlock(id);
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::setTargetBasicBlock(BasicBlock& basic_block) -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		// TODO: check that block is in function

		this->target_basic_block = &basic_block;
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::setTargetBasicBlockAtEnd() -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");

		this->setTargetBasicBlock(this->target_func->basic_blocks.back());
	}


	auto Agent::removeTargetBasicBlock() -> void {
		this->target_basic_block = nullptr;
		this->insert_index = std::numeric_limits<size_t>::max();
	}

	auto Agent::getTargetBasicBlock() const -> BasicBlock& {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		return *this->target_basic_block;
	}


	auto Agent::setInsertIndex(size_t index) -> void {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		// TODO: check that index is in block

		this->insert_index = index;
	}

	auto Agent::setInsertIndexAtEnd() -> void {
		this->insert_index = std::numeric_limits<size_t>::max();
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



	auto Agent::replaceExpr(Expr original, Expr replacement) const -> void {
		evo::debugAssert(this->hasTargetFunction(), "No target function is set");
		evo::debugAssert(
			!original.isMultiValueStmt(), "Cannot replace multi-value statement (extract values and manually remove)"
		);
		evo::debugAssert(
			!replacement.isMultiValueStmt(), "Replacement cannot be multi-value statement (extract values)"
		);
		evo::debugAssert(
			this->getExprType(original) == this->getExprType(replacement),
			"type of original and replacement do not match"
		);

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
					original_location.emplace(basic_block_id, i);
					continue;
				}

				switch(stmt.getKind()){
					case Expr::Kind::None: evo::debugFatalBreak("Invalid stmt");
					
					case Expr::Kind::GlobalValue: continue;
					case Expr::Kind::Number:      continue;
					case Expr::Kind::Boolean:     continue;
					case Expr::Kind::ParamExpr:   continue;
					
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

					case Expr::Kind::Breakpoint: continue;
	
					case Expr::Kind::Ret: {
						Ret& ret_inst = this->module.rets[stmt.index];

						if(ret_inst.value.has_value() && *ret_inst.value == original){
							ret_inst.value.emplace(replacement);
						}
					} break;
					
					case Expr::Kind::Branch: continue;

					case Expr::Kind::CondBranch: {
						CondBranch& cond_branch = this->module.cond_branches[stmt.index];

						if(cond_branch.cond == original){ cond_branch.cond = replacement; }
					} break;

					case Expr::Kind::Unreachable: continue;
					case Expr::Kind::Alloca:      continue;

					case Expr::Kind::Load: {
						Load& load = this->module.loads[stmt.index];

						if(load.source == original){ load.source = replacement; }
					} break;

					case Expr::Kind::Store: {
						Store& store = this->module.stores[stmt.index];

						if(store.destination == original){ store.destination = replacement; }
						if(store.value == original){ store.value = replacement; }
					} break;

					case Expr::Kind::CalcPtr: {
						CalcPtr& calc_ptr = this->module.calc_ptrs[stmt.index];

						if(calc_ptr.basePtr == original){
							calc_ptr.basePtr = replacement;
						}else{
							for(CalcPtr::Index& index : calc_ptr.indices){
								if(index.is<Expr>() && index.as<Expr>() == original){ index.as<Expr>() = replacement; }
							}
						}
					} break;

					case Expr::Kind::BitCast: {
						BitCast& bitcast = this->module.bitcasts[stmt.index];
						if(bitcast.fromValue == original){ bitcast.fromValue = replacement; }
					} break;

					case Expr::Kind::Trunc: {
						Trunc& trunc = this->module.truncs[stmt.index];
						if(trunc.fromValue == original){ trunc.fromValue = replacement; }
					} break;

					case Expr::Kind::FTrunc: {
						FTrunc& ftrunc = this->module.ftruncs[stmt.index];
						if(ftrunc.fromValue == original){ ftrunc.fromValue = replacement; }
					} break;

					case Expr::Kind::SExt: {
						SExt& sext = this->module.sexts[stmt.index];
						if(sext.fromValue == original){ sext.fromValue = replacement; }
					} break;

					case Expr::Kind::ZExt: {
						ZExt& zext = this->module.zexts[stmt.index];
						if(zext.fromValue == original){ zext.fromValue = replacement; }
					} break;

					case Expr::Kind::FExt: {
						FExt& fext = this->module.fexts[stmt.index];
						if(fext.fromValue == original){ fext.fromValue = replacement; }
					} break;

					case Expr::Kind::IToF: {
						IToF& itof = this->module.itofs[stmt.index];
						if(itof.fromValue == original){ itof.fromValue = replacement; }
					} break;

					case Expr::Kind::UIToF: {
						UIToF& uitof = this->module.uitofs[stmt.index];
						if(uitof.fromValue == original){ uitof.fromValue = replacement; }
					} break;

					case Expr::Kind::FToI: {
						FToI& ftoi = this->module.ftois[stmt.index];
						if(ftoi.fromValue == original){ ftoi.fromValue = replacement; }
					} break;

					case Expr::Kind::FToUI: {
						FToUI& ftoui = this->module.ftouis[stmt.index];
						if(ftoui.fromValue == original){ ftoui.fromValue = replacement; }
					} break;

					
					case Expr::Kind::Add: {
						Add& add = this->module.adds[stmt.index];

						if(add.lhs == original){ add.lhs = replacement; }
						if(add.rhs == original){ add.rhs = replacement; }
					} break;

					case Expr::Kind::FAdd: {
						FAdd& fadd = this->module.fadds[stmt.index];

						if(fadd.lhs == original){ fadd.lhs = replacement; }
						if(fadd.rhs == original){ fadd.rhs = replacement; }
					} break;

					case Expr::Kind::SAddWrap: {
						SAddWrap& sadd_wrap = this->module.sadd_wraps[stmt.index];

						if(sadd_wrap.lhs == original){ sadd_wrap.lhs = replacement; }
						if(sadd_wrap.rhs == original){ sadd_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::SAddWrapResult: continue;
					case Expr::Kind::SAddWrapWrapped: continue;

					case Expr::Kind::UAddWrap: {
						UAddWrap& uadd_wrap = this->module.uadd_wraps[stmt.index];

						if(uadd_wrap.lhs == original){ uadd_wrap.lhs = replacement; }
						if(uadd_wrap.rhs == original){ uadd_wrap.rhs = replacement; }
					} break;

					case Expr::Kind::UAddWrapResult: continue;
					case Expr::Kind::UAddWrapWrapped: continue;
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
		evo::debugAssert(
			type.getKind() == Type::Kind::Integer, "Type and value must both be integer or both be floating"
		);

		return Expr(Expr::Kind::Number, this->module.numbers.emplace_back(type, std::move(value)));
	}

	auto Agent::createNumber(const Type& type, const core::GenericInt& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(
			type.getKind() == Type::Kind::Integer, "Type and value must both be integer or both be floating"
		);

		return Expr(Expr::Kind::Number, this->module.numbers.emplace_back(type, value));
	}

	auto Agent::createNumber(const Type& type, core::GenericFloat&& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.isFloat(), "Type and value must both be integer or both be floating");

		return Expr(Expr::Kind::Number, this->module.numbers.emplace_back(type, std::move(value)));
	}

	auto Agent::createNumber(const Type& type, const core::GenericFloat& value) const -> Expr {
		evo::debugAssert(type.isNumeric(), "Number type must be numeric");
		evo::debugAssert(type.isFloat(), "Type and value must both be integer or both be floating");

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
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
		return new_expr;
	}


	auto Agent::getCallVoid(const Expr& expr) const -> const CallVoid& {
		return ReaderAgent(this->module, this->getTargetFunction()).getCallVoid(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// breakpoint

	auto Agent::createBreakpoint() const -> Expr {
		const auto new_expr = Expr(Expr::Kind::Breakpoint);
		this->insert_stmt(new_expr);
		return new_expr;
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
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::createRet() const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->target_func->getReturnType().getKind() == Type::Kind::Void, "Return type must match"
		);

		const auto new_expr = Expr(Expr::Kind::Ret, this->module.rets.emplace_back(std::nullopt));
		this->insert_stmt(new_expr);
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
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getBranch(const Expr& expr) -> Branch {
		return ReaderAgent::getBranch(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// conditional branch instructions

	auto Agent::createCondBranch(const Expr& cond, BasicBlock::ID then_block, BasicBlock::ID else_block) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(cond).getKind() == Type::Kind::Bool, "Cond must be of type Bool");

		const auto new_expr = Expr(
			Expr::Kind::CondBranch, this->module.cond_branches.emplace_back(cond, then_block, else_block)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getCondBranch(const Expr& expr) const -> CondBranch {
		return ReaderAgent(this->module, this->getTargetFunction()).getCondBranch(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// unreachable

	auto Agent::createUnreachable() const -> Expr {
		const auto new_expr = Expr(Expr::Kind::Unreachable);
		this->insert_stmt(new_expr);
		return new_expr;
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
	// load

	auto Agent::createLoad(
		const Expr& source, const Type& type, bool is_volatile, AtomicOrdering atomic_ordering, std::string&& name
	) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(atomic_ordering.isValidForLoad(), "This atomic ordering is not valid for a load");
		evo::debugAssert(this->getExprType(source).getKind() == Type::Kind::Ptr, "Source must be of type Ptr");

		const auto new_stmt = Expr(
			Expr::Kind::Load,
			this->module.loads.emplace_back(
				this->get_stmt_name(std::move(name)), source, type, is_volatile, atomic_ordering
			)
		);
		this->insert_stmt(new_stmt);
		return new_stmt;
	}

	auto Agent::getLoad(const Expr& expr) const -> const Load& {
		return ReaderAgent(this->module, this->getTargetFunction()).getLoad(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// store

	auto Agent::createStore(
		const Expr& destination, const Expr& value, bool is_volatile, AtomicOrdering atomic_ordering
	) const -> void {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(atomic_ordering.isValidForStore(), "This atomic ordering is not valid for a store");
		evo::debugAssert(
			this->getExprType(destination).getKind() == Type::Kind::Ptr, "Destination must be of type Ptr"
		);

		const auto new_stmt = Expr(
			Expr::Kind::Store, this->module.stores.emplace_back(destination, value, is_volatile, atomic_ordering)
		);
		this->insert_stmt(new_stmt);
	}

	auto Agent::getStore(const Expr& expr) const -> const Store& {
		return ReaderAgent(this->module, this->getTargetFunction()).getStore(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// calc ptr

	EVO_NODISCARD auto Agent::createCalcPtr(
		const Expr& base_ptr, const Type& ptr_type, evo::SmallVector<CalcPtr::Index>&& indices, std::string&& name
	) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(base_ptr).getKind() == Type::Kind::Ptr, "Base ptr must be of type Ptr");
		evo::debugAssert(!indices.empty(), "There must be at least one index");
		#if defined(PCIT_CONFIG_DEBUG)
			Type target_type = ptr_type;
			
			for(size_t i = 1; i < indices.size(); i+=1){
				const CalcPtr::Index& index = indices[i];

				evo::debugAssert(target_type.isAggregate(), "ptr type must be an aggregate type");
				evo::debugAssert(
					index.is<int64_t>() || this->getExprType(index.as<Expr>()).getKind() == Type::Kind::Integer,
					"Index must be integer"
				);

				if(target_type.getKind() == Type::Kind::Array){
					const ArrayType& array_type = this->module.getArrayType(target_type);

					if(index.is<int64_t>()){
						evo::debugAssert(
							index.as<int64_t>() >= 0 && size_t(index.as<int64_t>()) < array_type.length,
							"indexing into an array must be a valid index"
						);
						
					}else if(index.as<Expr>().getKind() == Expr::Kind::Number){
						const int64_t member_index = static_cast<int64_t>(this->getNumber(index.as<Expr>()).getInt());
						evo::debugAssert(
							member_index >= 0 && size_t(member_index) < array_type.length,
							"indexing into an array must be a valid index"
						);
					}

					target_type = array_type.elemType;

				}else{
					evo::debugAssert(index.is<int64_t>(), "Cannot index into a struct with a pcit::pir::Expr");

					const StructType& struct_type = this->module.getStructType(target_type);

					evo::debugAssert(
						index.as<int64_t>() >= 0 && size_t(index.as<int64_t>()) < struct_type.members.size(),
						"indexing into a struct must be a valid member index"
					);
					target_type = struct_type.members[size_t(index.as<int64_t>())];
				}
			}
		#endif

		const auto new_stmt = Expr(
			Expr::Kind::CalcPtr,
			this->module.calc_ptrs.emplace_back(
				this->get_stmt_name(std::move(name)), ptr_type, base_ptr, std::move(indices)
			)
		);
		this->insert_stmt(new_stmt);
		return new_stmt;
	}

	EVO_NODISCARD auto Agent::getCalcPtr(const Expr& expr) const -> const CalcPtr& {
		return ReaderAgent(this->module, this->getTargetFunction()).getCalcPtr(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// BitCast
	
	EVO_NODISCARD auto Agent::createBitCast(const Expr& fromValue, const Type& toType, std::string&& name) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) == this->module.getSize(toType),
			"Cannot convert to a type of a different size"
		);

		const auto new_expr = Expr(
			Expr::Kind::BitCast,
			this->module.bitcasts.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getBitCast(const Expr& expr) const -> const BitCast& {
		return ReaderAgent(this->module, this->getTargetFunction()).getBitCast(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// Trunc
	
	EVO_NODISCARD auto Agent::createTrunc(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) >= this->module.getSize(toType),
			"Cannot convert to a type of a greater size"
		);
		evo::debugAssert(this->getExprType(fromValue).getKind() == Type::Kind::Integer, "can only convert integers");
		evo::debugAssert(toType.getKind() == Type::Kind::Integer, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::Trunc,
			this->module.truncs.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getTrunc(const Expr& expr) const -> const Trunc& {
		return ReaderAgent(this->module, this->getTargetFunction()).getTrunc(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// FTrunc
	
	EVO_NODISCARD auto Agent::createFTrunc(const Expr& fromValue, const Type& toType, std::string&& name) const
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) >= this->module.getSize(toType),
			"Cannot convert to a type of a greater size"
		);
		evo::debugAssert(this->getExprType(fromValue).isFloat(), "can only convert floats");
		evo::debugAssert(toType.isFloat(), "can only convert to floats");

		const auto new_expr = Expr(
			Expr::Kind::FTrunc,
			this->module.ftruncs.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getFTrunc(const Expr& expr) const -> const FTrunc& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFTrunc(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// SExt
	
	EVO_NODISCARD auto Agent::createSExt(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) <= this->module.getSize(toType),
			"Cannot convert to a type of a smaller size"
		);
		evo::debugAssert(this->getExprType(fromValue).getKind() == Type::Kind::Integer, "can only convert integers");
		evo::debugAssert(toType.getKind() == Type::Kind::Integer, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::SExt,
			this->module.sexts.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getSExt(const Expr& expr) const -> const SExt& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSExt(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// ZExt
	
	EVO_NODISCARD auto Agent::createZExt(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) <= this->module.getSize(toType),
			"Cannot convert to a type of a smaller size"
		);
		evo::debugAssert(this->getExprType(fromValue).getKind() == Type::Kind::Integer, "can only convert integers");
		evo::debugAssert(toType.getKind() == Type::Kind::Integer, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::ZExt,
			this->module.zexts.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getZExt(const Expr& expr) const -> const ZExt& {
		return ReaderAgent(this->module, this->getTargetFunction()).getZExt(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// FExt
	
	EVO_NODISCARD auto Agent::createFExt(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(
			this->module.getSize(this->getExprType(fromValue)) <= this->module.getSize(toType),
			"Cannot convert to a type of a smaller size"
		);
		evo::debugAssert(this->getExprType(fromValue).isFloat(), "can only convert floats");
		evo::debugAssert(toType.isFloat(), "can only convert to floats");

		const auto new_expr = Expr(
			Expr::Kind::FExt,
			this->module.fexts.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getFExt(const Expr& expr) const -> const FExt& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFExt(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// IToF
	
	EVO_NODISCARD auto Agent::createIToF(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(fromValue).getKind() == Type::Kind::Integer, "can only convert integers");
		evo::debugAssert(toType.isFloat(), "can only convert to floats");

		const auto new_expr = Expr(
			Expr::Kind::IToF,
			this->module.itofs.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getIToF(const Expr& expr) const -> const IToF& {
		return ReaderAgent(this->module, this->getTargetFunction()).getIToF(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// UIToF
	
	EVO_NODISCARD auto Agent::createUIToF(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(fromValue).getKind() == Type::Kind::Integer, "can only convert integers");
		evo::debugAssert(toType.isFloat(), "can only convert to floats");

		const auto new_expr = Expr(
			Expr::Kind::UIToF,
			this->module.uitofs.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getUIToF(const Expr& expr) const -> const UIToF& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUIToF(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// FToI
	
	EVO_NODISCARD auto Agent::createFToI(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(fromValue).isFloat(), "can only convert floats");
		evo::debugAssert(toType.getKind() == Type::Kind::Integer, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::FToI,
			this->module.ftois.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getFToI(const Expr& expr) const -> const FToI& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFToI(expr);
	}

	
	//////////////////////////////////////////////////////////////////////
	// FToUI
	
	EVO_NODISCARD auto Agent::createFToUI(const Expr& fromValue, const Type& toType, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(this->getExprType(fromValue).isFloat(), "can only convert floats");
		evo::debugAssert(toType.getKind() == Type::Kind::Integer, "can only convert to integers");

		const auto new_expr = Expr(
			Expr::Kind::FToUI,
			this->module.ftouis.emplace_back(this->get_stmt_name(std::move(name)), fromValue, toType)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	EVO_NODISCARD auto Agent::getFToUI(const Expr& expr) const -> const FToUI& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFToUI(expr);
	}




	//////////////////////////////////////////////////////////////////////
	// add

	auto Agent::createAdd(const Expr& lhs, const Expr& rhs, bool may_wrap, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(
			this->getExprType(lhs).getKind() == Type::Kind::Integer, "The @add instruction only supports integers"
		);

		const auto new_expr = Expr(
			Expr::Kind::Add,
			this->module.adds.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs, may_wrap)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getAdd(const Expr& expr) const -> const Add& {
		return ReaderAgent(this->module, this->getTargetFunction()).getAdd(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// fadd

	auto Agent::createFAdd(const Expr& lhs, const Expr& rhs, std::string&& name) const -> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).isFloat(), "The @fAdd instruction only supports float values");

		const auto new_expr = Expr(
			Expr::Kind::FAdd, this->module.adds.emplace_back(this->get_stmt_name(std::move(name)), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getFAdd(const Expr& expr) const -> const FAdd& {
		return ReaderAgent(this->module, this->getTargetFunction()).getFAdd(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// signed add wrap

	auto Agent::createSAddWrap(const Expr& lhs, const Expr& rhs, std::string&& result_name, std::string&& wrapped_name)
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).getKind() == Type::Kind::Integer, "Can only add wrap integers");

		const std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		const std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::SAddWrap,
			this->module.sadd_wraps.emplace_back(std::move(result_stmt_name), std::move(wrapped_stmt_name), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getSAddWrap(const Expr& expr) const -> const SAddWrap& {
		return ReaderAgent(this->module, this->getTargetFunction()).getSAddWrap(expr);
	}


	auto Agent::extractSAddWrapResult(const Expr& expr) -> Expr {
		return ReaderAgent::extractSAddWrapResult(expr);
	}

	auto Agent::extractSAddWrapWrapped(const Expr& expr) -> Expr {
		return ReaderAgent::extractSAddWrapWrapped(expr);
	}



	//////////////////////////////////////////////////////////////////////
	// unsigned add wrap

	auto Agent::createUAddWrap(const Expr& lhs, const Expr& rhs, std::string&& result_name, std::string&& wrapped_name)
	-> Expr {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");
		evo::debugAssert(lhs.isValue() && rhs.isValue(), "Arguments must be values");
		evo::debugAssert(this->getExprType(lhs) == this->getExprType(rhs), "Arguments must be same type");
		evo::debugAssert(this->getExprType(lhs).getKind() == Type::Kind::Integer, "Can only add wrap integers");

		const std::string result_stmt_name = this->get_stmt_name(std::move(result_name));
		const std::string wrapped_stmt_name = this->get_stmt_name_with_forward_include(
			std::move(wrapped_name), {result_stmt_name}
		);

		const auto new_expr = Expr(
			Expr::Kind::UAddWrap,
			this->module.uadd_wraps.emplace_back(std::move(result_stmt_name), std::move(wrapped_stmt_name), lhs, rhs)
		);
		this->insert_stmt(new_expr);
		return new_expr;
	}

	auto Agent::getUAddWrap(const Expr& expr) const -> const UAddWrap& {
		return ReaderAgent(this->module, this->getTargetFunction()).getUAddWrap(expr);
	}


	auto Agent::extractUAddWrapResult(const Expr& expr) -> Expr {
		return ReaderAgent::extractUAddWrapResult(expr);
	}

	auto Agent::extractUAddWrapWrapped(const Expr& expr) -> Expr {
		return ReaderAgent::extractUAddWrapWrapped(expr);
	}


	//////////////////////////////////////////////////////////////////////
	// internal


	auto Agent::insert_stmt(const Expr& stmt) const -> void {
		evo::debugAssert(this->hasTargetBasicBlock(), "No target basic block set");

		if(this->getInsertIndexAtEnd()){
			this->target_basic_block->append(stmt);
		}else{
			this->target_basic_block->insert(stmt, this->insert_index);
			this->insert_index += 1;
		}
	}


	auto Agent::delete_expr(const Expr& expr) const -> void {
		evo::debugAssert(this->hasTargetFunction(), "Not target function is set");

		switch(expr.getKind()){
			break; case Expr::Kind::None:            evo::debugFatalBreak("Invalid expr");
			break; case Expr::Kind::GlobalValue:     return;
			break; case Expr::Kind::Number:          this->module.numbers.erase(expr.index);
			break; case Expr::Kind::Boolean:         return;
			break; case Expr::Kind::ParamExpr:       return;
			break; case Expr::Kind::Call:            this->module.calls.erase(expr.index);
			break; case Expr::Kind::CallVoid:        this->module.call_voids.erase(expr.index);
			break; case Expr::Kind::Breakpoint:      return;
			break; case Expr::Kind::Ret:             this->module.rets.erase(expr.index);
			break; case Expr::Kind::Branch:          return;
			break; case Expr::Kind::CondBranch:      this->module.cond_branches.erase(expr.index);
			break; case Expr::Kind::Unreachable:     return;
			break; case Expr::Kind::Alloca:          this->target_func->allocas.erase(expr.index);
			break; case Expr::Kind::Load:            this->module.loads.erase(expr.index);
			break; case Expr::Kind::Store:           this->module.stores.erase(expr.index);
			break; case Expr::Kind::CalcPtr:         this->module.calc_ptrs.erase(expr.index);
			break; case Expr::Kind::BitCast:         this->module.bitcasts.erase(expr.index);
			break; case Expr::Kind::Trunc:           this->module.truncs.erase(expr.index);
			break; case Expr::Kind::FTrunc:          this->module.ftruncs.erase(expr.index);
			break; case Expr::Kind::SExt:            this->module.sexts.erase(expr.index);
			break; case Expr::Kind::ZExt:            this->module.zexts.erase(expr.index);
			break; case Expr::Kind::FExt:            this->module.fexts.erase(expr.index);
			break; case Expr::Kind::IToF:            this->module.itofs.erase(expr.index);
			break; case Expr::Kind::UIToF:           this->module.uitofs.erase(expr.index);
			break; case Expr::Kind::FToI:            this->module.ftois.erase(expr.index);
			break; case Expr::Kind::FToUI:           this->module.ftouis.erase(expr.index);
			break; case Expr::Kind::Add:             this->module.adds.erase(expr.index);
			break; case Expr::Kind::FAdd:            this->module.fadds.erase(expr.index);
			break; case Expr::Kind::SAddWrap:        this->module.sadd_wraps.erase(expr.index);
			break; case Expr::Kind::SAddWrapResult:  return;
			break; case Expr::Kind::SAddWrapWrapped: return;
			break; case Expr::Kind::UAddWrap:        this->module.uadd_wraps.erase(expr.index);
			break; case Expr::Kind::UAddWrapResult:  return;
			break; case Expr::Kind::UAddWrapWrapped: return;
		}

		if(this->getInsertIndexAtEnd() == false){
			this->insert_index -= 1;
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
					case Expr::Kind::None:        evo::debugFatalBreak("Invalid expr");
					case Expr::Kind::GlobalValue: continue;
					case Expr::Kind::Number:      continue;
					case Expr::Kind::Boolean:     continue;
					case Expr::Kind::ParamExpr:   continue;
					case Expr::Kind::Call:        if(this->getCall(stmt).name == name){ return true; } continue;
					case Expr::Kind::CallVoid:    continue;
					case Expr::Kind::Breakpoint:  continue;
					case Expr::Kind::Ret:         continue;
					case Expr::Kind::Branch:      continue;
					case Expr::Kind::CondBranch:  continue;
					case Expr::Kind::Unreachable: continue;
					case Expr::Kind::Alloca:      if(this->getAlloca(stmt).name == name){ return true; } continue;
					case Expr::Kind::Load:        if(this->getLoad(stmt).name == name){ return true; } continue;
					case Expr::Kind::Store:       continue;
					case Expr::Kind::CalcPtr:     if(this->getCalcPtr(stmt).name == name){ return true; } continue;
					case Expr::Kind::BitCast:     if(this->getBitCast(stmt).name == name){ return true; } continue;
					case Expr::Kind::Trunc:       if(this->getTrunc(stmt).name == name){ return true; } continue;
					case Expr::Kind::FTrunc:      if(this->getFTrunc(stmt).name == name){ return true; } continue;
					case Expr::Kind::SExt:        if(this->getSExt(stmt).name == name){ return true; } continue;
					case Expr::Kind::ZExt:        if(this->getZExt(stmt).name == name){ return true; } continue;
					case Expr::Kind::FExt:        if(this->getFExt(stmt).name == name){ return true; } continue;
					case Expr::Kind::IToF:        if(this->getIToF(stmt).name == name){ return true; } continue;
					case Expr::Kind::UIToF:       if(this->getUIToF(stmt).name == name){ return true; } continue;
					case Expr::Kind::FToI:        if(this->getFToI(stmt).name == name){ return true; } continue;
					case Expr::Kind::FToUI:       if(this->getFToUI(stmt).name == name){ return true; } continue;
					case Expr::Kind::Add:         if(this->getAdd(stmt).name == name){ return true; } continue;
					case Expr::Kind::FAdd:        if(this->getFAdd(stmt).name == name){ return true; } continue;
					case Expr::Kind::SAddWrap: {
						const SAddWrap& sadd_wrap = this->getSAddWrap(stmt);
						if(sadd_wrap.resultName == name){ return true; }
						if(sadd_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::SAddWrapResult:  continue;
					case Expr::Kind::SAddWrapWrapped: continue;
					case Expr::Kind::UAddWrap: {
						const UAddWrap& uadd_wrap = this->getUAddWrap(stmt);
						if(uadd_wrap.resultName == name){ return true; }
						if(uadd_wrap.wrappedName == name){ return true; }
						continue;
					} break;
					case Expr::Kind::UAddWrapResult:  continue;
					case Expr::Kind::UAddWrapWrapped: continue;
				}
			}
		}

		return false;
	}


	auto Agent::get_stmt_name(std::string&& name) const -> std::string {
		return this->get_stmt_name_with_forward_include(std::move(name), {});
	}


	auto Agent::get_stmt_name_with_forward_include(
		std::string&& name, evo::ArrayProxy<std::string_view> forward_includes
	) const -> std::string {
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

		auto name_exists_in_forward_includes = [&](const std::string& name_check) -> bool {
			for(std::string_view forward_include : forward_includes){
				if(name_check == forward_include){ return true; }
			}

			return false;
		};

		while(this->name_exists_in_func(converted_name) || name_exists_in_forward_includes(converted_name)){
			converted_name = std::format("{}.{}", name, suffix_num);
			suffix_num += 1;
		}

		return converted_name;
	}


}