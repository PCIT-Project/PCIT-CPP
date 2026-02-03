////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>


#include "./Type.h"
#include "./Expr.h"
#include "./BasicBlock.h"
#include "./Function.h"
#include "./Module.h"



namespace pcit::pir{


	// A unified way to interact with things like exprs and basic blocks

	class ReaderAgent{
		public:
			ReaderAgent(const Module& _module) : module(_module), target_func(nullptr) {}
			ReaderAgent(const Module& _module, const Function& func) : module(_module), target_func(&func) {}
			~ReaderAgent() = default;

			EVO_NODISCARD auto getModule() const -> const Module& { return this->module; }
			EVO_NODISCARD auto hasTargetFunction() const -> bool { return this->target_func != nullptr; }
			auto setTargetFunction(const Function& func) -> void { this->target_func = &func; }
			auto clearTargetFunction() -> void { this->target_func = nullptr; }
			EVO_NODISCARD auto getTargetFunction() const -> const Function& {
				evo::debugAssert(this->hasTargetFunction(), "ReaderAgent has no target function set");
				return *this->target_func;
			}


			EVO_NODISCARD auto getExprType(Expr expr) const -> Type;

			auto getBasicBlock(BasicBlock::ID id) const -> const BasicBlock&;

			EVO_NODISCARD auto getNumber(Expr expr) const -> const Number&;
			EVO_NODISCARD static auto getBoolean(Expr expr) -> bool;
			EVO_NODISCARD static auto getParamExpr(Expr expr) -> ParamExpr;
			EVO_NODISCARD static auto getGlobalValue(Expr expr) -> GlobalVar::ID;
			EVO_NODISCARD auto getFunctionPointer(Expr expr) const -> const Function&;

			EVO_NODISCARD auto getCall(Expr expr) const -> const Call&;
			EVO_NODISCARD auto getCallVoid(Expr expr) const -> const CallVoid&;
			EVO_NODISCARD auto getRet(Expr expr) const -> const Ret&;
			EVO_NODISCARD static auto getJump(Expr expr) -> Jump;
			EVO_NODISCARD auto getBranch(Expr expr) const -> const Branch&;
			EVO_NODISCARD auto getPhi(Expr expr) const -> const Phi&;
			EVO_NODISCARD auto getSwitch(Expr expr) const -> const Switch&;

			EVO_NODISCARD auto getAlloca(Expr expr) const -> const Alloca&;
			EVO_NODISCARD auto getLoad(Expr expr) const -> const Load&;
			EVO_NODISCARD auto getStore(Expr expr) const -> const Store&;
			EVO_NODISCARD auto getCalcPtr(Expr expr) const -> const CalcPtr&;
			EVO_NODISCARD auto getMemcpy(Expr expr) const -> const Memcpy&;
			EVO_NODISCARD auto getMemset(Expr expr) const -> const Memset&;

			EVO_NODISCARD auto getBitCast(Expr expr) const -> const BitCast&;
			EVO_NODISCARD auto getTrunc(Expr expr) const -> const Trunc&;
			EVO_NODISCARD auto getFTrunc(Expr expr) const -> const FTrunc&;
			EVO_NODISCARD auto getSExt(Expr expr) const -> const SExt&;
			EVO_NODISCARD auto getZExt(Expr expr) const -> const ZExt&;
			EVO_NODISCARD auto getFExt(Expr expr) const -> const FExt&;
			EVO_NODISCARD auto getIToF(Expr expr) const -> const IToF&;
			EVO_NODISCARD auto getUIToF(Expr expr) const -> const UIToF&;
			EVO_NODISCARD auto getFToI(Expr expr) const -> const FToI&;
			EVO_NODISCARD auto getFToUI(Expr expr) const -> const FToUI&;

