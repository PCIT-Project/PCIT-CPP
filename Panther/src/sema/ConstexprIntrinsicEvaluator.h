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

#include "../../include/TypeManager.h"
#include "./TermInfo.h"


namespace pcit::panther{


	class ConstexprIntrinsicEvaluator{
		public:
			ConstexprIntrinsicEvaluator(TypeManager& _type_manager, class SemaBuffer& _sema_buffer)
				: type_manager(_type_manager), sema_buffer(_sema_buffer) {}

			~ConstexprIntrinsicEvaluator() = default;


			///////////////////////////////////
			// type traits

			EVO_NODISCARD auto numBytes(TypeInfo::ID type_id) -> TermInfo;
			EVO_NODISCARD auto numBits(TypeInfo::ID type_id) -> TermInfo;


			///////////////////////////////////
			// type conversion

			EVO_NODISCARD auto trunc(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo;
			EVO_NODISCARD auto ftrunc(const TypeInfo::ID to_type_id, const core::GenericFloat& arg) -> TermInfo;
			EVO_NODISCARD auto sext(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo;
			EVO_NODISCARD auto zext(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo;
			EVO_NODISCARD auto fext(const TypeInfo::ID to_type_id, const core::GenericFloat& arg) -> TermInfo;
			EVO_NODISCARD auto iToF(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo;
			EVO_NODISCARD auto fToI(const TypeInfo::ID to_type_id, const core::GenericFloat& arg) -> TermInfo;




			///////////////////////////////////
			// add

			EVO_NODISCARD auto add(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;

			EVO_NODISCARD auto addWrap(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto addSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto fadd(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			///////////////////////////////////
			// sub

			EVO_NODISCARD auto sub(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;

			EVO_NODISCARD auto subWrap(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto subSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto fsub(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			///////////////////////////////////
			// mul

			EVO_NODISCARD auto mul(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;

			EVO_NODISCARD auto mulWrap(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto mulSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto fmul(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			///////////////////////////////////
			// div / rem

			EVO_NODISCARD auto div(
				const TypeInfo::ID type_id, bool is_exact, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;

			EVO_NODISCARD auto fdiv(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;

			EVO_NODISCARD auto rem(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto rem(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			///////////////////////////////////
			// fneg

			EVO_NODISCARD auto fneg(const TypeInfo::ID type_id, const core::GenericFloat& arg) -> TermInfo;


			///////////////////////////////////
			// logical

			EVO_NODISCARD auto eq(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto eq(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			EVO_NODISCARD auto neq(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto neq(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			EVO_NODISCARD auto lt(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto lt(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			EVO_NODISCARD auto lte(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto lte(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			EVO_NODISCARD auto gt(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto gt(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			EVO_NODISCARD auto gte(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto gte(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			///////////////////////////////////
			// bitwise

			EVO_NODISCARD auto bitwiseAnd(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto bitwiseOr(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto bitwiseXor(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto shl(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;

			EVO_NODISCARD auto shlSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			EVO_NODISCARD auto shr(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;


			EVO_NODISCARD auto bitReverse(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo;
			EVO_NODISCARD auto bSwap(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo;
			EVO_NODISCARD auto ctPop(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo;
			EVO_NODISCARD auto ctlz(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo;
			EVO_NODISCARD auto cttz(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo;


		private:
			EVO_NODISCARD auto add_wrap_impl(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt::WrapResult;

			EVO_NODISCARD auto sub_wrap_impl(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt::WrapResult;

			EVO_NODISCARD auto mul_wrap_impl(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt::WrapResult;



			template<class RETURN>
			using IntrinOp = std::function<RETURN(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			)>;

			template<class RETURN>
			EVO_NODISCARD auto intrin_base_impl(
				const TypeInfo::ID type_id,
				const core::GenericInt& lhs,
				const core::GenericInt& rhs,
				IntrinOp<RETURN> intrin_op
			) -> RETURN;

			template<class RETURN>
			EVO_NODISCARD auto intrin_base_impl(
				const TypeInfo::ID type_id,
				const core::GenericFloat& lhs,
				const core::GenericFloat& rhs,
				std::function<RETURN(const core::GenericFloat&, const core::GenericFloat&)> intrin_op
			) -> RETURN;
	
		private:
			TypeManager& type_manager;
			class SemaBuffer& sema_buffer;
	};


}
