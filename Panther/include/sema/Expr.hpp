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

#include "../tokens/Token.hpp"
#include "./sema_ids.hpp"
#include "../intrinsics.hpp"


namespace pcit::panther{
	class SemanticAnalyzer;
}


namespace pcit::panther::sema{


	struct Expr{
		enum class Kind : uint32_t {
			NONE, // only use for optional

			MODULE_IDENT,

			NULL_VALUE,
			UNINIT,
			ZEROINIT,

			INT_VALUE,
			FLOAT_VALUE,
			BOOL_VALUE,
			STRING_VALUE,
			AGGREGATE_VALUE,
			CHAR_VALUE,

			INTRINSIC_FUNC,
			TEMPLATED_INTRINSIC_FUNC_INSTANTIATION,

			COPY,
			MOVE,
			FORWARD,
			FUNC_CALL,
			ADDR_OF,
			CONVERSION_TO_OPTIONAL,
			OPTIONAL_NULL_CHECK,
			OPTIONAL_EXTRACT,
			DEREF,
			UNWRAP,
			ACCESSOR,
			UNION_ACCESSOR,
			LOGICAL_AND,
			LOGICAL_OR,
			TRY_ELSE_EXPR,
			TRY_ELSE_INTERFACE_EXPR,
			BLOCK_EXPR,
			FAKE_TERM_INFO,
			MAKE_INTERFACE_PTR,
			INTERFACE_PTR_EXTRACT_THIS,
			INTERFACE_CALL,
			INDEXER,
			DEFAULT_NEW,
			INIT_ARRAY_REF,
			ARRAY_REF_INDEXER,
			ARRAY_REF_SIZE,
			ARRAY_REF_DIMENSIONS,
			ARRAY_REF_DATA,
			UNION_DESIGNATED_INIT_NEW,
			UNION_TAG_CMP,
			SAME_TYPE_CMP,
				
			PARAM,
			VARIADIC_PARAM,
			RETURN_PARAM,
			ERROR_RETURN_PARAM,
			BLOCK_EXPR_OUTPUT,
			EXCEPT_PARAM,
			FOR_PARAM,

			VAR,
			GLOBAL_VAR,
			FUNC,
		};

		[[nodiscard]] static auto createModuleIdent(Token::ID id) -> Expr {
			return Expr(Kind::MODULE_IDENT, id);
		}

		explicit Expr(NullID id)               : _kind(Kind::NULL_VALUE),           value{.null = id}                {};
		explicit Expr(UninitID id)             : _kind(Kind::UNINIT),               value{.uninit = id}              {};
		explicit Expr(ZeroinitID id)           : _kind(Kind::ZEROINIT),             value{.zeroinit = id}            {};

		explicit Expr(IntValueID id)           : _kind(Kind::INT_VALUE),            value{.int_value = id}           {};
		explicit Expr(FloatValueID id)         : _kind(Kind::FLOAT_VALUE),          value{.float_value = id}         {};
		explicit Expr(BoolValueID id)          : _kind(Kind::BOOL_VALUE),           value{.bool_value = id}          {};
		explicit Expr(StringValueID id)        : _kind(Kind::STRING_VALUE),         value{.string_value = id}        {};
		explicit Expr(AggregateValueID id)     : _kind(Kind::AGGREGATE_VALUE),      value{.aggregate_value = id}     {};
		explicit Expr(CharValueID id)          : _kind(Kind::CHAR_VALUE),           value{.char_value = id}          {};

		explicit Expr(IntrinsicFunc::Kind intrinsic_func_kind) :
			_kind(Kind::INTRINSIC_FUNC), value{.intrinsic_func = intrinsic_func_kind} {};
		explicit Expr(TemplateIntrinsicFuncInstantiationID id)
			: _kind(Kind::TEMPLATED_INTRINSIC_FUNC_INSTANTIATION), 
			  value{.templated_intrinsic_func_instantiation = id} 
			  {};

