////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <deque>

#include <Evo.hpp>
#include <PCIT_core.hpp>

#include "./AST.hpp"


namespace pcit::panther{
	class Parser;
}


namespace pcit::panther::AST{


	class ASTBuffer{
		public:
			ASTBuffer();
			~ASTBuffer();


			[[nodiscard]] auto getGlobalStmts() const -> evo::ArrayProxy<Node>;
			[[nodiscard]] auto numGlobalStmts() const -> size_t;

			[[nodiscard]] static auto getIdent(const Node& node) -> Token::ID;
			[[nodiscard]] static auto getIntrinsic(const Node& node) -> Token::ID;
			[[nodiscard]] static auto getTypeThis(const Node& node) -> Token::ID;
			[[nodiscard]] static auto getLiteral(const Node& node) -> Token::ID;
			[[nodiscard]] static auto getAttribute(const Node& node) -> Token::ID;
			[[nodiscard]] static auto getPrimitiveType(const Node& node) -> Token::ID;
			[[nodiscard]] static auto getDeducer(const Node& node) -> Token::ID;
			[[nodiscard]] static auto getUninit(const Node& node) -> Token::ID;
			[[nodiscard]] static auto getZeroinit(const Node& node) -> Token::ID;
			[[nodiscard]] static auto getThis(const Node& node) -> Token::ID;
			[[nodiscard]] static auto getDiscard(const Node& node) -> Token::ID;


			[[nodiscard]] auto createVarDef(
				VarDef::Kind kind,
				Token::ID ident,
				std::optional<Node> type,
				Node attributeBlock,
				VarDef::ValueKind valueKind,
				std::optional<Node> value
			) -> Node;
			[[nodiscard]] auto getVarDef(const Node& node) const -> const VarDef&;


			[[nodiscard]] auto createFuncDef(
				Token::ID name, // either identifier or operator
				std::optional<Node> templatePack,
				evo::SmallVector<FuncDef::Param>&& params,
				bool isVariadic,
				FuncDef::Kind kind,
				Node attributeBlock,
				evo::SmallVector<FuncDef::Return>&& _returns,
				evo::SmallVector<FuncDef::Return>&& errorReturns,
				std::optional<Node> value // if Kind::DEF, nullopt if is an interface method with no default implementation
				                           // if Kind::EXTERN, always contains language
				                           // if Kind::DELETED, delete message (if one was provided)
			) -> Node;
			[[nodiscard]] auto getFuncDef(const Node& node) const -> const FuncDef&;


			[[nodiscard]] auto createDeletedSpecialMethod(Token::ID memberToken, std::optional<Node> message) -> Node;
			[[nodiscard]] auto getDeletedSpecialMethod(const Node& node) const -> const DeletedSpecialMethod&;

			[[nodiscard]] auto createFuncAliasDef(Token::ID ident, Node attributeBlock, Node type) -> Node;
			[[nodiscard]] auto getFuncAliasDef(const Node& node) const -> const FuncAliasDef&;

			[[nodiscard]] auto createAliasDef(Token::ID ident, Node attributeBlock, Node type) -> Node;
			[[nodiscard]] auto getAliasDef(const Node& node) const -> const AliasDef&;


			[[nodiscard]] auto createStructDef(
				Token::ID ident, std::optional<Node> templatePack, Node attributeBlock, Node block
			) -> Node;
			[[nodiscard]] auto getStructDef(const Node& node) const -> const StructDef&;


			[[nodiscard]] auto createUnionDef(
				Token::ID ident,
				Node attributeBlock,
				evo::SmallVector<UnionDef::Field>&& fields,
				evo::SmallVector<Node>&& statements
			) -> Node;
			[[nodiscard]] auto getUnionDef(const Node& node) const -> const UnionDef&;


			[[nodiscard]] auto createEnumDef(
				Token::ID ident,
				std::optional<Node> underlyingType,
				Node attributeBlock,
				evo::SmallVector<EnumDef::Enumerator>&& enumerators,
				evo::SmallVector<Node>&& statements
			) -> Node;
			[[nodiscard]] auto getEnumDef(const Node& node) const -> const EnumDef&;


			[[nodiscard]] auto createInterfaceDef(
				Token::ID ident,
				Node attributeBlock,
				evo::SmallVector<Node>&& methods, // Nodes are all FuncDef
				evo::SmallVector<Node>&& impls // Nodes are all InterfaceImpl
			) -> Node;
			[[nodiscard]] auto getInterfaceDef(const Node& node) const -> const InterfaceDef&;


			[[nodiscard]] auto createInterfaceImpl(
				Node target, Node attributeBlock, evo::SmallVector<InterfaceImpl::Method>&& methods
			) -> Node;
			[[nodiscard]] auto getInterfaceImpl(const Node& node) const -> const InterfaceImpl&;


