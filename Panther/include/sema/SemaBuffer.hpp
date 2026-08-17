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

#include "./sema.hpp"


namespace pcit::panther::sema{

	class ScopeManager;

	class SemaBuffer{
		public:
			SemaBuffer();
			~SemaBuffer();


			[[nodiscard]] auto getScopeManager() const -> const class sema::ScopeManager&;
			[[nodiscard]] auto getScopeManager()       ->       class sema::ScopeManager&;


			///////////////////////////////////
			// funcs

			[[nodiscard]] auto createFunc(
				evo::Variant<SourceID, CFamilySourceID, BuiltinModuleID> sourceID,
				Func::Name name,
				std::string&& cFamilyMangledName, // empty if not c-family
				std::optional<EncapsulatingSymbolID> parent,
				BaseType::Function::ID typeID,
				evo::SmallVector<Func::Param>&& params,
				evo::SmallVector<Token::ID>&& returnParamIdents, // empty if not named
				evo::SmallVector<Token::ID>&& errorParamIdents,  // empty if not named
				std::optional<SymbolProcID> symbolProcID, // only value if is sema src and not auto-generated
				uint32_t minNumArgs,
				bool hasInParam,
				Func::Attributes attributes,
				std::optional<sema::TemplatedFuncID> templated_func_id = std::nullopt,
				uint32_t instanceID = std::numeric_limits<uint32_t>::max() // max if not an instantiation
			) -> Func::ID;

			[[nodiscard]] auto getFunc(Func::ID id) const -> const Func&;


			[[nodiscard]] auto getFuncs() const -> evo::IterRange<Func::ID::Iterator>;;

			[[nodiscard]] auto numFuncs() const -> size_t;


			///////////////////////////////////
			// func alias

			[[nodiscard]] auto createFuncAlias(
				SourceID sourceID,
				Token::ID ident,
				std::optional<EncapsulatingSymbolID> parent,
				evo::SmallVector<evo::Variant<sema::FuncID, sema::TemplatedFuncID>>&& aliasedOverloads,
				bool isPub,
				bool isPriv
			) -> FuncAlias::ID;

			[[nodiscard]] auto getFuncAlias(FuncAlias::ID id) const -> const FuncAlias&;


			///////////////////////////////////
			// templated funcs

			[[nodiscard]] auto createTemplatedFunc(
				SymbolProc& symbol_proc,
				size_t min_num_template_args,
				evo::SmallVector<TemplatedFunc::TemplateParam>&& template_params,
				evo::SmallVector<bool>&& param_is_deducer,
				bool is_variadic
			) -> TemplatedFunc::ID;

			[[nodiscard]] auto getTemplatedFunc(TemplatedFunc::ID id) const -> const TemplatedFunc&;


			///////////////////////////////////
			// templated structs

			[[nodiscard]] auto createTemplatedStruct(BaseType::StructTemplate::ID templateID, SymbolProc& symbolProc)
				-> TemplatedStruct::ID;

			[[nodiscard]] auto getTemplatedStruct(TemplatedStruct::ID id) const -> const TemplatedStruct&;


			///////////////////////////////////
			// struct template alias

			[[nodiscard]] auto createStructTemplateAlias(
				SourceID sourceID,
				Token::ID ident,
				std::optional<EncapsulatingSymbolID> parent,
				evo::Variant<TemplatedStruct::ID, StructTemplateAlias::ID> aliasedID,
				bool requiresPub,
				bool isDistinct,
				bool isPub,
				bool isPriv
			) -> StructTemplateAlias::ID;

			[[nodiscard]] auto getStructTemplateAlias(StructTemplateAlias::ID id) const -> const StructTemplateAlias&;


			///////////////////////////////////
			// vars

			[[nodiscard]] auto createVar(
				AST::VarDef::Kind kind,
				Token::ID ident,
				Expr expr,
				std::optional<TypeInfo::ID> typeID, // is nullopt iff (kind == `def` && is fluid)
				uint32_t line = 0, // 0 if unused (must be both line and collumn)
				uint32_t collumn = 0 // 0 if unused (must be both line and collumn)
			) -> Var::ID;