		explicit Expr(CopyID id)               : _kind(Kind::COPY),                 value{.copy = id}                {};
		explicit Expr(MoveID id)               : _kind(Kind::MOVE),                 value{.move = id}                {};
		explicit Expr(ForwardID id)            : _kind(Kind::FORWARD),              value{.forward = id}             {};
		explicit Expr(FuncCallID id)           : _kind(Kind::FUNC_CALL),            value{.func_call = id}           {};
		explicit Expr(AddrOfID id)             : _kind(Kind::ADDR_OF),              value{.addr_of = id}             {};
		explicit Expr(sema::ConversionToOptionalID id) :
			 _kind(Kind::CONVERSION_TO_OPTIONAL), value{.conversion_to_optional = id} {};
		explicit Expr(OptionalNullCheckID id)  : _kind(Kind::OPTIONAL_NULL_CHECK),  value{.optional_null_check = id} {};
		explicit Expr(OptionalExtractID id)    : _kind(Kind::OPTIONAL_EXTRACT),     value{.optional_extract = id}    {};
		explicit Expr(DerefID id)              : _kind(Kind::DEREF),                value{.deref = id}               {};
		explicit Expr(UnwrapID id)             : _kind(Kind::UNWRAP),               value{.unwrap = id}              {};
		explicit Expr(AccessorID id)           : _kind(Kind::ACCESSOR),             value{.accessor = id}            {};
		explicit Expr(UnionAccessorID id)      : _kind(Kind::UNION_ACCESSOR),       value{.union_accessor = id}      {};
		explicit Expr(LogicalAndID id)         : _kind(Kind::LOGICAL_AND),          value{.logical_and = id}         {};
		explicit Expr(LogicalOrID id)          : _kind(Kind::LOGICAL_OR),           value{.logical_or = id}          {};
		explicit Expr(TryElseExprID id)        : _kind(Kind::TRY_ELSE_EXPR),        value{.try_else_expr = id}       {};
		explicit Expr(TryElseInterfaceExprID id)
			: _kind(Kind::TRY_ELSE_INTERFACE_EXPR), value{.try_else_interface_expr = id} {};
		explicit Expr(BlockExprID id)          : _kind(Kind::BLOCK_EXPR),           value{.block_expr = id}          {};
		explicit Expr(FakeTermInfoID id)       : _kind(Kind::FAKE_TERM_INFO),       value{.fake_term_info = id}      {};
		explicit Expr(MakeInterfacePtrID id)   : _kind(Kind::MAKE_INTERFACE_PTR),   value{.make_interface_ptr = id}  {};
		explicit Expr(InterfacePtrExtractThisID id)
			: _kind(Kind::INTERFACE_PTR_EXTRACT_THIS), value{.interface_ptr_extract_this = id} {};
		explicit Expr(InterfaceCallID id)      : _kind(Kind::INTERFACE_CALL),       value{.interface_call = id}      {};
		explicit Expr(IndexerID id)            : _kind(Kind::INDEXER),              value{.indexer = id}             {};
		explicit Expr(DefaultNewID id)         : _kind(Kind::DEFAULT_NEW),          value{.default_new = id}         {};
		explicit Expr(InitArrayRefID id)       : _kind(Kind::INIT_ARRAY_REF),       value{.init_array_ref = id}      {};
		explicit Expr(ArrayRefIndexerID id)    : _kind(Kind::ARRAY_REF_INDEXER),    value{.array_ref_indexer = id}   {};
		explicit Expr(ArrayRefSizeID id)       : _kind(Kind::ARRAY_REF_SIZE),       value{.array_ref_size = id}      {};
		explicit Expr(ArrayRefDimensionsID id) : _kind(Kind::ARRAY_REF_DIMENSIONS), value{.array_ref_dimensions = id}{};
		explicit Expr(ArrayRefDataID id)       : _kind(Kind::ARRAY_REF_DATA),       value{.array_ref_data = id}      {};
		explicit Expr(UnionDesignatedInitNewID id)
			: _kind(Kind::UNION_DESIGNATED_INIT_NEW), value{.union_designated_init_new = id} {};
		explicit Expr(UnionTagCmpID id)        : _kind(Kind::UNION_TAG_CMP),        value{.union_tag_cmp = id}       {};
		explicit Expr(SameTypeCmpID id)        : _kind(Kind::SAME_TYPE_CMP),        value{.same_type_cmp = id}       {};


