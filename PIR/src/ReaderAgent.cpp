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

		switch(expr.kind()){
			case Expr::Kind::NONE:             evo::unreachable();
			case Expr::Kind::GLOBAL_VALUE:     return this->module.createPtrType();
			case Expr::Kind::FUNCTION_POINTER: return this->module.createPtrType();
			case Expr::Kind::NUMBER:           return this->getNumber(expr).type;
			case Expr::Kind::BOOLEAN:          return this->module.createBoolType();
			case Expr::Kind::NULLPTR:          return this->module.createPtrType();
			case Expr::Kind::PARAM_EXPR: {
				evo::debugAssert(this->hasTargetFunction(), "No target function is set");

				const ParamExpr param_expr = this->getParamExpr(expr);
				evo::debugAssert(
					this->target_func->getParameters().size() > param_expr.index,
					"This function does not have a parameter of index {}", param_expr.index
				);
				return this->target_func->getParameters()[param_expr.index].getType();
			} break;
			case Expr::Kind::CALL: {
				evo::debugAssert(this->hasTargetFunction(), "No target function is set");

				const Call& call_inst = this->getCall(expr);

				return call_inst.target.visit([&](const auto& target) -> Type {
					using ValueT = std::decay_t<decltype(target)>;

					if constexpr(std::is_same<ValueT, Function::ID>()){
						return this->module.getFunction(target).getReturnType();

					}else if constexpr(std::is_same<ValueT, ExternalFunction::ID>()){
						return this->module.getExternalFunction(target).returnType;
						
					}else if constexpr(std::is_same<ValueT, PtrCall>()){
						return this->module.getFunctionType(target.funcType).returnType;

					}else{
						static_assert(false, "Unsupported call inst target");
					}
				});
			} break;
			case Expr::Kind::CALL_VOID:         evo::unreachable();
			case Expr::Kind::ABORT:             evo::unreachable();
			case Expr::Kind::BREAKPOINT:        evo::unreachable();
			case Expr::Kind::RET:               evo::unreachable();
			case Expr::Kind::JUMP:              evo::unreachable();
			case Expr::Kind::BRANCH:            evo::unreachable();
			case Expr::Kind::UNREACHABLE:       evo::unreachable();
			case Expr::Kind::PHI:               return this->getExprType(this->getPhi(expr).predecessors[0].value);
			case Expr::Kind::ALLOCA:            return this->module.createPtrType();
			case Expr::Kind::LOAD:              return this->getLoad(expr).type;
			case Expr::Kind::STORE:             evo::unreachable();
			case Expr::Kind::CALC_PTR:          return this->module.createPtrType();
			case Expr::Kind::MEMCPY:            evo::unreachable();
			case Expr::Kind::MEMSET:            evo::unreachable();
			case Expr::Kind::BIT_CAST:          return this->getBitCast(expr).toType;
			case Expr::Kind::TRUNC:             return this->getTrunc(expr).toType;
			case Expr::Kind::FTRUNC:            return this->getFTrunc(expr).toType;
			case Expr::Kind::SEXT:              return this->getSExt(expr).toType;
			case Expr::Kind::ZEXT:              return this->getZExt(expr).toType;
			case Expr::Kind::FEXT:              return this->getFExt(expr).toType;
			case Expr::Kind::ITOF:              return this->getIToF(expr).toType;
			case Expr::Kind::UITOF:             return this->getUIToF(expr).toType;
			case Expr::Kind::FTOI:              return this->getFToI(expr).toType;
			case Expr::Kind::FTOUI:             return this->getFToUI(expr).toType;
			case Expr::Kind::ADD:               return this->getExprType(this->getAdd(expr).lhs);
			case Expr::Kind::SADD_WRAP:         evo::unreachable();
			case Expr::Kind::SADD_WRAP_RESULT:  return this->getExprType(this->getSAddWrap(expr).lhs);
			case Expr::Kind::SADD_WRAP_WRAPPED: return this->module.createBoolType();
			case Expr::Kind::UADD_WRAP:         evo::unreachable();
			case Expr::Kind::UADD_WRAP_RESULT:  return this->getExprType(this->getUAddWrap(expr).lhs);
			case Expr::Kind::UADD_WRAP_WRAPPED: return this->module.createBoolType();
			case Expr::Kind::SADD_SAT:          return this->getExprType(this->getSAddSat(expr).lhs);
			case Expr::Kind::UADD_SAT:          return this->getExprType(this->getUAddSat(expr).lhs);
			case Expr::Kind::FADD:              return this->getExprType(this->getFAdd(expr).lhs);
			case Expr::Kind::SUB:               return this->getExprType(this->getSub(expr).lhs);
			case Expr::Kind::SSUB_WRAP:         evo::unreachable();
			case Expr::Kind::SSUB_WRAP_RESULT:  return this->getExprType(this->getSSubWrap(expr).lhs);
			case Expr::Kind::SSUB_WRAP_WRAPPED: return this->module.createBoolType();
			case Expr::Kind::USUB_WRAP:         evo::unreachable();
			case Expr::Kind::USUB_WRAP_RESULT:  return this->getExprType(this->getUSubWrap(expr).lhs);
			case Expr::Kind::USUB_WRAP_WRAPPED: return this->module.createBoolType();
			case Expr::Kind::SSUB_SAT:          return this->getExprType(this->getSSubSat(expr).lhs);
			case Expr::Kind::USUB_SAT:          return this->getExprType(this->getUSubSat(expr).lhs);
			case Expr::Kind::FSUB:              return this->getExprType(this->getFSub(expr).lhs);
			case Expr::Kind::MUL:               return this->getExprType(this->getMul(expr).lhs);
			case Expr::Kind::SMUL_WRAP:         evo::unreachable();
			case Expr::Kind::SMUL_WRAP_RESULT:  return this->getExprType(this->getSMulWrap(expr).lhs);
			case Expr::Kind::SMUL_WRAP_WRAPPED: return this->module.createBoolType();
			case Expr::Kind::UMUL_WRAP:         evo::unreachable();
			case Expr::Kind::UMUL_WRAP_RESULT:  return this->getExprType(this->getUMulWrap(expr).lhs);
			case Expr::Kind::UMUL_WRAP_WRAPPED: return this->module.createBoolType();
			case Expr::Kind::SMUL_SAT:          return this->getExprType(this->getSMulSat(expr).lhs);
			case Expr::Kind::UMUL_SAT:          return this->getExprType(this->getUMulSat(expr).lhs);
			case Expr::Kind::FMUL:              return this->getExprType(this->getFMul(expr).lhs);
			case Expr::Kind::SDIV:              return this->getExprType(this->getSDiv(expr).lhs);
			case Expr::Kind::UDIV:              return this->getExprType(this->getUDiv(expr).lhs);
			case Expr::Kind::FDIV:              return this->getExprType(this->getFDiv(expr).lhs);
			case Expr::Kind::SREM:              return this->getExprType(this->getSRem(expr).lhs);
			case Expr::Kind::UREM:              return this->getExprType(this->getURem(expr).lhs);
			case Expr::Kind::FREM:              return this->getExprType(this->getFRem(expr).lhs);
			case Expr::Kind::FNEG:              return this->getExprType(this->getFNeg(expr).rhs);
			case Expr::Kind::IEQ:               return this->module.createBoolType();
			case Expr::Kind::FEQ:               return this->module.createBoolType();
			case Expr::Kind::INEQ:              return this->module.createBoolType();
			case Expr::Kind::FNEQ:              return this->module.createBoolType();
			case Expr::Kind::SLT:               return this->module.createBoolType();
			case Expr::Kind::ULT:               return this->module.createBoolType();
			case Expr::Kind::FLT:               return this->module.createBoolType();
			case Expr::Kind::SLTE:              return this->module.createBoolType();
			case Expr::Kind::ULTE:              return this->module.createBoolType();
			case Expr::Kind::FLTE:              return this->module.createBoolType();
			case Expr::Kind::SGT:               return this->module.createBoolType();
			case Expr::Kind::UGT:               return this->module.createBoolType();
			case Expr::Kind::FGT:               return this->module.createBoolType();
			case Expr::Kind::SGTE:              return this->module.createBoolType();
			case Expr::Kind::UGTE:              return this->module.createBoolType();
			case Expr::Kind::FGTE:              return this->module.createBoolType();
			case Expr::Kind::AND:               return this->getExprType(this->getAnd(expr).lhs);
			case Expr::Kind::OR:                return this->getExprType(this->getOr(expr).lhs);
			case Expr::Kind::XOR:               return this->getExprType(this->getXor(expr).lhs);
			case Expr::Kind::SHL:               return this->getExprType(this->getSHL(expr).lhs);
			case Expr::Kind::SSHL_SAT:          return this->getExprType(this->getSSHLSat(expr).lhs);
			case Expr::Kind::USHL_SAT:          return this->getExprType(this->getUSHLSat(expr).lhs);
			case Expr::Kind::SSHR:              return this->getExprType(this->getSSHR(expr).lhs);
			case Expr::Kind::USHR:              return this->getExprType(this->getUSHR(expr).lhs);
			case Expr::Kind::BIT_REVERSE:       return this->getExprType(this->getBitReverse(expr).arg);
			case Expr::Kind::BSWAP:             return this->getExprType(this->getBSwap(expr).arg);
			case Expr::Kind::CTPOP:             return this->getExprType(this->getCtPop(expr).arg);
			case Expr::Kind::CTLZ:              return this->getExprType(this->getCTLZ(expr).arg);
			case Expr::Kind::CTTZ:              return this->getExprType(this->getCTTZ(expr).arg);
		}

		evo::unreachable();
	}

	
	auto ReaderAgent::getBasicBlock(BasicBlock::ID id) const -> const BasicBlock& {
		return this->module.basic_blocks[id];
	}


	auto ReaderAgent::getNumber(const Expr& expr) const -> const Number& {
		evo::debugAssert(expr.kind() == Expr::Kind::NUMBER, "Not a number");
		return this->module.numbers[expr.index];
	}

	auto ReaderAgent::getBoolean(const Expr& expr) -> bool {
		evo::debugAssert(expr.kind() == Expr::Kind::BOOLEAN, "Not a Boolean");
		return bool(expr.index);
	}


	auto ReaderAgent::getParamExpr(const Expr& expr) -> ParamExpr {
		evo::debugAssert(expr.kind() == Expr::Kind::PARAM_EXPR, "not a param expr");
		return ParamExpr(expr.index);
	}

	auto ReaderAgent::getGlobalValue(const Expr& expr) const -> const GlobalVar& {
		evo::debugAssert(expr.kind() == Expr::Kind::GLOBAL_VALUE, "Not a global");
		return this->module.getGlobalVar(GlobalVar::ID(expr.index));
	}

	auto ReaderAgent::getFunctionPointer(const Expr& expr) const -> const Function& {
		evo::debugAssert(expr.kind() == Expr::Kind::FUNCTION_POINTER, "Not a function pointer");
		return this->module.getFunction(Function::ID(expr.index));
	}



	auto ReaderAgent::getCall(const Expr& expr) const -> const Call& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CALL, "not a call inst");

		return this->module.calls[expr.index];
	}

	auto ReaderAgent::getCallVoid(const Expr& expr) const -> const CallVoid& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CALL_VOID, "not a call void inst");

		return this->module.call_voids[expr.index];
	}




	auto ReaderAgent::getRet(const Expr& expr) const -> const Ret& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::RET, "Not a ret");

		return this->module.rets[expr.index];
	}



	auto ReaderAgent::getJump(const Expr& expr) -> Jump {
		evo::debugAssert(expr.kind() == Expr::Kind::JUMP, "Not a jump");

		return Jump(BasicBlock::ID(expr.index));
	}


	auto ReaderAgent::getBranch(const Expr& expr) const -> const Branch& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::BRANCH, "Not a branch");

		return this->module.branches[expr.index];
	}


	auto ReaderAgent::getPhi(const Expr& expr) const -> const Phi& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::PHI, "Not a phi");

		return this->module.phis[expr.index];
	}


	auto ReaderAgent::getAlloca(const Expr& expr) const -> const Alloca& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ALLOCA, "Not an alloca");

		return this->target_func->allocas[expr.index];
	}

	auto ReaderAgent::getLoad(const Expr& expr) const -> const Load& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::LOAD, "Not a load");

		return this->module.loads[expr.index];
	}

	auto ReaderAgent::getStore(const Expr& expr) const -> const Store& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::STORE, "Not an store");

		return this->module.stores[expr.index];
	}


	auto ReaderAgent::getCalcPtr(const Expr& expr) const -> const CalcPtr& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CALC_PTR, "Not a calc ptr");

		return this->module.calc_ptrs[expr.index];
	}


	auto ReaderAgent::getMemcpy(const Expr& expr) const -> const Memcpy& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::MEMCPY, "Not a memcpy");

		return this->module.memcpys[expr.index];
	}

	auto ReaderAgent::getMemset(const Expr& expr) const -> const Memset& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::MEMSET, "Not a memset");

		return this->module.memsets[expr.index];
	}



	auto ReaderAgent::getBitCast(const Expr& expr) const -> const BitCast& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::BIT_CAST, "Not a BitCast");

		return this->module.bitcasts[expr.index];
	}

	auto ReaderAgent::getTrunc(const Expr& expr) const -> const Trunc& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::TRUNC, "Not a Trunc");

		return this->module.truncs[expr.index];
	}

	auto ReaderAgent::getFTrunc(const Expr& expr) const -> const FTrunc& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FTRUNC, "Not a FTrunc");

		return this->module.ftruncs[expr.index];
	}

	auto ReaderAgent::getSExt(const Expr& expr) const -> const SExt& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SEXT, "Not an SExt");

		return this->module.sexts[expr.index];
	}

	auto ReaderAgent::getZExt(const Expr& expr) const -> const ZExt& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ZEXT, "Not a ZExt");

		return this->module.zexts[expr.index];
	}

	auto ReaderAgent::getFExt(const Expr& expr) const -> const FExt& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FEXT, "Not a FExt");

		return this->module.fexts[expr.index];
	}

	auto ReaderAgent::getIToF(const Expr& expr) const -> const IToF& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ITOF, "Not an IToF");

		return this->module.itofs[expr.index];
	}

	auto ReaderAgent::getUIToF(const Expr& expr) const -> const UIToF& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UITOF, "Not an UIToF");

		return this->module.uitofs[expr.index];
	}

	auto ReaderAgent::getFToI(const Expr& expr) const -> const FToI& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FTOI, "Not a FToI");

		return this->module.ftois[expr.index];
	}

	auto ReaderAgent::getFToUI(const Expr& expr) const -> const FToUI& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FTOUI, "Not a FToUI");

		return this->module.ftouis[expr.index];
	}




	auto ReaderAgent::getAdd(const Expr& expr) const -> const Add& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ADD, "Not an add");

		return this->module.adds[expr.index];
	}


	auto ReaderAgent::getSAddWrap(const Expr& expr) const -> const SAddWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::SADD_WRAP
				|| expr.kind() == Expr::Kind::SADD_WRAP_RESULT
				|| expr.kind() == Expr::Kind::SADD_WRAP_WRAPPED,
			"Not a signed add wrap"
		);

		return this->module.sadd_wraps[expr.index];
	}

	auto ReaderAgent::extractSAddWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SADD_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractSAddWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SADD_WRAP_WRAPPED, expr.index);
	}



	auto ReaderAgent::getUAddWrap(const Expr& expr) const -> const UAddWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::UADD_WRAP
				|| expr.kind() == Expr::Kind::UADD_WRAP_RESULT
				|| expr.kind() == Expr::Kind::UADD_WRAP_WRAPPED,
			"Not an unsigned add wrap"
		);

		return this->module.uadd_wraps[expr.index];
	}

	auto ReaderAgent::extractUAddWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::UADD_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractUAddWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::UADD_WRAP_WRAPPED, expr.index);
	}

	auto ReaderAgent::getSAddSat(const Expr& expr) const -> const SAddSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SADD_SAT, "Not an saddSat");

		return this->module.sadd_sats[expr.index];
	}

	auto ReaderAgent::getUAddSat(const Expr& expr) const -> const UAddSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UADD_SAT, "Not an uaddSat");

		return this->module.uadd_sats[expr.index];
	}

	auto ReaderAgent::getFAdd(const Expr& expr) const -> const FAdd& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FADD, "Not a fadd");

		return this->module.fadds[expr.index];
	}




	auto ReaderAgent::getSub(const Expr& expr) const -> const Sub& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SUB, "Not a sub");

		return this->module.subs[expr.index];
	}


	auto ReaderAgent::getSSubWrap(const Expr& expr) const -> const SSubWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::SSUB_WRAP
				|| expr.kind() == Expr::Kind::SSUB_WRAP_RESULT
				|| expr.kind() == Expr::Kind::SSUB_WRAP_WRAPPED,
			"Not a signed sub wrap"
		);

		return this->module.ssub_wraps[expr.index];
	}

	auto ReaderAgent::extractSSubWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SSUB_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractSSubWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SSUB_WRAP_WRAPPED, expr.index);
	}



	auto ReaderAgent::getUSubWrap(const Expr& expr) const -> const USubWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::USUB_WRAP
				|| expr.kind() == Expr::Kind::USUB_WRAP_RESULT
				|| expr.kind() == Expr::Kind::USUB_WRAP_WRAPPED,
			"Not an unsigned sub wrap"
		);

		return this->module.usub_wraps[expr.index];
	}

	auto ReaderAgent::extractUSubWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::USUB_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractUSubWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::USUB_WRAP_WRAPPED, expr.index);
	}

	auto ReaderAgent::getSSubSat(const Expr& expr) const -> const SSubSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SSUB_SAT, "Not a ssubSat");

		return this->module.ssub_sats[expr.index];
	}

	auto ReaderAgent::getUSubSat(const Expr& expr) const -> const USubSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::USUB_SAT, "Not a usubSat");

		return this->module.usub_sats[expr.index];
	}

	auto ReaderAgent::getFSub(const Expr& expr) const -> const FSub& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FSUB, "Not a fsub");

		return this->module.fsubs[expr.index];
	}




	auto ReaderAgent::getMul(const Expr& expr) const -> const Mul& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::MUL, "Not a mul");

		return this->module.muls[expr.index];
	}


	auto ReaderAgent::getSMulWrap(const Expr& expr) const -> const SMulWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::SMUL_WRAP
				|| expr.kind() == Expr::Kind::SMUL_WRAP_RESULT
				|| expr.kind() == Expr::Kind::SMUL_WRAP_WRAPPED,
			"Not a signed mul wrap"
		);

		return this->module.smul_wraps[expr.index];
	}

	auto ReaderAgent::extractSMulWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SMUL_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractSMulWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::SMUL_WRAP_WRAPPED, expr.index);
	}



	auto ReaderAgent::getUMulWrap(const Expr& expr) const -> const UMulWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::UMUL_WRAP
				|| expr.kind() == Expr::Kind::UMUL_WRAP_RESULT
				|| expr.kind() == Expr::Kind::UMUL_WRAP_WRAPPED,
			"Not an unsigned mul wrap"
		);

		return this->module.umul_wraps[expr.index];
	}

	auto ReaderAgent::extractUMulWrapResult(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::UMUL_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractUMulWrapWrapped(const Expr& expr) -> Expr {
		return Expr(Expr::Kind::UMUL_WRAP_WRAPPED, expr.index);
	}

	auto ReaderAgent::getSMulSat(const Expr& expr) const -> const SMulSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SMUL_SAT, "Not a smulSat");

		return this->module.smul_sats[expr.index];
	}

	auto ReaderAgent::getUMulSat(const Expr& expr) const -> const UMulSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UMUL_SAT, "Not a umulSat");

		return this->module.umul_sats[expr.index];
	}

	auto ReaderAgent::getFMul(const Expr& expr) const -> const FMul& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FMUL, "Not a fmul");

		return this->module.fmuls[expr.index];
	}


	auto ReaderAgent::getSDiv(const Expr& expr) const -> const SDiv& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SDIV, "Not an sdiv");

		return this->module.sdivs[expr.index];
	}

	auto ReaderAgent::getUDiv(const Expr& expr) const -> const UDiv& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UDIV, "Not an udiv");

		return this->module.udivs[expr.index];
	}

	auto ReaderAgent::getFDiv(const Expr& expr) const -> const FDiv& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FDIV, "Not a fdiv");

		return this->module.fdivs[expr.index];
	}

	auto ReaderAgent::getSRem(const Expr& expr) const -> const SRem& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SREM, "Not an srem");

		return this->module.srems[expr.index];
	}

	auto ReaderAgent::getURem(const Expr& expr) const -> const URem& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UREM, "Not an urem");

		return this->module.urems[expr.index];
	}

	auto ReaderAgent::getFRem(const Expr& expr) const -> const FRem& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FREM, "Not a frem");

		return this->module.frems[expr.index];
	}

	auto ReaderAgent::getFNeg(const Expr& expr) const -> const FNeg& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FNEG, "Not a fneg");

		return this->module.fnegs[expr.index];
	}


	auto ReaderAgent::getIEq(const Expr& expr) const -> const IEq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::IEQ, "Not an ieq");

		return this->module.ieqs[expr.index];
	}

	auto ReaderAgent::getFEq(const Expr& expr) const -> const FEq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FEQ, "Not a feq");

		return this->module.feqs[expr.index];
	}

	auto ReaderAgent::getINeq(const Expr& expr) const -> const INeq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::INEQ, "Not an ineq");

		return this->module.ineqs[expr.index];
	}

	auto ReaderAgent::getFNeq(const Expr& expr) const -> const FNeq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FNEQ, "Not a fneq");

		return this->module.fneqs[expr.index];
	}

	auto ReaderAgent::getSLT(const Expr& expr) const -> const SLT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SLT, "Not an slt");

		return this->module.slts[expr.index];
	}

	auto ReaderAgent::getULT(const Expr& expr) const -> const ULT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ULT, "Not an ult");

		return this->module.ults[expr.index];
	}

	auto ReaderAgent::getFLT(const Expr& expr) const -> const FLT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FLT, "Not a flt");

		return this->module.flts[expr.index];
	}

	auto ReaderAgent::getSLTE(const Expr& expr) const -> const SLTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SLTE, "Not an slte");

		return this->module.sltes[expr.index];
	}

	auto ReaderAgent::getULTE(const Expr& expr) const -> const ULTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ULTE, "Not an ulte");

		return this->module.ultes[expr.index];
	}

	auto ReaderAgent::getFLTE(const Expr& expr) const -> const FLTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FLTE, "Not a flte");

		return this->module.fltes[expr.index];
	}

	auto ReaderAgent::getSGT(const Expr& expr) const -> const SGT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SGT, "Not an sgt");

		return this->module.sgts[expr.index];
	}

	auto ReaderAgent::getUGT(const Expr& expr) const -> const UGT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UGT, "Not an ugt");

		return this->module.ugts[expr.index];
	}

	auto ReaderAgent::getFGT(const Expr& expr) const -> const FGT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FGT, "Not a fgt");

		return this->module.fgts[expr.index];
	}

	auto ReaderAgent::getSGTE(const Expr& expr) const -> const SGTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SGTE, "Not an sgte");

		return this->module.sgtes[expr.index];
	}

	auto ReaderAgent::getUGTE(const Expr& expr) const -> const UGTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UGTE, "Not an ugte");

		return this->module.ugtes[expr.index];
	}

	auto ReaderAgent::getFGTE(const Expr& expr) const -> const FGTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FGTE, "Not a fgte");

		return this->module.fgtes[expr.index];
	}




	auto ReaderAgent::getAnd(const Expr& expr) const -> const And& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::AND, "Not an and");

		return this->module.ands[expr.index];
	}

	auto ReaderAgent::getOr(const Expr& expr) const -> const Or& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::OR, "Not an or");

		return this->module.ors[expr.index];
	}

	auto ReaderAgent::getXor(const Expr& expr) const -> const Xor& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::XOR, "Not an xor");

		return this->module.xors[expr.index];
	}

	auto ReaderAgent::getSHL(const Expr& expr) const -> const SHL& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SHL, "Not an shl");

		return this->module.shls[expr.index];
	}

	auto ReaderAgent::getSSHLSat(const Expr& expr) const -> const SSHLSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SSHL_SAT, "Not an sshlsat");

		return this->module.sshlsats[expr.index];
	}

	auto ReaderAgent::getUSHLSat(const Expr& expr) const -> const USHLSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::USHL_SAT, "Not an ushlsat");

		return this->module.ushlsats[expr.index];
	}

	auto ReaderAgent::getSSHR(const Expr& expr) const -> const SSHR& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SSHR, "Not a sshr");

		return this->module.sshrs[expr.index];
	}

	auto ReaderAgent::getUSHR(const Expr& expr) const -> const USHR& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::USHR, "Not an ushr");

		return this->module.ushrs[expr.index];
	}


	auto ReaderAgent::getBitReverse(const Expr& expr) const -> const BitReverse& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::BIT_REVERSE, "Not a bitReverse");

		return this->module.bit_reverses[expr.index];
	}

	auto ReaderAgent::getBSwap(const Expr& expr) const -> const BSwap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::BSWAP, "Not a bSwap");

		return this->module.bswaps[expr.index];
	}

	auto ReaderAgent::getCtPop(const Expr& expr) const -> const CtPop& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CTPOP, "Not a ctPop");

		return this->module.ctpops[expr.index];
	}

	auto ReaderAgent::getCTLZ(const Expr& expr) const -> const CTLZ& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CTLZ, "Not a ctlz");

		return this->module.ctlzs[expr.index];
	}

	auto ReaderAgent::getCTTZ(const Expr& expr) const -> const CTTZ& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CTTZ, "Not a cttz");

		return this->module.cttzs[expr.index];
	}



}