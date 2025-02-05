////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SemanticAnalyzer.h"

#include <queue>


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther{

	using Instruction = SymbolProc::Instruction;

	class Attribute{
		public:
			Attribute(SemanticAnalyzer& _sema, std::string_view _name) : sema(_sema), name(_name) {}
			~Attribute() = default;

			EVO_NODISCARD auto is_set() const -> bool {
				return this->set_location.has_value() || this->implicitly_set_location.has_value();
			}

			EVO_NODISCARD auto set(Token::ID location) -> bool {
				if(this->set_location.has_value()){
					this->sema.emit_error(
						Diagnostic::Code::SemaAttributeAlreadySet,
						location,
						std::format("Attribute #{} was already set", this->name),
						Diagnostic::Info(
							"First set here:", Diagnostic::Location::get(this->set_location.value(), this->sema.source)
						)
					);
					return false;
				}

				if(this->implicitly_set_location.has_value()){
					// TODO: make this warning turn-off-able in settings
					this->sema.emit_warning(
						Diagnostic::Code::SemaAttributeImplictSet,
						location,
						std::format("Attribute #{} was already implicitly set", this->name),
						Diagnostic::Info(
							"Implicitly set here:",
							Diagnostic::Location::get(this->implicitly_set_location.value(), this->sema.source)
						)
					);
					return true;
				}

				this->set_location = location;
				return true;
			}

			EVO_NODISCARD auto implicitly_set(Token::ID location) -> void {
				if(this->set_location.has_value()){
					// TODO: make this warning turn-off-able in settings
					this->sema.emit_warning(
						Diagnostic::Code::SemaAttributeImplictSet,
						this->set_location.value(),
						std::format("Attribute #{} was implicitly set", this->name),
						Diagnostic::Info(
							"Implicitly set here:", Diagnostic::Location::get(location, this->sema.source)
						)
					);
					return;
				}

				evo::debugAssert(
					this->implicitly_set_location.has_value() == false,
					"Attribute #{} already implicitly set. Should this be handled? Design changed?",
					this->name
				);

				this->implicitly_set_location = location;
			}
	
		private:
			SemanticAnalyzer& sema;
			std::string_view name;
			std::optional<Token::ID> set_location{};
			std::optional<Token::ID> implicitly_set_location{};
	};




	class ConditionalAttribute{
		public:
			ConditionalAttribute(SemanticAnalyzer& _sema, std::string_view _name) : sema(_sema), name(_name) {}
			~ConditionalAttribute() = default;

			EVO_NODISCARD auto is_set() const -> bool {
				return this->is_set_true || this->implicitly_set_location.has_value();
			}

			EVO_NODISCARD auto set(Token::ID location, bool cond) -> bool {
				if(this->set_location.has_value()){
					this->sema.emit_error(
						Diagnostic::Code::SemaAttributeAlreadySet,
						location,
						std::format("Attribute #{} was already set", this->name),
						Diagnostic::Info(
							"First set here:", Diagnostic::Location::get(this->set_location.value(), this->sema.source)
						)
					);
					return false;
				}

				if(this->implicitly_set_location.has_value()){
					// TODO: make this warning turn-off-able in settings
					this->sema.emit_warning(
						Diagnostic::Code::SemaAttributeImplictSet,
						location,
						std::format("Attribute #{} was already implicitly set", this->name),
						Diagnostic::Info(
							"Implicitly set here:",
							Diagnostic::Location::get(this->implicitly_set_location.value(), this->sema.source)
						)
					);
					return true;
				}

				this->is_set_true = cond;
				this->set_location = location;
				return true;
			}

			EVO_NODISCARD auto implicitly_set(Token::ID location) -> void {
				if(this->set_location.has_value()){
					if(this->is_set_true){
						// TODO: make this warning turn-off-able in settings
						this->sema.emit_warning(
							Diagnostic::Code::SemaAttributeImplictSet,
							this->set_location.value(),
							std::format("Attribute #{} was implicitly set", this->name),
							Diagnostic::Info(
								"Implicitly set here:", Diagnostic::Location::get(location, this->sema.source)
							)
						);
						return;
					}else{
						this->sema.emit_error(
							Diagnostic::Code::SemaAttributeAlreadySet,
							this->set_location.value(),
							std::format("Attribute #{} was implicitly set", this->name),
							Diagnostic::Info(
								"Implicitly set here:", Diagnostic::Location::get(location, this->sema.source)
							)
						);
						return;
					}

				}

				evo::debugAssert(
					this->implicitly_set_location.has_value() == false,
					"Attribute #{} already implicitly set. Should this be handled? Design changed?",
					this->name
				);

				this->implicitly_set_location = location;
			}
	
		private:
			SemanticAnalyzer& sema;
			std::string_view name;

			bool is_set_true = false;
			std::optional<Token::ID> set_location{};
			std::optional<Token::ID> implicitly_set_location{};
	};





	//////////////////////////////////////////////////////////////////////
	// semantic analyzer


	auto SemanticAnalyzer::analyze() -> void {
		while(this->symbol_proc.isAtEnd() == false){
			switch(this->analyze_instr(this->symbol_proc.getInstruction())){
				case Result::Success: break;

				case Result::Error: {
					this->context.symbol_proc_manager.symbol_proc_done();
					return;
				} break;

				case Result::NeedToWait: {
					return;
				} break;
			}
			this->symbol_proc.nextInstruction();
		}

		this->context.trace("==> Finished semantic analysis of symbol: \"{}\"", this->symbol_proc.ident);

		const auto lock = std::scoped_lock(this->symbol_proc.waiting_lock);

		this->symbol_proc.def_done = true;

		for(const SymbolProc::ID& def_waited_on_id : this->symbol_proc.def_waited_on_by){
			SymbolProc& def_waited_on = this->context.symbol_proc_manager.getSymbolProc(def_waited_on_id);

			for(size_t i = 0; i < def_waited_on.waiting_for.size(); i+=1){
				if(def_waited_on.waiting_for[i] == this->symbol_proc_id){
					if(i + 1 < def_waited_on.waiting_for.size()){
						def_waited_on.waiting_for[i] = def_waited_on.waiting_for.back();
					}
				}

				def_waited_on.waiting_for.pop_back();

				if(def_waited_on.waiting_for.empty()){
					this->context.add_task_to_work_manager(def_waited_on_id);
				}
			}
		}

		this->context.symbol_proc_manager.symbol_proc_done();
	}


	auto SemanticAnalyzer::analyze_instr(const Instruction& instruction) -> Result {
		return instruction.visit([&](const auto& instr) -> Result {
			using InstrType = std::decay_t<decltype(instr)>;

			if constexpr(std::is_same<InstrType, Instruction::GlobalWhenCond>()){
				return this->instr_global_when_cond(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::GlobalVarDecl>()){
				return this->instr_global_var_decl(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::GlobalVarDef>()){
				return this->instr_global_var_def(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::GlobalVarDeclDef>()){
				return this->instr_global_var_decl_def(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncCall>()){
				return this->instr_func_call(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Import>()){
				return this->instr_import(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::ComptimeExprAccessor>()){
				return this->instr_expr_accessor<true>(instr.infix, instr.lhs, instr.rhs_ident, instr.output);

			}else if constexpr(std::is_same<InstrType, Instruction::ExprAccessor>()){
				return this->instr_expr_accessor<false>(instr.infix, instr.lhs, instr.rhs_ident, instr.output);

			}else if constexpr(std::is_same<InstrType, Instruction::PrimitiveType>()){
				return this->instr_primitive_type(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::ComptimeIdent>()){
				return this->instr_ident<true>(instr.ident, instr.output);

			}else if constexpr(std::is_same<InstrType, Instruction::Ident>()){
				return this->instr_ident<false>(instr.ident, instr.output);

			}else if constexpr(std::is_same<InstrType, Instruction::Intrinsic>()){
				return this->instr_intrinsic(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Literal>()){
				return this->instr_literal(instr);

			}else{
				static_assert(false, "Unsupported instruction type");
			}
		});
	}



	auto SemanticAnalyzer::instr_global_var_decl(const Instruction::GlobalVarDecl& instr) -> Result {
		const std::string_view var_ident = this->source.getTokenBuffer()[instr.var_decl.ident].getString();

		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_global_var_decl: {}", var_ident); });

		const evo::Result<VarAttrs> var_attrs = this->analyze_var_attrs(instr.var_decl, instr.attribute_exprs);
		if(var_attrs.isError()){ return Result::Error; }


		const TypeInfo::VoidableID got_type_info_id = this->get_type(instr.type_id);

		if(got_type_info_id.isVoid()){
			this->emit_error(
				Diagnostic::Code::SemaVarTypeVoid,
				*instr.var_decl.type,
				"Variables cannot be type `Void`"
			);
			return Result::Error;
		}

		const sema::Var::ID new_sema_var = this->context.sema_buffer.createVar(
			instr.var_decl.kind,
			instr.var_decl.ident,
			std::nullopt,
			got_type_info_id.asTypeID(),
			var_attrs.value().is_pub
		);

		if(this->add_ident_to_scope(var_ident, instr.var_decl, new_sema_var) == false){ return Result::Error; }

		this->symbol_proc.extra_info.emplace<SymbolProc::VarInfo>(new_sema_var);

		this->propogate_finished_decl();
		return Result::Success;
	}


	auto SemanticAnalyzer::instr_global_var_def(const Instruction::GlobalVarDef& instr) -> Result {
		ExprInfo& value_expr_info = this->get_expr_info(instr.value_id);
		if(value_expr_info.value_category == ExprInfo::ValueCategory::Module){
			// TODO: is an error
			evo::unimplemented();
		}


		if(value_expr_info.is_ephemeral() == false){
			this->emit_error(
				Diagnostic::Code::SemaVarDefNotEphemeral,
				*instr.var_decl.value,
				"Cannot define a variable with a non-ephemeral value"
			);
			return Result::Error;
		}

		sema::Var& sema_var = this->context.sema_buffer.vars[
			this->symbol_proc.extra_info.as<SymbolProc::VarInfo>().sema_var_id
		];

		if(this->type_check<true>(
			*sema_var.typeID, value_expr_info, "Variable definition", *instr.var_decl.value
		).ok == false){
			return Result::Error;
		}

		sema_var.expr = value_expr_info.getExpr();

		return Result::Success;
	}


	auto SemanticAnalyzer::instr_global_var_decl_def(const Instruction::GlobalVarDeclDef& instr) -> Result {
		const std::string_view var_ident = this->source.getTokenBuffer()[instr.var_decl.ident].getString();

		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_global_var_decl_def: {}", var_ident); });

		const evo::Result<VarAttrs> var_attrs = this->analyze_var_attrs(instr.var_decl, instr.attribute_exprs);
		if(var_attrs.isError()){ return Result::Error; }


		ExprInfo& value_expr_info = this->get_expr_info(instr.value_id);
		if(value_expr_info.value_category == ExprInfo::ValueCategory::Module){
			const bool is_redef = !this->add_ident_to_scope(
				var_ident,
				instr.var_decl,
				value_expr_info.type_id.as<Source::ID>(),
				instr.var_decl.ident,
				var_attrs.value().is_pub
			);

			return is_redef ? Result::Error : Result::Success;
		}


		if(value_expr_info.is_ephemeral() == false){
			this->emit_error(
				Diagnostic::Code::SemaVarDefNotEphemeral,
				*instr.var_decl.value,
				"Cannot define a variable with a non-ephemeral value"
			);
			return Result::Error;
		}

			
		if(value_expr_info.isMultiValue()){
			this->emit_error(
				Diagnostic::Code::SemaMultiReturnIntoSingleValue,
				*instr.var_decl.value,
				"Cannot define a variable with multiple values"
			);
			return Result::Error;
		}

		const std::optional<TypeInfo::ID> type_id = [&](){
			if(value_expr_info.type_id.is<TypeInfo::ID>()){
				return std::optional<TypeInfo::ID>(value_expr_info.type_id.as<TypeInfo::ID>());
			}
			return std::optional<TypeInfo::ID>();
		}();

		const sema::Var::ID new_sema_var = this->context.sema_buffer.createVar(
			instr.var_decl.kind, instr.var_decl.ident, value_expr_info.getExpr(), type_id, var_attrs.value().is_pub
		);

		if(this->add_ident_to_scope(var_ident, instr.var_decl, new_sema_var) == false){ return Result::Error; }

		this->propogate_finished_decl();
		return Result::Success;
	}


	auto SemanticAnalyzer::instr_global_when_cond(const Instruction::GlobalWhenCond& instr) -> Result {
		ExprInfo cond_expr_info = this->get_expr_info(instr.cond);

		if(this->type_check<true>(
			this->context.getTypeManager().getTypeBool(),
			cond_expr_info,
			"Condition in when conditional",
			instr.when_cond.cond
		).ok == false){
			// TODO: propgate error to children
			return Result::Error;
		}

		SymbolProc::WhenCondInfo& when_cond_info = this->symbol_proc.extra_info.as<SymbolProc::WhenCondInfo>();
		auto passed_symbols = std::queue<SymbolProc::ID>();

		const bool cond = this->context.sema_buffer.getBoolValue(cond_expr_info.getExpr().boolValueID()).value;

		if(cond){
			for(const SymbolProc::ID& then_id : when_cond_info.then_ids){
				this->set_waiting_for_is_done(then_id, this->symbol_proc_id);
			}

			for(const SymbolProc::ID& else_id : when_cond_info.else_ids){
				passed_symbols.push(else_id);
			}

		}else{
			for(const SymbolProc::ID& else_id : when_cond_info.else_ids){
				this->set_waiting_for_is_done(else_id, this->symbol_proc_id);
			}

			for(const SymbolProc::ID& then_id : when_cond_info.then_ids){
				passed_symbols.push(then_id);
			}
		}

		while(passed_symbols.empty() == false){
			SymbolProc::ID passed_symbol_id = passed_symbols.front();
			passed_symbols.pop();

			SymbolProc& passed_symbol = this->context.symbol_proc_manager.getSymbolProc(passed_symbol_id);

			auto waited_on_queue = std::queue<SymbolProc::ID>();

			{
				const auto lock = std::scoped_lock(passed_symbol.waiting_lock);
				passed_symbol.passed_on_by_when_cond = true;
				this->context.symbol_proc_manager.symbol_proc_done();


				for(const SymbolProc::ID& decl_waited_on_id : passed_symbol.decl_waited_on_by){
					this->set_waiting_for_is_done(decl_waited_on_id, passed_symbol_id);
				}
				for(const SymbolProc::ID& def_waited_on_id : passed_symbol.def_waited_on_by){
					this->set_waiting_for_is_done(def_waited_on_id, passed_symbol_id);
				}
			}


			passed_symbol.extra_info.visit([&](const auto& extra_info) -> void {
				using ExtraInfo = std::decay_t<decltype(extra_info)>;

				if constexpr(std::is_same<ExtraInfo, std::monostate>()){
					return;

				}else if constexpr(std::is_same<ExtraInfo, SymbolProc::VarInfo>()){
					return;

				}else if constexpr(std::is_same<ExtraInfo, SymbolProc::WhenCondInfo>()){
					for(const SymbolProc::ID& then_id : extra_info.then_ids){
						passed_symbols.push(then_id);
					}

					for(const SymbolProc::ID& else_id : extra_info.else_ids){
						passed_symbols.push(else_id);
					}

				}else{
					static_assert(false, "Unsupported extra info");
				}
			});
		}

		return Result::Success;
	}



	auto SemanticAnalyzer::instr_func_call(const Instruction::FuncCall& instr) -> Result {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			instr.func_call,
			"Semantic Analysis of function calls (other than @import) is unimplemented"
		);
		return Result::Error;
	}


	auto SemanticAnalyzer::instr_import(const Instruction::Import& instr) -> Result {
		const ExprInfo& location = this->get_expr_info(instr.location);

		// TODO: type checking of location

		const std::string_view lookup_path = this->context.getSemaBuffer().getStringValue(
			location.getExpr().stringValueID()
		).value;

		const evo::Expected<Source::ID, Context::LookupSourceIDError> import_lookup = 
			this->context.lookupSourceID(lookup_path, this->source);

		if(import_lookup.has_value()){
			this->return_expr_info(instr.output, 
				ExprInfo(
					ExprInfo::ValueCategory::Module, ExprInfo::ValueStage::Comptime, import_lookup.value(), std::nullopt
				)
			);
			return Result::Success;
		}

		switch(import_lookup.error()){
			case Context::LookupSourceIDError::EmptyPath: {
				this->emit_error(
					Diagnostic::Code::SemaFailedToImportModule,
					instr.func_call.args[0].value,
					"Empty path is an invalid import location"
				);
				return Result::Error;
			} break;

			case Context::LookupSourceIDError::SameAsCaller: {
				// TODO: better messaging
				this->emit_error(
					Diagnostic::Code::SemaFailedToImportModule,
					instr.func_call.args[0].value,
					"Cannot import self"
				);
				return Result::Error;
			} break;

			case Context::LookupSourceIDError::NotOneOfSources: {
				this->emit_error(
					Diagnostic::Code::SemaFailedToImportModule,
					instr.func_call.args[0].value,
					std::format("File \"{}\" is not one of the files being compiled", lookup_path)
				);
				return Result::Error;
			} break;

			case Context::LookupSourceIDError::DoesntExist: {
				this->emit_error(
					Diagnostic::Code::SemaFailedToImportModule,
					instr.func_call.args[0].value,
					std::format("Couldn't find file \"{}\"", lookup_path)
				);
				return Result::Error;
			} break;

			case Context::LookupSourceIDError::FailedDuringAnalysisOfNewlyLoaded: {
				return Result::Error;
			} break;
		}

		evo::unreachable();
	}

	template<bool IS_COMPTIME>
	auto SemanticAnalyzer::instr_expr_accessor(
		const AST::Infix& infix, SymbolProcExprInfoID lhs_id, Token::ID rhs_ident, SymbolProcExprInfoID output
	) -> Result {
		const ExprInfo& lhs = this->get_expr_info(lhs_id);

		if(lhs.type_id.is<Source::ID>()){
			const Source& source_module = this->context.getSourceManager()[lhs.type_id.as<Source::ID>()];

			const std::string_view rhs_ident_str = this->source.getTokenBuffer()[rhs_ident].getString();

			//////////////////
			// look in global sema scope

			const sema::ScopeManager::Scope& source_module_sema_scope = 
				this->context.sema_buffer.scope_manager.getScope(*source_module.sema_scope_id);

			// TODO: create specialized version for here
			// evo::Result<std::optional<ExprInfo>> ident_lookup = this->analyze_expr_ident_in_scope_level(
			// 	rhs_ident, rhs_ident_str, source_module_sema_scope.getGlobalLevel(), true, true
			// );

			// if(ident_lookup.isError()){ return Result::Error; }

			// if(ident_lookup.value().has_value()){
			// 	const sema::Expr& ident_expr = ident_lookup.value().value().getExpr();

			// 	switch(ident_expr.kind()){
			// 		case sema::Expr::Kind::Var: {
			// 			if(this->context.getSemaBuffer().getVar(ident_expr.varID()).isPub == false){
			// 				this->emit_error(
			// 					Diagnostic::Code::SemaSymbolNotPub,
			// 					ident_expr.varID(),
			// 					std::format("Identifier \"{}\" does not have the #pub attribute", rhs_ident_str)
			// 				);
			// 				return Result::Error;
			// 			}
			// 		} break;

			// 		case sema::Expr::Kind::Func: {
			// 			evo::unimplemented();
			// 			// return this->context.getSemaBuffer().getFunc(ident_expr.funcID()).isPub;
			// 		} break;

			// 		case sema::Expr::Kind::Struct: {
			// 			evo::unimplemented();
			// 			// return this->context.getSemaBuffer().getStruct(ident_expr.structID()).isPub;
			// 		} break;

			// 		default: evo::debugFatalBreak("Unknown Expr Kind");
			// 	}

			// 	this->return_expr_info(output, std::move(ident_lookup.value().value()));
			// 	return Result::Success;
			// }




			const sema::ScopeLevel& scope_level = this->context.sema_buffer.scope_manager.getLevel(
				source_module_sema_scope.getGlobalLevel()
			);

			const sema::ScopeLevel::IdentID* ident_id_lookup = scope_level.lookupIdent(rhs_ident_str);

			if(ident_id_lookup != nullptr){
				return ident_id_lookup->visit([&](const auto& ident_id) -> Result {
					using IdentIDType = std::decay_t<decltype(ident_id)>;

					if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()){
						this->emit_error(
							Diagnostic::Code::MiscUnimplementedFeature,
							rhs_ident,
							"function identifiers are unimplemented"
						);
						return Result::Error;

					}else if constexpr(std::is_same<IdentIDType, sema::VarID>()){
						const sema::Var& sema_var = this->context.getSemaBuffer().getVar(ident_id);

						if(sema_var.isPub == false){
							this->emit_error(
								Diagnostic::Code::SemaSymbolNotPub,
								rhs_ident,
								std::format("Identifier \"{}\" does not have the #pub attribute", rhs_ident_str),
								Diagnostic::Info(
									"Defined here:", Diagnostic::Location::get(ident_id, source_module, this->context)
								)
							);
							return Result::Error;
						}


						using ValueCategory = ExprInfo::ValueCategory;
						using ValueStage = ExprInfo::ValueStage;

						switch(sema_var.kind){
							case AST::VarDecl::Kind::Var: {
								this->return_expr_info(output,
									ExprInfo(
										ValueCategory::ConcreteMut,
										ValueStage::Runtime,
										*sema_var.typeID,
										sema::Expr(ident_id)
									)
								);
							} break;

							case AST::VarDecl::Kind::Const: {
								this->return_expr_info(output,
									ExprInfo(
										ValueCategory::ConcreteConst,
										ValueStage::Constexpr,
										*sema_var.typeID,
										sema::Expr(ident_id)
									)
								);
							} break;

							case AST::VarDecl::Kind::Def: {
								if(sema_var.typeID.has_value()){
									this->return_expr_info(output,
										ExprInfo(
											ValueCategory::Ephemeral,
											ValueStage::Comptime,
											*sema_var.typeID,
											*sema_var.expr
										)
									);
								}else{
									this->return_expr_info(output,
										ExprInfo(
											ValueCategory::EphemeralFluid,
											ValueStage::Comptime,
											ExprInfo::FluidType{},
											*sema_var.expr
										)
									);
								}
							};
						}

						return Result::Success;

					}else if constexpr(std::is_same<IdentIDType, sema::StructID>()){
						this->emit_error(
							Diagnostic::Code::SemaTypeUsedAsExpr,
							rhs_ident,
							"struct cannot be used as an expression"
						);
						return Result::Error;

					}else if constexpr(std::is_same<IdentIDType, sema::ParamID>()){
						this->emit_error(
							Diagnostic::Code::MiscUnimplementedFeature,
							rhs_ident,
							"parameter identifiers are unimplemented"
						);
						return Result::Error;

					}else if constexpr(std::is_same<IdentIDType, sema::ReturnParamID>()){
						this->emit_error(
							Diagnostic::Code::MiscUnimplementedFeature,
							rhs_ident,
							"return parameter identifiers are unimplemented"
						);
						return Result::Error;

					}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::ModuleInfo>()){
						if(ident_id.isPub == false){
							this->emit_error(
								Diagnostic::Code::SemaSymbolNotPub,
								ident_id,
								std::format("Identifier \"{}\" does not have the #pub attribute", rhs_ident_str)
							);
							return Result::Error;
						}

						this->return_expr_info(output,
							ExprInfo(
								ExprInfo::ValueCategory::Module,
								ExprInfo::ValueStage::Comptime,
								ident_id.sourceID,
								sema::Expr::createModuleIdent(ident_id.tokenID)
							)
						);

						return Result::Success;

					}else if constexpr(std::is_same<IdentIDType, BaseType::Alias::ID>()){
						this->emit_error(
							Diagnostic::Code::SemaTypeUsedAsExpr,
							rhs_ident,
							"Type alias cannot be used as an expression"
						);
						return Result::Error;

					}else if constexpr(std::is_same<IdentIDType, BaseType::Typedef::ID>()){
						this->emit_error(
							Diagnostic::Code::SemaTypeUsedAsExpr,
							rhs_ident,
							"Typedef cannot be used as an expression"
						);
						return Result::Error;

					}else{
						static_assert(false, "Unsupported IdentID");
					}
				});
			}

















			//////////////////
			// look in symbol procs

			const auto find = source_module.global_symbol_procs.equal_range(rhs_ident_str);

			if(find.first == source_module.global_symbol_procs.end()){
				this->emit_error(
					Diagnostic::Code::SemaNoSymbolInModuleWithThatIdent,
					infix.rhs,
					std::format("Module has no symbol named \"{}\"", rhs_ident_str)
				);
				return Result::Error;
			}

			const auto found_range = core::IterRange(find.first, find.second);


			// this->symbol_proc.waiting_lock.lock();

			bool found_was_passed_by_when_cond = false;
			for(auto& pair : found_range){
				const SymbolProc::ID& found_symbol_proc_id = pair.second;
				SymbolProc& found_symbol_proc = this->context.symbol_proc_manager.getSymbolProc(found_symbol_proc_id);

				const SymbolProc::WaitOnResult wait_on_result = [&](){
					if constexpr(IS_COMPTIME){
						return found_symbol_proc.waitOnDefIfNeeded(
							this->symbol_proc_id, this->context, found_symbol_proc_id
						);
					}else{
						return found_symbol_proc.waitOnDeclIfNeeded(
							this->symbol_proc_id, this->context, found_symbol_proc_id
						);
					}
				}();

				switch(wait_on_result){
					case SymbolProc::WaitOnResult::NotNeeded: {
						// TODO: do this better
						// call again as symbol finished
						return this->instr_expr_accessor<IS_COMPTIME>(infix, lhs_id, rhs_ident, output);
					} break;

					case SymbolProc::WaitOnResult::Waiting: {
						// this->symbol_proc.waiting_for.emplace_back(found_symbol_proc_id);
					} break;

					case SymbolProc::WaitOnResult::WasErrored: {
						const auto lock = std::scoped_lock(this->symbol_proc.waiting_lock);
						this->symbol_proc.errored = true;
						return Result::Error;
					} break;

					case SymbolProc::WaitOnResult::WasPassedOnByWhenCond: {
						found_was_passed_by_when_cond = true;
					} break;

					case SymbolProc::WaitOnResult::CircularDepDetected: {
						return Result::Error;
					} break;
				}
			}

			// this->symbol_proc.waiting_lock.unlock();

			if(this->symbol_proc.waiting_for.empty() == false){ return Result::NeedToWait; }

			auto infos = evo::SmallVector<Diagnostic::Info>();

			if(found_was_passed_by_when_cond){
				infos.emplace_back(
					Diagnostic::Info("The identifier was located in a when conditional block that wasn't taken")
				);
			}

			this->emit_error(
				Diagnostic::Code::SemaNoSymbolInModuleWithThatIdent,
				infix.rhs,
				std::format("Module has no symbol named \"{}\"", rhs_ident_str),
				std::move(infos)
			);
			return Result::Error;
		}

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			infix.opTokenID,
			"Accessor operator of non-modules is unimplemented"
		);
		return Result::Error;
	}


	auto SemanticAnalyzer::instr_primitive_type(const Instruction::PrimitiveType& instr) -> Result {
		auto base_type = std::optional<BaseType::ID>();
		auto qualifiers = evo::SmallVector<AST::Type::Qualifier>();

		const Token::ID primitive_type_token_id = ASTBuffer::getPrimitiveType(instr.type.base);
		const Token& primitive_type_token = this->source.getTokenBuffer()[primitive_type_token_id];

		switch(primitive_type_token.kind()){
			case Token::Kind::TypeVoid: {
				if(instr.type.qualifiers.empty() == false){
					this->emit_error(
						Diagnostic::Code::SemaVoidWithQualifiers,
						instr.type.base,
						"Type \"Void\" cannot have qualifiers"
					);
					return Result::Error;
				}
				this->return_type(instr.output, TypeInfo::VoidableID::Void());
				return Result::Success;
			} break;

			case Token::Kind::TypeThis: {
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					instr.type,
					"Type `This` is unimplemented"
				);
				return Result::Error;
			} break;

			case Token::Kind::TypeInt:       case Token::Kind::TypeISize:      case Token::Kind::TypeUInt:
			case Token::Kind::TypeUSize:     case Token::Kind::TypeF16:        case Token::Kind::TypeBF16:
			case Token::Kind::TypeF32:       case Token::Kind::TypeF64:        case Token::Kind::TypeF80:
			case Token::Kind::TypeF128:      case Token::Kind::TypeByte:       case Token::Kind::TypeBool:
			case Token::Kind::TypeChar:      case Token::Kind::TypeRawPtr:     case Token::Kind::TypeTypeID:
			case Token::Kind::TypeCShort:    case Token::Kind::TypeCUShort:    case Token::Kind::TypeCInt:
			case Token::Kind::TypeCUInt:     case Token::Kind::TypeCLong:      case Token::Kind::TypeCULong:
			case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong: case Token::Kind::TypeCLongDouble: {
				base_type = this->context.type_manager.getOrCreatePrimitiveBaseType(primitive_type_token.kind());
			} break;

			case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N: {
				base_type = this->context.type_manager.getOrCreatePrimitiveBaseType(
					primitive_type_token.kind(), primitive_type_token.getBitWidth()
				);
			} break;


			case Token::Kind::TypeType: {
				this->emit_error(
					Diagnostic::Code::SemaGenericTypeNotInTemplatePackDecl,
					instr.type,
					"Type \"Type\" may only be used in a template pack declaration"
				);
				return Result::Error;
			} break;

			default: {
				evo::debugFatalBreak("Unknown or unsupported PrimitiveType: {}", primitive_type_token.kind());
			} break;
		}

		evo::debugAssert(base_type.has_value(), "Base type was not set");

		for(const AST::Type::Qualifier& qualifier : instr.type.qualifiers){
			qualifiers.emplace_back(qualifier);
		}

		if(this->check_type_qualifiers(qualifiers, instr.type) == false){ return Result::Error; }

		this->return_type(
			instr.output,
			TypeInfo::VoidableID(
				this->context.type_manager.getOrCreateTypeInfo(TypeInfo(*base_type, std::move(qualifiers)))
			)
		);
		return Result::Success;
	}


	template<bool IS_COMPTIME>
	auto SemanticAnalyzer::instr_ident(Token::ID ident, SymbolProc::ExprInfoID output) -> Result {
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

			if(scope_level_lookup.isError()){ return Result::Error; }

			if(scope_level_lookup.value().has_value()){
				this->return_expr_info(output, *scope_level_lookup.value());
				return Result::Success;
			}

			i -= 1;
		}

		// TODO: look for Source::global_symbol_procs and wait if needed
		const auto find = this->source.global_symbol_procs.equal_range(ident_str);
		if(find.first == this->source.global_symbol_procs.end()){
			this->emit_error(
				Diagnostic::Code::SemaIdentNotInScope,
				ident,
				std::format("Identifier \"{}\" was not defined in this scope", ident_str)
			);
			return Result::Error;
		}

		bool found_was_passed_by_when_cond = false;
		for(auto iter = find.first; iter != find.second; ++iter){
			const SymbolProc::ID id_to_wait_on = iter->second;
			SymbolProc& symbol_proc_to_wait_on = this->context.symbol_proc_manager.getSymbolProc(id_to_wait_on);

			const SymbolProc::WaitOnResult wait_on_result = [&](){
				if constexpr(IS_COMPTIME){
					return symbol_proc_to_wait_on.waitOnDefIfNeeded(
						this->symbol_proc_id, this->context, id_to_wait_on
					);
				}else{
					return symbol_proc_to_wait_on.waitOnDeclIfNeeded(
						this->symbol_proc_id, this->context, id_to_wait_on
					);
				}
			}();

			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NotNeeded: {
					// TODO: better way of doing this
					return this->instr_ident<IS_COMPTIME>(ident, output);
				} break;
				case SymbolProc::WaitOnResult::Waiting: {
					// do nothing...
				} break;
				case SymbolProc::WaitOnResult::WasErrored: {
					const auto lock = std::scoped_lock(this->symbol_proc.waiting_lock);
					this->symbol_proc.errored = true;
					return Result::Error;
				} break;
				case SymbolProc::WaitOnResult::WasPassedOnByWhenCond: {
					found_was_passed_by_when_cond = true;
				} break;
				case SymbolProc::WaitOnResult::CircularDepDetected: {
					const auto lock = std::scoped_lock(this->symbol_proc.waiting_lock);
					this->symbol_proc.errored = true;
					return Result::Error;
				} break;
			}
		}

		if(this->symbol_proc.waiting_for.empty()){
			auto infos = evo::SmallVector<Diagnostic::Info>();

			if(found_was_passed_by_when_cond){
				infos.emplace_back(
					Diagnostic::Info("The identifier was declared in a when conditional block that wasn't taken")
				);
			}

			this->emit_error(
				Diagnostic::Code::SemaIdentNotInScope,
				ident,
				std::format("Identifier \"{}\" was not defined in this scope", ident_str),
				std::move(infos)
			);
			return Result::Error;

		}else{
			return Result::NeedToWait;
		}
	}


	auto SemanticAnalyzer::instr_intrinsic(const Instruction::Intrinsic& instr) -> Result {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			instr.intrinsic,
			"Semantic Analysis of intrinsics (other than @import) is unimplemented"
		);
		return Result::Error;
	}


	auto SemanticAnalyzer::instr_literal(const Instruction::Literal& instr) -> Result {
		const Token& literal_token = this->source.getTokenBuffer()[instr.literal];
		switch(literal_token.kind()){
			case Token::Kind::LiteralInt: {
				this->return_expr_info(instr.output,
					ExprInfo::ValueCategory::EphemeralFluid,
					ExprInfo::ValueStage::Comptime,
					ExprInfo::FluidType{},
					sema::Expr(this->context.sema_buffer.createIntValue(
						core::GenericInt(256, literal_token.getInt()), std::nullopt
					))
				);
				return Result::Success;
			} break;

			case Token::Kind::LiteralFloat: {
				this->return_expr_info(instr.output,
					ExprInfo::ValueCategory::EphemeralFluid,
					ExprInfo::ValueStage::Comptime,
					ExprInfo::FluidType{},
					sema::Expr(this->context.sema_buffer.createFloatValue(
						core::GenericFloat::createF128(literal_token.getFloat()), std::nullopt
					))
				);
				return Result::Success;
			} break;

			case Token::Kind::LiteralBool: {
				this->return_expr_info(instr.output,
					ExprInfo::ValueCategory::Ephemeral,
					ExprInfo::ValueStage::Comptime,
					this->context.getTypeManager().getTypeBool(),
					sema::Expr(this->context.sema_buffer.createBoolValue(literal_token.getBool()))
				);
				return Result::Success;
			} break;

			case Token::Kind::LiteralString: {
				this->return_expr_info(instr.output,
					ExprInfo::ValueCategory::Ephemeral,
					ExprInfo::ValueStage::Comptime,
					this->context.type_manager.getOrCreateTypeInfo(
						TypeInfo(
							this->context.type_manager.getOrCreateArray(
								BaseType::Array(
									this->context.getTypeManager().getTypeChar(),
									evo::SmallVector<uint64_t>{literal_token.getString().size()},
									core::GenericValue('\0')
								)
							)
						)
					),
					sema::Expr(this->context.sema_buffer.createStringValue(std::string(literal_token.getString())))
				);
				return Result::Success;
			} break;

			case Token::Kind::LiteralChar: {
				this->return_expr_info(instr.output,
					ExprInfo::ValueCategory::Ephemeral,
					ExprInfo::ValueStage::Comptime,
					this->context.getTypeManager().getTypeChar(),
					sema::Expr(this->context.sema_buffer.createCharValue(literal_token.getChar()))
				);
				return Result::Success;
			} break;

			default: evo::debugFatalBreak("Not a valid literal");
		}
	}



	///////////////////////////////////
	// misc

	auto SemanticAnalyzer::get_current_scope_level() const -> sema::ScopeLevel& {
		return this->context.sema_buffer.scope_manager.getLevel(this->scope.getCurrentLevel());
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

			if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()){
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ident,
					"function identifiers are unimplemented"
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
								ValueCategory::Ephemeral, ValueStage::Comptime, *sema_var.typeID, *sema_var.expr
							));
						}else{
							return std::optional<ExprInfo>(ExprInfo(
								ValueCategory::EphemeralFluid,
								ValueStage::Comptime,
								ExprInfo::FluidType{},
								*sema_var.expr
							));
						}
					};
				}

				evo::debugFatalBreak("Unknown or unsupported AST::VarDecl::Kind");

			}else if constexpr(std::is_same<IdentIDType, sema::StructID>()){
				this->emit_error(
					Diagnostic::Code::SemaTypeUsedAsExpr,
					ident,
					"struct cannot be used as an expression"
				);
				return evo::resultError;

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



	auto SemanticAnalyzer::set_waiting_for_is_done(SymbolProc::ID target_id, SymbolProc::ID done_id) -> void {
		SymbolProc& target = this->context.symbol_proc_manager.getSymbolProc(target_id);

		const auto lock = std::scoped_lock(target.waiting_lock);

		for(size_t i = 0; i < target.waiting_for.size(); i+=1){
			if(target.waiting_for[i] == done_id){
				if(i + 1 < target.waiting_for.size()){
					target.waiting_for[i] = target.waiting_for.back();
				}
			}

			target.waiting_for.pop_back();

			if(target.waiting_for.empty()){
				this->context.add_task_to_work_manager(target_id);
			}
		}
	}



	auto SemanticAnalyzer::analyze_var_attrs(
		const AST::VarDecl& var_decl, const SymbolProc::Instruction::AttributeExprs& attribute_exprs
	) -> evo::Result<VarAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");

		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(var_decl.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(attribute_exprs[i].empty()){
					if(attr_pub.set(attribute.attribute, true) == false){ return evo::resultError; } 

				}else if(attribute_exprs[i].size() == 1){
					ExprInfo cond_expr_info = this->get_expr_info(attribute_exprs[i][0]);

					if(this->type_check<true>(
						this->context.getTypeManager().getTypeBool(),
						cond_expr_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return evo::resultError;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_expr_info.getExpr().boolValueID()).value;

					if(attr_pub.set(attribute.attribute, pub_cond) == false){ return evo::resultError; }

				}else{
					this->emit_error(
						Diagnostic::Code::SemaTooManyAttributeArgs,
						attribute.args[1],
						"Attribute #pub does not accept more than 1 argument"
					);
					return evo::resultError;
				}

			}else{
				this->emit_error(
					Diagnostic::Code::SemaUnknownAttribute,
					attribute.attribute,
					std::format("Unknown variable attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}


		return VarAttrs(attr_pub.is_set());
	}


	auto SemanticAnalyzer::propogate_finished_decl() -> void {
		const auto lock = std::scoped_lock(this->symbol_proc.waiting_lock);

		for(const SymbolProc::ID& decl_waited_on_id : this->symbol_proc.decl_waited_on_by){
			SymbolProc& decl_waited_on = this->context.symbol_proc_manager.getSymbolProc(decl_waited_on_id);

			for(size_t i = 0; i < decl_waited_on.waiting_for.size(); i+=1){
				if(decl_waited_on.waiting_for[i] == this->symbol_proc_id){
					if(i + 1 < decl_waited_on.waiting_for.size()){
						decl_waited_on.waiting_for[i] = decl_waited_on.waiting_for.back();
					}
				}

				decl_waited_on.waiting_for.pop_back();

				if(decl_waited_on.waiting_for.empty()){
					this->context.add_task_to_work_manager(decl_waited_on_id);
				}
			}
		}

		this->symbol_proc.decl_done = true;
	}


	//////////////////////////////////////////////////////////////////////
	// exec value gets / returns


	auto SemanticAnalyzer::get_type(SymbolProc::TypeID symbol_proc_type_id) -> TypeInfo::VoidableID {
		return *this->symbol_proc.type_ids[symbol_proc_type_id.get()];
	}

	auto SemanticAnalyzer::return_type(SymbolProc::TypeID symbol_proc_type_id, TypeInfo::VoidableID&& id) -> void {
		this->symbol_proc.type_ids[symbol_proc_type_id.get()] = std::move(id);
	}


	auto SemanticAnalyzer::get_expr_info(SymbolProc::ExprInfoID symbol_proc_expr_info_id) -> ExprInfo& {
		return *this->symbol_proc.expr_infos[symbol_proc_expr_info_id.get()];
	}

	auto SemanticAnalyzer::return_expr_info(SymbolProc::ExprInfoID symbol_proc_expr_info_id, auto&&... args) -> void {
		this->symbol_proc.expr_infos[symbol_proc_expr_info_id.get()]
			.emplace(std::forward<decltype(args)>(args)...);
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
				"{} cannot accept an expression of a different type, and the expression cannot be implicitly converted",
				expected_type_location_name
			),
			std::move(infos)
		);
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


	auto SemanticAnalyzer::add_ident_to_scope(std::string_view ident_str, const auto& ast_node, auto&&... ident_id_info)
	-> bool {
		const auto error_already_defined = [&](const sema::ScopeLevel::IdentID& first_defined_id) -> void {
			first_defined_id.visit([&](const auto& first_decl_ident_id) -> void {
				using IdentIDType = std::decay_t<decltype(first_decl_ident_id)>;

				auto infos = evo::SmallVector<Diagnostic::Info>();

				if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()){
					first_decl_ident_id.front().visit([&](const auto& func_id) -> void {
						if(first_decl_ident_id.size() == 1){
							infos.emplace_back("First defined here:", this->get_location(func_id));

						}else if(first_decl_ident_id.size() == 2){
							infos.emplace_back(
								"First defined here (and 1 other place):", this->get_location(func_id)
							);
						}else{
							infos.emplace_back(
								std::format(
									"First defined here (and {} other places):", first_decl_ident_id.size() - 1
								),
								this->get_location(func_id)
							);
						}
					});

				}else{
					infos.emplace_back("First defined here:", this->get_location(first_decl_ident_id));
				}

				// if(scope_level_id != this->scope.getCurrentLevel()){
				// 	infos.emplace_back("Note: shadowing is not allowed");
				// }

				this->emit_error(
					Diagnostic::Code::SemaIdentAlreadyInScope,
					ast_node,
					std::format("Identifier \"{}\" was already defined in this scope", ident_str),
					std::move(infos)
				);
			});
		};


		sema::ScopeLevel& current_scope_level = this->get_current_scope_level();
		if(current_scope_level.addIdent(ident_str, std::forward<decltype(ident_id_info)>(ident_id_info)...) == false){
			error_already_defined(*current_scope_level.lookupIdent(ident_str));
			return false;
		}

		// TODO: look in parent scopes

		return true;
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


}