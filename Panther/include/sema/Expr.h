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
			CHAR_VALUE,

			INTRINSIC_FUNC,
			TEMPLATED_INTRINSIC_FUNC_INSTANTIATION,

			COPY,
			MOVE,
			FORWARD,
			FUNC_CALL,
			ADDR_OF,
			DEREF,
			TRY_ELSE,
				
			PARAM,
			RETURN_PARAM,

			GLOBAL_VAR,
			FUNC,
		};

		static auto createModuleIdent(Token::ID id) -> Expr {
			return Expr(Kind::MODULE_IDENT, id);
		}

		explicit Expr(UninitID id)      : _kind(Kind::UNINIT),       value{.uninit = id}       {};
		explicit Expr(ZeroinitID id)    : _kind(Kind::ZEROINIT),     value{.zeroinit = id}     {};

		explicit Expr(IntValueID id)    : _kind(Kind::INT_VALUE),    value{.int_value = id}    {};
		explicit Expr(FloatValueID id)  : _kind(Kind::FLOAT_VALUE),  value{.float_value = id}  {};
		explicit Expr(BoolValueID id)   : _kind(Kind::BOOL_VALUE),   value{.bool_value = id}   {};
		explicit Expr(StringValueID id) : _kind(Kind::STRING_VALUE), value{.string_value = id} {};
		explicit Expr(CharValueID id)   : _kind(Kind::CHAR_VALUE),   value{.char_value = id}   {};

		explicit Expr(IntrinsicFunc::Kind intrinsic_func_kind) :
			_kind(Kind::INTRINSIC_FUNC), value{.intrinsic_func = intrinsic_func_kind} {};
		explicit Expr(TemplateIntrinsicFuncInstantiationID id)
			: _kind(Kind::TEMPLATED_INTRINSIC_FUNC_INSTANTIATION), 
			  value{.templated_intrinsic_func_instantiation = id} 
			  {};

		explicit Expr(CopyID id)            : _kind(Kind::COPY),             value{.copy = id}         {};
		explicit Expr(MoveID id)            : _kind(Kind::MOVE),             value{.move = id}         {};
		explicit Expr(ForwardID id)         : _kind(Kind::FORWARD),          value{.forward = id}      {};
		explicit Expr(FuncCallID id)        : _kind(Kind::FUNC_CALL),        value{.func_call = id}    {};
		explicit Expr(AddrOfID id)          : _kind(Kind::ADDR_OF),          value{.addr_of = id}      {};
		explicit Expr(DerefID id)           : _kind(Kind::DEREF),            value{.deref = id}        {};
		explicit Expr(TryElseID id)         : _kind(Kind::TRY_ELSE),         value{.try_else = id}  {};

		explicit Expr(ParamID id)           : _kind(Kind::PARAM),            value{.param = id}        {};
		explicit Expr(ReturnParamID id)     : _kind(Kind::RETURN_PARAM),     value{.return_param = id} {};

		explicit Expr(GlobalVarID id)       : _kind(Kind::GLOBAL_VAR),       value{.global_var = id}   {};
		explicit Expr(FuncID id)            : _kind(Kind::FUNC),             value{.func = id}         {};


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
			evo::debugAssert(this->kind() == Kind::STRING_VALUE, "not a StringValue");
			return this->value.string_value;
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
			evo::debugAssert(this->kind() == Kind::DEREF, "not an deref");
			return this->value.deref;
		}
		EVO_NODISCARD auto tryElseID() const -> TryElseID {
			evo::debugAssert(this->kind() == Kind::TRY_ELSE, "not an try/else");
			return this->value.try_else;
		}

		EVO_NODISCARD auto paramID() const -> ParamID {
			evo::debugAssert(this->kind() == Kind::PARAM, "not a param");
			return this->value.param;
		}
		EVO_NODISCARD auto returnParamID() const -> ReturnParamID {
			evo::debugAssert(this->kind() == Kind::RETURN_PARAM, "not a return param");
			return this->value.return_param;
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
				CharValueID char_value;

				IntrinsicFunc::Kind intrinsic_func;
				TemplateIntrinsicFuncInstantiationID templated_intrinsic_func_instantiation;

				CopyID copy;
				MoveID move;
				ForwardID forward;
				FuncCallID func_call;
				AddrOfID addr_of;
				DerefID deref;
				TryElseID try_else;

				ParamID param;
				ReturnParamID return_param;

				GlobalVarID global_var;
				FuncID func;
			} value;

			friend struct ExprOptInterface;
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

}