			[[nodiscard]] auto getVar(Var::ID id) const -> const Var&;

			[[nodiscard]] auto getVars() const -> evo::IterRange<Var::ID::Iterator>;

			[[nodiscard]] auto numVars() const -> size_t;


			///////////////////////////////////
			// global vars

			[[nodiscard]] auto createGlobalVar(
				AST::VarDef::Kind kind,
				evo::Variant<SourceID, CFamilySourceID, BuiltinModuleID> sourceID,
				evo::Variant<Token::ID, CFamilySourceDeclInfoID, BuiltinModuleStringID> ident,
				std::string cFamilyMangledName, // empty if not c-family
				std::optional<EncapsulatingSymbolID> parent,
				evo::Variant<std::monostate, Expr, GlobalVar::DeletedInfo> value, // monostate if def not done, or c-family
				std::optional<TypeInfo::ID> typeID, // is nullopt iff (kind == `def` && is fluid)
				bool isPub,
				bool isPriv,
				std::optional<SymbolProcID> symbolProcID,
				bool defCompleted = false
			) -> GlobalVar::ID;

			[[nodiscard]] auto getGlobalVar(GlobalVar::ID id) const -> const GlobalVar&;

			[[nodiscard]] auto getGlobalVars() const -> evo::IterRange<GlobalVar::ID::Iterator>;

			[[nodiscard]] auto numGlobalVars() const -> size_t;


			///////////////////////////////////
			// params

			[[nodiscard]] auto createParam(uint32_t index, uint32_t abiIndex) -> Param::ID;

			[[nodiscard]] auto getParam(Param::ID id) const -> const Param&;


			///////////////////////////////////
			// variadic params

			[[nodiscard]] auto createVariadicParam(uint32_t startIndex, uint32_t startABIIndex, uint32_t numParams)
			-> VariadicParam::ID;

			[[nodiscard]] auto getVariadicParam(VariadicParam::ID id) const -> const VariadicParam&;


			///////////////////////////////////
			// return params

			[[nodiscard]] auto createReturnParam(uint32_t index, uint32_t abiIndex) -> ReturnParam::ID;

			[[nodiscard]] auto getReturnParam(ReturnParam::ID id) const -> const ReturnParam&;


			///////////////////////////////////
			// error return params

			[[nodiscard]] auto createErrorReturnParam(uint32_t index, uint32_t abiIndex) -> ErrorReturnParam::ID;

			[[nodiscard]] auto getErrorReturnParam(ErrorReturnParam::ID id) const -> const ErrorReturnParam&;


			///////////////////////////////////
			// block expr outputs

			[[nodiscard]] auto createBlockExprOutput(
				uint32_t index, Token::ID label, Token::ID ident, TypeInfo::ID typeID
			) -> BlockExprOutput::ID;

			[[nodiscard]] auto getBlockExprOutput(BlockExprOutput::ID id) const -> const BlockExprOutput&;


			///////////////////////////////////
			// except param

			[[nodiscard]] auto createExceptParam(Token::ID ident, uint32_t index, TypeInfo::ID typeID)
				-> ExceptParam::ID;

			[[nodiscard]] auto getExceptParam(ExceptParam::ID id) const -> const ExceptParam&;


			///////////////////////////////////
			// for param

			[[nodiscard]] auto createForParam(Token::ID ident, TypeInfo::ID typeID, bool isIndex, bool isMut)
				-> ForParam::ID;

			[[nodiscard]] auto getForParam(ForParam::ID id) const -> const ForParam&;


			///////////////////////////////////
			// func calls

			[[nodiscard]] auto createFuncCall(
				evo::Variant<
					FuncID, IntrinsicFunc::Kind, TemplateIntrinsicFuncInstantiationID, FuncCall::FuncPtr
				> target,
				evo::SmallVector<Expr>&& args,
				uint32_t line,
				uint32_t collumn
			) -> FuncCall::ID;

			[[nodiscard]] auto getFuncCall(FuncCall::ID id) const -> const FuncCall&;


			///////////////////////////////////
			// try else

