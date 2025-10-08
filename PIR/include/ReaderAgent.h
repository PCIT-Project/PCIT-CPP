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


			EVO_NODISCARD auto getExprType(const Expr& expr) const -> Type;

			auto getBasicBlock(BasicBlock::ID id) const -> const BasicBlock&;

			EVO_NODISCARD auto getNumber(const Expr& expr) const -> const Number&;
			EVO_NODISCARD static auto getBoolean(const Expr& expr) -> bool;
			EVO_NODISCARD static auto getParamExpr(const Expr& expr) -> ParamExpr;
			EVO_NODISCARD auto getGlobalValue(const Expr& expr) const -> const GlobalVar&;
			EVO_NODISCARD auto getFunctionPointer(const Expr& expr) const -> const Function&;

			EVO_NODISCARD auto getCall(const Expr& expr) const -> const Call&;
			EVO_NODISCARD auto getCallVoid(const Expr& expr) const -> const CallVoid&;
			EVO_NODISCARD auto getRet(const Expr& expr) const -> const Ret&;
			EVO_NODISCARD static auto getJump(const Expr& expr) -> Jump;
			EVO_NODISCARD auto getBranch(const Expr& expr) const -> const Branch&;
			EVO_NODISCARD auto getPhi(const Expr& expr) const -> const Phi&;
			EVO_NODISCARD auto getSwitch(const Expr& expr) const -> const Switch&;

			EVO_NODISCARD auto getAlloca(const Expr& expr) const -> const Alloca&;
			EVO_NODISCARD auto getLoad(const Expr& expr) const -> const Load&;
			EVO_NODISCARD auto getStore(const Expr& expr) const -> const Store&;
			EVO_NODISCARD auto getCalcPtr(const Expr& expr) const -> const CalcPtr&;
			EVO_NODISCARD auto getMemcpy(const Expr& expr) const -> const Memcpy&;
			EVO_NODISCARD auto getMemset(const Expr& expr) const -> const Memset&;

			EVO_NODISCARD auto getBitCast(const Expr& expr) const -> const BitCast&;
			EVO_NODISCARD auto getTrunc(const Expr& expr) const -> const Trunc&;
			EVO_NODISCARD auto getFTrunc(const Expr& expr) const -> const FTrunc&;
			EVO_NODISCARD auto getSExt(const Expr& expr) const -> const SExt&;
			EVO_NODISCARD auto getZExt(const Expr& expr) const -> const ZExt&;
			EVO_NODISCARD auto getFExt(const Expr& expr) const -> const FExt&;
			EVO_NODISCARD auto getIToF(const Expr& expr) const -> const IToF&;
			EVO_NODISCARD auto getUIToF(const Expr& expr) const -> const UIToF&;
			EVO_NODISCARD auto getFToI(const Expr& expr) const -> const FToI&;
			EVO_NODISCARD auto getFToUI(const Expr& expr) const -> const FToUI&;

			EVO_NODISCARD auto getAdd(const Expr& expr) const -> const Add&;
			EVO_NODISCARD auto getSAddWrap(const Expr& expr) const -> const SAddWrap&;
			EVO_NODISCARD static auto extractSAddWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractSAddWrapWrapped(const Expr& expr) -> Expr;
			EVO_NODISCARD auto getUAddWrap(const Expr& expr) const -> const UAddWrap&;
			EVO_NODISCARD static auto extractUAddWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractUAddWrapWrapped(const Expr& expr) -> Expr;
			EVO_NODISCARD auto getSAddSat(const Expr& expr) const -> const SAddSat&;
			EVO_NODISCARD auto getUAddSat(const Expr& expr) const -> const UAddSat&;
			EVO_NODISCARD auto getFAdd(const Expr& expr) const -> const FAdd&;
			EVO_NODISCARD auto getSub(const Expr& expr) const -> const Sub&;
			EVO_NODISCARD auto getSSubWrap(const Expr& expr) const -> const SSubWrap&;
			EVO_NODISCARD static auto extractSSubWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractSSubWrapWrapped(const Expr& expr) -> Expr;
			EVO_NODISCARD auto getUSubWrap(const Expr& expr) const -> const USubWrap&;
			EVO_NODISCARD static auto extractUSubWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractUSubWrapWrapped(const Expr& expr) -> Expr;
			EVO_NODISCARD auto getSSubSat(const Expr& expr) const -> const SSubSat&;
			EVO_NODISCARD auto getUSubSat(const Expr& expr) const -> const USubSat&;
			EVO_NODISCARD auto getFSub(const Expr& expr) const -> const FSub&;
			EVO_NODISCARD auto getMul(const Expr& expr) const -> const Mul&;
			EVO_NODISCARD auto getSMulWrap(const Expr& expr) const -> const SMulWrap&;
			EVO_NODISCARD static auto extractSMulWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractSMulWrapWrapped(const Expr& expr) -> Expr;
			EVO_NODISCARD auto getUMulWrap(const Expr& expr) const -> const UMulWrap&;
			EVO_NODISCARD static auto extractUMulWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractUMulWrapWrapped(const Expr& expr) -> Expr;
			EVO_NODISCARD auto getSMulSat(const Expr& expr) const -> const SMulSat&;
			EVO_NODISCARD auto getUMulSat(const Expr& expr) const -> const UMulSat&;
			EVO_NODISCARD auto getFMul(const Expr& expr) const -> const FMul&;
			EVO_NODISCARD auto getSDiv(const Expr& expr) const -> const SDiv&;
			EVO_NODISCARD auto getUDiv(const Expr& expr) const -> const UDiv&;
			EVO_NODISCARD auto getFDiv(const Expr& expr) const -> const FDiv&;
			EVO_NODISCARD auto getSRem(const Expr& expr) const -> const SRem&;
			EVO_NODISCARD auto getURem(const Expr& expr) const -> const URem&;
			EVO_NODISCARD auto getFRem(const Expr& expr) const -> const FRem&;
			EVO_NODISCARD auto getFNeg(const Expr& expr) const -> const FNeg&;

			EVO_NODISCARD auto getIEq(const Expr& expr) const -> const IEq&;
			EVO_NODISCARD auto getFEq(const Expr& expr) const -> const FEq&;
			EVO_NODISCARD auto getINeq(const Expr& expr) const -> const INeq&;
			EVO_NODISCARD auto getFNeq(const Expr& expr) const -> const FNeq&;
			EVO_NODISCARD auto getSLT(const Expr& expr) const -> const SLT&;
			EVO_NODISCARD auto getULT(const Expr& expr) const -> const ULT&;
			EVO_NODISCARD auto getFLT(const Expr& expr) const -> const FLT&;
			EVO_NODISCARD auto getSLTE(const Expr& expr) const -> const SLTE&;
			EVO_NODISCARD auto getULTE(const Expr& expr) const -> const ULTE&;
			EVO_NODISCARD auto getFLTE(const Expr& expr) const -> const FLTE&;
			EVO_NODISCARD auto getSGT(const Expr& expr) const -> const SGT&;
			EVO_NODISCARD auto getUGT(const Expr& expr) const -> const UGT&;
			EVO_NODISCARD auto getFGT(const Expr& expr) const -> const FGT&;
			EVO_NODISCARD auto getSGTE(const Expr& expr) const -> const SGTE&;
			EVO_NODISCARD auto getUGTE(const Expr& expr) const -> const UGTE&;
			EVO_NODISCARD auto getFGTE(const Expr& expr) const -> const FGTE&;

			EVO_NODISCARD auto getAnd(const Expr& expr) const -> const And&;
			EVO_NODISCARD auto getOr(const Expr& expr) const -> const Or&;
			EVO_NODISCARD auto getXor(const Expr& expr) const -> const Xor&;
			EVO_NODISCARD auto getSHL(const Expr& expr) const -> const SHL&;
			EVO_NODISCARD auto getSSHLSat(const Expr& expr) const -> const SSHLSat&;
			EVO_NODISCARD auto getUSHLSat(const Expr& expr) const -> const USHLSat&;
			EVO_NODISCARD auto getSSHR(const Expr& expr) const -> const SSHR&;
			EVO_NODISCARD auto getUSHR(const Expr& expr) const -> const USHR&;

			EVO_NODISCARD auto getBitReverse(const Expr& expr) const -> const BitReverse&;
			EVO_NODISCARD auto getBSwap(const Expr& expr) const -> const BSwap&;
			EVO_NODISCARD auto getCtPop(const Expr& expr) const -> const CtPop&;
			EVO_NODISCARD auto getCTLZ(const Expr& expr) const -> const CTLZ&;
			EVO_NODISCARD auto getCTTZ(const Expr& expr) const -> const CTTZ&;

		private:
			const Module& module;
			const Function* target_func;
	};


}


