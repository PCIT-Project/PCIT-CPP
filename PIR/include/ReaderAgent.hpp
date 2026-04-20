////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>
#include <PCIT_core.hpp>


#include "./Type.hpp"
#include "./Expr.hpp"
#include "./BasicBlock.hpp"
#include "./Function.hpp"
#include "./Module.hpp"



namespace pcit::pir{


	// A unified way to interact with things like exprs and basic blocks

	class ReaderAgent{
		public:
			ReaderAgent(const Module& _module) : module(_module), target_func(nullptr) {}
			ReaderAgent(const Module& _module, const Function& func) : module(_module), target_func(&func) {}
			~ReaderAgent() = default;

			[[nodiscard]] auto getModule() const -> const Module& { return this->module; }
			[[nodiscard]] auto hasTargetFunction() const -> bool { return this->target_func != nullptr; }
			auto setTargetFunction(const Function& func) -> void { this->target_func = &func; }
			auto clearTargetFunction() -> void { this->target_func = nullptr; }
			[[nodiscard]] auto getTargetFunction() const -> const Function& {
				evo::debugAssert(this->hasTargetFunction(), "ReaderAgent has no target function set");
				return *this->target_func;
			}


			[[nodiscard]] auto getExprType(Expr expr) const -> Type;

			auto getBasicBlock(BasicBlock::ID id) const -> const BasicBlock&;

			[[nodiscard]] auto getNumber(Expr expr) const -> const Number&;
			[[nodiscard]] static auto getBoolean(Expr expr) -> bool;
			[[nodiscard]] static auto getParamExpr(Expr expr) -> ParamExpr;
			[[nodiscard]] static auto getGlobalValue(Expr expr) -> GlobalVar::ID;
			[[nodiscard]] static auto getFunctionPointer(Expr expr) -> Function::ID;

			[[nodiscard]] auto getCall(Expr expr) const -> const Call&;
			[[nodiscard]] auto getCallVoid(Expr expr) const -> const CallVoid&;
			[[nodiscard]] auto getCallNoReturn(Expr expr) const -> const CallNoReturn&;
			[[nodiscard]] auto getAbort(Expr expr) const -> const Abort&;
			[[nodiscard]] auto getBreakpoint(Expr expr) const -> const Breakpoint&;
			[[nodiscard]] auto getRet(Expr expr) const -> const Ret&;
			[[nodiscard]] auto getJump(Expr expr) const -> const Jump&;
			[[nodiscard]] auto getBranch(Expr expr) const -> const Branch&;
			[[nodiscard]] auto getPhi(Expr expr) const -> const Phi&;
			[[nodiscard]] auto getSwitch(Expr expr) const -> const Switch&;

			[[nodiscard]] auto getAlloca(Expr expr) const -> const Alloca&;
			[[nodiscard]] auto getLoad(Expr expr) const -> const Load&;
			[[nodiscard]] auto getStore(Expr expr) const -> const Store&;
			[[nodiscard]] auto getCalcPtr(Expr expr) const -> const CalcPtr&;
			[[nodiscard]] auto getMemcpy(Expr expr) const -> const Memcpy&;
			[[nodiscard]] auto getMemset(Expr expr) const -> const Memset&;

			[[nodiscard]] auto getBitCast(Expr expr) const -> const BitCast&;
			[[nodiscard]] auto getTrunc(Expr expr) const -> const Trunc&;
			[[nodiscard]] auto getFTrunc(Expr expr) const -> const FTrunc&;
			[[nodiscard]] auto getSExt(Expr expr) const -> const SExt&;
			[[nodiscard]] auto getZExt(Expr expr) const -> const ZExt&;
			[[nodiscard]] auto getFExt(Expr expr) const -> const FExt&;
			[[nodiscard]] auto getIToF(Expr expr) const -> const IToF&;
			[[nodiscard]] auto getUIToF(Expr expr) const -> const UIToF&;
			[[nodiscard]] auto getFToI(Expr expr) const -> const FToI&;
			[[nodiscard]] auto getFToUI(Expr expr) const -> const FToUI&;

