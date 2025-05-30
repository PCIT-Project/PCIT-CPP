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
#include "../../include/intrinsics.h"
#include "../../include/sema/sema.h"


namespace pcit::panther{

	
	struct TermInfo{
		enum class ValueCategory{
			EPHEMERAL,
			EPHEMERAL_FLUID,
			CONCRETE_CONST,
			CONCRETE_MUT,
			CONCRETE_FORWARDABLE,

			INITIALIZER, // uninit / zeroinit
			MODULE,
			FUNCTION, // function, not func pointer
			METHOD_CALL, // the expr is the 'this'
			INTRINSIC_FUNC,
			TEMPLATE_INTRINSIC_FUNC, // uninstantiated
			TEMPLATE_TYPE,           // uninstantiated
			TYPE,
			TEMPLATE_DECL_INSTANTIATION_TYPE,
			EXCEPT_PARAM_PACK,
		};

		enum class ValueStage{
			CONSTEXPR,
			COMPTIME,
			RUNTIME,
		};

		struct InitializerType{};
		struct FluidType{};
		struct TemplateDeclInstantiationType{};
		struct ExceptParamPack{};


		using FuncOverloadList = evo::SmallVector<evo::Variant<sema::FuncID, sema::TemplatedFuncID>>;


		using TypeID = evo::Variant<
			InitializerType,                // INITIALIZER
			FluidType,                      // EPHEMERAL_FLUID
			TemplateDeclInstantiationType,  // TEMPLATE_DECL_INSTANTIATION_TYPE
			ExceptParamPack,                // EXCEPT_PARAM_PACK
			TypeInfo::ID,                   // CONCRETE_CONST|CONCRETE_MUT|CONCRETE_FORWARDABLE|EPHEMERAL|INTRINSIC_FUNC
			FuncOverloadList,               // FUNCTION|METHOD_CALL
			TypeInfo::VoidableID,           // TYPE
			evo::SmallVector<TypeInfo::ID>, // EPHEMERAL
			SourceID,                       // MODULE
			sema::TemplatedStruct::ID,      // TEMPLATE_TYPE
			TemplateIntrinsicFunc::Kind     // TEMPLATE_INTRINSIC_FUNC
		>;

		ValueCategory value_category;
		ValueStage value_stage;
		TypeID type_id;


		///////////////////////////////////
		// constructors

		TermInfo(ValueCategory vc, ValueStage vs, evo::SmallVector<TypeInfo::ID>&& _type_id, const sema::Expr& _expr)
			: value_category(vc), value_stage(vs), type_id(InitializerType()), exprs{_expr} {
			evo::debugAssert(this->value_category == ValueCategory::EPHEMERAL);

			// This is to get around the MSVC bug
			this->type_id.emplace<evo::SmallVector<TypeInfo::ID>>(std::move(_type_id));

			#if defined(PCIT_CONFIG_DEBUG)
				this->check_single_expr_construction();
			#endif
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
			// This is to get around the MSVC bug
			this->type_id.emplace<std::remove_cvref_t<decltype(_type_id)>>(_type_id);

			#if defined(PCIT_CONFIG_DEBUG)
				this->check_no_expr_construction();
			#endif
		}


		TermInfo(
			ValueCategory vc,
			ValueStage vs,
			evo::SmallVector<TypeInfo::ID>&& type_ids,
			evo::SmallVector<sema::Expr>&& expr_list
		)
			: value_category(vc), value_stage(vs), type_id(InitializerType{}), exprs(std::move(expr_list)) {
			// TODO(FUTURE): remove this and move directly into `type_id` when the MSVC bug is fixed
			this->type_id.emplace<evo::SmallVector<TypeInfo::ID>>(std::move(type_ids));

			evo::debugAssert(this->value_category == ValueCategory::EPHEMERAL, "multi-expr must be multi-return");
		}


