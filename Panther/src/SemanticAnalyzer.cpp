//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./SemanticAnalyzer.h"

#include "../include/AST.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{

	//////////////////////////////////////////////////////////////////////
	// intrinsic lookup table

	class IntrinsicLookupTable{
		public:
			IntrinsicLookupTable() = default;
			~IntrinsicLookupTable() = default;

			using IntrinKind = evo::Variant<std::monostate, Intrinsic::Kind, TemplatedIntrinsic::Kind>;

			auto setup() -> void {
				evo::debugAssert(this->isSetup() == false, "intrinsic lookup table was already setup");

				this->map = std::unordered_map<std::string_view, IntrinKind>{
					{"breakpoint", Intrinsic::Kind::Breakpoint},
					{"_printHelloWorld", Intrinsic::Kind::_printHelloWorld},

					{"sizeOf", TemplatedIntrinsic::Kind::SizeOf},
				};

				this->map_end = this->map.end();

				this->is_setup = true;
			}

			auto lookup(std::string_view intrinsic) -> IntrinKind {
				evo::debugAssert(this->isSetup(), "intrinsic lookup table was not setup");

				const auto lookup_iter = this->map.find(intrinsic);
				if(lookup_iter == this->map_end){ return std::monostate(); }

				return lookup_iter->second;
			}


			EVO_NODISCARD auto isSetup() const -> bool { return this->is_setup; }

		private:
			bool is_setup = false;
			std::unordered_map<std::string_view, IntrinKind> map{};
			std::unordered_map<std::string_view, IntrinKind>::iterator map_end{map.end()};
	};


	static IntrinsicLookupTable intrinsic_lookup_table{};


	auto setupIntrinsicLookupTable() -> void {
		intrinsic_lookup_table.setup();
	}


	auto isIntrinsicLookupTableSetup() -> bool {
		return intrinsic_lookup_table.isSetup();
	}



	//////////////////////////////////////////////////////////////////////
	// semantic analyzer

	SemanticAnalyzer::SemanticAnalyzer(Context& _context, Source::ID source_id) 
		: context(_context),
		source(this->context.getSourceManager()[source_id]),
		scope(),
		template_parents(),
		comptime_call_stack() {

		this->scope.pushLevel(this->source.global_scope_level);
	};

	SemanticAnalyzer::SemanticAnalyzer(
		Context& _context,
		Source& _source,
		const ScopeManager::Scope& _scope,
		evo::SmallVector<SourceLocation>&& _template_parents,
		evo::SmallVector<ASG::Func::LinkID>&& _comptime_call_stack
	) : context(_context),
		source(_source),
		scope(_scope),
		template_parents(std::move(_template_parents)),
		comptime_call_stack(_comptime_call_stack) {
		this->scope.pushLevel(this->source.global_scope_level);
	};

	
	auto SemanticAnalyzer::analyze_global_declarations() -> bool {
		for(const AST::Node& global_stmt : this->source.getASTBuffer().getGlobalStmts()){
			if(this->analyze_global_declaration(global_stmt) == false){ return false; }
		}

		return this->context.errored();
	}


	auto SemanticAnalyzer::analyze_global_comptime_stmts() -> bool {
		for(const GlobalScope::Func& global_func : this->source.global_scope.getFuncs()){
			const ASG::Func& asg_func = this->source.getASGBuffer().getFunc(global_func.asg_func);
			const BaseType::ID func_base_type_id = asg_func.baseTypeID;
			const BaseType::Function& func_base_type =
				this->context.getTypeManager().getFunction(func_base_type_id.funcID());
			
			if(func_base_type.isRuntime() == false){
				if(this->analyze_func_body<false>(global_func.ast_func, global_func.asg_func) == false){ return false; }
			}
		}

		return !this->context.errored();
	}


	auto SemanticAnalyzer::analyze_global_runtime_stmts() -> bool {
		for(const GlobalScope::Func& global_func : this->source.global_scope.getFuncs()){
			const ASG::Func& asg_func = this->source.getASGBuffer().getFunc(global_func.asg_func);
			const BaseType::ID func_base_type_id = asg_func.baseTypeID;
			const BaseType::Function& func_base_type =
				this->context.getTypeManager().getFunction(func_base_type_id.funcID());
			
			if(func_base_type.isRuntime()){
				if(this->analyze_func_body<true>(global_func.ast_func, global_func.asg_func) == false){ return false; }
			}
		}

		return !this->context.errored();
	}



	auto SemanticAnalyzer::analyze_global_declaration(const AST::Node& global_stmt) -> bool {
		switch(global_stmt.kind()){
			case AST::Kind::None: {
				this->emit_fatal(
					Diagnostic::Code::SemaEncounteredASTKindNone,
					std::nullopt,
					Diagnostic::createFatalMessage("Encountered AST node kind of None")
				);
				return false;
			} break;

			case AST::Kind::VarDecl: {
				return this->analyze_var_decl<true>(this->source.getASTBuffer().getVarDecl(global_stmt));
			} break;

			case AST::Kind::FuncDecl: {
				const AST::FuncDecl& func_decl = this->source.getASTBuffer().getFuncDecl(global_stmt);
				return !this->analyze_func_decl<true>(func_decl).isError();
			} break;

			case AST::Kind::AliasDecl: {
				return this->analyze_alias_decl<true>(this->source.getASTBuffer().getAliasDecl(global_stmt));
			} break;

			case AST::Kind::WhenConditional: {
				return this->analyze_when_conditional<true>(
					this->source.getASTBuffer().getWhenConditional(global_stmt)
				);
			} break;


			case AST::Kind::Return: case AST::Kind::Unreachable: case AST::Kind::Conditional:
			case AST::Kind::Block:  case AST::Kind::FuncCall:    case AST::Kind::TemplatedExpr:
			case AST::Kind::Infix:  case AST::Kind::Postfix:     case AST::Kind::MultiAssign:
			case AST::Kind::Ident:  case AST::Kind::Intrinsic:   case AST::Kind::Literal:
			case AST::Kind::This: {
				this->emit_error(
					Diagnostic::Code::SemaInvalidGlobalStmtKind,
					global_stmt,
					"Invalid global statement"
				);
				return this->may_recover();
			};


			case AST::Kind::TemplatePack:   case AST::Kind::Prefix:    case AST::Kind::Type:
			case AST::Kind::AttributeBlock: case AST::Kind::Attribute: case AST::Kind::BuiltinType:
			case AST::Kind::Uninit:         case AST::Kind::Zeroinit:  case AST::Kind::Discard: {
				// TODO: message the exact kind
				this->emit_fatal(
					Diagnostic::Code::SemaInvalidGlobalStmtKind,
					std::nullopt,
					Diagnostic::createFatalMessage("Invalid global statement")
				);
				return false;
			};

		}

		evo::debugFatalBreak("Unknown or unsupported AST::Kind");
	}



	template<bool IS_GLOBAL>
	auto SemanticAnalyzer::analyze_var_decl(const AST::VarDecl& var_decl) -> bool {
		if constexpr(IS_GLOBAL){ evo::debugAssert("Don't call this function for global scope yet"); }

		const std::string_view var_ident = this->source.getTokenBuffer()[var_decl.ident].getString();
		if(this->already_defined(var_ident, var_decl)){ return false; }


		///////////////////////////////////
		// type

		auto var_type_id = std::optional<TypeInfo::ID>();
		if(var_decl.type.has_value()){
			const evo::Result<TypeInfo::VoidableID> var_type_result = 
				this->get_type_id(this->source.getASTBuffer().getType(*var_decl.type));
			if(var_type_result.isError()){ return false; }

			if(var_type_result.value().isVoid()){
				this->emit_error(
					Diagnostic::Code::SemaImproperUseOfTypeVoid,
					*var_decl.type,
					"Variables cannot be of type \"Void\""
				);
				return false;
			}

			var_type_id = var_type_result.value().typeID();
		}


		///////////////////////////////////
		// value

		if(var_decl.value.has_value() == false){
			this->emit_error(Diagnostic::Code::SemaVarWithNoValue, var_decl.ident, "Variables must have values");
			return false;	
		}

		evo::Result<ExprInfo> expr_info_result = [&](){
			if constexpr(IS_GLOBAL){
				return this->analyze_expr<ExprValueKind::ConstEval>(*var_decl.value);
			}else{
				if(var_decl.kind == AST::VarDecl::Kind::Def){
					return this->analyze_expr<ExprValueKind::ConstEval>(*var_decl.value);
				}else{
					return this->analyze_expr<ExprValueKind::Runtime>(*var_decl.value);
				}
			}
		}();
		if(expr_info_result.isError()){ return false; }


		///////////////////////////////////
		// type checking

		if(expr_info_result.value().is_concrete()){
			// TODO: better messaging?
			this->emit_error(
				Diagnostic::Code::SemaIncorrectExprValueType,
				*var_decl.value,
				"Variable must be declared with an ephemeral expression value"
			);
			return false;

		}else if(expr_info_result.value().value_type == ExprInfo::ValueType::Import){
			if(var_decl.kind != AST::VarDecl::Kind::Def){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectImportDecl,
					var_decl,
					"Imports must be declared as \"def\""
				);
				return false;
			}

			if(var_type_id.has_value()){
				// TODO: better error message
				this->emit_error(
					Diagnostic::Code::SemaIncorrectImportDecl,
					var_decl,
					"Imports must be declared with type inference"
				);
				return false;
			}

			this->get_current_scope_level().addImport(
				var_ident, expr_info_result.value().type_id.as<Source::ID>(), var_decl.ident
			);
			return true;
		}

		evo::debugAssert(
			expr_info_result.value().is_ephemeral()
				|| expr_info_result.value().value_type == ExprInfo::ValueType::Initializer,
			"unhandled expr info value type"
		);


		if(expr_info_result.value().value_type == ExprInfo::ValueType::EpemeralFluid){
			if(var_type_id.has_value() == false){
				this->emit_error(
					Diagnostic::Code::SemaCannotInferType, *var_decl.value, "Cannot infer the type of a fluid literal"
				);
				return false;
			}

			if(this->type_check<true>(*var_type_id, expr_info_result.value(), "Variable", *var_decl.value).ok == false){
				return false;	
			}

		}else if(expr_info_result.value().value_type == ExprInfo::ValueType::Initializer){
			if(var_type_id.has_value() == false){
				this->emit_error(
					Diagnostic::Code::SemaCannotInferType,
					*var_decl.value,
					"Cannot infer the type of an initializer value"
				);
				return false;
			}

		}else if(var_type_id.has_value()){
			if(expr_info_result.value().type_id.as<evo::SmallVector<TypeInfo::ID>>().size() > 1){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectNumberOfAssignTargets,
					*var_decl.value,
					"Variable declaration value has multiple values - multiple-assignment is required"
				);
				return false;
			}

			if(this->type_check<true>(*var_type_id, expr_info_result.value(), "Variable", *var_decl.value).ok == false){
				return false;	
			}

		}else{
			const evo::SmallVector<TypeInfo::ID>& expr_types
				= expr_info_result.value().type_id.as<evo::SmallVector<TypeInfo::ID>>();

			if(expr_types.size() > 1){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectNumberOfAssignTargets,
					*var_decl.value,
					"Variable declaration value has multiple values - multiple-assignment is required"
				);
				return false;
			}


			var_type_id = expr_types.front();
		}


		///////////////////////////////////
		// attributes

		const AST::AttributeBlock& attr_block = this->source.getASTBuffer().getAttributeBlock(var_decl.attributeBlock);
		for(const AST::AttributeBlock::Attribute& attribute : attr_block.attributes){
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			// if(attribute_str == "something"){

			// }else{
				this->emit_error(
					Diagnostic::Code::SemaUnknownAttribute,
					attribute.attribute,
					std::format("Unknown variable attribute \"#{}\"", attribute_str)
				);				
			// }
		}



		///////////////////////////////////
		// create

		const ASG::Var::ID asg_var_id = this->source.asg_buffer.createVar(
			var_decl.kind,
			var_decl.ident,
			*var_type_id,
			expr_info_result.value().getExpr(),
			var_decl.kind == AST::VarDecl::Kind::Const
		);

		this->get_current_scope_level().addVar(var_ident, asg_var_id);

		if constexpr(IS_GLOBAL){
			this->source.global_scope.addVar(var_decl, asg_var_id);
		}else{
			this->get_current_scope_level().stmtBlock().emplace_back(asg_var_id);
		}
		
		return true;
	};



	template<bool IS_GLOBAL>
	auto SemanticAnalyzer::analyze_func_decl(const AST::FuncDecl& func_decl, ASG::Func::InstanceID instance_id)
	-> evo::Result<std::optional<ASG::Func::ID>> {
		const bool instantiate_template = instance_id.has_value();

		const Token::ID func_ident_tok_id = this->source.getASTBuffer().getIdent(func_decl.name);
		const std::string_view func_ident = this->source.getTokenBuffer()[func_ident_tok_id].getString();
		if(instantiate_template == false){
			const ScopeManager::Level& current_scope_level = this->get_current_scope_level();
			const ScopeManager::Level::IdentID* lookup_ident_id = current_scope_level.lookupIdent(func_ident);

			if(lookup_ident_id != nullptr && lookup_ident_id->is<evo::SmallVector<ASG::Func::ID>>() == false){
				[[maybe_unused]] const auto _ = this->already_defined(func_ident, func_decl);
				return evo::resultError;
			}
		}


		// lookup for ident reuse within the function declaration without having a whole new scope
		// 		(not needed until analyzing function body)
		// for templates, vars, return, and errors
		auto created_params = std::unordered_map<std::string_view, Token::ID>();


		///////////////////////////////////
		// attributes

		auto pub_attr = ConditionalAttribute();
		auto runtime_attr = ConditionalAttribute();
		bool is_entry = false;

		const AST::AttributeBlock& attr_block = this->source.getASTBuffer().getAttributeBlock(func_decl.attributeBlock);
		for(const AST::AttributeBlock::Attribute& attribute : attr_block.attributes){
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(pub_attr.check(*this, attribute, "pub") == false){ return evo::resultError; }

			}else if(attribute_str == "runtime"){
				if(is_entry){
					this->emit_warning(
						Diagnostic::Code::SemaWarnEntryIsImplicitRuntime,
						attribute,
						"The function with the \"entry\" attribute is implicitly runtime"
					);
					continue;
				}

				if(runtime_attr.check(*this, attribute, "runtime") == false){ return evo::resultError; }


			}else if(attribute_str == "entry"){
				if(runtime_attr.is_set()){
					this->emit_warning(
						Diagnostic::Code::SemaWarnEntryIsImplicitRuntime,
						attribute,
						"The function with the \"entry\" attribute is implicitly runtime"
					);
					continue;
				}

				if(is_entry){
					this->emit_error(
						Diagnostic::Code::SemaAttributeAlreadySet,
						attribute,
						"Attribute `#entry` was already set"
					);
					return evo::resultError;
				}

				if(func_decl.templatePack.has_value()){
					this->emit_error(
						Diagnostic::Code::SemaInvalidEntrySignature,
						*func_decl.templatePack,
						"Entry function cannot be templated"
					);
					return evo::resultError;
				}

				if(attribute.args.empty() == false){
					this->emit_error(
						Diagnostic::Code::SemaInvalidAttributeArgument,
						attribute.args.back(),
						"Invalid argument in attribute `#entry`"
					);
					return evo::resultError;
				}

				runtime_attr.force_set();
				is_entry = true;

			}else{
				this->emit_error(
					Diagnostic::Code::SemaUnknownAttribute,
					attribute.attribute,
					std::format("Unknown function attribute \"#{}\"", attribute_str)
				);
			}
		}



		///////////////////////////////////
		// template pack

		if(instantiate_template == false && func_decl.templatePack.has_value()){
			auto template_params = evo::SmallVector<ASG::TemplatedFunc::TemplateParam>{};

			const AST::TemplatePack& ast_template_pack = 
				this->source.getASTBuffer().getTemplatePack(*func_decl.templatePack);

			if(ast_template_pack.params.empty()){
				this->emit_error(
					Diagnostic::Code::SemaEmptyTemplatePackDeclaration,
					func_decl,
					"Template pack declarations cannot be empty"
				);
				return evo::resultError;
			}


			{ // get template params
				this->scope.pushFakeObjectScope();
				EVO_DEFER([&](){ this->scope.popFakeObjectScope(); });

				for(const AST::TemplatePack::Param& template_param : ast_template_pack.params){
					const Token& template_param_ident_token = this->source.getTokenBuffer()[template_param.ident];
					const std::string_view template_param_ident = template_param_ident_token.getString();

					// check if param already created
					if(this->already_defined(template_param_ident, template_param.ident)){ return evo::resultError; }
					if(const auto find = created_params.find(template_param_ident); find != created_params.end()){
						this->emit_error(
							Diagnostic::Code::SemaAlreadyDefined,
							template_param.ident,
							std::format("Identifier \"{}\" was already defined in this scope", template_param_ident),
							Diagnostic::Info("Declared here", this->get_source_location(find->second))
						);
						return evo::resultError;
					}

					created_params.emplace(template_param_ident, template_param.ident);

					// create param
					const AST::Type& param_ast_type = this->source.getASTBuffer().getType(template_param.type);

					const evo::Result<bool> param_type_is_generic = this->is_type_generic(param_ast_type);
					if(param_type_is_generic.isError()){ return evo::resultError; }

					if(param_type_is_generic.value()){
						template_params.emplace_back(template_param.ident, std::nullopt);

					}else{
						const evo::Result<TypeInfo::VoidableID> param_type_id = this->get_type_id(param_ast_type);
						if(param_type_id.isError()){ return evo::resultError; }

						if(param_type_id.value().isVoid()){
							this->emit_error(
								Diagnostic::Code::SemaImproperUseOfTypeVoid,
								param_ast_type,
								"Template parameter cannot be of type \"Void\""
							);
							return evo::resultError;
						}

						template_params.emplace_back(template_param.ident, param_type_id.value().typeID());
					}
				}
			}

			// create

			const ASG::Parent parent = this->get_parent<IS_GLOBAL>();

			const ASG::TemplatedFunc::ID asg_templated_func_id = this->source.asg_buffer.createTemplatedFunc(
				func_decl, parent, std::move(template_params), this->scope, pub_attr.is_set(), runtime_attr.is_set()
			);

			this->get_current_scope_level().addTemplatedFunc(func_ident, asg_templated_func_id);

			return std::optional<ASG::Func::ID>();
		}



		///////////////////////////////////
		// params

		auto params = evo::SmallVector<BaseType::Function::Param>();
		{
			this->scope.pushFakeObjectScope();
			EVO_DEFER([&](){ this->scope.popFakeObjectScope(); });

			for(const AST::FuncDecl::Param& param : func_decl.params){
				if(param.type.has_value() == false){
					this->emit_error(
						Diagnostic::Code::MiscUnimplementedFeature,
						param.name,
						"[this] parameters are not supported"
					);
					return evo::resultError;
				}

				const Token::ID param_ident_token_id = this->source.getASTBuffer().getIdent(param.name);
				const std::string_view param_ident = this->source.getTokenBuffer()[param_ident_token_id].getString();

				if(this->already_defined(param_ident, param.name)){ return evo::resultError; }
				if(const auto find = created_params.find(param_ident); find != created_params.end()){
					this->emit_error(
						Diagnostic::Code::SemaAlreadyDefined,
						param.name,
						std::format("Identifier \"{}\" was already defined in this scope", param_ident),
						Diagnostic::Info("Declared here", this->get_source_location(find->second))
					);
					return evo::resultError;
				}

				created_params.emplace(param_ident, param_ident_token_id);

				const evo::Result<TypeInfo::VoidableID> param_type_res = this->get_type_id(
					this->source.getASTBuffer().getType(*param.type)
				);
				if(param_type_res.isError()){ return evo::resultError; }

				if(param_type_res.value().isVoid()){
					this->emit_error(
						Diagnostic::Code::SemaParamTypeVoid,
						*param.type,
						"The type of a function parameter cannot be \"Void\""
					);
					return evo::resultError;
				}

				const AST::AttributeBlock& param_attr_block =
					this->source.getASTBuffer().getAttributeBlock(param.attributeBlock);

				bool is_must_label = false;

				for(const AST::AttributeBlock::Attribute& attribute : param_attr_block.attributes){
					const std::string_view attribute_str =
						this->source.getTokenBuffer()[attribute.attribute].getString();

					// TODO: check for attribute reuse
					if(attribute_str == "noAlias"){
						this->emit_warning(
							Diagnostic::Code::SemaUnknownAttribute,
							attribute.attribute,
							"Function parameter attribute \"#noAlias\" is not implemented yet - ignoring"
						);

					}else if(attribute_str == "restrict"){
						this->emit_error(
							Diagnostic::Code::SemaUnknownAttribute,
							attribute.attribute,
							std::format("Unknown parameter attribute \"#{}\"", attribute_str),
							Diagnostic::Info("Use \"#noAlias\" instead")
						);
						return evo::resultError;

					}else if(attribute_str == "mustLabel"){
						if(is_must_label){
							this->emit_error(
								Diagnostic::Code::SemaAttributeAlreadySet,
								attribute,
								"Attribute `#mustLabel` was already set"
							);
							return evo::resultError;
						}else{
							is_must_label = true;
						}


					}else{
						this->emit_error(
							Diagnostic::Code::SemaUnknownAttribute,
							attribute.attribute,
							std::format("Unknown parameter attribute \"#{}\"", attribute_str)
						);
						return evo::resultError;
					}
				}

				const bool optimize_with_copy = [&](){
					if(param.kind == AST::FuncDecl::Param::Kind::Mut){ return false; }

					const TypeInfo::ID param_type = param_type_res.value().typeID();
					const TypeManager& type_manager = this->context.getTypeManager();
					return type_manager.isTriviallyCopyable(param_type) && 
						(type_manager.sizeOf(param_type) <= type_manager.sizeOfGeneralRegister());
				}();

				params.emplace_back(
					param_ident_token_id, param_type_res.value().typeID(), param.kind, is_must_label, optimize_with_copy
				);
			}
		}


		///////////////////////////////////
		// returns

		auto return_params = evo::SmallVector<BaseType::Function::ReturnParam>{};
		for(size_t i = 0; const AST::FuncDecl::Return& return_param : func_decl.returns){
			const AST::Type& return_param_ast_type = this->source.getASTBuffer().getType(return_param.type);
			const evo::Result<TypeInfo::VoidableID> type_info = this->get_type_id(return_param_ast_type);
			if(type_info.isError()){ return evo::resultError; }

			if(return_param.ident.has_value() && type_info.value().isVoid()){
				this->emit_error(
					Diagnostic::Code::SemaImproperUseOfTypeVoid,
					return_param.type,
					"The type of a named function return parameter cannot be \"Void\""
				);
				return evo::resultError;
			}

			return_params.emplace_back(return_param.ident, type_info.value());

			i += 1;
		}


		if(is_entry){
			if(return_params.size() > 1){
				this->emit_error(
					Diagnostic::Code::SemaInvalidEntrySignature,
					*func_decl.returns[1].ident,
					"Entry function cannot have multiple returns"
				);
				return evo::resultError;
			}

			if(return_params[0].typeID != this->context.getTypeManager().getTypeUI8()){
				this->emit_error(
					Diagnostic::Code::SemaInvalidEntrySignature,
					func_decl.returns[0].type,
					"Entry function must return \"UI8\""
				);
				return evo::resultError;
			}

			if(func_decl.returns[0].ident.has_value()){
				this->emit_error(
					Diagnostic::Code::SemaInvalidEntrySignature,
					*func_decl.returns[0].ident,
					"Entry function cannot have a named return parameter"
				);
				return evo::resultError;
			}
		}


		///////////////////////////////////
		// check for overload redeclaration

		if(instantiate_template == false){
			const ScopeManager::Level& current_scope_level = this->get_current_scope_level();
			const ScopeManager::Level::IdentID* lookup_ident_id = current_scope_level.lookupIdent(func_ident);

			if(lookup_ident_id != nullptr){
				for(const ASG::Func::ID& overload_id : lookup_ident_id->as<evo::SmallVector<ASG::Func::ID>>()){
					bool is_different = false;

					const ASG::Func& overload = this->source.getASGBuffer().getFunc(overload_id);
					const BaseType::Function& overload_type =
						this->context.getTypeManager().getFunction(overload.baseTypeID.funcID());

					if(params.size() != overload_type.params().size()){ break; }

					for(size_t i = 0; const BaseType::Function::Param& overload_param : overload_type.params()){
						if(overload_param.typeID != params[i].typeID || overload_param.kind != params[i].kind){
							is_different = true;
							break;
						}
					
						i += 1;
					}

					if(is_different == false){
						this->emit_error(
							Diagnostic::Code::SemaOverloadAlreadyDefined,
							func_decl,
							"Function overload already defined",
							Diagnostic::Info("First defined here:", this->get_source_location(overload_id))
						);
						return evo::resultError;
					}
				}
			}
		}



		///////////////////////////////////
		// create

		const ASG::Parent parent = this->get_parent<IS_GLOBAL>();

		const BaseType::ID base_type_id = this->context.getTypeManager().getOrCreateFunction(
			BaseType::Function(std::move(params), std::move(return_params), runtime_attr.is_set())
		);

		const std::optional<ScopeManager::Scope> decl_scope = [&](){
			if(runtime_attr.is_set()){
				return std::optional<ScopeManager::Scope>();
			}else{
				return std::optional<ScopeManager::Scope>(this->scope);
			}
		}();

		const ASG::Func::ID asg_func_id = this->source.asg_buffer.createFunc(
			func_decl.name, base_type_id, parent, instance_id, pub_attr.is_set(), decl_scope, func_decl
		);
		
		if(instantiate_template == false){
			this->get_current_scope_level().addFunc(func_ident, asg_func_id);

			if(is_entry && this->context.setEntry(ASG::Func::LinkID(this->source.getID(), asg_func_id)) == false){
				this->emit_error(
					Diagnostic::Code::SemaMultipleEntriesDeclared,
					func_decl.returns[0].type,
					"Multiple entry functions declared",
					Diagnostic::Info("First declared here: ", this->get_source_location(*this->context.getEntry()))
				);
				return evo::resultError;
			}
		}


		if(instantiate_template){
			return std::optional<ASG::Func::ID>(asg_func_id);

		}else{
			if constexpr(IS_GLOBAL){
				this->source.global_scope.addFunc(func_decl, asg_func_id);
				return std::optional<ASG::Func::ID>(asg_func_id);
			}else{
				if(this->analyze_func_body<true>(func_decl, asg_func_id)){
					return std::optional<ASG::Func::ID>(asg_func_id);
				}else{
					return evo::resultError;
				}
			}
		}
	};


	template<bool IS_GLOBAL>
	auto SemanticAnalyzer::analyze_alias_decl(const AST::AliasDecl& alias_decl) -> bool {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, alias_decl, "alias declarations are currently unsupported"
		);
		return false;
	};


	template<bool IS_GLOBAL>
	auto SemanticAnalyzer::analyze_when_conditional(const AST::WhenConditional& when_conditional) -> bool {
		evo::Result<ExprInfo> cond = this->analyze_expr<ExprValueKind::ConstEval>(when_conditional.cond);
		if(cond.isError()){ return false; }

		const TypeInfo::ID bool_type_id = this->context.getTypeManager().getTypeBool();
		if(
			this->type_check<true>(
				bool_type_id, cond.value(), "when conditional condition", when_conditional.cond
			).ok == false
		){
			return false;
		}

		const bool cond_value = [&](){
			const ASG::Expr& expr = cond.value().getExpr();
			const ASG::LiteralBool& literal_bool = this->source.getASGBuffer().getLiteralBool(expr.literalBoolID());
			return literal_bool.value;
		}();

		if(cond_value){
			const AST::Block& then_block = this->source.getASTBuffer().getBlock(when_conditional.thenBlock);

			if constexpr(IS_GLOBAL){
				for(const AST::Node& global_stmt : then_block.stmts){
					if(this->analyze_global_declaration(global_stmt) == false){ return false; }
				}

				return true;

			}else{
				return this->analyze_block(then_block);
			}
		}

		if(when_conditional.elseBlock.has_value()){
			if(when_conditional.elseBlock->kind() == AST::Kind::Block){
				const AST::Block& else_block = this->source.getASTBuffer().getBlock(*when_conditional.elseBlock);
				for(const AST::Node& global_stmt : else_block.stmts){
					if(this->analyze_global_declaration(global_stmt) == false){ return false; }
				}

				return true;
				
			}else{
				return this->analyze_when_conditional<IS_GLOBAL>(
					this->source.getASTBuffer().getWhenConditional(*when_conditional.elseBlock)
				);
			}
		}

		return true;
	}





	auto SemanticAnalyzer::analyze_conditional(const AST::Conditional& conditional) -> bool {
		evo::Result<ExprInfo> cond = this->analyze_expr<ExprValueKind::Runtime>(conditional.cond);
		if(cond.isError()){ return this->may_recover(); }

		const TypeInfo::ID bool_type_id = this->context.getTypeManager().getTypeBool();
		if(
			this->type_check<true>(
				bool_type_id, cond.value(), "if conditional condition", conditional.cond
			).ok == false
		){
			return this->may_recover();
		}


		auto then_block = ASG::StmtBlock();
		{
			this->push_scope_level(&then_block);
			EVO_DEFER([&](){ this->pop_scope_level(); });

			const AST::Block ast_block = this->source.getASTBuffer().getBlock(conditional.thenBlock);
			if(this->analyze_block(ast_block) == false){ return this->may_recover(); }
		}


		auto else_block = ASG::StmtBlock();
		if(conditional.elseBlock.has_value()){
			this->push_scope_level(&else_block);
			EVO_DEFER([&](){ this->pop_scope_level(); });

			if(conditional.elseBlock->kind() == AST::Kind::Block){
				const AST::Block& ast_block = this->source.getASTBuffer().getBlock(*conditional.elseBlock);
				if(this->analyze_block(ast_block) == false){ return this->may_recover(); }
				
			}else{
				const AST::Conditional& ast_conditional =
					this->source.getASTBuffer().getConditional(*conditional.elseBlock);
				if(this->analyze_conditional(ast_conditional) == false){
					return this->may_recover();
				}
			}

		}else{
			// requires new sub-scope to be added even if theres no else block for termination tracking
			this->get_current_scope_level().addSubScope();
		}

		const ASG::Conditional::ID asg_cond_id = this->source.asg_buffer.createConditional(
			cond.value().getExpr(), std::move(then_block), std::move(else_block)
		);

		this->get_current_scope_level().stmtBlock().emplace_back(asg_cond_id);

		return true;
	}


	template<bool IS_RUNTIME>
	auto SemanticAnalyzer::analyze_func_body(const AST::FuncDecl& ast_func, ASG::Func::ID asg_func_id) -> bool {
		ASG::Func& asg_func = this->source.asg_buffer.funcs[asg_func_id];

		const auto asg_func_body_lock = std::unique_lock(asg_func.body_analysis_mutex);
		if(asg_func.is_body_analyzed){ return true; } // don't re-analyze


		this->push_scope_level(&asg_func.stmts, asg_func_id);
		EVO_DEFER([&](){ this->pop_scope_level(); });

		const BaseType::Function& asg_func_type = this->context.getTypeManager().getFunction(
			asg_func.baseTypeID.funcID()
		);

		// create params
		for(uint32_t i = 0; const BaseType::Function::Param& param : asg_func_type.params()){
			const ASG::Param::ID asg_param_id = this->source.asg_buffer.createParam(asg_func_id, i);
			this->get_current_scope_level().addParam(
				this->source.getTokenBuffer()[param.ident.as<Token::ID>()].getString(), asg_param_id
			);
			asg_func.params.emplace_back(asg_param_id);

			i += 1;
		}

		// create return params
		if(asg_func_type.returnParams()[0].ident.has_value()){
			for(uint32_t i = 0; const BaseType::Function::ReturnParam& ret_param : asg_func_type.returnParams()){
				const ASG::ReturnParam::ID asg_ret_param_id = this->source.asg_buffer.createReturnParam(asg_func_id, i);
				this->get_current_scope_level().addReturnParam(
					this->source.getTokenBuffer()[*ret_param.ident].getString(), asg_ret_param_id
				);
				asg_func.returnParams.emplace_back(asg_ret_param_id);

				i += 1;
			}
		}

		if(this->analyze_block(this->source.getASTBuffer().getBlock(ast_func.block)) == false){
			if(asg_func_type.isRuntime() == false){ asg_func.is_body_errored = true; }
			return this->may_recover();
		}

		if(this->get_current_scope_level().isTerminated()){
			this->get_current_func().isTerminated = true;

		}else if(asg_func_type.returnsVoid() == false){
			this->emit_error(
				Diagnostic::Code::SemaFuncIsntTerminated,
				ast_func,
				"Function isn't terminated",
				Diagnostic::Info(
					"A function is terminated when all control paths end in a [return], [unreachable], "
					"or a function call that has the attribute `#noReturn`"
				)
			);
			return false;
		}


		asg_func.is_body_analyzed = true;

		if constexpr(IS_RUNTIME == false){
			if(asg_func.is_body_errored){ return false; }
			this->context.comptime_executor.addFunc(ASG::Func::LinkID(this->source.getID(), asg_func_id));
		}

		return true;
	}



	auto SemanticAnalyzer::analyze_block(const AST::Block& block) -> bool {
		for(const AST::Node& stmt : block.stmts){
			if(this->analyze_stmt(stmt) == false){ return false; }
		}

		return true;
	}


	auto SemanticAnalyzer::analyze_local_scope_block(const AST::Block& block) -> bool {
		this->push_scope_level(&this->get_current_scope_level().stmtBlock());
		EVO_DEFER([&](){ this->pop_scope_level(); });

		return this->analyze_block(block);
	}




	auto SemanticAnalyzer::analyze_stmt(const AST::Node& node) -> bool {
		if(this->get_current_scope_level().isTerminated()){
			// TODO: better messaging
			this->emit_error(
				Diagnostic::Code::SemaStmtAfterScopeTerminated,
				node,
				"Encountered statement after scope is terminated"
			);
			return false;
		}


		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		// TODO: order cases correctly
		switch(node.kind()){
			case AST::Kind::None: {
				this->emit_fatal(
					Diagnostic::Code::SemaEncounteredASTKindNone,
					std::nullopt,
					Diagnostic::createFatalMessage("Encountered AST node kind of None")
				);
				return false;
			} break;

			case AST::Kind::VarDecl: {
				return this->analyze_var_decl<false>(ast_buffer.getVarDecl(node));
			} break;

			case AST::Kind::FuncDecl: {
				return this->analyze_func_decl<false>(ast_buffer.getFuncDecl(node)).isSuccess();
			} break;

            case AST::Kind::AliasDecl: {
            	return this->analyze_alias_decl<false>(ast_buffer.getAliasDecl(node));
        	} break;

        	case AST::Kind::FuncCall: {
        		return this->analyze_func_call(ast_buffer.getFuncCall(node));
    		} break;

    		case AST::Kind::Infix: {
    			return this->analyze_infix_stmt(ast_buffer.getInfix(node));
			} break;

			case AST::Kind::Return: {
    			return this->analyze_return_stmt(ast_buffer.getReturn(node));
			} break;

			case AST::Kind::Unreachable: {
    			return this->analyze_unreachable_stmt(ast_buffer.getUnreachable(node));
			} break;

			case AST::Kind::Conditional: {
    			return this->analyze_conditional(ast_buffer.getConditional(node));
			} break;

			case AST::Kind::WhenConditional: {
    			return this->analyze_when_conditional<false>(ast_buffer.getWhenConditional(node));
			} break;

			case AST::Kind::Block: {
				return this->analyze_local_scope_block(ast_buffer.getBlock(node));
			} break;


			case AST::Kind::MultiAssign: {
				return this->analyze_multi_assign_stmt(ast_buffer.getMultiAssign(node));
			} break;


			case AST::Kind::Literal:   case AST::Kind::This:    case AST::Kind::Ident:
			case AST::Kind::Intrinsic: case AST::Kind::Postfix: case AST::Kind::TemplatedExpr: {
				// TODO: message the exact kind
				this->emit_error(Diagnostic::Code::SemaInvalidStmtKind, node, "Invalid statement");
				return this->may_recover();
			} break;


			case AST::Kind::TemplatePack:   case AST::Kind::Prefix:    case AST::Kind::Type:
			case AST::Kind::AttributeBlock: case AST::Kind::Attribute: case AST::Kind::BuiltinType:
			case AST::Kind::Uninit:         case AST::Kind::Zeroinit:  case AST::Kind::Discard: {
				// TODO: message the exact kind
				this->emit_fatal(
					Diagnostic::Code::SemaInvalidStmtKind,
					std::nullopt,
					Diagnostic::createFatalMessage("Invalid statement")
				);
				return false;
			} break;
		}

		return true;
	}


	auto SemanticAnalyzer::analyze_func_call(const AST::FuncCall& func_call) -> bool {
		if(func_call.target.kind() == AST::Kind::Intrinsic){
			const Token::ID intrinsic_ident_token_id = this->source.getASTBuffer().getIntrinsic(func_call.target);
			const std::string_view intrinsic_ident = 
				this->source.getTokenBuffer()[intrinsic_ident_token_id].getString();

			if(intrinsic_ident == "import"){
				if(this->analyze_import<ExprValueKind::Runtime>(func_call).isError()){ return false; }

				this->emit_error(Diagnostic::Code::SemaDiscardingFuncReturn, func_call, "Cannot discard an import");
				return false;
			}
		}
		

		evo::Result<AnalyzedFuncCallData> analyzed_func_call_data
			= this->analyze_func_call_impl<ExprValueKind::Runtime, true>(func_call);
		if(analyzed_func_call_data.isError()){ return this->may_recover(); }

		this->get_current_scope_level().stmtBlock().emplace_back(*analyzed_func_call_data.value().asg_func_call_id);
		return true;
	}



	auto SemanticAnalyzer::analyze_infix_stmt(const AST::Infix& infix) -> bool {
		if(this->source.getTokenBuffer()[infix.opTokenID].kind() != Token::lookupKind("=")){
			// TODO: better messaging
			this->emit_error(Diagnostic::Code::SemaInvalidStmtKind, infix, "Invalid stmt kind");
			return this->may_recover();
		}

		///////////////////////////////////
		// assignment

		// lhs

		if(infix.lhs.kind() == AST::Kind::Discard){
			if(infix.rhs.kind() != AST::Kind::FuncCall){
				this->emit_error(
					Diagnostic::Code::SemaInvalidDiscardStmtRHS,
					infix.rhs,
					"Invalid rhs of discard assignment"
				);
				return this->may_recover();
			}

			const AST::FuncCall& func_call = this->source.getASTBuffer().getFuncCall(infix.rhs);
			const evo::Result<ExprInfo> func_call_info = 
				this->analyze_expr_func_call<ExprValueKind::Runtime>(func_call);
			if(func_call_info.isError()){ return false; }

			if(func_call_info.value().value_type == ExprInfo::ValueType::Import){
				this->emit_error(
					Diagnostic::Code::SemaInvalidDiscardStmtRHS,
					infix.rhs,
					"Invalid rhs of discard assignment"
				);
				return this->may_recover();
			}

			this->get_current_scope_level().stmtBlock().emplace_back(func_call_info.value().getExpr().funcCallID());
			return true;
		}

		const evo::Result<ExprInfo> lhs_info = this->analyze_expr<ExprValueKind::Runtime>(infix.lhs);
		if(lhs_info.isError()){ return this->may_recover(); }

		if(lhs_info.value().value_type == ExprInfo::ValueType::ConcreteMutGlobal){
			const BaseType::ID current_func_base_type_id = this->get_current_func().baseTypeID;
			const BaseType::Function& current_func_base_type =
				this->context.getTypeManager().getFunction(current_func_base_type_id.funcID());
			if(current_func_base_type.isRuntime() == false){
				this->emit_error(
					Diagnostic::Code::SemaAssignmentDstGlobalInRuntimeFunc,
					infix.lhs,
					"Cannot assign to a global in a function that does not have the runtime attribute"
				);
				return this->may_recover();
			}

		}else if(lhs_info.value().value_type != ExprInfo::ValueType::ConcreteMut){
			if(lhs_info.value().is_const()){
				this->emit_error(
					Diagnostic::Code::SemaAssignmentDstNotConcreteMutable,
					infix.lhs,
					"Cannot assign to a constant value"
				);
				return this->may_recover();

			}else{
				this->emit_error(
					Diagnostic::Code::SemaAssignmentDstNotConcreteMutable,
					infix.lhs,
					"Cannot assign to a non-concrete value"
				);
				return this->may_recover();
			}
		}


		// rhs

		evo::Result<ExprInfo> rhs_info = this->analyze_expr<ExprValueKind::Runtime>(infix.rhs);
		if(rhs_info.isError()){ return this->may_recover(); }

		if(rhs_info.value().is_ephemeral() == false){
			this->emit_error(
				Diagnostic::Code::SemaAssignmentValueNotEphemeral,
				infix.rhs,
				"Assignment value must be ephemeral"
			);
			return this->may_recover();
		}

		const evo::SmallVector<TypeInfo::ID> lhs_types = lhs_info.value().type_id.as<evo::SmallVector<TypeInfo::ID>>();
		if(lhs_types.size() > 1){
			this->emit_error(
				Diagnostic::Code::SemaIncorrectNumberOfAssignTargets,
				infix.rhs,
				"Multiple values cannot be assigned to a single assignment target"
			);
			return this->may_recover();
		}
		if(this->type_check<true>(lhs_types[0], rhs_info.value(), "Assignment", infix.rhs).ok == false){
			return this->may_recover();
		}


		// create

		const ASG::Assign::ID asg_assign_id = this->source.asg_buffer.createAssign(
			lhs_info.value().getExpr(), rhs_info.value().getExpr()
		);

		this->get_current_scope_level().stmtBlock().emplace_back(asg_assign_id);

		return true;
	}



	auto SemanticAnalyzer::analyze_return_stmt(const AST::Return& return_stmt) -> bool {
		if(return_stmt.label.has_value()){
			this->emit_error(
				Diagnostic::Code::MiscUnimplementedFeature,
				return_stmt.label.value(),
				"return statements with labels are currently unsupported"
			);
			return false;
		}

		const ASG::Func& current_func = this->get_current_func();
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(
			current_func.baseTypeID.funcID()
		);

		const evo::ArrayProxy<BaseType::Function::ReturnParam> returns = func_type.returnParams();

		auto return_value = std::optional<ASG::Expr>();
		const bool return_value_visit_result = return_stmt.value.visit([&](const auto& value) -> bool {
			using ValueT = std::decay_t<decltype(value)>;

			if constexpr(std::is_same_v<ValueT, std::monostate>){
				if(returns[0].ident.has_value()){
					this->emit_error(
						Diagnostic::Code::SemaIncorrectReturnStmtKind,
						return_stmt,
						"Incorrect return statement kind for a function named return parameters",
						Diagnostic::Info("Set all return values and use \"return...;\" instead")
					);
					return false;
				}

				if(returns[0].typeID.isVoid() == false){
					// TODO: different Diagnostic::Code?
					this->emit_error(
						Diagnostic::Code::SemaIncorrectReturnStmtKind,
						return_stmt,
						"Functions that have a return type other than \"Void\" must return a value"
					);
					return false;
				}

				return true;

			}else if constexpr(std::is_same_v<ValueT, panther::AST::Node>){
				evo::Result<ExprInfo> value_info = this->analyze_expr<ExprValueKind::Runtime>(value);
				if(value_info.isError()){ return false; }

				if(returns[0].typeID.isVoid()){
					// TODO: different Diagnostic::Code?
					this->emit_error(
						Diagnostic::Code::SemaIncorrectReturnStmtKind,
						return_stmt,
						"Functions that have a return type \"Void\" cannot return a value"
					);
					return false;
				}

				if(returns[0].ident.has_value()){
					this->emit_error(
						Diagnostic::Code::SemaIncorrectReturnStmtKind,
						value,
						"Incorrect return statement kind for a function named return parameters",
						Diagnostic::Info("Set all return values and use \"return...;\" instead")
					);
					return false;
				}

				if(value_info.value().is_ephemeral() == false){
					this->emit_error(
						Diagnostic::Code::SemaReturnNotEphemeral,
						value,
						"Value of return statement is not ephemeral"
					);
					return false;
				}
					
				if(this->type_check<true>(returns[0].typeID.typeID(), value_info.value(), "Return", value).ok == false){
					return this->may_recover();
				}

				return_value = value_info.value().getExpr();

				return true;

			}else if constexpr(std::is_same_v<ValueT, panther::Token::ID>){
				if(returns[0].ident.has_value() == false){
					this->emit_error(
						Diagnostic::Code::SemaIncorrectReturnStmtKind,
						value,
						"Incorrect return statement kind for single unnamed return parameters",
						Diagnostic::Info("Use \"return;\" instead")
					);
					return false;
				}

				return true;

			}else{
				static_assert(sizeof(ValueT) < 0, "Unknown or unsupported return value kind");
			}
		});
		if(!return_value_visit_result){ return false; }

		const ASG::Return::ID asg_return_id = this->source.asg_buffer.createReturn(return_value);

		this->get_current_scope_level().stmtBlock().emplace_back(asg_return_id);

		this->get_current_scope_level().setTerminated();

		return true;
	}



	auto SemanticAnalyzer::analyze_unreachable_stmt(const Token::ID& unreachable_stmt) -> bool {
		this->get_current_scope_level().stmtBlock().emplace_back(ASG::Stmt::createUnreachable(unreachable_stmt));

		this->get_current_scope_level().setTerminated();
		
		return true;
	}


	auto SemanticAnalyzer::analyze_multi_assign_stmt(const AST::MultiAssign& multi_assign) -> bool {
		if(multi_assign.assigns.size() == 1){
			this->emit_warning(
				Diagnostic::Code::SemaWarnSingleValInMultiAssign,
				multi_assign.assigns[0],
				"Single assignment target in multiple-assignment statemet"
			);
		}


		///////////////////////////////////
		// lhs

		auto assign_target_infos = evo::SmallVector<std::optional<ExprInfo>>();
		assign_target_infos.reserve(multi_assign.assigns.size());
		for(const AST::Node& assign_target : multi_assign.assigns){
			if(assign_target.kind() == AST::Kind::Discard){
				assign_target_infos.emplace_back(std::nullopt);
				continue;
			}

			const evo::Result<ExprInfo> assign_target_info = this->analyze_expr<ExprValueKind::Runtime>(assign_target);
			if(assign_target_info.isError()){ return this->may_recover(); }

			if(assign_target_info.value().value_type != ExprInfo::ValueType::ConcreteMut){
				if(assign_target_info.value().is_const()){
					this->emit_error(
						Diagnostic::Code::SemaAssignmentDstNotConcreteMutable,
						assign_target,
						"Cannot assign to a constant value"
					);
					return this->may_recover();

				}else{
					this->emit_error(
						Diagnostic::Code::SemaAssignmentDstNotConcreteMutable,
						assign_target,
						"Cannot assign to a non-concrete value"
					);
					return this->may_recover();
				}
			}

			assign_target_infos.emplace_back(assign_target_info.value());
		}



		///////////////////////////////////
		// rhs

		const evo::Result<ExprInfo> rhs_info = this->analyze_expr<ExprValueKind::Runtime>(multi_assign.value);
		if(rhs_info.isError()){ return this->may_recover(); }

		if(rhs_info.value().is_ephemeral() == false){
			this->emit_error(
				Diagnostic::Code::SemaAssignmentValueNotEphemeral,
				multi_assign.value,
				"Assignment value must be ephemeral"
			);
			return this->may_recover();
		}

		const evo::SmallVector<TypeInfo::ID>& rhs_type_ids
			= rhs_info.value().type_id.as<evo::SmallVector<TypeInfo::ID>>();


		if(rhs_type_ids.size() != assign_target_infos.size()){
			this->emit_error(
				Diagnostic::Code::SemaIncorrectNumberOfAssignTargets,
				multi_assign,
				"Incorrect number of assignment targets",
				evo::SmallVector<Diagnostic::Info>{
					Diagnostic::Info(std::format("assignment targets: {}", assign_target_infos.size())),
					Diagnostic::Info(std::format("expression values:  {}", rhs_type_ids.size()))
				}
			);
			return this->may_recover();
		}


		for(size_t i = 0; std::optional<ExprInfo>& assign_target : assign_target_infos){
			EVO_DEFER([&](){ i += 1; });
			if(assign_target.has_value() == false){ continue; }

			const TypeInfo::ID rhs_type_id = rhs_type_ids[i];

			if(
				this->type_check<true>(rhs_type_id, *assign_target, "Assignment", multi_assign.assigns[i]).ok == false
			){
				return this->may_recover();
			}
		}




		///////////////////////////////////
		// create

		auto assign_targets = evo::SmallVector<std::optional<ASG::Expr>>();
		assign_targets.reserve(assign_target_infos.size());
		for(const std::optional<ExprInfo>& assign_target_info : assign_target_infos){
			if(assign_target_info.has_value()){
				assign_targets.emplace_back(assign_target_info->getExpr());
			}else{
				assign_targets.emplace_back();
			}
		}

		const ASG::MultiAssign::ID asg_multi_assign_id = this->source.asg_buffer.createMultiAssign(
			std::move(assign_targets), rhs_info.value().getExpr()
		);

		this->get_current_scope_level().stmtBlock().emplace_back(asg_multi_assign_id);

		return true;
	}





	auto SemanticAnalyzer::get_type_id(const AST::Type& ast_type) -> evo::Result<TypeInfo::VoidableID> {
		auto base_type = std::optional<BaseType::ID>();
		auto qualifiers = evo::SmallVector<AST::Type::Qualifier>();

		switch(ast_type.base.kind()){
			case AST::Kind::BuiltinType: {
				const Token::ID builtin_type_token_id = ASTBuffer::getBuiltinType(ast_type.base);
				const Token& builtin_type_token = this->source.getTokenBuffer()[builtin_type_token_id];

				switch(builtin_type_token.kind()){
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
					case Token::Kind::TypeCShort:    case Token::Kind::TypeCUShort:    case Token::Kind::TypeCInt:
					case Token::Kind::TypeCUInt:     case Token::Kind::TypeCLong:      case Token::Kind::TypeCULong:
					case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong:
					case Token::Kind::TypeCLongDouble: {
						base_type = this->context.getTypeManager().getOrCreateBuiltinBaseType(
							builtin_type_token.kind()
						);
					} break;

					case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N: {
						base_type = this->context.getTypeManager().getOrCreateBuiltinBaseType(
							builtin_type_token.kind(), builtin_type_token.getBitWidth()
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
						evo::debugFatalBreak("Unknown or unsupported BuiltinType: {}", builtin_type_token.kind());
					} break;
				}
			} break;

			case AST::Kind::Ident: {
				const Token::ID base_type_token_id = ASTBuffer::getIdent(ast_type.base);
				const evo::Result<TypeInfo::VoidableID> ident_type_id = this->get_type_id(base_type_token_id);
				if(ident_type_id.isError()){ return evo::resultError; }

				if(ident_type_id.value().isVoid()){
					if(ast_type.qualifiers.empty() == false){
						this->emit_error(
							Diagnostic::Code::SemaVoidWithQualifiers,
							ast_type.base,
							"Type \"Void\" cannot have qualifiers"
						);
						return evo::resultError;
					}
					return TypeInfo::VoidableID::Void();
				}

				const TypeInfo& type_info = this->context.getTypeManager().getTypeInfo(ident_type_id.value().typeID());

				for(const AST::Type::Qualifier& qualifier : type_info.qualifiers()){
					qualifiers.emplace_back(qualifier);
				}

				base_type = type_info.baseTypeID();
			} break;

			// TODO: separate out into more kinds to be more specific (errors vs fatal)
			default: {
				this->emit_error(
					Diagnostic::Code::SemaInvalidBaseType,
					ast_type.base,
					"Unknown or unsupported base type"
				);
				return evo::resultError;
			} break;
		}

		evo::debugAssert(base_type.has_value(), "base type was not set");

		for(const AST::Type::Qualifier& qualifier : ast_type.qualifiers){
			qualifiers.emplace_back(qualifier);
		}

		if(this->check_type_qualifiers(qualifiers, ast_type) == false){ return evo::resultError; }

		return TypeInfo::VoidableID(
			this->context.getTypeManager().getOrCreateTypeInfo(TypeInfo(*base_type, std::move(qualifiers)))
		);
	}


	auto SemanticAnalyzer::get_type_id(const Token::ID& ident_token_id) -> evo::Result<TypeInfo::VoidableID> {
		const Token& type_token = this->source.getTokenBuffer()[ident_token_id];
		const std::string_view ident_str = type_token.getString();

		const auto template_type_find = this->template_arg_types.find(ident_str);
		if(template_type_find != this->template_arg_types.end()){
			return template_type_find->second;
		}

		this->emit_error(
			Diagnostic::Code::SemaIdentNotInScope,
			ident_token_id,
			std::format("Type \"{}\" was not defined in this scope", ident_str)
		);
		return evo::resultError;
	}



	auto SemanticAnalyzer::is_type_generic(const AST::Type& ast_type) -> evo::Result<bool> {
		if(ast_type.base.kind() != AST::Kind::BuiltinType){ return false; }

		const Token::ID base_type_token_id = this->source.getASTBuffer().getBuiltinType(ast_type.base);
		const Token& base_type_token = this->source.getTokenBuffer()[base_type_token_id];
		
		if(base_type_token.kind() != Token::Kind::TypeType){ return false; }

		if(ast_type.qualifiers.empty()){ return true; }

		this->emit_error(
			Diagnostic::Code::SemaGenericTypeWithQualifiers, ast_type, "Type \"Type\" cannot have qualifiers"
		);
		return evo::resultError;
	}



	auto SemanticAnalyzer::get_current_scope_level() const -> ScopeManager::Level& {
		return this->context.getScopeManager()[this->scope.getCurrentLevel()];
	}


	auto SemanticAnalyzer::push_scope_level(ASG::StmtBlock* stmt_block) -> void {
		if(this->scope.inObjectScope()){
			this->get_current_scope_level().addSubScope();
		}
		this->scope.pushLevel(this->context.getScopeManager().createLevel(stmt_block));
	}

	auto SemanticAnalyzer::push_scope_level(ASG::StmtBlock* stmt_block, ASG::Func::ID asg_func_id) -> void {
		// if(this->scope.inObjectScope()){
			this->get_current_scope_level().addSubScope();
		// }
		this->scope.pushLevel(this->context.getScopeManager().createLevel(stmt_block), asg_func_id);
	}

	auto SemanticAnalyzer::pop_scope_level() -> void {
		ScopeManager::Level& current_scope_level = this->get_current_scope_level();
		const bool current_scope_is_terminated = current_scope_level.isTerminated();

		if(
			current_scope_level.hasStmtBlock()                      &&
			current_scope_level.stmtBlock().isTerminated() == false &&
			current_scope_level.isTerminated()
		){
			current_scope_level.stmtBlock().setTerminated();
		}

		this->scope.popLevel(); // `current_scope_level` is now invalid

		if(current_scope_is_terminated && this->scope.inObjectScope() && !this->scope.inObjectMainScope()){
			this->get_current_scope_level().setSubScopeTerminated();
		}
	}


	template<bool IS_GLOBAL>
	auto SemanticAnalyzer::get_parent() const -> ASG::Parent {
		if constexpr(IS_GLOBAL){
			return std::monostate();

		}else{
			evo::debugAssert(this->scope.inObjectScope(), "expected to be in object scope");
			return this->scope.getCurrentObjectScope().visit([&](auto obj_scope_id) -> ASG::Parent {
				using ObjScopeID = std::decay_t<decltype(obj_scope_id)>;

				if constexpr(std::is_same_v<ObjScopeID, ASG::Func::ID>){
					return obj_scope_id;
				}else{
					evo::debugFatalBreak("Unknown or unsupported object scope type");
				}
			});
		}
	}



	auto SemanticAnalyzer::get_current_func() const -> ASG::Func& {
		const ScopeManager::Scope::ObjectScope& current_object_scope = this->scope.getCurrentObjectScope();
		return this->source.asg_buffer.funcs[current_object_scope.as<ASG::Func::ID>()];
	}




	//////////////////////////////////////////////////////////////////////
	// expr


	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr(const AST::Node& node) -> evo::Result<ExprInfo> {
		switch(node.kind()){
			case AST::Kind::Block: {
				return this->analyze_expr_block<EXPR_VALUE_KIND>(this->source.getASTBuffer().getBlock(node));
			} break;
			
			case AST::Kind::FuncCall: {
				return this->analyze_expr_func_call<EXPR_VALUE_KIND>(this->source.getASTBuffer().getFuncCall(node));
			} break;
			
			case AST::Kind::TemplatedExpr: {
				return this->analyze_expr_templated_expr<EXPR_VALUE_KIND>(
					this->source.getASTBuffer().getTemplatedExpr(node)
				);
			} break;
			
			case AST::Kind::Prefix: {
				return this->analyze_expr_prefix<EXPR_VALUE_KIND>(this->source.getASTBuffer().getPrefix(node));
			} break;
			
			case AST::Kind::Infix: {
				return this->analyze_expr_infix<EXPR_VALUE_KIND>(this->source.getASTBuffer().getInfix(node));
			} break;
			
			case AST::Kind::Postfix: {
				return this->analyze_expr_postfix<EXPR_VALUE_KIND>(this->source.getASTBuffer().getPostfix(node));
			} break;
			
			case AST::Kind::Ident: {
				return this->analyze_expr_ident<EXPR_VALUE_KIND>(this->source.getASTBuffer().getIdent(node));
			} break;
			
			case AST::Kind::Intrinsic: {
				return this->analyze_expr_intrinsic<EXPR_VALUE_KIND>(this->source.getASTBuffer().getIntrinsic(node));
			} break;
			
			case AST::Kind::Literal: {
				return this->analyze_expr_literal<EXPR_VALUE_KIND>(this->source.getASTBuffer().getLiteral(node));
			} break;
			
			case AST::Kind::Uninit: {
				return this->analyze_expr_uninit<EXPR_VALUE_KIND>(this->source.getASTBuffer().getUninit(node));
			} break;

			case AST::Kind::Zeroinit: {
				return this->analyze_expr_zeroinit<EXPR_VALUE_KIND>(this->source.getASTBuffer().getZeroinit(node));
			} break;
			
			case AST::Kind::This: {
				return this->analyze_expr_this<EXPR_VALUE_KIND>(this->source.getASTBuffer().getThis(node));
			} break;
			

			case AST::Kind::None: {
				this->emit_fatal(
					Diagnostic::Code::SemaEncounteredASTKindNone,
					std::nullopt,
					Diagnostic::createFatalMessage("Encountered AST node kind of None")
				);
				return evo::resultError;
			} break;

			case AST::Kind::VarDecl:     case AST::Kind::FuncDecl:       case AST::Kind::AliasDecl:
			case AST::Kind::Return:      case AST::Kind::Conditional:    case AST::Kind::WhenConditional:
			case AST::Kind::Unreachable: case AST::Kind::TemplatePack:   case AST::Kind::MultiAssign:
			case AST::Kind::Type:        case AST::Kind::AttributeBlock: case AST::Kind::Attribute:
			case AST::Kind::BuiltinType: case AST::Kind::Discard: {
				// TODO: better messaging (specify what kind)
				this->emit_fatal(
					Diagnostic::Code::SemaInvalidExprKind,
					std::nullopt,
					Diagnostic::createFatalMessage("Encountered expr of invalid AST kind")
				);
				return evo::resultError; 
			} break;
		}

		return evo::resultError;
	}



	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_block(const AST::Block& block) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, block, "block expressions are currently unsupported"
		);
		return evo::resultError;
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_func_call(const AST::FuncCall& func_call) -> evo::Result<ExprInfo> {
		if(func_call.target.kind() == AST::Kind::Intrinsic){
			const Token::ID intrinsic_ident_token_id = this->source.getASTBuffer().getIntrinsic(func_call.target);
			const std::string_view intrinsic_ident = 
				this->source.getTokenBuffer()[intrinsic_ident_token_id].getString();

			if(intrinsic_ident == "import"){
				return this->analyze_import<EXPR_VALUE_KIND>(func_call);
			}
		}
		

		evo::Result<AnalyzedFuncCallData> analyzed_func_call_data
			= this->analyze_func_call_impl<EXPR_VALUE_KIND, false>(func_call);
		if(analyzed_func_call_data.isError()){ return evo::resultError; }



		///////////////////////////////////
		// get return types

		const evo::ArrayProxy<BaseType::Function::ReturnParam> overload_return_params =
			analyzed_func_call_data.value().selected_func_type->returnParams();

		auto return_types = evo::SmallVector<TypeInfo::ID>();
		return_types.reserve(overload_return_params.size());

		for(const BaseType::Function::ReturnParam& return_param : overload_return_params){
			return_types.emplace_back(return_param.typeID.typeID());
		}


		///////////////////////////////////
		// create

		if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
			return ExprInfo(
				ExprInfo::ValueType::Ephemeral,
				ExprInfo::generateExprInfoTypeIDs(std::move(return_types)),
				std::nullopt
			);

		}else if constexpr(EXPR_VALUE_KIND == ExprValueKind::Runtime){
			return ExprInfo(
				ExprInfo::ValueType::Ephemeral,
				ExprInfo::generateExprInfoTypeIDs(std::move(return_types)),
				ASG::Expr(*analyzed_func_call_data.value().asg_func_call_id)
			);

		}else{
			static_assert(EXPR_VALUE_KIND == ExprValueKind::ConstEval, "Wrong expr value kind");

			if(analyzed_func_call_data.value().selected_func_type->isRuntime()){
				this->emit_error(
					Diagnostic::Code::SemaCantCallRuntimeFuncInComptimeContext,
					func_call,
					"Cannot get comptime value from function with \"runtime\" attribute"
				);
				return evo::resultError;				
			}

			const ASG::FuncCall& asg_func_call = this->source.getASGBuffer().getFuncCall(
				*analyzed_func_call_data.value().asg_func_call_id
			);


			evo::SmallVector<ASG::Expr> asg_exprs = asg_func_call.target.visit(
				[&](const auto& func_call_target) -> evo::SmallVector<ASG::Expr> {
				using FuncCallTarget = std::decay_t<decltype(func_call_target)>;

				if constexpr(std::is_same_v<FuncCallTarget, ASG::Func::LinkID>){
					#if defined(PCIT_CONFIG_DEBUG)
						const ASG::Func& asg_func_target = this->context.getSourceManager()[func_call_target.sourceID()]
							.getASGBuffer().getFunc(func_call_target.funcID());

						evo::debugAssert(asg_func_target.is_body_analyzed, "body of this function was not analyzed");
					#endif

					return this->context.comptime_executor.runFunc(
						func_call_target, asg_func_call.args, this->source.asg_buffer
					);

				}else if constexpr(std::is_same_v<FuncCallTarget, Intrinsic::Kind>){
					evo::debugFatalBreak("Cannot handle this consteval func target");

				}else{
					const ASG::TemplatedIntrinsicInstantiation& instantiation =
						this->source.getASGBuffer().getTemplatedIntrinsicInstantiation(func_call_target);

					switch(instantiation.kind){
						case TemplatedIntrinsic::Kind::SizeOf: {
							const size_t type_size = this->context.getTypeManager().sizeOf(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
							);

							const ASG::LiteralInt::ID literal_int_id = this->source.asg_buffer.createLiteralInt(
								core::GenericInt::create<uint64_t>(type_size),
								this->context.getTypeManager().getTypeUSize()
							);
							return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_int_id)};
						} break;

						case TemplatedIntrinsic::Kind::_max_: {
							evo::debugFatalBreak("Intrinsic::Kind::_max_ is not an actual intrinsic");
						} break;
					}

					evo::debugFatalBreak("Unknown or unsupported templated intrinsic kind");
				}
			});

			return ExprInfo(
				ExprInfo::ValueType::Ephemeral,
				ExprInfo::generateExprInfoTypeIDs(std::move(return_types)),
				std::move(asg_exprs)
			);
		}
	}



	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_import(const AST::FuncCall& func_call) -> evo::Result<ExprInfo> {
		const Token::ID intrinsic_ident_token_id = this->source.getASTBuffer().getIntrinsic(func_call.target);
		const std::string_view intrinsic_ident = this->source.getTokenBuffer()[intrinsic_ident_token_id].getString();
		evo::debugAssert(
			intrinsic_ident == "import", "Cannot analyze this intrinsic \"@{}\" with this function", intrinsic_ident
		);

		const Token::ID lookup_path_token_id = this->source.getASTBuffer().getLiteral(func_call.args[0].value);
		const std::string_view lookup_path = this->source.getTokenBuffer()[lookup_path_token_id].getString();

		if(this->source.locationIsPath()){
			const evo::Expected<Source::ID, Context::LookupSourceIDError> lookup_import =
				this->context.lookupRelativeSourceID(this->source.getLocationPath(), lookup_path);

			if(lookup_import.has_value() == false){
				switch(lookup_import.error()){
					case Context::LookupSourceIDError::EmptyPath: {
						this->emit_error(
							Diagnostic::Code::SemaFailedToImportFile,
							func_call.args[0].value,
							"Empty path is an invalid lookup location"
						);
						return evo::resultError;
					} break;

					case Context::LookupSourceIDError::SameAsCaller: {
						// TODO: better messaging
						this->emit_error(
							Diagnostic::Code::SemaFailedToImportFile,
							func_call.args[0].value,
							"Cannot import self"
						);
						return evo::resultError;
					} break;

					case Context::LookupSourceIDError::NotOneOfSources: {
						this->emit_error(
							Diagnostic::Code::SemaFailedToImportFile,
							func_call.args[0].value,
							std::format("File \"{}\" is not one of the files being compiled", lookup_path)
						);
						return evo::resultError;
					} break;

					case Context::LookupSourceIDError::DoesntExist: {
						this->emit_error(
							Diagnostic::Code::SemaFailedToImportFile,
							func_call.args[0].value,
							std::format("Couldn't find file \"{}\"", lookup_path)
						);
						return evo::resultError;
					} break;
				};

				evo::debugFatalBreak("Unknown or unsupported error code");
			}

			return ExprInfo(ExprInfo::ValueType::Import, lookup_import.value(), std::nullopt);
			
		}else{
			const evo::Result<Source::ID> lookup_import = this->context.lookupSourceID(lookup_path);
			if(lookup_import.isError()){
				this->emit_error(
					Diagnostic::Code::SemaFailedToImportFile,
					func_call.args[0].value,
					std::format("Unknown file \"{}\"", lookup_path)
				);
				return evo::resultError;
			}

			return ExprInfo(ExprInfo::ValueType::Import, lookup_import.value(), std::nullopt);
		}

	}








	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_templated_expr(const AST::TemplatedExpr& templated_expr)
	-> evo::Result<ExprInfo> {
		const evo::Result<ExprInfo> base_info = this->analyze_expr<ExprValueKind::None>(templated_expr.base);
		if(base_info.isError()){ return evo::resultError; }

		if(base_info.value().value_type != ExprInfo::ValueType::Templated){
			if(base_info.value().value_type == ExprInfo::ValueType::TemplatedIntrinsic){
				return this->analyze_expr_templated_intrinsic<EXPR_VALUE_KIND>(
					templated_expr, base_info.value().type_id.as<TemplatedIntrinsic::Kind>()
				);
			}

			// TODO: better messaging
			this->emit_error(
				Diagnostic::Code::SemaUnexpectedTemplateArgs,
				templated_expr.base,
				"Expression does not accept template arguments"
			);
			return evo::resultError;
		}

		evo::debugAssert(
			base_info.value().type_id.is<ASG::TemplatedFunc::LinkID>(),
			"currently unsupported kind of templated type"
		);

		const ASG::TemplatedFunc::LinkID templated_func_link_id = 
			base_info.value().type_id.as<ASG::TemplatedFunc::LinkID>();

		Source& declared_source = this->context.getSourceManager()[templated_func_link_id.sourceID()];
		ASG::TemplatedFunc& templated_func = declared_source.asg_buffer.templated_funcs[
			templated_func_link_id.templatedFuncID()
		];

		if(templated_expr.args.size() != templated_func.templateParams.size()){
			// TODO: give exact number of arguments
			this->emit_error(
				Diagnostic::Code::SemaIncorrectTemplateInstantiation,
				templated_expr,
				"Incorrect number of template arguments"
			);
			return evo::resultError;
		}

		auto instantiation_args = evo::SmallVector<ASG::TemplatedFunc::Arg>();
		instantiation_args.reserve(templated_func.templateParams.size());
		auto value_args = evo::SmallVector<evo::Variant<TypeInfo::VoidableID, ExprInfo>>();
		value_args.reserve(templated_func.templateParams.size());
		for(size_t i = 0; i < templated_func.templateParams.size(); i+=1){
			const ASG::TemplatedFunc::TemplateParam& template_param = templated_func.templateParams[i];
			const AST::Node& template_arg = templated_expr.args[i];

			if(template_param.typeID.has_value()){ // is expression
				if(template_arg.kind() == AST::Kind::Type){
					// TODO: show declaration of template
					this->emit_error(
						Diagnostic::Code::SemaIncorrectTemplateInstantiation,
						templated_expr.args[i],
						std::format("Expected expression in template argument index {}", i),
						Diagnostic::Info(
							"Template parameter declaration:", this->get_source_location(template_param.ident)
						)
					);
					return evo::resultError;
				}

				evo::Result<ExprInfo> arg_expr_info = this->analyze_expr<ExprValueKind::ConstEval>(template_arg);
				if(arg_expr_info.isError()){ return evo::resultError; }


				switch(arg_expr_info.value().value_type){
					case ExprInfo::ValueType::Import: case ExprInfo::ValueType::Templated:
					case ExprInfo::ValueType::Initializer: {
						this->emit_error(
							Diagnostic::Code::SemaIncorrectTemplateArgValueType,
							template_arg,
							"Invalid template argument"
						);
						return evo::resultError;
					} break;

					default: {
						if(this->type_check<true>(
							*template_param.typeID, arg_expr_info.value(), "Template parameter", template_arg
						).ok == false){
							return evo::resultError;
						}
					} break;
				}

				evo::debugAssert(
					arg_expr_info.value().value_type == ExprInfo::ValueType::Ephemeral,
					"consteval expr is not ephemeral"
				);

				value_args.emplace_back(arg_expr_info.value());

				switch(arg_expr_info.value().getExpr().kind()){
					case ASG::Expr::Kind::LiteralInt: {
						const ASG::LiteralInt::ID literal_id = arg_expr_info.value().getExpr().literalIntID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralInt(literal_id).value);
					} break;

					case ASG::Expr::Kind::LiteralFloat: {
						const ASG::LiteralFloat::ID literal_id = arg_expr_info.value().getExpr().literalFloatID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralFloat(literal_id).value);
					} break;

					case ASG::Expr::Kind::LiteralBool: {
						const ASG::LiteralBool::ID literal_id = arg_expr_info.value().getExpr().literalBoolID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralBool(literal_id).value);
					} break;

					case ASG::Expr::Kind::LiteralChar: {
						const ASG::LiteralChar::ID literal_id = arg_expr_info.value().getExpr().literalCharID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralChar(literal_id).value);
					} break;

					default: {
						this->emit_fatal(
							Diagnostic::Code::SemaExpectedConstEvalValue,
							template_arg,
							Diagnostic::createFatalMessage("Evaluated consteval value was not actually consteval")
						);
						return evo::resultError;
					} break;
				}

			}else{ // is type
				switch(template_arg.kind()){
					case AST::Kind::Type: {
						const AST::Type& arg_ast_type = this->source.getASTBuffer().getType(template_arg);
						const evo::Result<TypeInfo::VoidableID> arg_type = this->get_type_id(arg_ast_type);
						if(arg_type.isError()){ return evo::resultError; }

						instantiation_args.emplace_back(arg_type.value());
						value_args.emplace_back(arg_type.value());
					} break;

					case AST::Kind::Ident: {
						const Token::ID arg_ast_type_token_id = this->source.getASTBuffer().getIdent(template_arg);
						const evo::Result<TypeInfo::VoidableID> arg_type = this->get_type_id(arg_ast_type_token_id);
						if(arg_type.isError()){ return evo::resultError; }

						instantiation_args.emplace_back(arg_type.value());
						value_args.emplace_back(arg_type.value());
					} break;

					default: {
						this->emit_error(
							Diagnostic::Code::SemaIncorrectTemplateInstantiation,
							templated_expr.args[i],
							std::format("Expected type in template argument index {}", i),
							Diagnostic::Info(
								"Template parameter declaration:", this->get_source_location(template_param.ident)
							)
						);
						return evo::resultError;
					} break;
				}

			}
		}

		ASG::TemplatedFunc::LookupInfo lookup_info = templated_func.lookupInstance(std::move(instantiation_args));

		auto func_id = std::optional<ASG::Func::ID>();
		if(lookup_info.needToGenerate){
			evo::SmallVector<SourceLocation> template_instance_template_parents = this->template_parents;
			template_instance_template_parents.emplace_back(this->get_source_location(templated_expr));

			auto template_sema = SemanticAnalyzer(
				this->context,
				declared_source,
				templated_func.scope,
				std::move(template_instance_template_parents),
				evo::SmallVector<ASG::Func::LinkID>()
			);

			// if sub-template, add parent's template params to scope
			if(
				templated_func.parent.is<std::monostate>() == false &&
				templated_func.parent == this->scope.getCurrentObjectScope()
			){
				template_sema.template_arg_exprs = this->template_arg_exprs;
				template_sema.template_arg_types = this->template_arg_types;
			}

			// add template params to scope
			for(size_t i = 0; i < templated_func.templateParams.size(); i+=1){
				const ASG::TemplatedFunc::TemplateParam& template_param = templated_func.templateParams[i];
				const evo::Variant<TypeInfo::VoidableID, ExprInfo> value_arg = value_args[i];

				const std::string_view param_ident_str = 
					declared_source.getTokenBuffer()[template_param.ident].getString();


				value_arg.visit([&](const auto& value) -> void {
					using ValueT = std::decay_t<decltype(value)>;

					if constexpr(std::is_same_v<ValueT, TypeInfo::VoidableID>){
						template_sema.template_arg_types.emplace(param_ident_str, value);
					}else{
						if(this->source.getID() == declared_source.getID()){
							template_sema.template_arg_exprs.emplace(param_ident_str, value);

						}else{ // if not the same source, need to copy over the expression to the ASGBuffer
							switch(value.getExpr().kind()){
								case ASG::Expr::Kind::LiteralInt: {
									const ASG::LiteralInt& literal_int =
										this->source.getASGBuffer().getLiteralInt(value.getExpr().literalIntID());

									const ASG::LiteralInt::ID copied_value = 
										declared_source.asg_buffer.createLiteralInt(
											literal_int.value, literal_int.typeID
										);

									template_sema.template_arg_exprs.emplace(
										param_ident_str,
										ExprInfo(value.value_type, value.type_id, ASG::Expr(copied_value))
									);
								} break;

								case ASG::Expr::Kind::LiteralFloat: {
									const ASG::LiteralFloat& literal_float =this->source.getASGBuffer().getLiteralFloat(
										value.getExpr().literalFloatID()
									);

									const ASG::LiteralFloat::ID copied_value = 
										declared_source.asg_buffer.createLiteralFloat(
											literal_float.value, literal_float.typeID
										);

									template_sema.template_arg_exprs.emplace(
										param_ident_str,
										ExprInfo(value.value_type, value.type_id, ASG::Expr(copied_value))
									);
								} break;

								case ASG::Expr::Kind::LiteralBool: {
									const ASG::LiteralBool& literal_bool =
										this->source.getASGBuffer().getLiteralBool(value.getExpr().literalBoolID());

									const ASG::LiteralBool::ID copied_value = 
										declared_source.asg_buffer.createLiteralBool(literal_bool.value);

									template_sema.template_arg_exprs.emplace(
										param_ident_str,
										ExprInfo(value.value_type, value.type_id, ASG::Expr(copied_value))
									);
								} break;

								case ASG::Expr::Kind::LiteralChar: {
									const ASG::LiteralChar& literal_char =
										this->source.getASGBuffer().getLiteralChar(value.getExpr().literalCharID());

									const ASG::LiteralChar::ID copied_value = 
										declared_source.asg_buffer.createLiteralChar(literal_char.value);

									template_sema.template_arg_exprs.emplace(
										param_ident_str,
										ExprInfo(value.value_type, value.type_id, ASG::Expr(copied_value))
									);
								} break;

								default: {
									evo::debugFatalBreak("Unknown or unsupported consteval expr kind");
								} break;
							}
						}
					}
				});
			}


			// analyze func decl and get instantiation
			const evo::Result<std::optional<ASG::Func::ID>> instantiation = [&](){
				if(templated_func.scope.inObjectScope()){
					return template_sema.analyze_func_decl<false>(
						templated_func.funcDecl, lookup_info.instanceID
					);

				}else{
					return template_sema.analyze_func_decl<true>(
						templated_func.funcDecl, lookup_info.instanceID
					);
					
				}
			}();

			// set instantiation
			if(instantiation.isError()){ return evo::resultError; }
			func_id = *instantiation.value();
			lookup_info.store(*func_id);

			// analyze func body
			if(
				template_sema.analyze_func_body<EXPR_VALUE_KIND == ExprValueKind::Runtime>(
					templated_func.funcDecl, *func_id
				) == false
			){
				return evo::resultError;
			}

		}else{
			func_id = lookup_info.waitForAndGetID();
		}

		const ASG::Func& instantiated_func = declared_source.getASGBuffer().getFunc(*func_id);

		const TypeInfo::ID instantiated_func_type = this->context.getTypeManager().getOrCreateTypeInfo(
			TypeInfo(instantiated_func.baseTypeID)
		);

		if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
			return ExprInfo(
				ExprInfo::ValueType::ConcreteConst,
				ExprInfo::generateExprInfoTypeIDs(instantiated_func_type),
				std::nullopt
			);

		}else{
			return ExprInfo(
				ExprInfo::ValueType::ConcreteConst,
				ExprInfo::generateExprInfoTypeIDs(instantiated_func_type),
				ASG::Expr(ASG::Func::LinkID(declared_source.getID(), *func_id))
			);
		}
	}



	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_templated_intrinsic(
		const AST::TemplatedExpr& templated_expr, TemplatedIntrinsic::Kind templated_intrinsic_kind
	) -> evo::Result<ExprInfo> {
		const TemplatedIntrinsic& templated_intrinsic = this->context.getTemplatedIntrinsic(templated_intrinsic_kind);

		if(templated_expr.args.size() != templated_intrinsic.templateParams.size()){
			// TODO: give exact number of arguments
			this->emit_error(
				Diagnostic::Code::SemaIncorrectTemplateInstantiation,
				templated_expr,
				"Incorrect number of template arguments"
			);
			return evo::resultError;
		}

		auto instantiation_type_args = evo::SmallVector<std::optional<TypeInfo::VoidableID>>();
		instantiation_type_args.reserve(templated_intrinsic.templateParams.size());

		auto instantiation_args = evo::SmallVector<ASG::TemplatedIntrinsicInstantiation::TemplateArg>();
		instantiation_args.reserve(templated_intrinsic.templateParams.size());
		
		for(size_t i = 0; i < templated_intrinsic.templateParams.size(); i+=1){
			const std::optional<TypeInfo::ID>& template_param = templated_intrinsic.templateParams[i];
			const AST::Node& template_arg = templated_expr.args[i];

			if(template_param.has_value()){ // templated param is expr
				if(template_arg.kind() == AST::Kind::Type){
					// TODO: show declaration of template
					this->emit_error(
						Diagnostic::Code::SemaIncorrectTemplateInstantiation,
						templated_expr.args[i],
						std::format("Expected expression in template argument index {}", i)
					);
					return evo::resultError;
				}

				evo::Result<ExprInfo> arg_expr_info = this->analyze_expr<ExprValueKind::ConstEval>(template_arg);
				if(arg_expr_info.isError()){ return evo::resultError; }


				switch(arg_expr_info.value().value_type){
					case ExprInfo::ValueType::Import: case ExprInfo::ValueType::Templated:
					case ExprInfo::ValueType::Initializer: {
						this->emit_error(
							Diagnostic::Code::SemaIncorrectTemplateArgValueType,
							template_arg,
							"Invalid template argument"
						);
						return evo::resultError;
					} break;

					default: {
						if(this->type_check<true>(
							*template_param, arg_expr_info.value(), "Template parameter", template_arg
						).ok == false){
							return evo::resultError;
						}
					} break;
				}

				evo::debugAssert(
					arg_expr_info.value().value_type == ExprInfo::ValueType::Ephemeral,
					"consteval expr is not ephemeral"
				);

				instantiation_type_args.emplace_back(std::nullopt);

				switch(arg_expr_info.value().getExpr().kind()){
					case ASG::Expr::Kind::LiteralInt: {
						const ASG::LiteralInt::ID literal_id = arg_expr_info.value().getExpr().literalIntID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralInt(literal_id).value);
					} break;

					case ASG::Expr::Kind::LiteralFloat: {
						const ASG::LiteralFloat::ID literal_id = arg_expr_info.value().getExpr().literalFloatID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralFloat(literal_id).value);
					} break;

					case ASG::Expr::Kind::LiteralBool: {
						const ASG::LiteralBool::ID literal_id = arg_expr_info.value().getExpr().literalBoolID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralBool(literal_id).value);
					} break;

					case ASG::Expr::Kind::LiteralChar: {
						const ASG::LiteralChar::ID literal_id = arg_expr_info.value().getExpr().literalCharID();
						instantiation_args.emplace_back(this->source.getASGBuffer().getLiteralChar(literal_id).value);
					} break;

					default: {
						this->emit_fatal(
							Diagnostic::Code::SemaExpectedConstEvalValue,
							template_arg,
							Diagnostic::createFatalMessage("Evaluated consteval value was not actually consteval")
						);
						return evo::resultError;
					} break;
				}
				
			}else{ // templated param is type
				switch(template_arg.kind()){
					case AST::Kind::Type: {
						const AST::Type& arg_ast_type = this->source.getASTBuffer().getType(template_arg);
						const evo::Result<TypeInfo::VoidableID> arg_type = this->get_type_id(arg_ast_type);
						if(arg_type.isError()){ return evo::resultError; }

						instantiation_type_args.emplace_back(arg_type.value());
						instantiation_args.emplace_back(arg_type.value());
					} break;

					case AST::Kind::Ident: {
						const Token::ID arg_ast_type_token_id = this->source.getASTBuffer().getIdent(template_arg);
						const evo::Result<TypeInfo::VoidableID> arg_type = this->get_type_id(arg_ast_type_token_id);
						if(arg_type.isError()){ return evo::resultError; }

						instantiation_type_args.emplace_back(arg_type.value());
						instantiation_args.emplace_back(arg_type.value());
					} break;

					default: {
						this->emit_error(
							Diagnostic::Code::SemaIncorrectTemplateInstantiation,
							templated_expr.args[i],
							std::format("Expected type in template argument index {}", i)
						);
						return evo::resultError;
					} break;
				}
			}
		}

		const BaseType::Function instantiated_base_type = 
			templated_intrinsic.getTypeInstantiation(instantiation_type_args);

		const TypeInfo::ID instantiated_type = this->context.getTypeManager().getOrCreateTypeInfo(
			TypeInfo(
				this->context.getTypeManager().getOrCreateFunction(instantiated_base_type)
			)
		);


		if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
			return ExprInfo(
				ExprInfo::ValueType::Intrinsic, ExprInfo::generateExprInfoTypeIDs(instantiated_type), std::nullopt
			);
		}else{
			const ASG::TemplatedIntrinsicInstantiation::ID asg_templated_intrinsic_instantiation_id 
				= this->source.asg_buffer.createTemplatedIntrinsicInstantiation(
					templated_intrinsic_kind, std::move(instantiation_args)
				);

			return ExprInfo(
				ExprInfo::ValueType::Intrinsic,
				ExprInfo::generateExprInfoTypeIDs(instantiated_type),
				evo::SmallVector<ASG::Expr>{ASG::Expr(asg_templated_intrinsic_instantiation_id)}
			);
		}
	}



	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_prefix(const AST::Prefix& prefix) -> evo::Result<ExprInfo> {
		const evo::Result<ExprInfo> rhs_info = this->analyze_expr<EXPR_VALUE_KIND>(prefix.rhs);
		if(rhs_info.isError()){ return evo::resultError; }

		switch(this->source.getTokenBuffer()[prefix.opTokenID].kind()){
			case Token::lookupKind("&"):
				return this->analyze_expr_prefix_address_of<EXPR_VALUE_KIND, false>(prefix, rhs_info.value());

			case Token::lookupKind("&|"):
				return this->analyze_expr_prefix_address_of<EXPR_VALUE_KIND, true>(prefix, rhs_info.value());

			case Token::Kind::KeywordCopy: {
				if(rhs_info.value().is_concrete() == false){
					if(rhs_info.value().value_type == ExprInfo::ValueType::Function){
						this->emit_error(
							Diagnostic::Code::SemaCopyExprNotConcrete,
							prefix.rhs,
							"rhs of [copy] expression cannot be a function",
							Diagnostic::Info("To get a function pointer, use the address-of operator ([&])")
						);
					}else{
						this->emit_error(
							Diagnostic::Code::SemaCopyExprNotConcrete,
							prefix.rhs,
							"rhs of [copy] expression must be concrete"
						);
					}
					return evo::resultError;
				}

				if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
					return ExprInfo(ExprInfo::ValueType::Ephemeral, rhs_info.value().type_id, std::nullopt);
				}else{
					const ASG::Copy::ID asg_copy_id = this->source.asg_buffer.createCopy(rhs_info.value().getExpr());

					return ExprInfo(ExprInfo::ValueType::Ephemeral, rhs_info.value().type_id, ASG::Expr(asg_copy_id));
				}
			} break;

			case Token::Kind::KeywordMove: {
				if(rhs_info.value().is_concrete() == false){
					if(rhs_info.value().value_type == ExprInfo::ValueType::Function){
						this->emit_error(
							Diagnostic::Code::SemaMoveExprNotConcrete,
							prefix.rhs,
							"rhs of [move] expression cannot be a function",
							Diagnostic::Info("To get a function pointer, use the address-of operator ([&])")
						);
					}else{
						this->emit_error(
							Diagnostic::Code::SemaMoveExprNotConcrete,
							prefix.rhs,
							"rhs of [move] expression must be concrete"
						);
					}
					return evo::resultError;
				}

				if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
					return ExprInfo(ExprInfo::ValueType::Ephemeral, rhs_info.value().type_id, std::nullopt);
				}else{
					const ASG::Move::ID asg_move_id = this->source.asg_buffer.createMove(rhs_info.value().getExpr());

					return ExprInfo(ExprInfo::ValueType::Ephemeral, rhs_info.value().type_id, ASG::Expr(asg_move_id));
				}
			} break;

			case Token::lookupKind("-"): {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature, prefix, "prefix [-] expression is currently unsupported"
				);
				return evo::resultError;
			} break;

			case Token::lookupKind("!"): {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature, prefix, "prefix [!] expression is currently unsupported"
				);
				return evo::resultError;
			} break;

			case Token::lookupKind("~"): {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature, prefix, "prefix [~] expression is currently unsupported"
				);
				return evo::resultError;
			} break;


			default: {
				evo::debugFatalBreak("Unknown or unsupported infix operator");
			} break;
		}
	}


	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND, bool IS_CONST>
	auto SemanticAnalyzer::analyze_expr_prefix_address_of(const AST::Prefix& prefix, const ExprInfo& rhs_info)
	-> evo::Result<ExprInfo> {
		const bool rhs_is_function = rhs_info.value_type == ExprInfo::ValueType::Function;
		if(rhs_info.is_concrete() == false && rhs_is_function == false){
			this->emit_error(
				Diagnostic::Code::SemaInvalidAddrOfRHS,
				prefix.rhs,
				"RHS of an address-of expression ([&]) must be concrete or a function"
			);
			return evo::resultError;
		}

		if(rhs_info.type_id.as<evo::SmallVector<TypeInfo::ID>>().size() != 1){
			if(rhs_is_function){
				this->emit_error(
					Diagnostic::Code::SemaInvalidAddrOfRHS,
					prefix.rhs,
					"Cannot take an address-of ([&]) of a function that has overloads"
				);

			}else{
				// TODO: better messaging
				this->emit_error(
					Diagnostic::Code::SemaInvalidAddrOfRHS,
					prefix.rhs,
					"RHS of an address-of expression ([&]) must be have a single value"
				);
			}

			return evo::resultError;	
		}

		if constexpr(IS_CONST == false){
			if(rhs_info.value_type == ExprInfo::ValueType::ConcreteMutGlobal){
				const BaseType::ID current_func_base_type_id = this->get_current_func().baseTypeID;
				const BaseType::Function& current_func_base_type =
					this->context.getTypeManager().getFunction(current_func_base_type_id.funcID());

				if(rhs_info.is_const() == false && current_func_base_type.isRuntime() == false){
					// TODO: better messaging
					this->emit_error(
						Diagnostic::Code::SemaInvalidAddrOfRHS,
						prefix.rhs,
						"Cannot take address of a mutable global in a function that has the \"runtime\" attribute",
						Diagnostic::Info("Use \"&|\" instead of \"&\" to get a read-only pointer")
					);
					return evo::resultError;
				}
			}
		}


		const evo::Result<TypeInfo::ID> new_type_id = [&](){					
			const TypeInfo::ID rhs_type_id = 
				rhs_info.type_id.as<evo::SmallVector<TypeInfo::ID>>().front();

			if(rhs_is_function){ return evo::Result<TypeInfo::ID>(rhs_type_id);	}

			const TypeInfo& rhs_type = this->context.getTypeManager().getTypeInfo(rhs_type_id);

			auto rhs_type_qualifiers = evo::SmallVector<AST::Type::Qualifier>(
				rhs_type.qualifiers().begin(), rhs_type.qualifiers().end()
			);
			const bool is_read_only = rhs_info.is_const() || IS_CONST;
			rhs_type_qualifiers.emplace_back(true, is_read_only, false);

			if(this->check_type_qualifiers(rhs_type_qualifiers, prefix) == false){
				return evo::Result<TypeInfo::ID>(evo::resultError);
			}

			return evo::Result<TypeInfo::ID>(
				this->context.getTypeManager().getOrCreateTypeInfo(
					TypeInfo(rhs_type.baseTypeID(), std::move(rhs_type_qualifiers))
				)
			);
		}();
		if(new_type_id.isError()){ return evo::resultError; }


		if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
			return ExprInfo(
				ExprInfo::ValueType::Ephemeral,
				ExprInfo::generateExprInfoTypeIDs(new_type_id.value()),
				std::nullopt
			);
			
		}else{
			if(rhs_is_function) [[unlikely]] {
				return ExprInfo(
					ExprInfo::ValueType::Ephemeral,
					ExprInfo::generateExprInfoTypeIDs(new_type_id.value()),
					ASG::Expr(rhs_info.getExpr())
				);
			}else{
				const ASG::AddrOf::ID addr_of_id = this->source.asg_buffer.createAddrOf(
					ASG::Expr(rhs_info.getExpr())
				);
				return ExprInfo(
					ExprInfo::ValueType::Ephemeral,
					ExprInfo::generateExprInfoTypeIDs(new_type_id.value()),
					ASG::Expr(addr_of_id)
				);
			}
		}
	}



	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_infix(const AST::Infix& infix) -> evo::Result<ExprInfo> {
		switch(this->source.getTokenBuffer()[infix.opTokenID].kind()){
			case Token::lookupKind("."): {
				const evo::Result<ExprInfo> lhs_expr_info = this->analyze_expr<EXPR_VALUE_KIND>(infix.lhs);
				if(lhs_expr_info.isError()){ return evo::resultError; }

				if(lhs_expr_info.value().value_type != ExprInfo::ValueType::Import){
					this->emit_error(
						Diagnostic::Code::SemaUnsupportedOperator,
						infix,
						"This type does not support the accessor operator ([.])",
						Diagnostic::Info(std::format("Type of lhs: {}", this->print_type(lhs_expr_info.value())))
					);
					return evo::resultError;
				}

				const Token::ID rhs_ident_token_id = this->source.getASTBuffer().getIdent(infix.rhs);
				const std::string_view rhs_ident_str = this->source.getTokenBuffer()[rhs_ident_token_id].getString();

				const Source::ID import_target_source_id = lhs_expr_info.value().type_id.as<Source::ID>();
				const Source& import_target_source = this->context.getSourceManager()[import_target_source_id];

				const ScopeManager::Level& scope_level = this->context.getScopeManager()[
					import_target_source.global_scope_level
				];


				const ScopeManager::Level::IdentID* ident_id_lookup = scope_level.lookupIdent(rhs_ident_str);

				if(ident_id_lookup == nullptr){
					this->emit_error(
						Diagnostic::Code::SemaImportMemberDoesntExist,
						rhs_ident_token_id,
						std::format(
							"Imported source doesn't have identifier \"{}\" declared in global scope", rhs_ident_str
						)
					);
					return evo::resultError;
				}

				return ident_id_lookup->visit([&](const auto ident_id) -> evo::Result<ExprInfo> {
					using IdentID = std::decay_t<decltype(ident_id)>;

					if constexpr(std::is_same_v<IdentID, evo::SmallVector<ASG::Func::ID>>){ // functions
						auto type_ids = evo::SmallVector<TypeInfo::ID>();
						type_ids.reserve(ident_id.size());
						for(const ASG::FuncID& asg_func_id : ident_id){
							const ASG::Func& asg_func = import_target_source.getASGBuffer().getFunc(asg_func_id);

							if(asg_func.isPub == false){
								this->emit_error(
									Diagnostic::Code::SemaImportMemberIsntPub,
									rhs_ident_token_id,
									std::format("Function \"{}\" doesn't have the \"pub\" attribute", rhs_ident_str),
									Diagnostic::Info(
										"Function declared here:",
										this->get_source_location(
											ASG::Func::LinkID(import_target_source_id, asg_func_id)
										)
									)
								);
								return evo::resultError;
							}
							
							type_ids.emplace_back(this->context.getTypeManager().getOrCreateTypeInfo(
								TypeInfo(asg_func.baseTypeID)
							));
						}

						if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
							return ExprInfo(
								ExprInfo::ValueType::Function,
								ExprInfo::generateExprInfoTypeIDs(std::move(type_ids)),
								std::nullopt
							);
						}else{
							auto func_exprs = evo::SmallVector<ASG::Expr>();
							func_exprs.reserve(ident_id.size());
							for(const ASG::Func::ID& func_id : ident_id){
								func_exprs.emplace_back(ASG::Func::LinkID(import_target_source_id, func_id));
							}

							return ExprInfo(
								ExprInfo::ValueType::Function,
								ExprInfo::generateExprInfoTypeIDs(std::move(type_ids)),
								std::move(func_exprs)
							);
						}

					}else if constexpr(std::is_same_v<IdentID, ASG::TemplatedFunc::ID>){ // functions
						const ASG::TemplatedFunc& asg_templated_func = 
							import_target_source.getASGBuffer().getTemplatedFunc(ident_id);

						if(asg_templated_func.isPub == false){
							this->emit_error(
								Diagnostic::Code::SemaImportMemberIsntPub,
								rhs_ident_token_id,
								std::format(
									"Templated function \"{}\" doesn't have the \"pub\" attribute", rhs_ident_str
								)
							);
							return evo::resultError;
						}

						if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
							return ExprInfo(
								ExprInfo::ValueType::Templated,
								ASG::TemplatedFunc::LinkID(import_target_source_id, ident_id),
								std::nullopt
							);

						}else{
							this->emit_error(
								Diagnostic::Code::SemaExpectedTemplateArgs,
								rhs_ident_token_id,
								std::format(
									"Identifier \"{}\" is a templated function and requires template arguments",
									rhs_ident_str
								),
								Diagnostic::Info("Defined here:", this->get_source_location(ident_id))
							);
							return evo::resultError;
						}

					}else if constexpr(
						std::is_same_v<IdentID, ASG::VarID> ||
						std::is_same_v<IdentID, ASG::ParamID> ||
						std::is_same_v<IdentID, ASG::ReturnParamID> ||
						std::is_same_v<IdentID, ScopeManager::Level::ImportInfo>
					){
						evo::debugFatalBreak("Unsupported import kind");
						
					}else{
						static_assert(false, "Unknown or unsupported import kind");
					}
				});

			} break;

			default: {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					infix.opTokenID,
					std::format(
						"Infix [{}] expressions are currently unsupported", 
						this->source.getTokenBuffer()[infix.opTokenID].kind()
					)
				);
				return evo::resultError;
			} break;
		}
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_postfix(const AST::Postfix& postfix) -> evo::Result<ExprInfo> {
		switch(this->source.getTokenBuffer()[postfix.opTokenID].kind()){
			case Token::lookupKind(".*"): {
				const evo::Result<ExprInfo> lhs_info = this->analyze_expr<EXPR_VALUE_KIND>(postfix.lhs);
				if(lhs_info.isError()){ return evo::resultError; }

				if(
					lhs_info.value().type_id.is<evo::SmallVector<TypeInfo::ID>>() == false || 
					lhs_info.value().type_id.as<evo::SmallVector<TypeInfo::ID>>().size() > 1
				){
					this->emit_error(
						Diagnostic::Code::SemaInvalidDerefRHS,
						postfix.lhs,
						"Cannot dereference a value that is not a pointer"
					);
					return evo::resultError;
				}



				const TypeInfo& lhs_type = this->context.getTypeManager().getTypeInfo(
					lhs_info.value().type_id.as<evo::SmallVector<TypeInfo::ID>>().front()
				);
				if(lhs_type.isPointer() == false){
					this->emit_error(
						Diagnostic::Code::SemaInvalidDerefRHS,
						postfix.lhs,
						"Cannot dereference a value that is not a pointer"
					);
					return evo::resultError;
				}

				const auto qualifiers = evo::SmallVector<AST::Type::Qualifier>(
					lhs_type.qualifiers().begin(), --lhs_type.qualifiers().end()
				);

				const TypeInfo::ID new_type_id = this->context.getTypeManager().getOrCreateTypeInfo(
					TypeInfo(lhs_type.baseTypeID(), std::move(qualifiers))
				);


				const ExprInfo::ValueType value_type = lhs_type.qualifiers().back().isReadOnly
														? ExprInfo::ValueType::ConcreteConst  
														: ExprInfo::ValueType::ConcreteMut;

				if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
					return ExprInfo(value_type, ExprInfo::generateExprInfoTypeIDs(new_type_id), std::nullopt);

				}else{
					const ASG::Deref::ID asg_deref_id = this->source.asg_buffer.createDeref(
						lhs_info.value().getExpr(), new_type_id
					);
					return ExprInfo(
						value_type, ExprInfo::generateExprInfoTypeIDs(new_type_id), ASG::Expr(asg_deref_id)
					);
				}

			} break;

			case Token::lookupKind(".?"): {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					postfix.opTokenID,
					"Postfix [.?] expressions are currently unsupported"
				);
				return evo::resultError;
			} break;

			default: {
				this->emit_fatal(
					Diagnostic::Code::MiscInvalidKind,
					postfix.opTokenID,
					Diagnostic::createFatalMessage("Unknown or unsupported postfix operator")
				);
				return evo::resultError;
			} break;
		}
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_ident(const Token::ID& ident) -> evo::Result<ExprInfo> {
		const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();

		// template exprs
		const auto template_expr_find = this->template_arg_exprs.find(ident_str);
		if(template_expr_find != this->template_arg_exprs.end()){
			if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
				const ExprInfo& template_expr_info = template_expr_find->second;
				return ExprInfo(template_expr_info.value_type, template_expr_info.type_id, std::nullopt);
			}else{
				return template_expr_find->second;
			}
		}

		for(size_t i = this->scope.size() - 1; ScopeManager::Level::ID scope_level_id : this->scope){
			const evo::Result<std::optional<ExprInfo>> scope_level_lookup = 
				this->analyze_expr_ident_in_scope_level<EXPR_VALUE_KIND>(
					this->source.getID(),
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



	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_ident_in_scope_level(
		Source::ID source_id,
		const Token::ID& ident,
		std::string_view ident_str,
		ScopeManager::Level::ID scope_level_id,
		bool variables_in_scope,
		bool is_global
	) -> evo::Result<std::optional<ExprInfo>> {
		const ScopeManager::Level& scope_level = this->context.getScopeManager()[scope_level_id];

		const ScopeManager::Level::IdentID* ident_id_lookup = scope_level.lookupIdent(ident_str);
		if(ident_id_lookup == nullptr){ return std::optional<ExprInfo>(); }


		return ident_id_lookup->visit([&](const auto ident_id) -> evo::Result<std::optional<ExprInfo>> {
			using IdentID = std::decay_t<decltype(ident_id)>;

			if constexpr(std::is_same_v<IdentID, evo::SmallVector<ASG::Func::ID>>){ // functions
				auto type_ids = evo::SmallVector<TypeInfo::ID>();
				type_ids.reserve(ident_id.size());
				for(const ASG::FuncID& asg_func_id : ident_id){
					const ASG::Func& asg_func = this->source.getASGBuffer().getFunc(asg_func_id);
					
					type_ids.emplace_back(this->context.getTypeManager().getOrCreateTypeInfo(
						TypeInfo(asg_func.baseTypeID)
					));
				}

				if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
					return std::optional<ExprInfo>(
						ExprInfo(
							ExprInfo::ValueType::Function,
							ExprInfo::generateExprInfoTypeIDs(std::move(type_ids)),
							std::nullopt
						)
					);
				}else{
					auto func_exprs = evo::SmallVector<ASG::Expr>();
					func_exprs.reserve(ident_id.size());
					for(const ASG::Func::ID& func_id : ident_id){
						func_exprs.emplace_back(ASG::Func::LinkID(source_id, func_id));
					}

					return std::optional<ExprInfo>(
						ExprInfo(
							ExprInfo::ValueType::Function,
							ExprInfo::generateExprInfoTypeIDs(std::move(type_ids)),
							std::move(func_exprs)
						)
					);
				}

			}else if constexpr(std::is_same_v<IdentID, ASG::TemplatedFunc::ID>){ // templated functions
				if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
					return std::optional<ExprInfo>(
						ExprInfo(
							ExprInfo::ValueType::Templated,
							ASG::TemplatedFunc::LinkID(source_id, ident_id),
							std::nullopt
						)
					);

				}else{
					this->emit_error(
						Diagnostic::Code::SemaExpectedTemplateArgs,
						ident,
						std::format(
							"Identifier \"{}\" is a templated function and requires template arguments", ident_str
						),
						Diagnostic::Info("Defined here:", this->get_source_location(ident_id))
					);
					return evo::resultError;
				}


			}else if constexpr(std::is_same_v<IdentID, ASG::Var::ID>){ // variables
				if(variables_in_scope){
					const ASG::Var& asg_var = this->source.getASGBuffer().getVar(ident_id);

					auto get_value_type = [&](){
						switch(asg_var.kind){
							case AST::VarDecl::Kind::Var: {
								if(is_global){
									return ExprInfo::ValueType::ConcreteMutGlobal;
								}else{
									return ExprInfo::ValueType::ConcreteMut;
								}
							}
							case AST::VarDecl::Kind::Const: return ExprInfo::ValueType::ConcreteConst;
							case AST::VarDecl::Kind::Def:   return ExprInfo::ValueType::Ephemeral;
						}

						evo::debugFatalBreak("Unknown or unsupported AST::VarDecl::Kind");
					};

					if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
						return std::optional<ExprInfo>(
							ExprInfo(get_value_type(), ExprInfo::generateExprInfoTypeIDs(asg_var.typeID), std::nullopt)
						);

					}else if constexpr(EXPR_VALUE_KIND == ExprValueKind::ConstEval){
						if(asg_var.kind != AST::VarDecl::Kind::Def){
							// TODO: better messaging
							this->emit_error(
								Diagnostic::Code::SemaConstEvalVarNotDef,
								ident,
								"Cannot get a consteval value from a variable that isn't def",
								Diagnostic::Info("Declared here:", this->get_source_location(ident_id))
							);
							return evo::resultError;
						}

						return std::optional<ExprInfo>(
							ExprInfo(
								ExprInfo::ValueType::Ephemeral,
								ExprInfo::generateExprInfoTypeIDs(asg_var.typeID),
								/*copy*/ ASG::Expr(asg_var.expr)
							)
						);

					}else{
						ASG::Expr asg_expr = [&](){
							if(asg_var.kind == AST::VarDecl::Kind::Def){
								return /*copy*/ ASG::Expr(asg_var.expr);	
							}else{
								return ASG::Expr(ASG::Var::LinkID(source_id, ident_id));
							}
						}();
						return std::optional<ExprInfo>(
							ExprInfo(
								get_value_type(),
								ExprInfo::generateExprInfoTypeIDs(asg_var.typeID),
								std::move(asg_expr)
							)
						);
					}
				}else{
					// TODO: better messaging
					this->emit_error(
						Diagnostic::Code::SemaIdentNotInScope,
						ident,
						std::format("Identifier \"{}\" was not defined in this scope", ident_str),
						Diagnostic::Info(
							"Local variables, parameters, and members cannot be accessed inside a sub-object scope. "
							"Defined here:",
							this->get_source_location(ident_id)
						)
					);
					return evo::resultError;
				}

			}else if constexpr(std::is_same_v<IdentID, ASG::Param::ID>){ // parameters
				if(variables_in_scope){
					const ASG::Param& asg_param = this->source.getASGBuffer().getParam(ident_id);
					const ASG::Func& asg_func = this->source.getASGBuffer().getFunc(asg_param.func);

					const BaseType::Function& func_type = this->context.getTypeManager().getFunction(
						asg_func.baseTypeID.funcID()
					);

					const BaseType::Function::Param& param = func_type.params()[asg_param.index];

					auto get_value_type = [&](){
						switch(param.kind){
							case AST::FuncDecl::Param::Kind::Read: return ExprInfo::ValueType::ConcreteConst;
							case AST::FuncDecl::Param::Kind::Mut:  return ExprInfo::ValueType::ConcreteMut;
							case AST::FuncDecl::Param::Kind::In:   return ExprInfo::ValueType::ConcreteMut;
						}

						evo::debugFatalBreak("Unknown or unsupported AST::FuncDecl::Param::Kind");
					};


					if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
						return std::optional<ExprInfo>(
							ExprInfo(get_value_type(), ExprInfo::generateExprInfoTypeIDs(param.typeID), std::nullopt)
						);

					}else if constexpr(EXPR_VALUE_KIND == ExprValueKind::ConstEval){
						this->emit_error(
							Diagnostic::Code::SemaParamsCannotBeConstEval,
							ident,
							"Cannot get a consteval value from a parameter",
							Diagnostic::Info("Declared here:", this->get_source_location(ident_id))
						);
						return evo::resultError;

					}else{
						return std::optional<ExprInfo>(
							ExprInfo(
								get_value_type(),
								ExprInfo::generateExprInfoTypeIDs(param.typeID),
								ASG::Expr(
									ASG::Param::LinkID(
										ASG::Func::LinkID(this->source.getID(), asg_param.func), ident_id
									)
								)
							)
						);
					}
				}else{
					// TODO: better messaging
					this->emit_error(
						Diagnostic::Code::SemaIdentNotInScope,
						ident,
						std::format("Identifier \"{}\" was not defined in this scope", ident_str),
						Diagnostic::Info(
							"Local variables, parameters, and members cannot be accessed inside a sub-object scope. ",
							this->get_source_location(ident_id)
						)
					);
					return evo::resultError;
				}


			}else if constexpr(std::is_same_v<IdentID, ASG::ReturnParam::ID>){ // return parameters
				if(variables_in_scope){
					const ASG::ReturnParam& asg_ret_param = this->source.getASGBuffer().getReturnParam(ident_id);
					const ASG::Func& asg_func = this->source.getASGBuffer().getFunc(asg_ret_param.func);

					const BaseType::Function& func_type = this->context.getTypeManager().getFunction(
						asg_func.baseTypeID.funcID()
					);

					const BaseType::Function::ReturnParam& return_param = func_type.returnParams()[asg_ret_param.index];

					if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
						return std::optional<ExprInfo>(
							ExprInfo(
								ExprInfo::ValueType::ConcreteMut, 
								ExprInfo::generateExprInfoTypeIDs(return_param.typeID.typeID()),
								std::nullopt
							)
						);

					}else if constexpr(EXPR_VALUE_KIND == ExprValueKind::ConstEval){
						this->emit_error(
							Diagnostic::Code::SemaParamsCannotBeConstEval,
							ident,
							"Cannot get a consteval value from a parameter",
							Diagnostic::Info("Declared here:", this->get_source_location(ident_id))
						);
						return evo::resultError;

					}else{
						return std::optional<ExprInfo>(
							ExprInfo(
								ExprInfo::ValueType::ConcreteMut,
								ExprInfo::generateExprInfoTypeIDs(return_param.typeID.typeID()),
								ASG::Expr(
									ASG::ReturnParam::LinkID(
										ASG::Func::LinkID(this->source.getID(), asg_ret_param.func), ident_id
									)
								)
							)
						);
					}
				}else{
					// TODO: better messaging
					this->emit_error(
						Diagnostic::Code::SemaIdentNotInScope,
						ident,
						std::format("Identifier \"{}\" was not defined in this scope", ident_str),
						Diagnostic::Info(
							"Local variables, parameters, and members cannot be accessed inside a sub-object scope. ",
							this->get_source_location(ident_id)
						)
					);
					return evo::resultError;
				}

			}else if constexpr(std::is_same_v<IdentID, ScopeManager::Level::ImportInfo>){ // parameters
				return std::optional<ExprInfo>(
					ExprInfo(ExprInfo::ValueType::Import, ident_id.sourceID, std::nullopt)
				);

			}else{
				static_assert(false, "Unknown or unsupported ScopeManager::Level::IdentID kind");
			}
		});
	}




	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_intrinsic(const Token::ID& intrinsic_token_id) -> evo::Result<ExprInfo> {
		const std::string_view intrinsic_name = this->source.getTokenBuffer()[intrinsic_token_id].getString();

		const IntrinsicLookupTable::IntrinKind intrinsic_lookup = intrinsic_lookup_table.lookup(intrinsic_name);
		if(intrinsic_lookup.is<std::monostate>()){
			this->emit_error(
				Diagnostic::Code::SemaIntrinsicDoesntExist,
				intrinsic_token_id,
				std::format("Intrinsic \"@{}\" doesn't exist", intrinsic_name)
			);
			return evo::resultError;
		}

		return intrinsic_lookup.visit([&](auto intrin_kind) -> ExprInfo {
			using IntrinKindT = std::decay_t<decltype(intrin_kind)>;

			if constexpr(std::is_same_v<IntrinKindT, Intrinsic::Kind>){
				const Intrinsic& intrinsic = this->context.getIntrinsic(intrin_kind);
				const TypeInfo::ID intrinsic_type = this->context.getTypeManager().getOrCreateTypeInfo(
					TypeInfo(intrinsic.baseType)
				);

				if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
					return ExprInfo(
						ExprInfo::ValueType::Intrinsic, ExprInfo::generateExprInfoTypeIDs(intrinsic_type), std::nullopt
					);
				}else{
					return ExprInfo(
						ExprInfo::ValueType::Intrinsic,
						ExprInfo::generateExprInfoTypeIDs(intrinsic_type),
						ASG::Expr(intrin_kind)
					);
				}
				
			}else if constexpr(std::is_same_v<IntrinKindT, TemplatedIntrinsic::Kind>){
				if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
					return ExprInfo(ExprInfo::ValueType::TemplatedIntrinsic, intrin_kind, std::nullopt);
				}else{
					evo::debugFatalBreak("Cannot get a value from a templated intrinsic without template arguments");
				}

			}else{
				evo::debugFatalBreak("Intrinsic not existing should have been caught");
			}
		});
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_literal(const Token::ID& literal) -> evo::Result<ExprInfo> {
		const Token& token = this->source.getTokenBuffer()[literal];

		auto value_type = ExprInfo::ValueType::Ephemeral;
		auto type_id = ExprInfo::TypeID(std::monostate());
		auto expr = evo::SmallVector<ASG::Expr>();

		switch(token.kind()){
			case Token::Kind::LiteralInt: {
				value_type = ExprInfo::ValueType::EpemeralFluid;

				if constexpr(EXPR_VALUE_KIND != ExprValueKind::None){
					expr.emplace_back(
						this->source.asg_buffer.createLiteralInt(
							core::GenericInt::create<uint64_t>(token.getInt()),
							std::nullopt
						)
					);
				}
			} break;

			case Token::Kind::LiteralFloat: {
				value_type = ExprInfo::ValueType::EpemeralFluid;

				if constexpr(EXPR_VALUE_KIND != ExprValueKind::None){
					expr.emplace_back(
						this->source.asg_buffer.createLiteralFloat(core::GenericFloat(token.getFloat()), std::nullopt)
					);
				}
			} break;

			case Token::Kind::LiteralBool: {
				type_id = ExprInfo::generateExprInfoTypeIDs(this->context.getTypeManager().getTypeBool());

				if constexpr(EXPR_VALUE_KIND != ExprValueKind::None){
					expr.emplace_back(this->source.asg_buffer.createLiteralBool(token.getBool()));
				}
			} break;

			case Token::Kind::LiteralString: {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature, literal, "literal strings are currently unsupported"
				);
				return evo::resultError;
			} break;

			case Token::Kind::LiteralChar: {
				type_id = ExprInfo::generateExprInfoTypeIDs(this->context.getTypeManager().getTypeChar());

				if constexpr(EXPR_VALUE_KIND != ExprValueKind::None){
					expr.emplace_back(this->source.asg_buffer.createLiteralChar(token.getString()[0]));
				}
			} break;

			default: {
				evo::debugFatalBreak("Token is not a literal");
			} break;
		}

		return ExprInfo(value_type, std::move(type_id), std::move(expr));
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_uninit(const Token::ID& uninit) -> evo::Result<ExprInfo> {
		if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
			return ExprInfo(ExprInfo::ValueType::Initializer, std::monostate(), std::nullopt);
		}else{
			const ASG::Uninit::ID uninit_id = this->source.asg_buffer.createUninit(uninit);
			return ExprInfo(ExprInfo::ValueType::Initializer, std::monostate(), ASG::Expr(uninit_id));
		}
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_zeroinit(const Token::ID& zeroinit) -> evo::Result<ExprInfo> {
		if constexpr(EXPR_VALUE_KIND == ExprValueKind::None){
			return ExprInfo(ExprInfo::ValueType::Initializer, std::monostate(), std::nullopt);
		}else{
			const ASG::Zeroinit::ID zeroinit_id = this->source.asg_buffer.createZeroinit(zeroinit);
			return ExprInfo(ExprInfo::ValueType::Initializer, std::monostate(), ASG::Expr(zeroinit_id));
		}
	}

	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND>
	auto SemanticAnalyzer::analyze_expr_this(const Token::ID& this_expr) -> evo::Result<ExprInfo> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, this_expr, "[this] expressions are currently unsupported"
		);
		return evo::resultError;
	}



	template<SemanticAnalyzer::ExprValueKind EXPR_VALUE_KIND, bool IS_STMT>
	auto SemanticAnalyzer::analyze_func_call_impl(const AST::FuncCall& func_call) -> evo::Result<AnalyzedFuncCallData> {
		///////////////////////////////////
		// get function and check callable

		const evo::Result<ExprInfo> target_info_res = this->analyze_expr<EXPR_VALUE_KIND>(func_call.target);
		if(target_info_res.isError()){ return evo::resultError; }

		if(target_info_res.value().type_id.is<evo::SmallVector<TypeInfo::ID>>() == false){
			this->emit_error(
				Diagnostic::Code::SemaCannotCallLikeFunction,
				func_call.target,
				"Cannot call this expression like a function"
			);
			return evo::resultError;
		}


		///////////////////////////////////
		// get base type(s)

		const evo::SmallVector<TypeInfo::ID> target_type_ids =
			target_info_res.value().type_id.as<evo::SmallVector<TypeInfo::ID>>();

		auto target_type_infos = evo::SmallVector<const TypeInfo*>();
		target_type_infos.reserve(target_type_ids.size());
		for(const TypeInfo::ID& target_type_id : target_type_ids){
			target_type_infos.emplace_back(&this->context.getTypeManager().getTypeInfo(target_type_id));
		}


		if(target_type_infos.size() == 1){
			if(
				target_type_infos.front()->qualifiers().empty() == false ||
				target_type_infos.front()->baseTypeID().kind() != BaseType::Kind::Function
			){
				this->emit_error(
					Diagnostic::Code::SemaCannotCallLikeFunction,
					func_call.target,
					"Cannot call this expression like a function"
				);
				return evo::resultError;
			}

		}else if(target_info_res.value().value_type != ExprInfo::ValueType::Function){
			this->emit_error(
				Diagnostic::Code::SemaCannotCallLikeFunction,
				func_call.target,
				"Cannot call this expression like a function"
			);
			return evo::resultError;
		}


		auto func_types = evo::SmallVector<const BaseType::Function*>();
		func_types.reserve(target_type_infos.size());
		for(const TypeInfo* target_type_info : target_type_infos){
			func_types.emplace_back(
				&this->context.getTypeManager().getFunction(target_type_info->baseTypeID().funcID())
			);
		}


		///////////////////////////////////
		// check argument expressions

		auto arg_infos = evo::SmallVector<ArgInfo>();
		arg_infos.reserve(func_call.args.size());
		for(const AST::FuncCall::Arg& arg : func_call.args){
			const evo::Result<ExprInfo> arg_info = this->analyze_expr<EXPR_VALUE_KIND>(arg.value);
			if(arg_info.isError()){ return evo::resultError; }

			if(arg_info.value().value_type == ExprInfo::ValueType::Initializer){
				this->emit_error(
					Diagnostic::Code::SemaInvalidUseOfInitializerValueExpr,
					arg.value,
					"Initializer values cannot be used as function call arguments"
				);
				return evo::resultError;
			}


			if(
				arg_info.value().type_id.is<evo::SmallVector<TypeInfo::ID>>() &&
				arg_info.value().type_id.as<evo::SmallVector<TypeInfo::ID>>().size() > 1
			){
				this->emit_error(
					Diagnostic::Code::SemaMultipleValuesIntoOne,
					arg.value,
					"Cannot pass multiple values into a single function argument"
				);
				return evo::resultError;
			}

			arg_infos.emplace_back(arg.explicitIdent, arg_info.value(), arg.value);
		}


		///////////////////////////////////
		// select overload

		auto potential_func_link_ids = evo::SmallVector<ASG::Func::LinkID>();
		potential_func_link_ids.reserve(target_info_res.value().numExprs());
		for(const ASG::Expr& func_expr : target_info_res.value().getExprList()){
			switch(func_expr.kind()){
				case ASG::Expr::Kind::Func: {
					potential_func_link_ids.emplace_back(func_expr.funcLinkID());
				} break;

				case ASG::Expr::Kind::Var: {
					const ASG::Var::LinkID asg_var_link_id = func_expr.varLinkID();
					const Source& target_source = this->context.getSourceManager()[asg_var_link_id.sourceID()];
					const ASG::Var& asg_var = target_source.getASGBuffer().getVar(asg_var_link_id.varID());
					potential_func_link_ids.emplace_back(asg_var.expr.funcLinkID());
				} break;

				case ASG::Expr::Kind::Intrinsic:
				case ASG::Expr::Kind::TemplatedIntrinsicInstantiation: {
					// No func link id to add
				} break;

				default: {
					evo::debugFatalBreak("Cannot get function call from this ASG::Expr type");
				} break;
			}
		}

		const evo::Result<size_t> selected_func_overload_index = [&](){
			if(potential_func_link_ids.empty()){ // is intrinsic
				return this->select_func_overload<true>(func_call, nullptr, func_types, arg_infos, func_call.args);
				
			}else{ // not intrinsic
				return this->select_func_overload<false>(
					func_call, potential_func_link_ids, func_types, arg_infos, func_call.args
				);
			}
		}();

		if(selected_func_overload_index.isError()){ return evo::resultError; }


		///////////////////////////////////
		// check function return value

		if constexpr(IS_STMT){
			if(func_types[selected_func_overload_index.value()]->returnParams()[0].typeID.isVoid() == false){
				// TODO: better messaging - #mayDiscard
				this->emit_error(
					Diagnostic::Code::SemaDiscardingFuncReturn,
					func_call.target,
					"Discarding the return value of a function"
				);
				return evo::resultError;
			}
		}else{
			if(func_types[selected_func_overload_index.value()]->returnParams()[0].typeID.isVoid()){
				this->emit_error(
					Diagnostic::Code::SemaFuncDoesntReturnValue,
					func_call.target,
					"Function doesn't return a value"
				);
				return evo::resultError;
			}
		}


		///////////////////////////////////
		// check runtime attribute

		if(func_types[selected_func_overload_index.value()]->isRuntime()){
			const BaseType::ID current_func_base_type_id = this->get_current_func().baseTypeID;
			const BaseType::Function& current_func_base_type =
				this->context.getTypeManager().getFunction(current_func_base_type_id.funcID());
			
			if(current_func_base_type.isRuntime() == false){
				this->emit_error(
					Diagnostic::Code::SemaCantCallRuntimeFuncInComptimeContext,
					func_call,
					"Cannot call a function with the \"runtime\" attribute from a function that does not have it"
				);
				return evo::resultError;
			}
		}


		///////////////////////////////////
		// organize arguments

		auto args = evo::SmallVector<ASG::Expr>();
		args.reserve(arg_infos.size());
		for(const ArgInfo& arg_info : arg_infos){
			args.emplace_back(arg_info.expr_info.getExpr());
		}


		///////////////////////////////////
		// create func call

		auto asg_func_call_id = std::optional<ASG::FuncCall::ID>();

		if constexpr(EXPR_VALUE_KIND != ExprValueKind::None){
			if(potential_func_link_ids.empty()){
				const ASG::Expr& selected_expr = target_info_res.value().getExpr(selected_func_overload_index.value());
				
				if(selected_expr.kind() == ASG::Expr::Kind::Intrinsic){
					asg_func_call_id = this->source.asg_buffer.createFuncCall(
						selected_expr.intrinsicID(), std::move(args)
					);
				}else{
					evo::debugAssert(
						selected_expr.kind() == ASG::Expr::Kind::TemplatedIntrinsicInstantiation,
						"Unknown or unsupported intrinsic kind"
					);
					asg_func_call_id = this->source.asg_buffer.createFuncCall(
						selected_expr.templatedIntrinsicInstantiationID(), std::move(args)
					);
				}

			}else{
				const ASG::Func::LinkID target_link_id = potential_func_link_ids[selected_func_overload_index.value()];
				asg_func_call_id = this->source.asg_buffer.createFuncCall(target_link_id, std::move(args));

				const bool is_current_func_runtime = [&](){
					const BaseType::ID current_func_base_type_id = this->get_current_func().baseTypeID;
					const BaseType::Function& current_func_base_type =
						this->context.getTypeManager().getFunction(current_func_base_type_id.funcID());
					return current_func_base_type.isRuntime();
				}();

				if(is_current_func_runtime == false){
					Source& target_source = this->context.getSourceManager()[target_link_id.sourceID()];
					const ASG::Func& target_asg_func = target_source.getASGBuffer().getFunc(target_link_id.funcID());

					evo::SmallVector<ASG::Func::LinkID> comptime_call_stack_copy = this->comptime_call_stack;
					comptime_call_stack_copy.emplace_back(
						this->source.getID(), this->scope.getCurrentObjectScope().as<ASG::Func::ID>()
					);

					for(const ASG::Func::LinkID& comptime_caller : comptime_call_stack_copy){
						if(comptime_caller == target_link_id){
							// TODO: better message
							this->emit_error(
								Diagnostic::Code::SemaComptimeCircularDependency,
								func_call,
								"Comptime circular dependency detected"
							);

							return evo::resultError;
						}
					}

					auto func_sema = SemanticAnalyzer(
						this->context,
						target_source,
						*target_asg_func.scope,
						evo::SmallVector<SourceLocation>(this->template_parents),
						std::move(comptime_call_stack_copy)
					);

					if(func_sema.analyze_func_body<false>(target_asg_func.ast_func, target_link_id.funcID()) == false){
						return evo::resultError;
					}

					if(target_asg_func.is_body_errored){
						this->get_current_func().is_body_errored = true;
					}
				}
			}
		}

		///////////////////////////////////
		// done

		return AnalyzedFuncCallData(func_types[selected_func_overload_index.value()], asg_func_call_id);
	};



	template<bool IS_INTRINSIC, typename NODE_T>
	auto SemanticAnalyzer::select_func_overload(
		const NODE_T& location,
		evo::ArrayProxy<ASG::Func::LinkID> asg_funcs,
		evo::ArrayProxy<const BaseType::Function*> funcs,
		evo::SmallVector<ArgInfo>& arg_infos,
		evo::ArrayProxy<AST::FuncCall::Arg> args
	) -> evo::Result<size_t> {
		evo::debugAssert(funcs.empty() == false, "need at least 1 func");
		if constexpr(IS_INTRINSIC == false){
			evo::debugAssert(asg_funcs.size() == funcs.size(), "mismatched size of `asg_funcs` and `funcs`");
		}
		evo::debugAssert(arg_infos.size() == args.size(), "mismatched size of `arg_infos` and `args`");

		struct OverloadScore{
			struct NumMismatch{};
			struct TypeMismatch        { size_t arg_index; };
			struct ValueKindMismatch   { size_t arg_index; };
			struct MutGlobalNotRuntime { size_t arg_index; bool calling_runtime; bool target_runtime; };
			struct IncorrectLabel      { size_t arg_index; };
			struct LackingLabel        { size_t arg_index; };

			using Reason = evo::Variant<
				std::monostate,
				NumMismatch,
				TypeMismatch,
				ValueKindMismatch,
				MutGlobalNotRuntime,
				IncorrectLabel,
				LackingLabel
			>;

			evo::uint score;
			Reason reason;

			OverloadScore(evo::uint _score) : score(_score), reason(std::monostate()) {};
			OverloadScore(Reason _reason) : score(0), reason(_reason) {};

		};
		auto scores = evo::SmallVector<OverloadScore>();
		scores.reserve(funcs.size());

		for(size_t i = 0; const BaseType::Function* func_type_ptr : funcs){
			EVO_DEFER([&](){ i += 1; });

			bool failed = false;

			const BaseType::Function& func_type = *func_type_ptr;

			if(arg_infos.size() != func_type.params().size()){
				scores.emplace_back(OverloadScore::NumMismatch());
				continue;
			}

			evo::uint current_score = 0;

			for(size_t param_i = 0; const BaseType::Function::Param& param : func_type.params()){
				ArgInfo& arg_info = arg_infos[param_i];

				const TypeCheckInfo& type_check_info = this->type_check<false>(
					param.typeID, arg_info.expr_info, "Parameter", arg_info.ast_node
				);

				if(type_check_info.ok == false){
					scores.emplace_back(OverloadScore::TypeMismatch(param_i));
					failed = true;
					break;
				}

				switch(param.kind){
					case AST::FuncDecl::Param::Kind::Read: {
						// accepts any value type
					} break;

					case AST::FuncDecl::Param::Kind::Mut: {
						if(arg_info.expr_info.value_type == ExprInfo::ValueType::ConcreteMutGlobal){
							const bool is_target_func_runtime = func_type.isRuntime();
							const bool is_calling_func_runtime = [&](){
								const BaseType::ID current_func_base_type_id = this->get_current_func().baseTypeID;
								const BaseType::Function& current_func_base_type =
									this->context.getTypeManager().getFunction(current_func_base_type_id.funcID());
								return current_func_base_type.isRuntime();
							}();

							if(is_target_func_runtime && is_calling_func_runtime){
								current_score += 1;
							}else{
								scores.emplace_back(
									OverloadScore::MutGlobalNotRuntime(
										param_i, !is_calling_func_runtime, !is_target_func_runtime
									)
								);
								failed = true;	
							}

						}else if(arg_info.expr_info.value_type != ExprInfo::ValueType::ConcreteMut){
							scores.emplace_back(OverloadScore::ValueKindMismatch(param_i));
							failed = true;

						}else{
							current_score += 1;
						}

					} break;

					case AST::FuncDecl::Param::Kind::In: {
						if(arg_info.expr_info.is_ephemeral() == false){
							scores.emplace_back(OverloadScore::ValueKindMismatch(param_i));
							failed = true;
						}
					} break;
				}

				if(failed){ break; }

				const std::string_view param_ident = param.ident.visit([&](const auto& param_ident_id){
					if constexpr(std::is_same_v<std::decay_t<decltype(param_ident_id)>, Token::ID>){
						const Source& func_source = this->context.getSourceManager()[asg_funcs[i].sourceID()];
						return func_source.getTokenBuffer()[param_ident_id].getString();
					}else{
						return strings::toStringView(param_ident_id);
					}
				});

				if(args[param_i].explicitIdent.has_value()){
					const std::string_view arg_label =
						this->source.getTokenBuffer()[*args[param_i].explicitIdent].getString();

					if(param_ident != arg_label){
						scores.emplace_back(OverloadScore::IncorrectLabel(param_i));
						failed = true;
						break;
					}
				}else{
					if(param.mustLabel){
						scores.emplace_back(OverloadScore::LackingLabel(param_i));
						failed = true;
						break;
					}
				}

				if(type_check_info.requires_implicit_conversion == false){
					current_score += 1;
				}
			
				current_score += 1;

				param_i += 1;
			}

			if(failed){ continue; }

			scores.emplace_back(current_score + 1);
		}

		const OverloadScore* best_score = nullptr;
		bool found_best_score_match = false;
		for(const OverloadScore& score : scores){
			if(score.score > 0){
				if(best_score == nullptr){
					best_score = &score;

				}else if(best_score->score == score.score){
					found_best_score_match = true;

				}else if(best_score->score < score.score){
					best_score = &score;
					found_best_score_match = false;
				}
			}
		}

		if(found_best_score_match){
			auto infos = evo::SmallVector<Diagnostic::Info>();
			infos.reserve(2);
			for(size_t i = std::numeric_limits<size_t>::max(); const OverloadScore& score : scores){
				i += 1;

				if constexpr(IS_INTRINSIC == false){
					if(score.score == best_score->score){
						infos.emplace_back("Could be this one:", this->get_source_location(asg_funcs[i]));
					}
				}
			}

			this->emit_error(
				Diagnostic::Code::SemaMultipleMatchingFunctions,
				location,
				"Multiple matching functions",
				std::move(infos)
			);
			return evo::resultError;

		}else if(best_score == nullptr){
			auto infos = evo::SmallVector<Diagnostic::Info>();

			for(size_t i = 0; const OverloadScore& score : scores){
				const std::string_view fail_match_message = "Failed to match:";

				score.reason.visit([&](const auto& reason) -> void {
					using ReasonT = std::decay_t<decltype(reason)>;

					if constexpr(std::is_same_v<ReasonT, std::monostate>){
						evo::fatalBreak("std::monostate marks a passing match - should not have a score of 0");

					}else if constexpr(std::is_same_v<ReasonT, OverloadScore::NumMismatch>){
						infos.emplace_back(
							std::format(
								"{} mismatched number of arguments (expected {}, got {})",
								fail_match_message,
								arg_infos.size(),
								funcs[i]->params().size()
							),
							[&](){
								if constexpr(IS_INTRINSIC){
									return std::nullopt;
								}else{
									return this->get_source_location(asg_funcs[i]);
								}
							}()
						);

					}else if constexpr(std::is_same_v<ReasonT, OverloadScore::TypeMismatch>){
						infos.emplace_back(
							std::format(
								"{} mismatched types at argument index {}",
								fail_match_message,
								reason.arg_index
							),
							funcs[i]->params()[reason.arg_index].ident.visit([&](auto param_ident){
								if constexpr(std::is_same_v<std::decay_t<decltype(param_ident)>, Token::ID>){
									return std::optional<Source::Location>(this->get_source_location(
										param_ident, this->context.getSourceManager()[asg_funcs[i].sourceID()]
									));
								}else{
									return std::optional<Source::Location>();
								}
							}),
							std::vector<Diagnostic::Info>{
								std::format(
									"Parameter is of type: {}",
									this->context.getTypeManager().printType(
										funcs[i]->params()[reason.arg_index].typeID
									)
								),
								std::format(
									"Argument is of type:  {}", this->print_type(arg_infos[reason.arg_index].expr_info)
								),
							}
						);

					}else if constexpr(std::is_same_v<ReasonT, OverloadScore::ValueKindMismatch>){
						const AST::FuncDecl::Param::Kind param_kind = funcs[i]->params()[reason.arg_index].kind;

						switch(param_kind){
							case AST::FuncDecl::Param::Kind::Read: {
								evo::debugFatalBreak("Should not error on a read param");
							} break;

							case AST::FuncDecl::Param::Kind::Mut: {
								infos.emplace_back(
									std::format(
										"{} [mut] parameters require concrete mutable expressions", fail_match_message
									),
									funcs[i]->params()[reason.arg_index].ident.visit([&](auto param_ident){
										if constexpr(std::is_same_v<std::decay_t<decltype(param_ident)>, Token::ID>){
											return std::optional<Source::Location>(this->get_source_location(
												param_ident, this->context.getSourceManager()[asg_funcs[i].sourceID()]
											));
										}else{
											return std::optional<Source::Location>();
										}
									})
								);
							} break;

							case AST::FuncDecl::Param::Kind::In: {
								infos.emplace_back(
									std::format("{} [in] parameters require ephemeral expressions", fail_match_message),
									funcs[i]->params()[reason.arg_index].ident.visit([&](auto param_ident){
										if constexpr(std::is_same_v<std::decay_t<decltype(param_ident)>, Token::ID>){
											return std::optional<Source::Location>(this->get_source_location(
												param_ident, this->context.getSourceManager()[asg_funcs[i].sourceID()]
											));
										}else{
											return std::optional<Source::Location>();
										}
									})
								);
							} break;
						}

					}else if constexpr(std::is_same_v<ReasonT, OverloadScore::MutGlobalNotRuntime>){
						const std::string_view not_runtime_message = [&](){
							if(reason.target_runtime){
								if(reason.calling_runtime){
									return "cannot mutate a global variable in a function that "
                                            "doesn't have the runtime attribute, and "
											"cannot pass a global variable to a [mut] parameter "
											"when the target function does not have the runtime attribute";
								}else{
									return "cannot pass a global variable to a [mut] parameter "
											"when the target function does not have the runtime attribute";
								}
							}else{
								evo::debugAssert(
									reason.calling_runtime, "cannot be that both target and calling are runtime"
								);

								return "cannot mutate a global variable in a function that "
									    "doesn't have the runtime attribute";
							}	
						}();

						infos.emplace_back(
							std::format(
								"{} {} (argument index {})",
								fail_match_message,
								not_runtime_message,
								reason.arg_index
							),
							this->get_source_location(args[reason.arg_index].value)
						);

					}else if constexpr(std::is_same_v<ReasonT, OverloadScore::IncorrectLabel>){
						infos.emplace_back(
							std::format(
								"{} incorrect label at argument index {}",
								fail_match_message,
								reason.arg_index
							),
							this->get_source_location(*args[reason.arg_index].explicitIdent)
						);

					}else if constexpr(std::is_same_v<ReasonT, OverloadScore::LackingLabel>){
						infos.emplace_back(
							std::format(
								"{} requrires label at argument index {}",
								fail_match_message,
								reason.arg_index
							),
							this->get_source_location(args[reason.arg_index].value)
						);
					}
				});

				i += 1;
			}

			this->emit_error(
				Diagnostic::Code::SemaNoMatchingFunction, location, "No matching function", std::move(infos)
			);
			return evo::resultError;
		}

		const size_t matched_index = size_t(best_score - scores.data());
		const BaseType::Function& matched_func_type = *funcs[matched_index];

		for(size_t i = 0; ArgInfo& arg_info : arg_infos){
			if(this->type_check<true>( // this is to implicitly convert all the required args
				matched_func_type.params()[i].typeID, arg_info.expr_info, "Parameter", arg_info.ast_node
			).ok == false){
				evo::debugFatalBreak("This should not be able to fail");
			}
		
			i += 1;
		}

		return matched_index;
	}



	auto SemanticAnalyzer::ConditionalAttribute::check(
		SemanticAnalyzer& sema, const AST::AttributeBlock::Attribute& attribute, std::string_view attr_name
	) -> bool {
		if(this->_was_used){
			sema.emit_error(
				Diagnostic::Code::SemaAttributeAlreadySet,
				attribute,
				std::format("Attribute `#{}` was already used", attr_name)
			);
			return false;
		}else{
			this->_was_used = true;
		}

		if(attribute.args.empty()){
			_is_set = true;
			return true;

		}else if(attribute.args.size() == 1){
			evo::Result<ExprInfo> cond = sema.analyze_expr<ExprValueKind::ConstEval>(attribute.args.front());
			if(cond.isError()){ return false; }

			const TypeInfo::ID bool_type_id = sema.context.getTypeManager().getTypeBool();
			if(
				sema.type_check<true>(
					bool_type_id,
					cond.value(),
					std::format("attribute `#{}` condition argument", attr_name),
					attribute.args.front()
				).ok == false
			){
				return false;
			}

			const bool cond_value = [&](){
				const ASG::Expr& expr = cond.value().getExpr();
				const ASG::LiteralBool& literal_bool 
					= sema.source.getASGBuffer().getLiteralBool(expr.literalBoolID());
				return literal_bool.value;
			}();

			_is_set = cond_value;
			return true;

		}else{
			sema.emit_error(
				Diagnostic::Code::SemaInvalidAttributeArgument,
				attribute.args.back(),
				"Invalid argument in attribute `#pub`"
			);
			return false;
		}
	}




	//////////////////////////////////////////////////////////////////////
	// error handling

	template<bool IMPLICITLY_CONVERT, typename NODE_T>
	auto SemanticAnalyzer::type_check(
		TypeInfo::ID expected_type_id, ExprInfo& got_expr, std::string_view name, const NODE_T& location
	) -> TypeCheckInfo {
		switch(got_expr.value_type){
			case ExprInfo::ValueType::ConcreteConst:
			case ExprInfo::ValueType::ConcreteMut:
			case ExprInfo::ValueType::ConcreteMutGlobal:
			case ExprInfo::ValueType::Ephemeral: {
				if(got_expr.type_id.as<evo::SmallVector<TypeInfo::ID>>().size() != 1){
					auto name_copy = std::string(name);
					name_copy[0] = char(std::tolower(int(name_copy[0])));

					this->emit_error(
						Diagnostic::Code::SemaMultipleValuesIntoOne,
						location,
						std::format("Cannot set {} with multiple values", name_copy)
					);
					return TypeCheckInfo(false, false);
				}

				const TypeInfo::ID got_type_id = got_expr.type_id.as<evo::SmallVector<TypeInfo::ID>>().front();

				if(expected_type_id != got_type_id){
					const TypeInfo& expected_type = this->context.getTypeManager().getTypeInfo(expected_type_id);
					const TypeInfo& got_type      = this->context.getTypeManager().getTypeInfo(got_type_id);

					if(
						expected_type.baseTypeID()        != got_type.baseTypeID() || 
						expected_type.qualifiers().size() != got_type.qualifiers().size()
					){	

						if constexpr(IMPLICITLY_CONVERT){
							this->error_type_mismatch(expected_type_id, got_expr, name, location);
						}
						return TypeCheckInfo(false, false);
					}

					// TODO: optimze this?
					for(size_t i = 0; i < expected_type.qualifiers().size(); i+=1){
						const AST::Type::Qualifier& expected_qualifier = expected_type.qualifiers()[i];
						const AST::Type::Qualifier& got_qualifier      = got_type.qualifiers()[i];

						if(expected_qualifier.isPtr != got_qualifier.isPtr){
							if constexpr(IMPLICITLY_CONVERT){
								this->error_type_mismatch(expected_type_id, got_expr, name, location);
							}
							return TypeCheckInfo(false, false);
						}
						if(expected_qualifier.isReadOnly == false && got_qualifier.isReadOnly){
							if constexpr(IMPLICITLY_CONVERT){
								this->error_type_mismatch(expected_type_id, got_expr, name, location);
							}
							return TypeCheckInfo(false, false);
						}
					}
				}

				if constexpr(IMPLICITLY_CONVERT){
					const auto _ = evo::Defer([&](){
						got_expr.type_id.emplace<evo::SmallVector<TypeInfo::ID>>({expected_type_id});
					});
				}

				return TypeCheckInfo(
					true, got_expr.type_id.as<evo::SmallVector<TypeInfo::ID>>().front() != expected_type_id
				);
			} break;


			case ExprInfo::ValueType::EpemeralFluid: {
				const TypeInfo& expected_type_info = this->context.getTypeManager().getTypeInfo(expected_type_id);

				if(
					expected_type_info.qualifiers().empty() == false || 
					expected_type_info.baseTypeID().kind() != BaseType::Kind::Builtin
				){
					if constexpr(IMPLICITLY_CONVERT){
						this->error_type_mismatch(expected_type_id, got_expr, name, location);
					}
					return TypeCheckInfo(false, false);
				}


				const BaseType::Builtin::ID expected_type_builtin_id = expected_type_info.baseTypeID().builtinID();

				const BaseType::Builtin& expected_type_builtin = 
					this->context.getTypeManager().getBuiltin(expected_type_builtin_id);

				if(got_expr.getExpr().kind() == ASG::Expr::Kind::LiteralInt){
					switch(expected_type_builtin.kind()){
						case Token::Kind::TypeInt:
						case Token::Kind::TypeISize:
						case Token::Kind::TypeI_N:
						case Token::Kind::TypeUInt:
						case Token::Kind::TypeUSize:
						case Token::Kind::TypeUI_N:
						case Token::Kind::TypeByte:
						case Token::Kind::TypeCShort:
						case Token::Kind::TypeCUShort:
						case Token::Kind::TypeCInt:
						case Token::Kind::TypeCUInt:
						case Token::Kind::TypeCLong:
						case Token::Kind::TypeCULong:
						case Token::Kind::TypeCLongLong:
						case Token::Kind::TypeCULongLong:
						case Token::Kind::TypeCLongDouble:
							break;

						default: {
							if constexpr(IMPLICITLY_CONVERT){
								this->error_type_mismatch(expected_type_id, got_expr, name, location);
							}
							return TypeCheckInfo(false, false);
						}
					}

					if constexpr(IMPLICITLY_CONVERT){
						const ASG::LiteralInt::ID literal_int_id = got_expr.getExpr().literalIntID();
						this->source.asg_buffer.literal_ints[literal_int_id].typeID = expected_type_id;
					}


				}else{
					evo::debugAssert(
						got_expr.getExpr().kind() == ASG::Expr::Kind::LiteralFloat, "Expected literal float"
					);

					switch(expected_type_builtin.kind()){
						case Token::Kind::TypeF16:
						case Token::Kind::TypeBF16:
						case Token::Kind::TypeF32:
						case Token::Kind::TypeF64:
						case Token::Kind::TypeF80:
						case Token::Kind::TypeF128:
							break;

						default: {
							if constexpr(IMPLICITLY_CONVERT){
								this->error_type_mismatch(expected_type_id, got_expr, name, location);
							}
							return TypeCheckInfo(false, false);
						}
					}

					if constexpr(IMPLICITLY_CONVERT){
						const ASG::LiteralFloat::ID literal_float_id = got_expr.getExpr().literalFloatID();
						this->source.asg_buffer.literal_floats[literal_float_id].typeID = expected_type_id;
					}
				}

				if constexpr(IMPLICITLY_CONVERT){
					got_expr.value_type = ExprInfo::ValueType::Ephemeral;
					got_expr.type_id.emplace<evo::SmallVector<TypeInfo::ID>>({expected_type_id});
				}

				return TypeCheckInfo(true, true);
			} break;


			case ExprInfo::ValueType::Import: {
				evo::debugFatalBreak("Imports should not be compared with this function");
			} break;
			
			case ExprInfo::ValueType::Templated: {
				evo::debugFatalBreak("Templated types should not be compared with this function");
			} break;

			case ExprInfo::ValueType::Initializer: {
				evo::debugFatalBreak("Initializer types should not be compared with this function");
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported ExprInfo::ValueType");
	}


	template<typename NODE_T>
	auto SemanticAnalyzer::error_type_mismatch(
		TypeInfo::ID expected_type_id, const ExprInfo& got_expr, std::string_view name, const NODE_T& location

	) -> void {
		std::string expected_type_str = std::format("{} is of type: ", name);
		auto got_type_str = std::string("Expression is of type: ");

		while(expected_type_str.size() < got_type_str.size()){
			expected_type_str += ' ';
		}

		while(got_type_str.size() < expected_type_str.size()){
			got_type_str += ' ';
		}

		auto infos = evo::SmallVector<Diagnostic::Info>();
		infos.emplace_back(expected_type_str + this->context.getTypeManager().printType(expected_type_id));
		infos.emplace_back(got_type_str + this->print_type(got_expr));

		this->emit_error(
			Diagnostic::Code::SemaTypeMismatch,
			location,
			std::format("{} cannot accept an expression of a different type, and cannot be implicitly converted", name),
			std::move(infos)
		);
	}

	template<typename NODE_T>
	auto SemanticAnalyzer::check_type_qualifiers(
		evo::ArrayProxy<AST::Type::Qualifier> qualifiers, const NODE_T& location
	) -> bool {
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



	template<typename NODE_T>
	auto SemanticAnalyzer::already_defined(std::string_view ident_str, const NODE_T& node) const -> bool {
		// template types
		const auto template_type_find = this->template_arg_types.find(ident_str);
		if(template_type_find != this->template_arg_types.end()){
			this->emit_error(
				Diagnostic::Code::SemaAlreadyDefined,
				node,
				std::format("Identifier \"{}\" was already defined in this scope", ident_str),
				evo::SmallVector<Diagnostic::Info>{
					// TODO: definition location
					Diagnostic::Info(std::format("\"{}\" is a template parameter", ident_str)),
					Diagnostic::Info("Note: shadowing is not allowed")
				}
			);
			return true;
		}

		// template exprs
		const auto template_expr_find = this->template_arg_exprs.find(ident_str);
		if(template_expr_find != this->template_arg_exprs.end()){
			this->emit_error(
				Diagnostic::Code::SemaAlreadyDefined,
				node,
				std::format("Identifier \"{}\" was already defined in this scope", ident_str),
				evo::SmallVector<Diagnostic::Info>{
					// TODO: definition location
					Diagnostic::Info(std::format("\"{}\" is a template parameter", ident_str)),
					Diagnostic::Info("Note: shadowing is not allowed")
				}
			);
			return true;
		}


		for(size_t i = this->scope.size() - 1; ScopeManager::Level::ID scope_level_id : this->scope){
			const ScopeManager::Level& scope_level = this->context.getScopeManager()[scope_level_id];

			const ScopeManager::Level::IdentID* lookup_ident_id = scope_level.lookupIdent(ident_str);

			if(lookup_ident_id != nullptr){
				lookup_ident_id->visit([&](const auto ident_id) -> void {
					auto infos = evo::SmallVector<Diagnostic::Info>();

					using IdentIDType = std::decay_t<decltype(ident_id)>;

					if constexpr(std::is_same_v<IdentIDType, evo::SmallVector<ASG::FuncID>>){
						if(ident_id.size() == 1){
							infos.emplace_back("First defined here:", this->get_source_location(ident_id.front()));

						}else if(ident_id.size() == 2){
							infos.emplace_back(
								"First defined here (and 1 other place):", this->get_source_location(ident_id.front())
							);
						}else{
							infos.emplace_back(
								std::format("First defined here (and {} other places):", ident_id.size() - 1),
								this->get_source_location(ident_id.front())
							);
						}

					}else{
						infos.emplace_back("First defined here:", this->get_source_location(ident_id));
					}


					if(scope_level_id != this->scope.getCurrentLevel()){
						infos.emplace_back("Note: shadowing is not allowed");
					}

					this->emit_error(
						Diagnostic::Code::SemaAlreadyDefined,
						node,
						std::format("Identifier \"{}\" was already defined in this scope", ident_str),
						std::move(infos)
					);
				});

				return true;
			}
			i -= 1;
		}

		return false;
	}



	auto SemanticAnalyzer::may_recover() const -> bool {
		return !this->context.hasHitFailCondition() && this->context.getConfig().mayRecover;
	}


	auto SemanticAnalyzer::print_type(const ExprInfo& expr_info) const -> std::string {
		if(expr_info.type_id.is<evo::SmallVector<TypeInfo::ID>>()){
			if(expr_info.type_id.as<evo::SmallVector<TypeInfo::ID>>().size() == 1){
				return this->context.getTypeManager().printType(
					expr_info.type_id.as<evo::SmallVector<TypeInfo::ID>>().front()
				);
			}else{
				return "{MULTIPLE}";
			}

		}else if(expr_info.value_type == ExprInfo::ValueType::Import){
			return "{IMPORT}";

		}else{
			evo::debugAssert(expr_info.value_type == ExprInfo::ValueType::EpemeralFluid, "expected fluid literal");

			if(expr_info.hasExpr()){
				if(expr_info.getExpr().kind() == ASG::Expr::Kind::LiteralInt){
					return "{LITERAL INTEGER}";
					
				}else{
					evo::debugAssert(
						expr_info.getExpr().kind() == ASG::Expr::Kind::LiteralFloat, "expected literal float"
					);
					return "{LITERAL FLOAT}";
				}

			}else{
				return "{LITERAL NUMBER}";
			}
		}
	}



	template<typename NODE_T>
	auto SemanticAnalyzer::emit_fatal(
		Diagnostic::Code code,
		const NODE_T& location,
		std::string&& msg,
		evo::SmallVector<Diagnostic::Info>&& infos
	) const -> void {
		this->add_template_location_infos(infos);
		this->context.emitFatal(code, this->get_source_location(location), std::move(msg), std::move(infos));
	}

	template<typename NODE_T>
	auto SemanticAnalyzer::emit_error(
		Diagnostic::Code code,
		const NODE_T& location,
		std::string&& msg,
		evo::SmallVector<Diagnostic::Info>&& infos
	) const -> void {
		this->add_template_location_infos(infos);
		this->context.emitError(code, this->get_source_location(location), std::move(msg), std::move(infos));
	}

	template<typename NODE_T>
	auto SemanticAnalyzer::emit_warning(
		Diagnostic::Code code,
		const NODE_T& location,
		std::string&& msg,
		evo::SmallVector<Diagnostic::Info>&& infos
	) const -> void {
		this->add_template_location_infos(infos);
		this->context.emitWarning(code, this->get_source_location(location), std::move(msg), std::move(infos));
	}


	auto SemanticAnalyzer::add_template_location_infos(evo::SmallVector<Diagnostic::Info>& infos) const -> void {
		if(this->template_parents.empty()){
			return;

		}else if(this->template_parents.size() == 1){
			infos.emplace_back("template instantiated here:", this->template_parents[0]);

		}else{
			for(size_t i = 0; const SourceLocation& template_parent : this->template_parents){
				infos.emplace_back(std::format("template instantiated here (depth: {}):", i), template_parent);
			
				i += 1;
			}
		}
	}


	//////////////////////////////////////////////////////////////////////
	// get source location

	auto SemanticAnalyzer::get_source_location(std::nullopt_t) const -> std::optional<SourceLocation> {
		return std::nullopt;
	};


	auto SemanticAnalyzer::get_source_location(Token::ID token_id, const Source& src) const -> SourceLocation {
		return src.getTokenBuffer().getSourceLocation(token_id, src.getID());
	}
	
	auto SemanticAnalyzer::get_source_location(const AST::Node& node, const Source& src) const -> SourceLocation {
		#if defined(EVO_COMPILER_MSVC)
			#pragma warning(default : 4062)
		#endif

		const ASTBuffer& ast_buffer = src.getASTBuffer();

		switch(node.kind()){
			case AST::Kind::None:            evo::debugFatalBreak("Cannot get location of AST::Kind::None");
			case AST::Kind::VarDecl:         return this->get_source_location(ast_buffer.getVarDecl(node), src);
			case AST::Kind::FuncDecl:        return this->get_source_location(ast_buffer.getFuncDecl(node), src);
			case AST::Kind::AliasDecl:       return this->get_source_location(ast_buffer.getAliasDecl(node), src);
			case AST::Kind::Return:          return this->get_source_location(ast_buffer.getReturn(node), src);
			case AST::Kind::Unreachable:     return this->get_source_location(ast_buffer.getUnreachable(node), src);
			case AST::Kind::Conditional:     return this->get_source_location(ast_buffer.getConditional(node), src);
			case AST::Kind::WhenConditional: return this->get_source_location(ast_buffer.getWhenConditional(node), src);
			case AST::Kind::Block:           return this->get_source_location(ast_buffer.getBlock(node), src);
			case AST::Kind::FuncCall:        return this->get_source_location(ast_buffer.getFuncCall(node), src);
			case AST::Kind::TemplatePack:    evo::debugFatalBreak("Cannot get location of AST::Kind::TemplatePack");
			case AST::Kind::TemplatedExpr:   return this->get_source_location(ast_buffer.getTemplatedExpr(node), src);
			case AST::Kind::Prefix:          return this->get_source_location(ast_buffer.getPrefix(node), src);
			case AST::Kind::Infix:           return this->get_source_location(ast_buffer.getInfix(node), src);
			case AST::Kind::Postfix:         return this->get_source_location(ast_buffer.getPostfix(node), src);
			case AST::Kind::MultiAssign:     return this->get_source_location(ast_buffer.getMultiAssign(node), src);
			case AST::Kind::Type:            return this->get_source_location(ast_buffer.getType(node), src);
			case AST::Kind::AttributeBlock:  evo::debugFatalBreak("Cannot get location of AST::Kind::AttributeBlock");
			case AST::Kind::Attribute:       return this->get_source_location(ast_buffer.getAttribute(node), src);
			case AST::Kind::BuiltinType:     return this->get_source_location(ast_buffer.getBuiltinType(node), src);
			case AST::Kind::Ident:           return this->get_source_location(ast_buffer.getIdent(node), src);
			case AST::Kind::Intrinsic:       return this->get_source_location(ast_buffer.getIntrinsic(node), src);
			case AST::Kind::Literal:         return this->get_source_location(ast_buffer.getLiteral(node), src);
			case AST::Kind::Uninit:          return this->get_source_location(ast_buffer.getUninit(node), src);
			case AST::Kind::Zeroinit:        return this->get_source_location(ast_buffer.getZeroinit(node), src);
			case AST::Kind::This:            return this->get_source_location(ast_buffer.getThis(node), src);
			case AST::Kind::Discard:         return this->get_source_location(ast_buffer.getDiscard(node), src);
		}

		evo::debugFatalBreak("Unknown or unsupported AST::Kind");
	}


	auto SemanticAnalyzer::get_source_location(const AST::VarDecl& var_decl, const Source& src) const
	-> SourceLocation {
		return this->get_source_location(var_decl.ident, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::FuncDecl& func_decl, const Source& src) const
	-> SourceLocation {
		return this->get_source_location(func_decl.name, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::AliasDecl& alias_decl, const Source& src) const
	-> SourceLocation {
		return this->get_source_location(alias_decl.ident, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::Return& return_stmt, const Source& src) const
	-> SourceLocation {
		return this->get_source_location(return_stmt.keyword, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::Conditional& conditional, const Source& src) const
	-> SourceLocation {
		return this->get_source_location(conditional.keyword, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::WhenConditional& when_cond, const Source& src) const
	-> SourceLocation {
		return this->get_source_location(when_cond.keyword, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::Block& block, const Source& src) const -> SourceLocation {
		return this->get_source_location(block.openBrace, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::FuncCall& func_call, const Source& src) const
	-> SourceLocation {
		return this->get_source_location(func_call.target, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::TemplatedExpr& templated_expr, const Source& src) const
	-> SourceLocation {
		return this->get_source_location(templated_expr.base, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::Prefix& prefix, const Source& src) const -> SourceLocation {
		return this->get_source_location(prefix.opTokenID, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::Infix& infix, const Source& src) const -> SourceLocation {
		const Token& infix_op_token = src.getTokenBuffer()[infix.opTokenID];
		if(infix_op_token.kind() == Token::lookupKind(".")){
			return this->get_source_location(infix.rhs, src);
		}else{
			return this->get_source_location(infix.opTokenID, src);
		}
	}

	auto SemanticAnalyzer::get_source_location(const AST::Postfix& postfix, const Source& src) const -> SourceLocation {
		return this->get_source_location(postfix.opTokenID, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::MultiAssign& multi_assign, const Source& src) const
	-> SourceLocation {
		return this->get_source_location(multi_assign.openBracketLocation, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::Type& type, const Source& src) const -> SourceLocation {
		return this->get_source_location(type.base, src);
	}

	auto SemanticAnalyzer::get_source_location(const AST::AttributeBlock::Attribute& attr, const Source& src) const
	-> SourceLocation {
		return this->get_source_location(attr.attribute, src);
	}



	auto SemanticAnalyzer::get_source_location(ASG::Func::ID func_id, const Source& src) const -> SourceLocation {
		const ASG::Func& asg_func = src.getASGBuffer().getFunc(func_id);
		return this->get_source_location(asg_func.name, src);
	}

	auto SemanticAnalyzer::get_source_location(ASG::Func::LinkID func_link_id) const -> SourceLocation {
		const Source& lookup_source = this->context.getSourceManager()[func_link_id.sourceID()];
		const ASG::Func& asg_func = lookup_source.getASGBuffer().getFunc(func_link_id.funcID());

		evo::debugAssert(asg_func.name.kind() == AST::Kind::Ident, "func name was assumed to be ident");
		const Token::ID ident_token_id = lookup_source.getASTBuffer().getIdent(asg_func.name);
		return lookup_source.getTokenBuffer().getSourceLocation(ident_token_id, func_link_id.sourceID());
	}

	auto SemanticAnalyzer::get_source_location(ASG::TemplatedFunc::ID templated_func_id, const Source& src) const
	-> SourceLocation {
		const ASG::TemplatedFunc& asg_templated_func = src.getASGBuffer().getTemplatedFunc(templated_func_id);
		return this->get_source_location(asg_templated_func.funcDecl.name, src);
	}


	auto SemanticAnalyzer::get_source_location(ASG::Var::ID var_id, const Source& src) const -> SourceLocation {
		const ASG::Var& asg_var = src.getASGBuffer().getVar(var_id);
		return this->get_source_location(asg_var.ident, src);
	}

	auto SemanticAnalyzer::get_source_location(ASG::Param::ID param_id, const Source& src) const -> SourceLocation {
		const ASG::Param& asg_param = src.getASGBuffer().getParam(param_id);
		const ASG::Func& asg_func = src.getASGBuffer().getFunc(asg_param.func);

		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(asg_func.baseTypeID.funcID());

		evo::debugAssert(
			func_type.params()[asg_param.index].ident.is<Token::ID>(), "Cannot get location of intrinsic param ident"
		);

		return this->get_source_location(func_type.params()[asg_param.index].ident.as<Token::ID>(), src);
	}

	auto SemanticAnalyzer::get_source_location(ASG::ReturnParam::ID ret_param_id, const Source& src) const
	-> SourceLocation {
		const ASG::ReturnParam& asg_ret_param = src.getASGBuffer().getReturnParam(ret_param_id);
		const ASG::Func& asg_func = src.getASGBuffer().getFunc(asg_ret_param.func);

		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(asg_func.baseTypeID.funcID());

		return this->get_source_location(*func_type.returnParams()[asg_ret_param.index].ident, src);
	}

	auto SemanticAnalyzer::get_source_location(ScopeManager::Level::ImportInfo import_info, const Source& src) const
	-> SourceLocation {
		return this->get_source_location(import_info.tokenID, src);
	}


}