			[[nodiscard]] auto createTryElse(
				evo::Variant<FuncID, TryElse::FuncPtr> target,
				evo::SmallVector<Expr>&& args,
				evo::SmallVector<ExceptParamID>&& exceptParams,
				StmtBlock&& elseBlock,
				uint32_t line,
				uint32_t collumn
			) -> TryElse::ID;

			[[nodiscard]] auto getTryElse(TryElse::ID id) const -> const TryElse&;


			///////////////////////////////////
			// try else interface

			[[nodiscard]] auto createTryElseInterface(
				Expr value,
				BaseType::Function::ID funcTypeID,
				BaseType::Interface::ID interfaceID,
				uint32_t vtableFuncIndex,
				evo::SmallVector<Expr>&& args,
				evo::SmallVector<ExceptParamID>&& exceptParams,
				StmtBlock&& elseBlock,
				uint32_t line,
				uint32_t collumn
			) -> TryElseInterface::ID;

			[[nodiscard]] auto getTryElseInterface(TryElseInterface::ID id) const -> const TryElseInterface&;


			///////////////////////////////////
			// asm

			[[nodiscard]] auto createAsm(
				std::string_view code,
				evo::SmallVector<Asm::Param>&& params,
				evo::SmallVector<std::string_view>&& clobbers,
				evo::SmallVector<Asm::RetParam>&& retParams,
				bool isSideEffect,
				bool isAlignStack,
				uint32_t line = 0, // 0 if unused (must be both line and collumn)
				uint32_t collumn = 0 // 0 if unused (must be both line and collumn)
			) -> Asm::ID;

			[[nodiscard]] auto getAsm(Asm::ID id) const -> const Asm&;


			///////////////////////////////////
			// assignments

			[[nodiscard]] auto createAssign(
				std::optional<Expr> lhs, // nullopt if is a discard
				Expr rhs,
				uint32_t line = 0, // 0 if unused (must be both line and collumn)
				uint32_t collumn = 0 // 0 if unused (must be both line and collumn)
			) -> Assign::ID;

			[[nodiscard]] auto getAssign(Assign::ID id) const -> const Assign&;


			///////////////////////////////////
			// multi-assign

			[[nodiscard]] auto createMultiAssign(
				evo::SmallVector<evo::Variant<Expr, TypeInfo::ID>>&& targets, // TypeInfo::ID if is a discard
				Expr value,
				uint32_t line = 0, // 0 if unused (must be both line and collumn)
				uint32_t collumn = 0 // 0 if unused (must be both line and collumn)
			) -> MultiAssign::ID;

			[[nodiscard]] auto getMultiAssign(MultiAssign::ID id) const -> const MultiAssign&;


			///////////////////////////////////
			// returns

			[[nodiscard]] auto createReturn(
				std::optional<Expr> value, // nullopt means return void
				std::optional<Token::ID> targetLabel,
				uint32_t line = 0, // 0 if unused (must be both line and collumn)
				uint32_t collumn = 0 // 0 if unused (must be both line and collumn)
			) -> Return::ID;

			[[nodiscard]] auto getReturn(Return::ID id) const -> const Return&;


			///////////////////////////////////
			// errors

			[[nodiscard]] auto createError(
				std::optional<Expr> value, // nullopt means return void
				uint32_t line = 0, // 0 if unused (must be both line and collumn)
				uint32_t collumn = 0 // 0 if unused (must be both line and collumn)
			) -> Error::ID;

			[[nodiscard]] auto getError(Error::ID id) const -> const Error&;


			///////////////////////////////////
			// unreachables

			[[nodiscard]] auto createUnreachable(
				std::optional<Expr> message,
				uint32_t line = 0, // 0 if unused (must be both line and collumn)
				uint32_t collumn = 0 // 0 if unused (must be both line and collumn)
			) -> Unreachable::ID;

			[[nodiscard]] auto getUnreachable(Unreachable::ID id) const -> const Unreachable&;


			///////////////////////////////////
			// breaks

			[[nodiscard]] auto createBreak(std::optional<Token::ID> label) -> Break::ID;

			[[nodiscard]] auto getBreak(Break::ID id) const -> const Break&;


			///////////////////////////////////
			// continues

