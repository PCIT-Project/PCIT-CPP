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

#include "../../include/source/source_data.h"
#include "../../include/TypeManager.h"
#include "../../include/sema/sema.h"


namespace pcit::panther{

	
	struct TermInfo{
		enum class ValueCategory{
			Ephemeral,
			EphemeralFluid,
			ConcreteConst,
			ConcreteMut,
			ConcreteConstForwardable,
			ConcreteConstDestrMovable,

			Initializer, // uninit / zeroinit
			Module,
			Function, // function, not func pointer
			Intrinsic,
			TemplateIntrinsic, // uninstantiated
			TemplateType, // uninstantiated
			Type,
		};

		enum class ValueStage{
			Constexpr,
			Comptime,
			Runtime,
		};

		struct InitializerType{};
		
		struct FluidType{};

		using FuncOverloadList = evo::SmallVector<evo::Variant<sema::FuncID, sema::TemplatedFuncID>>;


		using TypeID = evo::Variant<
			InitializerType,                // Initializer
			FluidType,                      // EphemeralFluid
			TypeInfo::ID,                   // ConcreteConst|ConcreteMut|ConcreteConstForwardable
						                    //   |ConcreteConstDestrMovable|Ephemeral|Intrinsic
			FuncOverloadList,               // Function
			TypeInfo::VoidableID,           // Type
			evo::SmallVector<TypeInfo::ID>, // Ephemeral
			SourceID,                       // Module
			sema::TemplatedStruct::ID       // TemplateType
			// TODO: TemplateIntrinsic
		>;

		ValueCategory value_category;
		ValueStage value_stage;
		TypeID type_id;


		///////////////////////////////////
		// constructors

		TermInfo(ValueCategory vc, ValueStage vs, evo::SmallVector<TypeInfo::ID>&& _type_id, const sema::Expr& _expr)
			: value_category(vc), value_stage(vs), type_id(InitializerType()), exprs{_expr} {
			evo::debugAssert(this->value_category == ValueCategory::Ephemeral);

			// This is to get around the MSVC bug
			this->type_id.emplace<evo::SmallVector<TypeInfo::ID>>(std::move(_type_id));
		}

		TermInfo(ValueCategory vc, ValueStage vs, auto&& _type_id, const sema::Expr& _expr)
			: value_category(vc), value_stage(vs), type_id(std::forward<decltype(_type_id)>(_type_id)), exprs{_expr} {
			#if defined(PCIT_CONFIG_DEBUG)
				this->check_single_expr_construction();
			#endif
		}

		TermInfo(ValueCategory vc, ValueStage vs, const auto& _type_id, const sema::Expr& _expr)
			: value_category(vc), value_stage(vs), type_id(_type_id), exprs{_expr} {
			#if defined(PCIT_CONFIG_DEBUG)
				this->check_single_expr_construction();
			#endif
		}


		TermInfo(ValueCategory vc, ValueStage vs, auto&& _type_id, std::nullopt_t)
			: value_category(vc), value_stage(vs), type_id(std::forward<decltype(_type_id)>(_type_id)), exprs() {
			#if defined(PCIT_CONFIG_DEBUG)
				this->check_no_expr_construction();
			#endif
		}


		TermInfo(ValueCategory vc, ValueStage vs, const auto& _type_id, std::nullopt_t)
			: value_category(vc), value_stage(vs), type_id(InitializerType()), exprs() {
			#if defined(PCIT_CONFIG_DEBUG)
				this->check_no_expr_construction();
			#endif

			// This is to get around the MSVC bug
			this->type_id.emplace<std::remove_cvref_t<decltype(_type_id)>>(_type_id);
		}


