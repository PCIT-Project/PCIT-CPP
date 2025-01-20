////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./DependencyAnalysis.h"

namespace pcit::panther{
	

	auto DependencyAnalysis::analyze() -> bool {
		this->source.dg_scope_id = this->context.dg_buffer.scope_manager.createScope();
		this->scopes.push(*this->source.dg_scope_id);
		this->enter_scope();

		for(const AST::Node& ast_node : this->source.getASTBuffer().getGlobalStmts()){
			if(this->analyze_global_decl_ident(ast_node, std::nullopt, nullptr) == false){ return false; }
		}

		for(const DG::Node::ID& dg_node_id : this->source.dg_node_ids){
			if(this->analyze_symbol_deps(dg_node_id) == false){ return false; }
		}

		// get `requiredBy`s
		for(const DG::Node::ID& dg_node_id : this->source.dg_node_ids){
			const DG::Node& dg_node = this->context.dg_buffer[dg_node_id];

			for(const DG::Node::ID& dep_id : dg_node.declDeps.decls){
				this->context.dg_buffer[dep_id].requiredBy.emplace(dg_node_id);
			}

			for(const DG::Node::ID& dep_id : dg_node.declDeps.defs){
				this->context.dg_buffer[dep_id].requiredBy.emplace(dg_node_id);
			}

			for(const DG::Node::ID& dep_id : dg_node.defDeps.decls){
				this->context.dg_buffer[dep_id].requiredBy.emplace(dg_node_id);
			}

			for(const DG::Node::ID& dep_id : dg_node.defDeps.defs){
				this->context.dg_buffer[dep_id].requiredBy.emplace(dg_node_id);
			}
		}

		return true;
	}