			[[nodiscard]] auto createContinue(std::optional<Token::ID> label) -> Continue::ID;

			[[nodiscard]] auto getContinue(Continue::ID id) const -> const Continue&;


			///////////////////////////////////
			// deletes

			[[nodiscard]] auto createDelete(
				Expr expr,
				TypeInfo::ID exprTypeID,
				uint32_t line = 0, // 0 if unused (must be both line and collumn)
				uint32_t collumn = 0 // 0 if unused (must be both line and collumn)
			) -> Delete::ID;

			[[nodiscard]] auto getDelete(Delete::ID id) const -> const Delete&;


			///////////////////////////////////
			// block scopes

			[[nodiscard]] auto createBlockScope(
				Token::ID openBrace, Token::ID closeBrace, StmtBlock&& block = StmtBlock{}
			) -> BlockScope::ID;

			[[nodiscard]] auto getBlockScope(BlockScope::ID id) const -> const BlockScope&;


			///////////////////////////////////
			// conditionals

			[[nodiscard]] auto createConditional(
				Expr cond,
				Token::ID ifToken,
				std::optional<Token::ID> elseToken,
				Token::ID closeBraceToken,
				StmtBlock&& thenStmts = StmtBlock{},
				StmtBlock&& elseStmts = StmtBlock{}
			) -> Conditional::ID;

			[[nodiscard]] auto getConditional(Conditional::ID id) const -> const Conditional&;


			///////////////////////////////////
			// whiles

			[[nodiscard]] auto createWhile(
				Expr cond, Token::ID whileToken, std::optional<Token::ID> label, StmtBlock&& block = StmtBlock{}
			) -> While::ID;

			[[nodiscard]] auto getWhile(While::ID id) const -> const While&;


			///////////////////////////////////
			// fors

			[[nodiscard]] auto createFor(
				evo::SmallVector<For::Iterable>&& iterables,
				Token::ID forToken,
				std::optional<Token::ID> label,
				bool hasIndex,
				evo::SmallVector<For::Param>&& params = evo::SmallVector<For::Param>{},
				StmtBlock&& block = StmtBlock{}
			) -> For::ID;

			[[nodiscard]] auto getFor(For::ID id) const -> const For&;

			///////////////////////////////////
			// for unrolls

			[[nodiscard]] auto createForUnroll(
				Token::ID forToken,
				std::optional<Token::ID> label,
				evo::SmallVector<StmtBlock>&& stmtBlocks = evo::SmallVector<StmtBlock>{}
			) -> ForUnroll::ID;

			[[nodiscard]] auto getForUnroll(ForUnroll::ID id) const -> const ForUnroll&;


			///////////////////////////////////
			// switches

			[[nodiscard]] auto createSwitch(
				Token::ID switchToken,
				TypeInfo::ID condTypeID,
				Expr cond,
				evo::SmallVector<Switch::Case>&& cases,
				Switch::Kind kind
			) -> Switch::ID;

			[[nodiscard]] auto getSwitch(Switch::ID id) const -> const Switch&;


			///////////////////////////////////
			// defers

			[[nodiscard]] auto createDefer(
				Token::ID deferToken,
				bool isErrorDefer,
				StmtBlock&& block = StmtBlock{}
			) -> Defer::ID;

			[[nodiscard]] auto getDefer(Defer::ID id) const -> const Defer&;


			///////////////////////////////////
			// lifetime start

			[[nodiscard]] auto createLifetimeStart(evo::Variant<Expr, OpDeleteThisAccessor> target, TypeInfo::ID typeID)
				-> LifetimeStart::ID;

			[[nodiscard]] auto getLifetimeStart(LifetimeStart::ID id) const -> const LifetimeStart&;


			///////////////////////////////////
			// lifetime end

			[[nodiscard]] auto createLifetimeEnd(evo::Variant<Expr, OpDeleteThisAccessor> target, TypeInfo::ID typeID)
				-> LifetimeEnd::ID;

			[[nodiscard]] auto getLifetimeEnd(LifetimeEnd::ID id) const -> const LifetimeEnd&;


			///////////////////////////////////
			// unused expr