			[[nodiscard]] auto createReturn(
				Token::ID keyword,
				std::optional<Node> label,
				evo::Variant<std::monostate, Node, Token::ID> value // std::monostate == return; Token::ID == return...;
			) -> Node;
			[[nodiscard]] auto getReturn(const Node& node) const -> const Return&;


			[[nodiscard]] auto createError(
				Token::ID keyword,
				evo::Variant<std::monostate, Node, Token::ID> value // std::monostate == error; Token::ID == error...;
			) -> Node;
			[[nodiscard]] auto getError(const Node& node) const -> const Error&;


			[[nodiscard]] auto createUnreachable(Token::ID keyword, std::optional<Node> message) -> Node;
			[[nodiscard]] auto getUnreachable(const Node& node) const -> const Unreachable&;

			[[nodiscard]] auto createBreak(Token::ID keyword, std::optional<Token::ID> label) -> Node;
			[[nodiscard]] auto getBreak(const Node& node) const -> const Break&;

			[[nodiscard]] auto createContinue(Token::ID keyword, std::optional<Token::ID> label) -> Node;
			[[nodiscard]] auto getContinue(const Node& node) const -> const Continue&;

			[[nodiscard]] auto createDelete(Token::ID keyword, Node value) -> Node;
			[[nodiscard]] auto getDelete(const Node& node) const -> const Delete&;


			[[nodiscard]] auto createConditional(
				Token::ID ifToken,
				std::optional<Token::ID> elseToken,
				Token::ID closeBraceToken,
				Node cond,
				Node thenBlock,
				std::optional<Node> elseBlock // either `Block` or `Conditional`
			) -> Node;
			[[nodiscard]] auto getConditional(const Node& node) const -> const Conditional&;


			[[nodiscard]] auto createWhenConditional(
				Token::ID keyword,
				Node cond,
				Node thenBlock,
				std::optional<Node> elseBlock // either `Block` or `WhenConditional`
			) -> Node;
			[[nodiscard]] auto getWhenConditional(const Node& node) const -> const WhenConditional&;


			[[nodiscard]] auto createWhile(Token::ID keyword, Node cond, Node block) -> Node;
			[[nodiscard]] auto getWhile(const Node& node) const -> const While&;


			[[nodiscard]] auto createFor(
				Token::ID keyword,
				evo::SmallVector<Node>&& iterables,
				std::optional<For::Param> index, // nullopt means `_`
				evo::SmallVector<For::Param>&& values,
				Node attributeBlock,
				Node block
			) -> Node;
			[[nodiscard]] auto getFor(const Node& node) const -> const For&;


			[[nodiscard]] auto createSwitch(
				Token::ID keyword,
				Token::ID closeBrace,
				Node cond,
				Node attributeBlock,
				evo::SmallVector<Switch::Case>&& cases
			) -> Node;
			[[nodiscard]] auto getSwitch(const Node& node) const -> const Switch&;


			[[nodiscard]] auto createWhenSwitch(
				Token::ID keyword,
				Token::ID closeBrace,
				Node cond,
				Node attributeBlock,
				evo::SmallVector<WhenSwitch::Case>&& cases
			) -> Node;
			[[nodiscard]] auto getWhenSwitch(const Node& node) const -> const WhenSwitch&;


			[[nodiscard]] auto createDefer(Token::ID keyword, Node block) -> Node;
			[[nodiscard]] auto getDefer(const Node& node) const -> const Defer&;


			[[nodiscard]] auto createBlock(
				Token::ID openBrace,
				Token::ID closeBrace,
				std::optional<Token::ID> label,
				evo::SmallVector<Block::Output>&& outputs, // only used if `.label` has value
				evo::SmallVector<Node>&& statements
			) -> Node;
			[[nodiscard]] auto getBlock(const Node& node) const -> const Block&;


			[[nodiscard]] auto createFuncCall(Node target, evo::SmallVector<FuncCall::Arg>&& args) -> Node;
			[[nodiscard]] auto getFuncCall(const Node& node) const -> const FuncCall&;


			[[nodiscard]] auto createIndexer(Node target, evo::SmallVector<Node>&& indices, Token::ID openBracket)
				-> Node;
			[[nodiscard]] auto getIndexer(const Node& node) const -> const Indexer&;


			[[nodiscard]] auto createTemplatePack(evo::SmallVector<TemplatePack::Param>&& params) -> Node;
			[[nodiscard]] auto getTemplatePack(const Node& node) const -> const TemplatePack&;

			[[nodiscard]] auto createTemplatedExpr(Node base, evo::SmallVector<Node>&& args) -> Node;
			[[nodiscard]] auto getTemplatedExpr(const Node& node) const -> const TemplatedExpr&;