		TermInfo(ValueCategory vc, ValueStage vs, ExceptParamPack, evo::SmallVector<sema::Expr>&& expr_list)
			: value_category(vc), value_stage(vs), type_id(ExceptParamPack{}), exprs(std::move(expr_list)) {}

		
		#if defined(PCIT_CONFIG_DEBUG)
			auto check_single_expr_construction() -> void {
				switch(this->value_category){
					break; case ValueCategory::EPHEMERAL:
						evo::debugAssert(
							this->type_id.is<TypeInfo::ID>() || this->type_id.is<evo::SmallVector<TypeInfo::ID>>(),
							"Incorrect TermInfo creation"
						);

					break; case ValueCategory::EPHEMERAL_FLUID:
						evo::debugAssert(this->type_id.is<FluidType>(), "Incorrect TermInfo creation");

					break; case ValueCategory::CONCRETE_CONST:
						evo::debugAssert(this->type_id.is<TypeInfo::ID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::CONCRETE_MUT:
						evo::debugAssert(this->type_id.is<TypeInfo::ID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::CONCRETE_FORWARDABLE:
						evo::debugAssert(this->type_id.is<TypeInfo::ID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::INITIALIZER:
						evo::debugAssert(this->type_id.is<InitializerType>(), "Incorrect TermInfo creation");

					break; case ValueCategory::MODULE:
						evo::debugAssert(this->type_id.is<SourceID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::FUNCTION:
						// evo::debugAssert(this->type_id.is<FuncOverloadList>(), "Incorrect TermInfo creation");
						evo::debugFatalBreak("Incorrect TermInfo creation");

					break; case ValueCategory::METHOD_CALL:
						evo::debugAssert(this->type_id.is<FuncOverloadList>(), "Incorrect TermInfo creation");

					break; case ValueCategory::INTRINSIC_FUNC:
						evo::debugAssert(this->type_id.is<TypeInfo::ID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::TEMPLATE_INTRINSIC_FUNC:
						evo::debugFatalBreak("Incorrect TermInfo creation");

					break; case ValueCategory::TEMPLATE_TYPE:
						evo::debugAssert(this->type_id.is<sema::TemplatedStruct::ID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::TYPE:
						evo::debugAssert(this->type_id.is<TypeInfo::VoidableID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::TEMPLATE_DECL_INSTANTIATION_TYPE:
						evo::debugAssert(
							this->type_id.is<TemplateDeclInstantiationType>(), "Incorrect TermInfo creation"
						);

					break; case ValueCategory::EXCEPT_PARAM_PACK:
						evo::debugFatalBreak("Incorrect TermInfo creation");

				}
			}


			auto check_no_expr_construction() -> void {
				evo::debugAssert(
					this->value_category == ValueCategory::MODULE
					|| this->value_category == ValueCategory::TEMPLATE_TYPE
					|| this->value_category == ValueCategory::TYPE
					|| this->value_category == ValueCategory::FUNCTION
					|| this->value_category == ValueCategory::TEMPLATE_INTRINSIC_FUNC
					|| this->value_category == ValueCategory::TEMPLATE_DECL_INSTANTIATION_TYPE
				);

				evo::debugAssert(this->value_stage == ValueStage::CONSTEXPR);

				if(this->value_category == ValueCategory::TYPE){
					evo::debugAssert(
						this->type_id.is<TypeInfo::VoidableID>(),
						"ValueCategory of TYPE must have a `type_id` of TypeInfo::VoidableID"
					);
				}
			}
		#endif


		EVO_NODISCARD static auto fromFakeTermInfo(const sema::FakeTermInfo& fake_term_info) -> TermInfo {
			const ValueCategory value_category = [&](){
				switch(fake_term_info.valueCategory){
					case sema::FakeTermInfo::ValueCategory::EPHEMERAL:
						return ValueCategory::EPHEMERAL;

					case sema::FakeTermInfo::ValueCategory::CONCRETE_CONST:
						return ValueCategory::CONCRETE_CONST;

					case sema::FakeTermInfo::ValueCategory::CONCRETE_MUT:
						return ValueCategory::CONCRETE_MUT;

					case sema::FakeTermInfo::ValueCategory::CONCRETE_FORWARDABLE:
						return ValueCategory::CONCRETE_FORWARDABLE;
				}
				evo::unreachable();
			}();

			const ValueStage value_stage = [&](){
				switch(fake_term_info.valueStage){
					case sema::FakeTermInfo::ValueStage::CONSTEXPR: return ValueStage::CONSTEXPR;
					case sema::FakeTermInfo::ValueStage::COMPTIME:  return ValueStage::COMPTIME;
					case sema::FakeTermInfo::ValueStage::RUNTIME:   return ValueStage::RUNTIME;
				}
				evo::unreachable();
			}();

			return TermInfo(value_category, value_stage, fake_term_info.typeID, fake_term_info.expr);
		}


		///////////////////////////////////
		// value type checking

		EVO_NODISCARD constexpr auto is_ephemeral() const -> bool {
			return this->value_category == ValueCategory::EPHEMERAL
				|| this->value_category == ValueCategory::EPHEMERAL_FLUID;
		}

		EVO_NODISCARD constexpr auto is_concrete() const -> bool {
			return this->value_category == ValueCategory::CONCRETE_CONST
				|| this->value_category == ValueCategory::CONCRETE_MUT
				|| this->value_category == ValueCategory::CONCRETE_FORWARDABLE;
		}

		EVO_NODISCARD constexpr auto is_const() const -> bool {
			return this->value_category == ValueCategory::CONCRETE_CONST 
				|| this->value_category == ValueCategory::FUNCTION
				|| this->value_category == ValueCategory::METHOD_CALL;
		}


		///////////////////////////////////
		// value

		EVO_NODISCARD auto isSingleValue() const -> bool {
			return this->type_id.is<TypeInfo::ID>()
				|| this->type_id.is<FluidType>()
				|| this->type_id.is<InitializerType>()
				|| this->value_category == ValueCategory::METHOD_CALL;
		}

		EVO_NODISCARD auto getExpr() const -> const sema::Expr& {
			evo::debugAssert(
				this->isSingleValue() || (this->isMultiValue() && this->exprs.size() == 1), "does not hold expr value"
			);
			return this->exprs[0];
		}

		EVO_NODISCARD auto getExpr() -> sema::Expr& {
			evo::debugAssert(
				this->isSingleValue() || (this->isMultiValue() && this->exprs.size() == 1), "does not hold expr value"
			);
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
			evo::debugAssert(this->isMultiValue(), "does not hold multi expr value");
			return this->exprs;
		}


		//////////////////
		// multi-value

		EVO_NODISCARD auto isExceptParamPack() const -> bool {
			return this->type_id.is<ExceptParamPack>();
		}

		EVO_NODISCARD auto getExceptParamPack() const -> const evo::SmallVector<sema::Expr>& {
			evo::debugAssert(this->isExceptParamPack(), "does not hold except param pack");
			return this->exprs;
		}


		private:
			evo::SmallVector<sema::Expr> exprs; // empty if from ValueCategory::Module
	};
	


}