	auto DependencyAnalysis::analyze_global_decl_ident(
		const AST::Node& node,
		std::optional<DG::Node::ID> parent,
		evo::SmallVector<DG::Node::ID>* when_cond_parent_deps_list
	) -> bool {
		switch(node.kind()){
			case AST::Kind::None: {
				evo::debugFatalBreak("Invalid AST node");
			} break;

			case AST::Kind::VarDecl: {
				const AST::VarDecl& var_decl = this->source.getASTBuffer().getVarDecl(node);
				const std::string_view ident = this->source.getTokenBuffer()[var_decl.ident].getString();

				const DG::Node::UsageKind usage_kind = [&](){
					switch(var_decl.kind){
						case AST::VarDecl::Kind::Var:   return DG::Node::UsageKind::Runtime;
						case AST::VarDecl::Kind::Const: return DG::Node::UsageKind::Constexpr;
						case AST::VarDecl::Kind::Def:   return DG::Node::UsageKind::Comptime;
					}
					evo::unreachable();
				}();

				const DG::Node::ID dg_node_id = this->create_node(node, usage_kind);
				if(parent.has_value()){ this->add_deps(this->context.dg_buffer[dg_node_id].declDeps.decls, *parent); }
				this->add_symbol(ident, dg_node_id);

				return true;
			} break;

			case AST::Kind::FuncDecl: {
				const AST::FuncDecl& func_decl = this->source.getASTBuffer().getFuncDecl(node);

				if(func_decl.name.kind() != AST::Kind::Ident){
					// TODO: better messaging
					this->emit_error(Diagnostic::Code::DepInalidGlobalStmtKind, node, "Invalid global function");
					return false;
				}

				const std::string_view ident = this->source.getTokenBuffer()[
					this->source.getASTBuffer().getIdent(func_decl.name)
				].getString();

				const DG::Node::ID dg_node_id = this->create_node(node, DG::Node::UsageKind::Unknown);
				if(parent.has_value()){ this->add_deps(this->context.dg_buffer[dg_node_id].declDeps.decls, *parent); }
				this->add_symbol(ident, dg_node_id);
				if(when_cond_parent_deps_list != nullptr){ when_cond_parent_deps_list->emplace_back(dg_node_id); }

				return true;
			} break;

			case AST::Kind::AliasDecl: {
				const AST::AliasDecl& alias_decl = this->source.getASTBuffer().getAliasDecl(node);
				const std::string_view ident = this->source.getTokenBuffer()[alias_decl.ident].getString();

				const DG::Node::ID dg_node_id = this->create_node(node, DG::Node::UsageKind::Comptime);
				if(parent.has_value()){ this->add_deps(this->context.dg_buffer[dg_node_id].declDeps.decls, *parent); }
				this->add_symbol(ident, dg_node_id);
				if(when_cond_parent_deps_list != nullptr){ when_cond_parent_deps_list->emplace_back(dg_node_id); }

				return true;
			} break;

			case AST::Kind::TypedefDecl: {
				const AST::TypedefDecl& typedef_decl = this->source.getASTBuffer().getTypedefDecl(node);
				const std::string_view ident = this->source.getTokenBuffer()[typedef_decl.ident].getString();

				const DG::Node::ID dg_node_id = this->create_node(node, DG::Node::UsageKind::Comptime);
				if(parent.has_value()){ this->add_deps(this->context.dg_buffer[dg_node_id].declDeps.decls, *parent); }
				this->add_symbol(ident, dg_node_id);
				if(when_cond_parent_deps_list != nullptr){ when_cond_parent_deps_list->emplace_back(dg_node_id); }

				return true;
			} break;

			case AST::Kind::StructDecl: {
				const AST::StructDecl& struct_decl = this->source.getASTBuffer().getStructDecl(node);
				const std::string_view ident = this->source.getTokenBuffer()[struct_decl.ident].getString();

				const DGBuffer::Scope::ID copied_scope = 
					this->context.dg_buffer.scope_manager.copyScope(this->scopes.top());
				const DG::Struct::ID dg_struct_id = this->context.dg_buffer.create_struct(copied_scope);
				const DG::Node::ID dg_node_id = this->create_node(node, DG::Node::UsageKind::Comptime, dg_struct_id);
				if(parent.has_value()){ this->add_deps(this->context.dg_buffer[dg_node_id].declDeps.decls, *parent); }
				this->add_symbol(ident, dg_node_id);
				if(when_cond_parent_deps_list != nullptr){ when_cond_parent_deps_list->emplace_back(dg_node_id); }

				this->scopes.push(copied_scope);
				this->enter_scope(dg_node_id);
				const AST::Block& struct_block = this->source.getASTBuffer().getBlock(struct_decl.block);
				for(const AST::Node& stmt : struct_block.stmts){
					if(this->analyze_global_decl_ident(stmt, dg_node_id, nullptr) == false){ return false; }
				}
				this->scopes.pop();

				return true;
			} break;

			case AST::Kind::WhenConditional: {
				return this->analyze_global_when_cond(node, parent, when_cond_parent_deps_list);
			} break;

			case AST::Kind::FuncCall: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
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



	auto DependencyAnalysis::analyze_global_when_cond(
		const AST::Node& node,
		std::optional<DG::Node::ID> parent,
		evo::SmallVector<DG::Node::ID>* when_cond_parent_deps_list
	) -> bool {
		const DG::WhenCond::ID new_dg_node_when_cond_id = this->context.dg_buffer.create_when_cond();
		const DG::Node::ID new_dg_node_id = this->create_node(
			node, DG::Node::UsageKind::Comptime, new_dg_node_when_cond_id
		);
		DG::Node& dg_node = this->context.dg_buffer[new_dg_node_id];
		DG::WhenCond& dg_when_cond = this->context.dg_buffer.getWhenCond(new_dg_node_when_cond_id);

		if(parent.has_value()){ this->add_deps(dg_node.declDeps.decls, *parent); }
		if(when_cond_parent_deps_list != nullptr){ when_cond_parent_deps_list->emplace_back(new_dg_node_id); }

		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::WhenConditional& when_cond = ast_buffer.getWhenConditional(node);

		this->enter_scope(new_dg_node_id);
		for(const AST::Node& then_block_stmt : ast_buffer.getBlock(when_cond.thenBlock).stmts){
			if(this->analyze_global_decl_ident(then_block_stmt, new_dg_node_id, &dg_when_cond.thenNodes) == false){
				return false;
			}
		}

		if(when_cond.elseBlock.has_value()){
			if(when_cond.elseBlock->kind() == AST::Kind::Block){
				for(const AST::Node& else_block_stmt : ast_buffer.getBlock(*when_cond.elseBlock).stmts){
					const bool res = this->analyze_global_decl_ident(
						else_block_stmt, new_dg_node_id, &dg_when_cond.elseNodes
					);
					if(res == false){ return false; }
				}
			}else{
				return this->analyze_global_when_cond(*when_cond.elseBlock, new_dg_node_id, &dg_when_cond.elseNodes);
			}
		}
		this->leave_scope();

		return true;
	}


	auto DependencyAnalysis::analyze_symbol_deps(DG::Node::ID dg_node_id) -> bool {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		DG::Node& dg_node = this->context.dg_buffer[dg_node_id];

		switch(dg_node.astNode.kind()){
			case AST::Kind::VarDecl: {
				const AST::VarDecl& var_decl = this->source.getASTBuffer().getVarDecl(dg_node.astNode);

				// type
				if(var_decl.type.has_value()){
					const evo::Result<Deps> type_deps = this->analyze_type_deps(ast_buffer.getType(*var_decl.type));
					if(type_deps.isError()){ return false; }
					if(this->add_deps(dg_node.declDeps.decls, type_deps.value()) == false){ return false; }
				}


				// attributes
				const evo::Result<Deps> attribute_deps = this->analyze_attributes_deps(
					ast_buffer.getAttributeBlock(var_decl.attributeBlock)
				);
				if(attribute_deps.isError()){ return false; }
				if(this->add_deps(dg_node.declDeps.defs, attribute_deps.value()) == false){ return false; }


				// value 
				if(var_decl.value.has_value()){
					const evo::Result<DepsInfo> value_deps = this->analyze_expr(*var_decl.value);
					if(value_deps.isError()){ return false; }

					if(var_decl.kind == AST::VarDecl::Kind::Def){
						if(value_deps.value().mightBeComptime() == false){
							this->emit_error(
								Diagnostic::Code::DepRequiredComptime,
								*var_decl.value,
								"The value of `def` variables is requried to be comptime"
							);
							return false;
						}
						for(const DG::Node::ID& dep_id : value_deps.value().decls){
							this->context.dg_buffer[dep_id].usedInComptime = true;
						}
						for(const DG::Node::ID& dep_id : value_deps.value().defs){
							this->context.dg_buffer[dep_id].usedInComptime = true;
						}
					}

					if(var_decl.type.has_value()){
						if(this->add_deps(dg_node.defDeps.defs, value_deps.value().defs) == false){ return false; }
						if(this->add_deps(dg_node.defDeps.decls, value_deps.value().decls) == false){ return false; }
					}else{
						if(this->add_deps(dg_node.declDeps.decls, value_deps.value().decls) == false){ return false; }
						if(this->add_deps(dg_node.declDeps.defs, value_deps.value().defs) == false){ return false; }
					}
				}
			} break;

			case AST::Kind::FuncDecl: {
				const AST::FuncDecl& func_decl = ast_buffer.getFuncDecl(dg_node.astNode);
				const AST::Block& func_body = ast_buffer.getBlock(func_decl.block);

				this->enter_scope(dg_node_id);
				for(const AST::Node& func_body_stmt : func_body.stmts){
					const evo::Result<DepsInfo> stmt_res = this->analyze_stmt(func_body_stmt);
					if(stmt_res.isError()){ return false; }

					if(this->add_deps(dg_node.defDeps.decls, stmt_res.value().decls) == false){ return false; }
					if(this->add_deps(dg_node.defDeps.defs, stmt_res.value().defs) == false){ return false; }
				}
				this->leave_scope();
			} break;

			case AST::Kind::AliasDecl: {
				const AST::AliasDecl& alias_decl = ast_buffer.getAliasDecl(dg_node.astNode);

				// attributes 
				const evo::Result<Deps> attribute_deps = this->analyze_attributes_deps(
					ast_buffer.getAttributeBlock(alias_decl.attributeBlock)
				);
				if(attribute_deps.isError()){ return false; }
				if(this->add_deps(dg_node.declDeps.defs, attribute_deps.value()) == false){ return false; }

				// type
				const evo::Result<Deps> type_deps = this->analyze_type_deps(ast_buffer.getType(alias_decl.type));
				if(type_deps.isError()){ return false; }
				if(this->add_deps(dg_node.defDeps.defs, type_deps.value()) == false){ return false; }
			} break;

			case AST::Kind::TypedefDecl: {
				const AST::TypedefDecl& typedef_decl = ast_buffer.getTypedefDecl(dg_node.astNode);

				// attributes 
				const evo::Result<Deps> attribute_deps = this->analyze_attributes_deps(
					ast_buffer.getAttributeBlock(typedef_decl.attributeBlock)
				);
				if(attribute_deps.isError()){ return false; }
				if(this->add_deps(dg_node.declDeps.defs, attribute_deps.value()) == false){ return false; }

				// type
				const evo::Result<Deps> type_deps = this->analyze_type_deps(ast_buffer.getType(typedef_decl.type));
				if(type_deps.isError()){ return false; }
				if(this->add_deps(dg_node.defDeps.defs, type_deps.value()) == false){ return false; }
			} break;

			case AST::Kind::StructDecl: {
				this->scopes.push(this->context.dg_buffer.getStruct(dg_node.extraData.as<DG::StructID>()).scopeID);

				for(const auto& symbols : this->get_current_scope_level().symbols){
					for(const DG::Node::ID& struct_member : symbols.second){
						if(this->analyze_symbol_deps(struct_member) == false){ return false; }
					}
				}

				this->scopes.pop();
			} break;

			case AST::Kind::WhenConditional: {
				const AST::WhenConditional& when_cond_decl = ast_buffer.getWhenConditional(dg_node.astNode);

				const evo::Result<DepsInfo> value_deps = this->analyze_expr(when_cond_decl.cond);
				if(value_deps.isError()){ return false; }

				if(this->add_deps(dg_node.declDeps.decls, value_deps.value().decls) == false){ return false; }
				if(this->add_deps(dg_node.declDeps.defs, value_deps.value().defs) == false){ return false; }

				for(const DG::Node::ID& dep_id : value_deps.value().decls){
					this->context.dg_buffer[dep_id].usedInComptime = true;
				}
				for(const DG::Node::ID& dep_id : value_deps.value().defs){
					this->context.dg_buffer[dep_id].usedInComptime = true;
				}
			} break;

			default: evo::debugFatalBreak("Invalid DG Node kind");
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

				auto deps_info = DepsInfo(Deps(), Deps(), DG::Node::UsageKind::Comptime);

				if(var_decl.type.has_value()){
					const evo::Result<Deps> type_deps = this->analyze_type_deps(ast_buffer.getType(*var_decl.type));
					if(type_deps.isError()){ return evo::resultError; }

					deps_info.add_to_defs(type_deps.value());
				}

				const evo::Result<Deps> attribute_deps = this->analyze_attributes_deps(
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
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::AliasDecl: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::TypedefDecl: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::StructDecl: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::Return: {
				const AST::Return& ast_return = ast_buffer.getReturn(stmt);

				if(ast_return.value.is<AST::Node>() == false){ return DepsInfo(); }

				return this->analyze_expr(ast_return.value.as<AST::Node>());
			} break;

			case AST::Kind::Conditional: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::WhenConditional: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::While: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::Unreachable: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::Block: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::FuncCall: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::Infix: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::MultiAssign: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
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


	auto DependencyAnalysis::analyze_type_deps(const AST::Type& ast_type) -> evo::Result<Deps> {
		const TokenBuffer& token_buffer = this->source.getTokenBuffer();
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(ast_type.base.kind()){
			case AST::Kind::PrimitiveType: {
				return Deps();
			} break;

			case AST::Kind::Ident: {
				DepsInfo deps_info = this->analyze_ident_deps(ast_buffer.getIdent(ast_type.base));

				if(deps_info.usage_kind != DG::Node::UsageKind::Comptime){
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
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::Infix: {
				const AST::Infix& infix_expr = ast_buffer.getInfix(ast_type.base);
				evo::debugAssert(
					token_buffer[infix_expr.opTokenID].kind() == Token::lookupKind("."), "Unexpected infix kind"
				);

				evo::Result<DepsInfo> lhs_deps = this->analyze_expr(infix_expr.lhs);
				if(lhs_deps.isError()){ return evo::resultError; }

				const Deps defs = std::move(lhs_deps.value().defs);
				for(const DG::Node::ID& def_id : defs){
					const DG::Node& def = this->context.dg_buffer[def_id];

					if(def.astNode.kind() != AST::Kind::StructDecl){
						lhs_deps.value().defs.emplace(def_id);
						continue;
					}

					const DG::Struct& dg_struct = this->context.dg_buffer.getStruct(def.extraData.as<DG::Struct::ID>());
					this->scopes.push(dg_struct.scopeID);

					const std::string_view rhs_ident_str = 
						token_buffer[ast_buffer.getIdent(infix_expr.rhs)].getString();

					auto find = this->get_current_scope_level().symbols.find(rhs_ident_str);
					if(find == this->get_current_scope_level().symbols.end()){
						lhs_deps.value().defs.emplace(def_id);
					}else{
						lhs_deps.value().defs.reserve(lhs_deps.value().defs.size() + find->second.size());
						for(const DG::Node::ID& dep : find->second){
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
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;
			
			case AST::Kind::FuncCall: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;
			
			case AST::Kind::TemplatedExpr: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;
			
			case AST::Kind::Prefix: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;
			
			case AST::Kind::Infix: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;
			
			case AST::Kind::Postfix: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;

			case AST::Kind::New: {
				// TODO: 
				evo::debugFatalBreak("UNIMPLEMENTED");
			} break;
			
			case AST::Kind::Ident: {
				return this->analyze_ident_deps(ast_buffer.getIdent(node));
			} break;
			
			case AST::Kind::Intrinsic: {
				return DepsInfo(Deps(), Deps(), DG::Node::UsageKind::Comptime);
			} break;
			
			case AST::Kind::Literal: {
				return DepsInfo(Deps(), Deps(), DG::Node::UsageKind::Comptime);
			} break;
			
			case AST::Kind::Uninit: {
				return DepsInfo(Deps(), Deps(), DG::Node::UsageKind::Comptime);
			} break;

			case AST::Kind::Zeroinit: {
				return DepsInfo(Deps(), Deps(), DG::Node::UsageKind::Comptime);
			} break;
			
			case AST::Kind::This: {
				return DepsInfo(Deps(), Deps(), DG::Node::UsageKind::Constexpr);
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


	auto DependencyAnalysis::analyze_attributes_deps(const AST::AttributeBlock& attribute_block) -> evo::Result<Deps> {
		auto deps = Deps();

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

				const Deps arg_deps = arg_info.value().merge_and_export();
				deps.reserve(deps.size() + arg_deps.size());
				for(const DG::Node::ID& dep : arg_deps){
					deps.emplace(dep);
				}
			}
		}

		return deps;
	}

	auto DependencyAnalysis::analyze_ident_deps(const Token::ID& token_id) -> DepsInfo {
		const std::string_view ident = this->source.getTokenBuffer()[token_id].getString();

		for(const DGBuffer::ScopeLevel::ID& scope_level_id : this->get_scope()){
			const DGBuffer::ScopeLevel& scope_level = this->context.dg_buffer.scope_manager.getLevel(scope_level_id);
			auto find = scope_level.symbols.find(ident);

			if(find != scope_level.symbols.end()){
				const evo::SmallVector<DG::Node::ID>& found_deps = find->second;

				auto output = DepsInfo();

				// TODO: optimize control flow
				const DG::Node::UsageKind usage_kind = [&](){
					DG::Node::UsageKind expected_usage_kind = this->context.dg_buffer[found_deps.front()].usageKind;

					switch(expected_usage_kind){
						break; case DG::Node::UsageKind::Comptime:  output.defs.emplace(found_deps[0]);
						break; case DG::Node::UsageKind::Constexpr: output.decls.emplace(found_deps[0]);
						break; case DG::Node::UsageKind::Runtime:   output.decls.emplace(found_deps[0]);
						break; case DG::Node::UsageKind::Unknown:   output.decls.emplace(found_deps[0]);
					}

					for(size_t i = 1; i < found_deps.size(); i+=1){
						const DG::Node::UsageKind ith_usage_kind = this->context.dg_buffer[found_deps[i]].usageKind;
						if(expected_usage_kind != ith_usage_kind){
							if(
								expected_usage_kind == DG::Node::UsageKind::Comptime 
								&& ith_usage_kind == DG::Node::UsageKind::Constexpr
							){
								expected_usage_kind = DG::Node::UsageKind::Constexpr;

							}else if(
								expected_usage_kind == DG::Node::UsageKind::Constexpr 
								&& ith_usage_kind == DG::Node::UsageKind::Comptime
							){
								// do nothing...

							}else{
								return DG::Node::UsageKind::Unknown;
							}
						}

						switch(ith_usage_kind){
							break; case DG::Node::UsageKind::Comptime:  output.defs.emplace(found_deps[i]);
							break; case DG::Node::UsageKind::Constexpr: output.decls.emplace(found_deps[i]);
							break; case DG::Node::UsageKind::Runtime:   output.decls.emplace(found_deps[i]);
							break; case DG::Node::UsageKind::Unknown:   output.decls.emplace(found_deps[i]);
						}
					}

					return expected_usage_kind;
				}();

				output.usage_kind = usage_kind;
				return output;
			}
		}

		return DepsInfo();
	}




	auto DependencyAnalysis::create_node(
		const AST::Node& ast_node, DG::Node::UsageKind usage_kind, DG::Node::ExtraData extra_data
	) -> DG::Node::ID {
		const DG::Node::ID new_dg_node_id = this->context.dg_buffer.create_node(
			ast_node, this->source.getID(), usage_kind, extra_data
		);
		this->source.dg_node_ids.emplace_back(new_dg_node_id);
		return new_dg_node_id;
	}




	auto DependencyAnalysis::get_scope() -> DGBuffer::Scope& {
		return this->context.dg_buffer.scope_manager.getScope(this->scopes.top());
	}

	auto DependencyAnalysis::get_current_scope_level() -> DGBuffer::ScopeLevel& {
		return this->context.dg_buffer.scope_manager.getLevel(this->get_scope().getCurrentLevel());
	}


	auto DependencyAnalysis::enter_scope() -> void {
		this->get_scope().pushLevel(this->context.dg_buffer.scope_manager.createLevel());
	}

	auto DependencyAnalysis::enter_scope(const auto& object_scope) -> void {
		this->get_scope().pushLevel(this->context.dg_buffer.scope_manager.createLevel(), object_scope);
	}

	auto DependencyAnalysis::leave_scope() -> void {
		this->get_scope().popLevel();
	}


	auto DependencyAnalysis::add_symbol(std::string_view ident, DG::Node::ID dg_node_id) -> void {
		this->get_current_scope_level().add_symbol(ident, dg_node_id);

		const DGBuffer::Scope& current_scope = this->get_scope();
		if(current_scope.inObjectScope()){
			const DGBuffer::Scope::ObjectScope& current_object_scope = this->get_scope().getCurrentObjectScope();
			if(current_object_scope.is<DG::Node::ID>()){
				this->context.dg_buffer[current_object_scope.as<DG::Node::ID>()].extraData.visit(
					[&](auto& extra_data) -> void {
						using ExtraData = std::decay_t<decltype(extra_data)>;

						if constexpr(std::is_same<ExtraData, DG::Node::NoExtraData>()){
							// do nothing...

						}else if constexpr(std::is_same<ExtraData, DG::Struct::ID>()){
							this->context.dg_buffer.getStruct(extra_data).members.emplace(ident, dg_node_id);

						}else if constexpr(std::is_same<ExtraData, DG::WhenCond::ID>()){
							// do nothing...

						}else{
							static_assert(false, "Invlid DG::Node::ExtraData kind");
						}
					}
				);
			}
		}
	}



	auto DependencyAnalysis::add_deps(Deps& target, const Deps& deps) const -> bool {
		target.reserve(target.size() + deps.size());
		for(const DG::Node::ID& dep : deps){
			if(this->add_deps(target, dep) == false){ return false; }
		}
		return true;
	}


	auto DependencyAnalysis::add_deps(Deps& target, const DG::Node::ID& dep) const -> bool {
		target.emplace(dep);
		return true;
	}




	auto DependencyAnalysis::DepsInfo::operator+=(const DepsInfo& rhs) -> DepsInfo& {
		this->decls.reserve(this->decls.size() + rhs.decls.size());
		for(const DG::Node::ID& dep : rhs.decls){
			this->decls.emplace(dep);
		}

		this->defs.reserve(this->defs.size() + rhs.defs.size());
		for(const DG::Node::ID& dep : rhs.defs){
			this->defs.emplace(dep);
		}

		if(this->usage_kind != rhs.usage_kind){
			switch(rhs.usage_kind){
				case DG::Node::UsageKind::Comptime: {
					switch(this->usage_kind){
						case DG::Node::UsageKind::Comptime: evo::unreachable();

						case DG::Node::UsageKind::Constexpr:
							this->usage_kind = DG::Node::UsageKind::Constexpr;
							break;

						case DG::Node::UsageKind::Runtime:
							this->usage_kind = DG::Node::UsageKind::Constexpr;
							break;

						case DG::Node::UsageKind::Unknown: break;
					}
				} break;

				case DG::Node::UsageKind::Constexpr: {
					if(this->usage_kind == DG::Node::UsageKind::Unknown){ break; }
					if(this->usage_kind != DG::Node::UsageKind::Runtime){
						this->usage_kind = DG::Node::UsageKind::Constexpr;
					}
				} break;

				case DG::Node::UsageKind::Runtime: {
					if(this->usage_kind == DG::Node::UsageKind::Unknown){ break; }
					this->usage_kind = DG::Node::UsageKind::Runtime;
				} break;

				case DG::Node::UsageKind::Unknown: {
					this->usage_kind = DG::Node::UsageKind::Unknown;
				} break;
			}
		}

		return *this;
	}


}