			[[nodiscard]] auto createUnusedExpr(Expr expr) -> UnusedExpr::ID;

			[[nodiscard]] auto getUnusedExpr(UnusedExpr::ID id) const -> const UnusedExpr&;


			///////////////////////////////////
			// copies

			[[nodiscard]] auto createCopy(Expr expr, TypeInfo::ID exprTypeID, bool isInitialization) -> Copy::ID;

			[[nodiscard]] auto getCopy(Copy::ID id) const -> const Copy&;


			///////////////////////////////////
			// moves

			[[nodiscard]] auto createMove(Expr expr, TypeInfo::ID exprTypeID, bool isInitialization) -> Move::ID;

			[[nodiscard]] auto getMove(Move::ID id) const -> const Move&;


			///////////////////////////////////
			// forwards

			[[nodiscard]] auto createForward(Expr expr, TypeInfo::ID exprTypeID, bool isInitialization) -> Forward::ID;

			[[nodiscard]] auto getForward(Forward::ID id) const -> const Forward&;


			///////////////////////////////////
			// func ptrs

			[[nodiscard]] auto createFuncPtr(FuncID targetFuncID) -> FuncPtr::ID;

			[[nodiscard]] auto getFuncPtr(FuncPtr::ID id) const -> const FuncPtr&;


			///////////////////////////////////
			// address ofs

			[[nodiscard]] auto createAddrOf(Expr expr) -> AddrOf::ID;

			[[nodiscard]] auto getAddrOf(AddrOf::ID id) const -> const Expr&;


			///////////////////////////////////
			// optional null check

			[[nodiscard]] auto createConversionToOptional(Expr expr, TypeInfo::ID targetTypeID)
				-> ConversionToOptional::ID;

			[[nodiscard]] auto getConversionToOptional(ConversionToOptional::ID id) const
				-> const ConversionToOptional&;


			///////////////////////////////////
			// optional null check

			[[nodiscard]] auto createOptionalNullCheck(Expr expr, TypeInfo::ID targetTypeID, bool equal)
				-> OptionalNullCheck::ID;

			[[nodiscard]] auto getOptionalNullCheck(OptionalNullCheck::ID id) const -> const OptionalNullCheck&;


			///////////////////////////////////
			// optional extract

			[[nodiscard]] auto createOptionalExtract(Expr expr, TypeInfo::ID targetTypeID) -> OptionalExtract::ID;

			[[nodiscard]] auto getOptionalExtract(OptionalExtract::ID id) const -> const OptionalExtract&;



			///////////////////////////////////
			// dereferences

			[[nodiscard]] auto createDeref(Expr expr, TypeInfo::ID targetTypeID) -> Deref::ID;

			[[nodiscard]] auto getDeref(Deref::ID id) const -> const Deref&;


			///////////////////////////////////
			// unwraps

			[[nodiscard]] auto createUnwrap(Expr expr, TypeInfo::ID targetTypeID, bool isComptime) -> Unwrap::ID;

			[[nodiscard]] auto getUnwrap(Unwrap::ID id) const -> const Unwrap&;


			///////////////////////////////////
			// accessors

			[[nodiscard]] auto createAccessor(Expr target, TypeInfo::ID targetTypeID, uint32_t memberABIIndex)
				-> Accessor::ID;

			[[nodiscard]] auto getAccessor(Accessor::ID id) const -> const Accessor&;


			///////////////////////////////////
			// union accessors

			[[nodiscard]] auto createUnionAccessor(Expr target, TypeInfo::ID targetTypeID, uint32_t fieldIndex)
				-> UnionAccessor::ID;

			[[nodiscard]] auto getUnionAccessor(UnionAccessor::ID id) const -> const UnionAccessor&;


			///////////////////////////////////
			// logical and

			[[nodiscard]] auto createLogicalAnd(Expr lhs, Expr rhs) -> LogicalAnd::ID;

			[[nodiscard]] auto getLogicalAnd(LogicalAnd::ID id) const -> const LogicalAnd&;


			///////////////////////////////////
			// logical or

			[[nodiscard]] auto createLogicalOr(Expr lhs, Expr rhs) -> LogicalOr::ID;

