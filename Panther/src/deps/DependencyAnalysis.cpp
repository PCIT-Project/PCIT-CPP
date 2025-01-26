////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./DependencyAnalysis.h"

#include <queue>

namespace pcit::panther{
	

	auto DependencyAnalysis::analyze() -> bool {
		this->source.deps_scope_id = this->context.deps_buffer.scope_manager.createScope();
		this->scopes.push(*this->source.deps_scope_id);
		this->enter_scope();

		for(const AST::Node& ast_node : this->source.getASTBuffer().getGlobalStmts()){
			if(this->analyze_symbol_decl_ident<true>(ast_node, std::nullopt, nullptr) == false){ return false; }
		}

		for(const deps::Node::ID& deps_node_id : this->source.deps_node_ids){
			if(this->analyze_symbol_deps(deps_node_id) == false){ return false; }
		}

		// get `requiredBy`s
		for(const deps::Node::ID& deps_node_id : this->source.deps_node_ids){
			const deps::Node& deps_node = this->context.deps_buffer[deps_node_id];

			for(const deps::Node::ID& dep_id : deps_node.declDeps.decls){
				this->context.deps_buffer[dep_id].requiredBy.emplace(deps_node_id);
			}

			for(const deps::Node::ID& dep_id : deps_node.declDeps.defs){
				this->context.deps_buffer[dep_id].requiredBy.emplace(deps_node_id);
			}

			for(const deps::Node::ID& dep_id : deps_node.defDeps.decls){
				this->context.deps_buffer[dep_id].requiredBy.emplace(deps_node_id);
			}

			for(const deps::Node::ID& dep_id : deps_node.defDeps.defs){
				this->context.deps_buffer[dep_id].requiredBy.emplace(deps_node_id);
			}
		}

		return true;
	}