			EVO_NODISCARD auto getAdd(Expr expr) const -> const Add&;
			EVO_NODISCARD auto getSAddWrap(Expr expr) const -> const SAddWrap&;
			EVO_NODISCARD static auto extractSAddWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractSAddWrapWrapped(Expr expr) -> Expr;
			EVO_NODISCARD auto getUAddWrap(Expr expr) const -> const UAddWrap&;
			EVO_NODISCARD static auto extractUAddWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractUAddWrapWrapped(Expr expr) -> Expr;
			EVO_NODISCARD auto getSAddSat(Expr expr) const -> const SAddSat&;
			EVO_NODISCARD auto getUAddSat(Expr expr) const -> const UAddSat&;
			EVO_NODISCARD auto getFAdd(Expr expr) const -> const FAdd&;
			EVO_NODISCARD auto getSub(Expr expr) const -> const Sub&;
			EVO_NODISCARD auto getSSubWrap(Expr expr) const -> const SSubWrap&;
			EVO_NODISCARD static auto extractSSubWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractSSubWrapWrapped(Expr expr) -> Expr;
			EVO_NODISCARD auto getUSubWrap(Expr expr) const -> const USubWrap&;
			EVO_NODISCARD static auto extractUSubWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractUSubWrapWrapped(Expr expr) -> Expr;
			EVO_NODISCARD auto getSSubSat(Expr expr) const -> const SSubSat&;
			EVO_NODISCARD auto getUSubSat(Expr expr) const -> const USubSat&;
			EVO_NODISCARD auto getFSub(Expr expr) const -> const FSub&;
			EVO_NODISCARD auto getMul(Expr expr) const -> const Mul&;
			EVO_NODISCARD auto getSMulWrap(Expr expr) const -> const SMulWrap&;
			EVO_NODISCARD static auto extractSMulWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractSMulWrapWrapped(Expr expr) -> Expr;
			EVO_NODISCARD auto getUMulWrap(Expr expr) const -> const UMulWrap&;
			EVO_NODISCARD static auto extractUMulWrapResult(Expr expr) -> Expr;
			EVO_NODISCARD static auto extractUMulWrapWrapped(Expr expr) -> Expr;
			EVO_NODISCARD auto getSMulSat(Expr expr) const -> const SMulSat&;
			EVO_NODISCARD auto getUMulSat(Expr expr) const -> const UMulSat&;
			EVO_NODISCARD auto getFMul(Expr expr) const -> const FMul&;
			EVO_NODISCARD auto getSDiv(Expr expr) const -> const SDiv&;
			EVO_NODISCARD auto getUDiv(Expr expr) const -> const UDiv&;
			EVO_NODISCARD auto getFDiv(Expr expr) const -> const FDiv&;
			EVO_NODISCARD auto getSRem(Expr expr) const -> const SRem&;
			EVO_NODISCARD auto getURem(Expr expr) const -> const URem&;
			EVO_NODISCARD auto getFRem(Expr expr) const -> const FRem&;
			EVO_NODISCARD auto getFNeg(Expr expr) const -> const FNeg&;

			EVO_NODISCARD auto getIEq(Expr expr) const -> const IEq&;
			EVO_NODISCARD auto getFEq(Expr expr) const -> const FEq&;
			EVO_NODISCARD auto getINeq(Expr expr) const -> const INeq&;
			EVO_NODISCARD auto getFNeq(Expr expr) const -> const FNeq&;
			EVO_NODISCARD auto getSLT(Expr expr) const -> const SLT&;
			EVO_NODISCARD auto getULT(Expr expr) const -> const ULT&;
			EVO_NODISCARD auto getFLT(Expr expr) const -> const FLT&;
			EVO_NODISCARD auto getSLTE(Expr expr) const -> const SLTE&;
			EVO_NODISCARD auto getULTE(Expr expr) const -> const ULTE&;
			EVO_NODISCARD auto getFLTE(Expr expr) const -> const FLTE&;
			EVO_NODISCARD auto getSGT(Expr expr) const -> const SGT&;
			EVO_NODISCARD auto getUGT(Expr expr) const -> const UGT&;
			EVO_NODISCARD auto getFGT(Expr expr) const -> const FGT&;
			EVO_NODISCARD auto getSGTE(Expr expr) const -> const SGTE&;
			EVO_NODISCARD auto getUGTE(Expr expr) const -> const UGTE&;
			EVO_NODISCARD auto getFGTE(Expr expr) const -> const FGTE&;

			EVO_NODISCARD auto getAnd(Expr expr) const -> const And&;
			EVO_NODISCARD auto getOr(Expr expr) const -> const Or&;
			EVO_NODISCARD auto getXor(Expr expr) const -> const Xor&;
			EVO_NODISCARD auto getSHL(Expr expr) const -> const SHL&;
			EVO_NODISCARD auto getSSHLSat(Expr expr) const -> const SSHLSat&;
			EVO_NODISCARD auto getUSHLSat(Expr expr) const -> const USHLSat&;
			EVO_NODISCARD auto getSSHR(Expr expr) const -> const SSHR&;
			EVO_NODISCARD auto getUSHR(Expr expr) const -> const USHR&;

			EVO_NODISCARD auto getBitReverse(Expr expr) const -> const BitReverse&;
			EVO_NODISCARD auto getByteSwap(Expr expr) const -> const ByteSwap&;
			EVO_NODISCARD auto getCtPop(Expr expr) const -> const CtPop&;
			EVO_NODISCARD auto getCTLZ(Expr expr) const -> const CTLZ&;
			EVO_NODISCARD auto getCTTZ(Expr expr) const -> const CTTZ&;

			EVO_NODISCARD auto getCmpXchg(Expr expr) const -> const CmpXchg&;
			EVO_NODISCARD static auto extractCmpXchgLoaded(Expr) -> Expr;
			EVO_NODISCARD static auto extractCmpXchgSucceeded(Expr) -> Expr;
			EVO_NODISCARD auto getAtomicRMW(Expr expr) const -> const AtomicRMW&;

			EVO_NODISCARD auto getLifetimeStart(Expr expr) const -> const LifetimeStart&;
			EVO_NODISCARD auto getLifetimeEnd(Expr expr) const -> const LifetimeEnd&;
			

		private:
			const Module& module;
			const Function* target_func;
	};


}


