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

#include "../../include/source/source_data.hpp"
#include "../../include/TypeManager.hpp"
#include "../../include/intrinsics.hpp"
#include "../../include/sema/sema.hpp"


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
			sema::StructTemplateAlias::ID,  // TEMPLATE_TYPE|TEMPLATE_TYPE_PUB_REQUIRED
			TemplateIntrinsicFunc::Kind,    // TEMPLATE_INTRINSIC_FUNC
			TaggedUnionFieldAccessor,       // TAGGED_UNION_FIELD_ACCESSOR
			VariadicParamTypes              // VARIADIC_PARAM
		>;

		ValueCategory value_category;
		ValueState value_state;
		TypeID type_id;
		bool isComptime;


		///////////////////////////////////
		// constructors

		TermInfo(
			ValueCategory cat, bool is_comptime, evo::SmallVector<TypeInfo::ID>&& _type_id, const sema::Expr& _expr
		) :
			value_category(cat),
			value_state(ValueState::NOT_APPLICABLE),
			type_id(InitializerType()),
			isComptime(is_comptime),
			exprs{_expr}
		{
			evo::debugAssert(this->value_category == ValueCategory::EPHEMERAL);

			// This is to get around the MSVC bug
			this->type_id.emplace<evo::SmallVector<TypeInfo::ID>>(std::move(_type_id));

			#if defined(PCIT_CONFIG_DEBUG)
				this->check_single_expr_construction();
			#endif
		}

		TermInfo(ValueCategory cat, bool is_comptime, ValueState state, auto&& _type_id, const sema::Expr& _expr) :
			value_category(cat),
			value_state(state),
			type_id(std::forward<decltype(_type_id)>(_type_id)),
			isComptime(is_comptime),
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

		TermInfo(ValueCategory cat, bool is_comptime, ValueState state, const auto& _type_id, const sema::Expr& _expr)
			: value_category(cat), value_state(state), type_id(_type_id), isComptime(is_comptime), exprs{_expr} {
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
			value_state(ValueState::NOT_APPLICABLE),
			type_id(std::forward<decltype(_type_id)>(_type_id)),
			isComptime(true),
			exprs()
		{
			#if defined(PCIT_CONFIG_DEBUG)
				this->check_no_expr_construction();
			#endif
		}


		TermInfo(ValueCategory cat, const auto& _type_id) :
			value_category(cat),
			value_state(ValueState::NOT_APPLICABLE),
			type_id(InitializerType()),
			isComptime(true),
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
			bool is_comptime,
			evo::SmallVector<TypeInfo::ID>&& type_ids,
			evo::SmallVector<sema::Expr>&& expr_list
		) : 
			value_category(cat),
			value_state(ValueState::NOT_APPLICABLE),
			type_id(InitializerType{}),
			isComptime(is_comptime),
			exprs(std::move(expr_list))
		{
			// TODO(FUTURE): remove this and move directly into `type_id` when the MSVC bug is fixed
			this->type_id.emplace<evo::SmallVector<TypeInfo::ID>>(std::move(type_ids));

			evo::debugAssert(this->value_category == ValueCategory::EPHEMERAL, "multi-expr must be multi-return");
		}


		TermInfo(ValueCategory cat, bool is_comptime, ExceptParamPack, evo::SmallVector<sema::Expr>&& expr_list) :
			value_category(cat),
			value_state(ValueState::NOT_APPLICABLE),
			type_id(ExceptParamPack{}),
			isComptime(is_comptime),
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
						evo::debugAssert(
							this->type_id.is<sema::TemplatedStruct::ID>()
								|| this->type_id.is<sema::StructTemplateAlias::ID>(),
							"Incorrect TermInfo creation"
						);

					break; case ValueCategory::TEMPLATE_TYPE_PUB_REQUIRED:
						evo::debugAssert(
							this->type_id.is<sema::TemplatedStruct::ID>()
								|| this->type_id.is<sema::StructTemplateAlias::ID>(),
							"Incorrect TermInfo creation"
						);

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


		[[nodiscard]] static auto fromFakeTermInfo(const sema::FakeTermInfo& fake_term_info) -> TermInfo {
			const ValueCategory value_category = convertValueCategory(fake_term_info.valueCategory);
			const ValueState value_state = convertValueState(fake_term_info.valueState);

			return TermInfo(
				value_category, fake_term_info.isComptime, value_state, fake_term_info.typeID, fake_term_info.expr
			);
		}


		[[nodiscard]] static auto convertValueCategory(ValueCategory value_category)
		-> sema::FakeTermInfo::ValueCategory {
			switch(value_category){
				case TermInfo::ValueCategory::EPHEMERAL:      return sema::FakeTermInfo::ValueCategory::EPHEMERAL;
				case TermInfo::ValueCategory::CONCRETE_CONST: return sema::FakeTermInfo::ValueCategory::CONCRETE_CONST;
				case TermInfo::ValueCategory::CONCRETE_MUT:   return sema::FakeTermInfo::ValueCategory::CONCRETE_MUT;
				case TermInfo::ValueCategory::FORWARDABLE:    return sema::FakeTermInfo::ValueCategory::FORWARDABLE;
				default: evo::debugFatalBreak("Invalid value category");
			}
		}
		[[nodiscard]] static auto convertValueCategory(sema::FakeTermInfo::ValueCategory value_category)
		-> ValueCategory {
			switch(value_category){
				case sema::FakeTermInfo::ValueCategory::EPHEMERAL:      return TermInfo::ValueCategory::EPHEMERAL;
				case sema::FakeTermInfo::ValueCategory::CONCRETE_CONST: return TermInfo::ValueCategory::CONCRETE_CONST;
				case sema::FakeTermInfo::ValueCategory::CONCRETE_MUT:   return TermInfo::ValueCategory::CONCRETE_MUT;
				case sema::FakeTermInfo::ValueCategory::FORWARDABLE:    return TermInfo::ValueCategory::FORWARDABLE;
				default: evo::debugFatalBreak("Invalid value category");
			}
		}


		[[nodiscard]] static auto convertValueState(ValueState value_state) -> sema::FakeTermInfo::ValueState {
			switch(value_state){
				case TermInfo::ValueState::NOT_APPLICABLE: return sema::FakeTermInfo::ValueState::NOT_APPLICABLE;
				case TermInfo::ValueState::INIT:           return sema::FakeTermInfo::ValueState::INIT;
				case TermInfo::ValueState::INITIALIZING:   return sema::FakeTermInfo::ValueState::INITIALIZING;
				case TermInfo::ValueState::UNINIT:         return sema::FakeTermInfo::ValueState::UNINIT;
				case TermInfo::ValueState::MOVED_FROM:     return sema::FakeTermInfo::ValueState::MOVED_FROM;
			}
			evo::unreachable();
		}
		[[nodiscard]] static auto convertValueState(sema::FakeTermInfo::ValueState value_state) -> ValueState {
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

		[[nodiscard]] constexpr auto is_ephemeral() const -> bool {
			return this->value_category == ValueCategory::EPHEMERAL
				|| this->value_category == ValueCategory::EPHEMERAL_FLUID;
		}

		[[nodiscard]] constexpr auto is_concrete() const -> bool {
			return this->value_category == ValueCategory::CONCRETE_CONST
				|| this->value_category == ValueCategory::CONCRETE_MUT
				|| this->value_category == ValueCategory::FORWARDABLE;
		}

		[[nodiscard]] constexpr auto is_const() const -> bool {
			return this->value_category == ValueCategory::CONCRETE_CONST 
				|| this->value_category == ValueCategory::FUNCTION
				|| this->value_category == ValueCategory::FUNCTION_PUB_REQUIRED
				|| this->value_category == ValueCategory::FUNCTION_NOT_PRIV_REQUIRED;
		}


		[[nodiscard]] constexpr auto is_mutable() const -> bool {
			return isValueCategoryMutable(this->value_category);
		}


		[[nodiscard]] constexpr auto is_module() const -> bool {
			return this->value_category == ValueCategory::MODULE
				|| this->value_category == ValueCategory::CLANG_MODULE
				|| this->value_category == ValueCategory::BUILTIN_MODULE;
		}



		[[nodiscard]] static constexpr auto isValueCategoryMutable(ValueCategory value_category) -> bool {
			return value_category == ValueCategory::EPHEMERAL
				|| value_category == ValueCategory::EPHEMERAL_FLUID
			    || value_category == ValueCategory::CONCRETE_MUT
				|| value_category == ValueCategory::FORWARDABLE;
		}


		///////////////////////////////////
		// value state checking

		[[nodiscard]] constexpr auto isInitialized() const -> bool {
			return this->value_state == ValueState::NOT_APPLICABLE
				|| this->value_state == ValueState::INIT;
		}

		[[nodiscard]] constexpr auto isUninitialized() const -> bool {
			return !this->isInitialized();
		}



		///////////////////////////////////
		// value

		[[nodiscard]] auto isSingleValue() const -> bool {
			return this->type_id.is<TypeInfo::ID>()
				|| this->type_id.is<FluidType>()
				|| this->type_id.is<InitializerType>()
				|| this->type_id.is<NullType>()
				|| this->value_category == ValueCategory::METHOD_CALL
				|| this->value_category == ValueCategory::BUILTIN_TYPE_METHOD
				|| this->value_category == ValueCategory::INTERFACE_CALL
				|| this->value_category == ValueCategory::POLY_INTERFACE_CALL;
		}

		[[nodiscard]] auto isSingleNormalValue() const -> bool {
			return this->type_id.is<TypeInfo::ID>() || this->type_id.is<FluidType>();
		}

		[[nodiscard]] auto getExpr() const -> const sema::Expr& {
			evo::debugAssert(
				this->isSingleValue() || (this->isMultiValue() && this->exprs.size() == 1), "does not hold expr value"
			);
			return this->exprs[0];
		}

		[[nodiscard]] auto getExpr() -> sema::Expr& {
			evo::debugAssert(
				this->isSingleValue() || (this->isMultiValue() && this->exprs.size() == 1), "does not hold expr value"
			);
			return this->exprs[0];
		}


		//////////////////
		// module expr

		[[nodiscard]] auto isModuleExpr() const -> bool {
			return this->type_id.is<SourceID>();
		}

		[[nodiscard]] auto getModuleExpr() const -> const sema::Expr& {
			evo::debugAssert(this->isModuleExpr(), "does not hold module");
			return this->exprs[0];
		}


		//////////////////
		// multi-value

		[[nodiscard]] auto isMultiValue() const -> bool {
			return this->type_id.is<evo::SmallVector<TypeInfo::ID>>();
		}

		[[nodiscard]] auto getMultiExpr() const -> const evo::SmallVector<sema::Expr>& {
			evo::debugAssert(this->isMultiValue(), "does not hold multi expr value");
			return this->exprs;
		}


		//////////////////
		// except param pack

		[[nodiscard]] auto isExceptParamPack() const -> bool {
			return this->type_id.is<ExceptParamPack>();
		}

		[[nodiscard]] auto getExceptParamPack() const -> const evo::SmallVector<sema::Expr>& {
			evo::debugAssert(this->isExceptParamPack(), "does not hold except param pack");
			return this->exprs;
		}


		//////////////////
		// variadic param

		[[nodiscard]] auto isVariadicParam() const -> bool {
			return this->type_id.is<VariadicParamTypes>();
		}

		[[nodiscard]] auto getVariadicParam() const -> const sema::Expr& {
			evo::debugAssert(this->isVariadicParam(), "does not hold variadic param");
			return this->exprs[0];
		}


		private:
			evo::SmallVector<sema::Expr> exprs;
	};
	


}