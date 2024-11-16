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

#include "../include/ASG.h"
#include "../include/ASGBuffer.h"


namespace pcit::panther{


	class ComptimeExecutor{
		public:
			ComptimeExecutor(class Context& _context, core::Printer& _printer) : context(_context), printer(_printer) {}
			
			#if defined(PCIT_CONFIG_DEBUG)
				~ComptimeExecutor(){
					evo::debugAssert(
						this->isInitialized() == false, "Didn't deinit ComptimeExecutor before destructor"
					);
				}
			#else
				~ComptimeExecutor() = default;
			#endif

			auto init() -> std::string; // string is error message if initalization fails (empty if successful)
			auto deinit() -> void;

			EVO_NODISCARD auto isInitialized() const -> bool { return this->data != nullptr; }

			EVO_NODISCARD auto runFunc(
				const ASG::Func::LinkID& link_id, evo::ArrayProxy<ASG::Expr> params, ASGBuffer& asg_buffer
			) -> evo::Result<evo::SmallVector<ASG::Expr>>;


			auto addFunc(const ASG::Func::LinkID& func_link_id) -> void;


			//////////////////////////////////////////////////////////////////////
			// intrinsics

			///////////////////////////////////
			// add

			EVO_NODISCARD auto intrinAdd(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<core::GenericInt>;

			EVO_NODISCARD auto intrinAddWrap(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt::WrapResult;

			EVO_NODISCARD auto intrinAddSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt;

			EVO_NODISCARD auto intrinFAdd(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> core::GenericFloat;


			///////////////////////////////////
			// sub

			EVO_NODISCARD auto intrinSub(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<core::GenericInt>;

			EVO_NODISCARD auto intrinSubWrap(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt::WrapResult;

			EVO_NODISCARD auto intrinSubSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt;

			EVO_NODISCARD auto intrinFSub(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> core::GenericFloat;


			///////////////////////////////////
			// mul

			EVO_NODISCARD auto intrinMul(
				const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> evo::Result<core::GenericInt>;

			EVO_NODISCARD auto intrinMulWrap(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt::WrapResult;

			EVO_NODISCARD auto intrinMulSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt;

			EVO_NODISCARD auto intrinFMul(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> core::GenericFloat;


			///////////////////////////////////
			// div / rem

			EVO_NODISCARD auto intrinDiv(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt;

			EVO_NODISCARD auto intrinFDiv(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> core::GenericFloat;

			EVO_NODISCARD auto intrinRem(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt;

			EVO_NODISCARD auto intrinRem(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> core::GenericFloat;


			///////////////////////////////////
			// logical

			EVO_NODISCARD auto intrinEQ(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> bool;

			EVO_NODISCARD auto intrinEQ(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> bool;


			EVO_NODISCARD auto intrinNEQ(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> bool;

			EVO_NODISCARD auto intrinNEQ(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> bool;


			EVO_NODISCARD auto intrinLT(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> bool;

			EVO_NODISCARD auto intrinLT(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> bool;


			EVO_NODISCARD auto intrinLTE(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> bool;

			EVO_NODISCARD auto intrinLTE(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> bool;


			EVO_NODISCARD auto intrinGT(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> bool;

			EVO_NODISCARD auto intrinGT(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> bool;


			EVO_NODISCARD auto intrinGTE(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> bool;

			EVO_NODISCARD auto intrinGTE(
				const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
			) -> bool;


			///////////////////////////////////
			// bitwise

			EVO_NODISCARD auto intrinAnd(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt;

			EVO_NODISCARD auto intrinOr(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt;

			EVO_NODISCARD auto intrinXor(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt;

			EVO_NODISCARD auto intrinSHL(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs, bool may_overflow
			) -> evo::Result<core::GenericInt>;

			EVO_NODISCARD auto intrinSHLSat(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			) -> core::GenericInt;

			EVO_NODISCARD auto intrinSHR(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs, bool may_overflow
			) -> evo::Result<core::GenericInt>;


		private:
			auto restart_engine_if_needed() -> void;

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
			class Context& context;
			core::Printer& printer;

			mutable core::SpinLock mutex{};
			bool requires_engine_restart = false;

			struct Data;
			Data* data = nullptr;
	};


}
