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

			EVO_NODISCARD auto getExprType(const Expr& expr) const -> Type;

			auto getBasicBlock(BasicBlock::ID id) const -> const BasicBlock&;

			EVO_NODISCARD auto getNumber(const Expr& expr) const -> const Number&;
			EVO_NODISCARD static auto getParamExpr(const Expr& expr) -> ParamExpr;

			EVO_NODISCARD auto getCallVoidInst(const Expr& expr) const -> const CallVoidInst&;
			EVO_NODISCARD auto getCallInst(const Expr& expr) const -> const CallInst&;
			EVO_NODISCARD auto getRetInst(const Expr& expr) const -> const RetInst&;
			EVO_NODISCARD static auto getBrInst(const Expr& expr) -> BrInst;
			EVO_NODISCARD auto getAdd(const Expr& expr) const -> const Add&;



		private:
			const Module& module;
			const Function* target_func;
	};


}