			[[nodiscard]] auto createPrefix(Token::ID opTokenID, Node rhs) -> Node;
			[[nodiscard]] auto getPrefix(const Node& node) const -> const Prefix&;

			[[nodiscard]] auto createInfix(Node lhs, Token::ID opTokenID, Node rhs) -> Node;
			[[nodiscard]] auto getInfix(const Node& node) const -> const Infix&;

			[[nodiscard]] auto createPostfix(Node lhs, Token::ID opTokenID) -> Node;
			[[nodiscard]] auto getPostfix(const Node& node) const -> const Postfix&;


			[[nodiscard]] auto createMultiAssign(
				Token::ID openBracketLocation, evo::SmallVector<Node>&& assigns, Node value
			) -> Node;
			[[nodiscard]] auto getMultiAssign(const Node& node) const -> const MultiAssign&;


			[[nodiscard]] auto createNew(Token::ID keyword, Node type, evo::SmallVector<FuncCall::Arg>&& args) -> Node;
			[[nodiscard]] auto getNew(const Node& node) const -> const New&;


			[[nodiscard]] auto createArrayInitNew(Token::ID keyword, Node type, evo::SmallVector<Node>&& values)
				-> Node;
			[[nodiscard]] auto getArrayInitNew(const Node& node) const -> const ArrayInitNew&;


			[[nodiscard]] auto createDesignatedInitNew(
				Token::ID keyword, Node type, evo::SmallVector<DesignatedInitNew::MemberInit>&& memberInits
			) -> Node;
			[[nodiscard]] auto getDesignatedInitNew(const Node& node) const -> const DesignatedInitNew&;


			[[nodiscard]] auto createTryElse(
				Node attemptExpr,
				Node exceptExpr,
				evo::SmallVector<Token::ID>&& exceptParams,
				Token::ID elseTokenID,
				std::optional<Token::ID> semicolonTokenID // nullopt of expr
			) -> Node;
			[[nodiscard]] auto getTryElse(const Node& node) const -> const TryElse&;


			[[nodiscard]] auto createUnsafe(Token::ID keyword, Node block) -> Node;
			[[nodiscard]] auto getUnsafe(const Node& node) const -> const Unsafe&;


			[[nodiscard]] auto createAsm(
				Token::ID startToken,
				Token::ID asmStr,
				evo::SmallVector<Asm::Param>&& params,
				Node attributeBlock,
				evo::SmallVector<Asm::RetParam>&& retParams // empty if `Void`
			) -> Node;
			[[nodiscard]] auto getAsm(const Node& node) const -> const Asm&;


			[[nodiscard]] auto createArrayType(
				Token::ID openBracket,
				Node elemType,
				evo::SmallVector<std::optional<Node>>&& dimensions, // element is nullopt if dimension is ptr
				std::optional<Node> terminator,
				std::optional<bool> refIsMut // only has value if is array ref
			) -> Node;
			[[nodiscard]] auto getArrayType(const Node& node) const -> const ArrayType&;


			[[nodiscard]] auto createFuncType(
				Token::ID funcKeyword,
				evo::SmallVector<FuncType::Param>&& params,
				Node attributeBlock,
				evo::SmallVector<Node>&& returnTypes,
				evo::SmallVector<Node>&& errorTypes,
				bool hasNamedReturns
			) -> Node;
			[[nodiscard]] auto getFuncType(const Node& node) const -> const FuncType&;


			[[nodiscard]] auto createInterfaceMap(
				evo::Variant<InterfaceMap::Ptr, InterfaceMap::PtrDeducer, Node> underlyingType,
				Token::ID colonToken,
				Node interface
			) -> Node;
			[[nodiscard]] auto getInterfaceMap(const Node& node) const -> const InterfaceMap&;


			[[nodiscard]] auto createType(Node base, evo::SmallVector<Type::Qualifier>&& qualifiers) -> Node;
			[[nodiscard]] auto getType(const Node& node) const -> const Type&;

			[[nodiscard]] auto createTypeIDConverter(Token::ID keyword, Node expr) -> Node;
			[[nodiscard]] auto getTypeIDConverter(const Node& node) const -> const TypeIDConverter&;

			[[nodiscard]] auto createAttributeBlock(evo::SmallVector<AttributeBlock::Attribute>&& attributes) -> Node;
			[[nodiscard]] auto getAttributeBlock(const Node& node) const -> const AttributeBlock&;


			auto lock() -> void;
			[[nodiscard]] auto isLocked() const -> bool;

		private:
			[[nodiscard]] auto getMutGlobalStmts() -> evo::SmallVector<Node>&;

	
		private:
			struct Internal;
			Internal* internal;

			bool is_locked = false;

			friend class Parser;
	};


}