	template<bool IS_GLOBAL>
	auto DependencyAnalysis::analyze_symbol_decl_ident(
		const AST::Node& node,
		std::optional<deps::Node::ID> parent,
		evo::SmallVector<deps::Node::ID>* when_cond_parent_deps_list
	) -> bool {
		switch(node.kind()){
			case AST::Kind::None: {
				evo::debugFatalBreak("Invalid AST node");
			} break;

			case AST::Kind::VarDecl: {
				const AST::VarDecl& var_decl = this->source.getASTBuffer().getVarDecl(node);
				const std::string_view ident = this->source.getTokenBuffer()[var_decl.ident].getString();

				const deps::Node::ValueStage value_stage = [&](){
					switch(var_decl.kind){
						case AST::VarDecl::Kind::Var:   return deps::Node::ValueStage::Runtime;
						case AST::VarDecl::Kind::Const: return deps::Node::ValueStage::Constexpr;
						case AST::VarDecl::Kind::Def:   return deps::Node::ValueStage::Comptime;
					}
					evo::unreachable();
				}();

				const deps::Node::ID deps_node_id = this->create_node(node, value_stage);
				if(parent.has_value()){
					this->add_decl_deps_decl(this->context.deps_buffer[deps_node_id], *parent);
					this->add_def_deps_def(this->context.deps_buffer[*parent], deps_node_id);
				}

				this->add_symbol(ident, deps_node_id);
				if(when_cond_parent_deps_list != nullptr){ when_cond_parent_deps_list->emplace_back(deps_node_id); }
				if constexpr(IS_GLOBAL){
					return this->add_global_symbol(ident, var_decl.ident, false, when_cond_parent_deps_list != nullptr);
				}else{
					return true;
				}
			} break;

			case AST::Kind::FuncDecl: {
				const AST::FuncDecl& func_decl = this->source.getASTBuffer().getFuncDecl(node);

				if(func_decl.name.kind() != AST::Kind::Ident){
					// TODO: better messaging
					this->emit_error(Diagnostic::Code::DepInalidGlobalStmtKind, node, "Invalid global function");
					return false;
				}

				const Token::ID ident_token_id = this->source.getASTBuffer().getIdent(func_decl.name);
				const std::string_view ident = this->source.getTokenBuffer()[ident_token_id].getString();

				const deps::Node::ID deps_node_id = this->create_node(node, deps::Node::ValueStage::Unknown);
				if(parent.has_value()){
					this->add_decl_deps_decl(this->context.deps_buffer[deps_node_id], *parent);
					this->add_def_deps_def(this->context.deps_buffer[*parent], deps_node_id);
				}
				this->add_symbol(ident, deps_node_id);
				if(when_cond_parent_deps_list != nullptr){ when_cond_parent_deps_list->emplace_back(deps_node_id); }

				if constexpr(IS_GLOBAL){
					return this->add_global_symbol(ident, ident_token_id, true, when_cond_parent_deps_list != nullptr);
				}else{
					return true;
				}
			} break;

			case AST::Kind::AliasDecl: {
				const AST::AliasDecl& alias_decl = this->source.getASTBuffer().getAliasDecl(node);
				const std::string_view ident = this->source.getTokenBuffer()[alias_decl.ident].getString();

				const deps::Node::ID deps_node_id = this->create_node(node, deps::Node::ValueStage::Comptime);
				if(parent.has_value()){
					this->add_decl_deps_decl(this->context.deps_buffer[deps_node_id], *parent);
					this->add_def_deps_def(this->context.deps_buffer[*parent], deps_node_id);
				}
				this->add_symbol(ident, deps_node_id);
				if(when_cond_parent_deps_list != nullptr){ when_cond_parent_deps_list->emplace_back(deps_node_id); }

				return true;
			} break;

			case AST::Kind::TypedefDecl: {
				const AST::TypedefDecl& typedef_decl = this->source.getASTBuffer().getTypedefDecl(node);
				const std::string_view ident = this->source.getTokenBuffer()[typedef_decl.ident].getString();

				const deps::Node::ID deps_node_id = this->create_node(node, deps::Node::ValueStage::Comptime);
				if(parent.has_value()){
					this->add_decl_deps_decl(this->context.deps_buffer[deps_node_id], *parent);
					this->add_def_deps_def(this->context.deps_buffer[*parent], deps_node_id);
				}
				this->add_symbol(ident, deps_node_id);
				if(when_cond_parent_deps_list != nullptr){ when_cond_parent_deps_list->emplace_back(deps_node_id); }

				return true;
			} break;

			case AST::Kind::StructDecl: {
				const AST::StructDecl& struct_decl = this->source.getASTBuffer().getStructDecl(node);
				const std::string_view ident = this->source.getTokenBuffer()[struct_decl.ident].getString();

				const DepsBuffer::Scope::ID copied_scope = 
					this->context.deps_buffer.scope_manager.copyScope(this->scopes.top());
				const deps::Struct::ID deps_struct_id = this->context.deps_buffer.create_struct(copied_scope);
				const deps::Node::ID deps_node_id = 
					this->create_node(node, deps::Node::ValueStage::Comptime, deps_struct_id);
				if(parent.has_value()){
					this->add_decl_deps_decl(this->context.deps_buffer[deps_node_id], *parent);
					this->add_def_deps_def(this->context.deps_buffer[*parent], deps_node_id);
				}
				this->add_symbol(ident, deps_node_id);
				if(when_cond_parent_deps_list != nullptr){ when_cond_parent_deps_list->emplace_back(deps_node_id); }

				this->scopes.push(copied_scope);
				this->enter_scope(deps_node_id);
				const AST::Block& struct_block = this->source.getASTBuffer().getBlock(struct_decl.block);
				for(const AST::Node& stmt : struct_block.stmts){
					if(this->analyze_symbol_decl_ident<false>(stmt, deps_node_id, nullptr) == false){ return false; }
				}
				this->scopes.pop();

				return true;
			} break;

			case AST::Kind::WhenConditional: {
				return this->analyze_symbol_when_cond<IS_GLOBAL>(node, parent, when_cond_parent_deps_list);
			} break;

			case AST::Kind::FuncCall: {
				// TODO: 
				evo::unimplemented("AST::Kind::FuncCall");
			} break;

			case AST::Kind::Return:  case AST::Kind::Unreachable: case AST::Kind::Conditional:
			case AST::Kind::While:   case AST::Kind::Block:       case AST::Kind::TemplatedExpr:
			case AST::Kind::Infix:   case AST::Kind::Postfix:     case AST::Kind::MultiAssign:
			case AST::Kind::New:     case AST::Kind::Ident:       case AST::Kind::Intrinsic:
			case AST::Kind::Literal: case AST::Kind::This: {
				this->emit_error(Diagnostic::Code::DepInalidGlobalStmtKind, node, "Invalid global statement");
				return false;
			};


			case AST::Kind::TemplatePack:    case AST::Kind::Prefix:         case AST::Kind::Type:
			case AST::Kind::TypeIDConverter: case AST::Kind::AttributeBlock: case AST::Kind::Attribute:
			case AST::Kind::PrimitiveType:   case AST::Kind::Uninit:         case AST::Kind::Zeroinit:
			case AST::Kind::Discard: {
				// TODO: message the exact kind
				this->emit_fatal(
					Diagnostic::Code::DepInalidGlobalStmtKind,
					node,
					Diagnostic::createFatalMessage("Invalid global statement")
				);
				return false;
			};
		}

		evo::debugFatalBreak("Unknown AST::Kind");
	}


	template<bool IS_GLOBAL>
	auto DependencyAnalysis::analyze_symbol_when_cond(
		const AST::Node& node,
		std::optional<deps::Node::ID> parent,
		evo::SmallVector<deps::Node::ID>* when_cond_parent_deps_list
	) -> bool {
		const deps::WhenCond::ID new_deps_node_when_cond_id = this->context.deps_buffer.create_when_cond();
		const deps::Node::ID new_deps_node_id = this->create_node(
			node, deps::Node::ValueStage::Comptime, new_deps_node_when_cond_id
		);
		deps::WhenCond& deps_when_cond = this->context.deps_buffer.getWhenCond(new_deps_node_when_cond_id);

		if(parent.has_value()){ this->add_decl_deps_decl(this->context.deps_buffer[new_deps_node_id], *parent); }
		if(when_cond_parent_deps_list != nullptr){ when_cond_parent_deps_list->emplace_back(new_deps_node_id); }

		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::WhenConditional& when_cond = ast_buffer.getWhenConditional(node);

		this->enter_scope(new_deps_node_id);
		for(const AST::Node& then_block_stmt : ast_buffer.getBlock(when_cond.thenBlock).stmts){
			if(this->analyze_symbol_decl_ident<IS_GLOBAL>(
				then_block_stmt, new_deps_node_id, &deps_when_cond.thenNodes
			) == false){
				return false;
			}
		}

		if(when_cond.elseBlock.has_value()){
			if(when_cond.elseBlock->kind() == AST::Kind::Block){
				for(const AST::Node& else_block_stmt : ast_buffer.getBlock(*when_cond.elseBlock).stmts){
					const bool res = this->analyze_symbol_decl_ident<IS_GLOBAL>(
						else_block_stmt, new_deps_node_id, &deps_when_cond.elseNodes
					);
					if(res == false){ return false; }
				}
			}else{
				return this->analyze_symbol_when_cond<IS_GLOBAL>(
					*when_cond.elseBlock, new_deps_node_id, &deps_when_cond.elseNodes
				);
			}
		}
		this->leave_scope();

		return true;
	}