			[[nodiscard]] auto getAdd(Expr expr) const -> const Add&;
			[[nodiscard]] auto getSAddWrap(Expr expr) const -> const SAddWrap&;
			[[nodiscard]] static auto extractSAddWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractSAddWrapWrapped(Expr expr) -> Expr;
			[[nodiscard]] auto getUAddWrap(Expr expr) const -> const UAddWrap&;
			[[nodiscard]] static auto extractUAddWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractUAddWrapWrapped(Expr expr) -> Expr;
			[[nodiscard]] auto getSAddSat(Expr expr) const -> const SAddSat&;
			[[nodiscard]] auto getUAddSat(Expr expr) const -> const UAddSat&;
			[[nodiscard]] auto getFAdd(Expr expr) const -> const FAdd&;
			[[nodiscard]] auto getSub(Expr expr) const -> const Sub&;
			[[nodiscard]] auto getSSubWrap(Expr expr) const -> const SSubWrap&;
			[[nodiscard]] static auto extractSSubWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractSSubWrapWrapped(Expr expr) -> Expr;
			[[nodiscard]] auto getUSubWrap(Expr expr) const -> const USubWrap&;
			[[nodiscard]] static auto extractUSubWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractUSubWrapWrapped(Expr expr) -> Expr;
			[[nodiscard]] auto getSSubSat(Expr expr) const -> const SSubSat&;
			[[nodiscard]] auto getUSubSat(Expr expr) const -> const USubSat&;
			[[nodiscard]] auto getFSub(Expr expr) const -> const FSub&;
			[[nodiscard]] auto getMul(Expr expr) const -> const Mul&;
			[[nodiscard]] auto getSMulWrap(Expr expr) const -> const SMulWrap&;
			[[nodiscard]] static auto extractSMulWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractSMulWrapWrapped(Expr expr) -> Expr;
			[[nodiscard]] auto getUMulWrap(Expr expr) const -> const UMulWrap&;
			[[nodiscard]] static auto extractUMulWrapResult(Expr expr) -> Expr;
			[[nodiscard]] static auto extractUMulWrapWrapped(Expr expr) -> Expr;
			[[nodiscard]] auto getSMulSat(Expr expr) const -> const SMulSat&;
			[[nodiscard]] auto getUMulSat(Expr expr) const -> const UMulSat&;
			[[nodiscard]] auto getFMul(Expr expr) const -> const FMul&;
			[[nodiscard]] auto getSDiv(Expr expr) const -> const SDiv&;
			[[nodiscard]] auto getUDiv(Expr expr) const -> const UDiv&;
			[[nodiscard]] auto getFDiv(Expr expr) const -> const FDiv&;
			[[nodiscard]] auto getSRem(Expr expr) const -> const SRem&;
			[[nodiscard]] auto getURem(Expr expr) const -> const URem&;
			[[nodiscard]] auto getFRem(Expr expr) const -> const FRem&;
			[[nodiscard]] auto getFNeg(Expr expr) const -> const FNeg&;

			[[nodiscard]] auto getIEq(Expr expr) const -> const IEq&;
			[[nodiscard]] auto getFEq(Expr expr) const -> const FEq&;
			[[nodiscard]] auto getINeq(Expr expr) const -> const INeq&;
			[[nodiscard]] auto getFNeq(Expr expr) const -> const FNeq&;
			[[nodiscard]] auto getSLT(Expr expr) const -> const SLT&;
			[[nodiscard]] auto getULT(Expr expr) const -> const ULT&;
			[[nodiscard]] auto getFLT(Expr expr) const -> const FLT&;
			[[nodiscard]] auto getSLTE(Expr expr) const -> const SLTE&;
			[[nodiscard]] auto getULTE(Expr expr) const -> const ULTE&;
			[[nodiscard]] auto getFLTE(Expr expr) const -> const FLTE&;
			[[nodiscard]] auto getSGT(Expr expr) const -> const SGT&;
			[[nodiscard]] auto getUGT(Expr expr) const -> const UGT&;
			[[nodiscard]] auto getFGT(Expr expr) const -> const FGT&;
			[[nodiscard]] auto getSGTE(Expr expr) const -> const SGTE&;
			[[nodiscard]] auto getUGTE(Expr expr) const -> const UGTE&;
			[[nodiscard]] auto getFGTE(Expr expr) const -> const FGTE&;

			[[nodiscard]] auto getAnd(Expr expr) const -> const And&;
			[[nodiscard]] auto getOr(Expr expr) const -> const Or&;
			[[nodiscard]] auto getXor(Expr expr) const -> const Xor&;
			[[nodiscard]] auto getSHL(Expr expr) const -> const SHL&;
			[[nodiscard]] auto getSSHLSat(Expr expr) const -> const SSHLSat&;
			[[nodiscard]] auto getUSHLSat(Expr expr) const -> const USHLSat&;
			[[nodiscard]] auto getSSHR(Expr expr) const -> const SSHR&;
			[[nodiscard]] auto getUSHR(Expr expr) const -> const USHR&;

			[[nodiscard]] auto getBitReverse(Expr expr) const -> const BitReverse&;
			[[nodiscard]] auto getByteSwap(Expr expr) const -> const ByteSwap&;
			[[nodiscard]] auto getCtPop(Expr expr) const -> const CtPop&;
			[[nodiscard]] auto getCTLZ(Expr expr) const -> const CTLZ&;
			[[nodiscard]] auto getCTTZ(Expr expr) const -> const CTTZ&;

			[[nodiscard]] auto getCmpXchg(Expr expr) const -> const CmpXchg&;
			[[nodiscard]] static auto extractCmpXchgLoaded(Expr) -> Expr;
			[[nodiscard]] static auto extractCmpXchgSucceeded(Expr) -> Expr;
			[[nodiscard]] auto getAtomicRMW(Expr expr) const -> const AtomicRMW&;

			[[nodiscard]] auto getMetaLocalVar(Expr expr) const -> const MetaLocalVar&;


		private:
			const Module& module;
			const Function* target_func;
	};


}


