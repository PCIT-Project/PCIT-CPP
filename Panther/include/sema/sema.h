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
#include <PIR.h>

#include "../AST/AST.h"
#include "../TypeManager.h"
#include "./sema_ids.h"
#include "./Stmt.h"
#include "./Expr.h"
#include "../../src/symbol_proc/symbol_proc_ids.h"
#include "../intrinsics.h"


namespace pcit::panther{
	class SymbolProc;
	class Context;
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


	struct TemplateIntrinsicFuncInstantiation{
		using ID = TemplateIntrinsicFuncInstantiationID;

		TemplateIntrinsicFunc::Kind kind;
		evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>> templateArgs;
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

		evo::Variant<FuncID, IntrinsicFunc::Kind, TemplateIntrinsicFuncInstantiationID> target;
		evo::SmallVector<Expr> args;
		// SourceLocation location;
	};


	struct Assign{
		using ID = AssignID;

		std::optional<Expr> lhs; // nullopt if is a discard
		Expr rhs;
	};

	struct MultiAssign{
		using ID = MultiAssignID;

		evo::SmallVector<evo::Variant<Expr, TypeInfo::ID>> targets; // TypeInfo::ID if is a discard
		Expr value;
	};

	struct Return{
		using ID = ReturnID;

		std::optional<Expr> value; // nullopt means return void
	};

	struct Error{
		using ID = ErrorID;

		std::optional<Expr> value; // nullopt means return void
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

		uint32_t index;
		uint32_t abiIndex;
	};

	struct ReturnParam{
		using ID = ReturnParamID;

		uint32_t index;
		uint32_t abiIndex;
	};



	struct GlobalVar{
		using ID = GlobalVarID;

		AST::VarDecl::Kind kind;
		Token::ID ident;
		SourceID sourceID;
		std::atomic<std::optional<Expr>> expr; // is nullopt if decl is done, but not def
		std::optional<TypeInfo::ID> typeID; // is nullopt iff (kind == `def` && is fluid)
		bool isPub;
	};


	struct Func{
		using ID = FuncID;

		struct Param{
			Token::ID ident;
			std::optional<Expr> defaultValue;
		};

		AST::Node name;
		SourceID sourceID;
		BaseType::Function::ID typeID;
		evo::SmallVector<Param> params;
		SymbolProc& symbolProc;
		SymbolProcID symbolProcID;
		uint32_t minNumArgs; // TODO(PERF): make sure this optimization actually improves perf
		bool isPub;
		bool isConstexpr;
		bool hasInParam;
		
		uint32_t instanceID = std::numeric_limits<uint32_t>::max(); // max if not an instantiation

		sema::StmtBlock stmtBlock{};

		bool isTerminated = false;
		std::atomic<bool> defCompleted = false;

		std::optional<pir::Function::ID> constexprJITFunc{};
		std::optional<pir::Function::ID> constexprJITInterfaceFunc{};

		EVO_NODISCARD auto isEquivalentOverload(const Func& rhs, const class panther::Context& context) const -> bool;
	};




	struct TemplatedFunc{
		using ID = TemplatedFuncID;
		using Arg = evo::Variant<TypeInfo::VoidableID, core::GenericValue>;

		struct Instantiation{
			std::atomic<std::optional<SymbolProcID>> symbolProcID{}; // nullopt means its being generated
			std::optional<BaseType::Function::ID> funcID{}; // nullopt means it's being worked on
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


		TemplatedFunc(SymbolProc& symbol_proc, size_t min_num_template_args, evo::SmallVector<Param>&& _params)
			: symbolProc(symbol_proc), minNumTemplateArgs(min_num_template_args), params(std::move(_params)) {}

		private:
			core::LinearStepAlloc<Instantiation, size_t> instantiations{};
			std::unordered_map<evo::SmallVector<Arg>, Instantiation&> instantiation_map{};
			mutable core::SpinLock instantiation_lock{};
	};


	struct TemplatedStruct{
		using ID = TemplatedStructID;

		BaseType::StructTemplate::ID templateID;
		SymbolProc& symbolProc;
	};


}
