////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SymbolProcBuilder.h"

#include "./SymbolProcManager.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther{

	using Instruction = SymbolProc::Instruction;
	

	auto SymbolProcBuilder::build(const AST::Node& stmt) -> evo::Result<> {
		const evo::Result<std::string_view> symbol_ident = this->get_symbol_ident(stmt);
		if(symbol_ident.isError()){ return evo::resultError; }

		SymbolProc* parent_symbol = (this->symbol_proc_infos.empty() == false) 
			? &this->symbol_proc_infos.back().symbol_proc
			: nullptr;

		SymbolProc::ID symbol_proc_id = this->context.symbol_proc_manager.create_symbol_proc(
			stmt, this->source.getID(), symbol_ident.value(), parent_symbol
		);
		SymbolProc& symbol_proc = this->context.symbol_proc_manager.getSymbolProc(symbol_proc_id);

		this->symbol_proc_infos.emplace_back(symbol_proc_id, symbol_proc);
		
		switch(stmt.kind()){
			break; case AST::Kind::VAR_DECL:
				if(this->build_var_decl(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::FUNC_DECL:
				if(this->build_func_decl(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::ALIAS_DECL:
				if(this->build_alias_decl(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::TYPEDEF_DECL:
				if(this->build_typedef_decl(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::STRUCT_DECL:
				if(this->build_struct_decl(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::WHEN_CONDITIONAL:
				if(this->build_when_conditional(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::FUNC_CALL:
				if(this->build_func_call(stmt).isError()){ return evo::resultError; }

			break; default: evo::unreachable();
		}

		symbol_proc.term_infos.resize(this->get_current_symbol().num_term_infos);
		symbol_proc.type_ids.resize(this->get_current_symbol().num_type_ids);
		symbol_proc.struct_instantiations.resize(this->get_current_symbol().num_struct_instantiations);

		if(this->get_current_symbol().is_template == false){
			for(auto iter = this->symbol_proc_infos.rbegin(); iter != this->symbol_proc_infos.rend(); ++iter){
				if(iter->is_template){
					symbol_proc.setIsTemplateSubSymbol();
					this->context.symbol_proc_manager.num_procs_not_done -= 1;
					break;
				}
			}
		}


		this->symbol_proc_infos.pop_back();

		this->context.trace("Finished building symbol proc of \"{}\"", symbol_ident.value());

		return evo::Result<>();
	}



	auto SymbolProcBuilder::buildTemplateInstance(
		const SymbolProc& template_symbol_proc,
		BaseType::StructTemplate::Instantiation& instantiation,
		sema::ScopeManager::Scope::ID sema_scope_id,
		uint32_t instantiation_id
	) -> evo::Result<SymbolProc::ID> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::StructDecl& struct_decl = ast_buffer.getStructDecl(template_symbol_proc.ast_node);

		SymbolProc::ID symbol_proc_id = this->context.symbol_proc_manager.create_symbol_proc(
			template_symbol_proc.ast_node,
			template_symbol_proc.source_id,
			template_symbol_proc.ident,
			template_symbol_proc.parent
		);
		SymbolProc& symbol_proc = this->context.symbol_proc_manager.getSymbolProc(symbol_proc_id);

		symbol_proc.sema_scope_id = sema_scope_id;

		this->symbol_proc_infos.emplace_back(symbol_proc_id, symbol_proc);


		///////////////////////////////////
		// build struct decl

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
			this->analyze_attributes(ast_buffer.getAttributeBlock(struct_decl.attributeBlock));
		if(attribute_params_info.isError()){ return evo::resultError; }

		this->add_instruction(
			Instruction::StructDecl<true>(struct_decl, std::move(attribute_params_info.value()), instantiation_id)
		);
		this->add_instruction(Instruction::StructDef());

		SymbolProc::StructInfo& struct_info = this->get_current_symbol().symbol_proc.extra_info
			.emplace<SymbolProc::StructInfo>(&instantiation);

		this->symbol_scopes.emplace_back(&struct_info.stmts);
		this->symbol_namespaces.emplace_back(&struct_info.member_symbols);
		for(const AST::Node& struct_stmt : ast_buffer.getBlock(struct_decl.block).stmts){
			if(this->build(struct_stmt).isError()){ return evo::resultError; }
		}
		this->symbol_namespaces.pop_back();
		this->symbol_scopes.pop_back();


		///////////////////////////////////
		// done

		symbol_proc.term_infos.resize(this->get_current_symbol().num_term_infos);
		symbol_proc.type_ids.resize(this->get_current_symbol().num_type_ids);
		symbol_proc.struct_instantiations.resize(this->get_current_symbol().num_struct_instantiations);

		this->symbol_proc_infos.pop_back();

		this->context.trace(
			"Finished building template instantiation symbol proc of \"{}\"", template_symbol_proc.ident
		);

		return symbol_proc_id;
	}






	auto SymbolProcBuilder::get_symbol_ident(const AST::Node& stmt) -> evo::Result<std::string_view> {
		const TokenBuffer& token_buffer = this->source.getTokenBuffer();
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(stmt.kind()){
			case AST::Kind::NONE: evo::debugFatalBreak("Not a valid AST node");

			case AST::Kind::VAR_DECL: {
				return token_buffer[ast_buffer.getVarDecl(stmt).ident].getString();
			} break;

			case AST::Kind::FUNC_DECL: {
				const AST::FuncDecl& func_decl = ast_buffer.getFuncDecl(stmt);
				if(func_decl.name.kind() == AST::Kind::IDENT){
					return token_buffer[ast_buffer.getIdent(func_decl.name)].getString();
				}else{
					return std::string_view();
				}
			} break;

			case AST::Kind::ALIAS_DECL: {
				return token_buffer[ast_buffer.getAliasDecl(stmt).ident].getString();
			} break;

			case AST::Kind::TYPEDEF_DECL: {
				return token_buffer[ast_buffer.getTypedefDecl(stmt).ident].getString();
			} break;

			case AST::Kind::STRUCT_DECL: {
				return token_buffer[ast_buffer.getStructDecl(stmt).ident].getString();
			} break;

			case AST::Kind::RETURN: {
				return std::string_view();
			} break;

			case AST::Kind::ERROR: {
				return std::string_view();
			} break;


			case AST::Kind::CONDITIONAL:     case AST::Kind::WHEN_CONDITIONAL: case AST::Kind::WHILE:
			case AST::Kind::UNREACHABLE:     case AST::Kind::BLOCK:            case AST::Kind::FUNC_CALL:
			case AST::Kind::TEMPLATE_PACK:   case AST::Kind::TEMPLATED_EXPR:   case AST::Kind::PREFIX:
			case AST::Kind::INFIX:           case AST::Kind::POSTFIX:          case AST::Kind::MULTI_ASSIGN:
			case AST::Kind::NEW:             case AST::Kind::TYPE:             case AST::Kind::TYPEID_CONVERTER:
			case AST::Kind::ATTRIBUTE_BLOCK: case AST::Kind::ATTRIBUTE:        case AST::Kind::PRIMITIVE_TYPE:
			case AST::Kind::IDENT:           case AST::Kind::INTRINSIC:        case AST::Kind::LITERAL:
			case AST::Kind::UNINIT:          case AST::Kind::ZEROINIT:         case AST::Kind::THIS:
			case AST::Kind::DISCARD: {
				this->context.emitError(
					Diagnostic::Code::SYMBOL_PROC_INVALID_GLOBAL_STMT,
					Diagnostic::Location::get(stmt, this->source),
					"Invalid global statement"
				);
				return evo::resultError;
			};
		}

		evo::unreachable();
	}



	auto SymbolProcBuilder::build_var_decl(const AST::Node& stmt) -> evo::Result<> {
		const AST::VarDecl& var_decl = this->source.getASTBuffer().getVarDecl(stmt);

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info = this->analyze_attributes(
			this->source.getASTBuffer().getAttributeBlock(var_decl.attributeBlock)
		);
		if(attribute_params_info.isError()){ return evo::resultError; }


		auto type_id = std::optional<SymbolProc::TypeID>();
		if(var_decl.type.has_value()){
			const evo::Result<SymbolProc::TypeID> type_id_res = 
				this->analyze_type(this->source.getASTBuffer().getType(*var_decl.type));
			if(type_id_res.isError()){ return evo::resultError; }


			if(var_decl.kind != AST::VarDecl::Kind::Def){
				this->add_instruction(
					Instruction::VarDecl(
						var_decl, std::move(attribute_params_info.value()), type_id_res.value()
					)
				);
			}else{
				type_id = type_id_res.value();
			}
		}


		if(var_decl.value.has_value() == false){
			this->emit_error(
				Diagnostic::Code::SYMBOL_PROC_VAR_WITH_NO_VALUE, var_decl, "Variables need to be defined with a value"
			);
			return evo::resultError;
		}

		const evo::Result<SymbolProc::TermInfoID> value_id = this->analyze_expr<true>(*var_decl.value);
		if(value_id.isError()){ return evo::resultError; }

		if(var_decl.type.has_value() && var_decl.kind != AST::VarDecl::Kind::Def){
			this->add_instruction(
				Instruction::VarDef(var_decl, value_id.value())
			);

		}else{
			this->add_instruction(
				Instruction::VarDeclDef(
					var_decl, std::move(attribute_params_info.value()), type_id, value_id.value()
				)
			);
		}

		SymbolProcInfo& current_symbol = this->get_current_symbol();

		if(this->is_child_symbol()){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol.symbol_proc_id);
			current_symbol.symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol.symbol_proc_id);
		}

		this->symbol_namespaces.back()->emplace(current_symbol.symbol_proc.getIdent(), current_symbol.symbol_proc_id);

		return evo::Result<>();
	}


	auto SymbolProcBuilder::build_func_decl(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::FuncDecl& func_decl = ast_buffer.getFuncDecl(stmt);

		SymbolProcInfo* current_symbol = &this->get_current_symbol();


		auto template_param_infos = evo::SmallVector<Instruction::TemplateParamInfo>();
		if(func_decl.templatePack.has_value()){
			current_symbol->is_template = true;
			evo::Result<evo::SmallVector<Instruction::TemplateParamInfo>> template_param_infos_res =
				this->analyze_template_param_pack(ast_buffer.getTemplatePack(*func_decl.templatePack));

			if(template_param_infos_res.isError()){ return evo::resultError; }
			template_param_infos = std::move(template_param_infos_res.value());
		}



		if(template_param_infos.empty()){
			evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
				this->analyze_attributes(ast_buffer.getAttributeBlock(func_decl.attributeBlock));
			if(attribute_params_info.isError()){ return evo::resultError; }

			auto types = evo::SmallVector<std::optional<SymbolProcTypeID>>();
			types.reserve(func_decl.params.size() + func_decl.returns.size() + func_decl.errorReturns.size());

			auto default_param_values = evo::SmallVector<std::optional<SymbolProc::TermInfoID>>();
			default_param_values.reserve(func_decl.params.size());
			for(const AST::FuncDecl::Param& param : func_decl.params){
				if(param.type.has_value() == false){
					types.emplace_back();
					break;
				}
					
				const evo::Result<SymbolProc::TypeID> param_type = this->analyze_type(ast_buffer.getType(*param.type));
				if(param_type.isError()){ return evo::resultError; }
				types.emplace_back(param_type.value());

				if(param.defaultValue.has_value()){
					const evo::Result<SymbolProc::TermInfoID> param_default_value =
						this->analyze_expr<false>(*param.defaultValue);
					if(param_default_value.isError()){ return evo::resultError; }

					default_param_values.emplace_back(param_default_value.value());
				}else{
					default_param_values.emplace_back();
				}
			}

			for(const AST::FuncDecl::Return& return_param : func_decl.returns){
				const evo::Result<SymbolProc::TypeID> param_type = this->analyze_type(
					ast_buffer.getType(return_param.type)
				);
				if(param_type.isError()){ return evo::resultError; }
				types.emplace_back(param_type.value());
			}

			for(const AST::FuncDecl::Return& error_return_param : func_decl.errorReturns){
				const evo::Result<SymbolProc::TypeID> param_type = this->analyze_type(
					ast_buffer.getType(error_return_param.type)
				);
				if(param_type.isError()){ return evo::resultError; }
				types.emplace_back(param_type.value());
			}

			this->add_instruction(
				Instruction::FuncDecl<false>(
					func_decl,
					std::move(attribute_params_info.value()),
					std::move(default_param_values),
					std::move(types)
				)
			);

			for(const AST::Node& func_stmt : ast_buffer.getBlock(func_decl.block).stmts){
				if(this->analyze_stmt(func_stmt).isError()){ return evo::resultError; }
			}

			this->add_instruction(Instruction::FuncDef(func_decl));
			this->add_instruction(Instruction::FuncConstexprPIRReadyIfNeeded());

			// need to set again as address may have changed
			current_symbol = &this->get_current_symbol();

		}else{
			this->add_instruction(Instruction::TemplateFunc(func_decl, std::move(template_param_infos)));
		}


		if(this->is_child_symbol()){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol->symbol_proc_id);
			current_symbol->symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol->symbol_proc_id);
		}

		this->symbol_namespaces.back()->emplace(current_symbol->symbol_proc.getIdent(), current_symbol->symbol_proc_id);

		return evo::Result<>();
	}


	auto SymbolProcBuilder::build_alias_decl(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::AliasDecl& alias_decl = ast_buffer.getAliasDecl(stmt);


		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info = this->analyze_attributes(
			ast_buffer.getAttributeBlock(alias_decl.attributeBlock)
		);
		if(attribute_params_info.isError()){ return evo::resultError; }

		this->add_instruction(Instruction::AliasDecl(alias_decl, std::move(attribute_params_info.value())));
		
		const evo::Result<SymbolProc::TypeID> aliased_type = this->analyze_type(ast_buffer.getType(alias_decl.type));
		if(aliased_type.isError()){ return evo::resultError; }

		this->add_instruction(Instruction::AliasDef(alias_decl, aliased_type.value()));


		SymbolProcInfo& current_symbol = this->get_current_symbol();

		if(this->is_child_symbol()){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol.symbol_proc_id);
			current_symbol.symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol.symbol_proc_id);
		}

		this->symbol_namespaces.back()->emplace(current_symbol.symbol_proc.getIdent(), current_symbol.symbol_proc_id);

		return evo::Result<>();
	}


	auto SymbolProcBuilder::build_typedef_decl(const AST::Node& stmt) -> evo::Result<> {
		// const AST::TypedefDecl& typedef_decl = this->source.getASTBuffer().getTypedefDecl(stmt);
		this->emit_error(
			Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
			stmt,
			"Building symbol process of Typedef Decl is unimplemented"
		);
		return evo::resultError;
	}

	auto SymbolProcBuilder::build_struct_decl(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::StructDecl& struct_decl = ast_buffer.getStructDecl(stmt);

		SymbolProcInfo* current_symbol = &this->get_current_symbol();

		auto template_param_infos = evo::SmallVector<Instruction::TemplateParamInfo>();
		if(struct_decl.templatePack.has_value()){
			current_symbol->is_template = true;
			evo::Result<evo::SmallVector<Instruction::TemplateParamInfo>> template_param_infos_res =
				this->analyze_template_param_pack(ast_buffer.getTemplatePack(*struct_decl.templatePack));

			if(template_param_infos_res.isError()){ return evo::resultError; }
			template_param_infos = std::move(template_param_infos_res.value());
		}

		if(template_param_infos.empty()){
			evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
				this->analyze_attributes(ast_buffer.getAttributeBlock(struct_decl.attributeBlock));
			if(attribute_params_info.isError()){ return evo::resultError; }

			this->add_instruction(
				Instruction::StructDecl<false>(struct_decl, std::move(attribute_params_info.value()))
			);
			this->add_instruction(Instruction::StructDef());

			SymbolProc::StructInfo& struct_info =
				current_symbol->symbol_proc.extra_info.emplace<SymbolProc::StructInfo>();


			this->symbol_scopes.emplace_back(&struct_info.stmts);
			this->symbol_namespaces.emplace_back(&struct_info.member_symbols);
			for(const AST::Node& struct_stmt : ast_buffer.getBlock(struct_decl.block).stmts){
				if(this->build(struct_stmt).isError()){ return evo::resultError; }
			}
			this->symbol_namespaces.pop_back();
			this->symbol_scopes.pop_back();

			// need to set again as address may have changed
			current_symbol = &this->get_current_symbol();

		}else{
			this->add_instruction(Instruction::TemplateStruct(struct_decl, std::move(template_param_infos)));
		}


		if(this->is_child_symbol()){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol->symbol_proc_id);
			current_symbol->symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol->symbol_proc_id);
		}

		this->symbol_namespaces.back()->emplace(current_symbol->symbol_proc.getIdent(), current_symbol->symbol_proc_id);

		return evo::Result<>();
	}

	auto SymbolProcBuilder::build_when_conditional(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::WhenConditional& when_conditional = ast_buffer.getWhenConditional(stmt);

		const evo::Result<SymbolProc::TermInfoID> cond_id = this->analyze_expr<true>(when_conditional.cond);
		if(cond_id.isError()){ return evo::resultError; }

		auto then_symbol_scope = SymbolScope();
		this->symbol_scopes.emplace_back(&then_symbol_scope);
		for(const AST::Node& then_stmt : ast_buffer.getBlock(when_conditional.thenBlock).stmts){
			if(this->build(then_stmt).isError()){ return evo::resultError; }
		}
		this->symbol_scopes.pop_back();

		auto else_symbol_scope = SymbolScope();
		if(when_conditional.elseBlock.has_value()){
			this->symbol_scopes.emplace_back(&else_symbol_scope);
			if(when_conditional.elseBlock->kind() == AST::Kind::BLOCK){
				for(const AST::Node& else_stmt : ast_buffer.getBlock(*when_conditional.elseBlock).stmts){
					if(this->build(else_stmt).isError()){ return evo::resultError; }
				}
			}else{
				if(this->build(*when_conditional.elseBlock).isError()){ return evo::resultError; }
			}
			this->symbol_scopes.pop_back();
		}
		

		this->add_instruction(Instruction::WhenCond(when_conditional, cond_id.value()));


		SymbolProcInfo& current_symbol = this->get_current_symbol();

		if(this->is_child_symbol()){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol.symbol_proc_id);
			current_symbol.symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol.symbol_proc_id);
		}

		this->symbol_namespaces.back()->emplace("", this->get_current_symbol().symbol_proc_id);

		// TODO: address these directly instead of moving them in
		current_symbol.symbol_proc.extra_info.emplace<SymbolProc::WhenCondInfo>(
			std::move(then_symbol_scope), std::move(else_symbol_scope)
		);

		return evo::Result<>();
	}

	auto SymbolProcBuilder::build_func_call(const AST::Node& stmt) -> evo::Result<> {
		// const AST::FuncCall& func_call = this->source.getASTBuffer().getFuncCall(stmt);
		this->emit_error(
			Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
			stmt,
			"Building symbol process of Func Call is unimplemented"
		);
		return evo::resultError;
	}




	auto SymbolProcBuilder::analyze_type(const AST::Type& ast_type) -> evo::Result<SymbolProc::TypeID> {
		const SymbolProc::TypeID created_type_id = this->create_type();

		if(ast_type.base.kind() == AST::Kind::PRIMITIVE_TYPE){
			this->add_instruction(Instruction::PrimitiveType(ast_type, created_type_id));
			return created_type_id;
		}else{
			const evo::Result<SymbolProc::TermInfoID> type_base = this->analyze_type_base(ast_type.base);
			if(type_base.isError()){ return evo::resultError; }

			this->add_instruction(Instruction::UserType(ast_type, type_base.value(), created_type_id));
			return created_type_id;
		}
	}




	auto SymbolProcBuilder::analyze_type_base(const AST::Node& ast_type_base) -> evo::Result<SymbolProc::TermInfoID> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(ast_type_base.kind()){
			case AST::Kind::IDENT: { 
				return this->analyze_expr_ident<true>(ast_type_base);
			} break;

			case AST::Kind::TEMPLATED_EXPR: {
				const AST::TemplatedExpr& templated_expr = ast_buffer.getTemplatedExpr(ast_type_base);

				const evo::Result<SymbolProc::TermInfoID> base_type = this->analyze_type_base(templated_expr.base);
				if(base_type.isError()){ return evo::resultError; }

				auto args = evo::SmallVector<evo::Variant<SymbolProc::TermInfoID, SymbolProc::TypeID>>();
				args.reserve(templated_expr.args.size());
				for(const AST::Node& arg : templated_expr.args){
					if(arg.kind() == AST::Kind::TYPE){
						const evo::Result<SymbolProc::TypeID> arg_type = this->analyze_type(ast_buffer.getType(arg));
						if(arg_type.isError()){ return evo::resultError; }

						args.emplace_back(arg_type.value());

					}else{
						const evo::Result<SymbolProc::TermInfoID> arg_expr_info = this->analyze_expr<true>(arg);
						if(arg_expr_info.isError()){ return evo::resultError; }

						args.emplace_back(arg_expr_info.value());
					}
				}

				const SymbolProc::StructInstantiationID created_struct_inst_id = this->create_struct_instantiation();
				const SymbolProc::TermInfoID created_base_term_info_id = this->create_term_info();

				this->add_instruction(
					Instruction::TemplatedTerm(
						templated_expr, base_type.value(), std::move(args), created_struct_inst_id
					)
				);

				this->add_instruction(
					Instruction::TemplatedTermWait(created_struct_inst_id, created_base_term_info_id)
				);

				return created_base_term_info_id;
			} break;

			case AST::Kind::INFIX: { 
				const AST::Infix& base_type_infix = ast_buffer.getInfix(ast_type_base);

				const evo::Result<SymbolProc::TermInfoID> base_lhs = this->analyze_type_base(base_type_infix.lhs);
				if(base_lhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID created_base_type_type = this->create_term_info();
				this->add_instruction(
					Instruction::Accessor<true>(
						base_type_infix,
						base_lhs.value(),
						ast_buffer.getIdent(base_type_infix.rhs),
						created_base_type_type
					)
				);
				return created_base_type_type;
			} break;

			case AST::Kind::TYPEID_CONVERTER: { 
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					ast_type_base,
					"Type ID converters are unimplemented"
				);
				return evo::resultError;
			} break;

			// TODO: separate out into more kinds to be more specific (errors vs fatal)
			default: {
				this->emit_error(
					Diagnostic::Code::SYMBOL_PROC_INVALID_BASE_TYPE, ast_type_base, "Invalid base type"
				);
				return evo::resultError;
			} break;
		}
	}



	// TODO: error on invalid statements
	auto SymbolProcBuilder::analyze_stmt(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(stmt.kind()){
			case AST::Kind::NONE:             evo::debugFatalBreak("Not a valid AST node");
			case AST::Kind::VAR_DECL:         evo::unimplemented("AST::Kind::VAR_DECL");
			case AST::Kind::FUNC_DECL:        evo::unimplemented("AST::Kind::FUNC_DECL");
			case AST::Kind::ALIAS_DECL:       evo::unimplemented("AST::Kind::ALIAS_DECL");
			case AST::Kind::TYPEDEF_DECL:     evo::unimplemented("AST::Kind::TYPEDEF_DECL");
			case AST::Kind::STRUCT_DECL:      evo::unimplemented("AST::Kind::STRUCT_DECL");
			case AST::Kind::RETURN:           return this->analyze_return(ast_buffer.getReturn(stmt));
			case AST::Kind::ERROR:            return this->analyze_error(ast_buffer.getError(stmt));
			case AST::Kind::CONDITIONAL:      evo::unimplemented("AST::Kind::CONDITIONAL");
			case AST::Kind::WHEN_CONDITIONAL: evo::unimplemented("AST::Kind::WHEN_CONDITIONAL");
			case AST::Kind::WHILE:            evo::unimplemented("AST::Kind::WHILE");
			case AST::Kind::UNREACHABLE:      evo::unimplemented("AST::Kind::UNREACHABLE");
			case AST::Kind::BLOCK:            evo::unimplemented("AST::Kind::BLOCK");
			case AST::Kind::FUNC_CALL:        return this->analyze_func_call(ast_buffer.getFuncCall(stmt));
			case AST::Kind::TEMPLATE_PACK:    evo::unimplemented("AST::Kind::TEMPLATE_PACK");
			case AST::Kind::TEMPLATED_EXPR:   evo::unimplemented("AST::Kind::TEMPLATED_EXPR");
			case AST::Kind::PREFIX:           evo::unimplemented("AST::Kind::PREFIX");
			case AST::Kind::INFIX:            evo::unimplemented("AST::Kind::INFIX");
			case AST::Kind::POSTFIX:          evo::unimplemented("AST::Kind::POSTFIX");
			case AST::Kind::MULTI_ASSIGN:     evo::unimplemented("AST::Kind::MULTI_ASSIGN");
			case AST::Kind::NEW:              evo::unimplemented("AST::Kind::NEW");
			case AST::Kind::TYPE:             evo::unimplemented("AST::Kind::TYPE");
			case AST::Kind::TYPEID_CONVERTER: evo::unimplemented("AST::Kind::TYPEID_CONVERTER");
			case AST::Kind::ATTRIBUTE_BLOCK:  evo::unimplemented("AST::Kind::ATTRIBUTE_BLOCK");
			case AST::Kind::ATTRIBUTE:        evo::unimplemented("AST::Kind::ATTRIBUTE");
			case AST::Kind::PRIMITIVE_TYPE:   evo::unimplemented("AST::Kind::PRIMITIVE_TYPE");
			case AST::Kind::IDENT:            evo::unimplemented("AST::Kind::IDENT");
			case AST::Kind::INTRINSIC:        evo::unimplemented("AST::Kind::INTRINSIC");
			case AST::Kind::LITERAL:          evo::unimplemented("AST::Kind::LITERAL");
			case AST::Kind::UNINIT:           evo::unimplemented("AST::Kind::UNINIT");
			case AST::Kind::ZEROINIT:         evo::unimplemented("AST::Kind::ZEROINIT");
			case AST::Kind::THIS:             evo::unimplemented("AST::Kind::THIS");
			case AST::Kind::DISCARD:          evo::unimplemented("AST::Kind::DISCARD");
		}

		evo::unreachable();
	}



	auto SymbolProcBuilder::analyze_return(const AST::Return& return_stmt) -> evo::Result<> {
		if(return_stmt.value.is<AST::Node>()){
			const evo::Result<SymbolProc::TermInfoID> return_value = 
				this->analyze_expr<false>(return_stmt.value.as<AST::Node>());
			if(return_value.isError()){ return evo::resultError; }

			this->add_instruction(Instruction::Return(return_stmt, return_value.value()));
			return evo::Result<>();
			
		}else{
			this->add_instruction(Instruction::Return(return_stmt, std::nullopt));
			return evo::Result<>();
		}
	}

	auto SymbolProcBuilder::analyze_error(const AST::Error& error_stmt) -> evo::Result<> {
		if(error_stmt.value.is<AST::Node>()){
			const evo::Result<SymbolProc::TermInfoID> error_value = 
				this->analyze_expr<false>(error_stmt.value.as<AST::Node>());
			if(error_value.isError()){ return evo::resultError; }

			this->add_instruction(Instruction::Error(error_stmt, error_value.value()));
			return evo::Result<>();
			
		}else{
			this->add_instruction(Instruction::Error(error_stmt, std::nullopt));
			return evo::Result<>();
		}
	}

	// TODO: deduplicate with `analyze_expr_func_call`?
	auto SymbolProcBuilder::analyze_func_call(const AST::FuncCall& func_call) -> evo::Result<> {
		bool is_target_template = false;
		const evo::Result<SymbolProc::TermInfoID> target = [&](){
			if(func_call.target.kind() == AST::Kind::TEMPLATED_EXPR){
				is_target_template = true;
				const AST::TemplatedExpr& target_templated_expr = 
					this->source.getASTBuffer().getTemplatedExpr(func_call.target);
				return this->analyze_expr<false>(target_templated_expr.base);

			}else{
				return this->analyze_expr<false>(func_call.target);
			}
		}();
		if(target.isError()){ return evo::resultError; }

		auto args = evo::SmallVector<SymbolProc::TermInfoID>();
		args.reserve(func_call.args.size());
		for(const AST::FuncCall::Arg& arg : func_call.args){
			const evo::Result<SymbolProc::TermInfoID> arg_value = this->analyze_expr<false>(arg.value);
			if(arg_value.isError()){ return evo::resultError; }
			args.emplace_back(arg_value.value());
		}

		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();

		if(is_target_template){
			this->emit_error(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				func_call.target,
				"Templated function calls are currently unimplemented"
			);
			return evo::resultError;
		}

		this->add_instruction(Instruction::FuncCall(func_call, target.value(), std::move(args)));
		return evo::Result<>();
	}




	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_term(const AST::Node& expr) -> evo::Result<SymbolProc::TermInfoID> {
		return this->analyze_term_impl<IS_CONSTEXPR, false>(expr);
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr(const AST::Node& expr) -> evo::Result<SymbolProc::TermInfoID> {
		return this->analyze_term_impl<IS_CONSTEXPR, true>(expr);
	}


	template<bool IS_CONSTEXPR, bool MUST_BE_EXPR>
	auto SymbolProcBuilder::analyze_term_impl(const AST::Node& expr) -> evo::Result<SymbolProc::TermInfoID> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(expr.kind()){
			case AST::Kind::NONE: {
				evo::debugFatalBreak("Invalid AST::Node");
			} break;

			case AST::Kind::BLOCK:          return this->analyze_expr_block<IS_CONSTEXPR>(expr);
			case AST::Kind::FUNC_CALL:      return this->analyze_expr_func_call<IS_CONSTEXPR>(expr);
			case AST::Kind::TEMPLATED_EXPR: return this->analyze_expr_templated<IS_CONSTEXPR>(expr);
			case AST::Kind::PREFIX:         return this->analyze_expr_prefix<IS_CONSTEXPR>(expr);
			case AST::Kind::INFIX:          return this->analyze_expr_infix<IS_CONSTEXPR>(expr);
			case AST::Kind::POSTFIX:        return this->analyze_expr_postfix<IS_CONSTEXPR>(expr);
			case AST::Kind::NEW:            return this->analyze_expr_new<IS_CONSTEXPR>(expr);
			case AST::Kind::IDENT:          return this->analyze_expr_ident<IS_CONSTEXPR>(expr);
			case AST::Kind::INTRINSIC:      return this->analyze_expr_intrinsic(expr);
			case AST::Kind::LITERAL:        return this->analyze_expr_literal(ast_buffer.getLiteral(expr));
			case AST::Kind::UNINIT:         return this->analyze_expr_uninit(ast_buffer.getUninit(expr));
			case AST::Kind::ZEROINIT:       return this->analyze_expr_zeroinit(ast_buffer.getZeroinit(expr));
			case AST::Kind::THIS:           return this->analyze_expr_this(expr);

			case AST::Kind::TYPE: {
				if constexpr(MUST_BE_EXPR){
					this->emit_error(Diagnostic::Code::SYMBOL_PROC_TYPE_USED_AS_EXPR, expr, "Type used as expression");
					return evo::resultError;
				}else{
					const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();

					const evo::Result<SymbolProc::TypeID> type_id = this->analyze_type(ast_buffer.getType(expr));
					if(type_id.isError()){ return evo::resultError; }

					this->add_instruction(Instruction::TypeToTerm(type_id.value(), new_term_info_id));
					return new_term_info_id;
				}
			} break;

			case AST::Kind::TYPEID_CONVERTER: {
				if constexpr(MUST_BE_EXPR){
					this->emit_error(
						Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
						expr,
						"Type ID converter is currently unimplemented"
					);
					return evo::resultError;
				}else{
					const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();

					const evo::Result<SymbolProc::TypeID> type_id = this->analyze_type(ast_buffer.getType(expr));
					if(type_id.isError()){ return evo::resultError; }

					this->add_instruction(Instruction::TypeToTerm(type_id.value(), new_term_info_id));
					return new_term_info_id;
				}
			} break;

			case AST::Kind::VAR_DECL:       case AST::Kind::FUNC_DECL:       case AST::Kind::ALIAS_DECL:
			case AST::Kind::TYPEDEF_DECL:   case AST::Kind::STRUCT_DECL:     case AST::Kind::RETURN:
			case AST::Kind::ERROR:          case AST::Kind::CONDITIONAL:     case AST::Kind::WHEN_CONDITIONAL:
			case AST::Kind::WHILE:          case AST::Kind::UNREACHABLE:     case AST::Kind::TEMPLATE_PACK:
			case AST::Kind::MULTI_ASSIGN:   case AST::Kind::ATTRIBUTE_BLOCK: case AST::Kind::ATTRIBUTE:
			case AST::Kind::PRIMITIVE_TYPE: case AST::Kind::DISCARD: {
				// TODO: better messaging (specify what kind)
				this->emit_fatal(
					Diagnostic::Code::SYMBOL_PROC_INVALID_EXPR_KIND,
					Diagnostic::Location::NONE,
					Diagnostic::createFatalMessage("Encountered expr of invalid AST kind")
				);
				return evo::resultError; 
			} break;
		}

		evo::unreachable();
	}



	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_block(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		this->emit_error(
			Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE, node, "Building symbol proc of block is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_func_call(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const AST::FuncCall& func_call = this->source.getASTBuffer().getFuncCall(node);

		if(func_call.target.kind() == AST::Kind::INTRINSIC){
			const Token::ID intrin_tok_id = this->source.getASTBuffer().getIntrinsic(func_call.target);
			if(this->source.getTokenBuffer()[intrin_tok_id].getString() == "import"){
				if(func_call.args.empty()){
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_IMPORT_REQUIRES_ONE_ARG,
						intrin_tok_id,
						"Calls to @import requires a path"
					);
					return evo::resultError;
				}

				if(func_call.args.size() > 1){
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_IMPORT_REQUIRES_ONE_ARG,
						func_call.args[1].value,
						"Calls to @import requires a path, and no other arguments"
					);
					return evo::resultError;
				}

				const evo::Result<SymbolProc::TermInfoID> path_value = this->analyze_expr<true>(
					func_call.args[0].value
				);
				if(path_value.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				this->add_instruction(Instruction::Import(func_call, path_value.value(), new_term_info_id));
				return new_term_info_id;
			}
		}


		bool is_target_template = false;
		const evo::Result<SymbolProc::TermInfoID> target = [&](){
			if(func_call.target.kind() == AST::Kind::TEMPLATED_EXPR){
				is_target_template = true;
				const AST::TemplatedExpr& target_templated_expr = 
					this->source.getASTBuffer().getTemplatedExpr(func_call.target);
				return this->analyze_expr<IS_CONSTEXPR>(target_templated_expr.base);

			}else{
				return this->analyze_expr<IS_CONSTEXPR>(func_call.target);
			}
		}();
		if(target.isError()){ return evo::resultError; }

		auto args = evo::SmallVector<SymbolProc::TermInfoID>();
		args.reserve(func_call.args.size());
		for(const AST::FuncCall::Arg& arg : func_call.args){
			const evo::Result<SymbolProc::TermInfoID> arg_value = this->analyze_expr<IS_CONSTEXPR>(arg.value);
			if(arg_value.isError()){ return evo::resultError; }
			args.emplace_back(arg_value.value());
		}

		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();

		if(is_target_template){
			this->emit_error(
				Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
				func_call.target,
				"Templated function calls are currently unimplemented"
			);
			return evo::resultError;
		}


		if constexpr(IS_CONSTEXPR){
			this->add_instruction(
				Instruction::FuncCallExpr<true>(func_call, target.value(), new_term_info_id, std::move(args))
			);

			const SymbolProc::TermInfoID comptime_res_term_info_id = this->create_term_info();
			this->add_instruction(
				Instruction::ConstexprFuncCallRun(func_call, new_term_info_id, comptime_res_term_info_id)
			);
			return comptime_res_term_info_id;

		}else{
			this->add_instruction(
				Instruction::FuncCallExpr<false>(func_call, target.value(), new_term_info_id, std::move(args))
			);

			return new_term_info_id;
		}

	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_templated(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::TemplatedExpr& templated_expr = ast_buffer.getTemplatedExpr(node);

		const evo::Result<SymbolProc::TermInfoID> base_type = this->analyze_expr<IS_CONSTEXPR>(templated_expr.base);
		if(base_type.isError()){ return evo::resultError; }

		auto args = evo::SmallVector<evo::Variant<SymbolProc::TermInfoID, SymbolProc::TypeID>>();
		args.reserve(templated_expr.args.size());
		for(const AST::Node& arg : templated_expr.args){
			if(arg.kind() == AST::Kind::TYPE){
				const evo::Result<SymbolProc::TypeID> arg_type = this->analyze_type(ast_buffer.getType(arg));
				if(arg_type.isError()){ return evo::resultError; }

				args.emplace_back(arg_type.value());

			}else{
				const evo::Result<SymbolProc::TermInfoID> arg_expr_info = this->analyze_expr<true>(arg);
				if(arg_expr_info.isError()){ return evo::resultError; }

				args.emplace_back(arg_expr_info.value());
			}
		}

		const SymbolProc::StructInstantiationID created_struct_inst_id = this->create_struct_instantiation();
		const SymbolProc::TermInfoID created_base_term_info_id = this->create_term_info();

		this->add_instruction(
			Instruction::TemplatedTerm(
				templated_expr, base_type.value(), std::move(args), created_struct_inst_id
			)
		);

		this->add_instruction(
			Instruction::TemplatedTermWait(created_struct_inst_id, created_base_term_info_id)
		);

		return created_base_term_info_id;
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_prefix(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		this->emit_error(
			Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE, node, "Building symbol proc of prefix is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_infix(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const AST::Infix& infix = this->source.getASTBuffer().getInfix(node);

		if(this->source.getTokenBuffer()[infix.opTokenID].kind() == Token::lookupKind(".")){
			const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
			if(lhs.isError()){ return evo::resultError; }

			const Token::ID rhs = this->source.getASTBuffer().getIdent(infix.rhs);

			const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
			this->add_instruction(Instruction::Accessor<IS_CONSTEXPR>(infix, lhs.value(), rhs, new_term_info_id));
			return new_term_info_id;
		}

		this->emit_error(
			Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE, node, "Building symbol proc of infix (not [.]) is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_postfix(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		this->emit_error(
			Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE, node, "Building symbol proc of postfix is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_new(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		this->emit_error(
			Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE, node, "Building symbol proc of new is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_ident(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(
			Instruction::Ident<IS_CONSTEXPR>(this->source.getASTBuffer().getIdent(node), new_term_info_id)
		);
		return new_term_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_intrinsic(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(
			Instruction::Intrinsic(this->source.getASTBuffer().getIntrinsic(node), new_term_info_id)
		);
		return new_term_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_literal(const Token::ID& literal) -> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(Instruction::Literal(literal, new_term_info_id));
		return new_term_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_uninit(const Token::ID& uninit_token) -> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(Instruction::Uninit(uninit_token, new_term_info_id));
		return new_term_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_zeroinit(const Token::ID& zeroinit_token)
	-> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(Instruction::Zeroinit(zeroinit_token, new_term_info_id));
		return new_term_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_this(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		this->emit_error(
			Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE, node, "Building symbol proc of this is unimplemented"
		);
		return evo::resultError;
	}



	auto SymbolProcBuilder::analyze_attributes(const AST::AttributeBlock& attribute_block)
	-> evo::Result<evo::SmallVector<Instruction::AttributeParams>> {
		auto attribute_params_info = evo::SmallVector<Instruction::AttributeParams>();

		for(const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			attribute_params_info.emplace_back();

			for(const AST::Node& arg : attribute.args){
				const evo::Result<SymbolProc::TermInfoID> arg_expr = this->analyze_expr<true>(arg);
				if(arg_expr.isError()){ return evo::resultError; }

				attribute_params_info.back().emplace_back(arg_expr.value());
			}
		}

		return attribute_params_info;
	}



	auto SymbolProcBuilder::analyze_template_param_pack(const AST::TemplatePack& template_pack)
	-> evo::Result<evo::SmallVector<SymbolProc::Instruction::TemplateParamInfo>> {
		const TokenBuffer& token_buffer = this->source.getTokenBuffer();
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		auto template_param_infos = evo::SmallVector<SymbolProc::Instruction::TemplateParamInfo>();

		for(const AST::TemplatePack::Param& param : template_pack.params){
			const AST::Type& param_ast_type = ast_buffer.getType(param.type);
			auto param_type = std::optional<SymbolProc::TypeID>();
			if(
				param_ast_type.base.kind() != AST::Kind::PRIMITIVE_TYPE 
				|| token_buffer[ast_buffer.getPrimitiveType(param_ast_type.base)].kind() != Token::Kind::TYPE_TYPE
			){
				const evo::Result<SymbolProc::TypeID> param_type_res = this->analyze_type(param_ast_type);
				if(param_type_res.isError()){ return evo::resultError; }
				param_type = param_type_res.value();
			}

			auto default_value = std::optional<SymbolProc::TermInfoID>();
			if(param.defaultValue.has_value()){
				const evo::Result<SymbolProc::TermInfoID> default_value_res =
					this->analyze_term<true>(*param.defaultValue);
				if(default_value_res.isError()){ return evo::resultError; }
				default_value = default_value_res.value();
			}

			template_param_infos.emplace_back(param, param_type, default_value);
		}

		return template_param_infos;
	}


}