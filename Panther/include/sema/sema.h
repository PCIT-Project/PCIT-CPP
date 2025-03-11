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
#include "./Stmt.h"
#include "./Expr.h"
#include "../../src/symbol_proc/symbol_proc_ids.h"


namespace pcit::panther{
	class SymbolProc;
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

	struct StringValue{
		using ID = StringValueID;

		std::string value;
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
		std::atomic<std::optional<Expr>> expr; // is nullopt if decl is done, but not def
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




	struct TemplatedStruct{
		using ID = TemplatedStructID;
		using Arg = evo::Variant<TypeInfo::VoidableID, core::GenericValue>;

		struct Instantiation{
			std::atomic<std::optional<SymbolProcID>> symbolProcID{}; // nullopt means its being generated
			std::optional<BaseType::Struct::ID> structID{}; // nullopt means it's being worked on
			std::atomic<bool> errored = false;

			Instantiation() = default;
			Instantiation(const Instantiation&) = delete;
		};

		struct Param{
			std::optional<TypeInfo::ID> typeID;
			evo::Variant<std::monostate, Expr, TypeInfo::VoidableID> defaultValue;
		};

		SymbolProc& symbolProc;
		size_t minNumTemplateArgs;
		evo::SmallVector<Param> params;

		struct InstantiationInfo{
			Instantiation& instantiation;
			std::optional<uint32_t> instantiationID; // only has value if it needs to be compiled

			EVO_NODISCARD auto needsToBeCompiled() const -> bool { return this->instantiationID.has_value(); }
		};
		EVO_NODISCARD auto lookupInstantiation(evo::SmallVector<Arg>&& args) -> InstantiationInfo;

		EVO_NODISCARD auto hasAnyDefaultParams() const -> bool {
			return this->minNumTemplateArgs != this->params.size();
		}


		TemplatedStruct(SymbolProc& symbol_proc, size_t min_num_template_args, evo::SmallVector<Param>&& _params)
			: symbolProc(symbol_proc), minNumTemplateArgs(min_num_template_args), params(std::move(_params)) {}

		private:
			core::LinearStepAlloc<Instantiation, size_t> instantiations{};
			std::unordered_map<evo::SmallVector<Arg>, Instantiation&> instantiation_map{};
			mutable core::SpinLock instantiation_lock{};
	};


}