		explicit Expr(ParamID id)              : _kind(Kind::PARAM),                value{.param = id}               {};
		explicit Expr(ReturnParamID id)        : _kind(Kind::RETURN_PARAM),         value{.return_param = id}        {};
		explicit Expr(VariadicParamID id)      : _kind(Kind::VARIADIC_PARAM),       value{.variadic_param = id}      {};
		explicit Expr(ErrorReturnParamID id)   : _kind(Kind::ERROR_RETURN_PARAM),   value{.error_return_param = id}  {};
		explicit Expr(BlockExprOutputID id)    : _kind(Kind::BLOCK_EXPR_OUTPUT),    value{.block_expr_output = id}   {};
		explicit Expr(ExceptParamID id)        : _kind(Kind::EXCEPT_PARAM),         value{.except_param = id}        {};
		explicit Expr(ForParamID id)           : _kind(Kind::FOR_PARAM),            value{.for_param = id}           {};

		explicit Expr(VarID id)                : _kind(Kind::VAR),                  value{.var = id}                 {};
		explicit Expr(GlobalVarID id)          : _kind(Kind::GLOBAL_VAR),           value{.global_var = id}          {};
		explicit Expr(FuncID id)               : _kind(Kind::FUNC),                 value{.func = id}                {};


		[[nodiscard]] constexpr auto kind() const -> Kind { return this->_kind; }


		[[nodiscard]] auto moduleIdent() const -> Token::ID {
			evo::debugAssert(this->kind() == Kind::MODULE_IDENT, "not a MODULE_IDENT");
			return this->value.token;
		}

		[[nodiscard]] auto nullID() const -> NullID {
			evo::debugAssert(this->kind() == Kind::NULL_VALUE, "not a Null");
			return this->value.null;
		}

		[[nodiscard]] auto uninitID() const -> UninitID {
			evo::debugAssert(this->kind() == Kind::UNINIT, "not a Uninit");
			return this->value.uninit;
		}

		[[nodiscard]] auto zeroinitID() const -> ZeroinitID {
			evo::debugAssert(this->kind() == Kind::ZEROINIT, "not a Zeroinit");
			return this->value.zeroinit;
		}

		[[nodiscard]] auto intValueID() const -> IntValueID {
			evo::debugAssert(this->kind() == Kind::INT_VALUE, "not a IntValue");
			return this->value.int_value;
		}
		[[nodiscard]] auto floatValueID() const -> FloatValueID {
			evo::debugAssert(this->kind() == Kind::FLOAT_VALUE, "not a FloatValue");
			return this->value.float_value;
		}
		[[nodiscard]] auto boolValueID() const -> BoolValueID {
			evo::debugAssert(this->kind() == Kind::BOOL_VALUE, "not a BoolValue");
			return this->value.bool_value;
		}
		[[nodiscard]] auto stringValueID() const -> StringValueID {
			evo::debugAssert(this->kind() == Kind::STRING_VALUE, "not an StringValue");
			return this->value.string_value;
		}
		[[nodiscard]] auto aggregateValueID() const -> AggregateValueID {
			evo::debugAssert(this->kind() == Kind::AGGREGATE_VALUE, "not an AggregateValue");
			return this->value.aggregate_value;
		}
		[[nodiscard]] auto charValueID() const -> CharValueID {
			evo::debugAssert(this->kind() == Kind::CHAR_VALUE, "not a CharValue");
			return this->value.char_value;
		}

		[[nodiscard]] auto intrinsicFuncID() const -> IntrinsicFunc::Kind {
			evo::debugAssert(this->kind() == Kind::INTRINSIC_FUNC, "not an IntrinsicFunc");
			return this->value.intrinsic_func;
		}
		[[nodiscard]] auto templatedIntrinsicInstantiationID() const -> TemplateIntrinsicFuncInstantiationID {
			evo::debugAssert(
				this->kind() == Kind::TEMPLATED_INTRINSIC_FUNC_INSTANTIATION,
				"not an TEMPLATED_INTRINSIC_FUNC_INSTANTIATION"
			);
			return this->value.templated_intrinsic_func_instantiation;
		}

