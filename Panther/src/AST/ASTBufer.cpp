////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/AST/ASTBuffer.hpp"



namespace pcit::panther::AST{


	struct ASTBuffer::Internal{
		evo::SmallVector<Node> global_stmts{};

		core::LinearStepAlloc<VarDef, uint32_t> var_defs{};
		core::LinearStepAlloc<FuncDef, uint32_t> func_defs{};
		core::LinearStepAlloc<DeletedSpecialMethod, uint32_t> deleted_sepcial_methods{};
		core::LinearStepAlloc<FuncAliasDef, uint32_t> func_alias_defs{};
		core::LinearStepAlloc<AliasDef, uint32_t> alias_defs{};
		core::LinearStepAlloc<StructDef, uint32_t> struct_defs{};
		core::LinearStepAlloc<UnionDef, uint32_t> union_defs{};
		core::LinearStepAlloc<EnumDef, uint32_t> enum_defs{};
		core::LinearStepAlloc<InterfaceDef, uint32_t> interface_defs{};
		core::LinearStepAlloc<InterfaceImpl, uint32_t> interface_impls{};

		core::LinearStepAlloc<Return, uint32_t> _returns{};
		core::LinearStepAlloc<Error, uint32_t> errors{};
		core::LinearStepAlloc<Unreachable, uint32_t> unreachables{};
		core::LinearStepAlloc<Break, uint32_t> breaks{};
		core::LinearStepAlloc<Continue, uint32_t> continues{};
		core::LinearStepAlloc<Delete, uint32_t> deletes{};
		core::LinearStepAlloc<Conditional, uint32_t> conditionals{};
		core::LinearStepAlloc<WhenConditional, uint32_t> when_conditionals{};
		core::LinearStepAlloc<While, uint32_t> whiles{};
		core::LinearStepAlloc<For, uint32_t> fors{};
		core::LinearStepAlloc<WhenSwitch, uint32_t> when_switches{};
		core::LinearStepAlloc<Switch, uint32_t> switches{};
		core::LinearStepAlloc<Defer, uint32_t> defers{};

		core::LinearStepAlloc<Block, uint32_t> blocks{};
		core::LinearStepAlloc<FuncCall, uint32_t> func_calls{};
		core::LinearStepAlloc<Indexer, uint32_t> indexers{};
		core::LinearStepAlloc<TemplatePack, uint32_t> template_packs{};
		core::LinearStepAlloc<TemplatedExpr, uint32_t> templated_expr{};

		core::LinearStepAlloc<Prefix, uint32_t> prefixes{};
		core::LinearStepAlloc<Infix, uint32_t> infixes{};
		core::LinearStepAlloc<Postfix, uint32_t> postfixes{};

		core::LinearStepAlloc<MultiAssign, uint32_t> multi_assigns{};

		core::LinearStepAlloc<New, uint32_t> news{};
		core::LinearStepAlloc<ArrayInitNew, uint32_t> array_init_news{};
		core::LinearStepAlloc<DesignatedInitNew, uint32_t> designated_init_news{};

		core::LinearStepAlloc<TryElse, uint32_t> try_elses{};
		core::LinearStepAlloc<Unsafe, uint32_t> unsafes{};
		core::LinearStepAlloc<Asm, uint32_t> asms{};

		core::LinearStepAlloc<ArrayType, uint32_t> array_types{};
		core::LinearStepAlloc<FuncType, uint32_t> func_types{};
		core::LinearStepAlloc<InterfaceMap, uint32_t> interface_maps{};
		core::LinearStepAlloc<Type, uint32_t> types{};
		core::LinearStepAlloc<TypeIDConverter, uint32_t> type_id_converters{};

		core::LinearStepAlloc<AttributeBlock, uint32_t> attribute_blocks{};
	};



	ASTBuffer::ASTBuffer() : internal(new Internal()){}
	ASTBuffer::~ASTBuffer(){
		delete this->internal;
	}

	


