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

			EVO_NODISCARD auto getBuiltinType(const AST::Node& node) const noexcept -> Token::ID {
				evo::debugAssert(node.getKind() == AST::Kind::BuiltinType, "Node is not a BuiltinType");

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
			// TODO: change these vectors to deques? to evo::SmallVectors? some combination?

			std::vector<AST::Node> global_stmts{};

			std::vector<AST::VarDecl> var_decls{};
			std::vector<AST::FuncDecl> func_decls{};

			std::vector<AST::Block> blocks{};
			std::vector<AST::Type> types{};



			friend class Parser;
	};


};