		[[nodiscard]] auto copyID() const -> CopyID {
			evo::debugAssert(this->kind() == Kind::COPY, "not a copy");
			return this->value.copy;
		}
		[[nodiscard]] auto moveID() const -> MoveID {
			evo::debugAssert(this->kind() == Kind::MOVE, "not a move");
			return this->value.move;
		}
		[[nodiscard]] auto forwardID() const -> ForwardID {
			evo::debugAssert(this->kind() == Kind::FORWARD, "not a forward");
			return this->value.forward;
		}
		[[nodiscard]] auto funcCallID() const -> FuncCallID {
			evo::debugAssert(this->kind() == Kind::FUNC_CALL, "not a func call");
			return this->value.func_call;
		}
		[[nodiscard]] auto addrOfID() const -> AddrOfID {
			evo::debugAssert(this->kind() == Kind::ADDR_OF, "not an addr of");
			return this->value.addr_of;
		}
		[[nodiscard]] auto conversionToOptionalID() const -> ConversionToOptionalID {
			evo::debugAssert(this->kind() == Kind::CONVERSION_TO_OPTIONAL, "not an conversion to optional");
			return this->value.conversion_to_optional;
		}
		[[nodiscard]] auto optionalNullCheckID() const -> OptionalNullCheckID {
			evo::debugAssert(this->kind() == Kind::OPTIONAL_NULL_CHECK, "not an optional null check");
			return this->value.optional_null_check;
		}
		[[nodiscard]] auto optionalExtractID() const -> OptionalExtractID {
			evo::debugAssert(this->kind() == Kind::OPTIONAL_EXTRACT, "not an optional extract");
			return this->value.optional_extract;
		}
		[[nodiscard]] auto derefID() const -> DerefID {
			evo::debugAssert(this->kind() == Kind::DEREF, "not a deref");
			return this->value.deref;
		}
		[[nodiscard]] auto unwrapID() const -> UnwrapID {
			evo::debugAssert(this->kind() == Kind::UNWRAP, "not an unwrap");
			return this->value.unwrap;
		}
		[[nodiscard]] auto accessorID() const -> AccessorID {
			evo::debugAssert(this->kind() == Kind::ACCESSOR, "not an accessor");
			return this->value.accessor;
		}
		[[nodiscard]] auto unionAccessorID() const -> UnionAccessorID {
			evo::debugAssert(this->kind() == Kind::UNION_ACCESSOR, "not a union accessor");
			return this->value.union_accessor;
		}
		[[nodiscard]] auto logicalAndID() const -> LogicalAndID {
			evo::debugAssert(this->kind() == Kind::LOGICAL_AND, "not a logical and");
			return this->value.logical_and;
		}
		[[nodiscard]] auto logicalOrID() const -> LogicalOrID {
			evo::debugAssert(this->kind() == Kind::LOGICAL_OR, "not a logical or");
			return this->value.logical_or;
		}
		[[nodiscard]] auto tryElseExprID() const -> TryElseExprID {
			evo::debugAssert(this->kind() == Kind::TRY_ELSE_EXPR, "not a try/else expr");
			return this->value.try_else_expr;
		}
		[[nodiscard]] auto tryElseInterfaceExprID() const -> TryElseInterfaceExprID {
			evo::debugAssert(this->kind() == Kind::TRY_ELSE_INTERFACE_EXPR, "not a try/else interface expr");
			return this->value.try_else_interface_expr;
		}
		[[nodiscard]] auto blockExprID() const -> BlockExprID {
			evo::debugAssert(this->kind() == Kind::BLOCK_EXPR, "not a block expr");
			return this->value.block_expr;
		}
		[[nodiscard]] auto fakeTermInfoID() const -> FakeTermInfoID {
			evo::debugAssert(this->kind() == Kind::FAKE_TERM_INFO, "not a fake term info");
			return this->value.fake_term_info;
		}
		[[nodiscard]] auto makeInterfacePtrID() const -> MakeInterfacePtrID {
			evo::debugAssert(this->kind() == Kind::MAKE_INTERFACE_PTR, "not a make interface ptr");
			return this->value.make_interface_ptr;
		}
		[[nodiscard]] auto interfacePtrExtractThisID() const -> InterfacePtrExtractThisID {
			evo::debugAssert(this->kind() == Kind::INTERFACE_PTR_EXTRACT_THIS, "not a interface ptr extract this");
			return this->value.interface_ptr_extract_this;
		}
		[[nodiscard]] auto interfaceCallID() const -> InterfaceCallID {
			evo::debugAssert(this->kind() == Kind::INTERFACE_CALL, "not an interface call");
			return this->value.interface_call;
		}
		[[nodiscard]] auto indexerID() const -> IndexerID {
			evo::debugAssert(this->kind() == Kind::INDEXER, "not an indexer");
			return this->value.indexer;
		}
		[[nodiscard]] auto defaultNewID() const -> DefaultNewID {
			evo::debugAssert(this->kind() == Kind::DEFAULT_NEW, "not a default new");
			return this->value.default_new;
		}
		[[nodiscard]] auto initArrayRefID() const -> InitArrayRefID {
			evo::debugAssert(this->kind() == Kind::INIT_ARRAY_REF, "not an init array ref");
			return this->value.init_array_ref;
		}
		[[nodiscard]] auto arrayRefIndexerID() const -> ArrayRefIndexerID {
			evo::debugAssert(this->kind() == Kind::ARRAY_REF_INDEXER, "not an array ref indexer");
			return this->value.array_ref_indexer;
		}
		[[nodiscard]] auto arrayRefSizeID() const -> ArrayRefSizeID {
			evo::debugAssert(this->kind() == Kind::ARRAY_REF_SIZE, "not an array ref size");
			return this->value.array_ref_size;
		}
		[[nodiscard]] auto arrayRefDimensionsID() const -> ArrayRefDimensionsID {
			evo::debugAssert(this->kind() == Kind::ARRAY_REF_DIMENSIONS, "not an array ref dimensions");
			return this->value.array_ref_dimensions;
		}
		[[nodiscard]] auto arrayRefDataID() const -> ArrayRefDataID {
			evo::debugAssert(this->kind() == Kind::ARRAY_REF_DATA, "not an array ref data");
			return this->value.array_ref_data;
		}
		[[nodiscard]] auto unionDesignatedInitNewID() const -> UnionDesignatedInitNewID {
			evo::debugAssert(this->kind() == Kind::UNION_DESIGNATED_INIT_NEW, "not a union designated init new");
			return this->value.union_designated_init_new;
		}
		[[nodiscard]] auto unionTagCmpID() const -> UnionTagCmpID {
			evo::debugAssert(this->kind() == Kind::UNION_TAG_CMP, "not a union tag cmp");
			return this->value.union_tag_cmp;
		}
		[[nodiscard]] auto sameTypeCmpID() const -> SameTypeCmpID {
			evo::debugAssert(this->kind() == Kind::SAME_TYPE_CMP, "not a same type cmp");
			return this->value.same_type_cmp;
		}


