////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/ReaderAgent.h"



namespace pcit::pir{


	auto ReaderAgent::getExprType(const Expr& expr) const -> Type {		
		evo::debugAssert(expr.isValue(), "Expr must be a value in order to get the type");

		switch(expr.getKind()){
			case Expr::Kind::None:         evo::unreachable();
			case Expr::Kind::Number:       return this->getNumber(expr).type;
			case Expr::Kind::Boolean:      return this->module.createBoolType();
			case Expr::Kind::GlobalValue:  return this->module.createPtrType();
			case Expr::Kind::ParamExpr: {
				evo::debugAssert(this->hasTargetFunction(), "No target function is set");

				const ParamExpr param_expr = this->getParamExpr(expr);
				evo::debugAssert(
					this->target_func->getParameters().size() > param_expr.index,
					"This function does not have a parameter of index {}", param_expr.index
				);
				return this->target_func->getParameters()[param_expr.index].getType();
			} break;
			case Expr::Kind::Call: {
				evo::debugAssert(this->hasTargetFunction(), "No target function is set");

				const Call& call_inst = this->getCall(expr);

				return call_inst.target.visit([&](const auto& target) -> Type {
					using ValueT = std::decay_t<decltype(target)>;

					if constexpr(std::is_same<ValueT, Function::ID>()){
						return this->module.getFunction(target).getReturnType();

					}else if constexpr(std::is_same<ValueT, FunctionDecl::ID>()){
						return this->module.getFunctionDecl(target).returnType;
						
					}else if constexpr(std::is_same<ValueT, PtrCall>()){
						return this->module.getFunctionType(target.funcType).returnType;

					}else{
						static_assert(false, "Unsupported call inst target");
					}
				});
			} break;
			case Expr::Kind::CallVoid:       evo::unreachable();
			case Expr::Kind::Ret:            evo::unreachable();
			case Expr::Kind::Branch:         evo::unreachable();
			case Expr::Kind::Alloca:         return this->module.createPtrType();
			case Expr::Kind::Add:            return this->getExprType(this->getAdd(expr).lhs);
			case Expr::Kind::AddWrap:        evo::unreachable();
			case Expr::Kind::AddWrapResult:  return this->getExprType(this->getAddWrap(expr).lhs);
			case Expr::Kind::AddWrapWrapped: return this->module.createBoolType();
		}

		evo::unreachable();
	}

	
	auto ReaderAgent::getBasicBlock(BasicBlock::ID id) const -> const BasicBlock& {
		return this->module.basic_blocks[id];
	}


	auto ReaderAgent::getNumber(const Expr& expr) const -> const Number& {
		evo::debugAssert(expr.getKind() == Expr::Kind::Number, "Not a number");
		return this->module.numbers[expr.index];
	}

	auto ReaderAgent::getBoolean(const Expr& expr) -> bool {
		evo::debugAssert(expr.getKind() == Expr::Kind::Boolean, "Not a Boolean");
		return bool(expr.index);
	}


	auto ReaderAgent::getParamExpr(const Expr& expr) -> ParamExpr {
		evo::debugAssert(expr.getKind() == Expr::Kind::ParamExpr, "not a param expr");
		return ParamExpr(expr.index);
	}

	auto ReaderAgent::getGlobalValue(const Expr& expr) const -> const GlobalVar& {
		evo::debugAssert(expr.getKind() == Expr::Kind::GlobalValue, "Not global");
		return this->module.getGlobalVar(GlobalVar::ID(expr.index));
	}



	auto ReaderAgent::getCall(const Expr& expr) const -> const Call& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Call, "not a call inst");

		return this->module.calls[expr.index];
	}

	auto ReaderAgent::getCallVoid(const Expr& expr) const -> const CallVoid& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::CallVoid, "not a call void inst");

		return this->module.call_voids[expr.index];
	}




	auto ReaderAgent::getRet(const Expr& expr) const -> const Ret& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Ret, "Not a ret");

		return this->module.rets[expr.index];
	}



	auto ReaderAgent::getBranch(const Expr& expr) -> Branch {
		evo::debugAssert(expr.getKind() == Expr::Kind::Branch, "Not a br");

		return Branch(BasicBlock::ID(expr.index));
	}

	auto ReaderAgent::getAlloca(const Expr& expr) const -> const Alloca& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Alloca, "Not an alloca");

		return this->target_func->allocas[expr.index];
	}

	auto ReaderAgent::getAdd(const Expr& expr) const -> const Add& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Add, "Not an add");

		return this->module.adds[expr.index];
	}


	auto ReaderAgent::getAddWrap(const Expr& expr) const -> const AddWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.getKind() == Expr::Kind::AddWrap
				|| expr.getKind() == Expr::Kind::AddWrapResult
				|| expr.getKind() == Expr::Kind::AddWrapWrapped,
			"Not an add wrap"
		);

		return this->module.add_wraps[expr.index];
	}

	auto ReaderAgent::extractAddWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::AddWrapResult, expr.index);
	}

	auto ReaderAgent::extractAddWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::AddWrapWrapped, expr.index);
	}


}