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

#include "../AST/AST.h"
#include "../TypeManager.h"
#include "./sema_ids.h"
#include "./sema_stmt.h"


namespace pcit::panther::sema{


	//////////////////////////////////////////////////////////////////////
	// expressions

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

			Var,
			Func,
			Struct,
		};

		static auto createModuleIdent(Token::ID id) -> Expr {
			return Expr(Kind::ModuleIdent, id);
		}

		explicit Expr(UninitID id)     : _kind(Kind::Uninit),     value{.uninit = id}      {};
		explicit Expr(ZeroinitID id)   : _kind(Kind::Zeroinit),   value{.zeroinit = id}    {};

		explicit Expr(IntValueID id)   : _kind(Kind::IntValue),   value{.int_value = id}   {};
		explicit Expr(FloatValueID id) : _kind(Kind::FloatValue), value{.float_value = id} {};
		explicit Expr(BoolValueID id)  : _kind(Kind::BoolValue),  value{.bool_value = id}  {};
		explicit Expr(CharValueID id)  : _kind(Kind::CharValue),  value{.char_value = id}  {};

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

		explicit Expr(VarID id)         : _kind(Kind::Var),             value{.var = id}          {};
		explicit Expr(FuncID id)        : _kind(Kind::Func),            value{.func = id}         {};
		explicit Expr(StructID id)      : _kind(Kind::Struct),          value{._struct = id} {};


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

		EVO_NODISCARD auto varID() const -> VarID {
			evo::debugAssert(this->kind() == Kind::Var, "not a var");
			return this->value.var;
		}
		EVO_NODISCARD auto funcID() const -> FuncID {
			evo::debugAssert(this->kind() == Kind::Func, "not a func");
			return this->value.func;
		}
		EVO_NODISCARD auto structID() const -> StructID {
			evo::debugAssert(this->kind() == Kind::Struct, "not a struct");
			return this->value._struct;
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

				VarID var;
				FuncID func;
				StructID _struct;
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


namespace pcit::panther::sema{

	struct IntValue{
		using ID = IntValueID;

		core::GenericInt value;
		std::optional<BaseType::ID> typeID; // nullopt if type is unknown (needs to be set before usage)
	};

	struct FloatValue{
		using ID = FloatValueID;

		core::GenericFloat value;
		std::optional<BaseType::ID> typeID; // nullopt if type is unknown (needs to be set before usage)
	};

	struct BoolValue{
		using ID = BoolValueID;

		bool value;
	};

	struct CharValue{
		using ID = CharValueID;

		char value;
	};


	struct TemplatedIntrinsicInstantiation{
		using ID = TemplatedIntrinsicInstantiationID;

	// 	using TemplateArg = evo::Variant<TypeInfo::VoidableID, core::GenericInt, core::GenericFloat, char, bool>;

	// 	TemplatedIntrinsic::Kind kind;
	// 	evo::SmallVector<TemplateArg> templateArgs;
	};


	namespace Copy{
		using ID = CopyID;
	}

	namespace Move{
		using ID = MoveID;
	}

	namespace DestructiveMove{
		using ID = DestructiveMoveID;
	}

	namespace Forward{
		using ID = ForwardID;
	}

	namespace AddrOf{
		using ID = AddrOfID;
	}

	namespace Uninit{
		using ID = UninitID;
	}

	namespace Zeroinit{
		using ID = ZeroinitID;
	}



	struct Deref{
		using ID = DerefID;

	// 	Expr expr;
	// 	TypeInfo::ID typeID;
	};



	//////////////////////////////////////////////////////////////////////
	// statements


	struct FuncCall{
		using ID = FuncCallID;

	// 	evo::Variant<FuncID, Intrinsic::Kind, TemplatedIntrinsicInstantiationID> target;
	// 	evo::SmallVector<Expr> args;
	// 	Location location;
	};


	struct Assign{
		using ID = AssignID;

	// 	Expr lhs;
	// 	Expr rhs;
	};

	struct MultiAssign{
		using ID = MultiAssignID;

	// 	evo::SmallVector<std::optional<Expr>> targets;
	// 	Expr value;
	};

	struct Return{
		using ID = ReturnID;

	// 	std::optional<Expr> value; // nullopt means `return;`
	};

	struct Conditional{
		using ID = ConditionalID;

	// 	Expr cond;
	// 	StmtBlock thenStmts;
	// 	StmtBlock elseStmts;
	};

	struct While{
		using ID = WhileID;

	// 	Expr cond;
	// 	StmtBlock block;
	};




	struct Param{
		using ID = ParamID;

	// 	Func::ID func;
	// 	uint32_t index;
	};

	struct ReturnParam{
		using ID = ReturnParamID;

	// 	Func::ID func;
	// 	uint32_t index;
	};



	struct Var{
		using ID = VarID;

		AST::VarDecl::Kind kind;
		Token::ID ident;
		std::optional<Expr> expr;
		std::optional<TypeInfo::ID> typeID; // is nullopt iff (kind == `def` && is fluid)
		bool isPub;
	};


	// // TODO: move .scope, .body_analysis_mutex, .is_body_analyzed, and .ast_func to somewhere else
	struct Func{
		using ID = FuncID;

	// 	struct InstanceID{
	// 		InstanceID() : id(std::numeric_limits<uint32_t>::max()) {}
	// 		InstanceID(uint32_t instance_id) : id(instance_id) {}

	// 		EVO_NODISCARD auto get() const -> uint32_t {
	// 			evo::debugAssert(this->has_value(), "cannot get instance value as it doesn't have one");
	// 			return this->id;
	// 		}

	// 		EVO_NODISCARD auto has_value() const -> bool {
	// 			return this->id != std::numeric_limits<uint32_t>::max();
	// 		}

	// 		private:
	// 			uint32_t id;
	// 	};

	// 	AST::Node name;
	// 	BaseType::ID baseTypeID;
	// 	Parent parent;
	// 	InstanceID instanceID;
	// 	bool isPub;
	// 	std::optional<ScopeManager::Scope> scope; // only if is constexpr

	// 	bool isTerminated = false;
	// 	evo::SmallVector<ParamID> params{};
	// 	evo::SmallVector<ReturnParamID> returnParams{}; // only for named return params
	// 	StmtBlock stmts{};

	// 	Func(
	// 		const AST::Node& _name,
	// 		const BaseType::ID& base_type_id,
	// 		const Parent& _parent,
	// 		const InstanceID& instance_id,
	// 		bool is_pub,
	// 		const std::optional<ScopeManager::Scope>& _scope,
	// 		const AST::FuncDecl& _ast_func
	// 	) : name(_name),
	// 		baseTypeID(base_type_id),
	// 		parent(_parent),
	// 		instanceID(instance_id),
	// 		isPub(is_pub),
	// 		scope(_scope),
	// 		ast_func(_ast_func)
	// 	{}


	// 	private:
	// 		mutable core::SpinLock body_analysis_mutex{};
	// 		bool is_body_analyzed = false;
	// 		bool is_body_errored = false; // only need to set if func is not runtime
	// 		AST::FuncDecl ast_func;

	// 		friend /*class*/ SemanticAnalyzer;
	};



	struct TemplatedFunc{
		using ID = TemplatedFuncID;


	// 	struct TemplateParam{
	// 		Token::ID ident;
	// 		std::optional<TypeInfo::ID> typeID; // nullopt means type "Type"
	// 	};

	// 	const AST::FuncDecl& funcDecl;
	// 	Parent parent;
	// 	evo::SmallVector<TemplateParam> templateParams;
	// 	ScopeManager::Scope scope;
	// 	bool isPub: 1;
	// 	bool isRuntime: 1;

	// 	TemplatedFunc(
	// 		const AST::FuncDecl& func_decl,
	// 		Parent _parent,
	// 		evo::SmallVector<TemplateParam>&& template_params,
	// 		const ScopeManager::Scope& _scope,
	// 		bool is_pub,
	// 		bool is_runtime
	// 	) : 
	// 		funcDecl(func_decl),
	// 		parent(_parent),
	// 		templateParams(std::move(template_params)),
	// 		scope(_scope),
	// 		isPub(is_pub),
	// 		isRuntime(is_runtime)
	// 	{}


	// 	struct LookupInfo{
	// 		bool needToGenerate;
	// 		sema::Func::InstanceID instanceID;

	// 		LookupInfo(
	// 			bool _need_to_generate,
	// 			sema::Func::InstanceID instance_id,
	// 			std::atomic<std::optional<Func::ID>>& func_id
	// 		) : needToGenerate(_need_to_generate), instanceID(instance_id), id(func_id) {}

	// 		auto waitForAndGetID() const -> Func::ID {
	// 			while(this->id.load().has_value() == false){}
	// 			return *this->id.load(); 
	// 		};

	// 		auto store(Func::ID func_id) -> void {
	// 			this->id.store(func_id);
	// 		}

	// 		private:
	// 			std::atomic<std::optional<Func::ID>>& id;
	// 	};

	// 	using Arg = evo::Variant<TypeInfo::VoidableID, core::GenericInt, core::GenericFloat, char, bool>;
	// 	EVO_NODISCARD auto lookupInstance(evo::SmallVector<Arg>&& args) -> LookupInfo;

	// 	private:
	// 		struct Instatiation{
	// 			std::atomic<std::optional<Func::ID>> id;
	// 			evo::SmallVector<Arg> args;
	// 		};
	// 		// TODO: speedup lookup?
	// 		// TODO: better allocation?
	// 		evo::SmallVector<std::unique_ptr<Instatiation>> instantiations{};
	// 		mutable core::SpinLock instance_lock{};
	};


	struct Struct{
		using ID = StructID;
	};


}
