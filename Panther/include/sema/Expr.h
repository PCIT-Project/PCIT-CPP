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
			DEREF,
			ACCESSOR,
			PTR_ACCESSOR,
			TRY_ELSE,
			BLOCK_EXPR,
			FAKE_TERM_INFO,
				
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

		explicit Expr(UninitID id)         : _kind(Kind::UNINIT),          value{.uninit = id}          {};
		explicit Expr(ZeroinitID id)       : _kind(Kind::ZEROINIT),        value{.zeroinit = id}        {};

		explicit Expr(IntValueID id)       : _kind(Kind::INT_VALUE),       value{.int_value = id}       {};
		explicit Expr(FloatValueID id)     : _kind(Kind::FLOAT_VALUE),     value{.float_value = id}     {};
		explicit Expr(BoolValueID id)      : _kind(Kind::BOOL_VALUE),      value{.bool_value = id}      {};
		explicit Expr(StringValueID id)    : _kind(Kind::STRING_VALUE),    value{.string_value = id}    {};
		explicit Expr(AggregateValueID id) : _kind(Kind::AGGREGATE_VALUE), value{.aggregate_value = id} {};
		explicit Expr(CharValueID id)      : _kind(Kind::CHAR_VALUE),      value{.char_value = id}      {};

		explicit Expr(IntrinsicFunc::Kind intrinsic_func_kind) :
			_kind(Kind::INTRINSIC_FUNC), value{.intrinsic_func = intrinsic_func_kind} {};
		explicit Expr(TemplateIntrinsicFuncInstantiationID id)
			: _kind(Kind::TEMPLATED_INTRINSIC_FUNC_INSTANTIATION), 
			  value{.templated_intrinsic_func_instantiation = id} 
			  {};

		explicit Expr(CopyID id)             : _kind(Kind::COPY),               value{.copy = id}               {};
		explicit Expr(MoveID id)             : _kind(Kind::MOVE),               value{.move = id}               {};
		explicit Expr(ForwardID id)          : _kind(Kind::FORWARD),            value{.forward = id}            {};
		explicit Expr(FuncCallID id)         : _kind(Kind::FUNC_CALL),          value{.func_call = id}          {};
		explicit Expr(AddrOfID id)           : _kind(Kind::ADDR_OF),            value{.addr_of = id}            {};
		explicit Expr(DerefID id)            : _kind(Kind::DEREF),              value{.deref = id}              {};
		explicit Expr(AccessorID id)         : _kind(Kind::ACCESSOR),           value{.accessor = id}           {};
		explicit Expr(PtrAccessorID id)      : _kind(Kind::PTR_ACCESSOR),       value{.ptr_accessor = id}       {};
		explicit Expr(TryElseID id)          : _kind(Kind::TRY_ELSE),           value{.try_else = id}           {};
		explicit Expr(BlockExprID id)        : _kind(Kind::BLOCK_EXPR),         value{.block_expr = id}         {};
		explicit Expr(FakeTermInfoID id)     : _kind(Kind::FAKE_TERM_INFO),     value{.fake_term_infos = id}    {};

		explicit Expr(ParamID id)            : _kind(Kind::PARAM),              value{.param = id}              {};
		explicit Expr(ReturnParamID id)      : _kind(Kind::RETURN_PARAM),       value{.return_param = id}       {};
		explicit Expr(ErrorReturnParamID id) : _kind(Kind::ERROR_RETURN_PARAM), value{.error_return_param = id} {};
		explicit Expr(BlockExprOutputID id)  : _kind(Kind::BLOCK_EXPR_OUTPUT),  value{.block_expr_output = id}  {};
		explicit Expr(ExceptParamID id)      : _kind(Kind::EXCEPT_PARAM),       value{.except_param = id}       {};

		explicit Expr(VarID id)              : _kind(Kind::VAR),                value{.var = id}                {};
		explicit Expr(GlobalVarID id)        : _kind(Kind::GLOBAL_VAR),         value{.global_var = id}         {};
		explicit Expr(FuncID id)             : _kind(Kind::FUNC),               value{.func = id}               {};


		EVO_NODISCARD constexpr auto kind() const -> Kind { return this->_kind; }


		EVO_NODISCARD auto moduleIdent() const -> Token::ID {
			evo::debugAssert(this->kind() == Kind::MODULE_IDENT, "not a MODULE_IDENT");
			return this->value.token;
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
				"not a TEMPLATED_INTRINSIC_FUNC_INSTANTIATION"
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
		EVO_NODISCARD auto funcCallID() const -> FuncCallID {
			evo::debugAssert(this->kind() == Kind::FUNC_CALL, "not a func call");
			return this->value.func_call;
		}
		EVO_NODISCARD auto addrOfID() const -> AddrOfID {
			evo::debugAssert(this->kind() == Kind::ADDR_OF, "not an addr of");
			return this->value.addr_of;
		}
		EVO_NODISCARD auto derefID() const -> DerefID {
			evo::debugAssert(this->kind() == Kind::DEREF, "not a deref");
			return this->value.deref;
		}
		EVO_NODISCARD auto accessorID() const -> AccessorID {
			evo::debugAssert(this->kind() == Kind::ACCESSOR, "not an accessor");
			return this->value.accessor;
		}
		EVO_NODISCARD auto ptrAccessorID() const -> PtrAccessorID {
			evo::debugAssert(this->kind() == Kind::PTR_ACCESSOR, "not a ptr accessor");
			return this->value.ptr_accessor;
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
			return this->value.fake_term_infos;
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
				DerefID deref;
				AccessorID accessor;
				PtrAccessorID ptr_accessor;
				TryElseID try_else;
				BlockExprID block_expr;
				FakeTermInfoID fake_term_infos;
				ExceptParamID except_param;

				ParamID param;
				ReturnParamID return_param;
				ErrorReturnParamID error_return_param;
				BlockExprOutputID block_expr_output;

				VarID var;
				GlobalVarID global_var;
				FuncID func;
			} value;

			friend struct ExprOptInterface;
			friend SemanticAnalyzer;
	};


	struct ExprOptInterface{
		static constexpr auto init(Expr* expr) -> void {
			expr->_kind = Expr::Kind::NONE;
		}

		static constexpr auto has_value(const Expr& expr) -> bool {
			return expr._kind != Expr::Kind::NONE;
		}
	};

	static_assert(sizeof(Expr) == 8, "sema::Expr is different size than expected");
	static_assert(std::is_trivially_copyable_v<Expr>, "sema::Expr is not trivially copyable");

}


namespace std{
	
	template<>
	class optional<pcit::panther::sema::Expr> 
		: public pcit::core::Optional<pcit::panther::sema::Expr, pcit::panther::sema::ExprOptInterface>{

		public:
			using pcit::core::Optional<pcit::panther::sema::Expr, pcit::panther::sema::ExprOptInterface>::Optional;
			using pcit::core::Optional<pcit::panther::sema::Expr, pcit::panther::sema::ExprOptInterface>::operator=;
	};


	template<>
	struct hash<pcit::panther::sema::Expr>{
		auto operator()(const pcit::panther::sema::Expr& expr) const noexcept -> size_t {
			return hash<uint64_t>{}(*(uint64_t*)&expr);
		};
	};


}