			[[nodiscard]] auto getLogicalOr(LogicalOr::ID id) const -> const LogicalOr&;


			///////////////////////////////////
			// try/else expr

			[[nodiscard]] auto createTryElseExpr(
				Expr attempt,
				Expr except,
				evo::SmallVector<ExceptParamID>&& exceptParams,
				uint32_t line,
				uint32_t collumn
			) -> TryElseExpr::ID;

			[[nodiscard]] auto getTryElseExpr(TryElseExpr::ID id) const -> const TryElseExpr&;


			///////////////////////////////////
			// try/else interface expr

			[[nodiscard]] auto createTryElseInterfaceExpr(
				Expr attempt,
				Expr except,
				evo::SmallVector<ExceptParamID>&& exceptParams,
				uint32_t line,
				uint32_t collumn
			) -> TryElseInterfaceExpr::ID;

			[[nodiscard]] auto getTryElseInterfaceExpr(TryElseInterfaceExpr::ID id) const
				-> const TryElseInterfaceExpr&;


			///////////////////////////////////
			// block expr

			[[nodiscard]] auto createBlockExpr(
				Token::ID label,
				evo::SmallVector<BlockExpr::Output>&& outputs = evo::SmallVector<BlockExpr::Output>{},
				StmtBlock&& block = StmtBlock{}
			) -> BlockExpr::ID;

			[[nodiscard]] auto getBlockExpr(BlockExpr::ID id) const -> const BlockExpr&;


			///////////////////////////////////
			// fake term info

			[[nodiscard]] auto createFakeTermInfo(
				FakeTermInfo::ValueCategory valueCategory,
				FakeTermInfo::ValueState valueState,
				TypeInfo::ID typeID,
				Expr expr,
				bool isComptime
			) -> FakeTermInfo::ID;

			[[nodiscard]] auto getFakeTermInfo(FakeTermInfo::ID id) const -> const FakeTermInfo&;


			///////////////////////////////////
			// make interface ptr

			[[nodiscard]] auto createMakeInterfacePtr(
				Expr expr, BaseType::Interface::ID interfaceID, TypeInfo::ID implTypeID
			) -> MakeInterfacePtr::ID;

			[[nodiscard]] auto getMakeInterfacePtr(MakeInterfacePtr::ID id) const
			-> const MakeInterfacePtr&;


			///////////////////////////////////
			// interface ptr extract this

			[[nodiscard]] auto createInterfacePtrExtractThis(Expr expr) -> InterfacePtrExtractThis::ID;

			[[nodiscard]] auto getInterfacePtrExtractThis(InterfacePtrExtractThis::ID id) const
			-> const InterfacePtrExtractThis&;


			///////////////////////////////////
			// interface call

			[[nodiscard]] auto createInterfaceCall(
				Expr value,
				BaseType::Function::ID funcTypeID,
				BaseType::Interface::ID interfaceID,
				uint32_t vtableFuncIndex,
				evo::SmallVector<Expr>&& args
			) -> InterfaceCall::ID;

			[[nodiscard]] auto getInterfaceCall(InterfaceCall::ID id) const -> const InterfaceCall&;


			///////////////////////////////////
			// indexer

			[[nodiscard]] auto createIndexer(
				Expr target, TypeInfo::ID targetTypeID, evo::SmallVector<Expr>&& indices
			) -> Indexer::ID;

			[[nodiscard]] auto getIndexer(Indexer::ID id) const -> const Indexer&;


			///////////////////////////////////
			// default init

			[[nodiscard]] auto createDefaultNew(TypeInfo::ID targetTypeID, bool isInitialization) -> DefaultNew::ID;

			[[nodiscard]] auto getDefaultNew(DefaultNew::ID id) const -> const DefaultNew&;


			///////////////////////////////////
			// init array ref

			[[nodiscard]] auto createInitArrayRef(
				Expr expr,
				BaseType::ArrayRef::ID targetTypeID,
				evo::SmallVector<evo::Variant<uint64_t, Expr>>&& dimensions
			) -> InitArrayRef::ID;

