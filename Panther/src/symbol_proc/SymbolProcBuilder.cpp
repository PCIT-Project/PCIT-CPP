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

		const SymbolProc::ID symbol_proc_id = this->context.symbol_proc_manager.create_symbol_proc(
			stmt, this->source.getID(), symbol_ident.value(), parent_symbol
		);
		SymbolProc& symbol_proc = this->context.symbol_proc_manager.getSymbolProc(symbol_proc_id);

		this->symbol_proc_infos.emplace_back(symbol_proc_id, symbol_proc);
		
		switch(stmt.kind()){
			break; case AST::Kind::VAR_DEF:
				if(this->build_var_def(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::FUNC_DEF:
				if(this->build_func_def(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::DELETED_SPECIAL_METHOD:
				if(this->build_deleted_special_method(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::FUNC_ALIAS_DEF:
				if(this->build_func_alias_def(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::ALIAS_DEF:
				if(this->build_alias_def(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::STRUCT_DEF:
				if(this->build_struct_def(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::UNION_DEF:
				if(this->build_union_def(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::ENUM_DEF:
				if(this->build_enum_def(stmt).isError()){ return evo::resultError; }

			break; case AST::Kind::INTERFACE_DEF:
				if(this->build_interface_def(stmt).isError()){ return evo::resultError; }

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

		const SymbolProc::ID symbol_proc_id = this->context.symbol_proc_manager.create_symbol_proc(
			template_symbol_proc.ast_node,
			template_symbol_proc.source_id,
			template_symbol_proc.ident,
			template_symbol_proc.parent
		);
		SymbolProc& symbol_proc = this->context.symbol_proc_manager.getSymbolProc(symbol_proc_id);

		symbol_proc.sema_scope_id = sema_scope_id;

		this->symbol_proc_infos.emplace_back(symbol_proc_id, symbol_proc);


		///////////////////////////////////
		// build struct def

		const AST::StructDef& struct_def = ast_buffer.getStructDef(template_symbol_proc.ast_node);

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
			this->analyze_attributes(ast_buffer.getAttributeBlock(struct_def.attributeBlock));
		if(attribute_params_info.isError()){ return evo::resultError; }

		this->add_instruction(
			this->context.symbol_proc_manager.createStructDeclInstatiation(
				struct_def, std::move(attribute_params_info.value()), struct_template_id, instantiation_id
			)
		);
		this->add_instruction(this->context.symbol_proc_manager.createStructDef());
		this->add_instruction(this->context.symbol_proc_manager.createStructCreatedSpecialMembersPIRIfNeeded());

		SymbolProc::StructInfo& struct_info = this->get_current_symbol().symbol_proc.extra_info
			.emplace<SymbolProc::StructInfo>(&instantiation);

		this->symbol_scopes.emplace_back(&struct_info.stmts);
		this->symbol_namespaces.emplace_back(&struct_info.member_symbols);
		for(const AST::Node& struct_stmt : ast_buffer.getBlock(struct_def.block).statements){
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

		const SymbolProc::ID symbol_proc_id = this->context.symbol_proc_manager.create_symbol_proc(
			template_symbol_proc.ast_node,
			template_symbol_proc.source_id,
			template_symbol_proc.ident,
			template_symbol_proc.parent
		);
		SymbolProc& symbol_proc = this->context.symbol_proc_manager.getSymbolProc(symbol_proc_id);

		symbol_proc.extra_info.emplace<SymbolProc::FuncInfo>();
		symbol_proc.extra_info.as<SymbolProc::FuncInfo>().instantiation = &instantiation;
		symbol_proc.extra_info.as<SymbolProc::FuncInfo>().instantiation_param_arg_types = std::move(arg_types);

		const evo::SmallVector<std::optional<TypeInfo::ID>>& instantiation_param_arg_types = 
			symbol_proc.extra_info.as<SymbolProc::FuncInfo>().instantiation_param_arg_types;

		symbol_proc.sema_scope_id = sema_scope_id;

		this->symbol_proc_infos.emplace_back(symbol_proc_id, symbol_proc);


		///////////////////////////////////
		// build func def

		const AST::FuncDef& func_def = ast_buffer.getFuncDef(template_symbol_proc.ast_node);

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
			this->analyze_attributes(ast_buffer.getAttributeBlock(func_def.attributeBlock));
		if(attribute_params_info.isError()){ return evo::resultError; }


		auto types = evo::SmallVector<std::optional<SymbolProcTypeID>>();
		types.reserve(func_def.params.size() + func_def.returns.size() + func_def.errorReturns.size());

		auto default_param_values = evo::SmallVector<std::optional<SymbolProc::TermInfoID>>();
		default_param_values.reserve(func_def.params.size());

		for(size_t i = 0; const AST::FuncDef::Param& param : func_def.params){
			if(param.type.has_value() == false){ // skip `this` param
				types.emplace_back();
				default_param_values.emplace_back();
				continue;
			}

			EVO_DEFER([&](){ i += 1; }); // this is purposely after the maybe skipping `this` param
				

			const evo::Result<SymbolProc::TypeID> param_type =
				this->analyze_type<false>(ast_buffer.getType(*param.type));
			if(param_type.isError()){ return evo::resultError; }
			types.emplace_back(param_type.value());

			if(
				instantiation_param_arg_types[i].has_value()
				&& this->is_deducer(*param.type)
			){
				this->add_instruction(
					this->context.symbol_proc_manager.createFuncDeclExtractDeducers(param_type.value(), i)
				);
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


		size_t num_extra_variadics = 0;
		if(func_def.isVariadic && instantiation_param_arg_types.size() >= func_def.params.size()){
			num_extra_variadics = instantiation_param_arg_types.size() - func_def.params.size();
		}


		for(const AST::FuncDef::Return& return_param : func_def.returns){
			const evo::Result<SymbolProc::TypeID> param_type = this->analyze_type<false>(
				ast_buffer.getType(return_param.type)
			);
			if(param_type.isError()){ return evo::resultError; }
			types.emplace_back(param_type.value());
		}

		for(const AST::FuncDef::Return& error_return_param : func_def.errorReturns){
			const evo::Result<SymbolProc::TypeID> param_type = this->analyze_type<false>(
				ast_buffer.getType(error_return_param.type)
			);
			if(param_type.isError()){ return evo::resultError; }
			types.emplace_back(param_type.value());
		}


		this->add_instruction(
			this->context.symbol_proc_manager.createFuncDeclInstantiation(
				func_def,
				std::move(attribute_params_info.value()),
				std::move(default_param_values),
				std::move(types),
				instantiation_id,
				num_extra_variadics
			)
		);

		this->add_instruction(this->context.symbol_proc_manager.createSuspendSymbolProc());


		// make sure definitions are ready for body of function
		// TODO(PERF): better way of doing this
		for(const AST::FuncDef::Param& param : func_def.params){
			if(param.type.has_value()){
				const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
					ast_buffer.getType(*param.type)
				);
				evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
			}else{
				this->add_instruction(this->context.symbol_proc_manager.createRequireThisDef());
			}
		}
		for(const AST::FuncDef::Return& return_param : func_def.returns){
			const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
				ast_buffer.getType(return_param.type)
			);
			evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
		}
		for(const AST::FuncDef::Return& error_return_param : func_def.errorReturns){
			const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
				ast_buffer.getType(error_return_param.type)
			);
			evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
		}


		this->add_instruction(this->context.symbol_proc_manager.createFuncPreBody(func_def));

		this->symbol_scopes.emplace_back(nullptr);
		this->symbol_namespaces.emplace_back(nullptr);
		for(const AST::Node& func_stmt : ast_buffer.getBlock(*func_def.block).statements){
			if(this->analyze_stmt(func_stmt).isError()){ return evo::resultError; }
		}
		this->symbol_namespaces.pop_back();
		this->symbol_scopes.pop_back();

		this->add_instruction(this->context.symbol_proc_manager.createFuncDef(func_def));
		this->add_instruction(this->context.symbol_proc_manager.createFuncPrepareConstexprPIRIfNeeded(func_def));
		this->add_instruction(this->context.symbol_proc_manager.createFuncConstexprPIRReadyIfNeeded());


		///////////////////////////////////
		// done

		symbol_proc.term_infos.resize(this->get_current_symbol().num_term_infos);
		symbol_proc.type_ids.resize(this->get_current_symbol().num_type_ids);
		symbol_proc.struct_instantiations.resize(this->get_current_symbol().num_struct_instantiations);

		this->symbol_proc_infos.pop_back();


		return symbol_proc_id;
	}



	auto SymbolProcBuilder::buildInterfaceImplDeducer(
		const BaseType::Interface::DeducerImpl& deducer_impl,
		BaseType::Interface::Impl& created_impl,
		SymbolProc* parent_interface_symbol_proc,
		sema::ScopeManager::Scope::ID sema_scope_id,
		TypeInfo::ID instantiation_type_id
	) -> SymbolProc::ID {
		const SymbolProc::ID symbol_proc_id = this->context.symbol_proc_manager.create_symbol_proc(
			deducer_impl.astInterfaceImpl, this->source.getID(), "impl", parent_interface_symbol_proc
		);
		SymbolProc& symbol_proc = this->context.symbol_proc_manager.getSymbolProc(symbol_proc_id);

		symbol_proc.sema_scope_id = sema_scope_id;

		this->symbol_proc_infos.emplace_back(symbol_proc_id, symbol_proc);


		///////////////////////////////////
		// build interface impl deducer

		const AST::InterfaceImpl& ast_interface_impl =
			this->source.getASTBuffer().getInterfaceImpl(deducer_impl.astInterfaceImpl);

		this->add_instruction(
			this->context.symbol_proc_manager.createInterfaceDeducerImplInstantiationDecl(
				ast_interface_impl, instantiation_type_id, created_impl
			)
		);


		for(SymbolProc::ID deducer_impl_method_id : deducer_impl.methods){
			const SymbolProc& deducer_impl_method =
				this->context.symbol_proc_manager.getSymbolProc(deducer_impl_method_id);

			SymbolProc::ID deducer_method_cloned_symbol_proc_id = this->context.symbol_proc_manager.create_symbol_proc(
				deducer_impl_method.ast_node,
				deducer_impl_method.source_id,
				deducer_impl_method.ident,
				&symbol_proc
			);

			SymbolProc& deducer_method_cloned_symbol_proc =
				this->context.symbol_proc_manager.getSymbolProc(deducer_method_cloned_symbol_proc_id);

			deducer_method_cloned_symbol_proc.instructions = deducer_impl_method.instructions;

			deducer_method_cloned_symbol_proc.term_infos.resize(deducer_impl_method.term_infos.size());
			deducer_method_cloned_symbol_proc.type_ids.resize(deducer_impl_method.type_ids.size());
			deducer_method_cloned_symbol_proc.struct_instantiations.resize(
				deducer_impl_method.struct_instantiations.size()
			);

			std::construct_at(&deducer_method_cloned_symbol_proc.extra_info, deducer_impl_method.extra_info);

			deducer_method_cloned_symbol_proc.is_local_symbol = true;
			

			this->context.symbol_proc_manager.num_procs_not_done -= 1;

			this->add_instruction(
				this->context.symbol_proc_manager.createInterfaceInDefImplMethod(deducer_method_cloned_symbol_proc_id)
			);
		}

		// for(const AST::InterfaceImpl::Method& method : ast_interface_impl.methods){
		// 	const evo::Result<SymbolProc::ID> func_symbol_proc_id = this->build(method.value.as<AST::Node>());
		// 	// if(func_symbol_proc_id.isError()){ return evo::resultError; }
		// 	evo::debugAssert(func_symbol_proc_id.isSuccess(), "Pretty sure this should never fail");

		// 	this->context.symbol_proc_manager.getSymbolProc(func_symbol_proc_id.value()).is_local_symbol = true;
		// 	this->context.symbol_proc_manager.num_procs_not_done -= 1;

		// 	this->add_instruction(
		// 		this->context.symbol_proc_manager.createInterfaceInDefImplMethod(func_symbol_proc_id.value())
		// 	);
		// }


		this->add_instruction(this->context.symbol_proc_manager.createInterfaceImplDef(ast_interface_impl));
		this->add_instruction(this->context.symbol_proc_manager.createInterfaceImplConstexprPIR());


		///////////////////////////////////
		// done

		symbol_proc.term_infos.resize(this->get_current_symbol().num_term_infos);
		symbol_proc.type_ids.resize(this->get_current_symbol().num_type_ids);
		symbol_proc.struct_instantiations.resize(this->get_current_symbol().num_struct_instantiations);

		this->symbol_proc_infos.pop_back();

		return symbol_proc_id;
	}






	auto SymbolProcBuilder::get_symbol_ident(const AST::Node& stmt) -> evo::Result<std::string_view> {
		const TokenBuffer& token_buffer = this->source.getTokenBuffer();
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(stmt.kind()){
			case AST::Kind::NONE: evo::debugFatalBreak("Not a valid AST node");

			case AST::Kind::VAR_DEF: {
				return token_buffer[ast_buffer.getVarDef(stmt).ident].getString();
			} break;

			case AST::Kind::FUNC_DEF: {
				const AST::FuncDef& func_def = ast_buffer.getFuncDef(stmt);
				const Token& name_token = token_buffer[func_def.name];

				if(name_token.kind() == Token::Kind::IDENT){
					return name_token.getString();
				}else{
					return std::string_view();
				}
			} break;

			case AST::Kind::DELETED_SPECIAL_METHOD: {
				return std::string_view();
			} break;

			case AST::Kind::FUNC_ALIAS_DEF: {
				return token_buffer[ast_buffer.getFuncAliasDef(stmt).ident].getString();
			} break;

			case AST::Kind::ALIAS_DEF: {
				return token_buffer[ast_buffer.getAliasDef(stmt).ident].getString();
			} break;

			case AST::Kind::STRUCT_DEF: {
				return token_buffer[ast_buffer.getStructDef(stmt).ident].getString();
			} break;

			case AST::Kind::UNION_DEF: {
				return token_buffer[ast_buffer.getUnionDef(stmt).ident].getString();
			} break;

			case AST::Kind::ENUM_DEF: {
				return token_buffer[ast_buffer.getEnumDef(stmt).ident].getString();
			} break;

			case AST::Kind::INTERFACE_DEF: {
				return token_buffer[ast_buffer.getInterfaceDef(stmt).ident].getString();
			} break;

			case AST::Kind::INTERFACE_IMPL: {
				return std::string_view();
			} break;

			case AST::Kind::WHEN_CONDITIONAL: {
				return std::string_view();
			} break;

			case AST::Kind::RETURN:         case AST::Kind::ERROR:               case AST::Kind::BREAK:
			case AST::Kind::CONTINUE:       case AST::Kind::DELETE:              case AST::Kind::CONDITIONAL:
			case AST::Kind::WHILE:          case AST::Kind::FOR:                 case AST::Kind::SWITCH:
			case AST::Kind::DEFER:          case AST::Kind::UNREACHABLE:         case AST::Kind::BLOCK:
			case AST::Kind::FUNC_CALL:      case AST::Kind::INDEXER:             case AST::Kind::TEMPLATE_PACK:
			case AST::Kind::TEMPLATED_EXPR: case AST::Kind::PREFIX:              case AST::Kind::INFIX:
			case AST::Kind::POSTFIX:        case AST::Kind::MULTI_ASSIGN:        case AST::Kind::NEW:
			case AST::Kind::ARRAY_INIT_NEW: case AST::Kind::DESIGNATED_INIT_NEW: case AST::Kind::TRY_ELSE:
			case AST::Kind::DEDUCER:        case AST::Kind::ARRAY_TYPE:          case AST::Kind::INTERFACE_MAP:
			case AST::Kind::TYPE:           case AST::Kind::TYPEID_CONVERTER:    case AST::Kind::ATTRIBUTE_BLOCK:
			case AST::Kind::ATTRIBUTE:      case AST::Kind::PRIMITIVE_TYPE:      case AST::Kind::IDENT:
			case AST::Kind::INTRINSIC:      case AST::Kind::LITERAL:             case AST::Kind::UNINIT:
			case AST::Kind::ZEROINIT:       case AST::Kind::THIS:                case AST::Kind::DISCARD: {
				this->context.emitError(
					Diagnostic::Code::SYMBOL_PROC_INVALID_GLOBAL_STMT,
					Diagnostic::Location::get(stmt, this->source),
					"Invalid global statement"
				);
				return evo::resultError;
			};
		}

		evo::debugFatalBreak("Unknown AST::Kind");
	}



	auto SymbolProcBuilder::build_var_def(const AST::Node& stmt) -> evo::Result<> {
		const AST::VarDef& var_def = this->source.getASTBuffer().getVarDef(stmt);

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info = this->analyze_attributes(
			this->source.getASTBuffer().getAttributeBlock(var_def.attributeBlock)
		);
		if(attribute_params_info.isError()){ return evo::resultError; }


		auto decl_def_type_id = std::optional<SymbolProc::TypeID>();
		if(var_def.type.has_value()){
			const evo::Result<SymbolProc::TypeID> type_id_res = 
				this->analyze_type<true>(this->source.getASTBuffer().getType(*var_def.type));
			if(type_id_res.isError()){ return evo::resultError; }


			if(this->source.getASTBuffer().getType(*var_def.type).base.kind() == AST::Kind::DEDUCER){
				decl_def_type_id = type_id_res.value();

			}else if(var_def.kind != AST::VarDef::Kind::DEF){
				this->add_instruction(
					this->context.symbol_proc_manager.createNonLocalVarDecl(
						var_def, std::move(attribute_params_info.value()), type_id_res.value()
					)
				);
			}else{
				decl_def_type_id = type_id_res.value();
			}
		}



		auto value_id = std::optional<SymbolProc::TermInfoID>();
		if(var_def.value.has_value()){
			const evo::Result<SymbolProc::TermInfoID> value_id_res = this->analyze_expr<true>(*var_def.value);
			if(value_id_res.isError()){ return evo::resultError; }

			value_id = value_id_res.value();
			
		}else{
			if(var_def.kind == AST::VarDef::Kind::DEF){
				this->emit_error(
					Diagnostic::Code::SYMBOL_PROC_VAR_WITH_NO_VALUE,
					var_def,
					"All [def] variables need to be defined with a value"
				);
				return evo::resultError;
			}

			if(var_def.type.has_value() == false){
				this->emit_error(
					Diagnostic::Code::SYMBOL_PROC_VAR_WITH_NO_VALUE,
					var_def,
					"Variables must be defined with a type and/or a value"
				);
				return evo::resultError;
			}
		}

	

		if(
			var_def.type.has_value()
			&& var_def.kind != AST::VarDef::Kind::DEF
			&& decl_def_type_id.has_value() == false
		){
			this->add_instruction(this->context.symbol_proc_manager.createNonLocalVarDef(var_def, value_id));

		}else{
			this->add_instruction(
				this->context.symbol_proc_manager.createNonLocalVarDeclDef(
					var_def, std::move(attribute_params_info.value()), decl_def_type_id, *value_id
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


	auto SymbolProcBuilder::build_func_def(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::FuncDef& func_def = ast_buffer.getFuncDef(stmt);

		SymbolProcInfo* current_symbol = &this->get_current_symbol();

		if(this->analyze_priority_and_builtin_attribute(
			ast_buffer.getAttributeBlock(func_def.attributeBlock)
		).isError()){
			return evo::resultError;
		}

		current_symbol->symbol_proc.extra_info.emplace<SymbolProc::FuncInfo>();


		auto template_param_infos = evo::SmallVector<Instruction::TemplateParamInfo>();
		if(func_def.templatePack.has_value()){
			current_symbol->is_template = true;
			evo::Result<evo::SmallVector<Instruction::TemplateParamInfo>> template_param_infos_res =
				this->analyze_template_param_pack(ast_buffer.getTemplatePack(*func_def.templatePack));

			if(template_param_infos_res.isError()){ return evo::resultError; }
			template_param_infos = std::move(template_param_infos_res.value());
		}



		const bool has_type_deducer_param = [&](){
			for(const AST::FuncDef::Param& param : func_def.params){
				if(param.type.has_value() && this->is_deducer(ast_buffer.getType(*param.type).base)){ 
					return true;
				}
			}

			return false;
		}();


		if(template_param_infos.empty() && has_type_deducer_param == false && func_def.isVariadic == false){
			evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
				this->analyze_attributes(ast_buffer.getAttributeBlock(func_def.attributeBlock));
			if(attribute_params_info.isError()){ return evo::resultError; }


			auto types = evo::SmallVector<std::optional<SymbolProcTypeID>>();
			types.reserve(func_def.params.size() + func_def.returns.size() + func_def.errorReturns.size());

			auto default_param_values = evo::SmallVector<std::optional<SymbolProc::TermInfoID>>();
			default_param_values.reserve(func_def.params.size());
			for(const AST::FuncDef::Param& param : func_def.params){
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

			for(const AST::FuncDef::Return& return_param : func_def.returns){
				const evo::Result<SymbolProc::TypeID> param_type = this->analyze_type<false>(
					ast_buffer.getType(return_param.type)
				);
				if(param_type.isError()){ return evo::resultError; }
				types.emplace_back(param_type.value());
			}

			for(const AST::FuncDef::Return& error_return_param : func_def.errorReturns){
				const evo::Result<SymbolProc::TypeID> param_type = this->analyze_type<false>(
					ast_buffer.getType(error_return_param.type)
				);
				if(param_type.isError()){ return evo::resultError; }
				types.emplace_back(param_type.value());
			}

			this->add_instruction(
				this->context.symbol_proc_manager.createFuncDecl(
					func_def,
					std::move(attribute_params_info.value()),
					std::move(default_param_values),
					std::move(types)
				)
			);


			// make sure definitions are ready for body of function
			// TODO(PERF): better way of doing this
			for(const AST::FuncDef::Param& param : func_def.params){
				if(param.type.has_value()){
					const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
						ast_buffer.getType(*param.type)
					);
					evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
				}else{
					this->add_instruction(this->context.symbol_proc_manager.createRequireThisDef());
				}
			}
			for(const AST::FuncDef::Return& return_param : func_def.returns){
				const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
					ast_buffer.getType(return_param.type)
				);
				evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
			}
			for(const AST::FuncDef::Return& error_return_param : func_def.errorReturns){
				const evo::Result<SymbolProc::TypeID> res = this->analyze_type<true>(
					ast_buffer.getType(error_return_param.type)
				);
				evo::debugAssert(res.isSuccess(), "Func param type def getting should never fail");
			}


			if(func_def.block.has_value()){
				for(const AST::FuncDef::Return& return_param : func_def.returns){
					if(this->is_deducer(return_param.type)){
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_DEFAULT_INTERFACE_METHOD_WITH_DEDUCER_RET,
							return_param.type,
							"Interface method with default implementation cannot have a deducer return type"
						);
						return evo::resultError;
					}
				}

				this->add_instruction(this->context.symbol_proc_manager.createFuncPreBody(func_def));

				this->symbol_scopes.emplace_back(nullptr);
				this->symbol_namespaces.emplace_back(nullptr);
				for(const AST::Node& func_stmt : ast_buffer.getBlock(*func_def.block).statements){
					if(this->analyze_stmt(func_stmt).isError()){ return evo::resultError; }
				}
				this->symbol_namespaces.pop_back();
				this->symbol_scopes.pop_back();

				this->add_instruction(this->context.symbol_proc_manager.createFuncDef(func_def));
				this->add_instruction(
					this->context.symbol_proc_manager.createFuncPrepareConstexprPIRIfNeeded(func_def)
				);
				this->add_instruction(this->context.symbol_proc_manager.createFuncConstexprPIRReadyIfNeeded());

			}else{
				this->add_instruction(this->context.symbol_proc_manager.createInterfaceFuncDef(func_def));
			}


			// need to set again as address may have changed
			current_symbol = &this->get_current_symbol();

		}else{
			if(func_def.block.has_value() == false){
				this->emit_error(
					Diagnostic::Code::SYMBOL_PROC_TEMPLATE_INTERFACE_METHOD,
					func_def,
					"Interface methods cannot be templates"
				);
				return evo::resultError;
			}


			if(func_def.isVariadic && this->is_named_deducer(*func_def.params.back().type)){
				this->emit_error(
					Diagnostic::Code::SYMBOL_PROC_VARIADIC_PARAM_TYPE_IS_NAMED_DEDUCER,
					*func_def.params.back().type,
					"Variadic function parameter cannot be a named deducer"
				);
				return evo::resultError;
			}


			auto template_names = std::unordered_set<std::string_view>();

			if(func_def.templatePack.has_value()){
				for(
					const AST::TemplatePack::Param& template_param
					: ast_buffer.getTemplatePack(*func_def.templatePack).params
				){
					template_names.emplace(this->source.getTokenBuffer()[template_param.ident].getString());
				}
			}


			this->add_instruction(
				this->context.symbol_proc_manager.createTemplateFuncBegin(func_def, std::move(template_param_infos))
			);

			for(size_t i = 0; const AST::FuncDef::Param& param : func_def.params){
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


				auto terms_to_check_for_deducers = std::stack<AST::Node, evo::SmallVector<AST::Node, 8>>();
				terms_to_check_for_deducers.emplace(param_type.base);

				while(terms_to_check_for_deducers.empty() == false){
					const AST::Node target_term = terms_to_check_for_deducers.top();
					terms_to_check_for_deducers.pop();

					switch(target_term.kind()){
						case AST::Kind::IDENT: {
							if(param_type.qualifiers.empty() == false){ continue; }

							const std::string_view ident_name = this->source.getTokenBuffer()[
								this->source.getASTBuffer().getIdent(target_term)
							].getString();

							if(template_names.contains(ident_name) == false){
								const evo::Result<SymbolProc::TypeID> symbol_proc_type_id =
									this->analyze_type<false>(param_type);
								if(symbol_proc_type_id.isError()){ return evo::resultError; }
							}

						} break;

						case AST::Kind::INFIX: {
							if(param_type.qualifiers.empty() == false){ continue; }

							const evo::Result<SymbolProc::TypeID> symbol_proc_type_id =
								this->analyze_type<false>(param_type);
							if(symbol_proc_type_id.isError()){ return evo::resultError; }
						} break;

						case AST::Kind::DEDUCER: {
							this->add_instruction(
								this->context.symbol_proc_manager.createTemplateFuncSetParamIsDeducer(i)
							);

							const Token::ID deducer_token_id = this->source.getASTBuffer().getDeducer(target_term);
							const Token& deducer_token = this->source.getTokenBuffer()[deducer_token_id];

							if(deducer_token.kind() == Token::Kind::DEDUCER){
								template_names.emplace(deducer_token.getString());
							}
						} break;

						case AST::Kind::TEMPLATED_EXPR: {
							const evo::SmallVector<std::string_view> type_deducer_names =
								this->extract_deducer_names(param_type.base);

							if(type_deducer_names.size() > 0){
								this->add_instruction(
									this->context.symbol_proc_manager.createTemplateFuncSetParamIsDeducer(i)
								);
								
								for(const std::string_view& type_deducer_name : type_deducer_names){
									template_names.emplace(type_deducer_name);
								}
							}
						} break;

						case AST::Kind::ARRAY_TYPE: {
							const AST::ArrayType& array_type = this->source.getASTBuffer().getArrayType(target_term);

							terms_to_check_for_deducers.emplace(array_type.elemType);

							for(const std::optional<AST::Node>& dimension : array_type.dimensions){
								if(dimension.has_value() == false){ continue; }
								terms_to_check_for_deducers.emplace(*dimension);
							}

							if(array_type.terminator.has_value()){
								terms_to_check_for_deducers.emplace(*array_type.terminator);
							}
						} break;

						default: break;
					}
				}
			}

			this->add_instruction(this->context.symbol_proc_manager.createTemplateFuncEnd(func_def));
		}



		if(
			this->is_child_symbol() == false
			|| this->get_parent_symbol().symbol_proc.ast_node.kind() != AST::Kind::INTERFACE_IMPL
		){ // prevent impl inline func defs from being added to wait on and symbol namespace
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
		}

		return evo::Result<>();
	}


	auto SymbolProcBuilder::build_deleted_special_method(const AST::Node& stmt) -> evo::Result<> {
		this->add_instruction(
			this->context.symbol_proc_manager.createDeletedSpecialMethod(
				this->source.getASTBuffer().getDeletedSpecialMethod(stmt)
			)
		);


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


	auto SymbolProcBuilder::build_func_alias_def(const AST::Node& stmt) -> evo::Result<> {
		evo::debugAssert(stmt.kind() == AST::Kind::FUNC_ALIAS_DEF, "Not an alias func decl");

		const AST::FuncAliasDef& func_alias_def = this->source.getASTBuffer().getFuncAliasDef(stmt);

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info = this->analyze_attributes(
			this->source.getASTBuffer().getAttributeBlock(func_alias_def.attributeBlock)
		);
		if(attribute_params_info.isError()){ return evo::resultError; }

		const evo::Result<SymbolProc::TermInfoID> aliased_func = this->analyze_expr<true>(func_alias_def.func);
		if(aliased_func.isError()){ return evo::resultError; }

		this->add_instruction(
			this->context.symbol_proc_manager.createFuncAliasDef(
				func_alias_def, std::move(attribute_params_info.value()), aliased_func.value()
			)
		);


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


	auto SymbolProcBuilder::build_alias_def(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::AliasDef& alias_def = ast_buffer.getAliasDef(stmt);


		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info = this->analyze_attributes(
			ast_buffer.getAttributeBlock(alias_def.attributeBlock)
		);
		if(attribute_params_info.isError()){ return evo::resultError; }

		
		const evo::Result<SymbolProc::TypeID> aliased_type =
			this->analyze_type<false>(ast_buffer.getType(alias_def.type));
		if(aliased_type.isError()){ return evo::resultError; }

		this->add_instruction(
			this->context.symbol_proc_manager.createAlias(
				alias_def, std::move(attribute_params_info.value()), aliased_type.value()
			)
		);


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


	auto SymbolProcBuilder::build_struct_def(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::StructDef& struct_def = ast_buffer.getStructDef(stmt);

		SymbolProcInfo* current_symbol = &this->get_current_symbol();

		auto template_param_infos = evo::SmallVector<Instruction::TemplateParamInfo>();
		if(struct_def.templatePack.has_value()){
			current_symbol->is_template = true;
			evo::Result<evo::SmallVector<Instruction::TemplateParamInfo>> template_param_infos_res =
				this->analyze_template_param_pack(ast_buffer.getTemplatePack(*struct_def.templatePack));

			if(template_param_infos_res.isError()){ return evo::resultError; }
			template_param_infos = std::move(template_param_infos_res.value());
		}

		if(template_param_infos.empty()){
			evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
				this->analyze_attributes(ast_buffer.getAttributeBlock(struct_def.attributeBlock));
			if(attribute_params_info.isError()){ return evo::resultError; }

			this->add_instruction(
				this->context.symbol_proc_manager.createStructDecl(
					struct_def, std::move(attribute_params_info.value())
				)
			);
			this->add_instruction(this->context.symbol_proc_manager.createStructDef());
			this->add_instruction(this->context.symbol_proc_manager.createStructCreatedSpecialMembersPIRIfNeeded());

			SymbolProc::StructInfo& struct_info =
				current_symbol->symbol_proc.extra_info.emplace<SymbolProc::StructInfo>();


			this->symbol_scopes.emplace_back(&struct_info.stmts);
			this->symbol_namespaces.emplace_back(&struct_info.member_symbols);
			for(const AST::Node& struct_stmt : ast_buffer.getBlock(struct_def.block).statements){
				if(this->build(struct_stmt).isError()){ return evo::resultError; }
			}
			this->symbol_namespaces.pop_back();
			this->symbol_scopes.pop_back();

			// need to set again as address may have changed
			current_symbol = &this->get_current_symbol();

		}else{
			this->add_instruction(
				this->context.symbol_proc_manager.createTemplateStruct(struct_def, std::move(template_param_infos))
			);
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
	

	auto SymbolProcBuilder::build_union_def(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::UnionDef& union_def = ast_buffer.getUnionDef(stmt);

		SymbolProcInfo* current_symbol = &this->get_current_symbol();

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
			this->analyze_attributes(ast_buffer.getAttributeBlock(union_def.attributeBlock));
		if(attribute_params_info.isError()){ return evo::resultError; }


		this->add_instruction(
			this->context.symbol_proc_manager.createUnionDecl(union_def, std::move(attribute_params_info.value()))
		);

		auto field_types = evo::SmallVector<SymbolProc::TypeID>();
		for(const AST::UnionDef::Field& field : union_def.fields){
			const evo::Result<SymbolProc::TypeID> field_type = this->analyze_type<true>(ast_buffer.getType(field.type));
			if(field_type.isError()){ return evo::resultError; }

			field_types.emplace_back(field_type.value());
		}

		this->add_instruction(
			this->context.symbol_proc_manager.createUnionAddFields(union_def, std::move(field_types))
		);

		this->add_instruction(this->context.symbol_proc_manager.createUnionDef());

		SymbolProc::UnionInfo& union_info =
			current_symbol->symbol_proc.extra_info.emplace<SymbolProc::UnionInfo>();

		this->symbol_scopes.emplace_back(&union_info.stmts);
		this->symbol_namespaces.emplace_back(&union_info.member_symbols);
		for(const AST::Node& union_stmt : union_def.statements){
			if(this->build(union_stmt).isError()){ return evo::resultError; }
		}
		this->symbol_namespaces.pop_back();
		this->symbol_scopes.pop_back();


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


	auto SymbolProcBuilder::build_enum_def(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::EnumDef& enum_def = ast_buffer.getEnumDef(stmt);

		SymbolProcInfo* current_symbol = &this->get_current_symbol();

		auto underlying_type = std::optional<SymbolProc::TypeID>();
		if(enum_def.underlyingType.has_value()){
			const evo::Result<SymbolProc::TypeID> underlying_type_result =
				this->analyze_type<true>(ast_buffer.getType(*enum_def.underlyingType));
			if(underlying_type_result.isError()){ return evo::resultError; }
			
			underlying_type = underlying_type_result.value();
		}

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
			this->analyze_attributes(ast_buffer.getAttributeBlock(enum_def.attributeBlock));
		if(attribute_params_info.isError()){ return evo::resultError; }


		this->add_instruction(
			this->context.symbol_proc_manager.createEnumDecl(
				enum_def, underlying_type, std::move(attribute_params_info.value())
			)
		);

		auto enumerator_values = evo::SmallVector<std::optional<SymbolProc::TermInfoID>>();
		for(const AST::EnumDef::Enumerator& enumerator : enum_def.enumerators){
			if(enumerator.value.has_value()){
				const evo::Result<SymbolProc::TermInfoID> enumerator_value =
					this->analyze_expr<true>(*enumerator.value);
				if(enumerator_value.isError()){ return evo::resultError; }

				enumerator_values.emplace_back(enumerator_value.value());
			}else{
				enumerator_values.emplace_back();
			}
		}

		this->add_instruction(
			this->context.symbol_proc_manager.createEnumAddEnumerators(enum_def, std::move(enumerator_values))
		);

		this->add_instruction(this->context.symbol_proc_manager.createEnumDef());

		SymbolProc::EnumInfo& enum_info =
			current_symbol->symbol_proc.extra_info.emplace<SymbolProc::EnumInfo>();

		this->symbol_scopes.emplace_back(&enum_info.stmts);
		this->symbol_namespaces.emplace_back(&enum_info.member_symbols);
		for(const AST::Node& enum_stmt : enum_def.statements){
			if(this->build(enum_stmt).isError()){ return evo::resultError; }
		}
		this->symbol_namespaces.pop_back();
		this->symbol_scopes.pop_back();


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



	auto SymbolProcBuilder::build_interface_def(const AST::Node& stmt) -> evo::Result<> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::InterfaceDef& interface_def = ast_buffer.getInterfaceDef(stmt);

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
			this->analyze_attributes(ast_buffer.getAttributeBlock(interface_def.attributeBlock));
		if(attribute_params_info.isError()){ return evo::resultError; }

		this->add_instruction(
			this->context.symbol_proc_manager.createInterfacePrepare(
				interface_def, std::move(attribute_params_info.value())
			)
		);

		this->symbol_scopes.emplace_back(nullptr);

		// methods
		this->symbol_namespaces.emplace_back(nullptr);
		for(const AST::Node& method : interface_def.methods){
			if(this->analyze_local_func(method).isError()){ return evo::resultError; }
		}
		this->symbol_namespaces.pop_back();

		this->add_instruction(this->context.symbol_proc_manager.createInterfaceDecl());

		// impls
		if(interface_def.impls.empty() == false){
			this->symbol_namespaces.emplace_back(nullptr);
			for(const AST::Node& impl : interface_def.impls){
				const evo::Result<SymbolProc::ID> impl_symbol_proc_id = this->build(impl);
				if(impl_symbol_proc_id.isError()){ return evo::resultError; }

				this->context.symbol_proc_manager.getSymbolProc(impl_symbol_proc_id.value()).is_local_symbol = true;
				this->context.symbol_proc_manager.num_procs_not_done -= 1;

				this->add_instruction(
					this->context.symbol_proc_manager.createWaitOnSubSymbolProcDef(impl_symbol_proc_id.value())
				);
			}
			this->symbol_namespaces.pop_back();
		}

		this->symbol_scopes.pop_back();

		this->add_instruction(this->context.symbol_proc_manager.createInterfaceDef());


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

		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
			this->analyze_attributes(ast_buffer.getAttributeBlock(interface_impl.attributeBlock));
		if(attribute_params_info.isError()){ return evo::resultError; }


		const evo::Result<SymbolProc::TypeID> target = 
			this->analyze_type<true>(this->source.getASTBuffer().getType(interface_impl.target));

		if(target.isError()){ return evo::resultError; }


		bool in_def = false;

		if(this->is_child_symbol()){
			in_def = this->get_parent_symbol().symbol_proc.ast_node.kind() == AST::Kind::INTERFACE_DEF;

		}else{
			const SymbolProc& current_symbol_proc = this->get_current_symbol().symbol_proc;

			if(current_symbol_proc.builtin_symbol_proc_kind.has_value() == false){
				this->emit_error(
					Diagnostic::Code::SYMBOL_PROC_INVALID_SCOPE_FOR_IMPL,
					stmt,
					"Invalid scope for interface impl"
				);
				return evo::resultError;
			}

			switch(*current_symbol_proc.builtin_symbol_proc_kind){
				case SymbolProcManager::constevalLookupBuiltinSymbolKind("array.IIterable"):
				case SymbolProcManager::constevalLookupBuiltinSymbolKind("array.IIterableRT"):
				case SymbolProcManager::constevalLookupBuiltinSymbolKind("arrayRef.IIterableRef"):
				case SymbolProcManager::constevalLookupBuiltinSymbolKind("arrayRef.IIterableRefRT"):
				case SymbolProcManager::constevalLookupBuiltinSymbolKind("arrayMutRef.IIterableMutRef"):
				case SymbolProcManager::constevalLookupBuiltinSymbolKind("arrayMutRef.IIterableMutRefRT"): {
					in_def = true;
				} break;

				default: {
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_INVALID_SCOPE_FOR_IMPL,
						stmt,
						"Invalid scope for interface impl"
					);
					return evo::resultError;
				} break;
			}
		}


		if(in_def){
			this->add_instruction(
				this->context.symbol_proc_manager.createInterfaceInDefImplDecl(interface_impl, stmt, target.value())
			);
		}else{
			this->add_instruction(
				this->context.symbol_proc_manager.createInterfaceImplDecl(interface_impl, target.value())
			);
		}

		for(const AST::InterfaceImpl::Method& method : interface_impl.methods){
			if(in_def || method.value.is<AST::Node>()){
				const evo::Result<SymbolProc::ID> func_symbol_proc_id = this->build(method.value.as<AST::Node>());
				if(func_symbol_proc_id.isError()){ return evo::resultError; }

				this->context.symbol_proc_manager.getSymbolProc(func_symbol_proc_id.value()).is_local_symbol = true;
				this->context.symbol_proc_manager.num_procs_not_done -= 1;

				this->add_instruction(
					this->context.symbol_proc_manager.createInterfaceInDefImplMethod(func_symbol_proc_id.value())
				);
				
			}else{
				this->add_instruction(
					this->context.symbol_proc_manager.createInterfaceImplMethodLookup(method.value.as<Token::ID>())
				);
			}
		}

		this->add_instruction(this->context.symbol_proc_manager.createInterfaceImplDef(interface_impl));
		this->add_instruction(this->context.symbol_proc_manager.createInterfaceImplConstexprPIR());


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
		for(const AST::Node& then_stmt : ast_buffer.getBlock(when_conditional.thenBlock).statements){
			if(this->build(then_stmt).isError()){ return evo::resultError; }
		}
		this->symbol_scopes.pop_back();

		auto else_symbol_scope = SymbolScope();
		if(when_conditional.elseBlock.has_value()){
			this->symbol_scopes.emplace_back(&else_symbol_scope);
			if(when_conditional.elseBlock->kind() == AST::Kind::BLOCK){
				for(const AST::Node& else_stmt : ast_buffer.getBlock(*when_conditional.elseBlock).statements){
					if(this->build(else_stmt).isError()){ return evo::resultError; }
				}
			}else{
				if(this->build(*when_conditional.elseBlock).isError()){ return evo::resultError; }
			}
			this->symbol_scopes.pop_back();
		}
		

		this->add_instruction(this->context.symbol_proc_manager.createWhenCond(when_conditional, cond_id.value()));


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
			if constexpr(NEEDS_DEF){
				this->add_instruction(
					this->context.symbol_proc_manager.createPrimitiveTypeNeedsDef(ast_type, created_type_id)
				);
			}else{
				this->add_instruction(this->context.symbol_proc_manager.createPrimitiveType(ast_type, created_type_id));
			}
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

			this->add_instruction(
				this->context.symbol_proc_manager.createUserType(ast_type, type_base.value(), created_type_id)
			);
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

			case AST::Kind::INTRINSIC: { 
				return this->analyze_expr_intrinsic(ast_type_base);
			} break;

			case AST::Kind::ARRAY_TYPE: {
				const AST::ArrayType& array_type = ast_buffer.getArrayType(ast_type_base);

				const evo::Result<SymbolProc::TypeID> elem_type = this->analyze_type<true>(
					this->source.getASTBuffer().getType(array_type.elemType)
				);
				if(elem_type.isError()){ return evo::resultError; }


				if(array_type.refIsMut.has_value()){ // array ref
					auto dimensions = evo::SmallVector<std::optional<SymbolProcTermInfoID>>();
					dimensions.reserve(array_type.dimensions.size());
					for(const std::optional<AST::Node>& dimension : array_type.dimensions){
						if(dimension.has_value()){
							const evo::Result<SymbolProc::TermInfoID> dimension_term_info =
								this->analyze_expr<true>(*dimension);

							if(dimension_term_info.isError()){ return evo::resultError; }
							dimensions.emplace_back(dimension_term_info.value());

						}else{
							dimensions.emplace_back();
						}
					}

					auto terminator = std::optional<SymbolProc::TermInfoID>();
					if(array_type.terminator.has_value()){
						const evo::Result<SymbolProc::TermInfoID> terminator_info = 
							this->analyze_expr<true>(*array_type.terminator);
						if(terminator_info.isError()){ return evo::resultError; }

						terminator = terminator_info.value();
					}

					const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
					this->add_instruction(
						this->context.symbol_proc_manager.createArrayRef(
							array_type, elem_type.value(), std::move(dimensions), terminator, new_term_info_id
						)
					);
					return new_term_info_id;

				}else{ // array
					auto dimensions = evo::SmallVector<SymbolProc::TermInfoID>();
					dimensions.reserve(array_type.dimensions.size());
					for(const std::optional<AST::Node>& dimension : array_type.dimensions){
						if(dimension->kind() == AST::Kind::DEDUCER){
							const SymbolProc::TermInfoID created_deducer = this->create_term_info();
							this->add_instruction(
								this->context.symbol_proc_manager.createExprDeducer(
									this->source.getASTBuffer().getDeducer(*dimension), created_deducer
								)
							);
							dimensions.emplace_back(created_deducer);

						}else{
							const evo::Result<SymbolProc::TermInfoID> dimension_term_info =
								this->analyze_expr<true>(*dimension);

							if(dimension_term_info.isError()){ return evo::resultError; }
							dimensions.emplace_back(dimension_term_info.value());
						}
					}

					auto terminator = std::optional<SymbolProc::TermInfoID>();
					if(array_type.terminator.has_value()){
						if(array_type.terminator->kind() == AST::Kind::DEDUCER){
							const SymbolProc::TermInfoID created_deducer = this->create_term_info();
							this->add_instruction(
								this->context.symbol_proc_manager.createExprDeducer(
									this->source.getASTBuffer().getDeducer(*array_type.terminator), created_deducer
								)
							);
							terminator = created_deducer;

						}else{
							const evo::Result<SymbolProc::TermInfoID> terminator_info = 
								this->analyze_expr<true>(*array_type.terminator);
							if(terminator_info.isError()){ return evo::resultError; }

							terminator = terminator_info.value();
						}
					}

					const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
					this->add_instruction(
						this->context.symbol_proc_manager.createArrayType(
							array_type, elem_type.value(), std::move(dimensions), terminator, new_term_info_id
						)
					);
					return new_term_info_id;
				}
			} break;

			case AST::Kind::INTERFACE_MAP: {
				const AST::InterfaceMap& interface_map = ast_buffer.getInterfaceMap(ast_type_base);

				auto base_type = std::optional<SymbolProc::TypeID>();
				if(interface_map.underlyingType.is<AST::Node>()){
					const evo::Result<SymbolProc::TypeID> underlying_type_type_res = this->analyze_type<true>(
						this->source.getASTBuffer().getType(interface_map.underlyingType.as<AST::Node>())
					);
					if(underlying_type_type_res.isError()){ return evo::resultError; }
					base_type = underlying_type_type_res.value();
				}

				const evo::Result<SymbolProc::TypeID> interface_type = this->analyze_type<true>(
					this->source.getASTBuffer().getType(interface_map.interface)
				);
				if(interface_type.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				this->add_instruction(
					this->context.symbol_proc_manager.createInterfaceMap(
						interface_map, base_type, interface_type.value(), new_term_info_id
					)
				);
				return new_term_info_id;
			} break;

			case AST::Kind::DEDUCER: {
				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				this->add_instruction(
					this->context.symbol_proc_manager.createTypeDeducer(
						ast_buffer.getDeducer(ast_type_base), new_term_info_id
					)
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
					this->context.symbol_proc_manager.createTemplatedTerm(
						templated_expr, base_type.value(), std::move(args), created_struct_inst_id
					)
				);


				if constexpr(NEEDS_DEF){
					this->add_instruction(
						this->context.symbol_proc_manager.createTemplatedTermWaitForDef(
							templated_expr, created_struct_inst_id, created_base_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createTemplatedTermWaitForDecl(
							templated_expr, created_struct_inst_id, created_base_term_info_id
						)
					);
				}

				return created_base_term_info_id;
			} break;

			case AST::Kind::INFIX: { 
				const AST::Infix& base_type_infix = ast_buffer.getInfix(ast_type_base);

				const evo::Result<SymbolProc::TermInfoID> base_lhs =
					this->analyze_type_base<NEEDS_DEF>(base_type_infix.lhs);
				if(base_lhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID created_base_type_type = this->create_term_info();
				this->add_instruction(
					this->context.symbol_proc_manager.createAccessorNeedsDef(
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
					this->context.symbol_proc_manager.createTypeIDConverter(
						type_id_converter, target_type_id.value(), created_base_type_type
					)
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
			case AST::Kind::NONE:                   evo::debugFatalBreak("Not a valid AST node");
			case AST::Kind::VAR_DEF:               return this->analyze_local_var(ast_buffer.getVarDef(stmt));
			case AST::Kind::FUNC_DEF:              return this->analyze_local_func(stmt);

			case AST::Kind::DELETED_SPECIAL_METHOD: {
				this->emit_error(
					Diagnostic::Code::SYMBOL_PROC_INVALID_STMT,
					stmt,
					"Invalid local statement",
					evo::SmallVector<Diagnostic::Info>{
						Diagnostic::Info("Deleted special methods are only allowed in structs")
					}
				);
				return evo::resultError;
			} break;

			case AST::Kind::FUNC_ALIAS_DEF:
				return this->analyze_local_func_alias(ast_buffer.getFuncAliasDef(stmt));

			case AST::Kind::ALIAS_DEF:              return this->analyze_local_alias(ast_buffer.getAliasDef(stmt));
			case AST::Kind::STRUCT_DEF:             return this->analyze_local_struct(stmt);
			case AST::Kind::UNION_DEF:              return this->analyze_local_union(stmt);
			case AST::Kind::ENUM_DEF:               return this->analyze_local_enum(stmt);
			case AST::Kind::INTERFACE_DEF:          return this->analyze_local_interface(stmt);
			case AST::Kind::INTERFACE_IMPL:         evo::debugFatalBreak("Invalid statment");
			case AST::Kind::RETURN:                 return this->analyze_return(ast_buffer.getReturn(stmt));
			case AST::Kind::ERROR:                  return this->analyze_error(ast_buffer.getError(stmt));
			case AST::Kind::UNREACHABLE:            return this->analyze_unreachable(ast_buffer.getUnreachable(stmt));
			case AST::Kind::BREAK:                  return this->analyze_break(ast_buffer.getBreak(stmt));
			case AST::Kind::CONTINUE:               return this->analyze_continue(ast_buffer.getContinue(stmt));
			case AST::Kind::DELETE:                 return this->analyze_delete(ast_buffer.getDelete(stmt));
			case AST::Kind::CONDITIONAL:            return this->analyze_conditional(ast_buffer.getConditional(stmt));
			case AST::Kind::WHEN_CONDITIONAL:       return this->analyze_when_cond(ast_buffer.getWhenConditional(stmt));
			case AST::Kind::WHILE:                  return this->analyze_while(ast_buffer.getWhile(stmt));
			case AST::Kind::FOR:                    return this->analyze_for(ast_buffer.getFor(stmt));
			case AST::Kind::SWITCH:                 return this->analyze_switch(ast_buffer.getSwitch(stmt));
			case AST::Kind::DEFER:                  return this->analyze_defer(ast_buffer.getDefer(stmt));
			case AST::Kind::BLOCK:                  return this->analyze_stmt_block(ast_buffer.getBlock(stmt));
			case AST::Kind::FUNC_CALL:              return this->analyze_func_call(ast_buffer.getFuncCall(stmt));
			case AST::Kind::INDEXER:                evo::debugFatalBreak("Invalid statment");
			case AST::Kind::TEMPLATE_PACK:          evo::debugFatalBreak("Invalid statment");
			case AST::Kind::TEMPLATED_EXPR:         evo::debugFatalBreak("Invalid statment");
			case AST::Kind::PREFIX:                 evo::debugFatalBreak("Invalid statment");
			case AST::Kind::INFIX:                  return this->analyze_assignment(ast_buffer.getInfix(stmt));
			case AST::Kind::POSTFIX:                evo::debugFatalBreak("Invalid statment");
			case AST::Kind::MULTI_ASSIGN:           return this->analyze_multi_assign(ast_buffer.getMultiAssign(stmt));
			case AST::Kind::NEW:                    evo::debugFatalBreak("Invalid statment");
			case AST::Kind::ARRAY_INIT_NEW:         evo::debugFatalBreak("Invalid statment");
			case AST::Kind::DESIGNATED_INIT_NEW:    evo::debugFatalBreak("Invalid statment");
			case AST::Kind::TRY_ELSE:               return this->analyze_try_else(ast_buffer.getTryElse(stmt));
			case AST::Kind::DEDUCER:                evo::debugFatalBreak("Invalid statment");
			case AST::Kind::ARRAY_TYPE:             evo::debugFatalBreak("Invalid statment");
			case AST::Kind::INTERFACE_MAP:          evo::debugFatalBreak("Invalid statment");
			case AST::Kind::TYPE:                   evo::debugFatalBreak("Invalid statment");
			case AST::Kind::TYPEID_CONVERTER:       evo::debugFatalBreak("Invalid statment");
			case AST::Kind::ATTRIBUTE_BLOCK:        evo::debugFatalBreak("Invalid statment");
			case AST::Kind::ATTRIBUTE:              evo::debugFatalBreak("Invalid statment");
			case AST::Kind::PRIMITIVE_TYPE:         evo::debugFatalBreak("Invalid statment");
			case AST::Kind::UNINIT:                 evo::debugFatalBreak("Invalid statment");
			case AST::Kind::ZEROINIT:               evo::debugFatalBreak("Invalid statment");
			case AST::Kind::DISCARD:                evo::debugFatalBreak("Invalid statment");

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



	auto SymbolProcBuilder::analyze_local_var(const AST::VarDef& var_def) -> evo::Result<> {
		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info = this->analyze_attributes(
			this->source.getASTBuffer().getAttributeBlock(var_def.attributeBlock)
		);
		if(attribute_params_info.isError()){ return evo::resultError; }

		auto type_id = std::optional<SymbolProc::TypeID>();
		if(var_def.type.has_value()){
			const evo::Result<SymbolProc::TypeID> type_id_res = this->analyze_type<true>(
				this->source.getASTBuffer().getType(*var_def.type)
			);
			if(type_id_res.isError()){ return evo::resultError; }
			type_id = type_id_res.value();
		}

		if(var_def.value.has_value() == false){
			this->emit_error(
				Diagnostic::Code::SYMBOL_PROC_VAR_WITH_NO_VALUE,
				var_def,
				"Local variables need to be defined with a value"
			);
			return evo::resultError;
		}
		const evo::Result<SymbolProc::TermInfoID> value = [&](){
			if(var_def.kind == AST::VarDef::Kind::DEF){
				return this->analyze_expr<true>(*var_def.value);
			}else{
				return this->analyze_expr<false>(*var_def.value);
			}
		}();
		if(value.isError()){ return evo::resultError; }

		this->add_instruction(
			this->context.symbol_proc_manager.createLocalVar(
				var_def, std::move(attribute_params_info.value()), type_id, value.value()
			)
		);
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_local_func(const AST::Node& stmt) -> evo::Result<> {
		evo::debugAssert(stmt.kind() == AST::Kind::FUNC_DEF, "Not a func decl");

		const evo::Result<SymbolProc::ID> func_symbol_proc_id = this->build(stmt);
		if(func_symbol_proc_id.isError()){ return evo::resultError; }

		this->context.symbol_proc_manager.getSymbolProc(func_symbol_proc_id.value()).is_local_symbol = true;
		this->context.symbol_proc_manager.num_procs_not_done -= 1;

		this->add_instruction(
			this->context.symbol_proc_manager.createWaitOnSubSymbolProcDef(func_symbol_proc_id.value())
		);
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_local_func_alias(const AST::FuncAliasDef& func_alias_def) -> evo::Result<> {
		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info = this->analyze_attributes(
			this->source.getASTBuffer().getAttributeBlock(func_alias_def.attributeBlock)
		);
		if(attribute_params_info.isError()){ return evo::resultError; }

		const evo::Result<SymbolProc::TermInfoID> aliased_func = this->analyze_expr<true>(func_alias_def.func);
		if(aliased_func.isError()){ return evo::resultError; }

		this->add_instruction(
			this->context.symbol_proc_manager.createLocalFuncAlias(
				func_alias_def, std::move(attribute_params_info.value()), aliased_func.value()
			)
		);

		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_local_alias(const AST::AliasDef& alias_def) -> evo::Result<> {
		const evo::Result<SymbolProc::TypeID> aliased_type =
			this->analyze_type<true>(this->source.getASTBuffer().getType(alias_def.type));
		if(aliased_type.isError()){ return evo::resultError; }


		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info = this->analyze_attributes(
			this->source.getASTBuffer().getAttributeBlock(alias_def.attributeBlock)
		);
		if(attribute_params_info.isError()){ return evo::resultError; }

		this->add_instruction(
			this->context.symbol_proc_manager.createLocalAlias(
				alias_def, std::move(attribute_params_info.value()), aliased_type.value()
			)
		);
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_local_struct(const AST::Node& stmt) -> evo::Result<> {
		evo::debugAssert(stmt.kind() == AST::Kind::STRUCT_DEF, "Not a struct decl");

		const evo::Result<SymbolProc::ID> func_symbol_proc_id = this->build(stmt);
		if(func_symbol_proc_id.isError()){ return evo::resultError; }

		this->context.symbol_proc_manager.getSymbolProc(func_symbol_proc_id.value()).is_local_symbol = true;
		this->context.symbol_proc_manager.num_procs_not_done -= 1;

		this->add_instruction(
			this->context.symbol_proc_manager.createWaitOnSubSymbolProcDef(func_symbol_proc_id.value())
		);
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_local_union(const AST::Node& stmt) -> evo::Result<> {
		evo::debugAssert(stmt.kind() == AST::Kind::UNION_DEF, "Not a union decl");

		const evo::Result<SymbolProc::ID> func_symbol_proc_id = this->build(stmt);
		if(func_symbol_proc_id.isError()){ return evo::resultError; }

		this->context.symbol_proc_manager.getSymbolProc(func_symbol_proc_id.value()).is_local_symbol = true;
		this->context.symbol_proc_manager.num_procs_not_done -= 1;

		this->add_instruction(
			this->context.symbol_proc_manager.createWaitOnSubSymbolProcDef(func_symbol_proc_id.value())
		);
		return evo::Result<>();
	}

	auto SymbolProcBuilder::analyze_local_enum(const AST::Node& stmt) -> evo::Result<> {
		evo::debugAssert(stmt.kind() == AST::Kind::ENUM_DEF, "Not a enum decl");

		const evo::Result<SymbolProc::ID> func_symbol_proc_id = this->build(stmt);
		if(func_symbol_proc_id.isError()){ return evo::resultError; }

		this->context.symbol_proc_manager.getSymbolProc(func_symbol_proc_id.value()).is_local_symbol = true;
		this->context.symbol_proc_manager.num_procs_not_done -= 1;

		this->add_instruction(
			this->context.symbol_proc_manager.createWaitOnSubSymbolProcDef(func_symbol_proc_id.value())
		);
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_local_interface(const AST::Node& stmt) -> evo::Result<> {
		evo::debugAssert(stmt.kind() == AST::Kind::INTERFACE_DEF, "Not an interface decl");

		const evo::Result<SymbolProc::ID> func_symbol_proc_id = this->build(stmt);
		if(func_symbol_proc_id.isError()){ return evo::resultError; }

		this->context.symbol_proc_manager.getSymbolProc(func_symbol_proc_id.value()).is_local_symbol = true;
		this->context.symbol_proc_manager.num_procs_not_done -= 1;

		this->add_instruction(
			this->context.symbol_proc_manager.createWaitOnSubSymbolProcDef(func_symbol_proc_id.value())
		);
		return evo::Result<>();
	}



	auto SymbolProcBuilder::analyze_return(const AST::Return& return_stmt) -> evo::Result<> {
		if(return_stmt.value.is<AST::Node>()){
			const evo::Result<SymbolProc::TermInfoID> return_value = 
				this->analyze_expr<false>(return_stmt.value.as<AST::Node>());
			if(return_value.isError()){ return evo::resultError; }

			if(return_stmt.label.has_value()) [[unlikely]] {
				this->add_instruction(
					this->context.symbol_proc_manager.createLabeledReturn(return_stmt, return_value.value())
				);
			}else{
				this->add_instruction(
					this->context.symbol_proc_manager.createReturn(return_stmt, return_value.value())
				);
			}
			return evo::Result<>();
			
		}else{
			if(return_stmt.label.has_value()){
				if(return_stmt.value.is<Token::ID>()){
					this->add_instruction(
						this->context.symbol_proc_manager.createLabeledReturn(return_stmt, std::nullopt)
					);
				}else{
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_LABELED_VOID_RETURN,
						return_stmt,
						"Labeled return must have a value or [...]"
					);
					return evo::Result<>();
				}
			}else{
				this->add_instruction(this->context.symbol_proc_manager.createReturn(return_stmt, std::nullopt));
			}
			return evo::Result<>();
		}
	}

	auto SymbolProcBuilder::analyze_error(const AST::Error& error_stmt) -> evo::Result<> {
		if(error_stmt.value.is<AST::Node>()){
			const evo::Result<SymbolProc::TermInfoID> error_value = 
				this->analyze_expr<false>(error_stmt.value.as<AST::Node>());
			if(error_value.isError()){ return evo::resultError; }

			this->add_instruction(this->context.symbol_proc_manager.createError(error_stmt, error_value.value()));
			return evo::Result<>();
			
		}else{
			this->add_instruction(this->context.symbol_proc_manager.createError(error_stmt, std::nullopt));
			return evo::Result<>();
		}
	}


	auto SymbolProcBuilder::analyze_break(const AST::Break& break_stmt) -> evo::Result<> {
		this->add_instruction(this->context.symbol_proc_manager.createBreak(break_stmt));
		return evo::Result<>();
	}

	auto SymbolProcBuilder::analyze_continue(const AST::Continue& continue_stmt) -> evo::Result<> {
		this->add_instruction(this->context.symbol_proc_manager.createContinue(continue_stmt));
		return evo::Result<>();
	}

	auto SymbolProcBuilder::analyze_delete(const AST::Delete& delete_stmt) -> evo::Result<> {
		const evo::Result<SymbolProc::TermInfoID> expr = this->analyze_expr<false>(delete_stmt.value);
		if(expr.isError()){ return evo::resultError; }

		this->add_instruction(this->context.symbol_proc_manager.createDelete(delete_stmt, expr.value()));
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_conditional(const AST::Conditional& conditional_stmt) -> evo::Result<> {
		const AST::Conditional* target_conditional = &conditional_stmt;

		auto close_brace = std::optional<Token::ID>();

		while(true){
			const evo::Result<SymbolProc::TermInfoID> cond = this->analyze_expr<false>(target_conditional->cond);
			if(cond.isError()){ return evo::resultError; }

			this->add_instruction(this->context.symbol_proc_manager.createBeginCond(conditional_stmt, cond.value()));

			const AST::Block& then_block = this->source.getASTBuffer().getBlock(target_conditional->thenBlock);
			for(const AST::Node& stmt : then_block.statements){
				if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
			}

			if(target_conditional->elseBlock.has_value() == false){
				this->add_instruction(this->context.symbol_proc_manager.createCondNoElse());
				close_brace = then_block.closeBrace;
				break;
			}

			if(target_conditional->elseBlock->kind() == AST::Kind::BLOCK){
				this->add_instruction(this->context.symbol_proc_manager.createCondElse());

				const AST::Block& else_block = this->source.getASTBuffer().getBlock(*target_conditional->elseBlock);
				for(const AST::Node& stmt : else_block.statements){
					if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
				}

				this->add_instruction(this->context.symbol_proc_manager.createEndCond());
				close_brace = else_block.closeBrace;
				break;
			}

			this->add_instruction(this->context.symbol_proc_manager.createCondElseIf());
			target_conditional = &this->source.getASTBuffer().getConditional(*target_conditional->elseBlock);
		}

		this->add_instruction(this->context.symbol_proc_manager.createEndCondSet(*close_brace));

		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_when_cond(const AST::WhenConditional& when_stmt) -> evo::Result<> {
		auto end_when_instrs = evo::SmallVector<Instruction>();

		const AST::WhenConditional* target_when = &when_stmt;

		while(true){
			const evo::Result<SymbolProc::TermInfoID> cond = this->analyze_expr<true>(target_when->cond);
			if(cond.isError()){ return evo::resultError; }

			const Instruction begin_local_when_cond = this->add_instruction(
				this->context.symbol_proc_manager.createBeginLocalWhenCond(
					when_stmt, cond.value(), SymbolProc::InstructionIndex::dummy()
				)
			);

			const AST::Block& then_block = this->source.getASTBuffer().getBlock(target_when->thenBlock);
			for(const AST::Node& stmt : then_block.statements){
				if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
			}
			end_when_instrs.emplace_back(
				this->add_instruction(
					this->context.symbol_proc_manager.createEndLocalWhenCond(SymbolProc::InstructionIndex::dummy())
				)
			);

			this->context.symbol_proc_manager.begin_local_when_conds[begin_local_when_cond._index].else_index = 
				SymbolProc::InstructionIndex(uint32_t(this->get_current_symbol().symbol_proc.instructions.size() - 1));

			if(target_when->elseBlock.has_value() == false){
				break;
			}

			if(target_when->elseBlock->kind() == AST::Kind::BLOCK){
				const AST::Block& else_block = this->source.getASTBuffer().getBlock(*target_when->elseBlock);
				for(const AST::Node& stmt : else_block.statements){
					if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
				}

				end_when_instrs.emplace_back(
					this->add_instruction(
						this->context.symbol_proc_manager.createEndLocalWhenCond(SymbolProc::InstructionIndex::dummy())
					)
				);
				break;
			}

			target_when = &this->source.getASTBuffer().getWhenConditional(*target_when->elseBlock);
		}

		for(const Instruction& end_when_instr : end_when_instrs){
			this->context.symbol_proc_manager.end_local_when_conds[end_when_instr._index].end_index = 
				SymbolProc::InstructionIndex(uint32_t(this->get_current_symbol().symbol_proc.instructions.size() - 1));
		}

		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_while(const AST::While& while_stmt) -> evo::Result<> {
		const evo::Result<SymbolProc::TermInfoID> cond_expr = this->analyze_expr<false>(while_stmt.cond);
		if(cond_expr.isError()){ return evo::resultError; }

		this->add_instruction(this->context.symbol_proc_manager.createBeginWhile(while_stmt, cond_expr.value()));

		const AST::Block& block = this->source.getASTBuffer().getBlock(while_stmt.block);
		for(const AST::Node& stmt : block.statements){
			if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
		}

		this->add_instruction(this->context.symbol_proc_manager.createEndWhile(block.closeBrace));

		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_for(const AST::For& for_stmt) -> evo::Result<> {
		auto iterables = evo::SmallVector<SymbolProc::TermInfoID>();
		iterables.reserve(for_stmt.iterables.size());
		for(const AST::Node& iterable : for_stmt.iterables){
			const evo::Result<SymbolProc::TermInfoID> iterable_term = this->analyze_expr<false>(iterable);
			if(iterable_term.isError()){ return evo::resultError; }

			iterables.emplace_back(iterable_term.value());
		}


		auto types = evo::SmallVector<SymbolProc::TypeID>();
		types.reserve(size_t(for_stmt.index.has_value()) + for_stmt.values.size());

		if(for_stmt.index.has_value()){
			const evo::Result<SymbolProc::TypeID> index_type =
				this->analyze_type<true>(this->source.getASTBuffer().getType(for_stmt.index->type));
			if(index_type.isError()){ return evo::resultError; }

			types.emplace_back(index_type.value());
		}

		for(const AST::For::Param& value : for_stmt.values){
			const evo::Result<SymbolProc::TypeID> value_type =
				this->analyze_type<true>(this->source.getASTBuffer().getType(value.type));
			if(value_type.isError()){ return evo::resultError; }

			types.emplace_back(value_type.value());
		}

		const AST::AttributeBlock& attribute_block =
			this->source.getASTBuffer().getAttributeBlock(for_stmt.attributeBlock);
		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
			this->analyze_attributes(attribute_block);
		if(attribute_params_info.isError()){ return evo::resultError; }

		const bool is_unroll = [&]() -> bool {
			for(const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
				const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

				if(attribute_str == "unroll"){ return true; }
			}

			return false;
		}();


		if(is_unroll){
			if(for_stmt.index.has_value()){
				this->add_instruction(
					this->context.symbol_proc_manager.createBeginForUnroll(for_stmt, iterables, std::nullopt)
				);
			}else{
				this->add_instruction(
					this->context.symbol_proc_manager.createBeginForUnroll(for_stmt, iterables, types[0])
				);
			}

			const Instruction& cond_instr = this->add_instruction(
				this->context.symbol_proc_manager.createForUnrollCond(
					for_stmt, std::move(iterables), std::move(types), SymbolProc::InstructionIndex::dummy()
				)
			);

			const auto cond_instr_index =
				SymbolProc::InstructionIndex(uint32_t(this->get_current_symbol().symbol_proc.instructions.size() - 2));


			const AST::Block& block = this->source.getASTBuffer().getBlock(for_stmt.block);
			for(const AST::Node& stmt : block.statements){
				if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
			}

			this->add_instruction(
				this->context.symbol_proc_manager.createForUnrollContinue(
					this->source.getASTBuffer().getBlock(for_stmt.block).closeBrace, cond_instr_index
				)
			);

			this->context.symbol_proc_manager.for_unroll_conds[cond_instr._index].end_index =
				SymbolProc::InstructionIndex(uint32_t(this->get_current_symbol().symbol_proc.instructions.size() - 1));

			return evo::Result<>();

		}else{
			this->add_instruction(
				this->context.symbol_proc_manager.createBeginFor(for_stmt, std::move(iterables), std::move(types))
			);

			const AST::Block& block = this->source.getASTBuffer().getBlock(for_stmt.block);
			for(const AST::Node& stmt : block.statements){
				if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
			}

			this->add_instruction(this->context.symbol_proc_manager.createEndFor(block.closeBrace));

			return evo::Result<>();
		}
	}



	auto SymbolProcBuilder::analyze_switch(const AST::Switch& switch_stmt) -> evo::Result<> {
		const evo::Result<SymbolProc::TermInfoID> cond = this->analyze_expr<false>(switch_stmt.cond);
		if(cond.isError()){ return evo::resultError; }

		const AST::AttributeBlock& attribute_block =
			this->source.getASTBuffer().getAttributeBlock(switch_stmt.attributeBlock);
		evo::Result<evo::SmallVector<Instruction::AttributeParams>> attribute_params_info =
			this->analyze_attributes(attribute_block);
		if(attribute_params_info.isError()){ return evo::resultError; }

		const bool is_no_jump = [&]() -> bool {
			for(const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
				const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

				if(attribute_str == "noJump"){ return true; }
			}

			return false;
		}();


		this->add_instruction(
			this->context.symbol_proc_manager.createBeginSwitch(
				switch_stmt, std::move(attribute_params_info.value()), cond.value(), is_no_jump
			)
		);

		for(size_t i = 0; const AST::Switch::Case& switch_case : switch_stmt.cases){
			auto values = evo::SmallVector<SymbolProc::TermInfoID>();
			values.reserve(switch_case.values.size());
			for(const AST::Node value : switch_case.values){
				const evo::Result<SymbolProc::TermInfoID> value_res = [&]() -> evo::Result<SymbolProc::TermInfoID> {
					if(is_no_jump){
						return this->analyze_expr<false>(value);
					}else{
						return this->analyze_expr<true>(value);
					}
				}();
				if(value_res.isError()){ return evo::resultError; }

				values.emplace_back(value_res.value());
			}

			this->add_instruction(
				this->context.symbol_proc_manager.createBeginCase(switch_case, std::move(values), i)
			);

			const AST::Block& block = this->source.getASTBuffer().getBlock(switch_case.block);
			for(const AST::Node& stmt : block.statements){
				if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
			}

			this->add_instruction(this->context.symbol_proc_manager.createEndCase());

			i += 1;
		}


		this->add_instruction(this->context.symbol_proc_manager.createEndSwitch(switch_stmt));

		return evo::Result<>();
	}



	auto SymbolProcBuilder::analyze_defer(const AST::Defer& defer_stmt) -> evo::Result<> {
		this->add_instruction(this->context.symbol_proc_manager.createBeginDefer(defer_stmt));

		const AST::Block& block = this->source.getASTBuffer().getBlock(defer_stmt.block);
		for(const AST::Node& stmt : block.statements){
			if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
		}

		this->add_instruction(this->context.symbol_proc_manager.createEndDefer(block.closeBrace));

		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_unreachable(Token::ID unreachable_token) -> evo::Result<> {
		this->add_instruction(this->context.symbol_proc_manager.createUnreachable(unreachable_token));
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_stmt_block(const AST::Block& stmt_block) -> evo::Result<> {
		this->add_instruction(this->context.symbol_proc_manager.createBeginStmtBlock(stmt_block));

		for(const AST::Node& stmt : stmt_block.statements){
			if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
		}

		this->add_instruction(this->context.symbol_proc_manager.createEndStmtBlock(stmt_block.closeBrace));

		return evo::Result<>();
	}


	// TODO(FUTURE): deduplicate with `analyze_expr_func_call`?
	auto SymbolProcBuilder::analyze_func_call(const AST::FuncCall& func_call) -> evo::Result<> {
		auto template_args = evo::SmallVector<SymbolProc::TermInfoID>();
		const auto target = [&]() -> evo::Result<SymbolProc::TermInfoID> {
			if(func_call.target.kind() == AST::Kind::TEMPLATED_EXPR){
				const AST::TemplatedExpr& target_templated_expr = 
					this->source.getASTBuffer().getTemplatedExpr(func_call.target);

				template_args.reserve(target_templated_expr.args.size());
				for(const AST::Node& arg : target_templated_expr.args){
					const evo::Result<SymbolProc::TermInfoID> arg_value = this->analyze_term<true>(arg);
					if(arg_value.isError()){ return evo::resultError; }
					template_args.emplace_back(arg_value.value());
				}

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

		this->add_instruction(
			this->context.symbol_proc_manager.createFuncCall(
				func_call, target.value(), std::move(template_args), std::move(args)
			)
		);
		return evo::Result<>();
	}



	auto SymbolProcBuilder::analyze_assignment(const AST::Infix& infix) -> evo::Result<> {
		if(infix.lhs.kind() == AST::Kind::DISCARD){
			const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<false>(infix.rhs);
			if(rhs.isError()){ return evo::resultError; }

			this->add_instruction(this->context.symbol_proc_manager.createDiscardingAssignment(infix, rhs.value()));
			return evo::Result<>();
		}

		if(this->source.getTokenBuffer()[infix.opTokenID].kind() == Token::lookupKind("=")){
			switch(infix.rhs.kind()){
				case AST::Kind::NEW: {
					const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<false>(infix.lhs);
					if(lhs.isError()){ return evo::resultError; }

					const AST::New& ast_new = this->source.getASTBuffer().getNew(infix.rhs);

					const evo::Result<SymbolProc::TypeID> type_id = this->analyze_type<true>(
						this->source.getASTBuffer().getType(ast_new.type)
					);
					if(type_id.isError()){ return evo::resultError; }

					auto args = evo::SmallVector<SymbolProc::TermInfoID>();
					args.reserve(ast_new.args.size());
					for(const AST::FuncCall::Arg& arg : ast_new.args){
						const evo::Result<SymbolProc::TermInfoID> value_expr = this->analyze_expr<false>(arg.value);
						if(value_expr.isError()){ return evo::resultError; }

						args.emplace_back(value_expr.value());
					}


					this->add_instruction(this->context.symbol_proc_manager.createAssignmentNew(
						infix, lhs.value(), type_id.value(), std::move(args))
					);
					return evo::Result<>();
				} break;

				case AST::Kind::PREFIX: {
					const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<false>(infix.lhs);
					if(lhs.isError()){ return evo::resultError; }

					const AST::Prefix& ast_prefix = this->source.getASTBuffer().getPrefix(infix.rhs);
					const Token& prefix_token = this->source.getTokenBuffer()[ast_prefix.opTokenID];

					if(prefix_token.kind() == Token::Kind::KEYWORD_COPY){
						const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<false>(ast_prefix.rhs);
						if(target.isError()){ return evo::resultError; }

						this->add_instruction(
							this->context.symbol_proc_manager.createAssignmentCopy(infix, lhs.value(), target.value())
						);
						return evo::Result<>();

					}else if(prefix_token.kind() == Token::Kind::KEYWORD_MOVE){
						const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<false>(ast_prefix.rhs);
						if(target.isError()){ return evo::resultError; }

						this->add_instruction(
							this->context.symbol_proc_manager.createAssignmentMove(infix, lhs.value(), target.value())
						);
						return evo::Result<>();

					}else if(prefix_token.kind() == Token::Kind::KEYWORD_FORWARD){
						const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<false>(ast_prefix.rhs);
						if(target.isError()){ return evo::resultError; }

						this->add_instruction(
							this->context.symbol_proc_manager.createAssignmentForward(
								infix, lhs.value(), target.value()
							)
						);
						return evo::Result<>();

					}else{
						break;
					}
				} break;

				default: break;
			}
		}


		const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<false>(infix.lhs);
		if(lhs.isError()){ return evo::resultError; }

		const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<false>(infix.rhs);
		if(rhs.isError()){ return evo::resultError; }


		// TODO(PERF): make optional less memory?
		const SymbolProc::TermInfoID builtin_composite_expr_term_info_id = this->create_term_info();

		this->add_instruction(
			this->context.symbol_proc_manager.createAssignment(
				infix, lhs.value(), rhs.value(), builtin_composite_expr_term_info_id
			)
		);
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

		this->add_instruction(
			this->context.symbol_proc_manager.createMultiAssign(multi_assign, std::move(targets), value.value())
		);
		return evo::Result<>();
	}


	auto SymbolProcBuilder::analyze_try_else(const AST::TryElse& try_else) -> evo::Result<> {
		const AST::FuncCall& func_call = this->source.getASTBuffer().getFuncCall(try_else.attemptExpr);

		auto template_args = evo::SmallVector<SymbolProc::TermInfoID>();
		const auto target = [&]() -> evo::Result<SymbolProc::TermInfoID> {
			if(func_call.target.kind() == AST::Kind::TEMPLATED_EXPR){
				const AST::TemplatedExpr& target_templated_expr = 
					this->source.getASTBuffer().getTemplatedExpr(func_call.target);

				template_args.reserve(target_templated_expr.args.size());
				for(const AST::Node& arg : target_templated_expr.args){
					const evo::Result<SymbolProc::TermInfoID> arg_value = this->analyze_term<true>(arg);
					if(arg_value.isError()){ return evo::resultError; }
					template_args.emplace_back(arg_value.value());
				}

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


		this->add_instruction(
			this->context.symbol_proc_manager.createTryElseBegin(
				try_else, target.value(), std::move(template_args), std::move(args)
			)
		);

		for(const AST::Node& stmt : this->source.getASTBuffer().getBlock(try_else.exceptExpr).statements){
			if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
		}

		this->add_instruction(this->context.symbol_proc_manager.createTryElseEnd());

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

				case AST::Kind::BLOCK:               return this->analyze_expr_block<IS_CONSTEXPR>(expr);
				case AST::Kind::FUNC_CALL:           return this->analyze_expr_func_call<IS_CONSTEXPR, false>(expr);
				case AST::Kind::INDEXER:             return this->analyze_expr_indexer<IS_CONSTEXPR>(expr);
				case AST::Kind::TEMPLATED_EXPR:      return this->analyze_expr_templated<IS_CONSTEXPR>(expr);
				case AST::Kind::PREFIX:              return this->analyze_expr_prefix<IS_CONSTEXPR>(expr);
				case AST::Kind::INFIX:               return this->analyze_expr_infix<IS_CONSTEXPR>(expr);
				case AST::Kind::POSTFIX:             return this->analyze_expr_postfix<IS_CONSTEXPR>(expr);
				case AST::Kind::NEW:                 return this->analyze_expr_new<IS_CONSTEXPR>(expr);
				case AST::Kind::ARRAY_INIT_NEW:      return this->analyze_expr_array_init_new<IS_CONSTEXPR>(expr);
				case AST::Kind::DESIGNATED_INIT_NEW: return this->analyze_expr_designated_init_new<IS_CONSTEXPR>(expr);
				case AST::Kind::TRY_ELSE:            return this->analyze_expr_try_else<IS_CONSTEXPR>(expr);
				case AST::Kind::IDENT:               return this->analyze_expr_ident<IS_CONSTEXPR>(expr);
				case AST::Kind::INTRINSIC:           return this->analyze_expr_intrinsic(expr);
				case AST::Kind::LITERAL:             return this->analyze_expr_literal(ast_buffer.getLiteral(expr));
				case AST::Kind::UNINIT:              return this->analyze_expr_uninit(ast_buffer.getUninit(expr));
				case AST::Kind::ZEROINIT:            return this->analyze_expr_zeroinit(ast_buffer.getZeroinit(expr));
				case AST::Kind::THIS:                return this->analyze_expr_this(ast_buffer.getThis(expr));

				case AST::Kind::DEDUCER: {
					evo::debugFatalBreak("Type deducer should not be allowed in this context");
				} break;

				case AST::Kind::ARRAY_TYPE: {
					const AST::ArrayType& array_type = ast_buffer.getArrayType(expr);

					const evo::Result<SymbolProc::TypeID> elem_type = this->analyze_type<true>(
						this->source.getASTBuffer().getType(array_type.elemType)
					);
					if(elem_type.isError()){ return evo::resultError; }


					if(array_type.refIsMut.has_value()){ // array ref
						auto dimensions = evo::SmallVector<std::optional<SymbolProcTermInfoID>>();
						dimensions.reserve(array_type.dimensions.size());
						for(const std::optional<AST::Node>& dimension : array_type.dimensions){
							if(dimension.has_value()){
								const evo::Result<SymbolProc::TermInfoID> dimension_term_info =
									this->analyze_expr<true>(*dimension);

								if(dimension_term_info.isError()){ return evo::resultError; }
								dimensions.emplace_back(dimension_term_info.value());

							}else{
								dimensions.emplace_back();
							}
						}

						auto terminator = std::optional<SymbolProc::TermInfoID>();
						if(array_type.terminator.has_value()){
							const evo::Result<SymbolProc::TermInfoID> terminator_info = 
								this->analyze_expr<true>(*array_type.terminator);
							if(terminator_info.isError()){ return evo::resultError; }

							terminator = terminator_info.value();
						}

						const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
						this->add_instruction(
							this->context.symbol_proc_manager.createArrayRef(
								array_type, elem_type.value(), std::move(dimensions), terminator, new_term_info_id
							)
						);
						return new_term_info_id;

					}else{ // array
						auto dimensions = evo::SmallVector<SymbolProc::TermInfoID>();
						dimensions.reserve(array_type.dimensions.size());
						for(const std::optional<AST::Node>& dimension : array_type.dimensions){
							const evo::Result<SymbolProc::TermInfoID> dimension_term_info =
								this->analyze_expr<true>(*dimension);

							if(dimension_term_info.isError()){ return evo::resultError; }
							dimensions.emplace_back(dimension_term_info.value());
						}

						auto terminator = std::optional<SymbolProc::TermInfoID>();
						if(array_type.terminator.has_value()){
							const evo::Result<SymbolProc::TermInfoID> terminator_info = 
								this->analyze_expr<true>(*array_type.terminator);
							if(terminator_info.isError()){ return evo::resultError; }

							terminator = terminator_info.value();
						}

						const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
						this->add_instruction(
							this->context.symbol_proc_manager.createArrayType(
								array_type, elem_type.value(), std::move(dimensions), terminator, new_term_info_id
							)
						);
						return new_term_info_id;
					}
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

						this->add_instruction(
							this->context.symbol_proc_manager.createTypeToTerm(type_id.value(), new_term_info_id)
						);
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
							this->context.symbol_proc_manager.createTypeIDConverter(
								type_id_converter, target_type_id.value(), created_base_type_type
							)
						);
						return created_base_type_type;
					}
				} break;

				case AST::Kind::VAR_DEF:         case AST::Kind::FUNC_DEF:      case AST::Kind::DELETED_SPECIAL_METHOD:
				case AST::Kind::ALIAS_DEF:       case AST::Kind::STRUCT_DEF:    case AST::Kind::UNION_DEF:
				case AST::Kind::ENUM_DEF:        case AST::Kind::INTERFACE_DEF: case AST::Kind::INTERFACE_IMPL:
				case AST::Kind::RETURN:          case AST::Kind::ERROR:         case AST::Kind::UNREACHABLE:
				case AST::Kind::BREAK:           case AST::Kind::CONTINUE:      case AST::Kind::CONDITIONAL:
				case AST::Kind::WHEN_CONDITIONAL:case AST::Kind::WHILE:         case AST::Kind::DEFER:
				case AST::Kind::TEMPLATE_PACK:   case AST::Kind::MULTI_ASSIGN:  case AST::Kind::ATTRIBUTE_BLOCK:
				case AST::Kind::ATTRIBUTE:       case AST::Kind::PRIMITIVE_TYPE:case AST::Kind::DISCARD: {
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

			this->add_instruction(
				this->context.symbol_proc_manager.createBeginExprBlock(block, *block.label, std::move(output_types))
			);

			for(const AST::Node& stmt : block.statements){
				if(this->analyze_stmt(stmt).isError()){ return evo::resultError; }
			}

			const SymbolProc::TermInfoID output_term_info = this->create_term_info();
			this->add_instruction(this->context.symbol_proc_manager.createEndExprBlock(block, output_term_info));
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
							Diagnostic::Code::SYMBOL_PROC_INTRINSIC_FUNC_WRONG_NUM_ARGS,
							intrin_tok_id,
							"Calls to @import requires a path"
						);
						return evo::resultError;
					}

					if(func_call.args.size() > 1){
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_INTRINSIC_FUNC_WRONG_NUM_ARGS,
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
					this->add_instruction(
						this->context.symbol_proc_manager.createImportPanther(
							func_call, path_value.value(), new_term_info_id
						)
					);
					return new_term_info_id;
				}

			}else if(this->source.getTokenBuffer()[intrin_tok_id].getString() == "importC"){
				if constexpr(ERRORS){
					this->emit_error(
						Diagnostic::Code::SEMA_IMPORT_DOESNT_ERROR,
						intrin_tok_id,
						"Intrinsic @importC does not error"
					);
					return evo::resultError;
				}else{
					if(func_call.args.empty()){
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_INTRINSIC_FUNC_WRONG_NUM_ARGS,
							intrin_tok_id,
							"Calls to @importC requires a path"
						);
						return evo::resultError;
					}

					if(func_call.args.size() > 1){
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_INTRINSIC_FUNC_WRONG_NUM_ARGS,
							func_call.args[1].value,
							"Calls to @importC requires a path, and no other arguments"
						);
						return evo::resultError;
					}

					const evo::Result<SymbolProc::TermInfoID> path_value = this->analyze_expr<true>(
						func_call.args[0].value
					);
					if(path_value.isError()){ return evo::resultError; }

					const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
					this->add_instruction(
						this->context.symbol_proc_manager.createImportC(
							func_call, path_value.value(), new_term_info_id
						)
					);
					return new_term_info_id;
				}

			}else if(this->source.getTokenBuffer()[intrin_tok_id].getString() == "importCPP"){
				if constexpr(ERRORS){
					this->emit_error(
						Diagnostic::Code::SEMA_IMPORT_DOESNT_ERROR,
						intrin_tok_id,
						"Intrinsic @importCPP does not error"
					);
					return evo::resultError;
				}else{
					if(func_call.args.empty()){
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_INTRINSIC_FUNC_WRONG_NUM_ARGS,
							intrin_tok_id,
							"Calls to @importCPP requires a path"
						);
						return evo::resultError;
					}

					if(func_call.args.size() > 1){
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_INTRINSIC_FUNC_WRONG_NUM_ARGS,
							func_call.args[1].value,
							"Calls to @importCPP requires a path, and no other arguments"
						);
						return evo::resultError;
					}

					const evo::Result<SymbolProc::TermInfoID> path_value = this->analyze_expr<true>(
						func_call.args[0].value
					);
					if(path_value.isError()){ return evo::resultError; }

					const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
					this->add_instruction(
						this->context.symbol_proc_manager.createImportCPP(
							func_call, path_value.value(), new_term_info_id
						)
					);
					return new_term_info_id;
				}

			}else if(this->source.getTokenBuffer()[intrin_tok_id].getString() == "isMacroDefined"){
				if constexpr(ERRORS){
					this->emit_error(
						Diagnostic::Code::SEMA_IMPORT_DOESNT_ERROR,
						intrin_tok_id,
						"Intrinsic @isMacroDefined does not error"
					);
					return evo::resultError;
				}else{
					if(func_call.args.size() != 2){
						if(func_call.args.empty()){
							this->emit_error(
								Diagnostic::Code::SYMBOL_PROC_INTRINSIC_FUNC_WRONG_NUM_ARGS,
								intrin_tok_id,
								"Calls to @isMacroDefined a Clang Module and a macro name string"
							);

						}else if(func_call.args.size() == 1){
							this->emit_error(
								Diagnostic::Code::SYMBOL_PROC_INTRINSIC_FUNC_WRONG_NUM_ARGS,
								func_call,
								"Calls to @isMacroDefined requires a macro name string"
							);
						}else{
							this->emit_error(
								Diagnostic::Code::SYMBOL_PROC_INTRINSIC_FUNC_WRONG_NUM_ARGS,
								func_call,
								"Calls to @isMacroDefined requires a macro name string"
							);
						}
						return evo::resultError;
					}

					const evo::Result<SymbolProc::TermInfoID> module_value = this->analyze_expr<true>(
						func_call.args[0].value
					);
					if(module_value.isError()){ return evo::resultError; }

					const evo::Result<SymbolProc::TermInfoID> macro_name_value = this->analyze_expr<true>(
						func_call.args[1].value
					);
					if(macro_name_value.isError()){ return evo::resultError; }


					const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
					this->add_instruction(
						this->context.symbol_proc_manager.createIsMacroDefined(
							func_call, module_value.value(), macro_name_value.value(), new_term_info_id
						)
					);
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
				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createTemplateIntrinsicFuncCallConstexpr(
							func_call, std::move(template_args), std::move(args), target.value(), new_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createTemplateIntrinsicFuncCall(
							func_call, std::move(template_args), std::move(args), target.value(), new_term_info_id
						)
					);
				}

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
					this->context.symbol_proc_manager.createFuncCallExprConstexpr(
						func_call, std::move(template_args), args, target.value(), new_term_info_id
					)
				);

				const SymbolProc::TermInfoID comptime_res_term_info_id = this->create_term_info();
				this->add_instruction(
					this->context.symbol_proc_manager.createConstexprFuncCallRun(
						func_call, new_term_info_id, comptime_res_term_info_id, std::move(args)
					)
				);
				return comptime_res_term_info_id;
			}

		}else{
			if constexpr(ERRORS){
				this->add_instruction(
					this->context.symbol_proc_manager.createFuncCallExprErrors(
						func_call, std::move(template_args), std::move(args), target.value(), new_term_info_id
					)
				);
			}else{
				this->add_instruction(
					this->context.symbol_proc_manager.createFuncCallExpr(
						func_call, std::move(template_args), std::move(args), target.value(), new_term_info_id
					)
				);
			}

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
			this->context.symbol_proc_manager.createIndexerConstexpr(
				indexer, target.value(), new_term_info_id, std::move(indices)
			)
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
			this->context.symbol_proc_manager.createTemplatedTerm(
				templated_expr, base_type.value(), std::move(args), created_struct_inst_id
			)
		);

		this->add_instruction(
			this->context.symbol_proc_manager.createTemplatedTermWaitForDef(
				templated_expr, created_struct_inst_id, created_base_term_info_id
			)
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

				this->add_instruction(
					this->context.symbol_proc_manager.createAddrOf(prefix, target.value(), created_term_info_id)
				);

				return created_term_info_id;
			} break;

			case Token::Kind::KEYWORD_COPY: {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(target.isError()){ return evo::resultError; }

				this->add_instruction(
					this->context.symbol_proc_manager.createCopy(prefix, target.value(), created_term_info_id)
				);

				return created_term_info_id;
			} break;

			case Token::Kind::KEYWORD_MOVE: {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(target.isError()){ return evo::resultError; }

				this->add_instruction(
					this->context.symbol_proc_manager.createMove(prefix, target.value(), created_term_info_id)
				);

				return created_term_info_id;
			} break;

			case Token::Kind::KEYWORD_FORWARD: {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(target.isError()){ return evo::resultError; }

				this->add_instruction(
					this->context.symbol_proc_manager.createForward(prefix, target.value(), created_term_info_id)
				);

				return created_term_info_id;
			} break;

			case Token::lookupKind("-"): {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> expr = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(expr.isError()){ return evo::resultError; }

				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createPrefixNegateConstexpr(
							prefix, expr.value(), created_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createPrefixNegate(
							prefix, expr.value(), created_term_info_id
						)
					);
				}

				return created_term_info_id;
			} break;

			case Token::lookupKind("!"): {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> expr = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(expr.isError()){ return evo::resultError; }

				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createPrefixNotConstexpr(
							prefix, expr.value(), created_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createPrefixNot(prefix, expr.value(), created_term_info_id)
					);
				}

				return created_term_info_id;
			} break;

			case Token::lookupKind("~"): {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> expr = this->analyze_expr<IS_CONSTEXPR>(prefix.rhs);
				if(expr.isError()){ return evo::resultError; }

				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createPrefixBitwiseNotConstexpr(
							prefix, expr.value(), created_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createPrefixBitwiseNot(
							prefix, expr.value(), created_term_info_id
						)
					);
				}

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

				if(infix.rhs.kind() != AST::Kind::IDENT){
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_INVALID_RHS_OF_ACCESSOR,
						infix.rhs,
						"Invalid RHS of accessor operator"
					);
					return evo::resultError;
				}

				const Token::ID rhs = this->source.getASTBuffer().getIdent(infix.rhs);

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createAccessorNeedsDef(
							infix, lhs.value(), rhs, new_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createAccessor(infix, lhs.value(), rhs, new_term_info_id)
					);
				}
				return new_term_info_id;
			} break;

			case Token::Kind::KEYWORD_AS: {
				const evo::Result<SymbolProc::TermInfoID> expr = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(expr.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TypeID> target_type =
					this->analyze_type<true>(this->source.getASTBuffer().getType(infix.rhs));
				if(target_type.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createAsConstexpr(
							infix, expr.value(), target_type.value(), new_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createAs(
							infix, expr.value(), target_type.value(), new_term_info_id
						)
					);
				}
				return new_term_info_id;
			} break;

			case Token::lookupKind("||"): case Token::lookupKind("&&"): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<IS_CONSTEXPR>(infix.rhs);
				if(rhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixConstexprLogical(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixLogical(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}
				return new_term_info_id;
			} break;


			case Token::lookupKind("=="): case Token::lookupKind("!="): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const bool rhs_is_null = [&](){
					if(infix.rhs.kind() != AST::Kind::LITERAL){ return false; }
					const Token::ID literal_token = this->source.getASTBuffer().getLiteral(infix.rhs);
					return this->source.getTokenBuffer()[literal_token].kind() == Token::Kind::KEYWORD_NULL;
				}();

				if(rhs_is_null){
					const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
					this->add_instruction(
						this->context.symbol_proc_manager.createOptionalNullCheck(infix, lhs.value(), new_term_info_id)
					);
					return new_term_info_id;

				}else{
					const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<IS_CONSTEXPR>(infix.rhs);
					if(rhs.isError()){ return evo::resultError; }

					const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
					if constexpr(IS_CONSTEXPR){
						this->add_instruction(
							this->context.symbol_proc_manager.createMathInfixConstexprComparative(
								infix, lhs.value(), rhs.value(), new_term_info_id
							)
						);
					}else{
						this->add_instruction(
							this->context.symbol_proc_manager.createMathInfixComparative(
								infix, lhs.value(), rhs.value(), new_term_info_id
							)
						);
					}
					return new_term_info_id;
				}
			} break;

			case Token::lookupKind("<"): case Token::lookupKind("<="): case Token::lookupKind(">"):
			case Token::lookupKind(">="): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<IS_CONSTEXPR>(infix.rhs);
				if(rhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixConstexprComparative(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixComparative(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}
				return new_term_info_id;
			} break;

			case Token::lookupKind("&"):  case Token::lookupKind("|"): case Token::lookupKind("^"): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<IS_CONSTEXPR>(infix.rhs);
				if(rhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixConstexprBitwiseLogical(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixBitwiseLogical(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}
				return new_term_info_id;
			} break;

			case Token::lookupKind("+%"): case Token::lookupKind("+|"): case Token::lookupKind("-%"):
			case Token::lookupKind("-|"): case Token::lookupKind("*%"): case Token::lookupKind("*|"): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<IS_CONSTEXPR>(infix.rhs);
				if(rhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixConstexprIntegralMath(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixIntegralMath(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}
				return new_term_info_id;
			} break;

			case Token::lookupKind("+"): case Token::lookupKind("-"): case Token::lookupKind("*"):
			case Token::lookupKind("/"): case Token::lookupKind("%"): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<IS_CONSTEXPR>(infix.rhs);
				if(rhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixConstexprMath(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixMath(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}
				return new_term_info_id;
			} break;


			case Token::lookupKind("<<"): case Token::lookupKind("<<|"): case Token::lookupKind(">>"): {
				const evo::Result<SymbolProc::TermInfoID> lhs = this->analyze_expr<IS_CONSTEXPR>(infix.lhs);
				if(lhs.isError()){ return evo::resultError; }

				const evo::Result<SymbolProc::TermInfoID> rhs = this->analyze_expr<IS_CONSTEXPR>(infix.rhs);
				if(rhs.isError()){ return evo::resultError; }

				const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
				if constexpr(IS_CONSTEXPR){
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixConstexprShift(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}else{
					this->add_instruction(
						this->context.symbol_proc_manager.createMathInfixShift(
							infix, lhs.value(), rhs.value(), new_term_info_id
						)
					);
				}
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

				this->add_instruction(
					this->context.symbol_proc_manager.createDeref(postfix, target.value(), created_term_info_id)
				);

				return created_term_info_id;
			} break;

			case Token::lookupKind(".?"): {
				const SymbolProc::TermInfoID created_term_info_id = this->create_term_info();

				const evo::Result<SymbolProc::TermInfoID> target = this->analyze_expr<IS_CONSTEXPR>(postfix.lhs);
				if(target.isError()){ return evo::resultError; }

				this->add_instruction(
					this->context.symbol_proc_manager.createUnwrap(postfix, target.value(), created_term_info_id)
				);

				return created_term_info_id;
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported postfix operator");
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_new(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const AST::New& ast_new = this->source.getASTBuffer().getNew(node);

		const evo::Result<SymbolProc::TypeID> type_id = this->analyze_type<true>(
			this->source.getASTBuffer().getType(ast_new.type)
		);
		if(type_id.isError()){ return evo::resultError; }

		auto args = evo::SmallVector<SymbolProc::TermInfoID>();
		args.reserve(ast_new.args.size());
		for(const AST::FuncCall::Arg& arg : ast_new.args){
			const evo::Result<SymbolProc::TermInfoID> value_expr = this->analyze_expr<IS_CONSTEXPR>(arg.value);
			if(value_expr.isError()){ return evo::resultError; }

			args.emplace_back(value_expr.value());
		}

		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		if constexpr(IS_CONSTEXPR){
			this->add_instruction(
				this->context.symbol_proc_manager.createNewConstexpr(
					ast_new, type_id.value(), new_term_info_id, std::move(args)
				)
			);
		}else{
			this->add_instruction(
				this->context.symbol_proc_manager.createNew(
					ast_new, type_id.value(), new_term_info_id, std::move(args)
				)
			);
		}
		return new_term_info_id;
	}


	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_array_init_new(const AST::Node& node)
	-> evo::Result<SymbolProc::TermInfoID> {
		const AST::ArrayInitNew& array_init_new =  this->source.getASTBuffer().getArrayInitNew(node);

		const evo::Result<SymbolProc::TypeID> type_id = this->analyze_type<true>(
			this->source.getASTBuffer().getType(array_init_new.type)
		);
		if(type_id.isError()){ return evo::resultError; }

		auto values = evo::SmallVector<SymbolProc::TermInfoID>();
		values.reserve(array_init_new.values.size());
		for(const AST::Node& value : array_init_new.values){
			const evo::Result<SymbolProc::TermInfoID> value_expr = this->analyze_expr<IS_CONSTEXPR>(value);
			if(value_expr.isError()){ return evo::resultError; }

			values.emplace_back(value_expr.value());
		}

		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		if constexpr(IS_CONSTEXPR){
			this->add_instruction(
				this->context.symbol_proc_manager.createArrayInitNewConstexpr(
					array_init_new, type_id.value(), new_term_info_id, std::move(values)
				)
			);
		}else{
			this->add_instruction(
				this->context.symbol_proc_manager.createArrayInitNew(
					array_init_new, type_id.value(), new_term_info_id, std::move(values)
				)
			);
		}
		return new_term_info_id;
	}



	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_designated_init_new(const AST::Node& node)
	-> evo::Result<SymbolProc::TermInfoID> {
		const AST::DesignatedInitNew& designated_init_new =  this->source.getASTBuffer().getDesignatedInitNew(node);

		const evo::Result<SymbolProc::TypeID> type_id = this->analyze_type<true>(
			this->source.getASTBuffer().getType(designated_init_new.type)
		);
		if(type_id.isError()){ return evo::resultError; }

		auto member_inits = evo::SmallVector<SymbolProc::TermInfoID>();
		member_inits.reserve(designated_init_new.memberInits.size());
		for(const AST::DesignatedInitNew::MemberInit& member_init : designated_init_new.memberInits){
			const evo::Result<SymbolProc::TermInfoID> member_init_expr =
				this->analyze_expr<IS_CONSTEXPR>(member_init.expr);
			if(member_init_expr.isError()){ return evo::resultError; }

			member_inits.emplace_back(member_init_expr.value());
		}

		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		if constexpr(IS_CONSTEXPR){
			this->add_instruction(
				this->context.symbol_proc_manager.createDesignatedInitNewConstexpr(
					designated_init_new, type_id.value(), new_term_info_id, std::move(member_inits)
				)
			);
		}else{
			this->add_instruction(
				this->context.symbol_proc_manager.createDesignatedInitNew(
					designated_init_new, type_id.value(), new_term_info_id, std::move(member_inits)
				)
			);
		}
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
			this->context.symbol_proc_manager.createPrepareTryHandler(
				try_else.exceptParams, attempt_expr.value(), except_params_term_info_id, try_else.elseTokenID
			)
		);

		const evo::Result<SymbolProc::TermInfoID> except_expr =
			this->analyze_expr<IS_CONSTEXPR>(try_else.exceptExpr);
		if(except_expr.isError()){ return evo::resultError; }
		
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(
			this->context.symbol_proc_manager.createTryElseExpr(
				try_else, attempt_expr.value(), except_params_term_info_id, except_expr.value(), new_term_info_id
			)
		);
		return new_term_info_id;
	}

	template<bool IS_CONSTEXPR>
	auto SymbolProcBuilder::analyze_expr_ident(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		if constexpr(IS_CONSTEXPR){
			this->add_instruction(
				this->context.symbol_proc_manager.createIdentNeedsDef(
					this->source.getASTBuffer().getIdent(node), new_term_info_id
				)
			);
		}else{
			this->add_instruction(
				this->context.symbol_proc_manager.createIdent(
					this->source.getASTBuffer().getIdent(node), new_term_info_id
				)
			);
		}
		return new_term_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_intrinsic(const AST::Node& node) -> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(
			this->context.symbol_proc_manager.createIntrinsic(
				this->source.getASTBuffer().getIntrinsic(node), new_term_info_id
			)
		);
		return new_term_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_literal(const Token::ID& literal) -> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(this->context.symbol_proc_manager.createLiteral(literal, new_term_info_id));
		return new_term_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_uninit(const Token::ID& uninit_token) -> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(this->context.symbol_proc_manager.createUninit(uninit_token, new_term_info_id));
		return new_term_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_zeroinit(const Token::ID& zeroinit_token)
	-> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(this->context.symbol_proc_manager.createZeroinit(zeroinit_token, new_term_info_id));
		return new_term_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_this(const Token::ID& this_token) -> evo::Result<SymbolProc::TermInfoID> {
		const SymbolProc::TermInfoID new_term_info_id = this->create_term_info();
		this->add_instruction(this->context.symbol_proc_manager.createThis(this_token, new_term_info_id));
		return new_term_info_id;
	}



	auto SymbolProcBuilder::analyze_attributes(const AST::AttributeBlock& attribute_block)
	-> evo::Result<evo::SmallVector<Instruction::AttributeParams>> {
		auto attribute_params_info = evo::SmallVector<Instruction::AttributeParams>();

		for(const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			const std::string_view attribute_name = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_name == "builtin"){
				if(attribute.args.size() != 1){
					if(attribute.args.size() > 1){
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_ATTRIBUTE_BUILTIN_INVALID_ARGS,
							attribute.args[1],
							"Attribute `#builtin` only accepts 1 argument"
						);
						
					}else{
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_ATTRIBUTE_BUILTIN_INVALID_ARGS,
							attribute.attribute,
							"Attribute `#builtin` requires an argument"
						);
					}

					return evo::resultError;
				}

				if(attribute.args[0].kind() != AST::Kind::LITERAL){
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_ATTRIBUTE_BUILTIN_INVALID_ARGS,
						attribute.args[0],
						"Attribute `#builtin` requires a string argument"
					);
					return evo::resultError;
				}

				const Token::ID attribute_arg_token_id = ASTBuffer::getLiteral(attribute.args[0]);
				const Token& attribute_arg_token = this->source.getTokenBuffer()[attribute_arg_token_id];

				if(attribute_arg_token.kind() != Token::Kind::LITERAL_STRING){
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_ATTRIBUTE_BUILTIN_INVALID_ARGS,
						attribute.args[0],
						"Attribute `#builtin` requires a string argument"
					);
					return evo::resultError;
				}


				evo::Result<SymbolProc::BuiltinSymbolKind> lookup_symbol_kind =
					this->context.symbol_proc_manager.lookupBuiltinSymbolKind(attribute_arg_token.getString());

				if(lookup_symbol_kind.isError()){
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_ATTRIBUTE_BUILTIN_INVALID_ARGS,
						attribute.args[0],
						"Unknown builtin symbol kind"
					);
					return evo::resultError;
				}

				SymbolProc& current_symbol_proc = this->get_current_symbol().symbol_proc;
				current_symbol_proc.is_always_priority = true;
				current_symbol_proc.builtin_symbol_proc_kind = lookup_symbol_kind.value();

				const evo::Expected<void, SymbolProc::ID> set_builtin_symbol_result = 
					this->context.symbol_proc_manager.setBuiltinSymbol(
						lookup_symbol_kind.value(), this->get_current_symbol().symbol_proc_id, this->context
					);

				if(set_builtin_symbol_result.has_value() == false){
					const SymbolProc& previous_defined_symbol_proc =
						this->context.symbol_proc_manager.getSymbolProc(set_builtin_symbol_result.error());

					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_ATTRIBUTE_BUILTIN_INVALID_ARGS,
						attribute.args[0],
						"This builtin symbol kind was already defined",
						Diagnostic::Info(
							"First defined here:",
							Diagnostic::Location::get(previous_defined_symbol_proc.ast_node, this->source)
						)
					);
					return evo::resultError;
				}
			}


			attribute_params_info.emplace_back();

			for(const AST::Node& arg : attribute.args){
				const evo::Result<SymbolProc::TermInfoID> arg_expr = this->analyze_expr<true>(arg);
				if(arg_expr.isError()){ return evo::resultError; }

				attribute_params_info.back().emplace_back(arg_expr.value());
			}
		}

		return attribute_params_info;
	}


	auto SymbolProcBuilder::analyze_priority_and_builtin_attribute(const AST::AttributeBlock& attribute_block)
	-> evo::Result<> {
		for(const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "builtin"){
				if(attribute.args.size() != 1){
					if(attribute.args.empty()){
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_ATTRIBUTE_BUILTIN_INVALID_ARGS,
							attribute.attribute,
							"Attribute #builtin requires an argument"
						);

					}else{
						this->emit_error(
							Diagnostic::Code::SYMBOL_PROC_ATTRIBUTE_BUILTIN_INVALID_ARGS,
							attribute.args[1],
							"Attribute #builtin requires 1 argument"
						);
					}

					return evo::resultError;
				}

				if(attribute.args[0].kind() != AST::Kind::LITERAL){
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_ATTRIBUTE_BUILTIN_INVALID_ARGS,
						attribute.args[0],
						"Attribute #builtin requires a string argument"
					);
					return evo::resultError;
				}

				const Token::ID arg_id = ASTBuffer::getLiteral(attribute.args[0]);
				const Token& arg = this->source.getTokenBuffer()[arg_id];

				if(arg.kind() != Token::Kind::LITERAL_STRING){
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_ATTRIBUTE_BUILTIN_INVALID_ARGS,
						attribute.args[0],
						"Attribute #builtin requires a string argument"
					);
					return evo::resultError;
				}


				const evo::Result<SymbolProc::BuiltinSymbolKind> lookup_symbol_kind
					= this->context.symbol_proc_manager.lookupBuiltinSymbolKind(arg.getString());

				if(lookup_symbol_kind.isError()){
					this->emit_error(
						Diagnostic::Code::SYMBOL_PROC_ATTRIBUTE_BUILTIN_INVALID_ARGS,
						attribute.args[0],
						"Unknown builtin symbol kind"
					);
					return evo::resultError;
				}


				const SymbolProcInfo& current_symbol_proc = this->get_current_symbol();
				current_symbol_proc.symbol_proc.is_always_priority = true;
				current_symbol_proc.symbol_proc.builtin_symbol_proc_kind = lookup_symbol_kind.value();

				this->emit_error(
					Diagnostic::Code::MISC_UNIMPLEMENTED_FEATURE,
					attribute.args[0],
					"This builtin is currently unimplemented"
				);
				return evo::resultError;
			}
		}

		return evo::Result<>();
	}



	auto SymbolProcBuilder::analyze_template_param_pack(const AST::TemplatePack& template_pack)
	-> evo::Result<evo::SmallVector<SymbolProc::Instruction::TemplateParamInfo>> {
		const TokenBuffer& token_buffer = this->source.getTokenBuffer();
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		auto template_param_infos = evo::SmallVector<SymbolProc::Instruction::TemplateParamInfo>();

		this->add_instruction(this->context.symbol_proc_manager.createPushTemplateDeclInstantiationTypesScope());
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
				this->add_instruction(this->context.symbol_proc_manager.createAddTemplateDeclInstantiationType(ident));
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
		this->add_instruction(this->context.symbol_proc_manager.createPopTemplateDeclInstantiationTypesScope());

		return template_param_infos;
	}



	auto SymbolProcBuilder::is_deducer(const AST::Node& node) const -> bool {
		switch(node.kind()){
			case AST::Kind::TYPE: {
				const AST::Type& type = this->source.getASTBuffer().getType(node);
				return this->is_deducer(type.base);
			} break;

			case AST::Kind::DEDUCER: {
				return true;
			} break;

			case AST::Kind::ARRAY_TYPE: {
				const AST::ArrayType& array_type = this->source.getASTBuffer().getArrayType(node);

				if(this->is_deducer(this->source.getASTBuffer().getType(array_type.elemType).base)){ return true; }

				for(const std::optional<AST::Node>& dimension : array_type.dimensions){
					if(dimension.has_value() == false){ continue; }
					if(dimension->kind() == AST::Kind::DEDUCER){ return true; }
				}

				return array_type.terminator.has_value() && array_type.terminator->kind() == AST::Kind::DEDUCER;
			} break;

			case AST::Kind::TEMPLATED_EXPR: {
				const AST::TemplatedExpr& templated_expr = this->source.getASTBuffer().getTemplatedExpr(node);

				for(const AST::Node& arg : templated_expr.args){
					if(this->is_deducer(arg)){ return true; }
				}

				return false;
			} break;

			case AST::Kind::INTERFACE_MAP: {
				const AST::InterfaceMap& interface_map_type = this->source.getASTBuffer().getInterfaceMap(node);
				if(interface_map_type.underlyingType.is<Token::ID>()){ return false; }
				return this->is_deducer(interface_map_type.underlyingType.as<AST::Node>());
			} break;

			default: {
				return false;
			} break;
		}
	}


	auto SymbolProcBuilder::is_named_deducer(const AST::Node& node) const -> bool {
		switch(node.kind()){
			case AST::Kind::TYPE: {
				const AST::Type& type = this->source.getASTBuffer().getType(node);
				return this->is_named_deducer(type.base);
			} break;

			case AST::Kind::DEDUCER: {
				const Token& deducer_token = this->source.getTokenBuffer()[ASTBuffer::getDeducer(node)];
				return deducer_token.kind() == Token::Kind::DEDUCER;
			} break;

			case AST::Kind::ARRAY_TYPE: {
				const AST::ArrayType& array_type = this->source.getASTBuffer().getArrayType(node);

				if(this->is_named_deducer(this->source.getASTBuffer().getType(array_type.elemType).base)){
					return true;
				}

				for(const std::optional<AST::Node>& dimension : array_type.dimensions){
					if(dimension.has_value() == false){ continue; }
					if(this->is_named_deducer(*dimension)){ return true; }
				}

				return array_type.terminator.has_value() && this->is_named_deducer(*array_type.terminator);
			} break;

			case AST::Kind::TEMPLATED_EXPR: {
				const AST::TemplatedExpr& templated_expr = this->source.getASTBuffer().getTemplatedExpr(node);

				for(const AST::Node& arg : templated_expr.args){
					if(this->is_named_deducer(arg)){ return true; }
				}

				return false;
			} break;

			case AST::Kind::INTERFACE_MAP: {
				const AST::InterfaceMap& interface_map_type = this->source.getASTBuffer().getInterfaceMap(node);
				if(interface_map_type.underlyingType.is<Token::ID>()){ return false; }
				return this->is_named_deducer(interface_map_type.underlyingType.as<AST::Node>());
			} break;

			default: {
				return false;
			} break;
		}
	}


	auto SymbolProcBuilder::extract_deducer_names(const AST::Node& node) const -> evo::SmallVector<std::string_view> {
		auto output = evo::SmallVector<std::string_view>();

		switch(node.kind()){
			case AST::Kind::TYPE: {
				return this->extract_deducer_names(this->source.getASTBuffer().getType(node).base);
			} break;

			case AST::Kind::DEDUCER: {
				const Token::ID deducer_token_id = this->source.getASTBuffer().getDeducer(node);
				const Token& deducer_token = this->source.getTokenBuffer()[deducer_token_id];

				if(deducer_token.kind() == Token::Kind::DEDUCER){
					output.emplace_back(deducer_token.getString());
				}
			} break;

			case AST::Kind::ARRAY_TYPE: {
				const AST::ArrayType& array_type = this->source.getASTBuffer().getArrayType(node);
				evo::SmallVector<std::string_view> extracted = this->extract_deducer_names(
					this->source.getASTBuffer().getType(array_type.elemType).base
				);

				output.reserve(std::bit_ceil(output.size() + extracted.size()));
				for(const std::string_view& extracted_str : extracted){
					output.emplace_back(extracted_str);
				}

				for(const std::optional<AST::Node>& dimension : array_type.dimensions){
					if(dimension.has_value() == false){ continue; }

					extracted = this->extract_deducer_names(*dimension);
					output.reserve(std::bit_ceil(output.size() + extracted.size()));
					for(const std::string_view& extracted_str : extracted){
						output.emplace_back(extracted_str);
					}
				}

				if(array_type.terminator.has_value()){
					extracted = this->extract_deducer_names(*array_type.terminator);
					output.reserve(std::bit_ceil(output.size() + extracted.size()));
					for(const std::string_view& extracted_str : extracted){
						output.emplace_back(extracted_str);
					}
				}
			} break;

			case AST::Kind::TEMPLATED_EXPR: {
				const AST::TemplatedExpr& templated_expr = this->source.getASTBuffer().getTemplatedExpr(node);

				for(const AST::Node& arg : templated_expr.args){
					const evo::SmallVector<std::string_view> extracted = this->extract_deducer_names(arg);
					output.reserve(std::bit_ceil(output.size() + extracted.size()));
					for(const std::string_view& extracted_str : extracted){
						output.emplace_back(extracted_str);
					}
				}
			} break;

			default: break;
		}

		return output;
	}



}