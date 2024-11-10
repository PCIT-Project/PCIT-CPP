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

#include "../../include/ASG.h"


namespace pcit::panther{
	class SemanticAnalyzer;
}

namespace pcit::panther::sema_helper{


	class ComptimeIntrins{
		public:
			ComptimeIntrins(class SemanticAnalyzer& _analyzer) : analyzer(_analyzer) {}
			ComptimeIntrins(class SemanticAnalyzer* _analyzer) : analyzer(*_analyzer) {}
			~ComptimeIntrins() = default;


			EVO_NODISCARD auto call(
				const ASG::TemplatedIntrinsicInstantiation& instantiation,
				evo::ArrayProxy<ASG::Expr> args,
				const SourceLocation& location
			) -> evo::Result<evo::SmallVector<ASG::Expr>>;


			///////////////////////////////////
			// type traits

			EVO_NODISCARD auto isSameType(const ASG::TemplatedIntrinsicInstantiation& instantiation)
				-> evo::SmallVector<ASG::Expr>;

			EVO_NODISCARD auto isTriviallyCopyable(const ASG::TemplatedIntrinsicInstantiation& instantiation)
				-> evo::SmallVector<ASG::Expr>;

			EVO_NODISCARD auto isTriviallyDestructable(const ASG::TemplatedIntrinsicInstantiation& instantiation)
				-> evo::SmallVector<ASG::Expr>;

			EVO_NODISCARD auto isPrimitive(const ASG::TemplatedIntrinsicInstantiation& instantiation)
				-> evo::SmallVector<ASG::Expr>;

			EVO_NODISCARD auto isBuiltin(const ASG::TemplatedIntrinsicInstantiation& instantiation)
				-> evo::SmallVector<ASG::Expr>;

			EVO_NODISCARD auto isIntegral(const ASG::TemplatedIntrinsicInstantiation& instantiation)
				-> evo::SmallVector<ASG::Expr>;

			EVO_NODISCARD auto isFloatingPoint(const ASG::TemplatedIntrinsicInstantiation& instantiation)
				-> evo::SmallVector<ASG::Expr>;

			EVO_NODISCARD auto sizeOf(const ASG::TemplatedIntrinsicInstantiation& instantiation)
				-> evo::SmallVector<ASG::Expr>;

			EVO_NODISCARD auto getTypeID(const ASG::TemplatedIntrinsicInstantiation& instantiation)
				-> evo::SmallVector<ASG::Expr>;



			///////////////////////////////////
			// arithmetic

			EVO_NODISCARD auto add(evo::ArrayProxy<ASG::Expr> args, bool may_wrap, const SourceLocation& location)
				-> evo::Result<evo::SmallVector<ASG::Expr>>;
			EVO_NODISCARD auto addWrap(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto addSat(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto fadd(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;

			EVO_NODISCARD auto sub(evo::ArrayProxy<ASG::Expr> args, bool may_wrap, const SourceLocation& location)
				-> evo::Result<evo::SmallVector<ASG::Expr>>;
			EVO_NODISCARD auto subWrap(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto subSat(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto fsub(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;

			EVO_NODISCARD auto mul(evo::ArrayProxy<ASG::Expr> args, bool may_wrap, const SourceLocation& location)
				-> evo::Result<evo::SmallVector<ASG::Expr>>;
			EVO_NODISCARD auto mulWrap(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto mulSat(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto fmul(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;

			EVO_NODISCARD auto div(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto fdiv(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto rem(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;


			///////////////////////////////////
			// logical

			EVO_NODISCARD auto eq(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto neq(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto lt(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto lte(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto gt(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto gte(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;


			///////////////////////////////////
			// bitwise

			EVO_NODISCARD auto bitwiseAnd(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto bitwiseOr(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto bitwiseXor(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto shl(evo::ArrayProxy<ASG::Expr> args, bool may_overflow, const SourceLocation& location)
				-> evo::Result<evo::SmallVector<ASG::Expr>>;
			EVO_NODISCARD auto shlSat(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr>;
			EVO_NODISCARD auto shr(evo::ArrayProxy<ASG::Expr> args, bool may_overflow, const SourceLocation& location)
				-> evo::Result<evo::SmallVector<ASG::Expr>>;


		private:
			template<class OP>
			EVO_NODISCARD auto logical_impl(evo::ArrayProxy<ASG::Expr> args, OP&& op)
				-> evo::SmallVector<ASG::Expr>;

		private:
			class SemanticAnalyzer& analyzer;
	};


}
