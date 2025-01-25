////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SemanticAnalyzer.h"

#include "../../include/Context.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{


	SemanticAnalyzer::SemanticAnalyzer(Context& _context, deps::Node::ID _deps_node_id)
		: context(_context),
		deps_node_id(_deps_node_id),
		deps_node(this->context.deps_buffer[_deps_node_id]),
		source(this->context.getSourceManager()[this->context.deps_buffer[_deps_node_id].sourceID]),
		scope(this->context.sema_buffer.scope_manager.getScope(*this->source.sema_scope_id))
		{}

	
	auto SemanticAnalyzer::analyzeDecl() -> bool {
		evo::debugAssert(
			this->deps_node.declSemaStatus == deps::Node::SemaStatus::InQueue,
			"Incorrect status for node to analyze semantics of decl"
		);

		switch(this->deps_node.astNode.kind()){
			break; case AST::Kind::None: evo::debugFatalBreak("Invalid AST::Node");

			break; case AST::Kind::VarDecl:         if(this->analyze_decl_var() == false){       return false; }
			break; case AST::Kind::FuncDecl:        if(this->analyze_decl_func().isError()){     return false; }
			break; case AST::Kind::AliasDecl:       if(this->analyze_decl_alias().isError()){    return false; }
			break; case AST::Kind::TypedefDecl:     if(this->analyze_decl_typedef().isError()){  return false; }
			break; case AST::Kind::StructDecl:      if(this->analyze_decl_struct().isError()){   return false; }
			break; case AST::Kind::WhenConditional: if(this->analyze_decl_when_cond() == false){ return false; }

			break; default: evo::debugFatalBreak("Unsupported decl kind");
		}

		this->deps_node.declSemaStatus = deps::Node::SemaStatus::Done;

		this->context.trace("SemanticAnalyzer::analyzeDecl");

		if(this->deps_node.defSemaStatus == deps::Node::SemaStatus::ReadyWhenDeclDone){
			return this->analyzeDef();
		}else{
			return true;
		}
	}


	auto SemanticAnalyzer::analyzeDef() -> bool {
		evo::debugAssert(
			this->deps_node.defSemaStatus == deps::Node::SemaStatus::InQueue
			|| this->deps_node.defSemaStatus == deps::Node::SemaStatus::ReadyWhenDeclDone,
			"Incorrect status for node to analyze semantics of def"
		);

		// hack to prevent definition analysis from happening during decl
		// TODO: improve this somehow
		while(this->deps_node.declSemaStatus != deps::Node::SemaStatus::Done){
			if(this->context.hasHitFailCondition()){ return false; }
			std::this_thread::yield();
		}

		const Source::SemaID decl_sema_id = [&](){
			const auto lock = std::scoped_lock(this->source.symbol_map_lock);
			return this->source.symbol_map.at(this->deps_node.astNode);
		}();

		switch(this->deps_node.astNode.kind()){
			case AST::Kind::None: evo::debugFatalBreak("Invalid AST::Node");

			case AST::Kind::VarDecl: {
				if(this->analyze_def_var(decl_sema_id.as<sema::Var::ID>()) == false){ return false; }
			} break;

			case AST::Kind::FuncDecl: {
				if(this->analyze_def_func(decl_sema_id.as<sema::Func::ID>()) == false){ return false; }
			} break;

			case AST::Kind::AliasDecl: {
				if(this->analyze_def_alias(decl_sema_id.as<BaseType::Alias::ID>()) == false){ return false; }
			} break;

			case AST::Kind::TypedefDecl: {
				if(this->analyze_def_typedef(decl_sema_id.as<BaseType::Typedef::ID>()) == false){ return false; }
			} break;

			case AST::Kind::StructDecl: {
				if(this->analyze_def_struct(decl_sema_id.as<sema::Struct::ID>()) == false){ return false; }
			} break;

			case AST::Kind::WhenConditional: evo::debugFatalBreak("Should never analyze def of when cond");
			
			default: evo::debugFatalBreak("Unsupported decl kind");
		}

		for(const deps::Node::ID& required_by_id : this->deps_node.requiredBy){
			this->propogate_to_required_by<false>(required_by_id);
		}
		this->deps_node.defSemaStatus = deps::Node::SemaStatus::Done;

		this->context.trace("SemanticAnalyzer::analyzeDef");
		this->context.deps_buffer.num_nodes_sema_status_not_done -= 1;
		return true;
	}


	auto SemanticAnalyzer::analyzeDeclDef() -> bool {
		evo::debugAssert(
			this->deps_node.declSemaStatus == deps::Node::SemaStatus::InQueue,
			"Incorrect status for node to analyze semantics of decl"
		);

		evo::debugAssert(
			this->deps_node.defSemaStatus == deps::Node::SemaStatus::InQueue,
			"Incorrect status for node to analyze semantics of def"
		);
			

		switch(this->deps_node.astNode.kind()){
			case AST::Kind::None: evo::debugFatalBreak("Invalid AST::Node");

			case AST::Kind::VarDecl: {
				if(this->analyze_decl_def_var() == false){ return false; }
			} break;

			case AST::Kind::FuncDecl: {
				const evo::Result<sema::Func::ID> decl_res = this->analyze_decl_func();
				if(decl_res.isError()){ return false; }
				if(this->analyze_def_func(decl_res.value()) == false){ return false; }
			} break;

			case AST::Kind::AliasDecl: {
				const evo::Result<BaseType::Alias::ID> decl_res = this->analyze_decl_alias();
				if(decl_res.isError()){ return false; }
				if(this->analyze_def_alias(decl_res.value()) == false){ return false; }
			} break;

			case AST::Kind::TypedefDecl: {
				const evo::Result<BaseType::Typedef::ID> decl_res = this->analyze_decl_typedef();
				if(decl_res.isError()){ return false; }
				if(this->analyze_def_typedef(decl_res.value()) == false){ return false; }
			} break;

			case AST::Kind::StructDecl: {
				const evo::Result<sema::Struct::ID> decl_res = this->analyze_decl_struct();
				if(decl_res.isError()){ return false; }
				if(this->analyze_def_struct(decl_res.value()) == false){ return false; }
			} break;

			case AST::Kind::WhenConditional: {
				evo::debugFatalBreak("Should never analyze def of when cond");
			} break;

			default: evo::debugFatalBreak("Unsupported decl kind");
		}


		for(const deps::Node::ID& required_by_id : this->deps_node.requiredBy){
			this->propogate_to_required_by<false>(required_by_id);
		}
		this->deps_node.declSemaStatus = deps::Node::SemaStatus::Done;
		this->deps_node.defSemaStatus = deps::Node::SemaStatus::Done;

		this->context.trace("SemanticAnalyzer::analyzeDeclDef");
		this->context.deps_buffer.num_nodes_sema_status_not_done -= 1;
		return true;
	}



	template<bool PROP_FOR_DECLS>
	auto SemanticAnalyzer::propogate_to_required_by(deps::Node::ID required_by_id) -> void {
		deps::Node& required_by = this->context.deps_buffer[required_by_id];


		const auto lock = std::scoped_lock(required_by.lock);

		if(required_by.defSemaStatus != deps::Node::SemaStatus::NotYet){ return; }

		if constexpr(PROP_FOR_DECLS){
			required_by.defDeps.decls.erase(this->deps_node_id);
		}else{
			required_by.defDeps.defs.erase(this->deps_node_id);
		}

		if(required_by.declSemaStatus != deps::Node::SemaStatus::NotYet){
			if(required_by.hasNoDefDeps()){
				if(required_by.declSemaStatus == deps::Node::SemaStatus::InQueue){
					required_by.defSemaStatus = deps::Node::SemaStatus::ReadyWhenDeclDone;
				}else{
					required_by.defSemaStatus = deps::Node::SemaStatus::InQueue;
					this->context.add_task_to_work_manager(Context::SemaDef(required_by_id));
				}
			}

		}else{
			if constexpr(PROP_FOR_DECLS){
				required_by.declDeps.decls.erase(this->deps_node_id);
			}else{
				required_by.declDeps.defs.erase(this->deps_node_id);
			}

			if(required_by.hasNoDeclDeps()){
				if(required_by.hasNoDeclDeps()){
					if(required_by.hasNoDefDeps() && required_by.astNode.kind() != AST::Kind::WhenConditional){
						required_by.declSemaStatus = deps::Node::SemaStatus::InQueue;
						required_by.defSemaStatus = deps::Node::SemaStatus::InQueue;
						this->context.add_task_to_work_manager(Context::SemaDeclDef(required_by_id));
					}else{
						required_by.declSemaStatus = deps::Node::SemaStatus::InQueue;
						this->context.add_task_to_work_manager(Context::SemaDecl(required_by_id));
					}
				}
			}
		}
	}




	auto SemanticAnalyzer::analyze_decl_var() -> bool {
		evo::debugAssert(this->deps_node.astNode.kind() == AST::Kind::VarDecl, "Incorrect AST::Kind");

		const evo::Result<DeclVarImplData> decl_data = this->analyze_decl_var_impl();
		if(decl_data.isError()){ return false; }


		///////////////////////////////////
		// done

		const sema::Var::ID sema_var_id = this->context.sema_buffer.createVar(
			decl_data.value().ast_var_decl.kind,
			decl_data.value().ast_var_decl.ident,
			std::nullopt,
			decl_data.value().type_id,
			decl_data.value().is_pub
		);

		this->get_current_scope_level().setVar(decl_data.value().var_ident, sema_var_id);

		{
			const auto lock = std::scoped_lock(this->source.symbol_map_lock);
			this->source.symbol_map.emplace(this->deps_node.astNode, sema_var_id);
		}

		for(const deps::Node::ID& required_by_id : this->deps_node.requiredBy){
			this->propogate_to_required_by<true>(required_by_id);
		}
		return true;
	}

	auto SemanticAnalyzer::analyze_decl_func() -> evo::Result<sema::Func::ID> {
		evo::debugAssert(this->deps_node.astNode.kind() == AST::Kind::FuncDecl, "Incorrect AST::Kind");

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->deps_node.astNode,
			"func declarations are unimplemented"
		);
		return evo::resultError;

		// for(const deps::Node::ID& required_by_id : this->deps_node.requiredBy){
		// 	this->propogate_to_required_by<true>(required_by_id);
		// }
		// return true;
	}

	auto SemanticAnalyzer::analyze_decl_alias() -> evo::Result<BaseType::Alias::ID> {
		evo::debugAssert(this->deps_node.astNode.kind() == AST::Kind::AliasDecl, "Incorrect AST::Kind");

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->deps_node.astNode,
			"alias declarations are unimplemented"
		);
		return evo::resultError;

		// for(const deps::Node::ID& required_by_id : this->deps_node.requiredBy){
		// 	this->propogate_to_required_by<true>(required_by_id);
		// }
		// return true;
	}

	auto SemanticAnalyzer::analyze_decl_typedef() -> evo::Result<BaseType::Typedef::ID> {
		evo::debugAssert(this->deps_node.astNode.kind() == AST::Kind::TypedefDecl, "Incorrect AST::Kind");

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->deps_node.astNode,
			"typedef declarations are unimplemented"
		);
		return evo::resultError;

		// for(const deps::Node::ID& required_by_id : this->deps_node.requiredBy){
		// 	this->propogate_to_required_by<true>(required_by_id);
		// }
		// return true;
	}

	auto SemanticAnalyzer::analyze_decl_struct() -> evo::Result<sema::Struct::ID> {
		evo::debugAssert(this->deps_node.astNode.kind() == AST::Kind::StructDecl, "Incorrect AST::Kind");

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->deps_node.astNode,
			"struct declarations are unimplemented"
		);
		return evo::resultError;

		// for(const deps::Node::ID& required_by_id : this->deps_node.requiredBy){
		// 	this->propogate_to_required_by<true>(required_by_id);
		// }
		// return true;
	}

	auto SemanticAnalyzer::analyze_decl_when_cond() -> bool {
		evo::debugAssert(this->deps_node.astNode.kind() == AST::Kind::WhenConditional, "Incorrect AST::Kind");

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->deps_node.astNode,
			"when declarations are unimplemented"
		);
		return false;

		// TODO: only propogate to actually used block
		// for(const deps::Node::ID& required_by_id : this->deps_node.requiredBy){
		// 	this->propogate_to_required_by<true>(required_by_id);
		// }
		// this->context.deps_buffer.num_nodes_sema_status_not_done -= 1;
		// return true;
	}





	auto SemanticAnalyzer::analyze_def_var(const sema::Var::ID& sema_var_id) -> bool {
		const AST::VarDecl& ast_var_decl = this->source.getASTBuffer().getVarDecl(this->deps_node.astNode);
		evo::debugAssert(ast_var_decl.type.has_value(), "A var with inferred type must be analyzed through DeclDef");

		sema::Var& sema_var = this->context.sema_buffer.vars[sema_var_id];

		const evo::Result<sema::Expr> expr_res = this->analyze_def_var_impl(ast_var_decl, sema_var.typeID);
		if(expr_res.isError()){ return false; }

		sema_var.expr = expr_res.value();
		return true;
	}

	auto SemanticAnalyzer::analyze_def_func(const sema::Func::ID& sema_func_id) -> bool {
		std::ignore = sema_func_id;

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->deps_node.astNode,
			"func definitions are unimplemented"
		);
		return false;
	}

	auto SemanticAnalyzer::analyze_def_alias(const BaseType::Alias::ID& sema_alias_id) -> bool {
		std::ignore = sema_alias_id;

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->deps_node.astNode,
			"alias definitions are unimplemented"
		);
		return false;
	}

	auto SemanticAnalyzer::analyze_def_typedef(const BaseType::Typedef::ID& sema_typedef_id) -> bool {
		std::ignore = sema_typedef_id;

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->deps_node.astNode,
			"typedef definitions are unimplemented"
		);
		return false;
	}

	auto SemanticAnalyzer::analyze_def_struct(const sema::Struct::ID& sema_struct_id) -> bool {
		std::ignore = sema_struct_id;

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			this->deps_node.astNode,
			"struct definitions are unimplemented"
		);
		return false;
	}




	auto SemanticAnalyzer::analyze_decl_def_var() -> bool {
		evo::Result<DeclVarImplData> decl_data = this->analyze_decl_var_impl();
		if(decl_data.isError()){ return false; }
		

		///////////////////////////////////
		// value

		const evo::Result<sema::Expr> expr_res = this->analyze_def_var_impl(
			decl_data.value().ast_var_decl, decl_data.value().type_id
		);
		if(expr_res.isError()){ return false; }


		///////////////////////////////////
		// done

		const sema::Var::ID sema_var_id = this->context.sema_buffer.createVar(
			decl_data.value().ast_var_decl.kind,
			decl_data.value().ast_var_decl.ident,
			expr_res.value(),
			decl_data.value().type_id,
			decl_data.value().is_pub
		);


		this->get_current_scope_level().setVar(decl_data.value().var_ident, sema_var_id);

		{
			const auto lock = std::scoped_lock(this->source.symbol_map_lock);
			this->source.symbol_map.emplace(this->deps_node.astNode, sema_var_id);
		}

		return true;
	}




	auto SemanticAnalyzer::analyze_decl_var_impl() -> evo::Result<DeclVarImplData> {
		const AST::VarDecl& ast_var_decl = this->source.getASTBuffer().getVarDecl(this->deps_node.astNode);

		///////////////////////////////////
		// check ident

		const std::string_view var_ident = this->source.getTokenBuffer()[ast_var_decl.ident].getString();
		if(this->add_ident_to_scope<true, false>(var_ident, ast_var_decl.ident) == false){ return evo::resultError; }


		///////////////////////////////////
		// type checking

		auto type_id = std::optional<TypeInfo::ID>();

		if(ast_var_decl.type.has_value()){
			const evo::Result<TypeInfo::VoidableID> type_id_res = this->get_type_id(
				this->source.getASTBuffer().getType(*ast_var_decl.type)
			);
			if(type_id_res.isError()){ return evo::resultError; }

			if(type_id_res.value().isVoid()){
				this->emit_error(
					Diagnostic::Code::SemaVarTypeVoid,
					*ast_var_decl.type,
					"Variables cannot be type `Void`"
				);
				return evo::resultError;
			}

			type_id = type_id_res.value().asTypeID();
		}


		///////////////////////////////////
		// attributes

		const AST::AttributeBlock& attr_block = 
			this->source.getASTBuffer().getAttributeBlock(ast_var_decl.attributeBlock);
		// for(const AST::AttributeBlock::Attribute& attribute : attr_block.attributes){
			
		// }
		if(attr_block.attributes.empty() == false){
			this->emit_error(
				Diagnostic::Code::MiscUnimplementedFeature,
				attr_block.attributes.front(),
				"var attributes are unimplemented"
			);
			return evo::resultError;
		}


		///////////////////////////////////
		// done

		return DeclVarImplData(ast_var_decl, var_ident, type_id, false);
	}



	auto SemanticAnalyzer::analyze_def_var_impl(
		const AST::VarDecl& ast_var_decl, std::optional<TypeInfo::ID>& type_id
	) -> evo::Result<sema::Expr> {
		if(ast_var_decl.value.has_value() == false){
			this->emit_error(
				Diagnostic::Code::SemaVarWithNoValue, ast_var_decl, "Variables must be declared with a value"
			);
			return evo::resultError;
		}

		evo::Result<ExprInfo> value_expr_info = this->analyze_expr<true>(*ast_var_decl.value);
		if(value_expr_info.isError()){ return evo::resultError; }

		if(value_expr_info.value().is_ephemeral() == false){
			this->emit_error(
				Diagnostic::Code::SemaVarDefNotEphemeral,
				*ast_var_decl.value,
				"Cannot define a variable with a non-ephemeral value"
			);
			return evo::resultError;
		}

		if(type_id.has_value()){
			if(this->type_check<true>(
				*type_id, value_expr_info.value(), "Variable definition", *ast_var_decl.value
			).ok == false){
				return evo::resultError;
			}
			
		}else{
			if(value_expr_info.value().isMultiValue()){
				this->emit_error(
					Diagnostic::Code::SemaMultiReturnIntoSingleValue,
					*ast_var_decl.value,
					"Cannot define a variable with multiple values"
				);
				return evo::resultError;
			}
		}

		return value_expr_info.value().getExpr();
	}




	//////////////////////////////////////////////////////////////////////
	// types

	auto SemanticAnalyzer::get_type_id(const AST::Type& ast_type) -> evo::Result<TypeInfo::VoidableID> {
		auto base_type = std::optional<BaseType::ID>();
		auto qualifiers = evo::SmallVector<AST::Type::Qualifier>();

		bool should_error_invalid_type_qualifiers = true;

		switch(ast_type.base.kind()){
			case AST::Kind::PrimitiveType: {
				const Token::ID primitive_type_token_id = ASTBuffer::getPrimitiveType(ast_type.base);
				const Token& primitive_type_token = this->source.getTokenBuffer()[primitive_type_token_id];

				switch(primitive_type_token.kind()){
					case Token::Kind::TypeVoid: {
						if(ast_type.qualifiers.empty() == false){
							this->emit_error(
								Diagnostic::Code::SemaVoidWithQualifiers,
								ast_type.base,
								"Type \"Void\" cannot have qualifiers"
							);
							return evo::resultError;
						}
						return TypeInfo::VoidableID::Void();
					} break;

					case Token::Kind::TypeThis:      case Token::Kind::TypeInt:        case Token::Kind::TypeISize:
					case Token::Kind::TypeUInt:      case Token::Kind::TypeUSize:      case Token::Kind::TypeF16:
					case Token::Kind::TypeBF16:      case Token::Kind::TypeF32:        case Token::Kind::TypeF64:
					case Token::Kind::TypeF80:       case Token::Kind::TypeF128:       case Token::Kind::TypeByte:
					case Token::Kind::TypeBool:      case Token::Kind::TypeChar:       case Token::Kind::TypeRawPtr:
					case Token::Kind::TypeTypeID:    case Token::Kind::TypeCShort:     case Token::Kind::TypeCUShort:
					case Token::Kind::TypeCInt:      case Token::Kind::TypeCUInt:      case Token::Kind::TypeCLong:
					case Token::Kind::TypeCULong:    case Token::Kind::TypeCLongLong:  case Token::Kind::TypeCULongLong:
					case Token::Kind::TypeCLongDouble: {
						base_type =this->context.type_manager.getOrCreatePrimitiveBaseType(primitive_type_token.kind());
					} break;

					case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N: {
						base_type = this->context.type_manager.getOrCreatePrimitiveBaseType(
							primitive_type_token.kind(), primitive_type_token.getBitWidth()
						);
					} break;


					case Token::Kind::TypeType: {
						this->emit_error(
							Diagnostic::Code::SemaGenericTypeNotInTemplatePackDecl,
							ast_type,
							"Type \"Type\" may only be used in a template pack declaration"
						);
						return evo::resultError;
					} break;

					default: {
						evo::debugFatalBreak("Unknown or unsupported PrimitiveType: {}", primitive_type_token.kind());
					} break;
				}
			} break;

			case AST::Kind::Ident: {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ast_type.base,
					"non-primitive are unimplemented"
				);
				return evo::resultError;
			} break;

			case AST::Kind::Infix: {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ast_type.base,
					"non-primitive types are unimplemented"
				);
				return evo::resultError;
			} break;

			case AST::Kind::TypeIDConverter: {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ast_type.base,
					"Type ID converters are unimplemented"
				);
				return evo::resultError;
			} break;

			// TODO: separate out into more kinds to be more specific (errors vs fatal)
			default: {
				this->emit_error(
					Diagnostic::Code::SemaInvalidBaseType, ast_type.base, "Invalid base type"
				);
				return evo::resultError;
			} break;
		}


		evo::debugAssert(base_type.has_value(), "base type was not set");

		for(const AST::Type::Qualifier& qualifier : ast_type.qualifiers){
			qualifiers.emplace_back(qualifier);
		}

		if(should_error_invalid_type_qualifiers){
			if(this->check_type_qualifiers(qualifiers, ast_type) == false){ return evo::resultError; }

		}else{
			bool found_read_only_ptr = false;
			for(auto iter = qualifiers.rbegin(); iter != qualifiers.rend(); ++iter){
				if(found_read_only_ptr){
					if(iter->isPtr){ iter->isReadOnly = true; }
				}else{
					if(iter->isPtr && iter->isReadOnly){ found_read_only_ptr = true; }
				}
			}
		}

		return TypeInfo::VoidableID(
			this->context.type_manager.getOrCreateTypeInfo(TypeInfo(*base_type, std::move(qualifiers)))
		);
	}


	//////////////////////////////////////////////////////////////////////
	// expr

	template<bool IS_COMPTIME>
	auto SemanticAnalyzer::analyze_expr(const AST::Node& node) -> evo::Result<ExprInfo> {
		switch(node.kind()){
			case AST::Kind::None: {
				evo::debugFatalBreak("Invalid AST::Node");
			} break;

			case AST::Kind::Block: {
				return this->analyze_expr_block(this->source.getASTBuffer().getBlock(node));
			} break;
			
			case AST::Kind::FuncCall: {
				return this->analyze_expr_func_call<IS_COMPTIME>(this->source.getASTBuffer().getFuncCall(node));
			} break;
			
			case AST::Kind::TemplatedExpr: {
				return this->analyze_expr_templated_expr(this->source.getASTBuffer().getTemplatedExpr(node));
			} break;
			
			case AST::Kind::Prefix: {
				return this->analyze_expr_prefix(this->source.getASTBuffer().getPrefix(node));
			} break;
			
			case AST::Kind::Infix: {
				return this->analyze_expr_infix(this->source.getASTBuffer().getInfix(node));
			} break;
			
			case AST::Kind::Postfix: {
				return this->analyze_expr_postfix(this->source.getASTBuffer().getPostfix(node));
			} break;

			case AST::Kind::New: {
				return this->analyze_expr_new(this->source.getASTBuffer().getNew(node));
			} break;
			
			case AST::Kind::Ident: {
				return this->analyze_expr_ident(this->source.getASTBuffer().getIdent(node));
			} break;
			
			case AST::Kind::Intrinsic: {
				return this->analyze_expr_intrinsic(this->source.getASTBuffer().getIntrinsic(node));
			} break;
			
			case AST::Kind::Literal: {
				return this->analyze_expr_literal(this->source.getASTBuffer().getLiteral(node));
			} break;
			
			case AST::Kind::Uninit: {
				return this->analyze_expr_uninit(this->source.getASTBuffer().getUninit(node));
			} break;

			case AST::Kind::Zeroinit: {
				return this->analyze_expr_zeroinit(this->source.getASTBuffer().getZeroinit(node));
			} break;
			
			case AST::Kind::This: {
				return this->analyze_expr_this(this->source.getASTBuffer().getThis(node));
			} break;

			case AST::Kind::VarDecl:     case AST::Kind::FuncDecl:        case AST::Kind::AliasDecl:
			case AST::Kind::TypedefDecl: case AST::Kind::StructDecl:      case AST::Kind::Return:
			case AST::Kind::Conditional: case AST::Kind::WhenConditional: case AST::Kind::While:
			case AST::Kind::Unreachable: case AST::Kind::TemplatePack:    case AST::Kind::MultiAssign:
			case AST::Kind::Type:        case AST::Kind::TypeIDConverter: case AST::Kind::AttributeBlock:
			case AST::Kind::Attribute:   case AST::Kind::PrimitiveType:   case AST::Kind::Discard: {
				// TODO: better messaging (specify what kind)
				this->emit_fatal(
					Diagnostic::Code::SemaInvalidExprKind,
					Diagnostic::Location::NONE,
					Diagnostic::createFatalMessage("Encountered expr of invalid AST kind")
				);
				return evo::resultError; 
			} break;
		}

		evo::unreachable();
	}




	
	auto SemanticAnalyzer::analyze_expr_block(const AST::Block& block) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			block,
			"block expressions are unimplemented"
		);
		return evo::resultError;
	}


	template<bool IS_COMPTIME>
	auto SemanticAnalyzer::analyze_expr_func_call(const AST::FuncCall& func_call) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			func_call,
			"function call expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	auto SemanticAnalyzer::analyze_import(const AST::FuncCall& func_call) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			func_call,
			"import expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	auto SemanticAnalyzer::analyze_expr_templated_expr(const AST::TemplatedExpr& templated_expr)
	-> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			templated_expr,
			"templated expressions are unimplemented"
		);
		return evo::resultError;
	}

	

	// auto SemanticAnalyzer::analyze_expr_templated_intrinsic(
	// 	const AST::TemplatedExpr& templated_expr, TemplatedIntrinsic::Kind templated_intrinsic_kind
	// ) -> evo::Result<ExprInfo> {
	// 	evo::unimplemented();
	// }

	
	auto SemanticAnalyzer::analyze_expr_prefix(const AST::Prefix& prefix) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			prefix,
			"prefix expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	template<bool IS_CONST>
	auto SemanticAnalyzer::analyze_expr_prefix_address_of(const AST::Prefix& prefix, const ExprInfo& rhs_info)
	-> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			prefix,
			"prefix address-of expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	auto SemanticAnalyzer::analyze_expr_infix(const AST::Infix& infix) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			infix,
			"infix expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	auto SemanticAnalyzer::analyze_expr_postfix(const AST::Postfix& postfix) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			postfix,
			"postfix expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	auto SemanticAnalyzer::analyze_expr_new(const AST::New& new_expr) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			new_expr,
			"new expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	auto SemanticAnalyzer::analyze_expr_new_impl(const AST::New& new_expr, TypeInfo::VoidableID type_id) 
	-> evo::Result<ExprInfo> {
		std::ignore = type_id;
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			new_expr,
			"new expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	auto SemanticAnalyzer::analyze_expr_new_impl(const AST::New& new_expr, TypeInfo::ID type_id) 
	-> evo::Result<ExprInfo> {
		std::ignore = type_id;
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			new_expr,
			"new expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	auto SemanticAnalyzer::analyze_expr_ident(const Token::ID& ident) -> evo::Result<ExprInfo> {
		const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();

		for(size_t i = this->scope.size() - 1; sema::ScopeLevel::ID scope_level_id : this->scope){
			const evo::Result<std::optional<ExprInfo>> scope_level_lookup = 
				this->analyze_expr_ident_in_scope_level(
					ident,
					ident_str,
					scope_level_id,
					i >= this->scope.getCurrentObjectScopeIndex() || i == 0,
					i == 0
				);

			if(scope_level_lookup.isError()){ return evo::resultError; }
			if(scope_level_lookup.value().has_value()){ return *scope_level_lookup.value(); }

			i -= 1;
		}

		this->emit_error(
			Diagnostic::Code::SemaIdentNotInScope,
			ident,
			std::format("Identifier \"{}\" was not defined in this scope", ident_str)
		);
		return evo::resultError;
	}

	// TODO: does this need to be separate function
	auto SemanticAnalyzer::analyze_expr_ident_in_scope_level(
		const Token::ID& ident,
		std::string_view ident_str,
		sema::ScopeLevel::ID scope_level_id,
		bool variables_in_scope,
		bool is_global_scope
	) -> evo::Result<std::optional<ExprInfo>> {
		const sema::ScopeLevel& scope_level = this->context.sema_buffer.scope_manager.getLevel(scope_level_id);

		const sema::ScopeLevel::IdentID* ident_id_lookup = scope_level.lookupIdent(ident_str);
		if(ident_id_lookup == nullptr){ return std::optional<ExprInfo>(); }

		return ident_id_lookup->visit([&](const auto& ident_id) -> evo::Result<std::optional<ExprInfo>> {
			using IdentIDType = std::decay_t<decltype(ident_id)>;

			if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::IDNotReady>()){
				evo::debugFatalBreak(
					"Should never be accessing an ident that is not ready - maybe issue with dependency analysis"
				);

			}else if constexpr(std::is_same<IdentIDType, evo::SmallVector<sema::FuncID>>()){
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ident,
					"function identifiers are unimplemented"
				);
				return evo::resultError;

			}else if constexpr(std::is_same<IdentIDType, sema::TemplatedFuncID>()){
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ident,
					"templated function identifiers are unimplemented"
				);
				return evo::resultError;

			}else if constexpr(std::is_same<IdentIDType, sema::VarID>()){
				if(!variables_in_scope){
					// TODO: better messaging
					this->emit_error(
						Diagnostic::Code::SemaIdentNotInScope,
						ident,
						std::format("Identifier \"{}\" was not defined in this scope", ident_str),
						Diagnostic::Info(
							"Local variables, parameters, and members cannot be accessed inside a sub-object scope. "
							"Defined here:",
							this->get_location(ident_id)
						)
					);
					return evo::resultError;
				}


				const sema::Var& sema_var = this->context.getSemaBuffer().getVar(ident_id);


				using ValueCategory = ExprInfo::ValueCategory;
				using ValueStage = ExprInfo::ValueStage;

				switch(sema_var.kind){
					case AST::VarDecl::Kind::Var: {
						return std::optional<ExprInfo>(ExprInfo(
							ValueCategory::ConcreteMut,
							is_global_scope ? ValueStage::Runtime : ValueStage::Constexpr,
							*sema_var.typeID,
							sema::Expr(ident_id)
						));
					} break;

					case AST::VarDecl::Kind::Const: {
						return std::optional<ExprInfo>(ExprInfo(
							ValueCategory::ConcreteConst, ValueStage::Constexpr, *sema_var.typeID, sema::Expr(ident_id)
						));
					} break;

					case AST::VarDecl::Kind::Def: {
						if(sema_var.typeID.has_value()){
							return std::optional<ExprInfo>(ExprInfo(
								ValueCategory::Ephemeral, ValueStage::Constexpr, *sema_var.typeID, *sema_var.expr
							));
						}else{
							return std::optional<ExprInfo>(ExprInfo(
								ValueCategory::EphemeralFluid,
								ValueStage::Constexpr,
								ExprInfo::FluidType{},
								*sema_var.expr
							));
						}
					};
				}

				evo::debugFatalBreak("Unknown or unsupported AST::VarDecl::Kind");

			}else if constexpr(std::is_same<IdentIDType, sema::ParamID>()){
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ident,
					"parameter identifiers are unimplemented"
				);
				return evo::resultError;

			}else if constexpr(std::is_same<IdentIDType, sema::ReturnParamID>()){
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ident,
					"return parameter identifiers are unimplemented"
				);
				return evo::resultError;

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::ModuleInfo>()){
				return std::optional<ExprInfo>(
					ExprInfo(
						ExprInfo::ValueCategory::Module,
						ExprInfo::ValueStage::Comptime,
						ident_id.sourceID,
						sema::Expr::createModuleIdent(ident_id.tokenID)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, BaseType::Alias::ID>()){
				this->emit_error(
					Diagnostic::Code::SemaTypeUsedAsExpr,
					ident,
					"Type alias cannot be used as an expression"
				);
				return evo::resultError;

			}else if constexpr(std::is_same<IdentIDType, BaseType::Typedef::ID>()){
				this->emit_error(
					Diagnostic::Code::SemaTypeUsedAsExpr,
					ident,
					"Typedef cannot be used as an expression"
				);
				return evo::resultError;

			}else{
				static_assert(false, "Unsupported IdentID");
			}
		});
	}

	
	auto SemanticAnalyzer::analyze_expr_intrinsic(const Token::ID& intrinsic_token_id) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			intrinsic_token_id,
			"intrinsic expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	auto SemanticAnalyzer::analyze_expr_literal(const Token::ID& literal) -> evo::Result<ExprInfo> {
		const Token& literal_token = this->source.getTokenBuffer()[literal];
		switch(literal_token.kind()){
			case Token::Kind::LiteralInt: {
				return ExprInfo(
					ExprInfo::ValueCategory::EphemeralFluid,
					ExprInfo::ValueStage::Comptime,
					ExprInfo::FluidType{},
					sema::Expr(this->context.sema_buffer.createIntValue(
						core::GenericInt(256, literal_token.getInt()), std::nullopt
					))
				);	
			} break;

			case Token::Kind::LiteralFloat: {
				return ExprInfo(
					ExprInfo::ValueCategory::EphemeralFluid,
					ExprInfo::ValueStage::Comptime,
					ExprInfo::FluidType{},
					sema::Expr(this->context.sema_buffer.createFloatValue(
						core::GenericFloat::createF128(literal_token.getFloat()), std::nullopt
					))
				);	
			} break;

			case Token::Kind::LiteralBool: {
				return ExprInfo(
					ExprInfo::ValueCategory::Ephemeral,
					ExprInfo::ValueStage::Comptime,
					this->context.getTypeManager().getTypeBool(),
					sema::Expr(this->context.sema_buffer.createBoolValue(literal_token.getBool()))
				);	
			} break;

			case Token::Kind::LiteralString: {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					literal,
					"string literals (except for in @import calls) are currently unsupported"
				);
				return evo::resultError;
			} break;

			case Token::Kind::LiteralChar: {
				return ExprInfo(
					ExprInfo::ValueCategory::Ephemeral,
					ExprInfo::ValueStage::Comptime,
					this->context.getTypeManager().getTypeChar(),
					sema::Expr(this->context.sema_buffer.createCharValue(literal_token.getChar()))
				);	
			} break;

			default: evo::debugFatalBreak("Not a valid literal");
		}
	}

	
	auto SemanticAnalyzer::analyze_expr_uninit(const Token::ID& uninit) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			uninit,
			"uninit expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	auto SemanticAnalyzer::analyze_expr_zeroinit(const Token::ID& zeroinit) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			zeroinit,
			"zeroinit expressions are unimplemented"
		);
		return evo::resultError;
	}

	
	auto SemanticAnalyzer::analyze_expr_this(const Token::ID& this_expr) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			this_expr,
			"this expressions are unimplemented"
		);
		return evo::resultError;
	}



	//////////////////////////////////////////////////////////////////////
	// misc

	auto SemanticAnalyzer::get_current_scope_level() const -> sema::ScopeLevel& {
		return this->context.sema_buffer.scope_manager.getLevel(this->scope.getCurrentLevel());
	}


	//////////////////////////////////////////////////////////////////////
	// error handling / diagnostics


	template<bool IS_NOT_TEMPLATE_ARG>
	auto SemanticAnalyzer::type_check(
		TypeInfo::ID expected_type_id,
		ExprInfo& got_expr,
		std::string_view expected_type_location_name,
		const auto& location
	) -> TypeCheckInfo {
		evo::debugAssert(
			std::isupper(int(expected_type_location_name[0])),
			"first character of expected_type_location_name should be upper-case"
		);

		switch(got_expr.value_category){
			case ExprInfo::ValueCategory::Ephemeral:
			case ExprInfo::ValueCategory::ConcreteConst:
			case ExprInfo::ValueCategory::ConcreteMut:
			case ExprInfo::ValueCategory::ConcreteConstForwardable:
			case ExprInfo::ValueCategory::ConcreteConstDestrMovable: {
				if(got_expr.isMultiValue()){
					auto name_copy = std::string(expected_type_location_name);
					name_copy[0] = char(std::tolower(int(name_copy[0])));

					this->emit_error(
						Diagnostic::Code::SemaMultiReturnIntoSingleValue,
						location,
						std::format("Cannot set {} with multiple values", name_copy)
					);
					return TypeCheckInfo(false, false);
				}

				const TypeInfo::ID got_type_id = got_expr.type_id.as<TypeInfo::ID>();

				if(expected_type_id != got_type_id){ // if types are not exact, check if implicit conversion is valid
					const TypeInfo& expected_type = this->context.getTypeManager().getTypeInfo(expected_type_id);
					const TypeInfo& got_type      = this->context.getTypeManager().getTypeInfo(got_type_id);

					if(
						expected_type.baseTypeID()        != got_type.baseTypeID() || 
						expected_type.qualifiers().size() != got_type.qualifiers().size()
					){	

						if constexpr(IS_NOT_TEMPLATE_ARG){
							this->error_type_mismatch(
								expected_type_id, got_expr, expected_type_location_name, location
							);
						}
						return TypeCheckInfo(false, false);
					}

					// TODO: optimze this?
					for(size_t i = 0; i < expected_type.qualifiers().size(); i+=1){
						const AST::Type::Qualifier& expected_qualifier = expected_type.qualifiers()[i];
						const AST::Type::Qualifier& got_qualifier      = got_type.qualifiers()[i];

						if(expected_qualifier.isPtr != got_qualifier.isPtr){
							if constexpr(IS_NOT_TEMPLATE_ARG){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location
								);
							}
							return TypeCheckInfo(false, false);
						}
						if(expected_qualifier.isReadOnly == false && got_qualifier.isReadOnly){
							if constexpr(IS_NOT_TEMPLATE_ARG){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location
								);
							}
							return TypeCheckInfo(false, false);
						}
					}
				}

				if constexpr(IS_NOT_TEMPLATE_ARG){
					EVO_DEFER([&](){ got_expr.type_id.emplace<TypeInfo::ID>(expected_type_id); });
				}

				return TypeCheckInfo(true, got_expr.type_id.as<TypeInfo::ID>() != expected_type_id);
			} break;

			case ExprInfo::ValueCategory::EphemeralFluid: {
				const TypeInfo& expected_type_info = this->context.getTypeManager().getTypeInfo(expected_type_id);

				if(
					expected_type_info.qualifiers().empty() == false || 
					expected_type_info.baseTypeID().kind() != BaseType::Kind::Primitive
				){
					if constexpr(IS_NOT_TEMPLATE_ARG){
						this->error_type_mismatch(
							expected_type_id, got_expr, expected_type_location_name, location
						);
					}
					return TypeCheckInfo(false, false);
				}

				const BaseType::Primitive::ID expected_type_primitive_id = expected_type_info.baseTypeID().primitiveID();

				const BaseType::Primitive& expected_type_primitive = 
					this->context.getTypeManager().getPrimitive(expected_type_primitive_id);

				if(got_expr.getExpr().kind() == sema::Expr::Kind::IntValue){
					bool is_unsigned = true;

					switch(expected_type_primitive.kind()){
						case Token::Kind::TypeInt:
						case Token::Kind::TypeISize:
						case Token::Kind::TypeI_N:
						case Token::Kind::TypeCShort:
						case Token::Kind::TypeCInt:
						case Token::Kind::TypeCLong:
						case Token::Kind::TypeCLongLong:
							is_unsigned = false;
							break;

						case Token::Kind::TypeUInt:
						case Token::Kind::TypeUSize:
						case Token::Kind::TypeUI_N:
						case Token::Kind::TypeByte:
						case Token::Kind::TypeCUShort:
						case Token::Kind::TypeCUInt:
						case Token::Kind::TypeCULong:
						case Token::Kind::TypeCULongLong:
							break;

						default: {
							if constexpr(IS_NOT_TEMPLATE_ARG){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location
								);
							}
							return TypeCheckInfo(false, false);
						}
					}

					if constexpr(IS_NOT_TEMPLATE_ARG){
						const TypeManager& type_manager = this->context.getTypeManager();

						const sema::IntValue::ID int_value_id = got_expr.getExpr().intValueID();
						sema::IntValue& int_value = this->context.sema_buffer.int_values[int_value_id];

						if(is_unsigned){
							if(int_value.value.slt(core::GenericInt(256, 0, true))){
								this->emit_error(
									Diagnostic::Code::SemaCannotConvertFluidValue,
									location,
									"Cannot implicitly convert this fluid value to the target type",
									Diagnostic::Info("Fluid value is negative and target type is unsigned")
								);
								return TypeCheckInfo(false, false);
							}
						}

						const core::GenericInt target_min = type_manager.getMin(expected_type_info.baseTypeID())
							.as<core::GenericInt>().extOrTrunc(int_value.value.getBitWidth(), is_unsigned);

						const core::GenericInt target_max = type_manager.getMax(expected_type_info.baseTypeID())
							.as<core::GenericInt>().extOrTrunc(int_value.value.getBitWidth(), is_unsigned);

						if(is_unsigned){
							if(int_value.value.ult(target_min) || int_value.value.ugt(target_max)){
								this->emit_error(
									Diagnostic::Code::SemaCannotConvertFluidValue,
									location,
									"Cannot implicitly convert this fluid value to the target type",
									Diagnostic::Info("Requires truncation (maybe use [as] operator)")
								);
								return TypeCheckInfo(false, false);
							}
						}else{
							if(int_value.value.slt(target_min) || int_value.value.sgt(target_max)){
								this->emit_error(
									Diagnostic::Code::SemaCannotConvertFluidValue,
									location,
									"Cannot implicitly convert this fluid value to the target type",
									Diagnostic::Info("Requires truncation (maybe use [as] operator)")
								);
								return TypeCheckInfo(false, false);
							}
						}

						int_value.typeID = this->context.getTypeManager().getTypeInfo(expected_type_id).baseTypeID();
					}

				}else{
					evo::debugAssert(
						got_expr.getExpr().kind() == sema::Expr::Kind::FloatValue, "Expected float"
					);

					switch(expected_type_primitive.kind()){
						case Token::Kind::TypeF16:
						case Token::Kind::TypeBF16:
						case Token::Kind::TypeF32:
						case Token::Kind::TypeF64:
						case Token::Kind::TypeF80:
						case Token::Kind::TypeF128:
						case Token::Kind::TypeCLongDouble:
							break;

						default: {
							if constexpr(IS_NOT_TEMPLATE_ARG){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location
								);
							}
							return TypeCheckInfo(false, false);
						}
					}

					if constexpr(IS_NOT_TEMPLATE_ARG){
						const TypeManager& type_manager = this->context.getTypeManager();

						const sema::FloatValue::ID float_value_id = got_expr.getExpr().floatValueID();
						sema::FloatValue& float_value = this->context.sema_buffer.float_values[float_value_id];


						const core::GenericFloat target_lowest = type_manager.getMin(expected_type_info.baseTypeID())
							.as<core::GenericFloat>().asF128();

						const core::GenericFloat target_max = type_manager.getMax(expected_type_info.baseTypeID())
							.as<core::GenericFloat>().asF128();


						const core::GenericFloat converted_literal = float_value.value.asF128();

						if(converted_literal.lt(target_lowest) || converted_literal.gt(target_max)){
							this->emit_error(
								Diagnostic::Code::SemaCannotConvertFluidValue,
								location,
								"Cannot implicitly convert this fluid value to the target type",
								Diagnostic::Info("Requires truncation (maybe use [as] operator)")
							);
							return TypeCheckInfo(false, false);
						}

						float_value.typeID = this->context.getTypeManager().getTypeInfo(expected_type_id).baseTypeID();
					}
				}

				if constexpr(IS_NOT_TEMPLATE_ARG){
					got_expr.value_category = ExprInfo::ValueCategory::Ephemeral;
					got_expr.type_id.emplace<TypeInfo::ID>(expected_type_id);
				}

				return TypeCheckInfo(true, true);
			} break;

			case ExprInfo::ValueCategory::Initializer:
				evo::debugFatalBreak("Initializer should not be compared with this function");

			case ExprInfo::ValueCategory::Module:
				evo::debugFatalBreak("Module should not be compared with this function");

			case ExprInfo::ValueCategory::Function:
				evo::debugFatalBreak("Function should not be compared with this function");

			case ExprInfo::ValueCategory::Intrinsic:
				evo::debugFatalBreak("Intrinsic should not be compared with this function");

			case ExprInfo::ValueCategory::TemplateIntrinsic:
				evo::debugFatalBreak("TemplateIntrinsic should not be compared with this function");

			case ExprInfo::ValueCategory::Template:
				evo::debugFatalBreak("Template should not be compared with this function");
		}

		evo::unreachable();
	}


	auto SemanticAnalyzer::error_type_mismatch(
		TypeInfo::ID expected_type_id,
		const ExprInfo& got_expr,
		std::string_view expected_type_location_name,
		const auto& location
	) -> void {
		evo::debugAssert(
			std::isupper(int(expected_type_location_name[0])), "first character of name should be upper-case"
		);

		std::string expected_type_str = std::format("{} is of type: ", expected_type_location_name);
		auto got_type_str = std::string("Expression is of type: ");

		while(expected_type_str.size() < got_type_str.size()){
			expected_type_str += ' ';
		}

		while(got_type_str.size() < expected_type_str.size()){
			got_type_str += ' ';
		}

		auto infos = evo::SmallVector<Diagnostic::Info>();
		infos.emplace_back(
			expected_type_str + 
			this->context.getTypeManager().printType(expected_type_id, this->context.getSourceManager())
		);
		infos.emplace_back(got_type_str + this->print_type(got_expr));

		this->emit_error(
			Diagnostic::Code::SemaTypeMismatch,
			location,
			std::format(
				"{} cannot accept an expression of a different type, and cannot be implicitly converted",
				expected_type_location_name
			),
			std::move(infos)
		);
	}



	template<bool IS_GLOBAL, bool IS_FUNC>
	auto SemanticAnalyzer::add_ident_to_scope(std::string_view ident_str, Token::ID location) -> bool {
		if(this->get_current_scope_level().addIdent(ident_str, location) == false){
			this->error_already_defined(ident_str, location);
			return false;
		}

		const auto find = this->source.global_decl_ident_infos_map.find(ident_str);
		if(find == this->source.global_decl_ident_infos_map.end()){ return true; }

		Source::GlobalDeclIdentInfo& found_info = this->source.global_decl_ident_infos[find->second];

		if constexpr(IS_FUNC){
			if(found_info.is_func){ return true; }
		}

		const auto lock = std::scoped_lock(found_info.lock);

		if(found_info.declared_location.has_value()){
			if(*found_info.declared_location == location){ return true; }

			this->emit_error(
				Diagnostic::Code::SemaIdentAlreadyInScope,
				location,
				std::format("Identifier \"{}\" was already defined in this scope", ident_str),
				Diagnostic::Info("First defined here:", this->get_location(*found_info.declared_location))
			);
			return false;
		}

		if constexpr(IS_GLOBAL){
			if(found_info.first_sub_scope_location.has_value()){
				this->emit_error(
					Diagnostic::Code::SemaIdentAlreadyInScope,
					location,
					std::format("Identifier \"{}\" was already defined in this scope", ident_str),
					Diagnostic::Info("First defined here:", this->get_location(*found_info.first_sub_scope_location))
				);
				return false;

			}else{
				found_info.declared_location = location;
			}
		}else{
			if(found_info.first_sub_scope_location.has_value() == false){
				found_info.first_sub_scope_location = location;
			}
		}

		return true;
	}



	auto SemanticAnalyzer::check_type_qualifiers(evo::ArrayProxy<AST::Type::Qualifier> qualifiers, const auto& location)
	-> bool {
		bool found_read_only_ptr = false;
		for(ptrdiff_t i = qualifiers.size() - 1; i >= 0; i-=1){
			const AST::Type::Qualifier& qualifier = qualifiers[i];

			if(found_read_only_ptr){
				if(qualifier.isPtr && qualifier.isReadOnly == false){
					this->emit_error(
						Diagnostic::Code::SemaInvalidTypeQualifiers,
						location,
						"Invalid type qualifiers",
						Diagnostic::Info(
							"If one type qualifier level is a read-only pointer, "
							"all previous pointer qualifier levels must also be read-only"
						)
					);
					return false;
				}

			}else if(qualifier.isPtr && qualifier.isReadOnly){
				found_read_only_ptr = true;
			}
		}
		return true;
	}


	auto SemanticAnalyzer::error_already_defined(std::string_view ident_str, const auto& redef_location) -> void {
		for(size_t i = this->scope.size() - 1; sema::ScopeLevel::ID scope_level_id : this->scope){
			const sema::ScopeLevel& scope_level = this->context.sema_buffer.scope_manager.getLevel(scope_level_id);
			const sema::ScopeLevel::IdentID* lookup_ident_id = scope_level.lookupIdent(ident_str);

			if(lookup_ident_id == nullptr){ i -= 1; continue; }

			lookup_ident_id->visit([&](const auto& ident_id) -> void {
				using IdentIDType = std::decay_t<decltype(ident_id)>;

				auto infos = evo::SmallVector<Diagnostic::Info>();

				if constexpr(std::is_same<IdentIDType, evo::SmallVector<sema::Func::ID>>()){
					if(ident_id.size() == 1){
						infos.emplace_back("First defined here:", this->get_location(ident_id.front()));

					}else if(ident_id.size() == 2){
						infos.emplace_back(
							"First defined here (and 1 other place):", this->get_location(ident_id.front())
						);
					}else{
						infos.emplace_back(
							std::format("First defined here (and {} other places):", ident_id.size() - 1),
							this->get_location(ident_id.front())
						);
					}

				}else{
					infos.emplace_back("First defined here:", this->get_location(ident_id));
				}

				if(scope_level_id != this->scope.getCurrentLevel()){
					infos.emplace_back("Note: shadowing is not allowed");
				}

				this->emit_error(
					Diagnostic::Code::SemaIdentAlreadyInScope,
					redef_location,
					std::format("Identifier \"{}\" was already defined in this scope", ident_str),
					std::move(infos)
				);
			});
			return;
		}

		evo::debugFatalBreak("Didn't find first defined location");
	}


	auto SemanticAnalyzer::print_type(const ExprInfo& expr_info) const -> std::string {
		return expr_info.type_id.visit([&](const auto& type_id) -> std::string {
			using TypeID = std::decay_t<decltype(type_id)>;

			if constexpr(std::is_same<TypeID, ExprInfo::InitializerType>()){
				return "{INITIALIZER}";

			}else if constexpr(std::is_same<TypeID, ExprInfo::FluidType>()){
				if(expr_info.getExpr().kind() == sema::Expr::Kind::IntValue){
					return "{FLUID INTEGRAL}";
				}else{
					evo::debugAssert(
						expr_info.getExpr().kind() == sema::Expr::Kind::FloatValue, "Unsupported fluid type"
					);
					return "{FLUID FLOAT}";
				}
				
			}else if constexpr(std::is_same<TypeID, TypeInfo::ID>()){
				return this->context.getTypeManager().printType(type_id, this->context.getSourceManager());

			}else if constexpr(std::is_same<TypeID, evo::SmallVector<TypeInfo::ID>>()){
				return "{MULTIPLE-RETURN}";

			}else if constexpr(std::is_same<TypeID, Source::ID>()){
				return "{MODULE}";

			}else{
				static_assert(false, "Unsupported type id kind");
			}
		});
	}


	auto SemanticAnalyzer::emit_warning(Diagnostic::Code code, const auto& location, auto&&... args) -> void {
		this->context.emitWarning(code, this->get_location(location), std::forward<decltype(args)>(args)...);
	}

	auto SemanticAnalyzer::emit_error(Diagnostic::Code code, const auto& location, auto&&... args) -> void {
		this->context.emitError(code, this->get_location(location), std::forward<decltype(args)>(args)...);
	}

	auto SemanticAnalyzer::emit_fatal(Diagnostic::Code code, const auto& location, auto&&... args) -> void {
		this->context.emitFatal(code, this->get_location(location), std::forward<decltype(args)>(args)...);
	}


}