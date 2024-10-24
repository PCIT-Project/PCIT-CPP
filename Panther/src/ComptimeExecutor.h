//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "../include/ASG.h"
#include "../include/ASGBuffer.h"


namespace pcit::panther{


	class ComptimeExecutor{
		public:
			ComptimeExecutor(class Context& _context, core::Printer& _printer) : context(_context), printer(_printer) {}
			~ComptimeExecutor() = default;

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



		private:
			auto restart_engine_if_needed() -> void;

			template<class RETURN>
			using IntrinOp = std::function<RETURN(
				const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
			)>;

			template<class RETURN>
			EVO_NODISCARD auto intrin_arithmetic(
				const TypeInfo::ID type_id,
				const core::GenericInt& lhs,
				const core::GenericInt& rhs,
				IntrinOp<RETURN> intrin_op
			) -> RETURN;

			EVO_NODISCARD auto intrin_arithmetic(
				const TypeInfo::ID type_id,
				const core::GenericFloat& lhs,
				const core::GenericFloat& rhs,
				std::function<core::GenericFloat(const core::GenericFloat&, const core::GenericFloat&)> intrin_op
			) -> core::GenericFloat;

		private:
			class Context& context;
			core::Printer& printer;

			mutable std::shared_mutex mutex{};
			bool requires_engine_restart = false;

			struct Data;
			Data* data = nullptr;
	};


}
