//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/ReaderAgent.h"



namespace pcit::pir{


	auto ReaderAgent::getExprType(const Expr& expr) const -> Type {		
		evo::debugAssert(expr.isValue(), "Expr must be a value in order to get the type");

		switch(expr.getKind()){
			case Expr::Kind::None:         evo::unreachable();
			case Expr::Kind::Number:       return this->getNumber(expr).type;
			case Expr::Kind::GlobalValue:  return this->module.createTypePtr();
			case Expr::Kind::ParamExpr: {
				evo::debugAssert(this->hasTargetFunction(), "No target function is set");

				const ParamExpr param_expr = this->getParamExpr(expr);
				return this->target_func->getParameters()[param_expr.index].getType();
			} break;
			case Expr::Kind::CallInst: {
				evo::debugAssert(this->hasTargetFunction(), "No target function is set");

				const CallInst& call_inst = this->getCallInst(expr);

				return call_inst.target.visit([&](const auto& target) -> Type {
					using ValueT = std::decay_t<decltype(target)>;

					if constexpr(std::is_same_v<ValueT, Function::ID>){
						return this->module.getFunction(target).getReturnType();

					}else if constexpr(std::is_same_v<ValueT, FunctionDecl::ID>){
						return this->module.getFunctionDecl(target).returnType;
						
					}else if constexpr(std::is_same_v<ValueT, PtrCall>){
						return this->module.getTypeFunction(target.funcType).returnType;

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

	
	auto ReaderAgent::getBasicBlock(BasicBlock::ID id) const -> const BasicBlock& {
		return this->module.basic_blocks[id];
	}


	auto ReaderAgent::getNumber(const Expr& expr) const -> const Number& {
		evo::debugAssert(expr.getKind() == Expr::Kind::Number, "Not a number");
		return this->module.numbers[expr.index];
	}


	auto ReaderAgent::getParamExpr(const Expr& expr) -> ParamExpr {
		evo::debugAssert(expr.getKind() == Expr::Kind::ParamExpr, "not a param expr");
		return ParamExpr(expr.index);
	}



	auto ReaderAgent::getCallInst(const Expr& expr) const -> const CallInst& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::CallInst, "not a call inst");

		return this->target_func->calls[expr.index];
	}

	auto ReaderAgent::getCallVoidInst(const Expr& expr) const -> const CallVoidInst& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::CallVoidInst, "not a call void inst");

		return this->target_func->call_voids[expr.index];
	}




	auto ReaderAgent::getRetInst(const Expr& expr) const -> const RetInst& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::RetInst, "Not a ret");

		return this->target_func->rets[expr.index];
	}



	auto ReaderAgent::getBrInst(const Expr& expr) -> BrInst {
		evo::debugAssert(expr.getKind() == Expr::Kind::BrInst, "Not a br");

		return BrInst(BasicBlock::ID(expr.index));
	}


	auto ReaderAgent::getAdd(const Expr& expr) const -> const Add& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Add, "Not an add");

		return this->target_func->adds[expr.index];
	}


}