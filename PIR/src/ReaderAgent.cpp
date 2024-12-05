////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/ReaderAgent.h"

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


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
			case Expr::Kind::Breakpoint:      evo::unreachable();
			case Expr::Kind::Ret:             evo::unreachable();
			case Expr::Kind::Branch:          evo::unreachable();
			case Expr::Kind::CondBranch:      evo::unreachable();
			case Expr::Kind::Unreachable:     evo::unreachable();
			case Expr::Kind::Alloca:          return this->module.createPtrType();
			case Expr::Kind::Load:            return this->getLoad(expr).type;
			case Expr::Kind::Store:           evo::unreachable();
			case Expr::Kind::CalcPtr:         return this->module.createPtrType();
			case Expr::Kind::Add:             return this->getExprType(this->getAdd(expr).lhs);
			case Expr::Kind::FAdd:            return this->getExprType(this->getFAdd(expr).lhs);
			case Expr::Kind::SAddWrap:        evo::unreachable();
			case Expr::Kind::SAddWrapResult:  return this->getExprType(this->getSAddWrap(expr).lhs);
			case Expr::Kind::SAddWrapWrapped: return this->module.createBoolType();
			case Expr::Kind::UAddWrap:        evo::unreachable();
			case Expr::Kind::UAddWrapResult:  return this->getExprType(this->getUAddWrap(expr).lhs);
			case Expr::Kind::UAddWrapWrapped: return this->module.createBoolType();
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


	auto ReaderAgent::getCondBranch(const Expr& expr) const -> const CondBranch& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::CondBranch, "Not an cond branch");

		return this->module.cond_branches[expr.index];
	}


	auto ReaderAgent::getAlloca(const Expr& expr) const -> const Alloca& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Alloca, "Not an alloca");

		return this->target_func->allocas[expr.index];
	}

	auto ReaderAgent::getLoad(const Expr& expr) const -> const Load& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Load, "Not a load");

		return this->module.loads[expr.index];
	}

	auto ReaderAgent::getStore(const Expr& expr) const -> const Store& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Store, "Not a store");

		return this->module.stores[expr.index];
	}


	auto ReaderAgent::getCalcPtr(const Expr& expr) const -> const CalcPtr& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::CalcPtr, "Not a calc ptr");

		return this->module.calc_ptrs[expr.index];
	}


	auto ReaderAgent::getAdd(const Expr& expr) const -> const Add& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Add, "Not an add");

		return this->module.adds[expr.index];
	}


	auto ReaderAgent::getFAdd(const Expr& expr) const -> const FAdd& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FAdd, "Not an fadd");

		return this->module.fadds[expr.index];
	}


	auto ReaderAgent::getSAddWrap(const Expr& expr) const -> const SAddWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.getKind() == Expr::Kind::SAddWrap
				|| expr.getKind() == Expr::Kind::SAddWrapResult
				|| expr.getKind() == Expr::Kind::SAddWrapWrapped,
			"Not a signed add wrap"
		);

		return this->module.sadd_wraps[expr.index];
	}

	auto ReaderAgent::extractSAddWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SAddWrapResult, expr.index);
	}

	auto ReaderAgent::extractSAddWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SAddWrapWrapped, expr.index);
	}



	auto ReaderAgent::getUAddWrap(const Expr& expr) const -> const UAddWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.getKind() == Expr::Kind::UAddWrap
				|| expr.getKind() == Expr::Kind::UAddWrapResult
				|| expr.getKind() == Expr::Kind::UAddWrapWrapped,
			"Not an unsigned add wrap"
		);

		return this->module.uadd_wraps[expr.index];
	}

	auto ReaderAgent::extractUAddWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::UAddWrapResult, expr.index);
	}

	auto ReaderAgent::extractUAddWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::UAddWrapWrapped, expr.index);
	}


}