		TermInfo(
			ValueCategory vc,
			ValueStage vs,
			evo::SmallVector<TypeInfo::ID>&& type_ids,
			evo::SmallVector<sema::Expr>&& expr_list
		)
			: value_category(vc), value_stage(vs), type_id(InitializerType{}), exprs(std::move(expr_list)) {
			// TODO: remove this and move directly into `type_id` when the MSVC bug is fixed
			this->type_id.emplace<evo::SmallVector<TypeInfo::ID>>(std::move(type_ids));

			evo::debugAssert(this->value_category == ValueCategory::Ephemeral, "multi-expr must be multi-return");
		}

		
		#if defined(PCIT_CONFIG_DEBUG)
			auto check_single_expr_construction() -> void {
				switch(this->value_category){
					break; case ValueCategory::Ephemeral:
						evo::debugAssert(
							this->type_id.is<TypeInfo::ID>() || this->type_id.is<evo::SmallVector<TypeInfo::ID>>(),
							"Incorrect TypeInfo creation"
						);

					break; case ValueCategory::EphemeralFluid:
						evo::debugAssert(this->type_id.is<FluidType>(), "Incorrect TypeInfo creation");

					break; case ValueCategory::ConcreteConst:
						evo::debugAssert(this->type_id.is<TypeInfo::ID>(), "Incorrect TypeInfo creation");

					break; case ValueCategory::ConcreteMut:
						evo::debugAssert(this->type_id.is<TypeInfo::ID>(), "Incorrect TypeInfo creation");

					break; case ValueCategory::ConcreteConstForwardable:
						evo::debugAssert(this->type_id.is<TypeInfo::ID>(), "Incorrect TypeInfo creation");

					break; case ValueCategory::ConcreteConstDestrMovable:
						evo::debugAssert(this->type_id.is<TypeInfo::ID>(), "Incorrect TypeInfo creation");

					break; case ValueCategory::Initializer:
						evo::debugAssert(this->type_id.is<InitializerType>(), "Incorrect TypeInfo creation");

					break; case ValueCategory::Module:
						evo::debugAssert(this->type_id.is<SourceID>(), "Incorrect TypeInfo creation");

					break; case ValueCategory::Function:
						evo::debugAssert(this->type_id.is<FuncOverloadList>(), "Incorrect TypeInfo creation");

					break; case ValueCategory::Intrinsic:
						evo::debugAssert(this->type_id.is<TypeInfo::ID>(), "Incorrect TypeInfo creation");

					break; case ValueCategory::TemplateIntrinsic:
						evo::unimplemented("ValueCategory::TemplateIntrinsic");

					break; case ValueCategory::TemplateType:
						evo::debugAssert(this->type_id.is<sema::TemplatedStruct::ID>(), "Incorrect TypeInfo creation");

					break; case ValueCategory::Type:
						evo::debugAssert(this->type_id.is<TypeInfo::VoidableID>(), "Incorrect TypeInfo creation");
				}
			}


			auto check_no_expr_construction() -> void {
				evo::debugAssert(
					this->value_category == ValueCategory::Module
					|| this->value_category == ValueCategory::TemplateType
					|| this->value_category == ValueCategory::Type
					|| this->value_category == ValueCategory::Function
				);
				evo::debugAssert(this->value_stage == ValueStage::Constexpr);
				evo::debugAssert(
					this->value_category != ValueCategory::Type || this->type_id.is<TypeInfo::VoidableID>()
				);
			}
		#endif


		///////////////////////////////////
		// value type checking

		EVO_NODISCARD constexpr auto is_ephemeral() const -> bool {
			return this->value_category == ValueCategory::Ephemeral
				|| this->value_category == ValueCategory::EphemeralFluid;
		}

		EVO_NODISCARD constexpr auto is_concrete() const -> bool {
			return this->value_category == ValueCategory::ConcreteConst
				|| this->value_category == ValueCategory::ConcreteMut
				|| this->value_category == ValueCategory::ConcreteConstForwardable
				|| this->value_category == ValueCategory::ConcreteConstDestrMovable;
		}

		EVO_NODISCARD constexpr auto is_const() const -> bool {
			return this->value_category == ValueCategory::ConcreteConst 
				|| this->value_category == ValueCategory::Function;
		}


		///////////////////////////////////
		// value

		EVO_NODISCARD auto isSingleValue() const -> bool {
			return this->type_id.is<TypeInfo::ID>()
				|| this->type_id.is<FluidType>()
				|| this->type_id.is<InitializerType>();
		}

		EVO_NODISCARD auto getExpr() const -> const sema::Expr& {
			evo::debugAssert(this->isSingleValue(), "does not hold expr value");
			return this->exprs[0];
		}

		EVO_NODISCARD auto getExpr() -> sema::Expr& {
			evo::debugAssert(this->isSingleValue(), "does not hold expr value");
			return this->exprs[0];
		}


		//////////////////
		// module expr

		EVO_NODISCARD auto isModuleExpr() const -> bool {
			return this->type_id.is<SourceID>();
		}

		EVO_NODISCARD auto getModuleExpr() const -> const sema::Expr& {
			evo::debugAssert(this->isModuleExpr(), "does not hold module");
			return this->exprs[0];
		}


		//////////////////
		// multi-value

		EVO_NODISCARD auto isMultiValue() const -> bool {
			return this->type_id.is<evo::SmallVector<TypeInfo::ID>>();
		}

		EVO_NODISCARD auto getMultiExpr() const -> const evo::SmallVector<sema::Expr>& {
			evo::debugAssert(this->isMultiValue(), "does not hold expr value");
			return this->exprs;
		}


		private:
			evo::SmallVector<sema::Expr> exprs; // empty if from ValueCategory::Module
	};
	


}