		[[nodiscard]] auto paramID() const -> ParamID {
			evo::debugAssert(this->kind() == Kind::PARAM, "not a param");
			return this->value.param;
		}
		[[nodiscard]] auto variadicParamID() const -> VariadicParamID {
			evo::debugAssert(this->kind() == Kind::VARIADIC_PARAM, "not a variadic param");
			return this->value.variadic_param;
		}
		[[nodiscard]] auto returnParamID() const -> ReturnParamID {
			evo::debugAssert(this->kind() == Kind::RETURN_PARAM, "not a return param");
			return this->value.return_param;
		}
		[[nodiscard]] auto errorReturnParamID() const -> ErrorReturnParamID {
			evo::debugAssert(this->kind() == Kind::ERROR_RETURN_PARAM, "not an error return param");
			return this->value.error_return_param;
		}
		[[nodiscard]] auto blockExprOutputID() const -> BlockExprOutputID {
			evo::debugAssert(this->kind() == Kind::BLOCK_EXPR_OUTPUT, "not a block expr output");
			return this->value.block_expr_output;
		}
		[[nodiscard]] auto exceptParamID() const -> ExceptParamID {
			evo::debugAssert(this->kind() == Kind::EXCEPT_PARAM, "not an except param");
			return this->value.except_param;
		}
		[[nodiscard]] auto forParamID() const -> ForParamID {
			evo::debugAssert(this->kind() == Kind::FOR_PARAM, "not a for param");
			return this->value.for_param;
		}

