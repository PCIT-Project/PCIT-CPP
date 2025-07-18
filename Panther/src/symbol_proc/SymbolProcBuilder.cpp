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
	

	auto SymbolProcBuilder::build(const AST::Node& stmt) -> evo::Result<SymbolProc::ID> {
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

			break; case AST::Kind::INTERFACE_DECL:
				if(this->build_interface_decl(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::INTERFACE_IMPL:
				if(this->build_interface_impl(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::WHEN_CONDITIONAL:
				if(this->build_when_conditional(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::FUNC_CALL:
				if(this->build_func_call(stmt).isError()){ return evo::resultError; }

			break; default: evo::debugFatalBreak("Unknown global statement");
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

		return symbol_proc_id;
	}


	// for structs
	auto SymbolProcBuilder::buildTemplateInstance(
		const SymbolProc& template_symbol_proc,
		BaseType::StructTemplate::Instantiation& instantiation,
		sema::ScopeManager::Scope::ID sema_scope_id,
		BaseType::StructTemplate::ID struct_template_id,
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
			Instruction::StructDecl<true>(
				struct_decl, std::move(attribute_params_info.value()), struct_template_id, instantiation_id
			)
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



	// for functions
	auto SymbolProcBuilder::buildTemplateInstance(
		const SymbolProc& template_symbol_proc,
		sema::TemplatedFunc::Instantiation& instantiation,
		sema::ScopeManager::Scope::ID sema_scope_id,
		uint32_t instantiation_id,
		evo::SmallVector<std::optional<TypeInfo::ID>>&& arg_types
	) -> evo::Result<SymbolProc::ID> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::FuncDecl& func_decl = ast_buffer.getFuncDecl(template_symbol_proc.ast_node);

		SymbolProc::ID symbol_proc_id = this->context.symbol_proc_manager.create_symbol_proc(
			template_symbol_proc.ast_node,
			template_symbol_proc.source_id,
			template_symbol_proc.ident,
			template_symbol_proc.parent
		);
		SymbolProc& symbol_proc = this->context.symbol_proc_manager.getSymbolProc(symbol_proc_id);

		symbol_proc.extra_info.emplace<SymbolProc::FuncInfo>();
		symbol_proc.extra_info.as<SymbolProc::FuncInfo>().instantiation = &instantiation;
		symbol_proc.extra_info.as<SymbolProc::FuncInfo>().instantiation_param_arg_types = std::move(arg_types);

		symbol_proc.sema_scope_id = sema_scope_id;

		this->symbol_proc_infos.emplace_back(symbol_proc_id, symbol_proc);


		///////////////////////////////////
		// build func decl

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
			this->analyze_attributes(ast_buffer.getAttributeBlock(func_decl.attributeBlock));
		if(attribute_params_info.isError()){ return evo::resultError; }


		auto types = evo::SmallVector<std::optional<SymbolProcTypeID>>();
		types.reserve(func_decl.params.size() + func_decl.returns.size() + func_decl.errorReturns.size());

		auto default_param_values = evo::SmallVector<std::optional<SymbolProc::TermInfoID>>();
		default_param_values.reserve(func_decl.params.size());
		for(size_t i = 0; const AST::FuncDecl::Param& param : func_decl.params){
			EVO_DEFER([&](){ i += 1; });

			if(param.type.has_value() == false){
				types.emplace_back();
				default_param_values.emplace_back();
				continue;
			}
				
			const evo::Result<SymbolProc::TypeID> param_type =
				this->analyze_type<false>(ast_buffer.getType(*param.type));
			if(param_type.isError()){ return evo::resultError; }
			types.emplace_back(param_type.value());

			if(symbol_proc.extra_info.as<SymbolProc::FuncInfo>().instantiation_param_arg_types[i].has_value()){
				this->add_instruction(Instruction::FuncDeclExtractDeducersIfNeeded(param_type.value(), i));
			}

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
			const evo::Result<SymbolProc::TypeID> param_type = this->analyze_type<false>(
				ast_buffer.getType(return_param.type)
			);
			if(param_type.isError()){ return evo::resultError; }
			types.emplace_back(param_type.value());
		}

		for(const AST::FuncDecl::Return& error_return_param : func_decl.errorReturns){
			const evo::Result<SymbolProc::TypeID> param_type = this->analyze_type<false>(
				ast_buffer.getType(error_return_param.type)
			);
			if(param_type.isError()){ return evo::resultError; }
			types.emplace_back(param_type.value());
		}

		this->add_instruction(
			Instruction::FuncDecl<true>(
				func_decl,
				std::move(attribute_params_info.value()),
				std::move(default_param_values),
				std::move(types),
				instantiation_id
			)
		);

		this->add_instruction(Instruction::SuspendSymbolProc{});


		// make sure definitions are ready for body of function
		// TODO(PERF): better way of doing this
		for(const AST::FuncDecl::Param& param : func_decl.params){
			if(param.type.has_value()){
				const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
					ast_buffer.getType(*param.type)
				);
				evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
			}else{
				this->add_instruction(Instruction::RequireThisDef{});
			}
		}
		for(const AST::FuncDecl::Return& return_param : func_decl.returns){
			const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
				ast_buffer.getType(return_param.type)
			);
			evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
		}
		for(const AST::FuncDecl::Return& error_return_param : func_decl.errorReturns){
			const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
				ast_buffer.getType(error_return_param.type)
			);
			evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
		}


		this->add_instruction(Instruction::FuncPreBody(func_decl));

		this->symbol_scopes.emplace_back(nullptr);
		this->symbol_namespaces.emplace_back(nullptr);
		for(const AST::Node& func_stmt : ast_buffer.getBlock(*func_decl.block).stmts){
			if(this->analyze_stmt(func_stmt).isError()){ return evo::resultError; }
		}
		this->symbol_namespaces.pop_back();
		this->symbol_scopes.pop_back();

		this->add_instruction(Instruction::FuncDef(func_decl));
		this->add_instruction(Instruction::FuncPrepareConstexprPIRIfNeeded(func_decl));
		this->add_instruction(Instruction::FuncConstexprPIRReadyIfNeeded());


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
				const Token& name_token = token_buffer[func_decl.name];

				if(name_token.kind() == Token::Kind::IDENT){
					return name_token.getString();
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

			case AST::Kind::INTERFACE_DECL: {
				return token_buffer[ast_buffer.getInterfaceDecl(stmt).ident].getString();
			} break;

			case AST::Kind::INTERFACE_IMPL: {
				return std::string_view();
			} break;

			case AST::Kind::WHEN_CONDITIONAL: {
				return std::string_view();
			} break;

			case AST::Kind::RETURN:          case AST::Kind::ERROR:        case AST::Kind::BREAK:
			case AST::Kind::CONTINUE:        case AST::Kind::CONDITIONAL:  case AST::Kind::WHILE:
			case AST::Kind::DEFER:           case AST::Kind::UNREACHABLE:  case AST::Kind::BLOCK:
			case AST::Kind::FUNC_CALL:       case AST::Kind::INDEXER:      case AST::Kind::TEMPLATE_PACK:
			case AST::Kind::TEMPLATED_EXPR:  case AST::Kind::PREFIX:       case AST::Kind::INFIX:
			case AST::Kind::POSTFIX:         case AST::Kind::MULTI_ASSIGN: case AST::Kind::NEW:
			case AST::Kind::STRUCT_INIT_NEW: case AST::Kind::TRY_ELSE:     case AST::Kind::TYPE_DEDUCER:
			case AST::Kind::ARRAY_TYPE:      case AST::Kind::TYPE:         case AST::Kind::TYPEID_CONVERTER:
			case AST::Kind::ATTRIBUTE_BLOCK: case AST::Kind::ATTRIBUTE:    case AST::Kind::PRIMITIVE_TYPE:
			case AST::Kind::IDENT:           case AST::Kind::INTRINSIC:    case AST::Kind::LITERAL:
			case AST::Kind::UNINIT:          case AST::Kind::ZEROINIT:     case AST::Kind::THIS:
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


		auto decl_def_type_id = std::optional<SymbolProc::TypeID>();
		if(var_decl.type.has_value()){
			const evo::Result<SymbolProc::TypeID> type_id_res = 
				this->analyze_type<true>(this->source.getASTBuffer().getType(*var_decl.type));
			if(type_id_res.isError()){ return evo::resultError; }


			if(this->source.getASTBuffer().getType(*var_decl.type).base.kind() == AST::Kind::TYPE_DEDUCER){
				decl_def_type_id = type_id_res.value();

			}else if(var_decl.kind != AST::VarDecl::Kind::DEF){
				this->add_instruction(
					Instruction::NonLocalVarDecl(
						var_decl, std::move(attribute_params_info.value()), type_id_res.value()
					)
				);
			}else{
				decl_def_type_id = type_id_res.value();
			}
		}



		auto value_id = std::optional<SymbolProc::TermInfoID>();
		if(var_decl.value.has_value()){
			const evo::Result<SymbolProc::TermInfoID> value_id_res = this->analyze_expr<true>(*var_decl.value);
			if(value_id_res.isError()){ return evo::resultError; }

			value_id = value_id_res.value();
			
		}else{
			if(var_decl.kind == AST::VarDecl::Kind::DEF){
				this->emit_error(
					Diagnostic::Code::SYMBOL_PROC_VAR_WITH_NO_VALUE,
					var_decl,
					"All [def] variables need to be defined with a value"
				);
				return evo::resultError;
			}

			if(var_decl.type.has_value() == false){
				this->emit_error(
					Diagnostic::Code::SYMBOL_PROC_VAR_WITH_NO_VALUE,
					var_decl,
					"Variables must be defined with a type and/or a value"
				);
				return evo::resultError;
			}
		}

	

		if(
			var_decl.type.has_value()
			&& var_decl.kind != AST::VarDecl::Kind::DEF
			&& decl_def_type_id.has_value() == false
		){
			this->add_instruction(Instruction::NonLocalVarDef(var_decl, value_id));

		}else{
			this->add_instruction(
				Instruction::NonLocalVarDeclDef(
					var_decl, std::move(attribute_params_info.value()), decl_def_type_id, *value_id
				)
			);
		}

		SymbolProcInfo& current_symbol = this->get_current_symbol();

		if(this->is_child_symbol() && this->symbol_scopes.back() != nullptr){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol.symbol_proc_id);
			current_symbol.symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol.symbol_proc_id);
		}

		if(this->symbol_namespaces.back() != nullptr){
			this->symbol_namespaces.back()->emplace(
				current_symbol.symbol_proc.getIdent(), current_symbol.symbol_proc_id
			);
		}

		return evo::Result<>();
	}


	auto SymbolProcBuilder::build_func_decl(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::FuncDecl& func_decl = ast_buffer.getFuncDecl(stmt);

		SymbolProcInfo* current_symbol = &this->get_current_symbol();

		current_symbol->symbol_proc.extra_info.emplace<SymbolProc::FuncInfo>();


		auto template_param_infos = evo::SmallVector<Instruction::TemplateParamInfo>();
		if(func_decl.templatePack.has_value()){
			current_symbol->is_template = true;
			evo::Result<evo::SmallVector<Instruction::TemplateParamInfo>> template_param_infos_res =
				this->analyze_template_param_pack(ast_buffer.getTemplatePack(*func_decl.templatePack));

			if(template_param_infos_res.isError()){ return evo::resultError; }
			template_param_infos = std::move(template_param_infos_res.value());
		}



		const bool has_type_deducer_param = [&](){
			for(const AST::FuncDecl::Param& param : func_decl.params){
				if(param.type.has_value() && this->is_type_deducer(ast_buffer.getType(*param.type))){ 
					return true;
				}
			}

			return false;
		}();



		if(template_param_infos.empty() && has_type_deducer_param == false){
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
					default_param_values.emplace_back();
					continue;
				}
					
				const evo::Result<SymbolProc::TypeID> param_type =
					this->analyze_type<false>(ast_buffer.getType(*param.type));
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
				const evo::Result<SymbolProc::TypeID> param_type = this->analyze_type<false>(
					ast_buffer.getType(return_param.type)
				);
				if(param_type.isError()){ return evo::resultError; }
				types.emplace_back(param_type.value());
			}

			for(const AST::FuncDecl::Return& error_return_param : func_decl.errorReturns){
				const evo::Result<SymbolProc::TypeID> param_type = this->analyze_type<false>(
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


			// make sure definitions are ready for body of function
			// TODO(PERF): better way of doing this
			for(const AST::FuncDecl::Param& param : func_decl.params){
				if(param.type.has_value()){
					const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
						ast_buffer.getType(*param.type)
					);
					evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
				}else{
					this->add_instruction(Instruction::RequireThisDef{});
				}
			}
			for(const AST::FuncDecl::Return& return_param : func_decl.returns){
				const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
					ast_buffer.getType(return_param.type)
				);
				evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
			}
			for(const AST::FuncDecl::Return& error_return_param : func_decl.errorReturns){
				const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
					ast_buffer.getType(error_return_param.type)
				);
				evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
			}


			if(func_decl.block.has_value()){
				this->add_instruction(Instruction::FuncPreBody(func_decl));

				this->symbol_scopes.emplace_back(nullptr);
				this->symbol_namespaces.emplace_back(nullptr);
				for(const AST::Node& func_stmt : ast_buffer.getBlock(*func_decl.block).stmts){
					if(this->analyze_stmt(func_stmt).isError()){ return evo::resultError; }
				}
				this->symbol_namespaces.pop_back();
				this->symbol_scopes.pop_back();

				this->add_instruction(Instruction::FuncDef(func_decl));
				this->add_instruction(Instruction::FuncPrepareConstexprPIRIfNeeded(func_decl));
				this->add_instruction(Instruction::FuncConstexprPIRReadyIfNeeded());

			}else{
				this->add_instruction(Instruction::InterfaceFuncDef(func_decl));
			}


			// need to set again as address may have changed
			current_symbol = &this->get_current_symbol();

		}else{
			if(func_decl.block.has_value() == false){
				this->emit_error(
					Diagnostic::Code::SYMBOL_PROC_TEMPLATE_INTERFACE_METHOD,
					func_decl,
					"Interface methods cannot be templates"
				);
				return evo::resultError;
			}


			auto template_names = std::unordered_set<std::string_view>();

			for(
				const AST::TemplatePack::Param& template_param
				: ast_buffer.getTemplatePack(*func_decl.templatePack).params
			){
				template_names.emplace(this->source.getTokenBuffer()[template_param.ident].getString());
			}


			this->add_instruction(Instruction::TemplateFuncBegin(func_decl, std::move(template_param_infos)));

			for(size_t i = 0; const AST::FuncDecl::Param& param : func_decl.params){
				EVO_DEFER([&](){ i += 1; });

				if(param.defaultValue.has_value()){
					this->emit_error(
						Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
						*param.defaultValue,
						"Template functions with default parameter values is unimplemented"
					);
					return evo::resultError;
				}
				
				if(param.type.has_value() == false){ continue; } // skip `this`
 
				const AST::Type& param_type = this->source.getASTBuffer().getType(*param.type);

				switch(param_type.base.kind()){
					case AST::Kind::IDENT: {
						if(param_type.qualifiers.empty() == false){ continue; }

						const std::string_view ident_name = this->source.getTokenBuffer()[
							this->source.getASTBuffer().getIdent(param_type.base)
						].getString();

						if(template_names.contains(ident_name) == false){
							const evo::Result<SymbolProc::TypeID> symbol_proc_type_id =
								this->analyze_type<false>(param_type);
							if(symbol_proc_type_id.isError()){ return evo::resultError; }

							this->add_instruction(
								Instruction::TemplateFuncCheckParamIsInterface(symbol_proc_type_id.value(), i)
							);
						}

					} break;

					case AST::Kind::INFIX: {
						if(param_type.qualifiers.empty() == false){ continue; }

						const evo::Result<SymbolProc::TypeID> symbol_proc_type_id =
							this->analyze_type<false>(param_type);
						if(symbol_proc_type_id.isError()){ return evo::resultError; }

						this->add_instruction(
							Instruction::TemplateFuncCheckParamIsInterface(symbol_proc_type_id.value(), i)
						);
					} break;

					case AST::Kind::TYPE_DEDUCER: {
						this->add_instruction(Instruction::TemplateFuncSetParamIsDeducer(i));

						const Token::ID deducer_token_id = this->source.getASTBuffer().getTypeDeducer(param_type.base);
						const Token& deducer_token = this->source.getTokenBuffer()[deducer_token_id];

						if(deducer_token.kind() == Token::Kind::TYPE_DEDUCER){
							template_names.emplace(deducer_token.getString());
						}
					} break;

					case AST::Kind::TEMPLATED_EXPR: {
						const evo::SmallVector<std::string_view> type_deducer_names =
							this->extract_type_deducer_names(param_type);

						if(type_deducer_names.size() > 0){
							this->add_instruction(Instruction::TemplateFuncSetParamIsDeducer(i));
							
							for(const std::string_view& type_deducer_name : type_deducer_names){
								template_names.emplace(type_deducer_name);
							}
						}
					} break;

					default: break;
				}
			}

			this->add_instruction(Instruction::TemplateFuncEnd(func_decl));
		}


		if(this->is_child_symbol() && this->symbol_scopes.back() != nullptr){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol->symbol_proc_id);
			current_symbol->symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol->symbol_proc_id);
		}

		if(this->symbol_namespaces.back() != nullptr){
			this->symbol_namespaces.back()->emplace(
				current_symbol->symbol_proc.getIdent(), current_symbol->symbol_proc_id
			);
		}

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
		
		const evo::Result<SymbolProc::TypeID> aliased_type =
			this->analyze_type<false>(ast_buffer.getType(alias_decl.type));
		if(aliased_type.isError()){ return evo::resultError; }

		this->add_instruction(Instruction::AliasDef(alias_decl, aliased_type.value()));


		SymbolProcInfo& current_symbol = this->get_current_symbol();

		if(this->is_child_symbol() && this->symbol_scopes.back() != nullptr){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol.symbol_proc_id);
			current_symbol.symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol.symbol_proc_id);
		}

		if(this->symbol_namespaces.back() != nullptr){
			this->symbol_namespaces.back()->emplace(
				current_symbol.symbol_proc.getIdent(), current_symbol.symbol_proc_id
			);
		}

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


		if(this->is_child_symbol() && this->symbol_scopes.back() != nullptr){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol->symbol_proc_id);
			current_symbol->symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol->symbol_proc_id);
		}

		if(this->symbol_namespaces.back() != nullptr){
			this->symbol_namespaces.back()->emplace(
				current_symbol->symbol_proc.getIdent(), current_symbol->symbol_proc_id
			);
		}

		return evo::Result<>();
	}


	auto SymbolProcBuilder::build_interface_decl(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::InterfaceDecl& interface_decl = ast_buffer.getInterfaceDecl(stmt);

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
			this->analyze_attributes(ast_buffer.getAttributeBlock(interface_decl.attributeBlock));
		if(attribute_params_info.isError()){ return evo::resultError; }

		this->add_instruction(Instruction::InterfaceDecl(interface_decl, std::move(attribute_params_info.value())));

		this->symbol_scopes.emplace_back(nullptr);
		this->symbol_namespaces.emplace_back(nullptr);
		for(const AST::Node& method : interface_decl.methods){
			if(this->analyze_local_func(method).isError()){ return evo::resultError; }
		}
		this->symbol_namespaces.pop_back();
		this->symbol_scopes.pop_back();

		this->add_instruction(Instruction::InterfaceDef{});

		SymbolProcInfo* current_symbol = &this->get_current_symbol();
		
		if(this->is_child_symbol() && this->symbol_scopes.back() != nullptr){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol->symbol_proc_id);
			current_symbol->symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol->symbol_proc_id);
		}

		if(this->symbol_namespaces.back() != nullptr){
			this->symbol_namespaces.back()->emplace(
				current_symbol->symbol_proc.getIdent(), current_symbol->symbol_proc_id
			);
		}

		return evo::Result<>();
	}


	auto SymbolProcBuilder::build_interface_impl(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::InterfaceImpl& interface_impl = ast_buffer.getInterfaceImpl(stmt);

		const evo::Result<SymbolProc::TypeID> target = 
			this->analyze_type<true>(this->source.getASTBuffer().getType(interface_impl.target));

		if(target.isError()){ return evo::resultError; }


		this->add_instruction(Instruction::InterfaceImplDecl(interface_impl, target.value()));

		for(const AST::InterfaceImpl::Method& method : interface_impl.methods){
			this->add_instruction(Instruction::InterfaceImplMethodLookup(method.value));
		}

		this->add_instruction(Instruction::InterfaceImplDef(interface_impl));
		this->add_instruction(Instruction::InterfaceImplConstexprPIR());


		SymbolProcInfo* current_symbol = &this->get_current_symbol();

		if(this->is_child_symbol() && this->symbol_scopes.back() != nullptr){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol->symbol_proc_id);
			current_symbol->symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol->symbol_proc_id);
		}

		if(this->symbol_namespaces.back() != nullptr){
			this->symbol_namespaces.back()->emplace("impl", current_symbol->symbol_proc_id);
		}

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

		if(this->is_child_symbol() && this->symbol_scopes.back() != nullptr){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol.symbol_proc_id);
			current_symbol.symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol.symbol_proc_id);
		}

		if(this->symbol_namespaces.back() != nullptr){
			this->symbol_namespaces.back()->emplace("", this->get_current_symbol().symbol_proc_id);
		}

		// TODO(PERF): address these directly instead of moving them in
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



	template<bool NEEDS_DEF>
	auto SymbolProcBuilder::analyze_type(const AST::Type& ast_type) -> evo::Result<SymbolProc::TypeID> {
		const SymbolProc::TypeID created_type_id = this->create_type();

		if(ast_type.base.kind() == AST::Kind::PRIMITIVE_TYPE){
			this->add_instruction(Instruction::PrimitiveType(ast_type, created_type_id));
			return created_type_id;
		}else{
			const evo::Result<SymbolProc::TermInfoID> type_base = [&](){
				if constexpr(NEEDS_DEF){
					if(ast_type.qualifiers.empty() == false && ast_type.qualifiers.back().isPtr){
						return this->analyze_type_base<false>(ast_type.base);
					}else{
						return this->analyze_type_base<true>(ast_type.base);
					}
				}else{
					return this->analyze_type_base<false>(ast_type.base);
				}
			}();
			if(type_base.isError()){ return evo::resultError; }

			this->add_instruction(Instruction::UserType(ast_type, type_base.value(), created_type_id));
			return created_type_id;
		}
	}



	template<bool NEEDS_DEF>
	auto SymbolProcBuilder::analyze_type_base(const AST::Node& ast_type_base) -> evo::Result<SymbolProc::TermInfoID> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(ast_type_base.kind()){
			case AST::Kind::IDENT: { 
				return this->analyze_expr_ident<NEEDS_DEF>(ast_type_base);
			} break;

			case AST::Kind::ARRAY_TYPE: {
				const AST::ArrayType& array_type = ast_buffer.getArrayType(ast_type_base);

				const evo::Result<SymbolProc::TypeID> elem_type = this->analyze_type<true>(
					this->source.getASTBuffer().getType(array_type.elemType)
				);
				if(elem_type.isError()){ return evo::resultError; }

				auto lengths = evo::SmallVector<SymbolProc::TermInfoID>();
				lengths.reserve(array_type.lengths.size());
				for(const AST::Node& length : array_type.lengths){
					const evo::Result<SymbolProc::TermInfoID> length_term_info = this->analyze_expr<true>(length);
					if(length_term_info.isError()){ return evo::resultError; }
					lengths.emplace_back(length_term_info.value());
				}

				auto terminator = std::optional<SymbolProc::TermInfoID>();
				if(array_type.terminator.has_value()){
					if(lengths.size() > 1){
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_MULTI_DIM_ARR_WITH_TERMINATOR,
							*array_type.terminator,
							"Multi-dimentional array type cannot have a terminator"
						);
						return evo::resultError;
					}

					const evo::Result<SymbolProc::TermInfoID> terminator_info = 
						this->analyze_expr<true>(*array_type.terminator);
					if(terminator_info.isError()){ return evo::resultError; }

					terminator = terminator_info.value();
				}

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				this->add_instruction(
					Instruction::ArrayType(
						array_type, elem_type.value(), std::move(lengths), terminator, new_term_info_id
					)
				);
				return new_term_info_id;
			} break;

			case AST::Kind::TYPE_DEDUCER: {
				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				this->add_instruction(
					Instruction::TypeDeducer(ast_buffer.getTypeDeducer(ast_type_base), new_term_info_id)
				);
				return new_term_info_id;
			} break;

			case AST::Kind::TEMPLATED_EXPR: {
				const AST::TemplatedExpr& templated_expr = ast_buffer.getTemplatedExpr(ast_type_base);

				const evo::Result<SymbolProc::TermInfoID> base_type =
					this->analyze_type_base<NEEDS_DEF>(templated_expr.base);
				if(base_type.isError()){ return evo::resultError; }

				auto args = evo::SmallVector<evo::Variant<SymbolProc::TermInfoID, SymbolProc::TypeID>>();
				args.reserve(templated_expr.args.size());
				for(const AST::Node& arg : templated_expr.args){
					if(arg.kind() == AST::Kind::TYPE){
						const evo::Result<SymbolProc::TypeID> arg_type =
							this->analyze_type<true>(ast_buffer.getType(arg));
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
					Instruction::TemplatedTermWait<NEEDS_DEF>(created_struct_inst_id, created_base_term_info_id)
				);

				return created_base_term_info_id;
			} break;

			case AST::Kind::INFIX: { 
				const AST::Infix& base_type_infix = ast_buffer.getInfix(ast_type_base);

				const evo::Result<SymbolProc::TermInfoID> base_lhs =
					this->analyze_type_base<NEEDS_DEF>(base_type_infix.lhs);
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
				const AST::TypeIDConverter& type_id_converter = ast_buffer.getTypeIDConverter(ast_type_base);

				const evo::Result<SymbolProc::TermInfoID> target_type_id =
					this->analyze_expr<true>(type_id_converter.expr);
				if(target_type_id.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID created_base_type_type = this->create_term_info();
				this->add_instruction(
					Instruction::TypeIDConverter(type_id_converter, target_type_id.value(), created_base_type_type)
				);
				return created_base_type_type;
			} break;

			// TODO(FUTURE): separate out into more kinds to be more specific (errors vs fatal)
			default: {
				this->emit_error(
					Diagnostic::Code::SYMBOL_PROC_INVALID_BASE_TYPE, ast_type_base, "Invalid base type"
				);
				return evo::resultError;
			} break;
		}
	}



	// TODO(FUTURE): error on invalid statements
	auto SymbolProcBuilder::analyze_stmt(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(stmt.kind()){
			case AST::Kind::NONE:             evo::debugFatalBreak("Not a valid AST node");
			case AST::Kind::VAR_DECL:         return this->analyze_local_var(ast_buffer.getVarDecl(stmt));
			case AST::Kind::FUNC_DECL:        return this->analyze_local_func(stmt);
			case AST::Kind::ALIAS_DECL:       return this->analyze_local_alias(ast_buffer.getAliasDecl(stmt));
			case AST::Kind::TYPEDEF_DECL:     evo::unimplemented("AST::Kind::TYPEDEF_DECL");
			case AST::Kind::STRUCT_DECL:      return this->analyze_local_struct(stmt);
			case AST::Kind::INTERFACE_DECL:   return this->analyze_local_interface(stmt);
			case AST::Kind::INTERFACE_IMPL:   evo::debugFatalBreak("Invalid statment");
			case AST::Kind::RETURN:           return this->analyze_return(ast_buffer.getReturn(stmt));
			case AST::Kind::ERROR:            return this->analyze_error(ast_buffer.getError(stmt));
			case AST::Kind::UNREACHABLE:      return this->analyze_unreachable(ast_buffer.getUnreachable(stmt));
			case AST::Kind::BREAK:            return this->analyze_break(ast_buffer.getBreak(stmt));
			case AST::Kind::CONTINUE:         return this->analyze_continue(ast_buffer.getContinue(stmt));
			case AST::Kind::CONDITIONAL:      return this->analyze_conditional(ast_buffer.getConditional(stmt));
			case AST::Kind::WHEN_CONDITIONAL: return this->analyze_when_cond(ast_buffer.getWhenConditional(stmt));
			case AST::Kind::WHILE:            return this->analyze_while(ast_buffer.getWhile(stmt));
			case AST::Kind::DEFER:            return this->analyze_defer(ast_buffer.getDefer(stmt));
			case AST::Kind::BLOCK:            return this->analyze_stmt_block(ast_buffer.getBlock(stmt));
			case AST::Kind::FUNC_CALL:        return this->analyze_func_call(ast_buffer.getFuncCall(stmt));
			case AST::Kind::INDEXER:          evo::debugFatalBreak("Invalid statment");
			case AST::Kind::TEMPLATE_PACK:    evo::debugFatalBreak("Invalid statment");
			case AST::Kind::TEMPLATED_EXPR:   evo::debugFatalBreak("Invalid statment");
			case AST::Kind::PREFIX:           evo::debugFatalBreak("Invalid statment");
			case AST::Kind::INFIX:            return this->analyze_assignment(ast_buffer.getInfix(stmt));
			case AST::Kind::POSTFIX:          evo::debugFatalBreak("Invalid statment");
			case AST::Kind::MULTI_ASSIGN:     return this->analyze_multi_assign(ast_buffer.getMultiAssign(stmt));
			case AST::Kind::NEW:              evo::debugFatalBreak("Invalid statment");
			case AST::Kind::STRUCT_INIT_NEW:  evo::debugFatalBreak("Invalid statment");
			case AST::Kind::TRY_ELSE:         evo::debugFatalBreak("Invalid statment");
			case AST::Kind::TYPE_DEDUCER:     evo::debugFatalBreak("Invalid statment");
			case AST::Kind::ARRAY_TYPE:       evo::debugFatalBreak("Invalid statment");
			case AST::Kind::TYPE:             evo::debugFatalBreak("Invalid statment");
			case AST::Kind::TYPEID_CONVERTER: evo::debugFatalBreak("Invalid statment");
			case AST::Kind::ATTRIBUTE_BLOCK:  evo::debugFatalBreak("Invalid statment");
			case AST::Kind::ATTRIBUTE:        evo::debugFatalBreak("Invalid statment");
			case AST::Kind::PRIMITIVE_TYPE:   evo::debugFatalBreak("Invalid statment");
			case AST::Kind::UNINIT:           evo::debugFatalBreak("Invalid statment");
			case AST::Kind::ZEROINIT:         evo::debugFatalBreak("Invalid statment");
			case AST::Kind::DISCARD:          evo::debugFatalBreak("Invalid statment");

			case AST::Kind::IDENT:
			case AST::Kind::INTRINSIC:
			case AST::Kind::LITERAL:
			case AST::Kind::THIS: {
				this->emit_error(Diagnostic::Code::SYMBOL_PROC_INVALID_STMT, stmt, "Invalid statement");
				return evo::resultError;
			} break;
		}

		evo::unreachable();
	}



	auto SymbolProcBuilder::analyze_local_var(const AST::VarDecl& var_decl) -> evo::Result<> {
		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info = this->analyze_attributes(
			this->source.getASTBuffer().getAttributeBlock(var_decl.attributeBlock)
		);
		if(attribute_params_info.isError()){ return evo::resultError; }

		auto type_id = std::optional<SymbolProc::TypeID>();
		if(var_decl.type.has_value()){
			const evo::Result<SymbolProc::TypeID> type_id_res = this->analyze_type<true>(
				this->source.getASTBuffer().getType(*var_decl.type)
			);
			if(type_id_res.isError()){ return evo::resultError; }
			type_id = type_id_res.value();
		}

		if(var_decl.value.has_value() == false){
			this->emit_error(
				Diagnostic::Code::SYMBOL_PROC_VAR_WITH_NO_VALUE,
				var_decl,
				"Local variables need to be defined with a value"
			);
			return evo::resultError;
		}
		const evo::Result<SymbolProc::TermInfoID> value = [&](){
			if(var_decl.kind == AST::VarDecl::Kind::DEF){
				return this->analyze_expr<true>(*var_decl.value);
			}else{
				return this->analyze_expr<false>(*var_decl.value);
			}
		}();
		if(value.isError()){ return evo::resultError; }

		this->add_instruction(
			Instruction::LocalVar(var_decl, std::move(attribute_params_info.value()), type_id, value.value())
		);
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_local_func(const AST::Node& stmt) -> evo::Result<> {
		evo::debugAssert(stmt.kind() == AST::Kind::FUNC_DECL, "Not a func decl");

		const evo::Result<SymbolProc::ID> func_symbol_proc_id = this->build(stmt);
		if(func_symbol_proc_id.isError()){ return evo::resultError; }

		this->context.symbol_proc_manager.getSymbolProc(func_symbol_proc_id.value()).is_sub_symbol = true;

		this->add_instruction(Instruction::WaitOnSubSymbolProcDef(func_symbol_proc_id.value()));
		return evo::Result<>();
	}



	auto SymbolProcBuilder::analyze_local_alias(const AST::AliasDecl& alias_decl) -> evo::Result<> {
		const evo::Result<SymbolProc::TypeID> aliased_type =
			this->analyze_type<true>(this->source.getASTBuffer().getType(alias_decl.type));
		if(aliased_type.isError()){ return evo::resultError; }


		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info = this->analyze_attributes(
			this->source.getASTBuffer().getAttributeBlock(alias_decl.attributeBlock)
		);
		if(attribute_params_info.isError()){ return evo::resultError; }

		this->add_instruction(
			Instruction::LocalAlias(alias_decl, std::move(attribute_params_info.value()), aliased_type.value())
		);
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_local_struct(const AST::Node& stmt) -> evo::Result<> {
		evo::debugAssert(stmt.kind() == AST::Kind::STRUCT_DECL, "Not a struct decl");

		const evo::Result<SymbolProc::ID> func_symbol_proc_id = this->build(stmt);
		if(func_symbol_proc_id.isError()){ return evo::resultError; }

		this->context.symbol_proc_manager.getSymbolProc(func_symbol_proc_id.value()).is_sub_symbol = true;

		this->add_instruction(Instruction::WaitOnSubSymbolProcDef(func_symbol_proc_id.value()));
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_local_interface(const AST::Node& stmt) -> evo::Result<> {
		evo::debugAssert(stmt.kind() == AST::Kind::INTERFACE_DECL, "Not an interface decl");

		const evo::Result<SymbolProc::ID> func_symbol_proc_id = this->build(stmt);
		if(func_symbol_proc_id.isError()){ return evo::resultError; }

		this->context.symbol_proc_manager.getSymbolProc(func_symbol_proc_id.value()).is_sub_symbol = true;

		this->add_instruction(Instruction::WaitOnSubSymbolProcDef(func_symbol_proc_id.value()));
		return evo::Result<>();
	}



	auto SymbolProcBuilder::analyze_return(const AST::Return& return_stmt) -> evo::Result<> {
		if(return_stmt.value.is<AST::Node>()){
			const evo::Result<SymbolProc::TermInfoID> return_value = 
				this->analyze_expr<false>(return_stmt.value.as<AST::Node>());
			if(return_value.isError()){ return evo::resultError; }

			if(return_stmt.label.has_value()) [[unlikely]] {
				this->add_instruction(Instruction::LabeledReturn(return_stmt, return_value.value()));
			}else{
				this->add_instruction(Instruction::Return(return_stmt, return_value.value()));
			}
			return evo::Result<>();
			
		}else{
			if(return_stmt.label.has_value()){
				if(return_stmt.value.is<Token::ID>()){
					this->add_instruction(Instruction::LabeledReturn(return_stmt, std::nullopt));
				}else{
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_LABELED_VOID_RETURN,
						return_stmt,
						"Labeled return must have a value or [...]"
					);
					return evo::Result<>();
				}
			}else{
				this->add_instruction(Instruction::Return(return_stmt, std::nullopt));
			}
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


	auto SymbolProcBuilder::analyze_break(const AST::Break& break_stmt) -> evo::Result<> {
		this->add_instruction(Instruction::Break(break_stmt));
		return evo::Result<>();
	}

	auto SymbolProcBuilder::analyze_continue(const AST::Continue& continue_stmt) -> evo::Result<> {
		this->add_instruction(Instruction::Continue(continue_stmt));
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_conditional(const AST::Conditional& conditional_stmt) -> evo::Result<> {
		const AST::Conditional* target_conditional = &conditional_stmt;

		while(true){
			const evo::Result<SymbolProc::TermInfoID> cond = this->analyze_expr<false>(target_conditional->cond);
			if(cond.isError()){ return evo::resultError; }

			this->add_instruction(Instruction::BeginCond(conditional_stmt, cond.value()));

			const AST::Block& then_block = this->source.getASTBuffer().getBlock(target_conditional->thenBlock);
			for(const AST::Node& stmt : then_block.stmts){
				if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
			}

			if(target_conditional->elseBlock.has_value() == false){
				this->add_instruction(Instruction::CondNoElse{});
				break;
			}

			if(target_conditional->elseBlock->kind() == AST::Kind::BLOCK){
				this->add_instruction(Instruction::CondElse{});

				const AST::Block& else_block = this->source.getASTBuffer().getBlock(*target_conditional->elseBlock);
				for(const AST::Node& stmt : else_block.stmts){
					if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
				}

				this->add_instruction(Instruction::EndCond{});

				break;
			}

			this->add_instruction(Instruction::CondElseIf{});
			target_conditional = &this->source.getASTBuffer().getConditional(*target_conditional->elseBlock);
		}

		this->add_instruction(Instruction::EndCondSet{});

		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_when_cond(const AST::WhenConditional& when_stmt) -> evo::Result<> {
		auto end_when_instrs = evo::SmallVector<Instruction*>();

		const AST::WhenConditional* target_when = &when_stmt;

		while(true){
			const evo::Result<SymbolProc::TermInfoID> cond = this->analyze_expr<false>(target_when->cond);
			if(cond.isError()){ return evo::resultError; }

			Instruction& new_instr = this->add_instruction(
				Instruction::BeginLocalWhenCond(when_stmt, cond.value(), SymbolProc::InstructionIndex::dummy())
			);

			const AST::Block& then_block = this->source.getASTBuffer().getBlock(target_when->thenBlock);
			for(const AST::Node& stmt : then_block.stmts){
				if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
			}
			end_when_instrs.emplace_back(
				&this->add_instruction(Instruction::EndLocalWhenCond(SymbolProc::InstructionIndex::dummy()))
			);

			new_instr.inst.as<Instruction::BeginLocalWhenCond>().else_index = SymbolProc::InstructionIndex(
				uint32_t(this->get_current_symbol().symbol_proc.instructions.size() - 1)
			);

			if(target_when->elseBlock.has_value() == false){
				break;
			}

			if(target_when->elseBlock->kind() == AST::Kind::BLOCK){
				const AST::Block& else_block = this->source.getASTBuffer().getBlock(*target_when->elseBlock);
				for(const AST::Node& stmt : else_block.stmts){
					if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
				}

				end_when_instrs.emplace_back(
					&this->add_instruction(Instruction::EndLocalWhenCond(SymbolProc::InstructionIndex::dummy()))
				);
				break;
			}

			target_when = &this->source.getASTBuffer().getWhenConditional(*target_when->elseBlock);
		}

		for(Instruction* end_when_instr : end_when_instrs){
			end_when_instr->inst.as<Instruction::EndLocalWhenCond>().end_index = SymbolProc::InstructionIndex(
				uint32_t(this->get_current_symbol().symbol_proc.instructions.size() - 1)
			);
		}

		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_while(const AST::While& while_stmt) -> evo::Result<> {
		const evo::Result<SymbolProc::TermInfoID> cond_expr = this->analyze_expr<false>(while_stmt.cond);
		if(cond_expr.isError()){ return evo::resultError; }

		this->add_instruction(Instruction::BeginWhile(while_stmt, cond_expr.value()));

		const AST::Block& block = this->source.getASTBuffer().getBlock(while_stmt.block);
		for(const AST::Node& stmt : block.stmts){
			if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
		}

		this->add_instruction(Instruction::EndWhile());

		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_defer(const AST::Defer& defer_stmt) -> evo::Result<> {
		this->add_instruction(Instruction::BeginDefer(defer_stmt));

		const AST::Block& block = this->source.getASTBuffer().getBlock(defer_stmt.block);
		for(const AST::Node& stmt : block.stmts){
			if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
		}

		this->add_instruction(Instruction::EndDefer());

		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_unreachable(Token::ID unreachable_token) -> evo::Result<> {
		this->add_instruction(Instruction::Unreachable(unreachable_token));
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_stmt_block(const AST::Block& stmt_block) -> evo::Result<> {
		this->add_instruction(Instruction::BeginStmtBlock(stmt_block));

		for(const AST::Node& stmt : stmt_block.stmts){
			if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
		}

		this->add_instruction(Instruction::EndStmtBlock());

		return evo::Result<>();
	}


	// TODO(FUTURE): deduplicate with `analyze_expr_func_call`?
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
				"Templated function call statements are currently unimplemented"
			);
			return evo::resultError;
		}

		this->add_instruction(Instruction::FuncCall(func_call, target.value(), std::move(args)));
		return evo::Result<>();
	}



	auto SymbolProcBuilder::analyze_assignment(const AST::Infix& infix) -> evo::Result<> {
		if(infix.lhs.kind() == AST::Kind::DISCARD){
			const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<false>(infix.rhs);
			if(rhs.isError()){ return evo::resultError; }

			this->add_instruction(Instruction::DiscardingAssignment(infix, rhs.value()));
			return evo::Result<>();
		}

		const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<false>(infix.lhs);
		if(lhs.isError()){ return evo::resultError; }

		const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<false>(infix.rhs);
		if(rhs.isError()){ return evo::resultError; }

		this->add_instruction(Instruction::Assignment(infix, lhs.value(), rhs.value()));
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_multi_assign(const AST::MultiAssign& multi_assign) -> evo::Result<> {
		auto targets = evo::SmallVector<std::optional<SymbolProc::TermInfoID>>();
		targets.reserve(multi_assign.assigns.size());
		for(const AST::Node assign : multi_assign.assigns){
			if(assign.kind() != AST::Kind::DISCARD){
				const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<false>(assign);
				if(target.isError()){ return evo::resultError; }

				targets.emplace_back(target.value());
			}else{
				targets.emplace_back();
			}
		}

		const evo::Result<SymbolProc::TermInfoID> value = this->analyze_expr<false>(multi_assign.value);
		if(value.isError()){ return evo::resultError; }

		this->add_instruction(Instruction::MultiAssign(multi_assign, std::move(targets), value.value()));
		return evo::Result<>();
	}



	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_term(const AST::Node& expr) -> evo::Result<SymbolProc::TermInfoID> {
		return this->analyze_term_impl<IS_CONSTEXPR, false, false>(expr);
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr(const AST::Node& expr) -> evo::Result<SymbolProc::TermInfoID> {
		return this->analyze_term_impl<IS_CONSTEXPR, true, false>(expr);
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_erroring_expr(const AST::Node& expr) -> evo::Result<SymbolProc::TermInfoID> {
		return this->analyze_term_impl<IS_CONSTEXPR, true, true>(expr);
	}


	template<bool IS_CONSTEXPR, bool MUST_BE_EXPR, bool ERRORS>
	auto SymbolProcBuilder::analyze_term_impl(const AST::Node& expr) -> evo::Result<SymbolProc::TermInfoID> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		if constexpr(ERRORS){
			if(expr.kind() != AST::Kind::FUNC_CALL){
				this->emit_error(
					Diagnostic::Code::SEMA_NOT_ERRORING_FUNC_CALL,
					expr,
					"The attempt in any flavor of a try expression must be an erroring function call"
				);
				return evo::resultError;
			}

			return this->analyze_expr_func_call<IS_CONSTEXPR, true>(expr);

		}else{
			switch(expr.kind()){
				case AST::Kind::NONE: {
					evo::debugFatalBreak("Invalid AST::Node");
				} break;

				case AST::Kind::BLOCK:           return this->analyze_expr_block<IS_CONSTEXPR>(expr);
				case AST::Kind::FUNC_CALL:       return this->analyze_expr_func_call<IS_CONSTEXPR, false>(expr);
				case AST::Kind::INDEXER:         return this->analyze_expr_indexer<IS_CONSTEXPR>(expr);
				case AST::Kind::TEMPLATED_EXPR:  return this->analyze_expr_templated<IS_CONSTEXPR>(expr);
				case AST::Kind::PREFIX:          return this->analyze_expr_prefix<IS_CONSTEXPR>(expr);
				case AST::Kind::INFIX:           return this->analyze_expr_infix<IS_CONSTEXPR>(expr);
				case AST::Kind::POSTFIX:         return this->analyze_expr_postfix<IS_CONSTEXPR>(expr);
				case AST::Kind::NEW:             return this->analyze_expr_new<IS_CONSTEXPR>(expr);
				case AST::Kind::STRUCT_INIT_NEW: return this->analyze_expr_struct_init_new<IS_CONSTEXPR>(expr);
				case AST::Kind::TRY_ELSE:        return this->analyze_expr_try_else<IS_CONSTEXPR>(expr);
				case AST::Kind::IDENT:           return this->analyze_expr_ident<IS_CONSTEXPR>(expr);
				case AST::Kind::INTRINSIC:       return this->analyze_expr_intrinsic(expr);
				case AST::Kind::LITERAL:         return this->analyze_expr_literal(ast_buffer.getLiteral(expr));
				case AST::Kind::UNINIT:          return this->analyze_expr_uninit(ast_buffer.getUninit(expr));
				case AST::Kind::ZEROINIT:        return this->analyze_expr_zeroinit(ast_buffer.getZeroinit(expr));
				case AST::Kind::THIS:            return this->analyze_expr_this(ast_buffer.getThis(expr));

				case AST::Kind::TYPE_DEDUCER: {
					evo::debugFatalBreak("Type deducer should not be allowed in this context");
				} break;

				case AST::Kind::ARRAY_TYPE: {
					const AST::ArrayType& array_type = ast_buffer.getArrayType(expr);

					const evo::Result<SymbolProc::TypeID> elem_type = this->analyze_type<true>(
						this->source.getASTBuffer().getType(array_type.elemType)
					);
					if(elem_type.isError()){ return evo::resultError; }

					auto lengths = evo::SmallVector<SymbolProc::TermInfoID>();
					lengths.reserve(array_type.lengths.size());
					for(const AST::Node& length : array_type.lengths){
						const evo::Result<SymbolProc::TermInfoID> length_term_info = this->analyze_expr<true>(length);
						if(length_term_info.isError()){ return evo::resultError; }
						lengths.emplace_back(length_term_info.value());
					}

					auto terminator = std::optional<SymbolProc::TermInfoID>();
					if(array_type.terminator.has_value()){
						if(lengths.size() > 1){
							this->emit_error(
								Diagnostic::Code::SYMBOL_PROC_MULTI_DIM_ARR_WITH_TERMINATOR,
								*array_type.terminator,
								"Multi-dimentional array type cannot have a terminator"
							);
							return evo::resultError;
						}

						const evo::Result<SymbolProc::TermInfoID> terminator_info = 
							this->analyze_expr<true>(*array_type.terminator);
						if(terminator_info.isError()){ return evo::resultError; }

						terminator = terminator_info.value();
					}

					const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
					this->add_instruction(
						Instruction::ArrayType(
							array_type, elem_type.value(), std::move(lengths), terminator, new_term_info_id
						)
					);
					return new_term_info_id;
				} break;

				case AST::Kind::TYPE: {
					if constexpr(MUST_BE_EXPR){
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_TYPE_USED_AS_EXPR, expr, "Type used as expression"
						);
						return evo::resultError;
					}else{
						const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();

						const evo::Result<SymbolProc::TypeID> type_id =
							this->analyze_type<true>(ast_buffer.getType(expr));
						if(type_id.isError()){ return evo::resultError; }

						this->add_instruction(Instruction::TypeToTerm(type_id.value(), new_term_info_id));
						return new_term_info_id;
					}
				} break;

				case AST::Kind::TYPEID_CONVERTER: {
					if constexpr(MUST_BE_EXPR){
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_TYPEID_CONVERTER_AS_EXPR,
							expr,
							"Type ID converter cannot be an expression"
						);
						return evo::resultError;
					}else{
						const AST::TypeIDConverter& type_id_converter = ast_buffer.getTypeIDConverter(expr);

						const evo::Result<SymbolProc::TermInfoID> target_type_id =
							this->analyze_expr<true>(type_id_converter.expr);
						if(target_type_id.isError()){ return evo::resultError; }

						const SymbolProc::TermInfoID created_base_type_type = this->create_term_info();
						this->add_instruction(
							Instruction::TypeIDConverter(
								type_id_converter, target_type_id.value(), created_base_type_type
							)
						);
						return created_base_type_type;
					}
				} break;

				case AST::Kind::VAR_DECL:        case AST::Kind::FUNC_DECL:        case AST::Kind::ALIAS_DECL:
				case AST::Kind::TYPEDEF_DECL:    case AST::Kind::STRUCT_DECL:      case AST::Kind::INTERFACE_DECL:
				case AST::Kind::INTERFACE_IMPL:  case AST::Kind::RETURN:           case AST::Kind::ERROR:
				case AST::Kind::UNREACHABLE:     case AST::Kind::BREAK:            case AST::Kind::CONTINUE:
				case AST::Kind::CONDITIONAL:     case AST::Kind::WHEN_CONDITIONAL: case AST::Kind::WHILE:
				case AST::Kind::DEFER:           case AST::Kind::TEMPLATE_PACK:    case AST::Kind::MULTI_ASSIGN:
				case AST::Kind::ATTRIBUTE_BLOCK: case AST::Kind::ATTRIBUTE:        case AST::Kind::PRIMITIVE_TYPE:
				case AST::Kind::DISCARD: {
					// TODO(FUTURE): better messaging (specify what kind)
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
	}



	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_block(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const AST::Block& block = this->source.getASTBuffer().getBlock(node);

		evo::debugAssert(block.label.has_value(), "Block expr must have label");

		if constexpr(IS_CONSTEXPR){
			this->emit_error(
				Diagnostic::Code::SYMBOL_PROC_CONSTEXPR_BLOCK_EXPR, block, "Block expressions cannot be constexpr"
			);
			return evo::resultError;

		}else{
			auto output_types = evo::SmallVector<SymbolProc::TypeID>();
			output_types.reserve(block.outputs.size());
			for(const AST::Block::Output& output : block.outputs){
				const evo::Result<SymbolProc::TypeID> output_type = this->analyze_type<true>(
					this->source.getASTBuffer().getType(output.typeID)
				);
				if(output_type.isError()){ return evo::resultError; }
				output_types.emplace_back(output_type.value());
			}

			this->add_instruction(Instruction::BeginExprBlock(block, *block.label, std::move(output_types)));

			for(const AST::Node& stmt : block.stmts){
				if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
			}

			const SymbolProc::TermInfoID output_term_info = this->create_term_info();
			this->add_instruction(Instruction::EndExprBlock(block, output_term_info));
			return output_term_info;
		}
	}

	

	template<bool IS_CONSTEXPR, bool ERRORS>
	auto SymbolProcBuilder::analyze_expr_func_call(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const AST::FuncCall& func_call = this->source.getASTBuffer().getFuncCall(node);

		if(func_call.target.kind() == AST::Kind::INTRINSIC){
			const Token::ID intrin_tok_id = this->source.getASTBuffer().getIntrinsic(func_call.target);
			if(this->source.getTokenBuffer()[intrin_tok_id].getString() == "import"){
				if constexpr(ERRORS){
					this->emit_error(
						Diagnostic::Code::SEMA_IMPORT_DOESNT_ERROR,
						intrin_tok_id,
						"Intrinsic @import does not error"
					);
					return evo::resultError;
				}else{
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
		}


		bool is_target_template = false;
		auto template_args = evo::SmallVector<SymbolProc::TermInfoID>();
		const auto target = [&]() -> evo::Result<SymbolProc::TermInfoID> {
			if(func_call.target.kind() == AST::Kind::TEMPLATED_EXPR){
				is_target_template = true;

				const AST::TemplatedExpr& target_templated_expr = 
					this->source.getASTBuffer().getTemplatedExpr(func_call.target);

				template_args.reserve(target_templated_expr.args.size());
				for(const AST::Node& arg : target_templated_expr.args){
					const evo::Result<SymbolProc::TermInfoID> arg_value = this->analyze_term<true>(arg);
					if(arg_value.isError()){ return evo::resultError; }
					template_args.emplace_back(arg_value.value());
				}

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
			if(this->source.getASTBuffer().getTemplatedExpr(func_call.target).base.kind() == AST::Kind::INTRINSIC){
				this->add_instruction(
					Instruction::TemplateIntrinsicFuncCall<IS_CONSTEXPR>(
						func_call, std::move(template_args), std::move(args), target.value(), new_term_info_id
					)
				);

				return new_term_info_id;

			}
		}


		if constexpr(IS_CONSTEXPR){
			if constexpr(ERRORS){
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					func_call.target,
					"Erroring constexpr function calls are unimplemented"
				);
				return evo::resultError;
			}else{
				this->add_instruction(
					Instruction::FuncCallExpr<true, false>(
						func_call, std::move(template_args), args, target.value(), new_term_info_id
					)
				);

				const SymbolProc::TermInfoID comptime_res_term_info_id = this->create_term_info();
				this->add_instruction(
					Instruction::ConstexprFuncCallRun(
						func_call, new_term_info_id, comptime_res_term_info_id, std::move(args)
					)
				);
				return comptime_res_term_info_id;
			}

		}else{
			this->add_instruction(
				Instruction::FuncCallExpr<false, ERRORS>(
					func_call, std::move(template_args), std::move(args), target.value(), new_term_info_id
				)
			);

			return new_term_info_id;
		}

	}


	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_indexer(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::Indexer& indexer = ast_buffer.getIndexer(node);

		const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<IS_CONSTEXPR>(indexer.target);
		if(target.isError()){ return evo::resultError; }
		
		auto indices = evo::SmallVector<SymbolProc::TermInfoID>();
		indices.reserve(indexer.indices.size());
		for(const AST::Node& index : indexer.indices){
			const evo::Result<SymbolProc::TermInfoID> index_res = this->analyze_expr<IS_CONSTEXPR>(index);
			if(index_res.isError()){ return evo::resultError; }
			indices.emplace_back(index_res.value());
		}

		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(
			Instruction::Indexer<IS_CONSTEXPR>(indexer, target.value(), new_term_info_id, std::move(indices))
		);
		return new_term_info_id;
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
				const evo::Result<SymbolProc::TypeID> arg_type = this->analyze_type<true>(ast_buffer.getType(arg));
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
			Instruction::TemplatedTermWait<true>(created_struct_inst_id, created_base_term_info_id)
		);

		return created_base_term_info_id;
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_prefix(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const AST::Prefix& prefix = this->source.getASTBuffer().getPrefix(node);

		switch(this->source.getTokenBuffer()[prefix.opTokenID].kind()){
			case Token::lookupKind("&"): {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(target.isError()){ return evo::resultError; }

				this->add_instruction(Instruction::AddrOf<false>(prefix, target.value(), created_term_info_id));

				return created_term_info_id;
			} break;

			case Token::lookupKind("&|"): {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(target.isError()){ return evo::resultError; }

				this->add_instruction(Instruction::AddrOf<true>(prefix, target.value(), created_term_info_id));

				return created_term_info_id;
			} break;

			case Token::Kind::KEYWORD_COPY: {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(target.isError()){ return evo::resultError; }

				this->add_instruction(Instruction::Copy(prefix, target.value(), created_term_info_id));

				return created_term_info_id;
			} break;

			case Token::Kind::KEYWORD_MOVE: {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(target.isError()){ return evo::resultError; }

				this->add_instruction(Instruction::Move(prefix, target.value(), created_term_info_id));

				return created_term_info_id;
			} break;

			case Token::Kind::KEYWORD_FORWARD: {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(target.isError()){ return evo::resultError; }

				this->add_instruction(Instruction::Forward(prefix, target.value(), created_term_info_id));

				return created_term_info_id;
			} break;

			case Token::lookupKind("-"): {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> expr = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(expr.isError()){ return evo::resultError; }

				this->add_instruction(
					Instruction::PrefixNegate<IS_CONSTEXPR>(prefix, expr.value(), created_term_info_id)
				);

				return created_term_info_id;
			} break;

			case Token::lookupKind("!"): {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> expr = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(expr.isError()){ return evo::resultError; }

				this->add_instruction(
					Instruction::PrefixNot<IS_CONSTEXPR>(prefix, expr.value(), created_term_info_id)
				);

				return created_term_info_id;
			} break;

			case Token::lookupKind("~"): {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> expr = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(expr.isError()){ return evo::resultError; }

				this->add_instruction(
					Instruction::PrefixBitwiseNot<IS_CONSTEXPR>(prefix, expr.value(), created_term_info_id)
				);

				return created_term_info_id;
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported prefix operator");
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_infix(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const AST::Infix& infix = this->source.getASTBuffer().getInfix(node);

		switch(this->source.getTokenBuffer()[infix.opTokenID].kind()){
			case Token::lookupKind("."): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const Token::ID rhs = this->source.getASTBuffer().getIdent(infix.rhs);

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				this->add_instruction(Instruction::Accessor<IS_CONSTEXPR>(infix, lhs.value(), rhs, new_term_info_id));
				return new_term_info_id;
			} break;

			case Token::Kind::KEYWORD_AS: {
				const evo::Result<SymbolProc::TermInfoID> expr = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(expr.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TypeID> target_type =
					this->analyze_type<true>(this->source.getASTBuffer().getType(infix.rhs));
				if(target_type.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				this->add_instruction(
					Instruction::As<IS_CONSTEXPR>(infix, expr.value(), target_type.value(), new_term_info_id)
				);
				return new_term_info_id;
			} break;

			case Token::lookupKind("||"): case Token::lookupKind("&&"): {
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					node,
					"Building symbol proc of [&&] and [||] is unimplemented"
				);
				return evo::resultError;
			} break;

			case Token::lookupKind("=="): case Token::lookupKind("!="): case Token::lookupKind("<"):
			case Token::lookupKind("<="): case Token::lookupKind(">"):  case Token::lookupKind(">="): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<IS_CONSTEXPR>(infix.rhs);
				if(rhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				this->add_instruction(
					Instruction::MathInfix<IS_CONSTEXPR, Instruction::MathInfixKind::COMPARATIVE>(
						infix, lhs.value(), rhs.value(), new_term_info_id
					)
				);
				return new_term_info_id;
			} break;

			case Token::lookupKind("&"):  case Token::lookupKind("|"):   case Token::lookupKind("^"):
			case Token::lookupKind("+%"): case Token::lookupKind("+|"):  case Token::lookupKind("-%"):
			case Token::lookupKind("-|"): case Token::lookupKind("*%"):  case Token::lookupKind("*|"): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<IS_CONSTEXPR>(infix.rhs);
				if(rhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				this->add_instruction(
					Instruction::MathInfix<IS_CONSTEXPR, Instruction::MathInfixKind::INTEGRAL_MATH>(
						infix, lhs.value(), rhs.value(), new_term_info_id
					)
				);
				return new_term_info_id;
			} break;

			case Token::lookupKind("+"): case Token::lookupKind("-"): case Token::lookupKind("*"):
			case Token::lookupKind("/"): case Token::lookupKind("%"): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<IS_CONSTEXPR>(infix.rhs);
				if(rhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				this->add_instruction(
					Instruction::MathInfix<IS_CONSTEXPR, Instruction::MathInfixKind::MATH>(
						infix, lhs.value(), rhs.value(), new_term_info_id
					)
				);
				return new_term_info_id;
			} break;


			case Token::lookupKind("<<"): case Token::lookupKind("<<|"): case Token::lookupKind(">>"): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<IS_CONSTEXPR>(infix.rhs);
				if(rhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				this->add_instruction(
					Instruction::MathInfix<IS_CONSTEXPR, Instruction::MathInfixKind::SHIFT>(
						infix, lhs.value(), rhs.value(), new_term_info_id
					)
				);
				return new_term_info_id;
			} break;

			default: {
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					node,
					"Building symbol proc of infix (not [.] or [as]) is unimplemented"
				);
				return evo::resultError;
			} break;
		}
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_postfix(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const AST::Postfix& postfix = this->source.getASTBuffer().getPostfix(node);

		switch(this->source.getTokenBuffer()[postfix.opTokenID].kind()){
			case Token::lookupKind(".*"): {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<IS_CONSTEXPR>(postfix.lhs);
				if(target.isError()){ return evo::resultError; }

				this->add_instruction(Instruction::Deref(postfix, target.value(), created_term_info_id));

				return created_term_info_id;
			} break;

			case Token::lookupKind(".?"): {
				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					node,
					"Building symbol proc of postfix [.?] is unimplemented"
				);
				return evo::resultError;
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported postfix operator");
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_new(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		this->emit_error(
			Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE, node, "Building symbol proc of [new] is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_struct_init_new(const AST::Node& node)
	-> evo::Result<SymbolProc::TermInfoID> {
		const AST::StructInitNew& struct_init_new =  this->source.getASTBuffer().getStructInitNew(node);

		const evo::Result<SymbolProc::TypeID> type_id = this->analyze_type<true>(
			this->source.getASTBuffer().getType(struct_init_new.type)
		);
		if(type_id.isError()){ return evo::resultError; }

		auto member_inits = evo::SmallVector<SymbolProc::TermInfoID>();
		member_inits.reserve(struct_init_new.memberInits.size());
		for(const AST::StructInitNew::MemberInit& member_init : struct_init_new.memberInits){
			const evo::Result<SymbolProc::TermInfoID> member_init_expr =
				this->analyze_expr<IS_CONSTEXPR>(member_init.expr);
			if(member_init_expr.isError()){ return evo::resultError; }

			member_inits.emplace_back(member_init_expr.value());
		}

		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(
			Instruction::StructInitNew<IS_CONSTEXPR>(
				struct_init_new, type_id.value(), new_term_info_id, std::move(member_inits)
			)
		);
		return new_term_info_id;
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_try_else(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const AST::TryElse& try_else = this->source.getASTBuffer().getTryElse(node);
 
		const evo::Result<SymbolProc::TermInfoID> attempt_expr =
			this->analyze_erroring_expr<IS_CONSTEXPR>(try_else.attemptExpr);
		if(attempt_expr.isError()){ return evo::resultError; }

		const SymbolProc::TermInfoID except_params_term_info_id = this->create_term_info();
		this->add_instruction(
			Instruction::PrepareTryHandler(
				try_else.exceptParams, attempt_expr.value(), except_params_term_info_id, try_else.elseTokenID
			)
		);

		const evo::Result<SymbolProc::TermInfoID> except_expr =
			this->analyze_expr<IS_CONSTEXPR>(try_else.exceptExpr);
		if(except_expr.isError()){ return evo::resultError; }
		
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(
			Instruction::TryElse(
				try_else, attempt_expr.value(), except_params_term_info_id, except_expr.value(), new_term_info_id
			)
		);
		return new_term_info_id;
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

	auto SymbolProcBuilder::analyze_expr_this(const Token::ID& this_token) -> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(Instruction::This(this_token, new_term_info_id));
		return new_term_info_id;
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

		this->add_instruction(Instruction::PushTemplateDeclInstantiationTypesScope());
		for(const AST::TemplatePack::Param& param : template_pack.params){
			const AST::Type& param_ast_type = ast_buffer.getType(param.type);
			auto param_type = std::optional<SymbolProc::TypeID>();
			if(
				param_ast_type.base.kind() != AST::Kind::PRIMITIVE_TYPE 
				|| token_buffer[ast_buffer.getPrimitiveType(param_ast_type.base)].kind() != Token::Kind::TYPE_TYPE
			){
				const evo::Result<SymbolProc::TypeID> param_type_res = this->analyze_type<false>(param_ast_type);
				if(param_type_res.isError()){ return evo::resultError; }
				param_type = param_type_res.value();
			}else{
				const std::string_view ident = this->source.getTokenBuffer()[param.ident].getString();
				this->add_instruction(Instruction::AddTemplateDeclInstantiationType(ident));
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
		this->add_instruction(Instruction::PopTemplateDeclInstantiationTypesScope());

		return template_param_infos;
	}



	auto SymbolProcBuilder::is_type_deducer(const AST::Type& type) const -> bool {
		return type.base.kind() == AST::Kind::TYPE_DEDUCER;
	}

	auto SymbolProcBuilder::extract_type_deducer_names(const AST::Type& type) const 
	-> evo::SmallVector<std::string_view> {
		auto output = evo::SmallVector<std::string_view>();

		if(type.base.kind() != AST::Kind::TYPE_DEDUCER){ return output; }

		const Token::ID deducer_token_id = this->source.getASTBuffer().getTypeDeducer(type.base);
		const Token& deducer_token = this->source.getTokenBuffer()[deducer_token_id];

		if(deducer_token.kind() == Token::Kind::ANONYMOUS_TYPE_DEDUCER){
			output.emplace_back(deducer_token.getString());
		}

		return output;
	}


}