	auto DependencyAnalysis::analyze_symbol_deps(deps::Node::ID deps_node_id) -> bool {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		deps::Node& deps_node = this->context.deps_buffer[deps_node_id];

		switch(deps_node.astNode.kind()){
			case AST::Kind::VarDecl: {
				const AST::VarDecl& var_decl = this->source.getASTBuffer().getVarDecl(deps_node.astNode);

				// type
				if(var_decl.type.has_value()){
					const evo::Result<DepsSet> type_deps = this->analyze_type_deps(ast_buffer.getType(*var_decl.type));
					if(type_deps.isError()){ return false; }
					if(this->add_decl_deps_decl(deps_node, type_deps.value()) == false){ return false; }
					if(this->add_def_deps_def(deps_node, type_deps.value()) == false){ return false; }
				}


				// attributes
				const evo::Result<DepsSet> attribute_deps = this->analyze_attributes_deps(
					ast_buffer.getAttributeBlock(var_decl.attributeBlock)
				);
				if(attribute_deps.isError()){ return false; }
				if(this->add_decl_deps_def(deps_node, attribute_deps.value()) == false){ return false; }


				// value 
				if(var_decl.value.has_value()){
					const evo::Result<DepsInfo> value_deps = this->analyze_expr(*var_decl.value);
					if(value_deps.isError()){ return false; }

					if(var_decl.kind == AST::VarDecl::Kind::Def){
						if(value_deps.value().mightBeComptime() == false){
							// TODO: give more information as to what it is (if possible)
							this->emit_error(
								Diagnostic::Code::DepRequiredComptime,
								*var_decl.value,
								"The value of `def` variables is requried to be comptime"
							);
							return false;
						}
						for(const deps::Node::ID& dep_id : value_deps.value().decls){
							this->context.deps_buffer[dep_id].usedInComptime = true;
						}
						for(const deps::Node::ID& dep_id : value_deps.value().defs){
							this->context.deps_buffer[dep_id].usedInComptime = true;
						}
					}

					if(var_decl.type.has_value()){
						if(this->add_def_deps_def(deps_node, value_deps.value().defs) == false){ return false; }
						if(this->add_def_deps_decl(deps_node, value_deps.value().decls) == false){ return false; }
					}else{
						if(this->add_decl_deps_decl(deps_node, value_deps.value().decls) == false){ return false; }
						if(this->add_decl_deps_def(deps_node, value_deps.value().defs) == false){ return false; }
					}
				}
			} break;

			case AST::Kind::FuncDecl: {
				const AST::FuncDecl& func_decl = ast_buffer.getFuncDecl(deps_node.astNode);
				const AST::Block& func_body = ast_buffer.getBlock(func_decl.block);

				this->enter_scope(deps_node_id);
				for(const AST::Node& func_body_stmt : func_body.stmts){
					const evo::Result<DepsInfo> stmt_res = this->analyze_stmt(func_body_stmt);
					if(stmt_res.isError()){ return false; }

					if(this->add_def_deps_decl(deps_node, stmt_res.value().decls) == false){ return false; }
					if(this->add_def_deps_def(deps_node, stmt_res.value().defs) == false){ return false; }
				}
				this->leave_scope();
			} break;

			case AST::Kind::AliasDecl: {
				const AST::AliasDecl& alias_decl = ast_buffer.getAliasDecl(deps_node.astNode);

				// attributes 
				const evo::Result<DepsSet> attribute_deps = this->analyze_attributes_deps(
					ast_buffer.getAttributeBlock(alias_decl.attributeBlock)
				);
				if(attribute_deps.isError()){ return false; }
				if(this->add_decl_deps_def(deps_node, attribute_deps.value()) == false){ return false; }

				// type
				const evo::Result<DepsSet> type_deps = this->analyze_type_deps(ast_buffer.getType(alias_decl.type));
				if(type_deps.isError()){ return false; }
				if(this->add_def_deps_def(deps_node, type_deps.value()) == false){ return false; }
			} break;

			case AST::Kind::TypedefDecl: {
				const AST::TypedefDecl& typedef_decl = ast_buffer.getTypedefDecl(deps_node.astNode);

				// attributes 
				const evo::Result<DepsSet> attribute_deps = this->analyze_attributes_deps(
					ast_buffer.getAttributeBlock(typedef_decl.attributeBlock)
				);
				if(attribute_deps.isError()){ return false; }
				if(this->add_decl_deps_def(deps_node, attribute_deps.value()) == false){ return false; }

				// type
				const evo::Result<DepsSet> type_deps = this->analyze_type_deps(ast_buffer.getType(typedef_decl.type));
				if(type_deps.isError()){ return false; }
				if(this->add_def_deps_def(deps_node, type_deps.value()) == false){ return false; }
			} break;

			case AST::Kind::StructDecl: {
				this->scopes.push(
					this->context.deps_buffer.getStruct(deps_node.extraData.as<deps::StructID>()).scopeID
				);

				for(const auto& symbols : this->get_current_scope_level().symbols){
					for(const deps::Node::ID& struct_member : symbols.second){
						if(this->analyze_symbol_deps(struct_member) == false){ return false; }
					}
				}

				this->scopes.pop();
			} break;

			case AST::Kind::WhenConditional: {
				const AST::WhenConditional& when_cond_decl = ast_buffer.getWhenConditional(deps_node.astNode);

				const evo::Result<DepsInfo> value_deps = this->analyze_expr(when_cond_decl.cond);
				if(value_deps.isError()){ return false; }

				if(this->add_decl_deps_decl(deps_node, value_deps.value().decls) == false){ return false; }
				if(this->add_decl_deps_def(deps_node, value_deps.value().defs) == false){ return false; }

				for(const deps::Node::ID& dep_id : value_deps.value().decls){
					this->context.deps_buffer[dep_id].usedInComptime = true;
				}
				for(const deps::Node::ID& dep_id : value_deps.value().defs){
					this->context.deps_buffer[dep_id].usedInComptime = true;
				}
			} break;

			default: evo::debugFatalBreak("Invalid deps Node kind");
		}

		return true;
	}


