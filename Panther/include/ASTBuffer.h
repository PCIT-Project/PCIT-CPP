////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


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


			EVO_NODISCARD static auto getIdent(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::Ident, "Node is not a Ident");
				return node._value.token_id;
			}

			EVO_NODISCARD static auto getIntrinsic(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::Intrinsic, "Node is not a Intrinsic");
				return node._value.token_id;
			}

			EVO_NODISCARD static auto getLiteral(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::Literal, "Node is not a Literal");
				return node._value.token_id;
			}

			EVO_NODISCARD static auto getAttribute(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::Attribute, "Node is not a Attribute");
				return node._value.token_id;
			}

			EVO_NODISCARD static auto getPrimitiveType(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::PrimitiveType, "Node is not a PrimitiveType");
				return node._value.token_id;
			}

			EVO_NODISCARD static auto getUninit(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::Uninit, "Node is not a Uninit");
				return node._value.token_id;
			}

			EVO_NODISCARD static auto getZeroinit(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::Zeroinit, "Node is not a Zeroinit");
				return node._value.token_id;
			}

			EVO_NODISCARD static auto getThis(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::This, "Node is not a This");
				return node._value.token_id;
			}

			EVO_NODISCARD static auto getDiscard(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::Discard, "Node is not a Discard");
				return node._value.token_id;
			}

			EVO_NODISCARD static auto getUnreachable(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::Unreachable, "Node is not a Unreachable");
				return node._value.token_id;
			}



			EVO_NODISCARD auto createVarDecl(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->var_decls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::VarDecl, node_index);
			}
			EVO_NODISCARD auto getVarDecl(const AST::Node& node) const -> const AST::VarDecl& {
				evo::debugAssert(node.kind() == AST::Kind::VarDecl, "Node is not a VarDecl");
				return this->var_decls[node._value.node_index];
			}

			EVO_NODISCARD auto createFuncDecl(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->func_decls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::FuncDecl, node_index);
			}
			EVO_NODISCARD auto getFuncDecl(const AST::Node& node) const -> const AST::FuncDecl& {
				evo::debugAssert(node.kind() == AST::Kind::FuncDecl, "Node is not a FuncDecl");
				return this->func_decls[node._value.node_index];
			}

			EVO_NODISCARD auto createAliasDecl(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->alias_decls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::AliasDecl, node_index);
			}
			EVO_NODISCARD auto getAliasDecl(const AST::Node& node) const -> const AST::AliasDecl& {
				evo::debugAssert(node.kind() == AST::Kind::AliasDecl, "Node is not a AliasDecl");
				return this->alias_decls[node._value.node_index];
			}


			EVO_NODISCARD auto createTypedefDecl(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->typedefs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::TypedefDecl, node_index);
			}
			EVO_NODISCARD auto getTypedefDecl(const AST::Node& node) const -> const AST::TypedefDecl& {
				evo::debugAssert(node.kind() == AST::Kind::TypedefDecl, "Node is not a TypedefDecl");
				return this->typedefs[node._value.node_index];
			}


			EVO_NODISCARD auto createReturn(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->returns.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Return, node_index);
			}
			EVO_NODISCARD auto getReturn(const AST::Node& node) const -> const AST::Return& {
				evo::debugAssert(node.kind() == AST::Kind::Return, "Node is not a Return");
				return this->returns[node._value.node_index];
			}

			EVO_NODISCARD auto createConditional(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->conditionals.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Conditional, node_index);
			}
			EVO_NODISCARD auto getConditional(const AST::Node& node) const -> const AST::Conditional& {
				evo::debugAssert(node.kind() == AST::Kind::Conditional, "Node is not a Conditional");
				return this->conditionals[node._value.node_index];
			}

			EVO_NODISCARD auto createWhenConditional(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->when_conditionals.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::WhenConditional, node_index);
			}
			EVO_NODISCARD auto getWhenConditional(const AST::Node& node) const -> const AST::WhenConditional& {
				evo::debugAssert(node.kind() == AST::Kind::WhenConditional, "Node is not a WhenConditional");
				return this->when_conditionals[node._value.node_index];
			}

			EVO_NODISCARD auto createWhile(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->whiles.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::While, node_index);
			}
			EVO_NODISCARD auto getWhile(const AST::Node& node) const -> const AST::While& {
				evo::debugAssert(node.kind() == AST::Kind::While, "Node is not a While");
				return this->whiles[node._value.node_index];
			}


			EVO_NODISCARD auto createBlock(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->blocks.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Block, node_index);
			}
			EVO_NODISCARD auto getBlock(const AST::Node& node) const -> const AST::Block& {
				evo::debugAssert(node.kind() == AST::Kind::Block, "Node is not a Block");
				return this->blocks[node._value.node_index];
			}

			EVO_NODISCARD auto createFuncCall(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->func_calls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::FuncCall, node_index);
			}
			EVO_NODISCARD auto getFuncCall(const AST::Node& node) const -> const AST::FuncCall& {
				evo::debugAssert(node.kind() == AST::Kind::FuncCall, "Node is not a FuncCall");
				return this->func_calls[node._value.node_index];
			}

			EVO_NODISCARD auto createTemplatePack(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->template_packs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::TemplatePack, node_index);
			}
			EVO_NODISCARD auto getTemplatePack(const AST::Node& node) const -> const AST::TemplatePack& {
				evo::debugAssert(node.kind() == AST::Kind::TemplatePack, "Node is not a TemplatePack");
				return this->template_packs[node._value.node_index];
			}

			EVO_NODISCARD auto createTemplatedExpr(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->templated_expr.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::TemplatedExpr, node_index);
			}
			EVO_NODISCARD auto getTemplatedExpr(const AST::Node& node) const -> const AST::TemplatedExpr& {
				evo::debugAssert(node.kind() == AST::Kind::TemplatedExpr, "Node is not a TemplatedExpr");
				return this->templated_expr[node._value.node_index];
			}


			EVO_NODISCARD auto createPrefix(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->prefixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Prefix, node_index);
			}
			EVO_NODISCARD auto getPrefix(const AST::Node& node) const -> const AST::Prefix& {
				evo::debugAssert(node.kind() == AST::Kind::Prefix, "Node is not a Prefix");
				return this->prefixes[node._value.node_index];
			}

			EVO_NODISCARD auto createInfix(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->infixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Infix, node_index);
			}
			EVO_NODISCARD auto getInfix(const AST::Node& node) const -> const AST::Infix& {
				evo::debugAssert(node.kind() == AST::Kind::Infix, "Node is not a Infix");
				return this->infixes[node._value.node_index];
			}

			EVO_NODISCARD auto createPostfix(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->postfixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Postfix, node_index);
			}
			EVO_NODISCARD auto getPostfix(const AST::Node& node) const -> const AST::Postfix& {
				evo::debugAssert(node.kind() == AST::Kind::Postfix, "Node is not a Postfix");
				return this->postfixes[node._value.node_index];
			}


			EVO_NODISCARD auto createMultiAssign(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->multi_assigns.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::MultiAssign, node_index);
			}
			EVO_NODISCARD auto getMultiAssign(const AST::Node& node) const -> const AST::MultiAssign& {
				evo::debugAssert(node.kind() == AST::Kind::MultiAssign, "Node is not a MultiAssign");
				return this->multi_assigns[node._value.node_index];
			}


			EVO_NODISCARD auto createNew(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->news.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::New, node_index);
			}
			EVO_NODISCARD auto getNew(const AST::Node& node) const -> const AST::New& {
				evo::debugAssert(node.kind() == AST::Kind::New, "Node is not a New");
				return this->news[node._value.node_index];
			}



			EVO_NODISCARD auto createType(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->types.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::Type, node_index);
			}
			EVO_NODISCARD auto getType(const AST::Node& node) const -> const AST::Type& {
				evo::debugAssert(node.kind() == AST::Kind::Type, "Node is not a Type");
				return this->types[node._value.node_index];
			}

			EVO_NODISCARD auto createTypeIDConverter(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->type_id_converters.emplace_back(
					std::forward<decltype(args)>(args)...
				);
				return AST::Node(AST::Kind::TypeIDConverter, node_index);
			}
			EVO_NODISCARD auto getTypeIDConverter(const AST::Node& node) const -> const AST::TypeIDConverter& {
				evo::debugAssert(node.kind() == AST::Kind::TypeIDConverter, "Node is not a TypeIDConverter");
				return this->type_id_converters[node._value.node_index];
			}


			EVO_NODISCARD auto createAttributeBlock(auto&&... args) -> AST::Node {
				const uint32_t node_index = this->attribute_blocks.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::AttributeBlock, node_index);
			}
			EVO_NODISCARD auto getAttributeBlock(const AST::Node& node) const -> const AST::AttributeBlock& {
				evo::debugAssert(node.kind() == AST::Kind::AttributeBlock, "Node is not a AttributeBlock");
				return this->attribute_blocks[node._value.node_index];
			}


	
		private:
			evo::SmallVector<AST::Node> global_stmts{};

			core::LinearStepAlloc<AST::VarDecl, uint32_t> var_decls{};
			core::LinearStepAlloc<AST::FuncDecl, uint32_t> func_decls{};
			core::LinearStepAlloc<AST::AliasDecl, uint32_t> alias_decls{};
			core::LinearStepAlloc<AST::TypedefDecl, uint32_t> typedefs{};

			core::LinearStepAlloc<AST::Return, uint32_t> returns{};
			core::LinearStepAlloc<AST::Conditional, uint32_t> conditionals{};
			core::LinearStepAlloc<AST::WhenConditional, uint32_t> when_conditionals{};
			core::LinearStepAlloc<AST::While, uint32_t> whiles{};

			core::LinearStepAlloc<AST::Block, uint32_t> blocks{};
			core::LinearStepAlloc<AST::FuncCall, uint32_t> func_calls{};
			core::LinearStepAlloc<AST::TemplatePack, uint32_t> template_packs{};
			core::LinearStepAlloc<AST::TemplatedExpr, uint32_t> templated_expr{};

			core::LinearStepAlloc<AST::Prefix, uint32_t> prefixes{};
			core::LinearStepAlloc<AST::Infix, uint32_t> infixes{};
			core::LinearStepAlloc<AST::Postfix, uint32_t> postfixes{};

			core::LinearStepAlloc<AST::MultiAssign, uint32_t> multi_assigns{};

			core::LinearStepAlloc<AST::New, uint32_t> news{};

			core::LinearStepAlloc<AST::Type, uint32_t> types{};
			core::LinearStepAlloc<AST::TypeIDConverter, uint32_t> type_id_converters{};

			core::LinearStepAlloc<AST::AttributeBlock, uint32_t> attribute_blocks{};


			friend class Parser;
	};


}