	auto ASTBuffer::getGlobalStmts() const -> evo::ArrayProxy<Node> { return this->internal->global_stmts; }
	auto ASTBuffer::numGlobalStmts() const -> size_t { return this->internal->global_stmts.size(); }

	auto ASTBuffer::getIdent(const Node& node) -> Token::ID {
		evo::debugAssert(node.kind() == Kind::IDENT, "Node is not a Ident");
		return node._value.token_id;
	}


	auto ASTBuffer::getIntrinsic(const Node& node) -> Token::ID {
		evo::debugAssert(node.kind() == Kind::INTRINSIC, "Node is not a Intrinsic");
		return node._value.token_id;
	}

	auto ASTBuffer::getTypeThis(const Node& node) -> Token::ID {
		evo::debugAssert(node.kind() == Kind::TYPE_THIS, "Node is not a TypeThis");
		return node._value.token_id;
	}

	auto ASTBuffer::getLiteral(const Node& node) -> Token::ID {
		evo::debugAssert(node.kind() == Kind::LITERAL, "Node is not a Literal");
		return node._value.token_id;
	}

	auto ASTBuffer::getAttribute(const Node& node) -> Token::ID {
		evo::debugAssert(node.kind() == Kind::ATTRIBUTE, "Node is not a Attribute");
		return node._value.token_id;
	}

	auto ASTBuffer::getPrimitiveType(const Node& node) -> Token::ID {
		evo::debugAssert(node.kind() == Kind::PRIMITIVE_TYPE, "Node is not a PrimitiveType");
		return node._value.token_id;
	}

	auto ASTBuffer::getDeducer(const Node& node) -> Token::ID {
		evo::debugAssert(node.kind() == Kind::DEDUCER, "Node is not a Deducer");
		return node._value.token_id;
	}

	auto ASTBuffer::getUninit(const Node& node) -> Token::ID {
		evo::debugAssert(node.kind() == Kind::UNINIT, "Node is not a Uninit");
		return node._value.token_id;
	}

	auto ASTBuffer::getZeroinit(const Node& node) -> Token::ID {
		evo::debugAssert(node.kind() == Kind::ZEROINIT, "Node is not a Zeroinit");
		return node._value.token_id;
	}

	auto ASTBuffer::getThis(const Node& node) -> Token::ID {
		evo::debugAssert(node.kind() == Kind::THIS, "Node is not a This");
		return node._value.token_id;
	}

	auto ASTBuffer::getDiscard(const Node& node) -> Token::ID {
		evo::debugAssert(node.kind() == Kind::DISCARD, "Node is not a Discard");
		return node._value.token_id;
	}




	auto ASTBuffer::createVarDef(
		VarDef::Kind kind,
		Token::ID ident,
		std::optional<Node> type,
		Node attributeBlock,
		VarDef::ValueKind valueKind,
		std::optional<Node> value
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->var_defs.emplace_back(kind, ident, type, attributeBlock, valueKind, value);
		return Node(Kind::VAR_DEF, node_index);
	}
	auto ASTBuffer::getVarDef(const Node& node) const -> const VarDef& {
		evo::debugAssert(node.kind() == Kind::VAR_DEF, "Node is not a VarDef");
		return this->internal->var_defs[node._value.node_index];
	}



	auto ASTBuffer::createFuncDef(
		Token::ID name,
		std::optional<Node> templatePack,
		evo::SmallVector<FuncDef::Param>&& params,
		bool isVariadic,
		FuncDef::Kind kind,
		Node attributeBlock,
		evo::SmallVector<FuncDef::Return>&& returns,
		evo::SmallVector<FuncDef::Return>&& errorReturns,
		std::optional<Node> value
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->func_defs.emplace_back(
			name,
			templatePack,
			std::move(params),
			isVariadic,
			kind,
			attributeBlock,
			std::move(returns),
			std::move(errorReturns),
			value
		);
		return Node(Kind::FUNC_DEF, node_index);
	}
	auto ASTBuffer::getFuncDef(const Node& node) const -> const FuncDef& {
		evo::debugAssert(node.kind() == Kind::FUNC_DEF, "Node is not a FuncDef");
		return this->internal->func_defs[node._value.node_index];
	}


