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


	auto ReaderAgent::getExprType(Expr expr) const -> Type {		
		evo::debugAssert(expr.isValue(), "Expr must be a value in order to get the type");

		switch(expr.kind()){
			case Expr::Kind::NONE:             evo::debugAssert("Not a valid value");
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
			case Expr::Kind::CALL_VOID:         evo::debugFatalBreak("Not a value");
			case Expr::Kind::CALL_NO_RETURN:    evo::debugFatalBreak("Not a value");
			case Expr::Kind::ABORT:             evo::debugFatalBreak("Not a value");
			case Expr::Kind::BREAKPOINT:        evo::debugFatalBreak("Not a value");
			case Expr::Kind::RET:               evo::debugFatalBreak("Not a value");
			case Expr::Kind::JUMP:              evo::debugFatalBreak("Not a value");
			case Expr::Kind::BRANCH:            evo::debugFatalBreak("Not a value");
			case Expr::Kind::UNREACHABLE:       evo::debugFatalBreak("Not a value");
			case Expr::Kind::PHI:               return this->getExprType(this->getPhi(expr).predecessors[0].value);
			case Expr::Kind::SWITCH:            evo::debugFatalBreak("Not a value");
			case Expr::Kind::ALLOCA:            return this->module.createPtrType();
			case Expr::Kind::LOAD:              return this->getLoad(expr).type;
			case Expr::Kind::STORE:             evo::debugFatalBreak("Not a value");
			case Expr::Kind::CALC_PTR:          return this->module.createPtrType();
			case Expr::Kind::MEMCPY:            evo::debugFatalBreak("Not a value");
			case Expr::Kind::MEMSET:            evo::debugFatalBreak("Not a value");
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
			case Expr::Kind::SADD_WRAP:         evo::debugFatalBreak("Not a value");
			case Expr::Kind::SADD_WRAP_RESULT:  return this->getExprType(this->getSAddWrap(expr).lhs);
			case Expr::Kind::SADD_WRAP_WRAPPED: return this->module.createBoolType();
			case Expr::Kind::UADD_WRAP:         evo::debugFatalBreak("Not a value");
			case Expr::Kind::UADD_WRAP_RESULT:  return this->getExprType(this->getUAddWrap(expr).lhs);
			case Expr::Kind::UADD_WRAP_WRAPPED: return this->module.createBoolType();
			case Expr::Kind::SADD_SAT:          return this->getExprType(this->getSAddSat(expr).lhs);
			case Expr::Kind::UADD_SAT:          return this->getExprType(this->getUAddSat(expr).lhs);
			case Expr::Kind::FADD:              return this->getExprType(this->getFAdd(expr).lhs);
			case Expr::Kind::SUB:               return this->getExprType(this->getSub(expr).lhs);
			case Expr::Kind::SSUB_WRAP:         evo::debugFatalBreak("Not a value");
			case Expr::Kind::SSUB_WRAP_RESULT:  return this->getExprType(this->getSSubWrap(expr).lhs);
			case Expr::Kind::SSUB_WRAP_WRAPPED: return this->module.createBoolType();
			case Expr::Kind::USUB_WRAP:         evo::debugFatalBreak("Not a value");
			case Expr::Kind::USUB_WRAP_RESULT:  return this->getExprType(this->getUSubWrap(expr).lhs);
			case Expr::Kind::USUB_WRAP_WRAPPED: return this->module.createBoolType();
			case Expr::Kind::SSUB_SAT:          return this->getExprType(this->getSSubSat(expr).lhs);
			case Expr::Kind::USUB_SAT:          return this->getExprType(this->getUSubSat(expr).lhs);
			case Expr::Kind::FSUB:              return this->getExprType(this->getFSub(expr).lhs);
			case Expr::Kind::MUL:               return this->getExprType(this->getMul(expr).lhs);
			case Expr::Kind::SMUL_WRAP:         evo::debugFatalBreak("Not a value");
			case Expr::Kind::SMUL_WRAP_RESULT:  return this->getExprType(this->getSMulWrap(expr).lhs);
			case Expr::Kind::SMUL_WRAP_WRAPPED: return this->module.createBoolType();
			case Expr::Kind::UMUL_WRAP:         evo::debugFatalBreak("Not a value");
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
			case Expr::Kind::BYTE_SWAP:         return this->getExprType(this->getByteSwap(expr).arg);
			case Expr::Kind::CTPOP:             return this->getExprType(this->getCtPop(expr).arg);
			case Expr::Kind::CTLZ:              return this->getExprType(this->getCTLZ(expr).arg);
			case Expr::Kind::CTTZ:              return this->getExprType(this->getCTTZ(expr).arg);
			case Expr::Kind::CMPXCHG:           evo::debugFatalBreak("Not a value");
			case Expr::Kind::CMPXCHG_LOADED:    return this->getExprType(this->getCmpXchg(expr).expected);
			case Expr::Kind::CMPXCHG_SUCCEEDED: return this->module.createBoolType();
			case Expr::Kind::ATOMIC_RMW:        return this->getExprType(this->getAtomicRMW(expr).value);
			case Expr::Kind::LIFETIME_START:    evo::debugFatalBreak("Not a value");
			case Expr::Kind::LIFETIME_END:      evo::debugFatalBreak("Not a value");
		}

		evo::debugFatalBreak("Unknown or unsupported expr");
	}

	
	auto ReaderAgent::getBasicBlock(BasicBlock::ID id) const -> const BasicBlock& {
		return this->module.basic_blocks[id];
	}


	auto ReaderAgent::getNumber(Expr expr) const -> const Number& {
		evo::debugAssert(expr.kind() == Expr::Kind::NUMBER, "Not a number");
		return this->module.numbers[expr.index];
	}

	auto ReaderAgent::getBoolean(Expr expr) -> bool {
		evo::debugAssert(expr.kind() == Expr::Kind::BOOLEAN, "Not a Boolean");
		return bool(expr.index);
	}


	auto ReaderAgent::getParamExpr(Expr expr) -> ParamExpr {
		evo::debugAssert(expr.kind() == Expr::Kind::PARAM_EXPR, "not a param expr");
		return ParamExpr(expr.index);
	}

	auto ReaderAgent::getGlobalValue(Expr expr) -> GlobalVar::ID {
		evo::debugAssert(expr.kind() == Expr::Kind::GLOBAL_VALUE, "Not a global");
		return GlobalVar::ID(expr.index);
	}

	auto ReaderAgent::getFunctionPointer(Expr expr) const -> const Function& {
		evo::debugAssert(expr.kind() == Expr::Kind::FUNCTION_POINTER, "Not a function pointer");
		return this->module.getFunction(Function::ID(expr.index));
	}



	auto ReaderAgent::getCall(Expr expr) const -> const Call& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CALL, "not a call inst");

		return this->module.calls[expr.index];
	}

	auto ReaderAgent::getCallVoid(Expr expr) const -> const CallVoid& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CALL_VOID, "not a call void inst");

		return this->module.call_voids[expr.index];
	}

	auto ReaderAgent::getCallNoReturn(Expr expr) const -> const CallNoReturn& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CALL_NO_RETURN, "not a call no return inst");

		return this->module.call_no_returns[expr.index];
	}




	auto ReaderAgent::getRet(Expr expr) const -> const Ret& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::RET, "Not a ret");

		return this->module.rets[expr.index];
	}



	auto ReaderAgent::getJump(Expr expr) -> Jump {
		evo::debugAssert(expr.kind() == Expr::Kind::JUMP, "Not a jump");

		return Jump(BasicBlock::ID(expr.index));
	}


	auto ReaderAgent::getBranch(Expr expr) const -> const Branch& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::BRANCH, "Not a branch");

		return this->module.branches[expr.index];
	}


	auto ReaderAgent::getPhi(Expr expr) const -> const Phi& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::PHI, "Not a phi");

		return this->module.phis[expr.index];
	}


	auto ReaderAgent::getSwitch(Expr expr) const -> const Switch& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SWITCH, "Not a switch");

		return this->module.switches[expr.index];
	}


	auto ReaderAgent::getAlloca(Expr expr) const -> const Alloca& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ALLOCA, "Not an alloca");

		return this->target_func->allocas[expr.index];
	}

	auto ReaderAgent::getLoad(Expr expr) const -> const Load& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::LOAD, "Not a load");

		return this->module.loads[expr.index];
	}

	auto ReaderAgent::getStore(Expr expr) const -> const Store& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::STORE, "Not an store");

		return this->module.stores[expr.index];
	}


	auto ReaderAgent::getCalcPtr(Expr expr) const -> const CalcPtr& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CALC_PTR, "Not a calc ptr");

		return this->module.calc_ptrs[expr.index];
	}


	auto ReaderAgent::getMemcpy(Expr expr) const -> const Memcpy& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::MEMCPY, "Not a memcpy");

		return this->module.memcpys[expr.index];
	}

	auto ReaderAgent::getMemset(Expr expr) const -> const Memset& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::MEMSET, "Not a memset");

		return this->module.memsets[expr.index];
	}



	auto ReaderAgent::getBitCast(Expr expr) const -> const BitCast& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::BIT_CAST, "Not a BitCast");

		return this->module.bitcasts[expr.index];
	}

	auto ReaderAgent::getTrunc(Expr expr) const -> const Trunc& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::TRUNC, "Not a Trunc");

		return this->module.truncs[expr.index];
	}

	auto ReaderAgent::getFTrunc(Expr expr) const -> const FTrunc& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FTRUNC, "Not a FTrunc");

		return this->module.ftruncs[expr.index];
	}

	auto ReaderAgent::getSExt(Expr expr) const -> const SExt& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SEXT, "Not an SExt");

		return this->module.sexts[expr.index];
	}

	auto ReaderAgent::getZExt(Expr expr) const -> const ZExt& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ZEXT, "Not a ZExt");

		return this->module.zexts[expr.index];
	}

	auto ReaderAgent::getFExt(Expr expr) const -> const FExt& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FEXT, "Not a FExt");

		return this->module.fexts[expr.index];
	}

	auto ReaderAgent::getIToF(Expr expr) const -> const IToF& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ITOF, "Not an IToF");

		return this->module.itofs[expr.index];
	}

	auto ReaderAgent::getUIToF(Expr expr) const -> const UIToF& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UITOF, "Not an UIToF");

		return this->module.uitofs[expr.index];
	}

	auto ReaderAgent::getFToI(Expr expr) const -> const FToI& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FTOI, "Not a FToI");

		return this->module.ftois[expr.index];
	}

	auto ReaderAgent::getFToUI(Expr expr) const -> const FToUI& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FTOUI, "Not a FToUI");

		return this->module.ftouis[expr.index];
	}




	auto ReaderAgent::getAdd(Expr expr) const -> const Add& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ADD, "Not an add");

		return this->module.adds[expr.index];
	}


	auto ReaderAgent::getSAddWrap(Expr expr) const -> const SAddWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::SADD_WRAP
				|| expr.kind() == Expr::Kind::SADD_WRAP_RESULT
				|| expr.kind() == Expr::Kind::SADD_WRAP_WRAPPED,
			"Not a signed add wrap"
		);

		return this->module.sadd_wraps[expr.index];
	}

	auto ReaderAgent::extractSAddWrapResult(Expr expr) -> Expr {
		return Expr(Expr::Kind::SADD_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractSAddWrapWrapped(Expr expr) -> Expr {
		return Expr(Expr::Kind::SADD_WRAP_WRAPPED, expr.index);
	}



	auto ReaderAgent::getUAddWrap(Expr expr) const -> const UAddWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::UADD_WRAP
				|| expr.kind() == Expr::Kind::UADD_WRAP_RESULT
				|| expr.kind() == Expr::Kind::UADD_WRAP_WRAPPED,
			"Not an unsigned add wrap"
		);

		return this->module.uadd_wraps[expr.index];
	}

	auto ReaderAgent::extractUAddWrapResult(Expr expr) -> Expr {
		return Expr(Expr::Kind::UADD_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractUAddWrapWrapped(Expr expr) -> Expr {
		return Expr(Expr::Kind::UADD_WRAP_WRAPPED, expr.index);
	}

	auto ReaderAgent::getSAddSat(Expr expr) const -> const SAddSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SADD_SAT, "Not an saddSat");

		return this->module.sadd_sats[expr.index];
	}

	auto ReaderAgent::getUAddSat(Expr expr) const -> const UAddSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UADD_SAT, "Not an uaddSat");

		return this->module.uadd_sats[expr.index];
	}

	auto ReaderAgent::getFAdd(Expr expr) const -> const FAdd& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FADD, "Not a fadd");

		return this->module.fadds[expr.index];
	}




	auto ReaderAgent::getSub(Expr expr) const -> const Sub& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SUB, "Not a sub");

		return this->module.subs[expr.index];
	}


	auto ReaderAgent::getSSubWrap(Expr expr) const -> const SSubWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::SSUB_WRAP
				|| expr.kind() == Expr::Kind::SSUB_WRAP_RESULT
				|| expr.kind() == Expr::Kind::SSUB_WRAP_WRAPPED,
			"Not a signed sub wrap"
		);

		return this->module.ssub_wraps[expr.index];
	}

	auto ReaderAgent::extractSSubWrapResult(Expr expr) -> Expr {
		return Expr(Expr::Kind::SSUB_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractSSubWrapWrapped(Expr expr) -> Expr {
		return Expr(Expr::Kind::SSUB_WRAP_WRAPPED, expr.index);
	}



	auto ReaderAgent::getUSubWrap(Expr expr) const -> const USubWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::USUB_WRAP
				|| expr.kind() == Expr::Kind::USUB_WRAP_RESULT
				|| expr.kind() == Expr::Kind::USUB_WRAP_WRAPPED,
			"Not an unsigned sub wrap"
		);

		return this->module.usub_wraps[expr.index];
	}

	auto ReaderAgent::extractUSubWrapResult(Expr expr) -> Expr {
		return Expr(Expr::Kind::USUB_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractUSubWrapWrapped(Expr expr) -> Expr {
		return Expr(Expr::Kind::USUB_WRAP_WRAPPED, expr.index);
	}

	auto ReaderAgent::getSSubSat(Expr expr) const -> const SSubSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SSUB_SAT, "Not a ssubSat");

		return this->module.ssub_sats[expr.index];
	}

	auto ReaderAgent::getUSubSat(Expr expr) const -> const USubSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::USUB_SAT, "Not a usubSat");

		return this->module.usub_sats[expr.index];
	}

	auto ReaderAgent::getFSub(Expr expr) const -> const FSub& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FSUB, "Not a fsub");

		return this->module.fsubs[expr.index];
	}




	auto ReaderAgent::getMul(Expr expr) const -> const Mul& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::MUL, "Not a mul");

		return this->module.muls[expr.index];
	}


	auto ReaderAgent::getSMulWrap(Expr expr) const -> const SMulWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::SMUL_WRAP
				|| expr.kind() == Expr::Kind::SMUL_WRAP_RESULT
				|| expr.kind() == Expr::Kind::SMUL_WRAP_WRAPPED,
			"Not a signed mul wrap"
		);

		return this->module.smul_wraps[expr.index];
	}

	auto ReaderAgent::extractSMulWrapResult(Expr expr) -> Expr {
		return Expr(Expr::Kind::SMUL_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractSMulWrapWrapped(Expr expr) -> Expr {
		return Expr(Expr::Kind::SMUL_WRAP_WRAPPED, expr.index);
	}



	auto ReaderAgent::getUMulWrap(Expr expr) const -> const UMulWrap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::UMUL_WRAP
				|| expr.kind() == Expr::Kind::UMUL_WRAP_RESULT
				|| expr.kind() == Expr::Kind::UMUL_WRAP_WRAPPED,
			"Not an unsigned mul wrap"
		);

		return this->module.umul_wraps[expr.index];
	}

	auto ReaderAgent::extractUMulWrapResult(Expr expr) -> Expr {
		return Expr(Expr::Kind::UMUL_WRAP_RESULT, expr.index);
	}

	auto ReaderAgent::extractUMulWrapWrapped(Expr expr) -> Expr {
		return Expr(Expr::Kind::UMUL_WRAP_WRAPPED, expr.index);
	}

	auto ReaderAgent::getSMulSat(Expr expr) const -> const SMulSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SMUL_SAT, "Not a smulSat");

		return this->module.smul_sats[expr.index];
	}

	auto ReaderAgent::getUMulSat(Expr expr) const -> const UMulSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UMUL_SAT, "Not a umulSat");

		return this->module.umul_sats[expr.index];
	}

	auto ReaderAgent::getFMul(Expr expr) const -> const FMul& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FMUL, "Not a fmul");

		return this->module.fmuls[expr.index];
	}


	auto ReaderAgent::getSDiv(Expr expr) const -> const SDiv& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SDIV, "Not an sdiv");

		return this->module.sdivs[expr.index];
	}

	auto ReaderAgent::getUDiv(Expr expr) const -> const UDiv& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UDIV, "Not an udiv");

		return this->module.udivs[expr.index];
	}

	auto ReaderAgent::getFDiv(Expr expr) const -> const FDiv& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FDIV, "Not a fdiv");

		return this->module.fdivs[expr.index];
	}

	auto ReaderAgent::getSRem(Expr expr) const -> const SRem& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SREM, "Not an srem");

		return this->module.srems[expr.index];
	}

	auto ReaderAgent::getURem(Expr expr) const -> const URem& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UREM, "Not an urem");

		return this->module.urems[expr.index];
	}

	auto ReaderAgent::getFRem(Expr expr) const -> const FRem& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FREM, "Not a frem");

		return this->module.frems[expr.index];
	}

	auto ReaderAgent::getFNeg(Expr expr) const -> const FNeg& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FNEG, "Not a fneg");

		return this->module.fnegs[expr.index];
	}


	auto ReaderAgent::getIEq(Expr expr) const -> const IEq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::IEQ, "Not an ieq");

		return this->module.ieqs[expr.index];
	}

	auto ReaderAgent::getFEq(Expr expr) const -> const FEq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FEQ, "Not a feq");

		return this->module.feqs[expr.index];
	}

	auto ReaderAgent::getINeq(Expr expr) const -> const INeq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::INEQ, "Not an ineq");

		return this->module.ineqs[expr.index];
	}

	auto ReaderAgent::getFNeq(Expr expr) const -> const FNeq& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FNEQ, "Not a fneq");

		return this->module.fneqs[expr.index];
	}

	auto ReaderAgent::getSLT(Expr expr) const -> const SLT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SLT, "Not an slt");

		return this->module.slts[expr.index];
	}

	auto ReaderAgent::getULT(Expr expr) const -> const ULT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ULT, "Not an ult");

		return this->module.ults[expr.index];
	}

	auto ReaderAgent::getFLT(Expr expr) const -> const FLT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FLT, "Not a flt");

		return this->module.flts[expr.index];
	}

	auto ReaderAgent::getSLTE(Expr expr) const -> const SLTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SLTE, "Not an slte");

		return this->module.sltes[expr.index];
	}

	auto ReaderAgent::getULTE(Expr expr) const -> const ULTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ULTE, "Not an ulte");

		return this->module.ultes[expr.index];
	}

	auto ReaderAgent::getFLTE(Expr expr) const -> const FLTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FLTE, "Not a flte");

		return this->module.fltes[expr.index];
	}

	auto ReaderAgent::getSGT(Expr expr) const -> const SGT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SGT, "Not an sgt");

		return this->module.sgts[expr.index];
	}

	auto ReaderAgent::getUGT(Expr expr) const -> const UGT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UGT, "Not an ugt");

		return this->module.ugts[expr.index];
	}

	auto ReaderAgent::getFGT(Expr expr) const -> const FGT& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FGT, "Not a fgt");

		return this->module.fgts[expr.index];
	}

	auto ReaderAgent::getSGTE(Expr expr) const -> const SGTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SGTE, "Not an sgte");

		return this->module.sgtes[expr.index];
	}

	auto ReaderAgent::getUGTE(Expr expr) const -> const UGTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::UGTE, "Not an ugte");

		return this->module.ugtes[expr.index];
	}

	auto ReaderAgent::getFGTE(Expr expr) const -> const FGTE& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::FGTE, "Not a fgte");

		return this->module.fgtes[expr.index];
	}




	auto ReaderAgent::getAnd(Expr expr) const -> const And& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::AND, "Not an and");

		return this->module.ands[expr.index];
	}

	auto ReaderAgent::getOr(Expr expr) const -> const Or& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::OR, "Not an or");

		return this->module.ors[expr.index];
	}

	auto ReaderAgent::getXor(Expr expr) const -> const Xor& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::XOR, "Not an xor");

		return this->module.xors[expr.index];
	}

	auto ReaderAgent::getSHL(Expr expr) const -> const SHL& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SHL, "Not an shl");

		return this->module.shls[expr.index];
	}

	auto ReaderAgent::getSSHLSat(Expr expr) const -> const SSHLSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SSHL_SAT, "Not an sshlsat");

		return this->module.sshlsats[expr.index];
	}

	auto ReaderAgent::getUSHLSat(Expr expr) const -> const USHLSat& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::USHL_SAT, "Not an ushlsat");

		return this->module.ushlsats[expr.index];
	}

	auto ReaderAgent::getSSHR(Expr expr) const -> const SSHR& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::SSHR, "Not a sshr");

		return this->module.sshrs[expr.index];
	}

	auto ReaderAgent::getUSHR(Expr expr) const -> const USHR& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::USHR, "Not an ushr");

		return this->module.ushrs[expr.index];
	}


	auto ReaderAgent::getBitReverse(Expr expr) const -> const BitReverse& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::BIT_REVERSE, "Not a bitReverse");

		return this->module.bit_reverses[expr.index];
	}

	auto ReaderAgent::getByteSwap(Expr expr) const -> const ByteSwap& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::BYTE_SWAP, "Not a byteSwap");

		return this->module.byte_swaps[expr.index];
	}

	auto ReaderAgent::getCtPop(Expr expr) const -> const CtPop& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CTPOP, "Not a ctPop");

		return this->module.ctpops[expr.index];
	}

	auto ReaderAgent::getCTLZ(Expr expr) const -> const CTLZ& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CTLZ, "Not a ctlz");

		return this->module.ctlzs[expr.index];
	}

	auto ReaderAgent::getCTTZ(Expr expr) const -> const CTTZ& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::CTTZ, "Not a cttz");

		return this->module.cttzs[expr.index];
	}

	auto ReaderAgent::getCmpXchg(Expr expr) const -> const CmpXchg& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(
			expr.kind() == Expr::Kind::CMPXCHG
				|| expr.kind() == Expr::Kind::CMPXCHG_LOADED
				|| expr.kind() == Expr::Kind::CMPXCHG_SUCCEEDED,
			"Not a cmpxchg"
		);

		return this->module.cmpxchgs[expr.index];
	}

	auto ReaderAgent::extractCmpXchgLoaded(Expr expr) -> Expr {
		return Expr(Expr::Kind::CMPXCHG_LOADED, expr.index);
	}

	auto ReaderAgent::extractCmpXchgSucceeded(Expr expr) -> Expr {
		return Expr(Expr::Kind::CMPXCHG_SUCCEEDED, expr.index);
	}

	auto ReaderAgent::getAtomicRMW(Expr expr) const -> const AtomicRMW& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::ATOMIC_RMW, "Not an atomic rmw");

		return this->module.atomic_rmws[expr.index];
	}


	auto ReaderAgent::getLifetimeStart(Expr expr) const -> const LifetimeStart& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::LIFETIME_START, "Not a lifetime start");

		return this->module.lifetime_starts[expr.index];
	}

	auto ReaderAgent::getLifetimeEnd(Expr expr) const -> const LifetimeEnd& {
		evo::debugAssert(this->hasTargetFunction(), "No target function set");
		evo::debugAssert(expr.kind() == Expr::Kind::LIFETIME_END, "Not a lifetime end");

		return this->module.lifetime_ends[expr.index];
	}


}