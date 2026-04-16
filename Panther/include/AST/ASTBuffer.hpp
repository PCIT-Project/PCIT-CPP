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


	class ASTBuffer{
		public:
			ASTBuffer() = default;
			~ASTBuffer() = default;


			[[nodiscard]] auto getGlobalStmts() const -> evo::ArrayProxy<AST::Node> { return this->global_stmts; }
			[[nodiscard]] auto numGlobalStmts() const -> size_t { return this->global_stmts.size(); }


			[[nodiscard]] static auto getIdent(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::IDENT, "Node is not a Ident");
				return node._value.token_id;
			}

			[[nodiscard]] static auto getIntrinsic(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::INTRINSIC, "Node is not a Intrinsic");
				return node._value.token_id;
			}

			[[nodiscard]] static auto getTypeThis(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::TYPE_THIS, "Node is not a TypeThis");
				return node._value.token_id;
			}

			[[nodiscard]] static auto getLiteral(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::LITERAL, "Node is not a Literal");
				return node._value.token_id;
			}

			[[nodiscard]] static auto getAttribute(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::ATTRIBUTE, "Node is not a Attribute");
				return node._value.token_id;
			}

			[[nodiscard]] static auto getPrimitiveType(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::PRIMITIVE_TYPE, "Node is not a PrimitiveType");
				return node._value.token_id;
			}

			[[nodiscard]] static auto getDeducer(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::DEDUCER, "Node is not a Deducer");
				return node._value.token_id;
			}

			[[nodiscard]] static auto getUninit(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::UNINIT, "Node is not a Uninit");
				return node._value.token_id;
			}

			[[nodiscard]] static auto getZeroinit(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::ZEROINIT, "Node is not a Zeroinit");
				return node._value.token_id;
			}

			[[nodiscard]] static auto getThis(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::THIS, "Node is not a This");
				return node._value.token_id;
			}

			[[nodiscard]] static auto getDiscard(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::DISCARD, "Node is not a Discard");
				return node._value.token_id;
			}

			[[nodiscard]] static auto getUnreachable(const AST::Node& node) -> Token::ID {
				evo::debugAssert(node.kind() == AST::Kind::UNREACHABLE, "Node is not a Unreachable");
				return node._value.token_id;
			}



			[[nodiscard]] auto createVarDef(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->var_defs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::VAR_DEF, node_index);
			}
			[[nodiscard]] auto getVarDef(const AST::Node& node) const -> const AST::VarDef& {
				evo::debugAssert(node.kind() == AST::Kind::VAR_DEF, "Node is not a VarDef");
				return this->var_defs[node._value.node_index];
			}

			[[nodiscard]] auto createFuncDef(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->func_defs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::FUNC_DEF, node_index);
			}
			[[nodiscard]] auto getFuncDef(const AST::Node& node) const -> const AST::FuncDef& {
				evo::debugAssert(node.kind() == AST::Kind::FUNC_DEF, "Node is not a FuncDef");
				return this->func_defs[node._value.node_index];
			}

			[[nodiscard]] auto createDeletedSpecialMethod(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index =
					this->deleted_sepcial_methods.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::DELETED_SPECIAL_METHOD, node_index);
			}
			[[nodiscard]] auto getDeletedSpecialMethod(const AST::Node& node) const
			-> const AST::DeletedSpecialMethod& {
				evo::debugAssert(
					node.kind() == AST::Kind::DELETED_SPECIAL_METHOD, "Node is not a DeletedSpecialMethod"
				);
				return this->deleted_sepcial_methods[node._value.node_index];
			}

			[[nodiscard]] auto createFuncAliasDef(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->func_alias_defs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::FUNC_ALIAS_DEF, node_index);
			}
			[[nodiscard]] auto getFuncAliasDef(const AST::Node& node) const -> const AST::FuncAliasDef& {
				evo::debugAssert(node.kind() == AST::Kind::FUNC_ALIAS_DEF, "Node is not a FuncAliasDef");
				return this->func_alias_defs[node._value.node_index];
			}

			[[nodiscard]] auto createAliasDef(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->alias_defs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::ALIAS_DEF, node_index);
			}
			[[nodiscard]] auto getAliasDef(const AST::Node& node) const -> const AST::AliasDef& {
				evo::debugAssert(node.kind() == AST::Kind::ALIAS_DEF, "Node is not a AliasDef");
				return this->alias_defs[node._value.node_index];
			}


			[[nodiscard]] auto createStructDef(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->struct_defs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::STRUCT_DEF, node_index);
			}
			[[nodiscard]] auto getStructDef(const AST::Node& node) const -> const AST::StructDef& {
				evo::debugAssert(node.kind() == AST::Kind::STRUCT_DEF, "Node is not a StructDef");
				return this->struct_defs[node._value.node_index];
			}

			[[nodiscard]] auto createUnionDef(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->union_defs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::UNION_DEF, node_index);
			}
			[[nodiscard]] auto getUnionDef(const AST::Node& node) const -> const AST::UnionDef& {
				evo::debugAssert(node.kind() == AST::Kind::UNION_DEF, "Node is not an Union");
				return this->union_defs[node._value.node_index];
			}

			[[nodiscard]] auto createEnumDef(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->enum_defs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::ENUM_DEF, node_index);
			}
			[[nodiscard]] auto getEnumDef(const AST::Node& node) const -> const AST::EnumDef& {
				evo::debugAssert(node.kind() == AST::Kind::ENUM_DEF, "Node is not an Enum");
				return this->enum_defs[node._value.node_index];
			}

			[[nodiscard]] auto createInterfaceDef(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->interface_defs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::INTERFACE_DEF, node_index);
			}
			[[nodiscard]] auto getInterfaceDef(const AST::Node& node) const -> const AST::InterfaceDef& {
				evo::debugAssert(node.kind() == AST::Kind::INTERFACE_DEF, "Node is not an InterfaceDef");
				return this->interface_defs[node._value.node_index];
			}

			[[nodiscard]] auto createInterfaceImpl(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->interface_impls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::INTERFACE_IMPL, node_index);
			}
			[[nodiscard]] auto getInterfaceImpl(const AST::Node& node) const -> const AST::InterfaceImpl& {
				evo::debugAssert(node.kind() == AST::Kind::INTERFACE_IMPL, "Node is not an InterfaceImpl");
				return this->interface_impls[node._value.node_index];
			}


			[[nodiscard]] auto createReturn(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->returns.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::RETURN, node_index);
			}
			[[nodiscard]] auto getReturn(const AST::Node& node) const -> const AST::Return& {
				evo::debugAssert(node.kind() == AST::Kind::RETURN, "Node is not a Return");
				return this->returns[node._value.node_index];
			}

			[[nodiscard]] auto createError(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->errors.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::ERROR, node_index);
			}
			[[nodiscard]] auto getError(const AST::Node& node) const -> const AST::Error& {
				evo::debugAssert(node.kind() == AST::Kind::ERROR, "Node is not a Error");
				return this->errors[node._value.node_index];
			}

			[[nodiscard]] auto createBreak(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->breaks.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::BREAK, node_index);
			}
			[[nodiscard]] auto getBreak(const AST::Node& node) const -> const AST::Break& {
				evo::debugAssert(node.kind() == AST::Kind::BREAK, "Node is not a Break");
				return this->breaks[node._value.node_index];
			}

			[[nodiscard]] auto createContinue(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->continues.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::CONTINUE, node_index);
			}
			[[nodiscard]] auto getContinue(const AST::Node& node) const -> const AST::Continue& {
				evo::debugAssert(node.kind() == AST::Kind::CONTINUE, "Node is not a Continue");
				return this->continues[node._value.node_index];
			}

			[[nodiscard]] auto createDelete(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->deletes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::DELETE, node_index);
			}
			[[nodiscard]] auto getDelete(const AST::Node& node) const -> const AST::Delete& {
				evo::debugAssert(node.kind() == AST::Kind::DELETE, "Node is not a Delete");
				return this->deletes[node._value.node_index];
			}

			[[nodiscard]] auto createConditional(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->conditionals.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::CONDITIONAL, node_index);
			}
			[[nodiscard]] auto getConditional(const AST::Node& node) const -> const AST::Conditional& {
				evo::debugAssert(node.kind() == AST::Kind::CONDITIONAL, "Node is not a Conditional");
				return this->conditionals[node._value.node_index];
			}

			[[nodiscard]] auto createWhenConditional(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->when_conditionals.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::WHEN_CONDITIONAL, node_index);
			}
			[[nodiscard]] auto getWhenConditional(const AST::Node& node) const -> const AST::WhenConditional& {
				evo::debugAssert(node.kind() == AST::Kind::WHEN_CONDITIONAL, "Node is not a WhenConditional");
				return this->when_conditionals[node._value.node_index];
			}

			[[nodiscard]] auto createWhile(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->whiles.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::WHILE, node_index);
			}
			[[nodiscard]] auto getWhile(const AST::Node& node) const -> const AST::While& {
				evo::debugAssert(node.kind() == AST::Kind::WHILE, "Node is not a While");
				return this->whiles[node._value.node_index];
			}

			[[nodiscard]] auto createFor(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->fors.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::FOR, node_index);
			}
			[[nodiscard]] auto getFor(const AST::Node& node) const -> const AST::For& {
				evo::debugAssert(node.kind() == AST::Kind::FOR, "Node is not a For");
				return this->fors[node._value.node_index];
			}

			[[nodiscard]] auto createSwitch(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->switches.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::SWITCH, node_index);
			}
			[[nodiscard]] auto getSwitch(const AST::Node& node) const -> const AST::Switch& {
				evo::debugAssert(node.kind() == AST::Kind::SWITCH, "Node is not a Switch");
				return this->switches[node._value.node_index];
			}

			[[nodiscard]] auto createDefer(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->defers.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::DEFER, node_index);
			}
			[[nodiscard]] auto getDefer(const AST::Node& node) const -> const AST::Defer& {
				evo::debugAssert(node.kind() == AST::Kind::DEFER, "Node is not a Defer");
				return this->defers[node._value.node_index];
			}


			[[nodiscard]] auto createBlock(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->blocks.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::BLOCK, node_index);
			}
			[[nodiscard]] auto getBlock(const AST::Node& node) const -> const AST::Block& {
				evo::debugAssert(node.kind() == AST::Kind::BLOCK, "Node is not a Block");
				return this->blocks[node._value.node_index];
			}

			[[nodiscard]] auto createFuncCall(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->func_calls.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::FUNC_CALL, node_index);
			}
			[[nodiscard]] auto getFuncCall(const AST::Node& node) const -> const AST::FuncCall& {
				evo::debugAssert(node.kind() == AST::Kind::FUNC_CALL, "Node is not a FuncCall");
				return this->func_calls[node._value.node_index];
			}

			[[nodiscard]] auto createIndexer(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->indexers.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::INDEXER, node_index);
			}
			[[nodiscard]] auto getIndexer(const AST::Node& node) const -> const AST::Indexer& {
				evo::debugAssert(node.kind() == AST::Kind::INDEXER, "Node is not an Indexer");
				return this->indexers[node._value.node_index];
			}

			[[nodiscard]] auto createTemplatePack(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->template_packs.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::TEMPLATE_PACK, node_index);
			}
			[[nodiscard]] auto getTemplatePack(const AST::Node& node) const -> const AST::TemplatePack& {
				evo::debugAssert(node.kind() == AST::Kind::TEMPLATE_PACK, "Node is not a TemplatePack");
				return this->template_packs[node._value.node_index];
			}

			[[nodiscard]] auto createTemplatedExpr(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->templated_expr.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::TEMPLATED_EXPR, node_index);
			}
			[[nodiscard]] auto getTemplatedExpr(const AST::Node& node) const -> const AST::TemplatedExpr& {
				evo::debugAssert(node.kind() == AST::Kind::TEMPLATED_EXPR, "Node is not a TemplatedExpr");
				return this->templated_expr[node._value.node_index];
			}


			[[nodiscard]] auto createPrefix(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->prefixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::PREFIX, node_index);
			}
			[[nodiscard]] auto getPrefix(const AST::Node& node) const -> const AST::Prefix& {
				evo::debugAssert(node.kind() == AST::Kind::PREFIX, "Node is not a Prefix");
				return this->prefixes[node._value.node_index];
			}

			[[nodiscard]] auto createInfix(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->infixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::INFIX, node_index);
			}
			[[nodiscard]] auto getInfix(const AST::Node& node) const -> const AST::Infix& {
				evo::debugAssert(node.kind() == AST::Kind::INFIX, "Node is not a Infix");
				return this->infixes[node._value.node_index];
			}

			[[nodiscard]] auto createPostfix(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->postfixes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::POSTFIX, node_index);
			}
			[[nodiscard]] auto getPostfix(const AST::Node& node) const -> const AST::Postfix& {
				evo::debugAssert(node.kind() == AST::Kind::POSTFIX, "Node is not a Postfix");
				return this->postfixes[node._value.node_index];
			}


			[[nodiscard]] auto createMultiAssign(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->multi_assigns.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::MULTI_ASSIGN, node_index);
			}
			[[nodiscard]] auto getMultiAssign(const AST::Node& node) const -> const AST::MultiAssign& {
				evo::debugAssert(node.kind() == AST::Kind::MULTI_ASSIGN, "Node is not a MultiAssign");
				return this->multi_assigns[node._value.node_index];
			}


			[[nodiscard]] auto createNew(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->news.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::NEW, node_index);
			}
			[[nodiscard]] auto getNew(const AST::Node& node) const -> const AST::New& {
				evo::debugAssert(node.kind() == AST::Kind::NEW, "Node is not a New");
				return this->news[node._value.node_index];
			}

			[[nodiscard]] auto createArrayInitNew(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->array_init_news.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::ARRAY_INIT_NEW, node_index);
			}
			[[nodiscard]] auto getArrayInitNew(const AST::Node& node) const -> const AST::ArrayInitNew& {
				evo::debugAssert(
					node.kind() == AST::Kind::ARRAY_INIT_NEW, "Node is not a ArrayInitNew"
				);
				return this->array_init_news[node._value.node_index];
			}

			[[nodiscard]] auto createDesignatedInitNew(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index =
					this->designated_init_news.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::DESIGNATED_INIT_NEW, node_index);
			}
			[[nodiscard]] auto getDesignatedInitNew(const AST::Node& node) const -> const AST::DesignatedInitNew& {
				evo::debugAssert(
					node.kind() == AST::Kind::DESIGNATED_INIT_NEW, "Node is not a DesignatedInitNew"
				);
				return this->designated_init_news[node._value.node_index];
			}

			[[nodiscard]] auto createTryElse(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->try_elses.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::TRY_ELSE, node_index);
			}
			[[nodiscard]] auto getTryElse(const AST::Node& node) const -> const AST::TryElse& {
				evo::debugAssert(node.kind() == AST::Kind::TRY_ELSE, "Node is not a TryElse");
				return this->try_elses[node._value.node_index];
			}

			[[nodiscard]] auto createUnsafe(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->unsafes.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::UNSAFE, node_index);
			}
			[[nodiscard]] auto getUnsafe(const AST::Node& node) const -> const AST::Unsafe& {
				evo::debugAssert(node.kind() == AST::Kind::UNSAFE, "Node is not a Unsafe");
				return this->unsafes[node._value.node_index];
			}


			[[nodiscard]] auto createArrayType(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->array_types.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::ARRAY_TYPE, node_index);
			}
			[[nodiscard]] auto getArrayType(const AST::Node& node) const -> const AST::ArrayType& {
				evo::debugAssert(node.kind() == AST::Kind::ARRAY_TYPE, "Node is not an ArrayType");
				return this->array_types[node._value.node_index];
			}


			[[nodiscard]] auto createInterfaceMap(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->interface_maps.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::INTERFACE_MAP, node_index);
			}
			[[nodiscard]] auto getInterfaceMap(const AST::Node& node) const -> const AST::InterfaceMap& {
				evo::debugAssert(node.kind() == AST::Kind::INTERFACE_MAP, "Node is not an InterfaceMap");
				return this->interface_maps[node._value.node_index];
			}

			[[nodiscard]] auto createType(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->types.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::TYPE, node_index);
			}
			[[nodiscard]] auto getType(const AST::Node& node) const -> const AST::Type& {
				evo::debugAssert(node.kind() == AST::Kind::TYPE, "Node is not a Type");
				return this->types[node._value.node_index];
			}

			[[nodiscard]] auto createTypeIDConverter(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->type_id_converters.emplace_back(
					std::forward<decltype(args)>(args)...
				);
				return AST::Node(AST::Kind::TYPEID_CONVERTER, node_index);
			}
			[[nodiscard]] auto getTypeIDConverter(const AST::Node& node) const -> const AST::TypeIDConverter& {
				evo::debugAssert(node.kind() == AST::Kind::TYPEID_CONVERTER, "Node is not a TypeIDConverter");
				return this->type_id_converters[node._value.node_index];
			}


			[[nodiscard]] auto createAttributeBlock(auto&&... args) -> AST::Node {
				evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
				const uint32_t node_index = this->attribute_blocks.emplace_back(std::forward<decltype(args)>(args)...);
				return AST::Node(AST::Kind::ATTRIBUTE_BLOCK, node_index);
			}
			[[nodiscard]] auto getAttributeBlock(const AST::Node& node) const -> const AST::AttributeBlock& {
				evo::debugAssert(node.kind() == AST::Kind::ATTRIBUTE_BLOCK, "Node is not an AttributeBlock");
				return this->attribute_blocks[node._value.node_index];
			}



			auto lock() -> void {
				evo::debugAssert(this->is_locked == false, "Already locked");
				this->is_locked = true;
			}
			[[nodiscard]] auto isLocked() const -> bool { return this->is_locked; }

	
		private:
			evo::SmallVector<AST::Node> global_stmts{};

			core::LinearStepAlloc<AST::VarDef, uint32_t> var_defs{};
			core::LinearStepAlloc<AST::FuncDef, uint32_t> func_defs{};
			core::LinearStepAlloc<AST::DeletedSpecialMethod, uint32_t> deleted_sepcial_methods{};
			core::LinearStepAlloc<AST::FuncAliasDef, uint32_t> func_alias_defs{};
			core::LinearStepAlloc<AST::AliasDef, uint32_t> alias_defs{};
			core::LinearStepAlloc<AST::StructDef, uint32_t> struct_defs{};
			core::LinearStepAlloc<AST::UnionDef, uint32_t> union_defs{};
			core::LinearStepAlloc<AST::EnumDef, uint32_t> enum_defs{};
			core::LinearStepAlloc<AST::InterfaceDef, uint32_t> interface_defs{};
			core::LinearStepAlloc<AST::InterfaceImpl, uint32_t> interface_impls{};

			core::LinearStepAlloc<AST::Return, uint32_t> returns{};
			core::LinearStepAlloc<AST::Error, uint32_t> errors{};
			core::LinearStepAlloc<AST::Break, uint32_t> breaks{};
			core::LinearStepAlloc<AST::Continue, uint32_t> continues{};
			core::LinearStepAlloc<AST::Delete, uint32_t> deletes{};
			core::LinearStepAlloc<AST::Conditional, uint32_t> conditionals{};
			core::LinearStepAlloc<AST::WhenConditional, uint32_t> when_conditionals{};
			core::LinearStepAlloc<AST::While, uint32_t> whiles{};
			core::LinearStepAlloc<AST::For, uint32_t> fors{};
			core::LinearStepAlloc<AST::Switch, uint32_t> switches{};
			core::LinearStepAlloc<AST::Defer, uint32_t> defers{};

			core::LinearStepAlloc<AST::Block, uint32_t> blocks{};
			core::LinearStepAlloc<AST::FuncCall, uint32_t> func_calls{};
			core::LinearStepAlloc<AST::Indexer, uint32_t> indexers{};
			core::LinearStepAlloc<AST::TemplatePack, uint32_t> template_packs{};
			core::LinearStepAlloc<AST::TemplatedExpr, uint32_t> templated_expr{};

			core::LinearStepAlloc<AST::Prefix, uint32_t> prefixes{};
			core::LinearStepAlloc<AST::Infix, uint32_t> infixes{};
			core::LinearStepAlloc<AST::Postfix, uint32_t> postfixes{};

			core::LinearStepAlloc<AST::MultiAssign, uint32_t> multi_assigns{};

			core::LinearStepAlloc<AST::New, uint32_t> news{};
			core::LinearStepAlloc<AST::ArrayInitNew, uint32_t> array_init_news{};
			core::LinearStepAlloc<AST::DesignatedInitNew, uint32_t> designated_init_news{};

			core::LinearStepAlloc<AST::TryElse, uint32_t> try_elses{};
			core::LinearStepAlloc<AST::Unsafe, uint32_t> unsafes{};

			core::LinearStepAlloc<AST::ArrayType, uint32_t> array_types{};
			core::LinearStepAlloc<AST::InterfaceMap, uint32_t> interface_maps{};
			core::LinearStepAlloc<AST::Type, uint32_t> types{};
			core::LinearStepAlloc<AST::TypeIDConverter, uint32_t> type_id_converters{};

			core::LinearStepAlloc<AST::AttributeBlock, uint32_t> attribute_blocks{};


			bool is_locked = false;


			friend class Parser;
	};


}
