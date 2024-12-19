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
			case Expr::Kind::CallVoid:        evo::unreachable();
			case Expr::Kind::Breakpoint:      evo::unreachable();
			case Expr::Kind::Ret:             evo::unreachable();
			case Expr::Kind::Branch:          evo::unreachable();
			case Expr::Kind::CondBranch:      evo::unreachable();
			case Expr::Kind::Unreachable:     evo::unreachable();
			case Expr::Kind::Alloca:          return this->module.createPtrType();
			case Expr::Kind::Load:            return this->getLoad(expr).type;
			case Expr::Kind::Store:           evo::unreachable();
			case Expr::Kind::CalcPtr:         return this->module.createPtrType();
			case Expr::Kind::BitCast:         return this->getBitCast(expr).toType;
			case Expr::Kind::Trunc:           return this->getTrunc(expr).toType;
			case Expr::Kind::FTrunc:          return this->getFTrunc(expr).toType;
			case Expr::Kind::SExt:            return this->getSExt(expr).toType;
			case Expr::Kind::ZExt:            return this->getZExt(expr).toType;
			case Expr::Kind::FExt:            return this->getFExt(expr).toType;
			case Expr::Kind::IToF:            return this->getIToF(expr).toType;
			case Expr::Kind::UIToF:           return this->getUIToF(expr).toType;
			case Expr::Kind::FToI:            return this->getFToI(expr).toType;
			case Expr::Kind::FToUI:           return this->getFToUI(expr).toType;
			case Expr::Kind::Add:             return this->getExprType(this->getAdd(expr).lhs);
			case Expr::Kind::SAddWrap:        evo::unreachable();
			case Expr::Kind::SAddWrapResult:  return this->getExprType(this->getSAddWrap(expr).lhs);
			case Expr::Kind::SAddWrapWrapped: return this->module.createBoolType();
			case Expr::Kind::UAddWrap:        evo::unreachable();
			case Expr::Kind::UAddWrapResult:  return this->getExprType(this->getUAddWrap(expr).lhs);
			case Expr::Kind::UAddWrapWrapped: return this->module.createBoolType();
			case Expr::Kind::SAddSat:         return this->getExprType(this->getSAddSat(expr).lhs);
			case Expr::Kind::UAddSat:         return this->getExprType(this->getUAddSat(expr).lhs);
			case Expr::Kind::FAdd:            return this->getExprType(this->getFAdd(expr).lhs);
			case Expr::Kind::Sub:             return this->getExprType(this->getSub(expr).lhs);
			case Expr::Kind::SSubWrap:        evo::unreachable();
			case Expr::Kind::SSubWrapResult:  return this->getExprType(this->getSSubWrap(expr).lhs);
			case Expr::Kind::SSubWrapWrapped: return this->module.createBoolType();
			case Expr::Kind::USubWrap:        evo::unreachable();
			case Expr::Kind::USubWrapResult:  return this->getExprType(this->getUSubWrap(expr).lhs);
			case Expr::Kind::USubWrapWrapped: return this->module.createBoolType();
			case Expr::Kind::SSubSat:         return this->getExprType(this->getSSubSat(expr).lhs);
			case Expr::Kind::USubSat:         return this->getExprType(this->getUSubSat(expr).lhs);
			case Expr::Kind::FSub:            return this->getExprType(this->getFSub(expr).lhs);
			case Expr::Kind::Mul:             return this->getExprType(this->getMul(expr).lhs);
			case Expr::Kind::SMulWrap:        evo::unreachable();
			case Expr::Kind::SMulWrapResult:  return this->getExprType(this->getSMulWrap(expr).lhs);
			case Expr::Kind::SMulWrapWrapped: return this->module.createBoolType();
			case Expr::Kind::UMulWrap:        evo::unreachable();
			case Expr::Kind::UMulWrapResult:  return this->getExprType(this->getUMulWrap(expr).lhs);
			case Expr::Kind::UMulWrapWrapped: return this->module.createBoolType();
			case Expr::Kind::SMulSat:         return this->getExprType(this->getSMulSat(expr).lhs);
			case Expr::Kind::UMulSat:         return this->getExprType(this->getUMulSat(expr).lhs);
			case Expr::Kind::FMul:            return this->getExprType(this->getFMul(expr).lhs);
			case Expr::Kind::SDiv:            return this->getExprType(this->getSDiv(expr).lhs);
			case Expr::Kind::UDiv:            return this->getExprType(this->getUDiv(expr).lhs);
			case Expr::Kind::FDiv:            return this->getExprType(this->getFDiv(expr).lhs);
			case Expr::Kind::SRem:            return this->getExprType(this->getSRem(expr).lhs);
			case Expr::Kind::URem:            return this->getExprType(this->getURem(expr).lhs);
			case Expr::Kind::FRem:            return this->getExprType(this->getFRem(expr).lhs);
			case Expr::Kind::IEq:             return this->module.createBoolType();
			case Expr::Kind::FEq:             return this->module.createBoolType();
			case Expr::Kind::INeq:            return this->module.createBoolType();
			case Expr::Kind::FNeq:            return this->module.createBoolType();
			case Expr::Kind::SLT:             return this->module.createBoolType();
			case Expr::Kind::ULT:             return this->module.createBoolType();
			case Expr::Kind::FLT:             return this->module.createBoolType();
			case Expr::Kind::SLTE:            return this->module.createBoolType();
			case Expr::Kind::ULTE:            return this->module.createBoolType();
			case Expr::Kind::FLTE:            return this->module.createBoolType();
			case Expr::Kind::SGT:             return this->module.createBoolType();
			case Expr::Kind::UGT:             return this->module.createBoolType();
			case Expr::Kind::FGT:             return this->module.createBoolType();
			case Expr::Kind::SGTE:            return this->module.createBoolType();
			case Expr::Kind::UGTE:            return this->module.createBoolType();
			case Expr::Kind::FGTE:            return this->module.createBoolType();
			case Expr::Kind::And:             return this->getExprType(this->getAnd(expr).lhs);
			case Expr::Kind::Or:              return this->getExprType(this->getOr(expr).lhs);
			case Expr::Kind::Xor:             return this->getExprType(this->getXor(expr).lhs);
			case Expr::Kind::SHL:             return this->getExprType(this->getSHL(expr).lhs);
			case Expr::Kind::SSHLSat:         return this->getExprType(this->getSSHLSat(expr).lhs);
			case Expr::Kind::USHLSat:         return this->getExprType(this->getUSHLSat(expr).lhs);
			case Expr::Kind::SSHR:            return this->getExprType(this->getSSHR(expr).lhs);
			case Expr::Kind::USHR:            return this->getExprType(this->getUSHR(expr).lhs);
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



	auto ReaderAgent::getBitCast(const Expr& expr) const -> const BitCast& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::BitCast, "Not a BitCast");

		return this->module.bitcasts[expr.index];
	}

	auto ReaderAgent::getTrunc(const Expr& expr) const -> const Trunc& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Trunc, "Not a Trunc");

		return this->module.truncs[expr.index];
	}

	auto ReaderAgent::getFTrunc(const Expr& expr) const -> const FTrunc& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FTrunc, "Not a FTrunc");

		return this->module.ftruncs[expr.index];
	}

	auto ReaderAgent::getSExt(const Expr& expr) const -> const SExt& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SExt, "Not a SExt");

		return this->module.sexts[expr.index];
	}

	auto ReaderAgent::getZExt(const Expr& expr) const -> const ZExt& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::ZExt, "Not a ZExt");

		return this->module.zexts[expr.index];
	}

	auto ReaderAgent::getFExt(const Expr& expr) const -> const FExt& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FExt, "Not a FExt");

		return this->module.fexts[expr.index];
	}

	auto ReaderAgent::getIToF(const Expr& expr) const -> const IToF& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::IToF, "Not a IToF");

		return this->module.itofs[expr.index];
	}

	auto ReaderAgent::getUIToF(const Expr& expr) const -> const UIToF& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::UIToF, "Not a UIToF");

		return this->module.uitofs[expr.index];
	}

	auto ReaderAgent::getFToI(const Expr& expr) const -> const FToI& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FToI, "Not a FToI");

		return this->module.ftois[expr.index];
	}

	auto ReaderAgent::getFToUI(const Expr& expr) const -> const FToUI& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FToUI, "Not a FToUI");

		return this->module.ftouis[expr.index];
	}




	auto ReaderAgent::getAdd(const Expr& expr) const -> const Add& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Add, "Not an add");

		return this->module.adds[expr.index];
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

	auto ReaderAgent::getSAddSat(const Expr& expr) const -> const SAddSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SAddSat, "Not an saddSat");

		return this->module.sadd_sats[expr.index];
	}

	auto ReaderAgent::getUAddSat(const Expr& expr) const -> const UAddSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::UAddSat, "Not an uaddSat");

		return this->module.uadd_sats[expr.index];
	}

	auto ReaderAgent::getFAdd(const Expr& expr) const -> const FAdd& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FAdd, "Not an fadd");

		return this->module.fadds[expr.index];
	}




	auto ReaderAgent::getSub(const Expr& expr) const -> const Sub& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Sub, "Not a sub");

		return this->module.subs[expr.index];
	}


	auto ReaderAgent::getSSubWrap(const Expr& expr) const -> const SSubWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.getKind() == Expr::Kind::SSubWrap
				|| expr.getKind() == Expr::Kind::SSubWrapResult
				|| expr.getKind() == Expr::Kind::SSubWrapWrapped,
			"Not a signed sub wrap"
		);

		return this->module.ssub_wraps[expr.index];
	}

	auto ReaderAgent::extractSSubWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SSubWrapResult, expr.index);
	}

	auto ReaderAgent::extractSSubWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SSubWrapWrapped, expr.index);
	}



	auto ReaderAgent::getUSubWrap(const Expr& expr) const -> const USubWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.getKind() == Expr::Kind::USubWrap
				|| expr.getKind() == Expr::Kind::USubWrapResult
				|| expr.getKind() == Expr::Kind::USubWrapWrapped,
			"Not an unsigned sub wrap"
		);

		return this->module.usub_wraps[expr.index];
	}

	auto ReaderAgent::extractUSubWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::USubWrapResult, expr.index);
	}

	auto ReaderAgent::extractUSubWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::USubWrapWrapped, expr.index);
	}

	auto ReaderAgent::getSSubSat(const Expr& expr) const -> const SSubSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SSubSat, "Not a ssubSat");

		return this->module.ssub_sats[expr.index];
	}

	auto ReaderAgent::getUSubSat(const Expr& expr) const -> const USubSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::USubSat, "Not a usubSat");

		return this->module.usub_sats[expr.index];
	}

	auto ReaderAgent::getFSub(const Expr& expr) const -> const FSub& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FSub, "Not an fsub");

		return this->module.fsubs[expr.index];
	}




	auto ReaderAgent::getMul(const Expr& expr) const -> const Mul& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Mul, "Not a mul");

		return this->module.muls[expr.index];
	}


	auto ReaderAgent::getSMulWrap(const Expr& expr) const -> const SMulWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.getKind() == Expr::Kind::SMulWrap
				|| expr.getKind() == Expr::Kind::SMulWrapResult
				|| expr.getKind() == Expr::Kind::SMulWrapWrapped,
			"Not a signed mul wrap"
		);

		return this->module.smul_wraps[expr.index];
	}

	auto ReaderAgent::extractSMulWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SMulWrapResult, expr.index);
	}

	auto ReaderAgent::extractSMulWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SMulWrapWrapped, expr.index);
	}



	auto ReaderAgent::getUMulWrap(const Expr& expr) const -> const UMulWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.getKind() == Expr::Kind::UMulWrap
				|| expr.getKind() == Expr::Kind::UMulWrapResult
				|| expr.getKind() == Expr::Kind::UMulWrapWrapped,
			"Not an unsigned mul wrap"
		);

		return this->module.umul_wraps[expr.index];
	}

	auto ReaderAgent::extractUMulWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::UMulWrapResult, expr.index);
	}

	auto ReaderAgent::extractUMulWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::UMulWrapWrapped, expr.index);
	}

	auto ReaderAgent::getSMulSat(const Expr& expr) const -> const SMulSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SMulSat, "Not a smulSat");

		return this->module.smul_sats[expr.index];
	}

	auto ReaderAgent::getUMulSat(const Expr& expr) const -> const UMulSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::UMulSat, "Not a umulSat");

		return this->module.umul_sats[expr.index];
	}

	auto ReaderAgent::getFMul(const Expr& expr) const -> const FMul& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FMul, "Not an fmul");

		return this->module.fmuls[expr.index];
	}


	auto ReaderAgent::getSDiv(const Expr& expr) const -> const SDiv& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SDiv, "Not an sdiv");

		return this->module.sdivs[expr.index];
	}

	auto ReaderAgent::getUDiv(const Expr& expr) const -> const UDiv& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::UDiv, "Not an udiv");

		return this->module.udivs[expr.index];
	}

	auto ReaderAgent::getFDiv(const Expr& expr) const -> const FDiv& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FDiv, "Not an fdiv");

		return this->module.fdivs[expr.index];
	}

	auto ReaderAgent::getSRem(const Expr& expr) const -> const SRem& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SRem, "Not an srem");

		return this->module.srems[expr.index];
	}

	auto ReaderAgent::getURem(const Expr& expr) const -> const URem& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::URem, "Not an urem");

		return this->module.urems[expr.index];
	}

	auto ReaderAgent::getFRem(const Expr& expr) const -> const FRem& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FRem, "Not an frem");

		return this->module.frems[expr.index];
	}


	auto ReaderAgent::getIEq(const Expr& expr) const -> const IEq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::IEq, "Not an ieq");

		return this->module.ieqs[expr.index];
	}

	auto ReaderAgent::getFEq(const Expr& expr) const -> const FEq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FEq, "Not an feq");

		return this->module.feqs[expr.index];
	}

	auto ReaderAgent::getINeq(const Expr& expr) const -> const INeq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::INeq, "Not an ineq");

		return this->module.ineqs[expr.index];
	}

	auto ReaderAgent::getFNeq(const Expr& expr) const -> const FNeq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FNeq, "Not an fneq");

		return this->module.fneqs[expr.index];
	}

	auto ReaderAgent::getSLT(const Expr& expr) const -> const SLT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SLT, "Not an slt");

		return this->module.slts[expr.index];
	}

	auto ReaderAgent::getULT(const Expr& expr) const -> const ULT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::ULT, "Not an ult");

		return this->module.ults[expr.index];
	}

	auto ReaderAgent::getFLT(const Expr& expr) const -> const FLT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FLT, "Not an flt");

		return this->module.flts[expr.index];
	}

	auto ReaderAgent::getSLTE(const Expr& expr) const -> const SLTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SLTE, "Not an slte");

		return this->module.sltes[expr.index];
	}

	auto ReaderAgent::getULTE(const Expr& expr) const -> const ULTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::ULTE, "Not an ulte");

		return this->module.ultes[expr.index];
	}

	auto ReaderAgent::getFLTE(const Expr& expr) const -> const FLTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FLTE, "Not an flte");

		return this->module.fltes[expr.index];
	}

	auto ReaderAgent::getSGT(const Expr& expr) const -> const SGT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SGT, "Not an sgt");

		return this->module.sgts[expr.index];
	}

	auto ReaderAgent::getUGT(const Expr& expr) const -> const UGT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::UGT, "Not an ugt");

		return this->module.ugts[expr.index];
	}

	auto ReaderAgent::getFGT(const Expr& expr) const -> const FGT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FGT, "Not an fgt");

		return this->module.fgts[expr.index];
	}

	auto ReaderAgent::getSGTE(const Expr& expr) const -> const SGTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SGTE, "Not an sgte");

		return this->module.sgtes[expr.index];
	}

	auto ReaderAgent::getUGTE(const Expr& expr) const -> const UGTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::UGTE, "Not an ugte");

		return this->module.ugtes[expr.index];
	}

	auto ReaderAgent::getFGTE(const Expr& expr) const -> const FGTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::FGTE, "Not an fgte");

		return this->module.fgtes[expr.index];
	}




	auto ReaderAgent::getAnd(const Expr& expr) const -> const And& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::And, "Not an and");

		return this->module.ands[expr.index];
	}

	auto ReaderAgent::getOr(const Expr& expr) const -> const Or& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Or, "Not an or");

		return this->module.ors[expr.index];
	}

	auto ReaderAgent::getXor(const Expr& expr) const -> const Xor& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::Xor, "Not an xor");

		return this->module.xors[expr.index];
	}

	auto ReaderAgent::getSHL(const Expr& expr) const -> const SHL& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SHL, "Not an shl");

		return this->module.shls[expr.index];
	}

	auto ReaderAgent::getSSHLSat(const Expr& expr) const -> const SSHLSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SSHLSat, "Not an sshlsat");

		return this->module.sshlsats[expr.index];
	}

	auto ReaderAgent::getUSHLSat(const Expr& expr) const -> const USHLSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::USHLSat, "Not an ushlsat");

		return this->module.ushlsats[expr.index];
	}

	auto ReaderAgent::getSSHR(const Expr& expr) const -> const SSHR& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::SSHR, "Not an sshr");

		return this->module.sshrs[expr.index];
	}

	auto ReaderAgent::getUSHR(const Expr& expr) const -> const USHR& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.getKind() == Expr::Kind::USHR, "Not an ushr");

		return this->module.ushrs[expr.index];
	}




}