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


namespace pcit::panther::sema{



	// TODO: make variant?

	struct Expr{
		enum class Kind : uint32_t {
			None, // only use for optional

			ModuleIdent,

			Uninit,
			Zeroinit,

			IntValue,
			FloatValue,
			BoolValue,
			StringValue,
			CharValue,

			Intrinsic,
			TemplatedIntrinsicInstantiation,

			Copy,
			Move,
			DestructiveMove,
			Forward,
			FuncCall,
			AddrOf,
			Deref,
				
			Param,
			ReturnParam,

			GlobalVar,
			Func,
		};

		static auto createModuleIdent(Token::ID id) -> Expr {
			return Expr(Kind::ModuleIdent, id);
		}

		explicit Expr(UninitID id)      : _kind(Kind::Uninit),      value{.uninit = id}       {};
		explicit Expr(ZeroinitID id)    : _kind(Kind::Zeroinit),    value{.zeroinit = id}     {};

		explicit Expr(IntValueID id)    : _kind(Kind::IntValue),    value{.int_value = id}    {};
		explicit Expr(FloatValueID id)  : _kind(Kind::FloatValue),  value{.float_value = id}  {};
		explicit Expr(BoolValueID id)   : _kind(Kind::BoolValue),   value{.bool_value = id}   {};
		explicit Expr(StringValueID id) : _kind(Kind::StringValue), value{.string_value = id} {};
		explicit Expr(CharValueID id)   : _kind(Kind::CharValue),   value{.char_value = id}   {};

		// explicit Expr(Intrinsic::Kind intrinsic_kind) : _kind(Kind::Intrinsic), value{.intrinsic = intrinsic_kind} {};
		// explicit Expr(TemplatedIntrinsicInstantiationID id)
		// 	: _kind(Kind::TemplatedIntrinsicInstantiation), 
		// 	  value{.templated_intrinsic_instantiation = id} 
		// 	  {};

		explicit Expr(CopyID id)            : _kind(Kind::Copy),            value{.copy = id}         {};
		explicit Expr(MoveID id)            : _kind(Kind::Move),            value{.move = id}         {};
		explicit Expr(DestructiveMoveID id) : _kind(Kind::DestructiveMove), value{.dest_move = id}    {};
		explicit Expr(ForwardID id)         : _kind(Kind::Forward),         value{.forward = id}      {};
		explicit Expr(FuncCallID id)        : _kind(Kind::FuncCall),        value{.func_call = id}    {};
		explicit Expr(AddrOfID id)          : _kind(Kind::AddrOf),          value{.addr_of = id}      {};
		explicit Expr(DerefID id)           : _kind(Kind::Deref),           value{.deref = id}        {};

		explicit Expr(ParamID id)       : _kind(Kind::Param),           value{.param = id}        {};
		explicit Expr(ReturnParamID id) : _kind(Kind::ReturnParam),     value{.return_param = id} {};

		explicit Expr(GlobalVarID id)   : _kind(Kind::GlobalVar),       value{.global_var = id}   {};
		explicit Expr(FuncID id)        : _kind(Kind::Func),            value{.func = id}         {};


		EVO_NODISCARD constexpr auto kind() const -> Kind { return this->_kind; }


		EVO_NODISCARD auto moduleIdent() const -> Token::ID {
			evo::debugAssert(this->kind() == Kind::ModuleIdent, "not a ModuleIdent");
			return this->value.token;
		}

		EVO_NODISCARD auto uninitID() const -> UninitID {
			evo::debugAssert(this->kind() == Kind::Uninit, "not a Uninit");
			return this->value.uninit;
		}

		EVO_NODISCARD auto zeroinitID() const -> ZeroinitID {
			evo::debugAssert(this->kind() == Kind::Zeroinit, "not a Zeroinit");
			return this->value.zeroinit;
		}

