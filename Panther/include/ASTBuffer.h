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


			EVO_NODISCARD auto numGlobalStmts() const noexcept -> size_t { return this->global_stmts.size(); };
			EVO_NODISCARD auto getGlobalStmts() const noexcept -> evo::ArrayProxy<AST::Node> {
				return this->global_stmts;
			};


			EVO_NODISCARD auto getLiteral(const AST::Node& node) const noexcept -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::Literal, "Node is not a Literal");
				return node.value.token_id;
			};

			EVO_NODISCARD auto getIdent(const AST::Node& node) const noexcept -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::Ident, "Node is not a Ident");
				return node.value.token_id;
			};

			EVO_NODISCARD auto getIntrinsic(const AST::Node& node) const noexcept -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::Intrinsic, "Node is not a Intrinsic");
				return node.value.token_id;
			};

			EVO_NODISCARD auto getBuiltinType(const AST::Node& node) const noexcept -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::BuiltinType, "Node is not a BuiltinType");
				return node.value.token_id;
			};

			EVO_NODISCARD auto getUninit(const AST::Node& node) const noexcept -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::Uninit, "Node is not a Uninit");
				return node.value.token_id;
			};



			EVO_NODISCARD auto createVarDecl(auto&&... args) noexcept -> AST::Node {
				const uint32_t node_index = uint32_t(this->var_decls.size());
				this->var_decls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::VarDecl, node_index);
			};
			EVO_NODISCARD auto getVarDecl(const AST::Node& node) const noexcept -> const AST::VarDecl& {
				evo::debugAssert(node.getKind() == AST::Kind::VarDecl, "Node is not a VarDecl");
				return this->var_decls[node.value.node_index];
			};

			EVO_NODISCARD auto createFuncDecl(auto&&... args) noexcept -> AST::Node {
				const uint32_t node_index = uint32_t(this->func_decls.size());
				this->func_decls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::FuncDecl, node_index);
			};
			EVO_NODISCARD auto getFuncDecl(const AST::Node& node) const noexcept -> const AST::FuncDecl& {
				evo::debugAssert(node.getKind() == AST::Kind::FuncDecl, "Node is not a FuncDecl");
				return this->func_decls[node.value.node_index];
			};


			EVO_NODISCARD auto createBlock(auto&&... args) noexcept -> AST::Node {
				const uint32_t node_index = uint32_t(this->blocks.size());
				this->blocks.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Block, node_index);
			};
			EVO_NODISCARD auto getBlock(const AST::Node& node) const noexcept -> const AST::Block& {
				evo::debugAssert(node.getKind() == AST::Kind::Block, "Node is not a VarDecl");
				return this->blocks[node.value.node_index];
			};



			EVO_NODISCARD auto createPrefix(auto&&... args) noexcept -> AST::Node {
				const uint32_t node_index = uint32_t(this->prefixes.size());
				this->prefixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Prefix, node_index);
			};
			EVO_NODISCARD auto getPrefix(const AST::Node& node) const noexcept -> const AST::Prefix& {
				evo::debugAssert(node.getKind() == AST::Kind::Prefix, "Node is not a Prefix");
				return this->prefixes[node.value.node_index];
			};

			EVO_NODISCARD auto createInfix(auto&&... args) noexcept -> AST::Node {
				const uint32_t node_index = uint32_t(this->infixes.size());
				this->infixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Infix, node_index);
			};
			EVO_NODISCARD auto getInfix(const AST::Node& node) const noexcept -> const AST::Infix& {
				evo::debugAssert(node.getKind() == AST::Kind::Infix, "Node is not a Infix");
				return this->infixes[node.value.node_index];
			};

			EVO_NODISCARD auto createPostfix(auto&&... args) noexcept -> AST::Node {
				const uint32_t node_index = uint32_t(this->postfixes.size());
				this->postfixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Postfix, node_index);
			};
			EVO_NODISCARD auto getPostfix(const AST::Node& node) const noexcept -> const AST::Postfix& {
				evo::debugAssert(node.getKind() == AST::Kind::Postfix, "Node is not a Postfix");
				return this->postfixes[node.value.node_index];
			};


			EVO_NODISCARD auto createType(auto&&... args) noexcept -> AST::Node {
				const uint32_t node_index = uint32_t(this->types.size());
				this->types.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Type, node_index);
			};
			EVO_NODISCARD auto getType(const AST::Node& node) const noexcept -> const AST::Type& {
				evo::debugAssert(node.getKind() == AST::Kind::Type, "Node is not a Type");
				return this->types[node.value.node_index];
			};


	
		private:
			// TODO: change these vectors to deques? some combination?

			evo::SmallVector<AST::Node> global_stmts{};

			evo::SmallVector<AST::VarDecl> var_decls{};
			evo::SmallVector<AST::FuncDecl> func_decls{};

			evo::SmallVector<AST::Block> blocks{};

			evo::SmallVector<AST::Prefix> prefixes{};
			evo::SmallVector<AST::Infix> infixes{};
			evo::SmallVector<AST::Postfix> postfixes{};

			evo::SmallVector<AST::Type> types{};



			friend class Parser;
	};


};
