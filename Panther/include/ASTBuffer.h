//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <deque>

#include <Evo.h>
#include <PCIT_core.h>

#include "./AST.h"


namespace pcit::panther{


	class ASTBuffer{
		public:
			ASTBuffer() = default;
			~ASTBuffer() = default;


			EVO_NODISCARD auto getGlobalStmts() const -> evo::ArrayProxy<AST::Node> { return this->global_stmts; }
			EVO_NODISCARD auto numGlobalStmts() const -> size_t { return this->global_stmts.size(); }


			EVO_NODISCARD auto getLiteral(const AST::Node& node) const -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::Literal, "Node is not a Literal");
				return node._value.token_id;
			}

			EVO_NODISCARD auto getIdent(const AST::Node& node) const -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::Ident, "Node is not a Ident");
				return node._value.token_id;
			}

			EVO_NODISCARD auto getIntrinsic(const AST::Node& node) const -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::Intrinsic, "Node is not a Intrinsic");
				return node._value.token_id;
			}

			EVO_NODISCARD auto getAttribute(const AST::Node& node) const -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::Attribute, "Node is not a Attribute");
				return node._value.token_id;
			}

			EVO_NODISCARD auto getBuiltinType(const AST::Node& node) const -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::BuiltinType, "Node is not a BuiltinType");
				return node._value.token_id;
			}

			EVO_NODISCARD auto getUninit(const AST::Node& node) const -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::Uninit, "Node is not a Uninit");
				return node._value.token_id;
			}

			EVO_NODISCARD auto getThis(const AST::Node& node) const -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::This, "Node is not a This");
				return node._value.token_id;
			}

			EVO_NODISCARD auto getDiscard(const AST::Node& node) const -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::Discard, "Node is not a Discard");
				return node._value.token_id;
			}



			EVO_NODISCARD auto createVarDecl(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->var_decls.size());
				this->var_decls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::VarDecl, node_index);
			}
			EVO_NODISCARD auto getVarDecl(const AST::Node& node) const -> const AST::VarDecl& {
				evo::debugAssert(node.getKind() == AST::Kind::VarDecl, "Node is not a VarDecl");
				return this->var_decls[node._value.node_index];
			}

			EVO_NODISCARD auto createFuncDecl(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->func_decls.size());
				this->func_decls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::FuncDecl, node_index);
			}
			EVO_NODISCARD auto getFuncDecl(const AST::Node& node) const -> const AST::FuncDecl& {
				evo::debugAssert(node.getKind() == AST::Kind::FuncDecl, "Node is not a FuncDecl");
				return this->func_decls[node._value.node_index];
			}

			EVO_NODISCARD auto createAliasDecl(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->alias_decls.size());
				this->alias_decls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::AliasDecl, node_index);
			}
			EVO_NODISCARD auto getAliasDecl(const AST::Node& node) const -> const AST::AliasDecl& {
				evo::debugAssert(node.getKind() == AST::Kind::AliasDecl, "Node is not a AliasDecl");
				return this->alias_decls[node._value.node_index];
			}


			EVO_NODISCARD auto createReturn(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->returns.size());
				this->returns.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Return, node_index);
			}
			EVO_NODISCARD auto getReturn(const AST::Node& node) const -> const AST::Return& {
				evo::debugAssert(node.getKind() == AST::Kind::Return, "Node is not a Return");
				return this->returns[node._value.node_index];
			}


			EVO_NODISCARD auto createBlock(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->blocks.size());
				this->blocks.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Block, node_index);
			}
			EVO_NODISCARD auto getBlock(const AST::Node& node) const -> const AST::Block& {
				evo::debugAssert(node.getKind() == AST::Kind::Block, "Node is not a VarDecl");
				return this->blocks[node._value.node_index];
			}

			EVO_NODISCARD auto createFuncCall(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->func_calls.size());
				this->func_calls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::FuncCall, node_index);
			}
			EVO_NODISCARD auto getFuncCall(const AST::Node& node) const -> const AST::FuncCall& {
				evo::debugAssert(node.getKind() == AST::Kind::FuncCall, "Node is not a FuncCall");
				return this->func_calls[node._value.node_index];
			}

			EVO_NODISCARD auto createTemplatePack(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->template_packs.size());
				this->template_packs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::TemplatePack, node_index);
			}
			EVO_NODISCARD auto getTemplatePack(const AST::Node& node) const -> const AST::TemplatePack& {
				evo::debugAssert(node.getKind() == AST::Kind::TemplatePack, "Node is not a TemplatePack");
				return this->template_packs[node._value.node_index];
			}

			EVO_NODISCARD auto createTemplatedExpr(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->templated_expr.size());
				this->templated_expr.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::TemplatedExpr, node_index);
			}
			EVO_NODISCARD auto getTemplatedExpr(const AST::Node& node) const -> const AST::TemplatedExpr& {
				evo::debugAssert(node.getKind() == AST::Kind::TemplatedExpr, "Node is not a TemplatedExpr");
				return this->templated_expr[node._value.node_index];
			}


			EVO_NODISCARD auto createPrefix(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->prefixes.size());
				this->prefixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Prefix, node_index);
			}
			EVO_NODISCARD auto getPrefix(const AST::Node& node) const -> const AST::Prefix& {
				evo::debugAssert(node.getKind() == AST::Kind::Prefix, "Node is not a Prefix");
				return this->prefixes[node._value.node_index];
			}

			EVO_NODISCARD auto createInfix(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->infixes.size());
				this->infixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Infix, node_index);
			}
			EVO_NODISCARD auto getInfix(const AST::Node& node) const -> const AST::Infix& {
				evo::debugAssert(node.getKind() == AST::Kind::Infix, "Node is not a Infix");
				return this->infixes[node._value.node_index];
			}

			EVO_NODISCARD auto createPostfix(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->postfixes.size());
				this->postfixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Postfix, node_index);
			}
			EVO_NODISCARD auto getPostfix(const AST::Node& node) const -> const AST::Postfix& {
				evo::debugAssert(node.getKind() == AST::Kind::Postfix, "Node is not a Postfix");
				return this->postfixes[node._value.node_index];
			}


			EVO_NODISCARD auto createMultiAssign(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->multi_assigns.size());
				this->multi_assigns.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::MultiAssign, node_index);
			}
			EVO_NODISCARD auto getMultiAssign(const AST::Node& node) const -> const AST::MultiAssign& {
				evo::debugAssert(node.getKind() == AST::Kind::MultiAssign, "Node is not a MultiAssign");
				return this->multi_assigns[node._value.node_index];
			}



			EVO_NODISCARD auto createType(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->types.size());
				this->types.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Type, node_index);
			}
			EVO_NODISCARD auto getType(const AST::Node& node) const -> const AST::Type& {
				evo::debugAssert(node.getKind() == AST::Kind::Type, "Node is not a Type");
				return this->types[node._value.node_index];
			}


			EVO_NODISCARD auto createAttributeBlock(auto&&... args) -> AST::Node {
				const uint32_t node_index = uint32_t(this->attribute_blocks.size());
				this->attribute_blocks.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::AttributeBlock, node_index);
			}
			EVO_NODISCARD auto getAttributeBlock(const AST::Node& node) const -> const AST::AttributeBlock& {
				evo::debugAssert(node.getKind() == AST::Kind::AttributeBlock, "Node is not a AttributeBlock");
				return this->attribute_blocks[node._value.node_index];
			}


	
		private:
			evo::SmallVector<AST::Node> global_stmts{};

			evo::SmallVector<AST::VarDecl> var_decls{};
			evo::SmallVector<AST::FuncDecl> func_decls{};
			evo::SmallVector<AST::AliasDecl> alias_decls{};

			evo::SmallVector<AST::Return> returns{};

			evo::SmallVector<AST::Block> blocks{};
			evo::SmallVector<AST::FuncCall> func_calls{};
			evo::SmallVector<AST::TemplatePack> template_packs{};
			evo::SmallVector<AST::TemplatedExpr> templated_expr{};

			evo::SmallVector<AST::Prefix> prefixes{};
			evo::SmallVector<AST::Infix> infixes{};
			evo::SmallVector<AST::Postfix> postfixes{};

			evo::SmallVector<AST::MultiAssign> multi_assigns{};

			evo::SmallVector<AST::Type> types{};

			evo::SmallVector<AST::AttributeBlock> attribute_blocks{};


			friend class Parser;
	};


}