	auto ASTBuffer::createDeletedSpecialMethod(Token::ID memberToken, std::optional<Node> message) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->deleted_sepcial_methods.emplace_back(memberToken, message);
		return Node(Kind::DELETED_SPECIAL_METHOD, node_index);
	}
	auto ASTBuffer::getDeletedSpecialMethod(const Node& node) const -> const DeletedSpecialMethod& {
		evo::debugAssert(
			node.kind() == Kind::DELETED_SPECIAL_METHOD, "Node is not a DeletedSpecialMethod"
		);
		return this->internal->deleted_sepcial_methods[node._value.node_index];
	}


	auto ASTBuffer::createFuncAliasDef(Token::ID ident, Node attributeBlock, Node type) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->func_alias_defs.emplace_back(ident, attributeBlock, type);
		return Node(Kind::FUNC_ALIAS_DEF, node_index);
	}
	auto ASTBuffer::getFuncAliasDef(const Node& node) const -> const FuncAliasDef& {
		evo::debugAssert(node.kind() == Kind::FUNC_ALIAS_DEF, "Node is not a FuncAliasDef");
		return this->internal->func_alias_defs[node._value.node_index];
	}


	auto ASTBuffer::createAliasDef(Token::ID ident, Node attributeBlock, Node type) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->alias_defs.emplace_back(ident, attributeBlock, type);
		return Node(Kind::ALIAS_DEF, node_index);
	}
	auto ASTBuffer::getAliasDef(const Node& node) const -> const AliasDef& {
		evo::debugAssert(node.kind() == Kind::ALIAS_DEF, "Node is not a AliasDef");
		return this->internal->alias_defs[node._value.node_index];
	}


	auto ASTBuffer::createStructDef(Token::ID ident, std::optional<Node> templatePack, Node attributeBlock, Node block)
	-> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->struct_defs.emplace_back(ident, templatePack, attributeBlock, block);
		return Node(Kind::STRUCT_DEF, node_index);
	}
	auto ASTBuffer::getStructDef(const Node& node) const -> const StructDef& {
		evo::debugAssert(node.kind() == Kind::STRUCT_DEF, "Node is not a StructDef");
		return this->internal->struct_defs[node._value.node_index];
	}


	auto ASTBuffer::createUnionDef(
		Token::ID ident,
		Node attributeBlock,
		evo::SmallVector<UnionDef::Field>&& fields,
		evo::SmallVector<Node>&& statements
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->union_defs.emplace_back(
			ident, attributeBlock, std::move(fields), std::move(statements)
		);
		return Node(Kind::UNION_DEF, node_index);
	}
	auto ASTBuffer::getUnionDef(const Node& node) const -> const UnionDef& {
		evo::debugAssert(node.kind() == Kind::UNION_DEF, "Node is not an Union");
		return this->internal->union_defs[node._value.node_index];
	}


	auto ASTBuffer::createEnumDef(
		Token::ID ident,
		std::optional<Node> underlyingType,
		Node attributeBlock,
		evo::SmallVector<EnumDef::Enumerator>&& enumerators,
		evo::SmallVector<Node>&& statements
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->enum_defs.emplace_back(
			ident, underlyingType, attributeBlock, std::move(enumerators), std::move(statements)
		);
		return Node(Kind::ENUM_DEF, node_index);
	}
	auto ASTBuffer::getEnumDef(const Node& node) const -> const EnumDef& {
		evo::debugAssert(node.kind() == Kind::ENUM_DEF, "Node is not an Enum");
		return this->internal->enum_defs[node._value.node_index];
	}


	auto ASTBuffer::createInterfaceDef(
		Token::ID ident,
		Node attributeBlock,
		evo::SmallVector<Node>&& methods, // Nodes are all FuncDef
		evo::SmallVector<Node>&& impls // Nodes are all InterfaceImpl
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->interface_defs.emplace_back(
			ident, attributeBlock, std::move(methods), std::move(impls)
		);
		return Node(Kind::INTERFACE_DEF, node_index);
	}
	auto ASTBuffer::getInterfaceDef(const Node& node) const -> const InterfaceDef& {
		evo::debugAssert(node.kind() == Kind::INTERFACE_DEF, "Node is not an InterfaceDef");
		return this->internal->interface_defs[node._value.node_index];
	}


	auto ASTBuffer::createInterfaceImpl(
		Node target, Node attributeBlock, evo::SmallVector<InterfaceImpl::Method>&& methods
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index =
			this->internal->interface_impls.emplace_back(target, attributeBlock, std::move(methods));
		return Node(Kind::INTERFACE_IMPL, node_index);
	}
	auto ASTBuffer::getInterfaceImpl(const Node& node) const -> const InterfaceImpl& {
		evo::debugAssert(node.kind() == Kind::INTERFACE_IMPL, "Node is not an InterfaceImpl");
		return this->internal->interface_impls[node._value.node_index];
	}


	auto ASTBuffer::createReturn(
		Token::ID keyword,
		std::optional<Node> label,
		evo::Variant<std::monostate, Node, Token::ID> value // std::monostate == return; Token::ID == return...;
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->_returns.emplace_back(keyword, label, value);
		return Node(Kind::RETURN, node_index);
	}
	auto ASTBuffer::getReturn(const Node& node) const -> const Return& {
		evo::debugAssert(node.kind() == Kind::RETURN, "Node is not a Return");
		return this->internal->_returns[node._value.node_index];
	}


	auto ASTBuffer::createError(
		Token::ID keyword,
		evo::Variant<std::monostate, Node, Token::ID> value // std::monostate == error; Token::ID == error...;
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->errors.emplace_back(keyword, value);
		return Node(Kind::ERROR, node_index);
	}
	auto ASTBuffer::getError(const Node& node) const -> const Error& {
		evo::debugAssert(node.kind() == Kind::ERROR, "Node is not a Error");
		return this->internal->errors[node._value.node_index];
	}


	auto ASTBuffer::createUnreachable(Token::ID keyword, std::optional<Node> message) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->unreachables.emplace_back(keyword, message);
		return Node(Kind::UNREACHABLE, node_index);
	}
	auto ASTBuffer::getUnreachable(const Node& node) const -> const Unreachable& {
		evo::debugAssert(node.kind() == Kind::UNREACHABLE, "Node is not a Unreachable");
		return this->internal->unreachables[node._value.node_index];
	}


	auto ASTBuffer::createBreak(Token::ID keyword, std::optional<Token::ID> label) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->breaks.emplace_back(keyword, label);
		return Node(Kind::BREAK, node_index);
	}
	auto ASTBuffer::getBreak(const Node& node) const -> const Break& {
		evo::debugAssert(node.kind() == Kind::BREAK, "Node is not a Break");
		return this->internal->breaks[node._value.node_index];
	}


	auto ASTBuffer::createContinue(Token::ID keyword, std::optional<Token::ID> label) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->continues.emplace_back(keyword, label);
		return Node(Kind::CONTINUE, node_index);
	}
	auto ASTBuffer::getContinue(const Node& node) const -> const Continue& {
		evo::debugAssert(node.kind() == Kind::CONTINUE, "Node is not a Continue");
		return this->internal->continues[node._value.node_index];
	}


	auto ASTBuffer::createDelete(Token::ID keyword, Node value) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->deletes.emplace_back(keyword, value);
		return Node(Kind::DELETE, node_index);
	}
	auto ASTBuffer::getDelete(const Node& node) const -> const Delete& {
		evo::debugAssert(node.kind() == Kind::DELETE, "Node is not a Delete");
		return this->internal->deletes[node._value.node_index];
	}


	auto ASTBuffer::createConditional(
		Token::ID ifToken,
		std::optional<Token::ID> elseToken,
		Token::ID closeBraceToken,
		Node cond,
		Node thenBlock,
		std::optional<Node> elseBlock // either `Block` or `Conditional`
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->conditionals.emplace_back(
			ifToken, elseToken, closeBraceToken, cond, thenBlock, elseBlock
		);
		return Node(Kind::CONDITIONAL, node_index);
	}
	auto ASTBuffer::getConditional(const Node& node) const -> const Conditional& {
		evo::debugAssert(node.kind() == Kind::CONDITIONAL, "Node is not a Conditional");
		return this->internal->conditionals[node._value.node_index];
	}


	auto ASTBuffer::createWhenConditional(
		Token::ID keyword,
		Node cond,
		Node thenBlock,
		std::optional<Node> elseBlock // either `Block` or `WhenConditional`
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->when_conditionals.emplace_back(keyword, cond, thenBlock, elseBlock);
		return Node(Kind::WHEN_CONDITIONAL, node_index);
	}
	auto ASTBuffer::getWhenConditional(const Node& node) const -> const WhenConditional& {
		evo::debugAssert(node.kind() == Kind::WHEN_CONDITIONAL, "Node is not a WhenConditional");
		return this->internal->when_conditionals[node._value.node_index];
	}


	auto ASTBuffer::createWhile(Token::ID keyword, Node cond, Node block) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->whiles.emplace_back(keyword, cond, block);
		return Node(Kind::WHILE, node_index);
	}
	auto ASTBuffer::getWhile(const Node& node) const -> const While& {
		evo::debugAssert(node.kind() == Kind::WHILE, "Node is not a While");
		return this->internal->whiles[node._value.node_index];
	}


	auto ASTBuffer::createFor(
		Token::ID keyword,
		evo::SmallVector<Node>&& iterables,
		std::optional<For::Param> index, // nullopt means `_`
		evo::SmallVector<For::Param>&& values,
		Node attributeBlock,
		Node block
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->fors.emplace_back(
			keyword, std::move(iterables), index, std::move(values), attributeBlock, block
		);
		return Node(Kind::FOR, node_index);
	}
	auto ASTBuffer::getFor(const Node& node) const -> const For& {
		evo::debugAssert(node.kind() == Kind::FOR, "Node is not a For");
		return this->internal->fors[node._value.node_index];
	}


	auto ASTBuffer::createSwitch(
		Token::ID keyword,
		Token::ID closeBrace,
		Node cond,
		Node attributeBlock,
		evo::SmallVector<Switch::Case>&& cases
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->switches.emplace_back(
			keyword, closeBrace, cond, attributeBlock, std::move(cases)
		);
		return Node(Kind::SWITCH, node_index);
	}
	auto ASTBuffer::getSwitch(const Node& node) const -> const Switch& {
		evo::debugAssert(node.kind() == Kind::SWITCH, "Node is not a Switch");
		return this->internal->switches[node._value.node_index];
	}


	auto ASTBuffer::createWhenSwitch(
		Token::ID keyword,
		Token::ID closeBrace,
		Node cond,
		Node attributeBlock,
		evo::SmallVector<WhenSwitch::Case>&& cases
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->when_switches.emplace_back(
			keyword, closeBrace, cond, attributeBlock, std::move(cases)
		);
		return Node(Kind::WHEN_SWITCH, node_index);
	}
	auto ASTBuffer::getWhenSwitch(const Node& node) const -> const WhenSwitch& {
		evo::debugAssert(node.kind() == Kind::WHEN_SWITCH, "Node is not a WhenSwitch");
		return this->internal->when_switches[node._value.node_index];
	}


	auto ASTBuffer::createDefer(Token::ID keyword, Node block) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->defers.emplace_back(keyword, block);
		return Node(Kind::DEFER, node_index);
	}
	auto ASTBuffer::getDefer(const Node& node) const -> const Defer& {
		evo::debugAssert(node.kind() == Kind::DEFER, "Node is not a Defer");
		return this->internal->defers[node._value.node_index];
	}


	auto ASTBuffer::createBlock(
		Token::ID openBrace,
		Token::ID closeBrace,
		std::optional<Token::ID> label,
		evo::SmallVector<Block::Output>&& outputs, // only used if `.label` has value
		evo::SmallVector<Node>&& statements
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->blocks.emplace_back(
			openBrace, closeBrace, label, std::move(outputs), statements
		);
		return Node(Kind::BLOCK, node_index);
	}
	auto ASTBuffer::getBlock(const Node& node) const -> const Block& {
		evo::debugAssert(node.kind() == Kind::BLOCK, "Node is not a Block");
		return this->internal->blocks[node._value.node_index];
	}


	auto ASTBuffer::createFuncCall(Node target, evo::SmallVector<FuncCall::Arg>&& args) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->func_calls.emplace_back(target, std::move(args));
		return Node(Kind::FUNC_CALL, node_index);
	}
	auto ASTBuffer::getFuncCall(const Node& node) const -> const FuncCall& {
		evo::debugAssert(node.kind() == Kind::FUNC_CALL, "Node is not a FuncCall");
		return this->internal->func_calls[node._value.node_index];
	}


	auto ASTBuffer::createIndexer(Node target, evo::SmallVector<Node>&& indices, Token::ID openBracket)
	-> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->indexers.emplace_back(target, std::move(indices), openBracket);
		return Node(Kind::INDEXER, node_index);
	}
	auto ASTBuffer::getIndexer(const Node& node) const -> const Indexer& {
		evo::debugAssert(node.kind() == Kind::INDEXER, "Node is not an Indexer");
		return this->internal->indexers[node._value.node_index];
	}


	auto ASTBuffer::createTemplatePack(evo::SmallVector<TemplatePack::Param>&& params) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->template_packs.emplace_back(std::move(params));
		return Node(Kind::TEMPLATE_PACK, node_index);
	}
	auto ASTBuffer::getTemplatePack(const Node& node) const -> const TemplatePack& {
		evo::debugAssert(node.kind() == Kind::TEMPLATE_PACK, "Node is not a TemplatePack");
		return this->internal->template_packs[node._value.node_index];
	}


	auto ASTBuffer::createTemplatedExpr(Node base, evo::SmallVector<Node>&& args) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->templated_expr.emplace_back(base, std::move(args));
		return Node(Kind::TEMPLATED_EXPR, node_index);
	}
	auto ASTBuffer::getTemplatedExpr(const Node& node) const -> const TemplatedExpr& {
		evo::debugAssert(node.kind() == Kind::TEMPLATED_EXPR, "Node is not a TemplatedExpr");
		return this->internal->templated_expr[node._value.node_index];
	}


	auto ASTBuffer::createPrefix(Token::ID opTokenID, Node rhs) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->prefixes.emplace_back(opTokenID, rhs);
		return Node(Kind::PREFIX, node_index);
	}
	auto ASTBuffer::getPrefix(const Node& node) const -> const Prefix& {
		evo::debugAssert(node.kind() == Kind::PREFIX, "Node is not a Prefix");
		return this->internal->prefixes[node._value.node_index];
	}


	auto ASTBuffer::createInfix(Node lhs, Token::ID opTokenID, Node rhs) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->infixes.emplace_back(lhs, opTokenID, rhs);
		return Node(Kind::INFIX, node_index);
	}
	auto ASTBuffer::getInfix(const Node& node) const -> const Infix& {
		evo::debugAssert(node.kind() == Kind::INFIX, "Node is not a Infix");
		return this->internal->infixes[node._value.node_index];
	}


	auto ASTBuffer::createPostfix(Node lhs, Token::ID opTokenID) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->postfixes.emplace_back(lhs, opTokenID);
		return Node(Kind::POSTFIX, node_index);
	}
	auto ASTBuffer::getPostfix(const Node& node) const -> const Postfix& {
		evo::debugAssert(node.kind() == Kind::POSTFIX, "Node is not a Postfix");
		return this->internal->postfixes[node._value.node_index];
	}


	auto ASTBuffer::createMultiAssign(
		Token::ID openBracketLocation, evo::SmallVector<Node>&& assigns, Node value
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->multi_assigns.emplace_back(
			openBracketLocation, std::move(assigns), value
		);
		return Node(Kind::MULTI_ASSIGN, node_index);
	}
	auto ASTBuffer::getMultiAssign(const Node& node) const -> const MultiAssign& {
		evo::debugAssert(node.kind() == Kind::MULTI_ASSIGN, "Node is not a MultiAssign");
		return this->internal->multi_assigns[node._value.node_index];
	}


	auto ASTBuffer::createNew(Token::ID keyword, Node type, evo::SmallVector<FuncCall::Arg>&& args) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->news.emplace_back(keyword, type, std::move(args));
		return Node(Kind::NEW, node_index);
	}
	auto ASTBuffer::getNew(const Node& node) const -> const New& {
		evo::debugAssert(node.kind() == Kind::NEW, "Node is not a New");
		return this->internal->news[node._value.node_index];
	}


	auto ASTBuffer::createArrayInitNew(Token::ID keyword, Node type, evo::SmallVector<Node>&& values)
	-> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->array_init_news.emplace_back(keyword, type, std::move(values));
		return Node(Kind::ARRAY_INIT_NEW, node_index);
	}
	auto ASTBuffer::getArrayInitNew(const Node& node) const -> const ArrayInitNew& {
		evo::debugAssert(
			node.kind() == Kind::ARRAY_INIT_NEW, "Node is not a ArrayInitNew"
		);
		return this->internal->array_init_news[node._value.node_index];
	}


	auto ASTBuffer::createDesignatedInitNew(
		Token::ID keyword, Node type, evo::SmallVector<DesignatedInitNew::MemberInit>&& memberInits
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index =
			this->internal->designated_init_news.emplace_back(keyword, type, std::move(memberInits));
		return Node(Kind::DESIGNATED_INIT_NEW, node_index);
	}
	auto ASTBuffer::getDesignatedInitNew(const Node& node) const -> const DesignatedInitNew& {
		evo::debugAssert(
			node.kind() == Kind::DESIGNATED_INIT_NEW, "Node is not a DesignatedInitNew"
		);
		return this->internal->designated_init_news[node._value.node_index];
	}


	auto ASTBuffer::createTryElse(
		Node attemptExpr,
		Node exceptExpr,
		evo::SmallVector<Token::ID>&& exceptParams,
		Token::ID elseTokenID,
		std::optional<Token::ID> semicolonTokenID
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->try_elses.emplace_back(
			attemptExpr, exceptExpr, std::move(exceptParams), elseTokenID, semicolonTokenID
		);
		return Node(Kind::TRY_ELSE, node_index);
	}
	auto ASTBuffer::getTryElse(const Node& node) const -> const TryElse& {
		evo::debugAssert(node.kind() == Kind::TRY_ELSE, "Node is not a TryElse");
		return this->internal->try_elses[node._value.node_index];
	}


	auto ASTBuffer::createUnsafe(Token::ID keyword, Node block) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->unsafes.emplace_back(keyword, block);
		return Node(Kind::UNSAFE, node_index);
	}
	auto ASTBuffer::getUnsafe(const Node& node) const -> const Unsafe& {
		evo::debugAssert(node.kind() == Kind::UNSAFE, "Node is not a Unsafe");
		return this->internal->unsafes[node._value.node_index];
	}


	auto ASTBuffer::createAsm(
		Token::ID startToken,
		Token::ID asmStr,
		evo::SmallVector<Asm::Param>&& params,
		Node attributeBlock,
		evo::SmallVector<Asm::RetParam>&& retParams // empty if `Void`
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->asms.emplace_back(
			startToken, asmStr, std::move(params), attributeBlock, std::move(retParams)
		);
		return Node(Kind::ASM, node_index);
	}
	auto ASTBuffer::getAsm(const Node& node) const -> const Asm& {
		evo::debugAssert(node.kind() == Kind::ASM, "Node is not a Asm");
		return this->internal->asms[node._value.node_index];
	}


	auto ASTBuffer::createArrayType(
		Token::ID openBracket,
		Node elemType,
		evo::SmallVector<std::optional<Node>>&& dimensions, // element is nullopt if dimension is ptr
		std::optional<Node> terminator,
		std::optional<bool> refIsMut // only has value if is array ref
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->array_types.emplace_back(
			openBracket, elemType, std::move(dimensions), terminator, refIsMut
		);
		return Node(Kind::ARRAY_TYPE, node_index);
	}
	auto ASTBuffer::getArrayType(const Node& node) const -> const ArrayType& {
		evo::debugAssert(node.kind() == Kind::ARRAY_TYPE, "Node is not an ArrayType");
		return this->internal->array_types[node._value.node_index];
	}


	auto ASTBuffer::createFuncType(
		Token::ID funcKeyword,
		evo::SmallVector<FuncType::Param>&& params,
		Node attributeBlock,
		evo::SmallVector<Node>&& returnTypes,
		evo::SmallVector<Node>&& errorTypes,
		bool hasNamedReturns
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->func_types.emplace_back(
			funcKeyword,
			std::move(params),
			attributeBlock,
			std::move(returnTypes),
			std::move(errorTypes),
			hasNamedReturns
		);
		return Node(Kind::FUNC_TYPE, node_index);
	}
	auto ASTBuffer::getFuncType(const Node& node) const -> const FuncType& {
		evo::debugAssert(node.kind() == Kind::FUNC_TYPE, "Node is not an FuncType");
		return this->internal->func_types[node._value.node_index];
	}


	auto ASTBuffer::createInterfaceMap(
		evo::Variant<InterfaceMap::Polymorphic, InterfaceMap::Ptr, Node> underlyingType,
		Token::ID colonToken,
		Node interface
	) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->interface_maps.emplace_back(underlyingType, colonToken, interface);
		return Node(Kind::INTERFACE_MAP, node_index);
	}
	auto ASTBuffer::getInterfaceMap(const Node& node) const -> const InterfaceMap& {
		evo::debugAssert(node.kind() == Kind::INTERFACE_MAP, "Node is not an InterfaceMap");
		return this->internal->interface_maps[node._value.node_index];
	}


	auto ASTBuffer::createType(Node base, evo::SmallVector<Type::Qualifier>&& qualifiers) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->types.emplace_back(base, std::move(qualifiers));
		return Node(Kind::TYPE, node_index);
	}
	auto ASTBuffer::getType(const Node& node) const -> const Type& {
		evo::debugAssert(node.kind() == Kind::TYPE, "Node is not a Type");
		return this->internal->types[node._value.node_index];
	}


	auto ASTBuffer::createTypeIDConverter(Token::ID keyword, Node expr) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->type_id_converters.emplace_back(keyword, expr);
		return Node(Kind::TYPEID_CONVERTER, node_index);
	}
	auto ASTBuffer::getTypeIDConverter(const Node& node) const -> const TypeIDConverter& {
		evo::debugAssert(node.kind() == Kind::TYPEID_CONVERTER, "Node is not a TypeIDConverter");
		return this->internal->type_id_converters[node._value.node_index];
	}


	auto ASTBuffer::createAttributeBlock(evo::SmallVector<AttributeBlock::Attribute>&& attributes) -> Node {
		evo::debugAssert(this->is_locked == false, "Cannot create as buffer is locked");
		const uint32_t node_index = this->internal->attribute_blocks.emplace_back(std::move(attributes));
		return Node(Kind::ATTRIBUTE_BLOCK, node_index);
	}
	auto ASTBuffer::getAttributeBlock(const Node& node) const -> const AttributeBlock& {
		evo::debugAssert(node.kind() == Kind::ATTRIBUTE_BLOCK, "Node is not an AttributeBlock");
		return this->internal->attribute_blocks[node._value.node_index];
	}



	auto ASTBuffer::lock() -> void {
		evo::debugAssert(this->is_locked == false, "Already locked");
		this->is_locked = true;
	}

	auto ASTBuffer::isLocked() const -> bool { return this->is_locked; }


	auto ASTBuffer::getMutGlobalStmts() -> evo::SmallVector<Node>& { return this->internal->global_stmts; }

}