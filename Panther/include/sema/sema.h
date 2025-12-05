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

	struct AggregateValue{
		using ID = AggregateValueID;

		evo::SmallVector<Expr> values;
		BaseType::ID typeID;
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

	struct Copy{
		using ID = CopyID;

		Expr expr;
		TypeInfo::ID exprTypeID;
	};

	struct Move{
		using ID = MoveID;

		Expr expr;
		TypeInfo::ID exprTypeID;
	};

	struct Forward{
		using ID = ForwardID;

		Expr expr;
		TypeInfo::ID exprTypeID;
		bool isInitialization;
	};

	namespace AddrOf{
		using ID = AddrOfID;
	}


	namespace Uninit{
		using ID = UninitID;
	}

	namespace Zeroinit{
		using ID = ZeroinitID;
	}


	struct Null{
		using ID = NullID;

		std::optional<TypeInfo::ID> targetTypeID; // nullopt if unknown
	};

	struct ConversionToOptional{
		using ID = ConversionToOptionalID;

		Expr expr;
		TypeInfo::ID targetTypeID;
	};


	struct OptionalNullCheck{
		using ID = OptionalNullCheckID;
		
		Expr expr;
		TypeInfo::ID targetTypeID;
		bool equal;
	};

	struct OptionalExtract{
		using ID = OptionalExtractID;
		
		Expr expr;
		TypeInfo::ID targetTypeID;
	};


	struct Deref{
		using ID = DerefID;

		Expr expr;
		TypeInfo::ID targetTypeID;
	};

	struct Unwrap{
		using ID = UnwrapID;

		Expr expr;
		TypeInfo::ID targetTypeID;
	};


	struct Accessor{
		using ID = AccessorID;

		Expr target;
		TypeInfo::ID targetTypeID;
		uint32_t memberABIIndex;
	};

	struct UnionAccessor{
		using ID = UnionAccessorID;

		Expr target;
		TypeInfo::ID targetTypeID;
		uint32_t fieldIndex;
	};


	struct LogicalAnd{
		using ID = LogicalAndID;

		Expr lhs;
		Expr rhs;
	};


	struct LogicalOr{
		using ID = LogicalOrID;

		Expr lhs;
		Expr rhs;
	};


	struct TryElseExpr{
		using ID = TryElseExprID;

		Expr attempt;
		Expr except;
		evo::SmallVector<ExceptParamID> exceptParams;
	};

	struct TryElseInterfaceExpr{
		using ID = TryElseInterfaceExprID;

		Expr attempt;
		Expr except;
		evo::SmallVector<ExceptParamID> exceptParams;
	};


	struct BlockExpr{
		using ID = BlockExprID;

		struct Output{
			TypeInfo::ID typeID;
			std::optional<Token::ID> ident;
		};

		Token::ID label;
		evo::SmallVector<Output> outputs;
		StmtBlock block;

		EVO_NODISCARD auto hasNamedOutputs() const -> bool { return this->outputs[0].ident.has_value(); }
	};


	struct FakeTermInfo{
		using ID = FakeTermInfoID;

		enum class ValueCategory{
			EPHEMERAL,
			CONCRETE_CONST,
			CONCRETE_MUT,
			FORWARDABLE,
		};

		enum class ValueStage{
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
		
		ValueCategory valueCategory;
		ValueStage valueStage;
		ValueState valueState;
		TypeInfo::ID typeID;
		Expr expr;
	};


	struct MakeInterfacePtr{
		using ID = MakeInterfacePtrID;

		Expr expr;
		BaseType::Interface::ID interfaceID;
		TypeInfo::ID implTypeID;
	};

	struct InterfacePtrExtractThis{
		using ID = InterfacePtrExtractThisID;

		Expr expr;
	};


	struct InterfaceCall{
		using ID = InterfaceCallID;

		Expr value;
		BaseType::Function::ID funcTypeID;
		BaseType::Interface::ID interfaceID;
		uint32_t vtableFuncIndex;
		evo::SmallVector<Expr> args;
	};


	struct Indexer{
		using ID = IndexerID;

		Expr target;
		TypeInfo::ID targetTypeID;
		evo::SmallVector<Expr> indices;
	};


	struct DefaultInitPrimitive{
		using ID = DefaultInitPrimitiveID;

		BaseType::ID targetTypeID;
	};

	struct DefaultTriviallyInitStruct{
		using ID = DefaultTriviallyInitStructID;

		BaseType::ID targetTypeID;
	};


	struct DefaultInitArrayRef{
		using ID = DefaultInitArrayRefID;

		BaseType::ArrayRef::ID targetTypeID;
	};

	struct InitArrayRef{
		using ID = InitArrayRefID;	

		Expr expr;
		evo::SmallVector<evo::Variant<uint64_t, Expr>> dimensions;
	};

	struct ArrayRefIndexer{
		using ID = ArrayRefIndexerID;

		Expr target;
		BaseType::ArrayRef::ID targetTypeID;
		evo::SmallVector<Expr> indices;
	};


	struct ArrayRefSize{
		using ID = ArrayRefSizeID;

		Expr target;
		BaseType::ArrayRef::ID targetTypeID;
	};


	struct ArrayRefDimensions{
		using ID = ArrayRefDimensionsID;

		Expr target;
		BaseType::ArrayRef::ID targetTypeID;
	};

	struct ArrayRefData{
		using ID = ArrayRefDataID;

		Expr target;
		BaseType::ArrayRef::ID targetTypeID;
	};


	struct UnionDesignatedInitNew{
		using ID = UnionDesignatedInitNewID;

		Expr value;
		BaseType::Union::ID unionTypeID;
		uint32_t fieldIndex;
	};

	struct UnionTagCmp{
		using ID = UnionTagCmpID;

		Expr value;
		BaseType::Union::ID unionTypeID;
		uint32_t fieldIndex;
		bool isEqual;
	};


	struct SameTypeCmp{
		using ID = SameTypeCmpID;

		TypeInfo::ID typeID;
		Expr lhs;
		Expr rhs;
		bool isEqual;
	};



	//////////////////////////////////////////////////////////////////////
	// statements


	struct FuncCall{
		using ID = FuncCallID;

		evo::Variant<FuncID, IntrinsicFunc::Kind, TemplateIntrinsicFuncInstantiationID> target;
		evo::SmallVector<Expr> args;
		// SourceLocation location;
	};


	struct TryElse{
		using ID = TryElseID;

		FuncID target;
		evo::SmallVector<Expr> args;
		evo::SmallVector<ExceptParamID> exceptParams;
		StmtBlock elseBlock{};
	};

	struct TryElseInterface{
		using ID = TryElseInterfaceID;

		Expr value;
		BaseType::Function::ID funcTypeID;
		BaseType::Interface::ID interfaceID;
		uint32_t vtableFuncIndex;
		evo::SmallVector<Expr> args;
		evo::SmallVector<ExceptParamID> exceptParams;
		StmtBlock elseBlock{};
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
		std::optional<Token::ID> targetLabel;
	};

	struct Error{
		using ID = ErrorID;

		std::optional<Expr> value; // nullopt means return void
	};

	struct Break{
		using ID = BreakID;

		std::optional<Token::ID> label;
	};

	struct Continue{
		using ID = ContinueID;

		std::optional<Token::ID> label;
	};

	struct Delete{
		using ID = DeleteID;

		Expr expr;
		TypeInfo::ID exprTypeID;
	};

	struct BlockScope{
		using ID = BlockScopeID;
		
		StmtBlock block{};
	};

	struct Conditional{
		using ID = ConditionalID;

		Expr cond;
		StmtBlock thenStmts{};
		StmtBlock elseStmts{};
	};

	struct While{
		using ID = WhileID;

		Expr cond;
		std::optional<Token::ID> label;
		StmtBlock block{};
	};

	struct For{
		using ID = ForID;

		struct Iterable{
			Expr expr;
			FuncID createIteratorFunc;
			const BaseType::Interface::Impl& iteratorImpl;
		};

		struct Param{
			Token::ID ident;
			TypeInfo::ID typeID;
			Expr expr;
		};
		
		evo::SmallVector<Iterable> iterables;
		std::optional<Token::ID> label;
		bool hasIndex;
		evo::SmallVector<Param> params{};
		StmtBlock block{};
	};

	struct Defer{
		using ID = DeferID;

		bool isErrorDefer;
		StmtBlock block{};
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

	struct ErrorReturnParam{
		using ID = ErrorReturnParamID;

		uint32_t index;
		uint32_t abiIndex;
	};

	struct BlockExprOutput{
		using ID = BlockExprOutputID;

		uint32_t index;
		Token::ID label;
		TypeInfo::ID typeID;
	};


	struct ExceptParam{
		using ID = ExceptParamID;

		Token::ID ident;
		uint32_t index;
		TypeInfo::ID typeID;
	};


	struct ForParam{
		using ID = ForParamID;
		
		Token::ID ident;
		TypeInfo::ID typeID;
		bool isIndex;
		bool isMut;
	};


	struct Var{
		using ID = VarID;

		AST::VarDef::Kind kind;
		Token::ID ident;
		Expr expr;
		std::optional<TypeInfo::ID> typeID; // is nullopt iff (kind == `def` && is fluid)
	};



	struct GlobalVar{
		using ID = GlobalVarID;

		AST::VarDef::Kind kind;
		evo::Variant<SourceID, ClangSourceID> sourceID;
		evo::Variant<Token::ID, ClangSourceDeclInfoID> ident;
		std::string clangMangledName; // empty if not clang type
		std::atomic<std::optional<Expr>> expr; // is nullopt if decl is done but not def, or if clang type
		std::optional<TypeInfo::ID> typeID; // is nullopt iff (kind == `def` && is fluid)
		bool isPub;
		std::optional<SymbolProcID> symbolProcID; // TODO(FUTURE): need both id and ref?

		std::optional<pir::GlobalVar::ID> constexprJITGlobal{};

		EVO_NODISCARD auto isClangVar() const -> bool { return this->sourceID.is<ClangSourceID>(); }
		EVO_NODISCARD auto getName(const class panther::SourceManager& source_manager) const -> std::string_view;
	};


	struct Func{
		using ID = FuncID;

		struct CompilerCreatedOpOverload{
			Token::ID parentName;
			Token::Kind overloadKind;
		};

		struct Param{
			evo::Variant<Token::ID, ClangSourceDeclInfoID, BuiltinModuleStringID> ident;
			std::optional<Expr> defaultValue;
		};

		enum class Status{
			NOT_DONE,
			INTERFACE_METHOD_NO_DEFAULT,
			SUSPENDED,
			DEF_DONE,
		};

		evo::Variant<SourceID, ClangSourceID, BuiltinModuleID> sourceID;
		evo::Variant<Token::ID, ClangSourceDeclInfoID, CompilerCreatedOpOverload, BuiltinModuleStringID> name;
		std::string clangMangledName; // empty if not clang type
		BaseType::Function::ID typeID;
		evo::SmallVector<Param> params;
		std::optional<SymbolProcID> symbolProcID; // only value if is sema src type
		uint32_t minNumArgs;
		bool isPub; // meaningless if is Clang or builtin type
		bool isConstexpr;
		bool isExport; // always true if is clang type
		bool hasInParam; // always false if is clang type
		
		uint32_t instanceID = std::numeric_limits<uint32_t>::max(); // max if not an instantiation

		sema::StmtBlock stmtBlock{};

		bool isTerminated = false;
		std::atomic<Status> status = Status::NOT_DONE;

		std::optional<pir::Function::ID> constexprJITFunc{};
		std::optional<pir::Function::ID> constexprJITInterfaceFunc{};

		EVO_NODISCARD auto isClangFunc() const -> bool { return this->sourceID.is<ClangSourceID>(); }
		EVO_NODISCARD auto isBuiltinType() const -> bool { return this->sourceID.is<BuiltinModuleID>(); }
		
		EVO_NODISCARD auto getName(const class panther::SourceManager& source_manager) const -> std::string_view;
		EVO_NODISCARD auto getParamName(
			const Param& param, const class panther::SourceManager& source_manager
		) const -> std::string_view;

		EVO_NODISCARD auto isEquivalentOverload(const Func& rhs, const class panther::Context& context) const -> bool;

		EVO_NODISCARD auto isMethod(const class panther::Context& context) const -> bool;
	};




	struct TemplatedFunc{
		using ID = TemplatedFuncID;
		using Arg = evo::Variant<TypeInfo::VoidableID, core::GenericValue>;

		struct Instantiation{
			struct ErroredReasonParamDeductionFailed{
				size_t arg_index;
			};

			struct ErroredReasonArgTypeMismatch{
				size_t arg_index;
				TypeInfo::ID expected_type_id;
				TypeInfo::ID got_type_id;
			};

			struct ErroredReasonTypeDoesntImplInterface{
				size_t arg_index;
				TypeInfo::ID interface_type_id;
				TypeInfo::ID got_type_id;
			};

			struct ErroredReasonErroredAfterDecl{};

			using ErroredReason = evo::Variant<
				std::monostate, // not errored
				ErroredReasonParamDeductionFailed,
				ErroredReasonArgTypeMismatch,
				ErroredReasonTypeDoesntImplInterface,
				ErroredReasonErroredAfterDecl
			>;

			std::atomic<std::optional<SymbolProcID>> symbolProcID{}; // nullopt means its being generated
			std::optional<Func::ID> funcID{}; // nullopt means it's being worked on

			ErroredReason errored_reason = std::monostate();

			EVO_NODISCARD auto errored() const -> bool { return this->errored_reason.is<std::monostate>() == false; }

			Instantiation() = default;
			Instantiation(const Instantiation&) = delete;
			Instantiation(Instantiation&&) = delete;
		};

		struct TemplateParam{
			std::optional<TypeInfo::ID> typeID;
			evo::Variant<std::monostate, Expr, TypeInfo::VoidableID> defaultValue;
		};

		SymbolProc& symbolProc;
		size_t minNumTemplateArgs;
		evo::SmallVector<TemplateParam> templateParams;
		evo::SmallVector<bool> paramIsTemplate; // if a func param is an interface or a type deducer

		struct InstantiationInfo{
			Instantiation& instantiation;
			std::optional<uint32_t> instantiationID; // only has value if it needs to be compiled

			EVO_NODISCARD auto needsToBeCompiled() const -> bool { return this->instantiationID.has_value(); }
		};
		EVO_NODISCARD auto createOrLookupInstantiation(evo::SmallVector<Arg>&& args) -> InstantiationInfo;

		EVO_NODISCARD auto hasAnyDefaultParams() const -> bool {
			return this->minNumTemplateArgs != this->templateParams.size();
		}

		EVO_NODISCARD auto isMethod(const class panther::Context& context) const -> bool;


		TemplatedFunc(
			SymbolProc& symbol_proc,
			size_t min_num_template_args,
			evo::SmallVector<TemplateParam>&& template_params,
			evo::SmallVector<bool>&& param_is_template
		) : 
			symbolProc(symbol_proc), 
			minNumTemplateArgs(min_num_template_args),
			templateParams(std::move(template_params)),
			paramIsTemplate(std::move(param_is_template))
		{}

		private:
			core::LinearStepAlloc<Instantiation, size_t> instantiations{};
			std::unordered_map<evo::SmallVector<Arg>, Instantiation&> instantiation_map{};
			mutable evo::SpinLock instantiation_lock{};
	};


	struct TemplatedStruct{
		using ID = TemplatedStructID;

		BaseType::StructTemplate::ID templateID;
		SymbolProc& symbolProc;
	};


}
