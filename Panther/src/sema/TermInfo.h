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
			FORWARDABLE,

			INITIALIZER, // uninit / zeroinit
			NULL_VALUE,
			EXPR_DEDUCER,
			MODULE,
			CLANG_MODULE,
			BUILTIN_MODULE,
			FUNCTION, // function, not func pointer
			FUNCTION_PUB_REQUIRED, // same as FUNCTION, but requires pub checking
			FUNCTION_NOT_PRIV_REQUIRED, // same as FUNCTION, but requires not priv checking
			METHOD_CALL, // the expr is the 'this'
			INTERFACE_CALL,
			POLY_INTERFACE_CALL,
			INTRINSIC_FUNC,
			TEMPLATE_INTRINSIC_FUNC, // uninstantiated
			BUILTIN_TYPE_METHOD,
			TEMPLATE_TYPE,  // uninstantiated
			TEMPLATE_TYPE_PUB_REQUIRED, // same as TEMPLATE_TYPE, but requires pub checking
			TYPE,
			TEMPLATE_DECL_INSTANTIATION_TYPE,
			EXCEPT_PARAM_PACK,
			TAGGED_UNION_FIELD_ACCESSOR,
			VARIADIC_PARAM,
		};

		enum class ValueStage{
			NOT_APPLICABLE,
			CONSTEXPR,
			COMPTIME,
			RUNTIME,
		};

		enum class ValueState{
			NOT_APPLICABLE,
			INIT,
			INITIALIZING,
			UNINIT,
			MOVED_FROM,
		};

		struct InitializerType{};
		struct NullType{};
		struct ExprDeducerType{ Token::ID deducer_token_id; };
		struct FluidType{};
		struct TemplateDeclInstantiationType{};
		struct ExceptParamPack{};
		struct VariadicParamTypes{ evo::SmallVector<TypeInfo::ID> type_ids; };


		using FuncOverloadList = evo::SmallVector<evo::Variant<sema::FuncID, sema::TemplatedFuncID>>;

		struct BuiltinTypeMethod{
			enum class Kind{
				OPT_EXTRACT,
				ARRAY_SIZE,
				ARRAY_DIMENSIONS,
				ARRAY_DATA,
				ARRAY_REF_SIZE,
				ARRAY_REF_DIMENSIONS,
				ARRAY_REF_DATA,
			};

			TypeInfo::ID typeID;
			Kind kind;
		};

		struct TaggedUnionFieldAccessor{
			BaseType::Union::ID union_id;
			uint32_t field_index;
		};


		using TypeID = evo::Variant<
			InitializerType,                // INITIALIZER
			NullType,                       // NULL_VALUE
			ExprDeducerType,                // EXPR_DEDUCER
			FluidType,                      // EPHEMERAL_FLUID
			TemplateDeclInstantiationType,  // TEMPLATE_DECL_INSTANTIATION_TYPE
			ExceptParamPack,                // EXCEPT_PARAM_PACK
			TypeInfo::ID,                   // CONCRETE_CONST|CONCRETE_MUT|FORWARDABLE|EPHEMERAL|INTRINSIC_FUNC
			BuiltinTypeMethod,              // BUILTIN_TYPE_METHOD
			FuncOverloadList,               // FUNCTION|FUNCTION_PUB_REQUIRED|FUNCTION_NOT_PRIV_REQUIRED|METHOD_CALL
											//     |INTERFACE_CALL|POLY_INTERFACE_CALL
			evo::SmallVector<TypeInfo::ID>, // EPHEMERAL
			SourceID,                       // MODULE
			ClangSourceID,                  // CLANG_MODULE
			BuiltinModuleID,                // BUILTIN_MODULE
			TypeInfo::VoidableID,           // TYPE
			sema::TemplatedStruct::ID,      // TEMPLATE_TYPE|TEMPLATE_TYPE_PUB_REQUIRED
			TemplateIntrinsicFunc::Kind,    // TEMPLATE_INTRINSIC_FUNC
			TaggedUnionFieldAccessor,       // TAGGED_UNION_FIELD_ACCESSOR
			VariadicParamTypes              // VARIADIC_PARAM
		>;

		ValueCategory value_category;
		ValueStage value_stage;
		ValueState value_state;
		TypeID type_id;


		///////////////////////////////////
		// constructors

		TermInfo(
			ValueCategory cat, ValueStage stage, evo::SmallVector<TypeInfo::ID>&& _type_id, const sema::Expr& _expr
		) :
			value_category(cat),
			value_stage(stage),
			value_state(ValueState::NOT_APPLICABLE),
			type_id(InitializerType()),
			exprs{_expr}
		{
			evo::debugAssert(this->value_category == ValueCategory::EPHEMERAL);

			// This is to get around the MSVC bug
			this->type_id.emplace<evo::SmallVector<TypeInfo::ID>>(std::move(_type_id));

			#if defined(PCIT_CONFIG_DEBUG)
				this->check_single_expr_construction();
			#endif
		}

		TermInfo(ValueCategory cat, ValueStage stage, ValueState state, auto&& _type_id, const sema::Expr& _expr) :
			value_category(cat),
			value_stage(stage),
			value_state(state),
			type_id(std::forward<decltype(_type_id)>(_type_id)),
			exprs{_expr}
		{
			#if defined(PCIT_CONFIG_DEBUG)
				this->check_single_expr_construction();
			#endif
			evo::debugAssert(
				state == ValueState::NOT_APPLICABLE || this->is_concrete(),
				"Only concrete values can have a value state of something besides NOT_APPLICABLE"
			);
		}

		TermInfo(ValueCategory cat, ValueStage stage, ValueState state, const auto& _type_id, const sema::Expr& _expr)
			: value_category(cat), value_stage(stage), value_state(state), type_id(_type_id), exprs{_expr} {
			#if defined(PCIT_CONFIG_DEBUG)
				this->check_single_expr_construction();
			#endif
			evo::debugAssert(
				state == ValueState::NOT_APPLICABLE || this->is_concrete(),
				"Only concrete values can have a value state of something besides NOT_APPLICABLE"
			);
		}


		TermInfo(ValueCategory cat, auto&& _type_id) :
			value_category(cat),
			value_stage(ValueStage::NOT_APPLICABLE),
			value_state(ValueState::NOT_APPLICABLE),
			type_id(std::forward<decltype(_type_id)>(_type_id)),
			exprs()
		{
			#if defined(PCIT_CONFIG_DEBUG)
				this->check_no_expr_construction();
			#endif
		}


		TermInfo(ValueCategory cat, const auto& _type_id) :
			value_category(cat),
			value_stage(ValueStage::NOT_APPLICABLE),
			value_state(ValueState::NOT_APPLICABLE),
			type_id(InitializerType()),
			exprs()
		{
			// This is to get around the MSVC bug
			this->type_id.emplace<std::remove_cvref_t<decltype(_type_id)>>(_type_id);

			#if defined(PCIT_CONFIG_DEBUG)
				this->check_no_expr_construction();
			#endif
		}


		TermInfo(
			ValueCategory cat,
			ValueStage stage,
			evo::SmallVector<TypeInfo::ID>&& type_ids,
			evo::SmallVector<sema::Expr>&& expr_list
		) : 
			value_category(cat),
			value_stage(stage),
			value_state(ValueState::NOT_APPLICABLE),
			type_id(InitializerType{}),
			exprs(std::move(expr_list))
		{
			// TODO(FUTURE): remove this and move directly into `type_id` when the MSVC bug is fixed
			this->type_id.emplace<evo::SmallVector<TypeInfo::ID>>(std::move(type_ids));

			evo::debugAssert(this->value_category == ValueCategory::EPHEMERAL, "multi-expr must be multi-return");
		}


		TermInfo(ValueCategory cat, ValueStage stage, ExceptParamPack, evo::SmallVector<sema::Expr>&& expr_list) :
			value_category(cat),
			value_stage(stage),
			value_state(ValueState::NOT_APPLICABLE),
			type_id(ExceptParamPack{}),
			exprs(std::move(expr_list))
		{}

		
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

					break; case ValueCategory::FORWARDABLE:
						evo::debugAssert(this->type_id.is<TypeInfo::ID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::INITIALIZER:
						evo::debugAssert(this->type_id.is<InitializerType>(), "Incorrect TermInfo creation");

					break; case ValueCategory::NULL_VALUE:
						evo::debugAssert(this->type_id.is<NullType>(), "Incorrect TermInfo creation");

					break; case ValueCategory::EXPR_DEDUCER:
						evo::debugAssert(this->type_id.is<ExprDeducerType>(), "Incorrect TermInfo creation");

					break; case ValueCategory::MODULE:
						evo::debugAssert(this->type_id.is<SourceID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::CLANG_MODULE:
						evo::debugAssert(this->type_id.is<ClangSourceID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::BUILTIN_MODULE:
						evo::debugAssert(this->type_id.is<BuiltinModuleID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::FUNCTION:
						evo::debugFatalBreak("Incorrect TermInfo creation");

					break; case ValueCategory::FUNCTION_PUB_REQUIRED:
						evo::debugFatalBreak("Incorrect TermInfo creation");

					break; case ValueCategory::FUNCTION_NOT_PRIV_REQUIRED:
						evo::debugFatalBreak("Incorrect TermInfo creation");

					break; case ValueCategory::METHOD_CALL:
						evo::debugAssert(this->type_id.is<FuncOverloadList>(), "Incorrect TermInfo creation");

					break; case ValueCategory::INTERFACE_CALL:
						evo::debugAssert(this->type_id.is<FuncOverloadList>(), "Incorrect TermInfo creation");

					break; case ValueCategory::POLY_INTERFACE_CALL:
						evo::debugAssert(this->type_id.is<FuncOverloadList>(), "Incorrect TermInfo creation");

					break; case ValueCategory::INTRINSIC_FUNC:
						evo::debugAssert(this->type_id.is<TypeInfo::ID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::TEMPLATE_INTRINSIC_FUNC:
						evo::debugFatalBreak("Incorrect TermInfo creation");

					break; case ValueCategory::BUILTIN_TYPE_METHOD:
						evo::debugAssert(this->type_id.is<BuiltinTypeMethod>(), "Incorrect TermInfo creation");

					break; case ValueCategory::TEMPLATE_TYPE:
						evo::debugAssert(this->type_id.is<sema::TemplatedStruct::ID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::TEMPLATE_TYPE_PUB_REQUIRED:
						evo::debugAssert(this->type_id.is<sema::TemplatedStruct::ID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::TYPE:
						evo::debugAssert(this->type_id.is<TypeInfo::VoidableID>(), "Incorrect TermInfo creation");

					break; case ValueCategory::TEMPLATE_DECL_INSTANTIATION_TYPE:
						evo::debugAssert(
							this->type_id.is<TemplateDeclInstantiationType>(), "Incorrect TermInfo creation"
						);

					break; case ValueCategory::EXCEPT_PARAM_PACK:
						evo::debugFatalBreak("Incorrect TermInfo creation");

					break; case ValueCategory::TAGGED_UNION_FIELD_ACCESSOR:
						evo::debugFatalBreak("Incorrect TermInfo creation");

					break; case ValueCategory::VARIADIC_PARAM:
						evo::debugAssert(this->type_id.is<VariadicParamTypes>(), "Incorrect TermInfo creation");
				}
			}


			auto check_no_expr_construction() -> void {
				evo::debugAssert(
					this->value_category == ValueCategory::EXPR_DEDUCER
					|| this->value_category == ValueCategory::MODULE
					|| this->value_category == ValueCategory::CLANG_MODULE
					|| this->value_category == ValueCategory::BUILTIN_MODULE
					|| this->value_category == ValueCategory::TEMPLATE_TYPE
					|| this->value_category == ValueCategory::TEMPLATE_TYPE_PUB_REQUIRED
					|| this->value_category == ValueCategory::TYPE
					|| this->value_category == ValueCategory::FUNCTION
					|| this->value_category == ValueCategory::FUNCTION_PUB_REQUIRED
					|| this->value_category == ValueCategory::FUNCTION_NOT_PRIV_REQUIRED
					|| this->value_category == ValueCategory::TEMPLATE_INTRINSIC_FUNC
					|| this->value_category == ValueCategory::TEMPLATE_DECL_INSTANTIATION_TYPE
					|| this->value_category == ValueCategory::TAGGED_UNION_FIELD_ACCESSOR
				);

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

					case sema::FakeTermInfo::ValueCategory::FORWARDABLE:
						return ValueCategory::FORWARDABLE;
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

			const ValueState value_state = [&](){
				switch(fake_term_info.valueState){
					case sema::FakeTermInfo::ValueState::NOT_APPLICABLE: return ValueState::NOT_APPLICABLE;
					case sema::FakeTermInfo::ValueState::INIT:           return ValueState::INIT;
					case sema::FakeTermInfo::ValueState::INITIALIZING:   return ValueState::INITIALIZING;
					case sema::FakeTermInfo::ValueState::UNINIT:         return ValueState::UNINIT;
					case sema::FakeTermInfo::ValueState::MOVED_FROM:     return ValueState::MOVED_FROM;
				}
				evo::unreachable();
			}();

			return TermInfo(value_category, value_stage, value_state, fake_term_info.typeID, fake_term_info.expr);
		}


		EVO_NODISCARD static auto convertValueCategory(ValueCategory value_category)
		-> sema::FakeTermInfo::ValueCategory {
			switch(value_category){
				case TermInfo::ValueCategory::EPHEMERAL:      return sema::FakeTermInfo::ValueCategory::EPHEMERAL;
				case TermInfo::ValueCategory::CONCRETE_CONST: return sema::FakeTermInfo::ValueCategory::CONCRETE_CONST;
				case TermInfo::ValueCategory::CONCRETE_MUT:   return sema::FakeTermInfo::ValueCategory::CONCRETE_MUT;
				case TermInfo::ValueCategory::FORWARDABLE:    return sema::FakeTermInfo::ValueCategory::FORWARDABLE;
				default: evo::debugFatalBreak("Invalid value category");
			}
		}
		EVO_NODISCARD static auto convertValueCategory(sema::FakeTermInfo::ValueCategory value_category)
		-> ValueCategory {
			switch(value_category){
				case sema::FakeTermInfo::ValueCategory::EPHEMERAL:      return TermInfo::ValueCategory::EPHEMERAL;
				case sema::FakeTermInfo::ValueCategory::CONCRETE_CONST: return TermInfo::ValueCategory::CONCRETE_CONST;
				case sema::FakeTermInfo::ValueCategory::CONCRETE_MUT:   return TermInfo::ValueCategory::CONCRETE_MUT;
				case sema::FakeTermInfo::ValueCategory::FORWARDABLE:    return TermInfo::ValueCategory::FORWARDABLE;
				default: evo::debugFatalBreak("Invalid value category");
			}
		}


		EVO_NODISCARD static auto convertValueStage(ValueStage value_stage) -> sema::FakeTermInfo::ValueStage {
			switch(value_stage){
				case TermInfo::ValueStage::NOT_APPLICABLE: evo::debugFatalBreak("Invalid value stage to convert");
				case TermInfo::ValueStage::CONSTEXPR:      return sema::FakeTermInfo::ValueStage::CONSTEXPR;
				case TermInfo::ValueStage::COMPTIME:       return sema::FakeTermInfo::ValueStage::COMPTIME;
				case TermInfo::ValueStage::RUNTIME:        return sema::FakeTermInfo::ValueStage::RUNTIME;
			}
			evo::unreachable();
		}
		EVO_NODISCARD static auto convertValueStage(sema::FakeTermInfo::ValueStage value_stage) -> ValueStage {
			switch(value_stage){
				case sema::FakeTermInfo::ValueStage::CONSTEXPR: return TermInfo::ValueStage::CONSTEXPR;
				case sema::FakeTermInfo::ValueStage::COMPTIME:  return TermInfo::ValueStage::COMPTIME;
				case sema::FakeTermInfo::ValueStage::RUNTIME:   return TermInfo::ValueStage::RUNTIME;
			}
			evo::unreachable();
		}


		EVO_NODISCARD static auto convertValueState(ValueState value_state) -> sema::FakeTermInfo::ValueState {
			switch(value_state){
				case TermInfo::ValueState::NOT_APPLICABLE: return sema::FakeTermInfo::ValueState::NOT_APPLICABLE;
				case TermInfo::ValueState::INIT:           return sema::FakeTermInfo::ValueState::INIT;
				case TermInfo::ValueState::INITIALIZING:   return sema::FakeTermInfo::ValueState::INITIALIZING;
				case TermInfo::ValueState::UNINIT:         return sema::FakeTermInfo::ValueState::UNINIT;
				case TermInfo::ValueState::MOVED_FROM:     return sema::FakeTermInfo::ValueState::MOVED_FROM;
			}
			evo::unreachable();
		}
		EVO_NODISCARD static auto convertValueState(sema::FakeTermInfo::ValueState value_state) -> ValueState {
			switch(value_state){
				case sema::FakeTermInfo::ValueState::NOT_APPLICABLE: return TermInfo::ValueState::NOT_APPLICABLE;
				case sema::FakeTermInfo::ValueState::INIT:           return TermInfo::ValueState::INIT;
				case sema::FakeTermInfo::ValueState::INITIALIZING:   return TermInfo::ValueState::INITIALIZING;
				case sema::FakeTermInfo::ValueState::UNINIT:         return TermInfo::ValueState::UNINIT;
				case sema::FakeTermInfo::ValueState::MOVED_FROM:     return TermInfo::ValueState::MOVED_FROM;
			}
			evo::unreachable();
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
				|| this->value_category == ValueCategory::FORWARDABLE;
		}

		EVO_NODISCARD constexpr auto is_const() const -> bool {
			return this->value_category == ValueCategory::CONCRETE_CONST 
				|| this->value_category == ValueCategory::FUNCTION
				|| this->value_category == ValueCategory::FUNCTION_PUB_REQUIRED
				|| this->value_category == ValueCategory::FUNCTION_NOT_PRIV_REQUIRED;
		}


		EVO_NODISCARD constexpr auto is_mutable() const -> bool {
			return isValueCategoryMutable(this->value_category);
		}


		EVO_NODISCARD constexpr auto is_module() const -> bool {
			return this->value_category == ValueCategory::MODULE
				|| this->value_category == ValueCategory::CLANG_MODULE
				|| this->value_category == ValueCategory::BUILTIN_MODULE;
		}



		EVO_NODISCARD static constexpr auto isValueCategoryMutable(ValueCategory value_category) -> bool {
			return value_category == ValueCategory::EPHEMERAL
				|| value_category == ValueCategory::EPHEMERAL_FLUID
			    || value_category == ValueCategory::CONCRETE_MUT
				|| value_category == ValueCategory::FORWARDABLE;
		}


		///////////////////////////////////
		// value

		EVO_NODISCARD auto isSingleValue() const -> bool {
			return this->type_id.is<TypeInfo::ID>()
				|| this->type_id.is<FluidType>()
				|| this->type_id.is<InitializerType>()
				|| this->type_id.is<NullType>()
				|| this->value_category == ValueCategory::METHOD_CALL
				|| this->value_category == ValueCategory::BUILTIN_TYPE_METHOD
				|| this->value_category == ValueCategory::INTERFACE_CALL
				|| this->value_category == ValueCategory::POLY_INTERFACE_CALL;
		}

		EVO_NODISCARD auto isSingleNormalValue() const -> bool {
			return this->type_id.is<TypeInfo::ID>() || this->type_id.is<FluidType>();
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
		// except param pack

		EVO_NODISCARD auto isExceptParamPack() const -> bool {
			return this->type_id.is<ExceptParamPack>();
		}

		EVO_NODISCARD auto getExceptParamPack() const -> const evo::SmallVector<sema::Expr>& {
			evo::debugAssert(this->isExceptParamPack(), "does not hold except param pack");
			return this->exprs;
		}


		//////////////////
		// variadic param

		EVO_NODISCARD auto isVariadicParam() const -> bool {
			return this->type_id.is<VariadicParamTypes>();
		}

		EVO_NODISCARD auto getVariadicParam() const -> const sema::Expr& {
			evo::debugAssert(this->isVariadicParam(), "does not hold variadic param");
			return this->exprs[0];
		}


		private:
			evo::SmallVector<sema::Expr> exprs;
	};
	


}