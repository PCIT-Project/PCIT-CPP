////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SemanticAnalyzer.h"

#include <queue>

#include "../symbol_proc/SymbolProcBuilder.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther{

	static thread_local bool may_break = false;

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
		if(this->symbol_proc.passed_on_by_when_cond){ return; }

		evo::debugAssert(
			this->symbol_proc.being_worked_on.exchange(true) == false, "This symbol is already being worked on"
		);
		#if defined(PCIT_CONFIG_DEBUG)
			EVO_DEFER([&](){ this->symbol_proc.being_worked_on = false; });
		#endif

		while(this->symbol_proc.isAtEnd() == false){
			evo::debugAssert(
				this->symbol_proc.passed_on_by_when_cond == false,
				"symbol was passed on by when cond - should not be analyzed"
			);

			evo::debugAssert(
				this->symbol_proc.errored == false,
				"symbol was errored - should not be analyzed"
			);

			switch(this->analyze_instr(this->symbol_proc.getInstruction())){
				case Result::Success: {
					this->symbol_proc.nextInstruction();
				} break;

				case Result::Error: {
					this->context.symbol_proc_manager.symbol_proc_done();
					this->symbol_proc.errored = true;
					if(this->symbol_proc.extra_info.is<SymbolProc::StructInfo>()){
						SymbolProc::StructInfo& struct_info = this->symbol_proc.extra_info.as<SymbolProc::StructInfo>();
						if(struct_info.instantiation != nullptr){ struct_info.instantiation->errored = true; }
					}
					return;
				} break;

				case Result::NeedToWait: {
					return;
				} break;

				case Result::NeedToWaitBeforeNextInstr: {
					this->symbol_proc.nextInstruction();
					return;
				} break;

				// case Result::NeedToWaitOnInstantiation: {
				// 	this->context.add_task_to_work_manager(this->symbol_proc_id);
				// 	return;
				// } break;
			}
		}

		this->context.trace("Finished semantic analysis of symbol: \"{}\"", this->symbol_proc.ident);
	}


	auto SemanticAnalyzer::analyze_instr(const Instruction& instruction) -> Result {
		return instruction.visit([&](const auto& instr) -> Result {
			using InstrType = std::decay_t<decltype(instr)>;


			if constexpr(std::is_same<InstrType, Instruction::VarDecl>()){
				return this->instr_var_decl(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::VarDef>()){
				return this->instr_var_def(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::VarDeclDef>()){
				return this->instr_var_decl_def(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::WhenCond>()){
				return this->instr_when_cond(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::AliasDecl>()){
				return this->instr_alias_decl(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::AliasDef>()){
				return this->instr_alias_def(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::StructDecl>()){
				return this->instr_struct_decl<false>(instr.struct_decl, instr.attribute_params_info);

			}else if constexpr(std::is_same<InstrType, Instruction::StructDeclInstantiation>()){
				return this->instr_struct_decl<true>(instr.struct_decl, instr.attribute_params_info);

			}else if constexpr(std::is_same<InstrType, Instruction::StructDef>()){
				return this->instr_struct_def();

			}else if constexpr(std::is_same<InstrType, Instruction::TemplateStruct>()){
				return this->instr_templated_struct(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TypeToTerm>()){
				return this->instr_type_to_term(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncCall>()){
				return this->instr_func_call(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Import>()){
				return this->instr_import(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Accessor<true>>()){
				return this->instr_expr_accessor<true>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Accessor<false>>()){
				return this->instr_expr_accessor<false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::PrimitiveType>()){
				return this->instr_primitive_type(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::UserType>()){
				return this->instr_user_type(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::BaseTypeIdent>()){
				return this->instr_base_type_ident(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TemplatedTerm>()){
				return this->instr_templated_term(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TemplatedTermWait>()){
				return this->instr_templated_term_wait(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Ident<true>>()){
				return this->instr_ident<true>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Ident<false>>()){
				return this->instr_ident<false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Intrinsic>()){
				return this->instr_intrinsic(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Literal>()){
				return this->instr_literal(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Uninit>()){
				return this->instr_uninit(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Zeroinit>()){
				return this->instr_zeroinit(instr);

			}else{
				static_assert(false, "Unsupported instruction type");
			}
		});
	}



	auto SemanticAnalyzer::instr_var_decl(const Instruction::VarDecl& instr) -> Result {
		const std::string_view var_ident = this->source.getTokenBuffer()[instr.var_decl.ident].getString();

		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_var_decl: {}", this->symbol_proc.ident); });

		const evo::Result<VarAttrs> var_attrs = this->analyze_var_attrs(instr.var_decl, instr.attribute_params_info);
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
			std::optional<sema::Expr>(),
			got_type_info_id.asTypeID(),
			var_attrs.value().is_pub
		);

		if(this->add_ident_to_scope(var_ident, instr.var_decl, new_sema_var) == false){ return Result::Error; }

		this->symbol_proc.extra_info.emplace<SymbolProc::VarInfo>(new_sema_var);

		this->propagate_finished_decl();
		return Result::Success;
	}


	auto SemanticAnalyzer::instr_var_def(const Instruction::VarDef& instr) -> Result {
		sema::Var& sema_var = this->context.sema_buffer.vars[
			this->symbol_proc.extra_info.as<SymbolProc::VarInfo>().sema_var_id
		];

		TermInfo& value_term_info = this->get_term_info(instr.value_id);


		if(value_term_info.value_category == TermInfo::ValueCategory::Initializer){
			if(instr.var_decl.kind != AST::VarDecl::Kind::Var){
				this->emit_error(
					Diagnostic::Code::SemaVarInitializerOnNonVar,
					instr.var_decl,
					"Only `var` variables can be defined with an initializer value"
				);
				return Result::Error;
			}

		}else{
			if(value_term_info.is_ephemeral() == false){
				if(this->check_term_isnt_type(value_term_info, *instr.var_decl.value) == false){ return Result::Error; }

				if(value_term_info.value_category == TermInfo::ValueCategory::Module){
					this->error_type_mismatch(
						*sema_var.typeID, value_term_info, "Variable definition", *instr.var_decl.value
					);
					return Result::Error;
				}

				this->emit_error(
					Diagnostic::Code::SemaVarDefNotEphemeral,
					*instr.var_decl.value,
					"Cannot define a variable with a non-ephemeral value"
				);
				return Result::Error;
			}
			
			if(this->type_check<true>(
				*sema_var.typeID, value_term_info, "Variable definition", *instr.var_decl.value
			).ok == false){
				return Result::Error;
			}
		}

		sema_var.expr = value_term_info.getExpr();

		this->propagate_finished_def();
		return Result::Success;
	}


	auto SemanticAnalyzer::instr_var_decl_def(const Instruction::VarDeclDef& instr) -> Result {
		const std::string_view var_ident = this->source.getTokenBuffer()[instr.var_decl.ident].getString();

		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_var_decl_def: {}", this->symbol_proc.ident); });

		const evo::Result<VarAttrs> var_attrs = this->analyze_var_attrs(instr.var_decl, instr.attribute_params_info);
		if(var_attrs.isError()){ return Result::Error; }


		TermInfo& value_term_info = this->get_term_info(instr.value_id);
		if(value_term_info.value_category == TermInfo::ValueCategory::Module){
			if(instr.var_decl.kind != AST::VarDecl::Kind::Def){
				this->emit_error(
					Diagnostic::Code::SemaModuleVarMustBeDef,
					*instr.var_decl.value,
					"Variable that has a module value must be declared as `def`"
				);
				return Result::Error;
			}

			const bool is_redef = !this->add_ident_to_scope(
				var_ident,
				instr.var_decl,
				value_term_info.type_id.as<Source::ID>(),
				instr.var_decl.ident,
				var_attrs.value().is_pub
			);

			this->propagate_finished_decl_def();
			return is_redef ? Result::Error : Result::Success;
		}


		if(value_term_info.value_category == TermInfo::ValueCategory::Initializer){
			if(instr.var_decl.kind != AST::VarDecl::Kind::Var){
				this->emit_error(
					Diagnostic::Code::SemaVarInitializerOnNonVar,
					instr.var_decl,
					"Only `var` variables can be defined with an initializer value"
				);
				return Result::Error;
			}else{
				this->emit_error(
					Diagnostic::Code::SemaVarInitializerWithoutExplicitType,
					*instr.var_decl.value,
					"Cannot define a variable with an initializer value without an explicit type"
				);
				return Result::Error;
			}
		}


		if(value_term_info.is_ephemeral() == false){
			if(this->check_term_isnt_type(value_term_info, *instr.var_decl.value) == false){ return Result::Error; }

			this->emit_error(
				Diagnostic::Code::SemaVarDefNotEphemeral,
				*instr.var_decl.value,
				"Cannot define a variable with a non-ephemeral value"
			);
			return Result::Error;
		}

			
		if(value_term_info.isMultiValue()){
			this->emit_error(
				Diagnostic::Code::SemaMultiReturnIntoSingleValue,
				*instr.var_decl.value,
				"Cannot define a variable with multiple values"
			);
			return Result::Error;
		}


		if(instr.type_id.has_value()){
			const TypeInfo::VoidableID got_type_info_id = this->get_type(*instr.type_id);

			if(got_type_info_id.isVoid()){
				this->emit_error(
					Diagnostic::Code::SemaVarTypeVoid, *instr.var_decl.type, "Variables cannot be type `Void`"
				);
				return Result::Error;
			}

			if(this->type_check<true>(
				got_type_info_id.asTypeID(), value_term_info, "Variable definition", *instr.var_decl.value
			).ok == false){
				return Result::Error;
			}
		}

		const std::optional<TypeInfo::ID> type_id = [&](){
			if(value_term_info.type_id.is<TypeInfo::ID>()){
				return std::optional<TypeInfo::ID>(value_term_info.type_id.as<TypeInfo::ID>());
			}
			return std::optional<TypeInfo::ID>();
		}();

		const sema::Var::ID new_sema_var = this->context.sema_buffer.createVar(
			instr.var_decl.kind,
			instr.var_decl.ident,
			std::optional<sema::Expr>(value_term_info.getExpr()),
			type_id,
			var_attrs.value().is_pub
		);

		if(this->add_ident_to_scope(var_ident, instr.var_decl, new_sema_var) == false){ return Result::Error; }

		this->propagate_finished_decl_def();
		return Result::Success;
	}



	auto SemanticAnalyzer::instr_when_cond(const Instruction::WhenCond& instr) -> Result {
		TermInfo& cond_term_info = this->get_term_info(instr.cond);
		if(this->check_term_isnt_type(cond_term_info, instr.when_cond.cond) == false){ return Result::Error; }

		if(this->type_check<true>(
			this->context.getTypeManager().getTypeBool(),
			cond_term_info,
			"Condition in when conditional",
			instr.when_cond.cond
		).ok == false){
			// TODO: propgate error to children
			return Result::Error;
		}

		SymbolProc::WhenCondInfo& when_cond_info = this->symbol_proc.extra_info.as<SymbolProc::WhenCondInfo>();
		auto passed_symbols = std::queue<SymbolProc::ID>();

		const bool cond = this->context.sema_buffer.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

		if(cond){
			for(const SymbolProc::ID& then_id : when_cond_info.then_ids){
				SymbolProc& then_symbol = this->context.symbol_proc_manager.getSymbolProc(then_id);
				then_symbol.sema_scope_id = this->context.sema_buffer.scope_manager.copyScope(
					*this->symbol_proc.sema_scope_id
				);
				this->set_waiting_for_is_done(then_id, this->symbol_proc_id);
			}

			for(const SymbolProc::ID& else_id : when_cond_info.else_ids){
				passed_symbols.push(else_id);
			}

		}else{
			for(const SymbolProc::ID& else_id : when_cond_info.else_ids){
				SymbolProc& else_symbol = this->context.symbol_proc_manager.getSymbolProc(else_id);
				else_symbol.sema_scope_id = this->context.sema_buffer.scope_manager.copyScope(
					*this->symbol_proc.sema_scope_id
				);
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
			passed_symbol.passed_on_by_when_cond = true;


			auto waited_on_queue = std::queue<SymbolProc::ID>();

			{
				const auto lock = std::scoped_lock(passed_symbol.decl_waited_on_lock, passed_symbol.def_waited_on_lock);
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

				}else if constexpr(std::is_same<ExtraInfo, SymbolProc::AliasInfo>()){
					return;

				}else if constexpr(std::is_same<ExtraInfo, SymbolProc::StructInfo>()){
					return;

				}else{
					static_assert(false, "Unsupported extra info");
				}
			});
		}

		this->propagate_finished_def();
		return Result::Success;
	}



	auto SemanticAnalyzer::instr_alias_decl(const Instruction::AliasDecl& instr) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_alias_decl: {}", this->symbol_proc.ident); });

		auto attr_pub = ConditionalAttribute(*this, "pub");

		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(instr.alias_decl.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(instr.attribute_params_info[i].empty()){
					if(attr_pub.set(attribute.attribute, true) == false){ return Result::Error; } 

				}else if(instr.attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(instr.attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]) == false){ return Result::Error; }

					if(this->type_check<true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return Result::Error;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

					if(attr_pub.set(attribute.attribute, pub_cond) == false){ return Result::Error; }

				}else{
					this->emit_error(
						Diagnostic::Code::SemaTooManyAttributeArgs,
						attribute.args[1],
						"Attribute #pub does not accept more than 1 argument"
					);
					return Result::Error;
				}

			}else{
				this->emit_error(
					Diagnostic::Code::SemaUnknownAttribute,
					attribute.attribute,
					std::format("Unknown alias attribute #{}", attribute_str)
				);
				return Result::Error;
			}
		}


		///////////////////////////////////
		// create

		const BaseType::ID created_alias = this->context.type_manager.getOrCreateAlias(
			BaseType::Alias(
				this->source.getID(), instr.alias_decl.ident, std::optional<TypeInfoID>(), attr_pub.is_set()
			)
		);

		this->symbol_proc.extra_info.emplace<SymbolProc::AliasInfo>(created_alias.aliasID());

		const std::string_view ident_str = this->source.getTokenBuffer()[instr.alias_decl.ident].getString();
		if(this->add_ident_to_scope(ident_str, instr.alias_decl, created_alias.aliasID()) == false){
			return Result::Error;
		}

		this->propagate_finished_decl();
		return Result::Success;
	}



	auto SemanticAnalyzer::instr_alias_def(const Instruction::AliasDef& instr) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_var_def: {}", this->symbol_proc.ident); });

		BaseType::Alias& alias_info = this->context.type_manager.aliases[
			this->symbol_proc.extra_info.as<SymbolProc::AliasInfo>().alias_id
		];

		const TypeInfo::VoidableID aliased_type = this->get_type(instr.aliased_type);
		if(aliased_type.isVoid()){
			this->emit_error(
				Diagnostic::Code::SemaAliasCannotBeVoid,
				instr.alias_decl.type,
				"Alias cannot be type `Void`"
			);
			return Result::Error;
		}


		alias_info.aliasedType = aliased_type.asTypeID();

		this->propagate_finished_def();
		return Result::Success;
	};


	template<bool IS_INSTANTIATION>
	auto SemanticAnalyzer::instr_struct_decl(
		const AST::StructDecl& struct_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_struct_decl: {}", this->symbol_proc.ident); });

		const evo::Result<StructAttrs> struct_attrs = this->analyze_struct_attrs(struct_decl, attribute_params_info);
		if(struct_attrs.isError()){ return Result::Error; }


		///////////////////////////////////
		// create

		SymbolProc::StructInfo& struct_info = this->symbol_proc.extra_info.as<SymbolProc::StructInfo>();

		const BaseType::ID created_struct = this->context.type_manager.getOrCreateStruct(
			BaseType::Struct(
				this->source.getID(),
				struct_decl.ident,
				struct_info.member_symbols,
				nullptr,
				struct_attrs.value().is_pub
			)
		);

		struct_info.struct_id = created_struct.structID();

		if constexpr(IS_INSTANTIATION == false){
			const std::string_view ident_str = this->source.getTokenBuffer()[struct_decl.ident].getString();
			if(this->add_ident_to_scope(ident_str, struct_decl, created_struct.structID()) == false){
				return Result::Error;
			}
		}


		///////////////////////////////////
		// setup member statements

		this->push_scope_level(created_struct.structID());

		this->context.type_manager.structs[created_struct.structID()].scopeLevel = &this->get_current_scope_level();

		for(const SymbolProc::ID& member_stmt_id : struct_info.stmts){
			SymbolProc& member_stmt = this->context.symbol_proc_manager.getSymbolProc(member_stmt_id);

			member_stmt.sema_scope_id = this->context.sema_buffer.scope_manager.copyScope(
				*this->symbol_proc.sema_scope_id
			);

			const auto lock = std::scoped_lock(this->symbol_proc.waiting_for_lock, member_stmt.def_waited_on_lock);
			this->symbol_proc.waiting_for.emplace_back(member_stmt_id);
			member_stmt.def_waited_on_by.emplace_back(this->symbol_proc_id);
		}


		if constexpr(IS_INSTANTIATION){
			struct_info.instantiation->structID = created_struct.structID();
		}

		this->propagate_finished_decl();

		if(struct_info.stmts.empty()){
			return Result::Success;
		}else{
			return Result::NeedToWaitBeforeNextInstr;
		}
	}


	auto SemanticAnalyzer::instr_struct_def() -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_struct_def: {}", this->symbol_proc.ident); });

		this->pop_scope_level(); // TODO: needed?

		this->propagate_finished_def();

		this->context.type_manager.structs[
			this->symbol_proc.extra_info.as<SymbolProc::StructInfo>().struct_id
		].defCompleted = true;

		return Result::Success;
	}



	auto SemanticAnalyzer::instr_templated_struct(const Instruction::TemplateStruct& instr) -> Result {
		EVO_DEFER([&](){
			this->context.trace("SemanticAnalyzer::instr_templated_struct: {}", this->symbol_proc.ident);
		});


		size_t minimum_num_template_args = 0;
		auto params = evo::SmallVector<sema::TemplatedStruct::Param>();

		for(const SymbolProc::Instruction::TemplateParamInfo& template_param_info : instr.template_param_infos){
			// const std::string_view ident_str = 
			// 	this->source.getTokenBuffer()[template_param_info.param.ident].getString();

			// for(auto iter = std::next(this->scope.begin()); iter != this->scope.end(); ++iter){
			// 	sema::ScopeLevel& scope_level = this->context.sema_buffer.scope_manager.getLevel(*iter);
			// 	if(scope_level.disallowIdentForShadowing(ident_str, add_ident_result.value()) == false){
			// 		this->error_already_defined<true>(ast_node, ident_str, *scope_level.lookupIdent(ident_str));
			// 		return false;
			// 	}
			// }


			auto type_id = std::optional<TypeInfo::ID>();
			if(template_param_info.type_id.has_value()){
				const TypeInfo::VoidableID type_info_voidable_id = this->get_type(*template_param_info.type_id);
				if(type_info_voidable_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SemaTemplateParamCannotBeTypeVoid,
						template_param_info.param.type,
						"Template parameter cannot be type `Void`"
					);
					return Result::Error;
				}
				type_id = type_info_voidable_id.asTypeID();
			}

			TermInfo* default_value = nullptr;
			if(template_param_info.default_value.has_value()){
				default_value = &this->get_term_info(*template_param_info.default_value);

				if(type_id.has_value()){
					if(default_value->isSingleValue() == false){
						if(default_value->isMultiValue()){
							this->emit_error(
								Diagnostic::Code::SemaTemplateParamExprDefaultMustBeExpr,
								*template_param_info.param.defaultValue,
								"Default of an expression template parameter must be a single expression"
							);	
						}else{
							this->emit_error(
								Diagnostic::Code::SemaTemplateParamExprDefaultMustBeExpr,
								*template_param_info.param.defaultValue,
								"Default of an expression template parameter must be an expression"
							);
						}
						return Result::Error;
					}

					const TypeCheckInfo type_check_info = this->type_check<true>(
						*type_id,
						*default_value,
						"Default value of template parameter",
						*template_param_info.param.defaultValue
					);
					if(type_check_info.ok == false){
						return Result::Error;
					}

				}else{
					if(default_value->value_category != TermInfo::ValueCategory::Type){
						this->emit_error(
							Diagnostic::Code::SemaTemplateParamTypeDefaultMustBeType,
							*template_param_info.param.defaultValue,
							"Default of a `Type` template parameter must be an type"
						);
						return Result::Error;
					}
				}
			}else{
				minimum_num_template_args += 1;
			}

			if(default_value == nullptr){
				params.emplace_back(type_id, std::monostate());

			}else if(default_value->value_category == TermInfo::ValueCategory::Type){
				params.emplace_back(type_id, default_value->type_id.as<TypeInfo::VoidableID>());

			}else{
				params.emplace_back(type_id, default_value->getExpr());
			}
		}


		const std::string_view ident_str = this->source.getTokenBuffer()[instr.struct_decl.ident].getString();
		
		const sema::TemplatedStruct::ID new_templated_struct = this->context.sema_buffer.createTemplatedStruct(
			this->symbol_proc, minimum_num_template_args, std::move(params)
		);

		if(this->add_ident_to_scope(ident_str, instr.struct_decl, new_templated_struct) == false){
			return Result::Error;
		}

		this->propagate_finished_decl_def();

		return Result::Success;
	};



	auto SemanticAnalyzer::instr_type_to_term(const Instruction::TypeToTerm& instr) -> Result {
		this->return_term_info(instr.to,
			TermInfo::ValueCategory::Type, TermInfo::ValueStage::Comptime, this->get_type(instr.from), std::nullopt
		);
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
		const TermInfo& location = this->get_term_info(instr.location);

		// TODO: type checking of location

		const std::string_view lookup_path = this->context.getSemaBuffer().getStringValue(
			location.getExpr().stringValueID()
		).value;

		const evo::Expected<Source::ID, Context::LookupSourceIDError> import_lookup = 
			this->context.lookupSourceID(lookup_path, this->source);

		if(import_lookup.has_value()){
			this->return_term_info(instr.output, 
				TermInfo(
					TermInfo::ValueCategory::Module, TermInfo::ValueStage::Comptime, import_lookup.value(), std::nullopt
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


	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::instr_expr_accessor(const Instruction::Accessor<NEEDS_DEF>& instr) -> Result {
		const std::string_view rhs_ident_str = this->source.getTokenBuffer()[instr.rhs_ident].getString();
		const TermInfo& lhs = this->get_term_info(instr.lhs);

		if(lhs.type_id.is<Source::ID>()){
			const Source& source_module = this->context.getSourceManager()[lhs.type_id.as<Source::ID>()];

			const sema::ScopeManager::Scope& source_module_sema_scope = 
				this->context.sema_buffer.scope_manager.getScope(*source_module.sema_scope_id);


			const sema::ScopeLevel& scope_level = this->context.sema_buffer.scope_manager.getLevel(
				source_module_sema_scope.getGlobalLevel()
			);

			const evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> expr_ident = 
				this->analyze_expr_ident_in_scope_level<NEEDS_DEF, true>(
					instr.rhs_ident, rhs_ident_str, scope_level, true, true, &source_module
				);

			if(expr_ident.has_value()){
				this->return_term_info(instr.output, std::move(expr_ident.value()));
				return Result::Success;
			}

			switch(expr_ident.error()){
				case AnalyzeExprIdentInScopeLevelError::DoesntExist:      break;
				case AnalyzeExprIdentInScopeLevelError::NeedsToWaitOnDef: break;
				case AnalyzeExprIdentInScopeLevelError::ErrorEmitted:     return Result::Error;
			}

			return this->wait_on_symbol_proc<NEEDS_DEF>(
				&source_module.global_symbol_procs,
				instr.infix.rhs,
				rhs_ident_str,
				std::format("Module has no symbol named \"{}\"", rhs_ident_str),
				[&]() -> Result {
					const evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> expr_ident = 
						this->analyze_expr_ident_in_scope_level<NEEDS_DEF, true>(
							instr.rhs_ident, rhs_ident_str, scope_level, true, true, &source_module
						);

					if(expr_ident.has_value()){
						this->return_term_info(instr.output, std::move(expr_ident.value()));
						return Result::Success;
					}

					switch(expr_ident.error()){
						case AnalyzeExprIdentInScopeLevelError::DoesntExist:
							evo::debugFatalBreak("Def is done, but can't find sema of symbol");

						case AnalyzeExprIdentInScopeLevelError::NeedsToWaitOnDef:
							evo::debugFatalBreak(
								"Sema doesn't have completed info for def despite SymbolProc saying it should"
							);

						case AnalyzeExprIdentInScopeLevelError::ErrorEmitted: return Result::Error;
					}

					evo::unreachable();
				}
			);

		}else if(lhs.type_id.is<TypeInfo::VoidableID>()){
			if(lhs.type_id.as<TypeInfo::VoidableID>().isVoid()){
				this->emit_error(
					Diagnostic::Code::SemaInvalidAccessorRHS,
					instr.infix.lhs,
					"Accessor operator of type `Void` is invalid"
				);
				return Result::Error;
			}

			const TypeInfo::ID actual_lhs_type_id = this->get_actual_type<true>(
				lhs.type_id.as<TypeInfo::VoidableID>().asTypeID()
			);
			const TypeInfo& actual_lhs_type = this->context.getTypeManager().getTypeInfo(actual_lhs_type_id);

			if(actual_lhs_type.qualifiers().empty() == false){
				// TODO: better message
				this->emit_error(
					Diagnostic::Code::SemaInvalidAccessorRHS,
					instr.infix.lhs,
					"Accessor operator of this LHS is unsupported"
				);
				return Result::Error;
			}

			if(actual_lhs_type.baseTypeID().kind() != BaseType::Kind::Struct){
				// TODO: better message
				this->emit_error(
					Diagnostic::Code::SemaInvalidAccessorRHS,
					instr.infix.lhs,
					"Accessor operator of this LHS is unsupported"
				);
				return Result::Error;	
			}


			const BaseType::Struct& lhs_struct = this->context.getTypeManager().getStruct(
				actual_lhs_type.baseTypeID().structID()
			);

			const Source& struct_source = this->context.getSourceManager()[lhs_struct.sourceID];

			const evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> expr_ident = 
				this->analyze_expr_ident_in_scope_level<NEEDS_DEF, false>(
					instr.rhs_ident, rhs_ident_str, *lhs_struct.scopeLevel, true, true, &struct_source
				);

			if(expr_ident.has_value()){
				this->return_term_info(instr.output, std::move(expr_ident.value()));
				return Result::Success;
			}

			switch(expr_ident.error()){
				case AnalyzeExprIdentInScopeLevelError::DoesntExist:      break;
				case AnalyzeExprIdentInScopeLevelError::NeedsToWaitOnDef: break;
				case AnalyzeExprIdentInScopeLevelError::ErrorEmitted:     return Result::Error;
			}


 			return this->wait_on_symbol_proc<NEEDS_DEF>(
				&lhs_struct.memberSymbols,
				instr.infix.rhs,
				rhs_ident_str,
				std::format("Struct has no member named \"{}\"", rhs_ident_str),
				[&]() -> Result {
					may_break = true;
					EVO_DEFER([&](){ may_break = false; });

					const evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> expr_ident = 
						this->analyze_expr_ident_in_scope_level<NEEDS_DEF, false>(
							instr.rhs_ident, rhs_ident_str, *lhs_struct.scopeLevel, true, true, &struct_source
						);

					if(expr_ident.has_value()){
						this->return_term_info(instr.output, std::move(expr_ident.value()));
						return Result::Success;
					}

					switch(expr_ident.error()){
						case AnalyzeExprIdentInScopeLevelError::DoesntExist: {
							evo::debugFatalBreak("Def is done, but can't find sema of symbol");
						}

						case AnalyzeExprIdentInScopeLevelError::NeedsToWaitOnDef:
							evo::debugFatalBreak(
								"Sema doesn't have completed info for def despite SymbolProc saying it should"
							);

						case AnalyzeExprIdentInScopeLevelError::ErrorEmitted: return Result::Error;
					}

					evo::unreachable();
				}
			);
		}

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			instr.infix.lhs,
			"Accessor operator of this LHS is unimplemented"
		);
		return Result::Error;
	}


	auto SemanticAnalyzer::instr_primitive_type(const Instruction::PrimitiveType& instr) -> Result {
		auto base_type = std::optional<BaseType::ID>();

		const Token::ID primitive_type_token_id = ASTBuffer::getPrimitiveType(instr.ast_type.base);
		const Token& primitive_type_token = this->source.getTokenBuffer()[primitive_type_token_id];

		switch(primitive_type_token.kind()){
			case Token::Kind::TypeVoid: {
				if(instr.ast_type.qualifiers.empty() == false){
					this->emit_error(
						Diagnostic::Code::SemaVoidWithQualifiers,
						instr.ast_type.base,
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
					instr.ast_type,
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
					instr.ast_type,
					"Type \"Type\" may only be used in a template pack declaration"
				);
				return Result::Error;
			} break;

			default: {
				evo::debugFatalBreak("Unknown or unsupported PrimitiveType: {}", primitive_type_token.kind());
			} break;
		}

		evo::debugAssert(base_type.has_value(), "Base type was not set");

		if(this->check_type_qualifiers(instr.ast_type.qualifiers, instr.ast_type) == false){ return Result::Error; }

		this->return_type(
			instr.output,
			TypeInfo::VoidableID(
				this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(*base_type, evo::copy(instr.ast_type.qualifiers))
				)
			)
		);
		return Result::Success;
	}


	auto SemanticAnalyzer::instr_user_type(const Instruction::UserType& instr) -> Result {
		evo::debugAssert(this->get_term_info(instr.base_type).value_category == TermInfo::ValueCategory::Type);
		evo::debugAssert(this->get_term_info(instr.base_type).type_id.as<TypeInfo::VoidableID>().isVoid() == false);

		const TypeInfo::ID base_type_id =
			this->get_term_info(instr.base_type).type_id.as<TypeInfo::VoidableID>().asTypeID();

		const TypeInfo& base_type = this->context.getTypeManager().getTypeInfo(base_type_id);

		if(this->check_type_qualifiers(instr.ast_type.qualifiers, instr.ast_type) == false){ return Result::Error; }

		this->return_type(
			instr.output,
			TypeInfo::VoidableID(
				this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(base_type.baseTypeID(), evo::copy(instr.ast_type.qualifiers))
				)
			)
		);

		return Result::Success;
	}


	auto SemanticAnalyzer::instr_base_type_ident(const Instruction::BaseTypeIdent& instr) -> Result {
		const evo::Expected<TermInfo, Result> lookup_ident_result = this->lookup_ident_impl<true>(instr.ident);
		if(lookup_ident_result.has_value() == false){ return lookup_ident_result.error(); }

		this->return_term_info(instr.output, lookup_ident_result.value());
		return Result::Success;
	}



	auto SemanticAnalyzer::instr_templated_term(const Instruction::TemplatedTerm& instr) -> Result {
		const TermInfo& templated_type_term_info = this->get_term_info(instr.base);

		if(templated_type_term_info.value_category != TermInfo::ValueCategory::TemplateType){
			this->emit_error(
				Diagnostic::Code::SemaNotTemplatedTypeWithTemplateArgs,
				instr.templated_expr.base,
				"Base of templated type is not a template"
			);
			return Result::Error;
		}

		sema::TemplatedStruct& templated_struct = this->context.sema_buffer.templated_structs[
			templated_type_term_info.type_id.as<sema::TemplatedStruct::ID>()
		];


		///////////////////////////////////
		// check args

		if(instr.arguments.size() < templated_struct.minNumTemplateArgs){
			auto infos = evo::SmallVector<Diagnostic::Info>();

			if(templated_struct.hasAnyDefaultParams()){
				infos.emplace_back(
					std::format(
						"This type requires at least {}, got {}",
						templated_struct.minNumTemplateArgs, instr.arguments.size()
					)
				);
			}else{
				infos.emplace_back(
					std::format(
						"This type requires {}, got {}", templated_struct.minNumTemplateArgs, instr.arguments.size()
					)
				);
			}

			this->emit_error(
				Diagnostic::Code::SemaTemplateTooFewArgs,
				instr.templated_expr,
				"Too few template arguments for this type",
				std::move(infos)
			);
			return Result::Error;
		}


		if(instr.arguments.size() > templated_struct.params.size()){
			auto infos = evo::SmallVector<Diagnostic::Info>();

			if(templated_struct.hasAnyDefaultParams()){
				infos.emplace_back(
					std::format(
						"This type requires at most {}, got {}",
						templated_struct.params.size(), instr.arguments.size()
					)
				);
			}else{
				infos.emplace_back(
					std::format(
						"This type requires {}, got {}", templated_struct.params.size(), instr.arguments.size()
					)
				);
			}

			this->emit_error(
				Diagnostic::Code::SemaTemplateTooManyArgs,
				instr.templated_expr,
				"Too many template arguments for this type",
				std::move(infos)
			);
			return Result::Error;
		}


		///////////////////////////////////
		// get instantiation args

		const SemaBuffer& sema_buffer = this->context.getSemaBuffer();

		auto instantiation_lookup_args = evo::SmallVector<sema::TemplatedStruct::Arg>();
		instantiation_lookup_args.reserve(instr.arguments.size());

		auto instantiation_args = evo::SmallVector<evo::Variant<TypeInfo::VoidableID, sema::Expr>>();
		instantiation_args.reserve(instr.arguments.size());
		for(size_t i = 0; const evo::Variant<SymbolProc::TermInfoID, SymbolProc::TypeID>& arg : instr.arguments){
			EVO_DEFER([&](){ i += 1; });

			if(arg.is<SymbolProc::TermInfoID>()){
				TermInfo& arg_term_info = this->get_term_info(arg.as<SymbolProc::TermInfoID>());

				if(arg_term_info.isMultiValue()){
					this->emit_error(
						Diagnostic::Code::SemaMultiReturnIntoSingleValue,
						instr.templated_expr.args[i],
						"Template argument cannot be multiple values"
					);
					return Result::Error;
				}

				if(arg_term_info.value_category == TermInfo::ValueCategory::Type){
					if(templated_struct.params[i].typeID.has_value()){
						const ASTBuffer& ast_buffer = this->source.getASTBuffer();
						const AST::StructDecl& ast_struct =
							ast_buffer.getStructDecl(templated_struct.symbolProc.ast_node);
						const AST::TemplatePack& ast_template_pack =
							ast_buffer.getTemplatePack(*ast_struct.templatePack);

						this->emit_error(
							Diagnostic::Code::SemaTemplateInvalidArg,
							instr.templated_expr.args[i],
							"Expected an expression template argument, got a type",
							Diagnostic::Info(
								"Parameter declared here:", this->get_location(ast_template_pack.params[i].ident)
							)
						);
						return Result::Error;
					}

					instantiation_lookup_args.emplace_back(arg_term_info.type_id.as<TypeInfo::VoidableID>());
					instantiation_args.emplace_back(arg_term_info.type_id.as<TypeInfo::VoidableID>());
					continue;
				}

				if(templated_struct.params[i].typeID.has_value() == false){
					const ASTBuffer& ast_buffer = this->source.getASTBuffer();
					const AST::StructDecl& ast_struct = ast_buffer.getStructDecl(templated_struct.symbolProc.ast_node);
					const AST::TemplatePack& ast_template_pack = ast_buffer.getTemplatePack(*ast_struct.templatePack);

					this->emit_error(
						Diagnostic::Code::SemaTemplateInvalidArg,
						instr.templated_expr.args[i],
						"Expected a type template argument, got an expression",
						Diagnostic::Info(
							"Parameter declared here:", this->get_location(ast_template_pack.params[i].ident)
						)
					);
					return Result::Error;
				}


				const TypeCheckInfo type_check_info = this->type_check<true>(
					*templated_struct.params[i].typeID, arg_term_info, "Template argument", instr.templated_expr.args[i]
				);
				if(type_check_info.ok == false){
					return Result::Error;
				}

				const sema::Expr& arg_expr = arg_term_info.getExpr();
				instantiation_args.emplace_back(arg_expr);
				switch(arg_expr.kind()){
					case sema::Expr::Kind::IntValue: {
						evo::log::debug("int value");
						instantiation_lookup_args.emplace_back(
							core::GenericValue(evo::copy(sema_buffer.getIntValue(arg_expr.intValueID()).value))
						);
					} break;

					case sema::Expr::Kind::FloatValue: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(evo::copy(sema_buffer.getFloatValue(arg_expr.floatValueID()).value))
						);
					} break;

					case sema::Expr::Kind::BoolValue: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(evo::copy(sema_buffer.getBoolValue(arg_expr.boolValueID()).value))
						);
					} break;

					case sema::Expr::Kind::StringValue: {
						evo::debugFatalBreak(
							"String value template args are not supported yet (getting here should be impossible)"
						);
					} break;

					case sema::Expr::Kind::CharValue: {
						instantiation_lookup_args.emplace_back(
							core::GenericValue(evo::copy(sema_buffer.getCharValue(arg_expr.charValueID()).value))
						);
					} break;

					default: evo::debugFatalBreak("Invalid template argument value");
				}
				
			}else{
				if(templated_struct.params[i].typeID.has_value()){
					const ASTBuffer& ast_buffer = this->source.getASTBuffer();
					const AST::StructDecl& ast_struct = ast_buffer.getStructDecl(templated_struct.symbolProc.ast_node);
					const AST::TemplatePack& ast_template_pack = ast_buffer.getTemplatePack(*ast_struct.templatePack);

					this->emit_error(
						Diagnostic::Code::SemaTemplateInvalidArg,
						instr.templated_expr.args[i],
						"Expected an expression template argument, got a type",
						Diagnostic::Info(
							"Parameter declared here:", this->get_location(ast_template_pack.params[i].ident)
						)
					);
					return Result::Error;
				}
				const TypeInfo::VoidableID type_id = this->get_type(arg.as<SymbolProc::TypeID>());
				instantiation_lookup_args.emplace_back(type_id);
				instantiation_args.emplace_back(type_id);
			}
		}


		for(size_t i = instr.arguments.size(); i < templated_struct.params.size(); i+=1){
			templated_struct.params[i].defaultValue.visit([&](const auto& default_value) -> void {
				using DefaultValue = std::decay_t<decltype(default_value)>;

				if constexpr(std::is_same<DefaultValue, std::monostate>()){
					evo::debugFatalBreak("Expected template default value, found none");

				}else if constexpr(std::is_same<DefaultValue, sema::Expr>()){
					switch(default_value.kind()){
						case sema::Expr::Kind::IntValue: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(sema_buffer.getIntValue(default_value.intValueID()).value)
								)
							);
						} break;

						case sema::Expr::Kind::FloatValue: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(sema_buffer.getFloatValue(default_value.floatValueID()).value)
								)
							);
						} break;

						case sema::Expr::Kind::BoolValue: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(sema_buffer.getBoolValue(default_value.boolValueID()).value)
								)
							);
						} break;

						case sema::Expr::Kind::StringValue: {
							evo::debugFatalBreak(
								"String value template args are not supported yet (getting here should be impossible)"
							);
						} break;

						case sema::Expr::Kind::CharValue: {
							instantiation_lookup_args.emplace_back(
								core::GenericValue(
									evo::copy(sema_buffer.getCharValue(default_value.charValueID()).value)
								)
							);
						} break;

						default: evo::debugFatalBreak("Invalid template argument value");
					}
					instantiation_args.emplace_back(default_value);

				}else if constexpr(std::is_same<DefaultValue, TypeInfo::VoidableID>()){
					instantiation_lookup_args.emplace_back(default_value);
					instantiation_args.emplace_back(default_value);

				}else{
					static_assert(false, "Unsupported template default value type");
				}
			});
		}


		const sema::TemplatedStruct::InstantiationInfo instantiation_info =
			templated_struct.lookupInstantiation(std::move(instantiation_lookup_args));

		if(instantiation_info.needs_to_be_compiled){
			auto symbol_proc_builder = SymbolProcBuilder(
				this->context, this->context.source_manager[templated_struct.symbolProc.source_id]
			);

			sema::ScopeManager& scope_manager = this->context.sema_buffer.scope_manager;

			const sema::ScopeManager::Scope::ID instantiation_sema_scope_id = 
				scope_manager.copyScope(*templated_struct.symbolProc.sema_scope_id);


			///////////////////////////////////
			// build instantiation

			const evo::Result<SymbolProc::ID> instantiation_symbol_proc_id = symbol_proc_builder.buildTemplateInstance(
				templated_struct.symbolProc, instantiation_info.instantiation, instantiation_sema_scope_id
			);
			if(instantiation_symbol_proc_id.isError()){ return Result::Error; }

			instantiation_info.instantiation.symbolProcID = instantiation_symbol_proc_id.value();


			///////////////////////////////////
			// add instantiation args to scope

			sema::ScopeManager::Scope& instantiation_sema_scope = scope_manager.getScope(instantiation_sema_scope_id);

			instantiation_sema_scope.pushLevel(scope_manager.createLevel());

			const AST::StructDecl& templated_struct_decl = 
				this->source.getASTBuffer().getStructDecl(templated_struct.symbolProc.ast_node);

			const AST::TemplatePack& ast_template_pack = this->source.getASTBuffer().getTemplatePack(
				*templated_struct_decl.templatePack
			);

			for(size_t i = 0; const evo::Variant<TypeInfo::VoidableID, sema::Expr>& arg : instantiation_args){
				EVO_DEFER([&](){ i += 1; });

				const bool add_ident_result = [&](){
					if(arg.is<TypeInfo::VoidableID>()){
						return this->add_ident_to_scope(
							instantiation_sema_scope,
							this->source.getTokenBuffer()[ast_template_pack.params[i].ident].getString(),
							ast_template_pack.params[i].ident,
							arg.as<TypeInfo::VoidableID>(),
							ast_template_pack.params[i].ident
						);

					}else{
						return this->add_ident_to_scope(
							instantiation_sema_scope,
							this->source.getTokenBuffer()[ast_template_pack.params[i].ident].getString(),
							ast_template_pack.params[i].ident,
							*templated_struct.params[i].typeID,
							arg.as<sema::Expr>(),
							ast_template_pack.params[i].ident	
						);
					}
				}();

				if(add_ident_result == false){ return Result::Error; }
			}

			// wait on instantiation
			SymbolProc& instantiation_symbol_proc = this->context.symbol_proc_manager.getSymbolProc(
				instantiation_symbol_proc_id.value()
			);
			SymbolProc::WaitOnResult wait_on_result = instantiation_symbol_proc.waitOnDeclIfNeeded(
				this->symbol_proc_id, this->context, instantiation_symbol_proc_id.value()
			);
			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NotNeeded:  evo::debugFatalBreak("Should never be possible");
				case SymbolProc::WaitOnResult::Waiting:    break;
				case SymbolProc::WaitOnResult::WasErrored: evo::debugFatalBreak("Should never be possible");
				case SymbolProc::WaitOnResult::WasPassedOnByWhenCond: evo::debugFatalBreak("Should never be possible");
				case SymbolProc::WaitOnResult::CircularDepDetected:   return Result::Error; // not sure this is possible
																						    // 	but just in case
			}

			this->return_struct_instantiation(instr.instantiation, instantiation_info.instantiation);


			this->context.add_task_to_work_manager(instantiation_symbol_proc_id.value());


			return Result::NeedToWaitBeforeNextInstr;

		}else{
			this->return_struct_instantiation(instr.instantiation, instantiation_info.instantiation);

			// TODO: better way of doing this?
			while(instantiation_info.instantiation.symbolProcID.load().has_value() == false){
				std::this_thread::yield();
			}

			SymbolProc& instantiation_symbol_proc = this->context.symbol_proc_manager.getSymbolProc(
				*instantiation_info.instantiation.symbolProcID.load()
			);

			SymbolProc::WaitOnResult wait_on_result = instantiation_symbol_proc.waitOnDeclIfNeeded(
				this->symbol_proc_id, this->context, *instantiation_info.instantiation.symbolProcID.load()
			);
			switch(wait_on_result){
				case SymbolProc::WaitOnResult::NotNeeded:  return Result::Success;
				case SymbolProc::WaitOnResult::Waiting:    return Result::NeedToWaitBeforeNextInstr;
				case SymbolProc::WaitOnResult::WasErrored: return Result::Error;
				case SymbolProc::WaitOnResult::WasPassedOnByWhenCond: evo::debugFatalBreak("Should never be possible");
				case SymbolProc::WaitOnResult::CircularDepDetected:   return Result::Error; // not sure this is possible
																						    //  but just in case
			}

			evo::unreachable();
		}
		
	}



	auto SemanticAnalyzer::instr_templated_term_wait(const Instruction::TemplatedTermWait& instr) -> Result {
		const sema::TemplatedStruct::Instantiation& instantiation = this->get_struct_instantiation(instr.instantiation);

		if(instantiation.errored.load()){ return Result::Error; }
		// if(instantiation.structID.load().has_value() == false){ return Result::NeedToWaitOnInstantiation; }
		evo::debugAssert(instantiation.structID.has_value(), "Should already be completed");

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::Type,
			TermInfo::ValueStage::Comptime,
			TypeInfo::VoidableID(
				this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(*instantiation.structID)))
			),
			std::nullopt
		);

		return Result::Success;
	}



	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::instr_ident(const Instruction::Ident<NEEDS_DEF>& instr) -> Result {
		const evo::Expected<TermInfo, Result> lookup_ident_result = this->lookup_ident_impl<NEEDS_DEF>(instr.ident);
		if(lookup_ident_result.has_value() == false){ return lookup_ident_result.error(); }

		this->return_term_info(instr.output, std::move(lookup_ident_result.value()));
		return Result::Success;
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
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EphemeralFluid,
					TermInfo::ValueStage::Comptime,
					TermInfo::FluidType{},
					sema::Expr(this->context.sema_buffer.createIntValue(
						core::GenericInt(256, literal_token.getInt()), std::nullopt
					))
				);
				return Result::Success;
			} break;

			case Token::Kind::LiteralFloat: {
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::EphemeralFluid,
					TermInfo::ValueStage::Comptime,
					TermInfo::FluidType{},
					sema::Expr(this->context.sema_buffer.createFloatValue(
						core::GenericFloat::createF128(literal_token.getFloat()), std::nullopt
					))
				);
				return Result::Success;
			} break;

			case Token::Kind::LiteralBool: {
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::Ephemeral,
					TermInfo::ValueStage::Comptime,
					this->context.getTypeManager().getTypeBool(),
					sema::Expr(this->context.sema_buffer.createBoolValue(literal_token.getBool()))
				);
				return Result::Success;
			} break;

			case Token::Kind::LiteralString: {
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::Ephemeral,
					TermInfo::ValueStage::Comptime,
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
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::Ephemeral,
					TermInfo::ValueStage::Comptime,
					this->context.getTypeManager().getTypeChar(),
					sema::Expr(this->context.sema_buffer.createCharValue(literal_token.getChar()))
				);
				return Result::Success;
			} break;

			default: evo::debugFatalBreak("Not a valid literal");
		}
	}


	auto SemanticAnalyzer::instr_uninit(const Instruction::Uninit& instr) -> Result {
		this->return_term_info(instr.output,
			TermInfo::ValueCategory::Initializer,
			TermInfo::ValueStage::Comptime,
			TermInfo::InitializerType(),
			sema::Expr(this->context.sema_buffer.createUninit(instr.uninit_token))
		);
		return Result::Success;
	}

	auto SemanticAnalyzer::instr_zeroinit(const Instruction::Zeroinit& instr) -> Result {
		this->return_term_info(instr.output,
			TermInfo::ValueCategory::Initializer,
			TermInfo::ValueStage::Comptime,
			TermInfo::InitializerType(),
			sema::Expr(this->context.sema_buffer.createZeroinit(instr.zeroinit_token))
		);
		return Result::Success;
	}



	///////////////////////////////////
	// misc

	auto SemanticAnalyzer::get_current_scope_level() const -> sema::ScopeLevel& {
		return this->context.sema_buffer.scope_manager.getLevel(this->scope.getCurrentLevel());
	}


	auto SemanticAnalyzer::push_scope_level(const auto& object_scope_id) -> void {
		this->scope.pushLevel(this->context.sema_buffer.scope_manager.createLevel(), object_scope_id);
	}

	auto SemanticAnalyzer::push_scope_level() -> void {
		this->scope.pushLevel(this->context.sema_buffer.scope_manager.createLevel());
	}

	auto SemanticAnalyzer::pop_scope_level() -> void {
		this->scope.popLevel();
	}




	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::lookup_ident_impl(Token::ID ident) -> evo::Expected<TermInfo, Result> {
		const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();

		// in-scope sema params
		for(size_t i = this->scope.size() - 1; sema::ScopeLevel::ID scope_level_id : this->scope){
			const evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> scope_level_lookup = 
				this->analyze_expr_ident_in_scope_level<NEEDS_DEF, false>(
					ident,
					ident_str,
					this->context.sema_buffer.scope_manager.getLevel(scope_level_id),
					i >= this->scope.getCurrentObjectScopeIndex() || i == 0,
					i == 0,
					nullptr
				);

			if(scope_level_lookup.has_value()){
				return scope_level_lookup.value();
			}

			if(scope_level_lookup.error() == AnalyzeExprIdentInScopeLevelError::ErrorEmitted){
				return evo::Unexpected(Result::Error);
			}
			if(scope_level_lookup.error() == AnalyzeExprIdentInScopeLevelError::NeedsToWaitOnDef){ break; }

			i -= 1;
		}



		// look in symbol procs
		auto symbol_proc_namespaces = evo::SmallVector<const SymbolProc::Namespace*>();

		SymbolProc* parent_symbol = this->symbol_proc.parent;
		while(parent_symbol != nullptr){
			if(parent_symbol->extra_info.is<SymbolProc::StructInfo>()){
				symbol_proc_namespaces.emplace_back(
					&parent_symbol->extra_info.as<SymbolProc::StructInfo>().member_symbols
				);
			}
			
			parent_symbol = parent_symbol->parent;
		}
		symbol_proc_namespaces.emplace_back(&this->source.global_symbol_procs);

		auto maybe_output = std::optional<TermInfo>();
		const Result wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
			symbol_proc_namespaces,
			ident,
			ident_str,
			std::format("Identifier \"{}\" was not defined in this scope", ident_str),
			[&]() -> Result {
				for(size_t i = this->scope.size() - 1; sema::ScopeLevel::ID scope_level_id : this->scope){
					const evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> scope_level_lookup = 
						this->analyze_expr_ident_in_scope_level<NEEDS_DEF, false>(
							ident,
							ident_str,
							this->context.sema_buffer.scope_manager.getLevel(scope_level_id),
							i >= this->scope.getCurrentObjectScopeIndex() || i == 0,
							i == 0,
							nullptr
						);

					if(scope_level_lookup.has_value()){
						maybe_output = scope_level_lookup.value();
						return Result::Success;
					}

					switch(scope_level_lookup.error()){
						case AnalyzeExprIdentInScopeLevelError::DoesntExist: break;

						case AnalyzeExprIdentInScopeLevelError::NeedsToWaitOnDef:
							evo::debugFatalBreak(
								"Sema doesn't have completed info for def despite SymbolProc saying it should"
							);

						case AnalyzeExprIdentInScopeLevelError::ErrorEmitted: return Result::Error;
					}

					i -= 1;
				}

				evo::debugFatalBreak("Should never get here");
			}
		);
		if(wait_on_symbol_proc_result == Result::Success){ return *maybe_output; }

		evo::debugAssert(maybe_output.has_value() == false, "`maybe_output` should not be set");

		return evo::Unexpected(wait_on_symbol_proc_result);
	}




	template<bool NEEDS_DEF, bool PUB_REQUIRED>
	auto SemanticAnalyzer::analyze_expr_ident_in_scope_level(
		const Token::ID& ident,
		std::string_view ident_str,
		const sema::ScopeLevel& scope_level,
		bool variables_in_scope, // TODO: make this template argument?
		bool is_global_scope, // TODO: make this template argumnet?
		const Source* source_module
	) -> evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError> {
		if constexpr(PUB_REQUIRED){
			evo::debugAssert(variables_in_scope, "IF `PUB_REQUIRED`, `variables_in_scope` should be true");
			evo::debugAssert(is_global_scope, "IF `PUB_REQUIRED`, `is_global_scope` should be true");
		}

		const sema::ScopeLevel::IdentID* ident_id_lookup = scope_level.lookupIdent(ident_str);
		if(ident_id_lookup == nullptr){
			return evo::Unexpected(AnalyzeExprIdentInScopeLevelError::DoesntExist);
		}


		using ReturnType = evo::Expected<TermInfo, AnalyzeExprIdentInScopeLevelError>;

		return ident_id_lookup->visit([&](const auto& ident_id) -> ReturnType {
			using IdentIDType = std::decay_t<decltype(ident_id)>;

			if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()){
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ident,
					"function identifiers are unimplemented"
				);
				return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ErrorEmitted));

			}else if constexpr(std::is_same<IdentIDType, sema::VarID>()){
				if(!variables_in_scope){
					// TODO: better messaging
					this->emit_error(
						Diagnostic::Code::SemaIdentNotInScope,
						ident,
						std::format("Variable \"{}\" is not accessable in this scope", ident_str),
						Diagnostic::Info(
							"Local variables, parameters, and members cannot be accessed inside a sub-object scope. "
								"Defined here:",
							this->get_location(ident_id)
						)
					);
					return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ErrorEmitted));
				}

				const sema::Var& sema_var = this->context.getSemaBuffer().getVar(ident_id);

				if constexpr(PUB_REQUIRED){
					if(sema_var.isPub == false){
						this->emit_error(
							Diagnostic::Code::SemaSymbolNotPub,
							ident,
							std::format("Variable \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Variable defined here:", 
								Diagnostic::Location::get(ident_id, *source_module, this->context)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ErrorEmitted));
					}

				}

				using ValueCategory = TermInfo::ValueCategory;
				using ValueStage = TermInfo::ValueStage;

				switch(sema_var.kind){
					case AST::VarDecl::Kind::Var: {
						if constexpr(NEEDS_DEF){
							if(sema_var.expr.load().has_value() == false){
								return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NeedsToWaitOnDef));
							}
						}
						
						return ReturnType(TermInfo(
							ValueCategory::ConcreteMut,
							is_global_scope ? ValueStage::Runtime : ValueStage::Constexpr,
							*sema_var.typeID,
							sema::Expr(ident_id)
						));
					} break;

					case AST::VarDecl::Kind::Const: {
						if constexpr(NEEDS_DEF){
							if(sema_var.expr.load().has_value() == false){
								return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NeedsToWaitOnDef));
							}
						}

						return ReturnType(TermInfo(
							ValueCategory::ConcreteConst, ValueStage::Constexpr, *sema_var.typeID, sema::Expr(ident_id)
						));
					} break;

					case AST::VarDecl::Kind::Def: {
						if(sema_var.typeID.has_value()){
							return ReturnType(TermInfo(
								ValueCategory::Ephemeral, ValueStage::Comptime, *sema_var.typeID, *sema_var.expr.load()
							));
						}else{
							return ReturnType(TermInfo(
								ValueCategory::EphemeralFluid,
								ValueStage::Comptime,
								TermInfo::FluidType{},
								*sema_var.expr.load()
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
				return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ErrorEmitted));

			}else if constexpr(std::is_same<IdentIDType, sema::ReturnParamID>()){
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ident,
					"return parameter identifiers are unimplemented"
				);
				return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ErrorEmitted));

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::ModuleInfo>()){
				if constexpr(PUB_REQUIRED){
					if(ident_id.isPub == false){
						this->emit_error(
							Diagnostic::Code::SemaSymbolNotPub,
							ident_id,
							std::format("Identifier \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Defined here:",
								Diagnostic::Location::get(ident_id.tokenID, *source_module)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ErrorEmitted));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::Module,
						TermInfo::ValueStage::Comptime,
						ident_id.sourceID,
						sema::Expr::createModuleIdent(ident_id.tokenID)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, BaseType::Alias::ID>()){
				const BaseType::Alias& alias = this->context.getTypeManager().getAlias(ident_id);

				if constexpr(NEEDS_DEF){
					if(alias.defCompleted() == false){
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NeedsToWaitOnDef));
					}
				}

				if constexpr(PUB_REQUIRED){
					if(alias.isPub == false){
						this->emit_error(
							Diagnostic::Code::SemaSymbolNotPub,
							ident,
							std::format("Alias \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Alias declared here:",
								Diagnostic::Location::get(ident_id, *source_module, this->context)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ErrorEmitted));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::Type,
						TermInfo::ValueStage::Comptime,
						TypeInfo::VoidableID(
							this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(ident_id)))
						),
						std::nullopt
					)
				);

			}else if constexpr(std::is_same<IdentIDType, BaseType::Typedef::ID>()){
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ident,
					"Using typedefs is currently unimplemented"
				);
				return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ErrorEmitted));

			}else if constexpr(std::is_same<IdentIDType, BaseType::Struct::ID>()){
				const BaseType::Struct& struct_info = this->context.getTypeManager().getStruct(ident_id);

				if constexpr(NEEDS_DEF){
					if(struct_info.defCompleted == false){
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::NeedsToWaitOnDef));
					}
				}

				if constexpr(PUB_REQUIRED){
					if(struct_info.isPub == false){
						this->emit_error(
							Diagnostic::Code::SemaSymbolNotPub,
							ident,
							std::format("Struct \"{}\" does not have the #pub attribute", ident_str),
							Diagnostic::Info(
								"Struct declared here:",
								Diagnostic::Location::get(ident_id, *source_module, this->context)
							)
						);
						return ReturnType(evo::Unexpected(AnalyzeExprIdentInScopeLevelError::ErrorEmitted));
					}
				}

				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::Type,
						TermInfo::ValueStage::Comptime,
						TypeInfo::VoidableID(
							this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(ident_id)))
						),
						std::nullopt
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::TemplatedStruct::ID>()){
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::TemplateType,
						TermInfo::ValueStage::Comptime,
						ident_id,
						std::nullopt
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::TemplateTypeParam>()){
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::Type,
						TermInfo::ValueStage::Comptime,
						ident_id.typeID,
						std::nullopt
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::TemplateExprParam>()){
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::Ephemeral,
						TermInfo::ValueStage::Comptime,
						ident_id.typeID,
						ident_id.value
					)
				);

			}else{
				static_assert(false, "Unsupported IdentID");
			}
		});
	}


	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::wait_on_symbol_proc(
		evo::ArrayProxy<const SymbolProc::Namespace*> symbol_proc_namespaces,
		const auto& ident,
		std::string_view ident_str,
		std::string&& error_msg_if_ident_doesnt_exist,
		std::function<Result()> func_if_def_completed
	) -> Result {
		auto found_range = std::optional<core::IterRange<SymbolProc::Namespace::const_iterator>>();
		for(const SymbolProc::Namespace* symbol_proc_namespace : symbol_proc_namespaces){
			const auto find = symbol_proc_namespace->equal_range(ident_str);

			if(find.first != symbol_proc_namespace->end()){
				found_range.emplace(find.first, find.second);
				break;
			}			
		}
		if(found_range.has_value() == false){
			this->emit_error(
				Diagnostic::Code::SemaNoSymbolInModuleWithThatIdent,
				ident,
				std::move(error_msg_if_ident_doesnt_exist)
			);
			return Result::Error;
		}



		bool found_was_passed_by_when_cond = false;
		for(auto& pair : *found_range){
			const SymbolProc::ID& found_symbol_proc_id = pair.second;
			SymbolProc& found_symbol_proc = this->context.symbol_proc_manager.getSymbolProc(found_symbol_proc_id);

			const SymbolProc::WaitOnResult wait_on_result = [&](){
				if constexpr(NEEDS_DEF){
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
					// call again as symbol finished
					return func_if_def_completed();
				} break;

				case SymbolProc::WaitOnResult::Waiting: {
					// do nothing...
				} break;

				case SymbolProc::WaitOnResult::WasErrored: {
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


		if(this->symbol_proc.isWaiting()){ return Result::NeedToWait; }

		if(found_was_passed_by_when_cond){
			this->emit_error(
				Diagnostic::Code::SemaNoSymbolInModuleWithThatIdent,
				ident,
				std::move(error_msg_if_ident_doesnt_exist),
				Diagnostic::Info("The identifier was declared in a when conditional block that wasn't taken")
			);
			return Result::Error;

		}else{
			// was waiting on, but it completed already
			return func_if_def_completed();
		}
	}





	auto SemanticAnalyzer::set_waiting_for_is_done(SymbolProc::ID target_id, SymbolProc::ID done_id) -> void {
		SymbolProc& target = this->context.symbol_proc_manager.getSymbolProc(target_id);

		const auto lock = std::scoped_lock(target.waiting_for_lock);


		evo::debugAssert(target.waiting_for.empty() == false, "Should never have empty list");

		for(size_t i = 0; i < target.waiting_for.size() - 1; i+=1){
			if(target.waiting_for[i] == done_id){
				target.waiting_for[i] = target.waiting_for.back();
				break;
			}
		}

		target.waiting_for.pop_back();

		if(
			target.waiting_for.empty() && 
			target.passed_on_by_when_cond == false && 
			target.errored == false && 
			target.isTemplateSubSymbol() == false
		){
			this->context.add_task_to_work_manager(target_id);
		}
	}


	template<bool ALLOW_TYPEDEF>
	auto SemanticAnalyzer::get_actual_type(TypeInfo::ID type_id) const -> TypeInfo::ID {
		const TypeManager& type_manager = this->context.getTypeManager();

		while(true){
			const TypeInfo& type_info = type_manager.getTypeInfo(type_id);
			if(type_info.qualifiers().empty() == false){ return type_id; }


			if(type_info.baseTypeID().kind() == BaseType::Kind::Alias){
				const BaseType::Alias& alias = type_manager.getAlias(type_info.baseTypeID().aliasID());

				evo::debugAssert(alias.aliasedType.load().has_value(), "Definition of alias was not completed");
				type_id = *alias.aliasedType.load();

			}else if(type_info.baseTypeID().kind() == BaseType::Kind::Typedef){
				if constexpr(ALLOW_TYPEDEF){
					const BaseType::Typedef& typedef_info = type_manager.getTypedef(type_info.baseTypeID().typedefID());

					evo::debugAssert(
						typedef_info.underlyingType.load().has_value(), "Definition of typedef was not completed"
					);
					type_id = *typedef_info.underlyingType.load();

				}else{
					return type_id;	
				}

			}else{
				return type_id;
			}
		}
	}



	auto SemanticAnalyzer::analyze_var_attrs(
		const AST::VarDecl& var_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<VarAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");

		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(var_decl.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(attribute_params_info[i].empty()){
					if(attr_pub.set(attribute.attribute, true) == false){ return evo::resultError; } 

				}else if(attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]) == false){return evo::resultError;}

					if(this->type_check<true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return evo::resultError;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

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



	auto SemanticAnalyzer::analyze_struct_attrs(
		const AST::StructDecl& struct_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<StructAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");

		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(struct_decl.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(attribute_params_info[i].empty()){
					if(attr_pub.set(attribute.attribute, true) == false){ return evo::resultError; } 

				}else if(attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]) == false){return evo::resultError;}

					if(this->type_check<true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return evo::resultError;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

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
					std::format("Unknown structiable attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}


		return StructAttrs(attr_pub.is_set());
	}




	auto SemanticAnalyzer::propagate_finished_impl(const evo::SmallVector<SymbolProc::ID>& waited_on_by_list) -> void {
		for(const SymbolProc::ID& waited_on_id : waited_on_by_list){
			SymbolProc& waited_on = this->context.symbol_proc_manager.getSymbolProc(waited_on_id);
			const auto lock = std::scoped_lock(waited_on.waiting_for_lock);

			evo::debugAssert(waited_on.waiting_for.empty() == false, "Should never have empty list");

			for(size_t i = 0; i < waited_on.waiting_for.size() - 1; i+=1){
				if(waited_on.waiting_for[i] == this->symbol_proc_id){
					waited_on.waiting_for[i] = waited_on.waiting_for.back();
					break;
				}
			}

			waited_on.waiting_for.pop_back();

			if(waited_on.waiting_for.empty() && waited_on.isTemplateSubSymbol() == false){
				this->context.add_task_to_work_manager(waited_on_id);
			}
		}
	}


	auto SemanticAnalyzer::propagate_finished_decl() -> void {
		const auto lock = std::scoped_lock(this->symbol_proc.decl_waited_on_lock);

		this->symbol_proc.decl_done = true;
		this->propagate_finished_impl(this->symbol_proc.decl_waited_on_by);
	}


	auto SemanticAnalyzer::propagate_finished_def() -> void {
		const auto lock = std::scoped_lock(this->symbol_proc.def_waited_on_lock);

		this->symbol_proc.def_done = true;
		this->propagate_finished_impl(this->symbol_proc.def_waited_on_by);

		this->context.symbol_proc_manager.symbol_proc_done();
	}



	auto SemanticAnalyzer::propagate_finished_decl_def() -> void {
		const auto lock = std::scoped_lock(this->symbol_proc.decl_waited_on_lock, this->symbol_proc.def_waited_on_lock);		

		this->symbol_proc.decl_done = true;
		this->symbol_proc.def_done = true;

		this->propagate_finished_impl(this->symbol_proc.decl_waited_on_by);
		this->propagate_finished_impl(this->symbol_proc.def_waited_on_by);

		this->context.symbol_proc_manager.symbol_proc_done();
	}



	//////////////////////////////////////////////////////////////////////
	// exec value gets / returns


	auto SemanticAnalyzer::get_type(SymbolProc::TypeID symbol_proc_type_id) -> TypeInfo::VoidableID {
		return *this->symbol_proc.type_ids[symbol_proc_type_id.get()];
	}

	auto SemanticAnalyzer::return_type(SymbolProc::TypeID symbol_proc_type_id, TypeInfo::VoidableID&& id) -> void {
		this->symbol_proc.type_ids[symbol_proc_type_id.get()] = std::move(id);
	}


	auto SemanticAnalyzer::get_term_info(SymbolProc::TermInfoID symbol_proc_term_info_id) -> TermInfo& {
		return *this->symbol_proc.term_infos[symbol_proc_term_info_id.get()];
	}

	auto SemanticAnalyzer::return_term_info(SymbolProc::TermInfoID symbol_proc_term_info_id, auto&&... args) -> void {
		this->symbol_proc.term_infos[symbol_proc_term_info_id.get()]
			.emplace(std::forward<decltype(args)>(args)...);
	}



	auto SemanticAnalyzer::get_struct_instantiation(SymbolProc::StructInstantiationID instantiation_id)
	-> const sema::TemplatedStruct::Instantiation& {
		return *this->symbol_proc.struct_instantiations[instantiation_id.get()];
	}

	auto SemanticAnalyzer::return_struct_instantiation(
		SymbolProc::StructInstantiationID instantiation_id,
		const sema::TemplatedStruct::Instantiation& instantiation
	) -> void {
		this->symbol_proc.struct_instantiations[instantiation_id.get()] = &instantiation;
	}



	//////////////////////////////////////////////////////////////////////
	// error handling / diagnostics

	template<bool IS_NOT_ARGUMENT>
	auto SemanticAnalyzer::type_check(
		TypeInfo::ID expected_type_id,
		TermInfo& got_expr,
		std::string_view expected_type_location_name,
		const auto& location
	) -> TypeCheckInfo {
		evo::debugAssert(
			std::isupper(int(expected_type_location_name[0])),
			"first character of expected_type_location_name should be upper-case"
		);

		TypeInfo::ID actual_expected_type_id = this->get_actual_type<false>(expected_type_id);

		switch(got_expr.value_category){
			case TermInfo::ValueCategory::Ephemeral:
			case TermInfo::ValueCategory::ConcreteConst:
			case TermInfo::ValueCategory::ConcreteMut:
			case TermInfo::ValueCategory::ConcreteConstForwardable:
			case TermInfo::ValueCategory::ConcreteConstDestrMovable: {
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

				TypeInfo::ID actual_got_type_id = this->get_actual_type<false>(got_expr.type_id.as<TypeInfo::ID>());

				// if types are not exact, check if implicit conversion is valid
				if(actual_expected_type_id != actual_got_type_id){
					const TypeInfo& expected_type = this->context.getTypeManager().getTypeInfo(actual_expected_type_id);
					const TypeInfo& got_type      = this->context.getTypeManager().getTypeInfo(actual_got_type_id);

					if(
						expected_type.baseTypeID()        != got_type.baseTypeID() || 
						expected_type.qualifiers().size() != got_type.qualifiers().size()
					){	

						if constexpr(IS_NOT_ARGUMENT){
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
							if constexpr(IS_NOT_ARGUMENT){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location
								);
							}
							return TypeCheckInfo(false, false);
						}
						if(expected_qualifier.isReadOnly == false && got_qualifier.isReadOnly){
							if constexpr(IS_NOT_ARGUMENT){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location
								);
							}
							return TypeCheckInfo(false, false);
						}
					}
				}

				if constexpr(IS_NOT_ARGUMENT){
					EVO_DEFER([&](){ got_expr.type_id.emplace<TypeInfo::ID>(expected_type_id); });
				}

				return TypeCheckInfo(true, got_expr.type_id.as<TypeInfo::ID>() != expected_type_id);
			} break;

			case TermInfo::ValueCategory::EphemeralFluid: {
				const TypeInfo& expected_type_info = 
					this->context.getTypeManager().getTypeInfo(actual_expected_type_id);

				if(
					expected_type_info.qualifiers().empty() == false || 
					expected_type_info.baseTypeID().kind() != BaseType::Kind::Primitive
				){
					if constexpr(IS_NOT_ARGUMENT){
						this->error_type_mismatch(
							expected_type_id, got_expr, expected_type_location_name, location
						);
					}
					return TypeCheckInfo(false, false);
				}

				const BaseType::Primitive::ID expected_type_primitive_id =
					expected_type_info.baseTypeID().primitiveID();

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
							if constexpr(IS_NOT_ARGUMENT){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location
								);
							}
							return TypeCheckInfo(false, false);
						}
					}

					if constexpr(IS_NOT_ARGUMENT){
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

						core::GenericInt target_min = type_manager.getMin(expected_type_info.baseTypeID())
							.as<core::GenericInt>();

						core::GenericInt target_max = type_manager.getMax(expected_type_info.baseTypeID())
							.as<core::GenericInt>();

						if(int_value.value.getBitWidth() >= target_min.getBitWidth()){
							target_min = target_min.ext(int_value.value.getBitWidth(), is_unsigned);
							target_max = target_max.ext(int_value.value.getBitWidth(), is_unsigned);

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

						}else{
							int_value.value = int_value.value.ext(target_min.getBitWidth(), is_unsigned);

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
							if constexpr(IS_NOT_ARGUMENT){
								this->error_type_mismatch(
									expected_type_id, got_expr, expected_type_location_name, location
								);
							}
							return TypeCheckInfo(false, false);
						}
					}

					if constexpr(IS_NOT_ARGUMENT){
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

				if constexpr(IS_NOT_ARGUMENT){
					got_expr.value_category = TermInfo::ValueCategory::Ephemeral;
					got_expr.type_id.emplace<TypeInfo::ID>(expected_type_id);
				}

				return TypeCheckInfo(true, true);
			} break;

			case TermInfo::ValueCategory::Initializer:
				evo::debugFatalBreak("Initializer should not be compared with this function");

			case TermInfo::ValueCategory::Module:
				evo::debugFatalBreak("Module should not be compared with this function");

			case TermInfo::ValueCategory::Function:
				evo::debugFatalBreak("Function should not be compared with this function");

			case TermInfo::ValueCategory::Intrinsic:
				evo::debugFatalBreak("Intrinsic should not be compared with this function");

			case TermInfo::ValueCategory::TemplateIntrinsic:
				evo::debugFatalBreak("TemplateIntrinsic should not be compared with this function");

			case TermInfo::ValueCategory::TemplateType:
				evo::debugFatalBreak("TemplateType should not be compared with this function");
		}

		evo::unreachable();
	}


	auto SemanticAnalyzer::error_type_mismatch(
		TypeInfo::ID expected_type_id,
		const TermInfo& got_expr,
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

		TypeInfo::ID actual_expected_type_id = expected_type_id;
		// TODO: improve perf
		while(true){
			const TypeInfo& actual_expected_type = this->context.getTypeManager().getTypeInfo(actual_expected_type_id);
			if(actual_expected_type.qualifiers().empty() == false){ break; }
			if(actual_expected_type.baseTypeID().kind() != BaseType::Kind::Alias){ break; }

			const BaseType::Alias& expected_alias = this->context.getTypeManager().getAlias(
				actual_expected_type.baseTypeID().aliasID()
			);

			evo::debugAssert(expected_alias.aliasedType.load().has_value(), "Definition of alias was not completed");
			actual_expected_type_id = *expected_alias.aliasedType.load();

			auto alias_of_str = std::string("\\-> Alias of: ");
			while(alias_of_str.size() < got_type_str.size()){
				alias_of_str += ' ';
			}

			infos.emplace_back(
				alias_of_str + 
				this->context.getTypeManager().printType(actual_expected_type_id, this->context.getSourceManager())
			);
		}


		infos.emplace_back(got_type_str + this->print_type(got_expr));

		if(got_expr.type_id.is<TypeInfo::ID>()){
			TypeInfo::ID actual_got_type_id = got_expr.type_id.as<TypeInfo::ID>();
			// TODO: improve perf
			while(true){
				const TypeInfo& actual_got_type = this->context.getTypeManager().getTypeInfo(actual_got_type_id);
				if(actual_got_type.qualifiers().empty() == false){ break; }
				if(actual_got_type.baseTypeID().kind() != BaseType::Kind::Alias){ break; }

				const BaseType::Alias& got_alias = this->context.getTypeManager().getAlias(
					actual_got_type.baseTypeID().aliasID()
				);

				evo::debugAssert(got_alias.aliasedType.load().has_value(), "Definition of alias was not completed");
				actual_got_type_id = *got_alias.aliasedType.load();

				auto alias_of_str = std::string("\\-> Alias of: ");
				while(alias_of_str.size() < got_type_str.size()){
					alias_of_str += ' ';
				}

				infos.emplace_back(
					alias_of_str + 
					this->context.getTypeManager().printType(actual_got_type_id, this->context.getSourceManager())
				);
			}
		}

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



	auto SemanticAnalyzer::check_term_isnt_type(const TermInfo& term_info, const auto& location) -> bool {
		if(term_info.value_category == TermInfo::ValueCategory::Type){
			this->emit_error(Diagnostic::Code::SemaTypeUsedAsExpr, location, "Type used as an expression");
			return false;
		}

		return true;
	}



	auto SemanticAnalyzer::add_ident_to_scope(
		sema::ScopeManager::Scope& target_scope,
		std::string_view ident_str,
		const auto& ast_node,
		auto&&... ident_id_info
	) -> bool {
		sema::ScopeLevel& current_scope_level = 
			this->context.sema_buffer.scope_manager.getLevel(target_scope.getCurrentLevel());

		const sema::ScopeLevel::AddIdentResult add_ident_result = current_scope_level.addIdent(
			ident_str, std::forward<decltype(ident_id_info)>(ident_id_info)...
		);

		if(add_ident_result.has_value() == false){
			const bool is_shadow_redef = add_ident_result.error();
			if(is_shadow_redef){
				const sema::ScopeLevel::IdentID& shadowed_ident =
					*current_scope_level.lookupDisallowedIdentForShadowing(ident_str);

				shadowed_ident.visit([&](const auto& first_decl_ident_id) -> void {
					using IdentIDType = std::decay_t<decltype(first_decl_ident_id)>;

					auto infos = evo::SmallVector<Diagnostic::Info>();

					infos.emplace_back("First defined here:", this->get_location(ast_node));

					const Diagnostic::Location first_ident_location = [&]() -> Diagnostic::Location {
						if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()){
							return first_decl_ident_id.front().visit([&](const auto& func_id) -> Diagnostic::Location {
								return this->get_location(func_id);	
							});

						}else{
							return this->get_location(first_decl_ident_id);
						}
					}();
					
					infos.emplace_back("Note: shadowing is not allowed");
					
					this->emit_error(
						Diagnostic::Code::SemaIdentAlreadyInScope,
						ast_node,
						std::format("Identifier \"{}\" was already defined in this scope", ident_str),
						std::move(infos)
					);
				});

			}else{
				this->error_already_defined<false>(ast_node, ident_str, *current_scope_level.lookupIdent(ident_str));
			}

			return false;
		}


		for(auto iter = std::next(target_scope.begin()); iter != target_scope.end(); ++iter){
			sema::ScopeLevel& scope_level = this->context.sema_buffer.scope_manager.getLevel(*iter);
			if(scope_level.disallowIdentForShadowing(ident_str, add_ident_result.value()) == false){
				this->error_already_defined<true>(ast_node, ident_str, *scope_level.lookupIdent(ident_str));
				return false;
			}
		}

		return true;
	}


	template<bool IS_SHADOWING>
	auto SemanticAnalyzer::error_already_defined(
		const auto& redef_id, std::string_view ident_str, const sema::ScopeLevel::IdentID& first_defined_id
	)  -> void {
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


			if constexpr(IS_SHADOWING){
				infos.emplace_back("Note: shadowing is not allowed");
			}


			this->emit_error(
				Diagnostic::Code::SemaIdentAlreadyInScope,
				redef_id,
				std::format("Identifier \"{}\" was already defined in this scope", ident_str),
				std::move(infos)
			);
		});
	};



	auto SemanticAnalyzer::print_type(const TermInfo& term_info) const -> std::string {
		return term_info.type_id.visit([&](const auto& type_id) -> std::string {
			using TypeID = std::decay_t<decltype(type_id)>;

			if constexpr(std::is_same<TypeID, TermInfo::InitializerType>()){
				return "{INITIALIZER}";

			}else if constexpr(std::is_same<TypeID, TermInfo::FluidType>()){
				if(term_info.getExpr().kind() == sema::Expr::Kind::IntValue){
					return "{FLUID INTEGRAL}";
				}else{
					evo::debugAssert(
						term_info.getExpr().kind() == sema::Expr::Kind::FloatValue, "Unsupported fluid type"
					);
					return "{FLUID FLOAT}";
				}
				
			}else if constexpr(std::is_same<TypeID, TypeInfo::ID>()){
				return this->context.getTypeManager().printType(type_id, this->context.getSourceManager());

			}else if constexpr(std::is_same<TypeID, TypeInfo::VoidableID>()){
				return this->context.getTypeManager().printType(type_id, this->context.getSourceManager());

			}else if constexpr(std::is_same<TypeID, evo::SmallVector<TypeInfo::ID>>()){
				return "{MULTIPLE-RETURN}";

			}else if constexpr(std::is_same<TypeID, Source::ID>()){
				return "{MODULE}";

			}else if constexpr(std::is_same<TypeID, sema::TemplatedStruct::ID>()){
				// TODO: actual name
				return "{TemplatedStruct}";

			}else{
				static_assert(false, "Unsupported type id kind");
			}
		});
	}


}