	auto DependencyAnalysis::analyze_stmt(const AST::Node& stmt) -> evo::Result<DepsInfo> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(stmt.kind()){
			case AST::Kind::None: {
				this->emit_fatal(
					Diagnostic::Code::DepInvalidStmtKind, stmt, Diagnostic::createFatalMessage("Not a valid AST Node")
				);
				return evo::resultError;
			} break;

			case AST::Kind::VarDecl: {
				const AST::VarDecl& var_decl = ast_buffer.getVarDecl(stmt);

				auto deps_info = DepsInfo(DepsSet(), DepsSet(), deps::Node::ValueStage::Comptime);

				if(var_decl.type.has_value()){
					const evo::Result<DepsSet> type_deps = this->analyze_type_deps(ast_buffer.getType(*var_decl.type));
					if(type_deps.isError()){ return evo::resultError; }

					deps_info.add_to_defs(type_deps.value());
				}

				const evo::Result<DepsSet> attribute_deps = this->analyze_attributes_deps(
					ast_buffer.getAttributeBlock(var_decl.attributeBlock)
				);
				if(attribute_deps.isError()){ return evo::resultError; }
				deps_info.add_to_defs(attribute_deps.value());

				if(var_decl.value.has_value()){
					const evo::Result<DepsInfo> value_deps = this->analyze_expr(*var_decl.value);
					if(value_deps.isError()){ return evo::resultError; }

					if(var_decl.kind == AST::VarDecl::Kind::Def){
						if(value_deps.value().mightBeComptime() == false){
							this->emit_error(
								Diagnostic::Code::DepRequiredComptime,
								*var_decl.value,
								"The value of `def` variables is requried to be comptime"
							);
							return evo::resultError;
						}
					}

					deps_info += value_deps.value();
				}

				return deps_info;
			} break;

			case AST::Kind::FuncDecl: {
				// TODO: 
				evo::unimplemented("AST::Kind::FuncDecl");
			} break;

			case AST::Kind::AliasDecl: {
				// TODO: 
				evo::unimplemented("AST::Kind::AliasDecl");
			} break;

			case AST::Kind::TypedefDecl: {
				// TODO: 
				evo::unimplemented("AST::Kind::TypedefDecl");
			} break;

			case AST::Kind::StructDecl: {
				// TODO: 
				evo::unimplemented("AST::Kind::StructDecl");
			} break;

			case AST::Kind::Return: {
				const AST::Return& ast_return = ast_buffer.getReturn(stmt);

				if(ast_return.value.is<AST::Node>() == false){ return DepsInfo(); }

				return this->analyze_expr(ast_return.value.as<AST::Node>());
			} break;

			case AST::Kind::Conditional: {
				// TODO: 
				evo::unimplemented("AST::Kind::Conditional");
			} break;

			case AST::Kind::WhenConditional: {
				// TODO: 
				evo::unimplemented("AST::Kind::WhenConditional");
			} break;

			case AST::Kind::While: {
				// TODO: 
				evo::unimplemented("AST::Kind::While");
			} break;

			case AST::Kind::Unreachable: {
				// TODO: 
				evo::unimplemented("AST::Kind::Unreachable");
			} break;

			case AST::Kind::Block: {
				// TODO: 
				evo::unimplemented("AST::Kind::Block");
			} break;

			case AST::Kind::FuncCall: {
				// TODO: 
				evo::unimplemented("AST::Kind::FuncCall");
			} break;

			case AST::Kind::Infix: {
				// TODO: 
				evo::unimplemented("AST::Kind::Infix");
			} break;

			case AST::Kind::MultiAssign: {
				// TODO: 
				evo::unimplemented("AST::Kind::MultiAssign");
			} break;

			case AST::Kind::New:   case AST::Kind::TemplatedExpr: case AST::Kind::Postfix:
			case AST::Kind::Ident: case AST::Kind::Intrinsic:     case AST::Kind::Literal:
			case AST::Kind::This: {
				this->emit_error(Diagnostic::Code::DepInvalidStmtKind, stmt, "Invalid statement");
				return evo::resultError;
			} break;

			case AST::Kind::TemplatePack:    case AST::Kind::Prefix:         case AST::Kind::Type:
			case AST::Kind::TypeIDConverter: case AST::Kind::AttributeBlock: case AST::Kind::Attribute:
			case AST::Kind::PrimitiveType:   case AST::Kind::Uninit:         case AST::Kind::Zeroinit:
			case AST::Kind::Discard: {
				this->emit_fatal(
					Diagnostic::Code::DepInvalidStmtKind, stmt, Diagnostic::createFatalMessage("Invalid statement")
				);
				return evo::resultError;
			} break;
		}