			[[nodiscard]] auto getInitArrayRef(InitArrayRef::ID id) const -> const InitArrayRef&;


			///////////////////////////////////
			// array ref indexer

			[[nodiscard]] auto createArrayRefIndexer(
				Expr target, BaseType::ArrayRef::ID targetTypeID, evo::SmallVector<Expr>&& indices
			) -> ArrayRefIndexer::ID;

			[[nodiscard]] auto getArrayRefIndexer(ArrayRefIndexer::ID id) const -> const ArrayRefIndexer&;


			///////////////////////////////////
			// array ref size

			[[nodiscard]] auto createArrayRefSize(Expr target, BaseType::ArrayRef::ID targetTypeID)
				-> ArrayRefSize::ID;

			[[nodiscard]] auto getArrayRefSize(ArrayRefSize::ID id) const -> const ArrayRefSize&;


			///////////////////////////////////
			// array ref dimensions

			[[nodiscard]] auto createArrayRefDimensions(Expr target, BaseType::ArrayRef::ID targetTypeID)
				-> ArrayRefDimensions::ID;

			[[nodiscard]] auto getArrayRefDimensions(ArrayRefDimensions::ID id) const -> const ArrayRefDimensions&;


			///////////////////////////////////
			// array ref data

			[[nodiscard]] auto createArrayRefData(Expr target, BaseType::ArrayRef::ID targetTypeID) -> ArrayRefData::ID;

			[[nodiscard]] auto getArrayRefData(ArrayRefData::ID id) const -> const ArrayRefData&;


			///////////////////////////////////
			// union designated init new

			[[nodiscard]] auto createUnionDesignatedInitNew(
				Expr value, BaseType::Union::ID unionTypeID, uint32_t fieldIndex
			) -> UnionDesignatedInitNew::ID;

			[[nodiscard]] auto getUnionDesignatedInitNew(UnionDesignatedInitNew::ID id) const
				-> const UnionDesignatedInitNew&;


			///////////////////////////////////
			// union tag cmp

			[[nodiscard]] auto createUnionTagCmp(
				Expr value, BaseType::Union::ID unionTypeID, uint32_t fieldIndex, bool isEqual
			) -> UnionTagCmp::ID;

			[[nodiscard]] auto getUnionTagCmp(UnionTagCmp::ID id) const -> const UnionTagCmp&;


			///////////////////////////////////
			// same type cmp

			[[nodiscard]] auto createSameTypeCmp(TypeInfo::ID typeID, Expr lhs, Expr rhs, bool isEqual)
				-> SameTypeCmp::ID;

			[[nodiscard]] auto getSameTypeCmp(SameTypeCmp::ID id) const -> const SameTypeCmp&;


			///////////////////////////////////
			// template intrinsic instantiations

			[[nodiscard]] auto createTemplateIntrinsicFuncInstantiation(
				TemplateIntrinsicFunc::Kind kind,
				evo::SmallVector<evo::Variant<TypeInfo::VoidableID, core::GenericValue>>&& templateArgs
			) -> TemplateIntrinsicFuncInstantiation::ID;

			[[nodiscard]] auto getTemplateIntrinsicFuncInstantiation(TemplateIntrinsicFuncInstantiation::ID id) const
				-> const TemplateIntrinsicFuncInstantiation&;


			///////////////////////////////////
			// ints

			[[nodiscard]] auto createIntValue(core::GenericInt&& integer, std::optional<BaseType::ID> type_info_id)
			-> IntValue::ID;

			[[nodiscard]] auto getIntValue(IntValue::ID id) const -> const IntValue&;


			///////////////////////////////////
			// floats

			[[nodiscard]] auto createFloatValue(
				core::GenericFloat&& floating_point, std::optional<BaseType::ID> type_info_id
			) -> FloatValue::ID;

			[[nodiscard]] auto getFloatValue(FloatValue::ID id) const -> const FloatValue&;


			///////////////////////////////////
			// bools

			[[nodiscard]] auto createBoolValue(bool boolean, bool is_bool_32) -> BoolValue::ID;

			[[nodiscard]] auto getBoolValue(BoolValue::ID id) const -> const BoolValue&;


			///////////////////////////////////
			// strings