		EVO_NODISCARD auto intValueID() const -> IntValueID {
			evo::debugAssert(this->kind() == Kind::IntValue, "not a IntValue");
			return this->value.int_value;
		}
		EVO_NODISCARD auto floatValueID() const -> FloatValueID {
			evo::debugAssert(this->kind() == Kind::FloatValue, "not a FloatValue");
			return this->value.float_value;
		}
		EVO_NODISCARD auto boolValueID() const -> BoolValueID {
			evo::debugAssert(this->kind() == Kind::BoolValue, "not a BoolValue");
			return this->value.bool_value;
		}
		EVO_NODISCARD auto stringValueID() const -> StringValueID {
			evo::debugAssert(this->kind() == Kind::StringValue, "not a StringValue");
			return this->value.string_value;
		}
		EVO_NODISCARD auto charValueID() const -> CharValueID {
			evo::debugAssert(this->kind() == Kind::CharValue, "not a CharValue");
			return this->value.char_value;
		}

		// EVO_NODISCARD auto intrinsicID() const -> Intrinsic::Kind {
		// 	evo::debugAssert(this->kind() == Kind::Intrinsic, "not a Intrinsic");
		// 	return this->value.intrinsic;
		// }
		// EVO_NODISCARD auto templatedIntrinsicInstantiationID() const -> TemplatedIntrinsicInstantiationID {
		// 	evo::debugAssert(
		// 		this->kind() == Kind::TemplatedIntrinsicInstantiation, "not a TemplatedIntrinsicInstantiation"
		// 	);
		// 	return this->value.templated_intrinsic_instantiation;
		// }

		EVO_NODISCARD auto copyID() const -> CopyID {
			evo::debugAssert(this->kind() == Kind::Copy, "not a copy");
			return this->value.copy;
		}
		EVO_NODISCARD auto moveID() const -> MoveID {
			evo::debugAssert(this->kind() == Kind::Move, "not a move");
			return this->value.move;
		}
		EVO_NODISCARD auto funcCallID() const -> FuncCallID {
			evo::debugAssert(this->kind() == Kind::FuncCall, "not a func call");
			return this->value.func_call;
		}
		EVO_NODISCARD auto addrOfID() const -> AddrOfID {
			evo::debugAssert(this->kind() == Kind::AddrOf, "not an addr of");
			return this->value.addr_of;
		}
		EVO_NODISCARD auto derefID() const -> DerefID {
			evo::debugAssert(this->kind() == Kind::Deref, "not an deref");
			return this->value.deref;
		}

		EVO_NODISCARD auto paramID() const -> ParamID {
			evo::debugAssert(this->kind() == Kind::Param, "not a param");
			return this->value.param;
		}
		EVO_NODISCARD auto returnParamID() const -> ReturnParamID {
			evo::debugAssert(this->kind() == Kind::ReturnParam, "not a return param");
			return this->value.return_param;
		}

		EVO_NODISCARD auto globalVarID() const -> GlobalVarID {
			evo::debugAssert(this->kind() == Kind::GlobalVar, "not a global var");
			return this->value.global_var;
		}
		EVO_NODISCARD auto funcID() const -> FuncID {
			evo::debugAssert(this->kind() == Kind::Func, "not a func");
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

				// Intrinsic::Kind intrinsic;
				// TemplatedIntrinsicInstantiationID templated_intrinsic_instantiation;

				CopyID copy;
				MoveID move;
				DestructiveMoveID dest_move;
				ForwardID forward;
				AddrOfID addr_of;
				DerefID deref;
				FuncCallID func_call;

				ParamID param;
				ReturnParamID return_param;

				GlobalVarID global_var;
				FuncID func;
			} value;

			friend struct ExprOptInterface;
	};


	struct ExprOptInterface{
		static constexpr auto init(Expr* expr) -> void {
			expr->_kind = Expr::Kind::None;
		}

		static constexpr auto has_value(const Expr& expr) -> bool {
			return expr._kind != Expr::Kind::None;
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
