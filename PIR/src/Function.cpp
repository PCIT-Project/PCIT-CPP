//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/Function.h"

#include "../include/Module.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::pir{
	
	
	auto Function::appendBasicBlock(BasicBlock::ID id) -> void {
		evo::debugAssert(!this->basic_block_is_already_in(id), "Basic block is already in this funciton");
		this->basic_blocks.emplace_back(id);
	}


	auto Function::insertBasicBlockBefore(BasicBlock::ID id, BasicBlock::ID before) -> void {
		evo::debugAssert(!this->basic_block_is_already_in(id), "Basic block is already in this funciton");

		for(auto iter = this->basic_blocks.begin(); iter != this->basic_blocks.end(); ++iter){
			if(*iter == before){
				this->basic_blocks.insert(iter, id);
				return;
			}
		}

		evo::debugFatalBreak("Before basic block is not in this function");
	}

	auto Function::insertBasicBlockAfter(BasicBlock::ID id, BasicBlock::ID after) -> void {
		evo::debugAssert(!this->basic_block_is_already_in(id), "Basic block is already in this funciton");
		
		for(auto iter = this->basic_blocks.begin(); iter != this->basic_blocks.end(); ++iter){
			if(*iter == after){
				this->basic_blocks.insert(++iter, id);
				return;
			}
		}

		evo::debugFatalBreak("After basic block is not in this function");
	}


	auto Function::getExprType(const Expr& expr) const -> Type {
		evo::debugAssert(expr.isValue(), "Expr must be a value in order to get the type");

		switch(expr.getKind()){
			case Expr::Kind::None:         evo::unreachable();
			case Expr::Kind::Number:       return this->parent_module.getNumber(expr).type;
			case Expr::Kind::GlobalValue:  return this->parent_module.createTypePtr();
			case Expr::Kind::ParamExpr: {
				const ParamExpr param_expr = this->getParamExpr(expr);
				return this->getParameters()[param_expr.index].getType();
			} break;
			case Expr::Kind::CallInst: {
				const CallInst& call_inst = this->getCallInst(expr);

				return call_inst.target.visit([&](const auto& target) -> Type {
					using ValueT = std::decay_t<decltype(target)>;

					if constexpr(std::is_same_v<ValueT, Function::ID>){
						return this->parent_module.getFunction(target).getReturnType();

					}else if constexpr(std::is_same_v<ValueT, FunctionDecl::ID>){
						return this->parent_module.getFunctionDecl(target).returnType;
						
					}else if constexpr(std::is_same_v<ValueT, PtrCall>){
						return this->parent_module.getTypeFunction(target.funcType).returnType;

					}else{
						static_assert(false, "Unsupported call inst target");
					}
				});
			} break;
			case Expr::Kind::CallVoidInst: evo::unreachable();
			case Expr::Kind::RetInst:      evo::unreachable();
			case Expr::Kind::BrInst:       evo::unreachable();
			case Expr::Kind::Add:          return this->getExprType(this->getAdd(expr).lhs);
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto Function::replaceExpr(const Expr& original, const Expr& replacement) -> void {
		struct OriginalLocation{
			BasicBlock::ID basic_block_id;
			size_t index;
		};
		auto original_location = std::optional<OriginalLocation>();

		for(const BasicBlock::ID& basic_block_id : *this){
			BasicBlock& basic_block = this->parent_module.getBasicBlock(basic_block_id);
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
						CallInst& call_inst = this->calls[stmt.index];

						if(call_inst.target.is<PtrCall>() && call_inst.target.as<PtrCall>().location == original){
							call_inst.target.as<PtrCall>().location = replacement;
						}

						for(Expr& arg : call_inst.args){
							if(arg == original){ arg = replacement; }
						}
					} break;
					
					case Expr::Kind::CallVoidInst: {
						CallVoidInst& call_void_inst = this->call_voids[stmt.index];

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
						RetInst& ret_inst = this->rets[stmt.index];

						if(ret_inst.value.has_value() && *ret_inst.value == original){
							ret_inst.value.emplace(replacement);
						}
					} break;
					
					case Expr::Kind::BrInst: continue;
					
					case Expr::Kind::Add: {
						Add& add = this->adds[stmt.index];

						if(add.lhs == original){ add.lhs = replacement; }
						if(add.rhs == original){ add.rhs = replacement; }
					} break;
				}
			}
		}

		if(original_location.has_value()){
			BasicBlock& basic_block = this->parent_module.getBasicBlock(original_location->basic_block_id);
			basic_block.remove(original_location->index);
		}
	}



	auto Function::createCallInst(std::string&& name, Function::ID func, evo::SmallVector<Expr>&& args) -> Expr {
		evo::debugAssert(
			this->parent_module.getFunction(func).getReturnType().getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func, args), "Func call args don't match");

		return Expr(Expr::Kind::CallInst, this->calls.emplace_back(std::move(name), func, std::move(args)));
	}

	auto Function::createCallInst(std::string&& name, Function::ID func, const evo::SmallVector<Expr>& args) -> Expr {
		evo::debugAssert(
			this->parent_module.getFunction(func).getReturnType().getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func, args), "Func call args don't match");

		return Expr(Expr::Kind::CallInst, this->calls.emplace_back(std::move(name), func, args));
	}


	auto Function::createCallInst(std::string&& name, FunctionDecl::ID func, evo::SmallVector<Expr>&& args) -> Expr {
		evo::debugAssert(
			this->parent_module.getFunctionDecl(func).returnType.getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func, args), "Func call args don't match");

		return Expr(Expr::Kind::CallInst, this->calls.emplace_back(std::move(name), func, std::move(args)));
	}

	auto Function::createCallInst(std::string&& name, FunctionDecl::ID func, const evo::SmallVector<Expr>& args)
	-> Expr {
		evo::debugAssert(
			this->parent_module.getFunctionDecl(func).returnType.getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func, args), "Func call args don't match");

		return Expr(Expr::Kind::CallInst, this->calls.emplace_back(std::move(name), func, args));
	}


	auto Function::createCallInst(
		std::string&& name, const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args
	) -> Expr {
		evo::debugAssert(
			this->parent_module.getTypeFunction(func_type).returnType.getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func_type, args), "Func call args don't match");

		return Expr(
			Expr::Kind::CallInst, this->calls.emplace_back(std::move(name), PtrCall(func, func_type), std::move(args))
		);
	}

	auto Function::createCallInst(
		std::string&& name, const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args
	) -> Expr {
		evo::debugAssert(
			this->parent_module.getTypeFunction(func_type).returnType.getKind() != Type::Kind::Void,
			"CallInst cannot return `Void` (did you mean CallVoidInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func_type, args), "Func call args don't match");

		return Expr(Expr::Kind::CallInst, this->calls.emplace_back(std::move(name), PtrCall(func, func_type), args));
	}




	auto Function::createCallVoidInst(Function::ID func, evo::SmallVector<Expr>&& args) -> Expr {
		evo::debugAssert(
			this->parent_module.getFunction(func).getReturnType().getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func, args), "Func call args don't match");

		return Expr(Expr::Kind::CallVoidInst, this->call_voids.emplace_back(func, std::move(args)));
	}

	auto Function::createCallVoidInst(Function::ID func, const evo::SmallVector<Expr>& args) -> Expr {
		evo::debugAssert(
			this->parent_module.getFunction(func).getReturnType().getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func, args), "Func call args don't match");

		return Expr(Expr::Kind::CallVoidInst, this->call_voids.emplace_back(func, args));
	}


	auto Function::createCallVoidInst(FunctionDecl::ID func, evo::SmallVector<Expr>&& args) -> Expr {
		evo::debugAssert(
			this->parent_module.getFunctionDecl(func).returnType.getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func, args), "Func call args don't match");

		return Expr(Expr::Kind::CallVoidInst, this->call_voids.emplace_back(func, std::move(args)));
	}

	auto Function::createCallVoidInst(FunctionDecl::ID func, const evo::SmallVector<Expr>& args) -> Expr {
		evo::debugAssert(
			this->parent_module.getFunctionDecl(func).returnType.getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func, args), "Func call args don't match");

		return Expr(Expr::Kind::CallVoidInst, this->call_voids.emplace_back(func, args));
	}


	auto Function::createCallVoidInst(const Expr& func, const Type& func_type, evo::SmallVector<Expr>&& args) -> Expr {
		evo::debugAssert(
			this->parent_module.getTypeFunction(func_type).returnType.getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func_type, args), "Func call args don't match");

		return Expr(
			Expr::Kind::CallVoidInst, this->call_voids.emplace_back(PtrCall(func, func_type), std::move(args))
		);
	}

	auto Function::createCallVoidInst(const Expr& func, const Type& func_type, const evo::SmallVector<Expr>& args)
	-> Expr {
		evo::debugAssert(
			this->parent_module.getTypeFunction(func_type).returnType.getKind() == Type::Kind::Void,
			"CallVoidInst must return `Void` (did you mean CallInst?)"
		);
		evo::debugAssert(this->check_func_call_args(func_type, args), "Func call args don't match");

		return Expr(
			Expr::Kind::CallVoidInst, this->call_voids.emplace_back(PtrCall(func, func_type), args)
		);
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
		return this->check_func_call_args(this->parent_module.getTypeFunction(func_type).parameters, args);
	}

	auto Function::check_func_call_args(evo::ArrayProxy<Type> param_types, evo::ArrayProxy<Expr> args) const -> bool {
		if(param_types.size() != args.size()){ return false; }

		for(size_t i = 0; i < param_types.size(); i+=1){
			if(param_types[i] != this->getExprType(args[i])){ return false; }
		}

		return true;
	}
	

}