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

			EVO_NODISCARD auto hasTargetFunction() const -> bool { return this->target_func != nullptr; }
			EVO_NODISCARD auto setTargetFunction(const Function& func) -> void { this->target_func = &func; }
			EVO_NODISCARD auto clearTargetFunction() -> void { this->target_func = nullptr; }


			EVO_NODISCARD auto getExprType(const Expr& expr) const -> Type;

			auto getBasicBlock(BasicBlock::ID id) const -> const BasicBlock&;

			EVO_NODISCARD auto getNumber(const Expr& expr) const -> const Number&;
			EVO_NODISCARD static auto getBoolean(const Expr& expr) -> bool;
			EVO_NODISCARD static auto getParamExpr(const Expr& expr) -> ParamExpr;
			EVO_NODISCARD auto getGlobalValue(const Expr& expr) const -> const GlobalVar&;

			EVO_NODISCARD auto getCall(const Expr& expr) const -> const Call&;
			EVO_NODISCARD auto getCallVoid(const Expr& expr) const -> const CallVoid&;
			EVO_NODISCARD auto getRet(const Expr& expr) const -> const Ret&;
			EVO_NODISCARD static auto getBranch(const Expr& expr) -> Branch;

			EVO_NODISCARD auto getAlloca(const Expr& expr) const -> const Alloca&;
			EVO_NODISCARD auto getLoad(const Expr& expr) const -> const Load&;
			EVO_NODISCARD auto getStore(const Expr& expr) const -> const Store&;

			EVO_NODISCARD auto getAdd(const Expr& expr) const -> const Add&;
			EVO_NODISCARD auto getAddWrap(const Expr& expr) const -> const AddWrap&;
			EVO_NODISCARD static auto extractAddWrapResult(const Expr& expr) -> Expr;
			EVO_NODISCARD static auto extractAddWrapWrapped(const Expr& expr) -> Expr;



		private:
			const Module& module;
			const Function* target_func;
	};


}