		evo::debugFatalBreak("Unsupported AST::Kind");
	}


	auto DependencyAnalysis::analyze_type_deps(const AST::Type& ast_type) -> evo::Result<DepsSet> {
		const TokenBuffer& token_buffer = this->source.getTokenBuffer();
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(ast_type.base.kind()){
			case AST::Kind::PrimitiveType: {
				return DepsSet();
			} break;

			case AST::Kind::Ident: {
				DepsInfo deps_info = this->analyze_ident_deps(ast_buffer.getIdent(ast_type.base));

				if(deps_info.value_stage != deps::Node::ValueStage::Comptime){
					this->emit_error(
						Diagnostic::Code::DepRequiredComptime,
						ast_type.base,
						"Types are required to be known at compile-time"
					);
					return evo::resultError;
				}

				return deps_info.merge_and_export();
			} break;

			case AST::Kind::TypeIDConverter: {
				// TODO: 
				evo::unimplemented("AST::Kind::TypeIDConverter");
			} break;

			case AST::Kind::Infix: {
				const AST::Infix& infix_expr = ast_buffer.getInfix(ast_type.base);
				evo::debugAssert(
					token_buffer[infix_expr.opTokenID].kind() == Token::lookupKind("."), "Unexpected infix kind"
				);

				evo::Result<DepsInfo> lhs_deps = this->analyze_expr(infix_expr.lhs);
				if(lhs_deps.isError()){ return evo::resultError; }

				const DepsSet defs = std::move(lhs_deps.value().defs);
				for(const deps::Node::ID& def_id : defs){
					const deps::Node& def = this->context.deps_buffer[def_id];

					if(def.astNode.kind() != AST::Kind::StructDecl){
						lhs_deps.value().defs.emplace(def_id);
						continue;
					}

					const deps::Struct& deps_struct = 
						this->context.deps_buffer.getStruct(def.extraData.as<deps::Struct::ID>());
					this->scopes.push(deps_struct.scopeID);

					const std::string_view rhs_ident_str = 
						token_buffer[ast_buffer.getIdent(infix_expr.rhs)].getString();

					auto find = this->get_current_scope_level().symbols.find(rhs_ident_str);
					if(find == this->get_current_scope_level().symbols.end()){
						lhs_deps.value().defs.emplace(def_id);
					}else{
						lhs_deps.value().defs.reserve(lhs_deps.value().defs.size() + find->second.size());
						for(const deps::Node::ID& dep : find->second){
							lhs_deps.value().defs.emplace(dep);
						}
					}

					this->scopes.pop();
				}

				return lhs_deps.value().merge_and_export();
			} break;

			// TODO: separate out into more kinds to be more specific (errors vs fatal)
			default: {
				this->emit_error(
					Diagnostic::Code::DepInvalidBaseType, ast_type.base, "Unknown or unsupported base type"
				);
				return evo::resultError;
			} break;
		}
	}

	auto DependencyAnalysis::analyze_expr(const AST::Node& node) -> evo::Result<DepsInfo> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(node.kind()){
			case AST::Kind::None: {
				evo::debugFatalBreak("Invalid AST Node");
			} break;

			case AST::Kind::Block: {
				// TODO: 
				evo::unimplemented("AST::Kind::Block");
			} break;
			
			case AST::Kind::FuncCall: {
				const AST::FuncCall& func_call = ast_buffer.getFuncCall(node);

				auto deps_info = DepsInfo();

				const evo::Result<DepsInfo> func_call_deps = this->analyze_expr(func_call.target);
				if(func_call_deps.isError()){ return evo::resultError; }

				deps_info += func_call_deps.value();

				for(const AST::FuncCall::Arg& arg : func_call.args){
					const evo::Result<DepsInfo> arg_deps = this->analyze_expr(arg.value);
					if(arg_deps.isError()){ return evo::resultError; }

					deps_info += arg_deps.value();
				}

				return deps_info;
			} break;
			
			case AST::Kind::TemplatedExpr: {
				// TODO: 
				evo::unimplemented("AST::Kind::TemplatedExpr");
			} break;
			
			case AST::Kind::Prefix: {
				// TODO: 
				evo::unimplemented("AST::Kind::Prefix");
			} break;
			
			case AST::Kind::Infix: {
				// TODO: 
				evo::unimplemented("AST::Kind::Infix");
			} break;
			
			case AST::Kind::Postfix: {
				// TODO: 
				evo::unimplemented("AST::Kind::Postfix");
			} break;

			case AST::Kind::New: {
				// TODO: 
				evo::unimplemented("AST::Kind::New");
			} break;
			
			case AST::Kind::Ident: {
				return this->analyze_ident_deps(ast_buffer.getIdent(node));
			} break;
			
			case AST::Kind::Intrinsic: {
				return DepsInfo(DepsSet(), DepsSet(), deps::Node::ValueStage::Comptime);
			} break;
			
			case AST::Kind::Literal: {
				return DepsInfo(DepsSet(), DepsSet(), deps::Node::ValueStage::Comptime);
			} break;
			
			case AST::Kind::Uninit: {
				return DepsInfo(DepsSet(), DepsSet(), deps::Node::ValueStage::Comptime);
			} break;

			case AST::Kind::Zeroinit: {
				return DepsInfo(DepsSet(), DepsSet(), deps::Node::ValueStage::Comptime);
			} break;
			
			case AST::Kind::This: {
				return DepsInfo(DepsSet(), DepsSet(), deps::Node::ValueStage::Constexpr);
			} break;

			case AST::Kind::VarDecl:       case AST::Kind::FuncDecl:       case AST::Kind::AliasDecl:
			case AST::Kind::Return:        case AST::Kind::Conditional:    case AST::Kind::WhenConditional:
			case AST::Kind::Unreachable:   case AST::Kind::TemplatePack:   case AST::Kind::MultiAssign:
			case AST::Kind::Type:          case AST::Kind::AttributeBlock: case AST::Kind::Attribute:
			case AST::Kind::PrimitiveType: case AST::Kind::Discard: {
				evo::debugFatalBreak("Invalid AST expr kind");
			} break;
		}

		evo::debugFatalBreak("Unknown AST Kind");
	}


	auto DependencyAnalysis::analyze_attributes_deps(const AST::AttributeBlock& attribute_block)
	-> evo::Result<DepsSet> {
		auto deps_set = DepsSet();

		for(const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			for(const AST::Node& arg : attribute.args){
				evo::Result<DepsInfo> arg_info = this->analyze_expr(arg);
				if(arg_info.isError()){ return evo::resultError; }

				if(arg_info.value().mightBeComptime() == false){
					this->emit_error(
						Diagnostic::Code::DepRequiredComptime, arg, "Attribute arguments must be comptime"
					);
					return evo::resultError;
				}

				const DepsSet arg_deps = arg_info.value().merge_and_export();
				deps_set.reserve(deps_set.size() + arg_deps.size());
				for(const deps::Node::ID& dep : arg_deps){
					deps_set.emplace(dep);
				}
			}
		}

		return deps_set;
	}

	auto DependencyAnalysis::analyze_ident_deps(const Token::ID& token_id) -> DepsInfo {
		const std::string_view ident = this->source.getTokenBuffer()[token_id].getString();

		for(const DepsBuffer::ScopeLevel::ID& scope_level_id : this->get_scope()){
			const DepsBuffer::ScopeLevel& scope_level = 
				this->context.deps_buffer.scope_manager.getLevel(scope_level_id);
			auto find = scope_level.symbols.find(ident);

			if(find != scope_level.symbols.end()){
				const evo::SmallVector<deps::Node::ID>& found_deps = find->second;

				auto output = DepsInfo();


				const auto add_to_output_deps_info = [&](deps::Node::ID node_id, const deps::Node& node) -> void {
					switch(node.valueStage){
						case deps::Node::ValueStage::Comptime: {
							output.defs.emplace(node_id);
						} break;

						case deps::Node::ValueStage::Constexpr: {
							if(node.astNode.kind() != AST::Kind::VarDecl){ output.decls.emplace(node_id); }

							const AST::VarDecl& var_decl = this->source.getASTBuffer().getVarDecl(node.astNode);
							if(var_decl.type.has_value()){
								output.defs.emplace(node_id);
							}else{
								output.decls.emplace(node_id);
							}
						} break;

						case deps::Node::ValueStage::Runtime: {
							if(node.astNode.kind() != AST::Kind::VarDecl){ output.decls.emplace(node_id); }

							const AST::VarDecl& var_decl = this->source.getASTBuffer().getVarDecl(node.astNode);
							if(var_decl.type.has_value()){
								output.defs.emplace(node_id);
							}else{
								output.decls.emplace(node_id);
							}
						} break;

						case deps::Node::ValueStage::Unknown: {
							output.decls.emplace(node_id);
						} break;
					}
				};

				// TODO: optimize control flow
				const deps::Node::ValueStage value_stage = [&](){
					const deps::Node& first_node = this->context.deps_buffer[found_deps.front()];
					deps::Node::ValueStage expected_value_stage = first_node.valueStage;

					add_to_output_deps_info(found_deps.front(), first_node);

					for(size_t i = 1; i < found_deps.size(); i+=1){
						const deps::Node& ith_node = this->context.deps_buffer[found_deps[i]];
						const deps::Node::ValueStage ith_value_stage = ith_node.valueStage;
						if(expected_value_stage != ith_value_stage){
							if(
								expected_value_stage == deps::Node::ValueStage::Comptime 
								&& ith_value_stage == deps::Node::ValueStage::Constexpr
							){
								expected_value_stage = deps::Node::ValueStage::Constexpr;

							}else if(
								expected_value_stage == deps::Node::ValueStage::Constexpr 
								&& ith_value_stage == deps::Node::ValueStage::Comptime
							){
								// do nothing...

							}else{
								return deps::Node::ValueStage::Unknown;
							}
						}

						switch(ith_value_stage){
							break; case deps::Node::ValueStage::Comptime:  output.defs.emplace(found_deps[i]);
							break; case deps::Node::ValueStage::Constexpr: output.decls.emplace(found_deps[i]);
							break; case deps::Node::ValueStage::Runtime:   output.decls.emplace(found_deps[i]);
							break; case deps::Node::ValueStage::Unknown:   output.decls.emplace(found_deps[i]);
						}

						add_to_output_deps_info(found_deps[i], ith_node);
					}

					return expected_value_stage;
				}();

				output.value_stage = value_stage;
				return output;
			}
		}

		return DepsInfo();
	}




	auto DependencyAnalysis::create_node(
		const AST::Node& ast_node, deps::Node::ValueStage value_stage, deps::Node::ExtraData extra_data
	) -> deps::Node::ID {
		const deps::Node::ID new_deps_node_id = this->context.deps_buffer.create_node(
			ast_node, this->source.getID(), value_stage, extra_data
		);
		this->source.deps_node_ids.emplace_back(new_deps_node_id);
		return new_deps_node_id;
	}




	auto DependencyAnalysis::get_scope() -> DepsBuffer::Scope& {
		return this->context.deps_buffer.scope_manager.getScope(this->scopes.top());
	}

	auto DependencyAnalysis::get_current_scope_level() -> DepsBuffer::ScopeLevel& {
		return this->context.deps_buffer.scope_manager.getLevel(this->get_scope().getCurrentLevel());
	}


	auto DependencyAnalysis::enter_scope() -> void {
		this->get_scope().pushLevel(this->context.deps_buffer.scope_manager.createLevel());
	}

	auto DependencyAnalysis::enter_scope(const auto& object_scope) -> void {
		this->get_scope().pushLevel(this->context.deps_buffer.scope_manager.createLevel(), object_scope);
	}

	auto DependencyAnalysis::leave_scope() -> void {
		this->get_scope().popLevel();
	}


	auto DependencyAnalysis::add_symbol(std::string_view ident, deps::Node::ID deps_node_id) -> void {
		this->get_current_scope_level().add_symbol(ident, deps_node_id);

		const DepsBuffer::Scope& current_scope = this->get_scope();
		if(current_scope.inObjectScope()){
			const DepsBuffer::Scope::ObjectScope& current_object_scope = this->get_scope().getCurrentObjectScope();
			if(current_object_scope.is<deps::Node::ID>()){
				this->context.deps_buffer[current_object_scope.as<deps::Node::ID>()].extraData.visit(
					[&](auto& extra_data) -> void {
						using ExtraData = std::decay_t<decltype(extra_data)>;

						if constexpr(std::is_same<ExtraData, deps::Node::NoExtraData>()){
							// do nothing...

						}else if constexpr(std::is_same<ExtraData, deps::Struct::ID>()){
							this->context.deps_buffer.getStruct(extra_data).members.emplace(ident, deps_node_id);

						}else if constexpr(std::is_same<ExtraData, deps::WhenCond::ID>()){
							// do nothing...

						}else{
							static_assert(false, "Invlid deps::Node::ExtraData kind");
						}
					}
				);
			}
		}
	}



	auto DependencyAnalysis::add_global_symbol(
		std::string_view ident, Token::ID location, bool is_func, bool is_in_when_cond
	) -> bool {
		const auto find = this->source.global_decl_ident_infos_map.find(ident);

		if(find == this->source.global_decl_ident_infos_map.end()){
			const uint32_t new_info_index = [&](){
				if(is_in_when_cond){
					return this->source.global_decl_ident_infos.emplace_back(std::nullopt, is_func);
				}else{
					return this->source.global_decl_ident_infos.emplace_back(location, is_func);
				}
			}();
			this->source.global_decl_ident_infos_map.emplace(ident, new_info_index);
			return true;
		}

		if(is_in_when_cond){ return true; }

		Source::GlobalDeclIdentInfo& found_info = this->source.global_decl_ident_infos[find->second];

		if(found_info.declared_location.has_value() == false){
			found_info.declared_location = location;
			found_info.is_func = is_func;
			return true;
		}

		if(found_info.is_func && is_func){ return true; }

		this->emit_error(
			Diagnostic::Code::DepGlobalIdentAlreadyDefined,
			location,
			std::format("Global identifier \"{}\" was already defined", ident),
			Diagnostic::Info(
				"Note: first defined here:", Diagnostic::Location::get(*found_info.declared_location, this->source)
			)
		);
		return false;
	}


	auto DependencyAnalysis::add_decl_deps_decl(deps::Node& deps_node, const DepsSet& deps) -> bool {
		return this->add_deps_impl(deps_node, deps_node.declDeps.decls, deps, false);
	}

	auto DependencyAnalysis::add_decl_deps_decl(deps::Node& deps_node, const deps::Node::ID& dep) -> bool {
		return this->add_deps_impl(deps_node, deps_node.declDeps.decls, dep, false);
	}


	auto DependencyAnalysis::add_decl_deps_def(deps::Node& deps_node, const DepsSet& deps) -> bool {
		return this->add_deps_impl(deps_node, deps_node.declDeps.defs, deps, false);
	}

	auto DependencyAnalysis::add_decl_deps_def(deps::Node& deps_node, const deps::Node::ID& dep) -> bool {
		return this->add_deps_impl(deps_node, deps_node.declDeps.defs, dep, false);
	}


	auto DependencyAnalysis::add_def_deps_decl(deps::Node& deps_node, const DepsSet& deps) -> bool {
		return this->add_deps_impl(deps_node, deps_node.defDeps.decls, deps, true);
	}

	auto DependencyAnalysis::add_def_deps_decl(deps::Node& deps_node, const deps::Node::ID& dep) -> bool {
		return this->add_deps_impl(deps_node, deps_node.defDeps.decls, dep, true);
	}


	auto DependencyAnalysis::add_def_deps_def(deps::Node& deps_node, const DepsSet& deps) -> bool {
		return this->add_deps_impl(deps_node, deps_node.defDeps.defs, deps, true);
	}

	auto DependencyAnalysis::add_def_deps_def(deps::Node& deps_node, const deps::Node::ID& dep) -> bool {
		return this->add_deps_impl(deps_node, deps_node.defDeps.defs, dep, true);
	}



	auto DependencyAnalysis::add_deps_impl(
		const deps::Node& deps_node, DepsSet& target, const DepsSet& deps_to_add, bool is_defs
	) -> bool {
		target.reserve(target.size() + deps_to_add.size());
		for(const deps::Node::ID& dep : deps_to_add){
			if(this->add_deps_impl(deps_node, target, dep, is_defs) == false){ return false; }
		}
		return true;
	}


	auto DependencyAnalysis::add_deps_impl(
		const deps::Node& deps_node, DepsSet& target, const deps::Node::ID& dep_to_add, bool is_defs
	) -> bool {
		struct VisitedNode{
			const deps::Node* node;
			bool is_defs;
		};

		auto visited_queue = std::queue<VisitedNode>();

		visited_queue.emplace(&deps_node, is_defs);
		visited_queue.emplace(&this->context.deps_buffer[dep_to_add], is_defs);


		bool first = true;
		while(visited_queue.empty() == false){
			const VisitedNode visited_node = visited_queue.front();
			visited_queue.pop();

			if(!first && visited_node.node == &deps_node && is_defs <= visited_node.is_defs){
				this->emit_error(
					Diagnostic::Code::DepCircularDep,
					deps_node.astNode,
					"Detected a circular dependency when analyzing this symbol:",
					Diagnostic::Info(
						std::format("Requires the {} of this symbol:", is_defs ? "declaration" : "definition"),
						Diagnostic::Location::get(this->context.deps_buffer[dep_to_add].astNode, this->source)
					)
				);
				return false;
			}
			first = false;

			if(visited_node.is_defs){
				for(const deps::Node::ID& dep : visited_node.node->defDeps.decls){
					visited_queue.emplace(&this->context.deps_buffer[dep], false);
				}

				for(const deps::Node::ID& dep : visited_node.node->defDeps.defs){
					visited_queue.emplace(&this->context.deps_buffer[dep], true);
				}

			}else{
				for(const deps::Node::ID& dep : visited_node.node->declDeps.decls){
					visited_queue.emplace(&this->context.deps_buffer[dep], false);
				}

				for(const deps::Node::ID& dep : visited_node.node->declDeps.defs){
					visited_queue.emplace(&this->context.deps_buffer[dep], true);
				}
			}
		}

		target.emplace(dep_to_add);
		return true;
	}




	auto DependencyAnalysis::DepsInfo::operator+=(const DepsInfo& rhs) -> DepsInfo& {
		this->decls.reserve(this->decls.size() + rhs.decls.size());
		for(const deps::Node::ID& dep : rhs.decls){
			this->decls.emplace(dep);
		}

		this->defs.reserve(this->defs.size() + rhs.defs.size());
		for(const deps::Node::ID& dep : rhs.defs){
			this->defs.emplace(dep);
		}

		if(this->value_stage != rhs.value_stage){
			switch(rhs.value_stage){
				case deps::Node::ValueStage::Comptime: {
					switch(this->value_stage){
						case deps::Node::ValueStage::Comptime: evo::unreachable();

						case deps::Node::ValueStage::Constexpr:
							this->value_stage = deps::Node::ValueStage::Constexpr;
							break;

						case deps::Node::ValueStage::Runtime:
							this->value_stage = deps::Node::ValueStage::Constexpr;
							break;

						case deps::Node::ValueStage::Unknown: break;
					}
				} break;

				case deps::Node::ValueStage::Constexpr: {
					if(this->value_stage == deps::Node::ValueStage::Unknown){ break; }
					if(this->value_stage != deps::Node::ValueStage::Runtime){
						this->value_stage = deps::Node::ValueStage::Constexpr;
					}
				} break;

				case deps::Node::ValueStage::Runtime: {
					if(this->value_stage == deps::Node::ValueStage::Unknown){ break; }
					this->value_stage = deps::Node::ValueStage::Runtime;
				} break;

				case deps::Node::ValueStage::Unknown: {
					this->value_stage = deps::Node::ValueStage::Unknown;
				} break;
			}
		}

		return *this;
	}


}