			[[nodiscard]] auto createStringValue(std::string&& value) -> StringValue::ID;

			[[nodiscard]] auto createStringValue(const std::string& value) -> StringValue::ID;

			[[nodiscard]] auto getStringValue(StringValue::ID id) const -> const StringValue&;


			///////////////////////////////////
			// aggregates

			[[nodiscard]] auto createAggregateValue(evo::SmallVector<Expr>&& values, BaseType::ID typeID)
				-> AggregateValue::ID;

			[[nodiscard]] auto createAggregateValue(const evo::SmallVector<Expr>& values, BaseType::ID typeID)
				-> AggregateValue::ID;

			[[nodiscard]] auto getAggregateValue(AggregateValue::ID id) const -> const AggregateValue&;


			///////////////////////////////////
			// chars

			[[nodiscard]] auto createCharValue(char character) -> CharValue::ID;

			[[nodiscard]] auto getCharValue(CharValue::ID id) const -> const CharValue&;


			///////////////////////////////////
			// null

			[[nodiscard]] auto createNull(Token::ID null_token_id) -> Null::ID;

			[[nodiscard]] auto getNull(Uninit::ID id) const -> Token::ID;


			///////////////////////////////////
			// uninit

			[[nodiscard]] auto createUninit(Token::ID uninit_token_id) -> Uninit::ID;

			[[nodiscard]] auto getUninit(Uninit::ID id) const -> Token::ID;


			///////////////////////////////////
			// zeroinit

			[[nodiscard]] auto createZeroinit(Token::ID zeroinit_token_id) -> Zeroinit::ID;

			[[nodiscard]] auto getZeroinit(Zeroinit::ID id) const -> Token::ID;


		// TODO(NOW): make private
		public:
			[[nodiscard]] auto getFunc(sema::Func::ID func_id) -> Func&;
			[[nodiscard]] auto getTemplatedFunc(sema::TemplatedFunc::ID templated_func_id) -> TemplatedFunc&;
			[[nodiscard]] auto getGlobalVar(sema::GlobalVar::ID global_var_id) -> GlobalVar&;
			[[nodiscard]] auto getTryElse(sema::TryElse::ID try_else_id) -> TryElse&;
			[[nodiscard]] auto getBlockScope(sema::BlockScope::ID block_scope_id) -> BlockScope&;
			[[nodiscard]] auto getConditional(sema::Conditional::ID cond_id) -> Conditional&;
			[[nodiscard]] auto getWhile(sema::While::ID while_id) -> While&;
			[[nodiscard]] auto getFor(sema::For::ID for_id) -> For&;
			[[nodiscard]] auto getForUnroll(sema::ForUnroll::ID for_unroll_id) -> ForUnroll&;
			[[nodiscard]] auto getSwitch(sema::Switch::ID switch_id) -> Switch&;
			[[nodiscard]] auto getDefer(sema::Defer::ID defer_id) -> Defer&;
			[[nodiscard]] auto getTryElseInterface(sema::TryElseInterface::ID try_else_interface_id)
				-> TryElseInterface&;
			[[nodiscard]] auto getBlockExpr(sema::BlockExpr::ID block_expr_id) -> BlockExpr&;
			[[nodiscard]] auto getCopy(sema::Copy::ID copy_id) -> Copy&;
			[[nodiscard]] auto getMove(sema::Move::ID move_id) -> Move&;
			[[nodiscard]] auto getForward(sema::Forward::ID forward_id) -> Forward&;
			[[nodiscard]] auto getDefaultNew(sema::DefaultNew::ID default_new_id) -> DefaultNew&;
			[[nodiscard]] auto getIntValue(sema::IntValue::ID int_value_id) -> IntValue&;
			[[nodiscard]] auto getFloatValue(sema::FloatValue::ID float_value_id) -> FloatValue&;
			[[nodiscard]] auto getBoolValue(sema::BoolValue::ID bool_value_id) -> BoolValue&;

	
		private:
			struct Internal;

			struct Internal* internal;

			// TODO(NOW): uncomment
			// friend class Context;
			// friend class SemanticAnalyzer;
	};


}
