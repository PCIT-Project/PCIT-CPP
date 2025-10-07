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

#include "./sema_ids.h"
#include "../intrinsics.h"


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
			TRY_ELSE,
			BLOCK_EXPR,
			FAKE_TERM_INFO,
			MAKE_INTERFACE_PTR,
			INTERFACE_PTR_EXTRACT_THIS,
			INTERFACE_CALL,
			INDEXER,
			DEFAULT_INIT_PRIMITIVE,
			DEFAULT_TRIVIALLY_INIT_STRUCT,
			DEFAULT_INIT_ARRAY_REF,
			INIT_ARRAY_REF,
			ARRAY_REF_INDEXER,
			ARRAY_REF_SIZE,
			ARRAY_REF_DIMENSIONS,
			UNION_DESIGNATED_INIT_NEW,
				
			PARAM,
			RETURN_PARAM,
			ERROR_RETURN_PARAM,
			BLOCK_EXPR_OUTPUT,
			EXCEPT_PARAM,

			VAR,
			GLOBAL_VAR,
			FUNC,
		};

		EVO_NODISCARD static auto createModuleIdent(Token::ID id) -> Expr {
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
		explicit Expr(TryElseID id)            : _kind(Kind::TRY_ELSE),             value{.try_else = id}            {};
		explicit Expr(BlockExprID id)          : _kind(Kind::BLOCK_EXPR),           value{.block_expr = id}          {};
		explicit Expr(FakeTermInfoID id)       : _kind(Kind::FAKE_TERM_INFO),       value{.fake_term_info = id}      {};
		explicit Expr(MakeInterfacePtrID id)   : _kind(Kind::MAKE_INTERFACE_PTR),   value{.make_interface_ptr = id}  {};
		explicit Expr(InterfacePtrExtractThisID id)
			: _kind(Kind::INTERFACE_PTR_EXTRACT_THIS), value{.interface_ptr_extract_this = id} {};
		explicit Expr(InterfaceCallID id)      : _kind(Kind::INTERFACE_CALL),       value{.interface_call = id}      {};
		explicit Expr(IndexerID id)            : _kind(Kind::INDEXER),              value{.indexer = id}             {};
		explicit Expr(DefaultInitPrimitiveID id)
			: _kind(Kind::DEFAULT_INIT_PRIMITIVE), value{.default_init_primitive = id} {};
		explicit Expr(DefaultTriviallyInitStructID id)
			: _kind(Kind::DEFAULT_TRIVIALLY_INIT_STRUCT), value{.default_trivially_init_struct = id} {};
		explicit Expr(DefaultInitArrayRefID id)
			: _kind(Kind::DEFAULT_INIT_ARRAY_REF), value{.default_init_array_ref = id} {};
		explicit Expr(InitArrayRefID id)       : _kind(Kind::INIT_ARRAY_REF),       value{.init_array_ref = id}      {};
		explicit Expr(ArrayRefIndexerID id)    : _kind(Kind::ARRAY_REF_INDEXER),    value{.array_ref_indexer = id}   {};
		explicit Expr(ArrayRefSizeID id)       : _kind(Kind::ARRAY_REF_SIZE),       value{.array_ref_size = id}      {};
		explicit Expr(ArrayRefDimensionsID id) : _kind(Kind::ARRAY_REF_DIMENSIONS), value{.array_ref_dimensions = id}{};
		explicit Expr(UnionDesignatedInitNewID id)
			: _kind(Kind::UNION_DESIGNATED_INIT_NEW), value{.union_designated_init_new = id} {};

		explicit Expr(ParamID id)              : _kind(Kind::PARAM),                value{.param = id}               {};
		explicit Expr(ReturnParamID id)        : _kind(Kind::RETURN_PARAM),         value{.return_param = id}        {};
		explicit Expr(ErrorReturnParamID id)   : _kind(Kind::ERROR_RETURN_PARAM),   value{.error_return_param = id}  {};
		explicit Expr(BlockExprOutputID id)    : _kind(Kind::BLOCK_EXPR_OUTPUT),    value{.block_expr_output = id}   {};
		explicit Expr(ExceptParamID id)        : _kind(Kind::EXCEPT_PARAM),         value{.except_param = id}        {};

		explicit Expr(VarID id)                : _kind(Kind::VAR),                  value{.var = id}                 {};
		explicit Expr(GlobalVarID id)          : _kind(Kind::GLOBAL_VAR),           value{.global_var = id}          {};
		explicit Expr(FuncID id)               : _kind(Kind::FUNC),                 value{.func = id}                {};


		EVO_NODISCARD constexpr auto kind() const -> Kind { return this->_kind; }


		EVO_NODISCARD auto moduleIdent() const -> Token::ID {
			evo::debugAssert(this->kind() == Kind::MODULE_IDENT, "not a MODULE_IDENT");
			return this->value.token;
		}

		EVO_NODISCARD auto nullID() const -> NullID {
			evo::debugAssert(this->kind() == Kind::NULL_VALUE, "not a Null");
			return this->value.null;
		}

		EVO_NODISCARD auto uninitID() const -> UninitID {
			evo::debugAssert(this->kind() == Kind::UNINIT, "not a Uninit");
			return this->value.uninit;
		}

		EVO_NODISCARD auto zeroinitID() const -> ZeroinitID {
			evo::debugAssert(this->kind() == Kind::ZEROINIT, "not a Zeroinit");
			return this->value.zeroinit;
		}

		EVO_NODISCARD auto intValueID() const -> IntValueID {
			evo::debugAssert(this->kind() == Kind::INT_VALUE, "not a IntValue");
			return this->value.int_value;
		}
		EVO_NODISCARD auto floatValueID() const -> FloatValueID {
			evo::debugAssert(this->kind() == Kind::FLOAT_VALUE, "not a FloatValue");
			return this->value.float_value;
		}
		EVO_NODISCARD auto boolValueID() const -> BoolValueID {
			evo::debugAssert(this->kind() == Kind::BOOL_VALUE, "not a BoolValue");
			return this->value.bool_value;
		}
		EVO_NODISCARD auto stringValueID() const -> StringValueID {
			evo::debugAssert(this->kind() == Kind::STRING_VALUE, "not an StringValue");
			return this->value.string_value;
		}
		EVO_NODISCARD auto aggregateValueID() const -> AggregateValueID {
			evo::debugAssert(this->kind() == Kind::AGGREGATE_VALUE, "not an AggregateValue");
			return this->value.aggregate_value;
		}
		EVO_NODISCARD auto charValueID() const -> CharValueID {
			evo::debugAssert(this->kind() == Kind::CHAR_VALUE, "not a CharValue");
			return this->value.char_value;
		}

		EVO_NODISCARD auto intrinsicFuncID() const -> IntrinsicFunc::Kind {
			evo::debugAssert(this->kind() == Kind::INTRINSIC_FUNC, "not an IntrinsicFunc");
			return this->value.intrinsic_func;
		}
		EVO_NODISCARD auto templatedIntrinsicInstantiationID() const -> TemplateIntrinsicFuncInstantiationID {
			evo::debugAssert(
				this->kind() == Kind::TEMPLATED_INTRINSIC_FUNC_INSTANTIATION,
				"not an TEMPLATED_INTRINSIC_FUNC_INSTANTIATION"
			);
			return this->value.templated_intrinsic_func_instantiation;
		}

		EVO_NODISCARD auto copyID() const -> CopyID {
			evo::debugAssert(this->kind() == Kind::COPY, "not a copy");
			return this->value.copy;
		}
		EVO_NODISCARD auto moveID() const -> MoveID {
			evo::debugAssert(this->kind() == Kind::MOVE, "not a move");
			return this->value.move;
		}
		EVO_NODISCARD auto forwardID() const -> ForwardID {
			evo::debugAssert(this->kind() == Kind::FORWARD, "not a forward");
			return this->value.forward;
		}
		EVO_NODISCARD auto funcCallID() const -> FuncCallID {
			evo::debugAssert(this->kind() == Kind::FUNC_CALL, "not a func call");
			return this->value.func_call;
		}
		EVO_NODISCARD auto addrOfID() const -> AddrOfID {
			evo::debugAssert(this->kind() == Kind::ADDR_OF, "not an addr of");
			return this->value.addr_of;
		}
		EVO_NODISCARD auto conversionToOptionalID() const -> ConversionToOptionalID {
			evo::debugAssert(this->kind() == Kind::CONVERSION_TO_OPTIONAL, "not an conversion to optional");
			return this->value.conversion_to_optional;
		}
		EVO_NODISCARD auto optionalNullCheckID() const -> OptionalNullCheckID {
			evo::debugAssert(this->kind() == Kind::OPTIONAL_NULL_CHECK, "not an optional null check");
			return this->value.optional_null_check;
		}
		EVO_NODISCARD auto optionalExtractID() const -> OptionalExtractID {
			evo::debugAssert(this->kind() == Kind::OPTIONAL_EXTRACT, "not an optional extract");
			return this->value.optional_extract;
		}
		EVO_NODISCARD auto derefID() const -> DerefID {
			evo::debugAssert(this->kind() == Kind::DEREF, "not a deref");
			return this->value.deref;
		}
		EVO_NODISCARD auto unwrapID() const -> UnwrapID {
			evo::debugAssert(this->kind() == Kind::UNWRAP, "not an unwrap");
			return this->value.unwrap;
		}
		EVO_NODISCARD auto accessorID() const -> AccessorID {
			evo::debugAssert(this->kind() == Kind::ACCESSOR, "not an accessor");
			return this->value.accessor;
		}
		EVO_NODISCARD auto unionAccessorID() const -> UnionAccessorID {
			evo::debugAssert(this->kind() == Kind::UNION_ACCESSOR, "not a union accessor");
			return this->value.union_accessor;
		}
		EVO_NODISCARD auto logicalAndID() const -> LogicalAndID {
			evo::debugAssert(this->kind() == Kind::LOGICAL_AND, "not a logical and");
			return this->value.logical_and;
		}
		EVO_NODISCARD auto logicalOrID() const -> LogicalOrID {
			evo::debugAssert(this->kind() == Kind::LOGICAL_OR, "not a logical or");
			return this->value.logical_or;
		}
		EVO_NODISCARD auto tryElseID() const -> TryElseID {
			evo::debugAssert(this->kind() == Kind::TRY_ELSE, "not a try/else");
			return this->value.try_else;
		}
		EVO_NODISCARD auto blockExprID() const -> BlockExprID {
			evo::debugAssert(this->kind() == Kind::BLOCK_EXPR, "not a block expr");
			return this->value.block_expr;
		}
		EVO_NODISCARD auto fakeTermInfoID() const -> FakeTermInfoID {
			evo::debugAssert(this->kind() == Kind::FAKE_TERM_INFO, "not a fake term info");
			return this->value.fake_term_info;
		}
		EVO_NODISCARD auto makeInterfacePtrID() const -> MakeInterfacePtrID {
			evo::debugAssert(this->kind() == Kind::MAKE_INTERFACE_PTR, "not a make interface ptr");
			return this->value.make_interface_ptr;
		}
		EVO_NODISCARD auto interfacePtrExtractThisID() const -> InterfacePtrExtractThisID {
			evo::debugAssert(this->kind() == Kind::INTERFACE_PTR_EXTRACT_THIS, "not a interface ptr extract this");
			return this->value.interface_ptr_extract_this;
		}
		EVO_NODISCARD auto interfaceCallID() const -> InterfaceCallID {
			evo::debugAssert(this->kind() == Kind::INTERFACE_CALL, "not an interface call");
			return this->value.interface_call;
		}
		EVO_NODISCARD auto indexerID() const -> IndexerID {
			evo::debugAssert(this->kind() == Kind::INDEXER, "not an indexer");
			return this->value.indexer;
		}
		EVO_NODISCARD auto defaultInitPrimitiveID() const -> DefaultInitPrimitiveID {
			evo::debugAssert(this->kind() == Kind::DEFAULT_INIT_PRIMITIVE, "not a default init primitive");
			return this->value.default_init_primitive;
		}
		EVO_NODISCARD auto defaultTriviallyInitStructID() const -> DefaultTriviallyInitStructID {
			evo::debugAssert(
				this->kind() == Kind::DEFAULT_TRIVIALLY_INIT_STRUCT, "not a default trivially init struct"
			);
			return this->value.default_trivially_init_struct;
		}
		EVO_NODISCARD auto defaultInitArrayRefID() const -> DefaultInitArrayRefID {
			evo::debugAssert(this->kind() == Kind::DEFAULT_INIT_ARRAY_REF, "not a default init array ref");
			return this->value.default_init_array_ref;
		}
		EVO_NODISCARD auto initArrayRefID() const -> InitArrayRefID {
			evo::debugAssert(this->kind() == Kind::INIT_ARRAY_REF, "not an init array ref");
			return this->value.init_array_ref;
		}
		EVO_NODISCARD auto arrayRefIndexerID() const -> ArrayRefIndexerID {
			evo::debugAssert(this->kind() == Kind::ARRAY_REF_INDEXER, "not an array ref indexer");
			return this->value.array_ref_indexer;
		}
		EVO_NODISCARD auto arrayRefSizeID() const -> ArrayRefSizeID {
			evo::debugAssert(this->kind() == Kind::ARRAY_REF_SIZE, "not an array ref size");
			return this->value.array_ref_size;
		}
		EVO_NODISCARD auto arrayRefDimensionsID() const -> ArrayRefDimensionsID {
			evo::debugAssert(this->kind() == Kind::ARRAY_REF_DIMENSIONS, "not an array ref dimensions");
			return this->value.array_ref_dimensions;
		}
		EVO_NODISCARD auto unionDesignatedInitNewID() const -> UnionDesignatedInitNewID {
			evo::debugAssert(this->kind() == Kind::UNION_DESIGNATED_INIT_NEW, "not a union designated init new");
			return this->value.union_designated_init_new;
		}


		EVO_NODISCARD auto paramID() const -> ParamID {
			evo::debugAssert(this->kind() == Kind::PARAM, "not a param");
			return this->value.param;
		}
		EVO_NODISCARD auto returnParamID() const -> ReturnParamID {
			evo::debugAssert(this->kind() == Kind::RETURN_PARAM, "not a return param");
			return this->value.return_param;
		}
		EVO_NODISCARD auto errorReturnParamID() const -> ErrorReturnParamID {
			evo::debugAssert(this->kind() == Kind::ERROR_RETURN_PARAM, "not an error return param");
			return this->value.error_return_param;
		}
		EVO_NODISCARD auto blockExprOutputID() const -> BlockExprOutputID {
			evo::debugAssert(this->kind() == Kind::BLOCK_EXPR_OUTPUT, "not a block expr output");
			return this->value.block_expr_output;
		}
		EVO_NODISCARD auto exceptParamID() const -> ExceptParamID {
			evo::debugAssert(this->kind() == Kind::EXCEPT_PARAM, "not an except param");
			return this->value.except_param;
		}

		EVO_NODISCARD auto varID() const -> VarID {
			evo::debugAssert(this->kind() == Kind::VAR, "not a var");
			return this->value.var;
		}
		EVO_NODISCARD auto globalVarID() const -> GlobalVarID {
			evo::debugAssert(this->kind() == Kind::GLOBAL_VAR, "not a global var");
			return this->value.global_var;
		}
		EVO_NODISCARD auto funcID() const -> FuncID {
			evo::debugAssert(this->kind() == Kind::FUNC, "not a func");
			return this->value.func;
		}


		EVO_NODISCARD auto operator==(const Expr& rhs) const -> bool {
			return evo::bitCast<uint64_t>(*this) == evo::bitCast<uint64_t>(rhs);
		}


		private:
			Expr(Kind k, Token::ID token) : _kind(k), value{.token = token} {}

			EVO_NODISCARD static auto createNone() -> Expr { return Expr(Kind::NONE, Token::ID(0)); }

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
				TryElseID try_else;
				BlockExprID block_expr;
				FakeTermInfoID fake_term_info;
				MakeInterfacePtrID make_interface_ptr;
				InterfacePtrExtractThisID interface_ptr_extract_this;
				InterfaceCallID interface_call;
				IndexerID indexer;
				DefaultInitPrimitiveID default_init_primitive;
				DefaultTriviallyInitStructID default_trivially_init_struct;
				DefaultInitArrayRefID default_init_array_ref;
				InitArrayRefID init_array_ref;
				ArrayRefIndexerID array_ref_indexer;
				ArrayRefSizeID array_ref_size;
				ArrayRefDimensionsID array_ref_dimensions;
				UnionDesignatedInitNewID union_designated_init_new;

				ExceptParamID except_param;
				ParamID param;
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