		[[nodiscard]] auto varID() const -> VarID {
			evo::debugAssert(this->kind() == Kind::VAR, "not a var");
			return this->value.var;
		}
		[[nodiscard]] auto globalVarID() const -> GlobalVarID {
			evo::debugAssert(this->kind() == Kind::GLOBAL_VAR, "not a global var");
			return this->value.global_var;
		}
		[[nodiscard]] auto funcID() const -> FuncID {
			evo::debugAssert(this->kind() == Kind::FUNC, "not a func");
			return this->value.func;
		}


		[[nodiscard]] auto operator==(const Expr& rhs) const -> bool {
			return evo::bitCast<uint64_t>(*this) == evo::bitCast<uint64_t>(rhs);
		}


		private:
			Expr(Kind k, Token::ID token) : _kind(k), value{.token = token} {}

			[[nodiscard]] static auto createNone() -> Expr { return Expr(Kind::NONE, Token::ID(0)); }

		private:
			Kind _kind;

			union {
				Token::ID token;

				NullID null;
				UninitID uninit;
				ZeroinitID zeroinit;

				IntValueID int_value;
				FloatValueID float_value;
				BoolValueID bool_value;
				StringValueID string_value;
				AggregateValueID aggregate_value;
				CharValueID char_value;

				IntrinsicFunc::Kind intrinsic_func;
				TemplateIntrinsicFuncInstantiationID templated_intrinsic_func_instantiation;

				CopyID copy;
				MoveID move;
				ForwardID forward;
				FuncCallID func_call;
				AddrOfID addr_of;
				ConversionToOptionalID conversion_to_optional;
				OptionalNullCheckID optional_null_check;
				OptionalExtractID optional_extract;
				DerefID deref;
				UnwrapID unwrap;
				AccessorID accessor;
				UnionAccessorID union_accessor;
				LogicalAndID logical_and;
				LogicalOrID logical_or;
				TryElseExprID try_else_expr;
				TryElseInterfaceExprID try_else_interface_expr;
				BlockExprID block_expr;
				FakeTermInfoID fake_term_info;
				MakeInterfacePtrID make_interface_ptr;
				InterfacePtrExtractThisID interface_ptr_extract_this;
				InterfaceCallID interface_call;
				IndexerID indexer;
				DefaultNewID default_new;
				InitArrayRefID init_array_ref;
				ArrayRefIndexerID array_ref_indexer;
				ArrayRefSizeID array_ref_size;
				ArrayRefDimensionsID array_ref_dimensions;
				ArrayRefDataID array_ref_data;
				UnionDesignatedInitNewID union_designated_init_new;
				UnionTagCmpID union_tag_cmp;
				SameTypeCmpID same_type_cmp;

				ExceptParamID except_param;
				ForParamID for_param;
				ParamID param;
				VariadicParamID variadic_param;
				ReturnParamID return_param;
				ErrorReturnParamID error_return_param;
				BlockExprOutputID block_expr_output;

				VarID var;
				GlobalVarID global_var;
				FuncID func;
			} value;

			friend struct core::OptionalInterface<Expr>;
			friend SemanticAnalyzer;
	};


	static_assert(sizeof(Expr) == 8, "sema::Expr is different size than expected");
	static_assert(std::is_trivially_copyable_v<Expr>, "sema::Expr is not trivially copyable");

}




namespace pcit::core{
	
	template<>
	struct OptionalInterface<panther::sema::Expr>{
		static constexpr auto init(panther::sema::Expr* expr) -> void {
			expr->_kind = panther::sema::Expr::Kind::NONE;
		}

		static constexpr auto has_value(const panther::sema::Expr& expr) -> bool {
			return expr._kind != panther::sema::Expr::Kind::NONE;
		}
	};

}



namespace std{
	
	template<>
	class optional<pcit::panther::sema::Expr> : public pcit::core::Optional<pcit::panther::sema::Expr>{
		public:
			using pcit::core::Optional<pcit::panther::sema::Expr>::Optional;
			using pcit::core::Optional<pcit::panther::sema::Expr>::operator=;
	};


	template<>
	struct hash<pcit::panther::sema::Expr>{
		auto operator()(const pcit::panther::sema::Expr& expr) const noexcept -> size_t {
			return hash<uint64_t>{}(*(uint64_t*)&expr);
		};
	};


}
