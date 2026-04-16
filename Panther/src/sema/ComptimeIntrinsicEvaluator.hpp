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

#include "../../include/TypeManager.hpp"
#include "./TermInfo.hpp"


namespace pcit::panther{


	class ComptimeIntrinsicEvaluator{
		public:
			ComptimeIntrinsicEvaluator(TypeManager& _type_manager, class SemaBuffer& _sema_buffer)
				: type_manager(_type_manager), sema_buffer(_sema_buffer) {}

			~ComptimeIntrinsicEvaluator() = default;


			///////////////////////////////////
			// type traits

			[[nodiscard]] auto getTypeID(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto arrayElementTypeID(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto arrayRefElementTypeID(TypeInfo::ID type_id) -> TermInfo;

			[[nodiscard]] auto numBytes(TypeInfo::ID type_id, bool include_padding) -> TermInfo;
			[[nodiscard]] auto numBits(TypeInfo::ID type_id, bool include_padding) -> TermInfo;

			[[nodiscard]] auto isDefaultInitializable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isTriviallyDefaultInitializable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isComptimeDefaultInitializable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isNoErrorDefaultInitializable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isSafeDefaultInitializable(TypeInfo::ID type_id) -> TermInfo;

			[[nodiscard]] auto isTriviallyDeletable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isComptimeDeletable(TypeInfo::ID type_id) -> TermInfo;

			[[nodiscard]] auto isCopyable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isTriviallyCopyable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isComptimeCopyable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isNoErrorCopyable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isSafeCopyable(TypeInfo::ID type_id) -> TermInfo;
			
			[[nodiscard]] auto isMovable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isTriviallyMovable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isComptimeMovable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isNoErrorMovable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isSafeMovable(TypeInfo::ID type_id) -> TermInfo;

			[[nodiscard]] auto isComparable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isTriviallyComparable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isComptimeComparable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isNoErrorComparable(TypeInfo::ID type_id) -> TermInfo;
			[[nodiscard]] auto isSafeComparable(TypeInfo::ID type_id) -> TermInfo;


			///////////////////////////////////
			// type conversion

			[[nodiscard]] auto trunc(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo;
			[[nodiscard]] auto ftrunc(const TypeInfo::ID to_type_id, const core::GenericFloat& arg) -> TermInfo;
			[[nodiscard]] auto sext(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo;
			[[nodiscard]] auto zext(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo;
			[[nodiscard]] auto fext(const TypeInfo::ID to_type_id, const core::GenericFloat& arg) -> TermInfo;
			[[nodiscard]] auto iToF(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo;
			[[nodiscard]] auto fToI(const TypeInfo::ID to_type_id, const core::GenericFloat& arg) -> TermInfo;




			///////////////////////////////////
			// add

			[[nodiscard]] auto add(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;

			[[nodiscard]] auto addWrap(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto addSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto fadd(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			///////////////////////////////////
			// sub

			[[nodiscard]] auto sub(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;

			[[nodiscard]] auto subWrap(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto subSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto fsub(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			///////////////////////////////////
			// mul

			[[nodiscard]] auto mul(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;

			[[nodiscard]] auto mulWrap(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto mulSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto fmul(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			///////////////////////////////////
			// div / rem

			[[nodiscard]] auto div(
				const TypeInfo::ID type_id, bool is_exact, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;

			[[nodiscard]] auto fdiv(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;

			[[nodiscard]] auto rem(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto rem(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			///////////////////////////////////
			// fneg

			[[nodiscard]] auto fneg(const TypeInfo::ID type_id, const core::GenericFloat& arg) -> TermInfo;


			///////////////////////////////////
			// logical

			[[nodiscard]] auto eq(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto eq(bool lhs, bool rhs) -> TermInfo;

			[[nodiscard]] auto eq(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			[[nodiscard]] auto neq(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto neq(bool lhs, bool rhs) -> TermInfo;

			[[nodiscard]] auto neq(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			[[nodiscard]] auto lt(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto lt(bool lhs, bool rhs) -> TermInfo;

			[[nodiscard]] auto lt(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			[[nodiscard]] auto lte(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto lte(bool lhs, bool rhs) -> TermInfo;

			[[nodiscard]] auto lte(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			[[nodiscard]] auto gt(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto gt(bool lhs, bool rhs) -> TermInfo;

			[[nodiscard]] auto gt(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			[[nodiscard]] auto gte(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto gte(bool lhs, bool rhs) -> TermInfo;

			[[nodiscard]] auto gte(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> TermInfo;


			///////////////////////////////////
			// bitwise

			[[nodiscard]] auto bitwiseAnd(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto bitwiseAnd(bool lhs, bool rhs) -> TermInfo;


			[[nodiscard]] auto bitwiseOr(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto bitwiseOr(bool lhs, bool rhs) -> TermInfo;


			[[nodiscard]] auto bitwiseXor(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto bitwiseXor(bool lhs, bool rhs) -> TermInfo;



			[[nodiscard]] auto shl(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;

			[[nodiscard]] auto shlSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> TermInfo;

			[[nodiscard]] auto shr(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<TermInfo>;


			[[nodiscard]] auto bitReverse(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo;
			[[nodiscard]] auto byteSwap(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo;
			[[nodiscard]] auto ctPop(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo;
			[[nodiscard]] auto ctlz(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo;
			[[nodiscard]] auto cttz(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo;


		private:
			[[nodiscard]] auto add_wrap_impl(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt::WrapResult;

			[[nodiscard]] auto sub_wrap_impl(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt::WrapResult;

			[[nodiscard]] auto mul_wrap_impl(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt::WrapResult;



			template<class RETURN>
			using IntrinOp = std::function<RETURN(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			)>;

			template<class RETURN>
			[[nodiscard]] auto intrin_base_impl(
				const TypeInfo::ID type_id,
				const core::GenericInt& lhs,
				const core::GenericInt& rhs,
				IntrinOp<RETURN> intrin_op
			) -> RETURN;

			template<class RETURN>
			[[nodiscard]] auto intrin_base_impl(
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
