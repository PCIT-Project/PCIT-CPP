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
				return this->is_set_true;
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

			EVO_NODISCARD auto implicitly_set(Token::ID location, bool cond) -> void {
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

				this->is_set_true = cond;
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

		while(this->symbol_proc.being_worked_on.exchange(true)){
			std::this_thread::yield();
		}
		EVO_DEFER([&](){ this->symbol_proc.being_worked_on = false; });

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

				case Result::RecoverableError: {
					this->symbol_proc.errored = true;
					if(this->symbol_proc.extra_info.is<SymbolProc::StructInfo>()){
						SymbolProc::StructInfo& struct_info = this->symbol_proc.extra_info.as<SymbolProc::StructInfo>();
						if(struct_info.instantiation != nullptr){ struct_info.instantiation->errored = true; }
					}
					this->symbol_proc.nextInstruction();
				} break;

				case Result::NeedToWait: {
					return;
				} break;

				case Result::NeedToWaitBeforeNextInstr: {
					this->symbol_proc.nextInstruction();
					return;
				} break;
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

			}else if constexpr(std::is_same<InstrType, Instruction::StructDecl<false>>()){
				return this->instr_struct_decl<false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::StructDecl<true>>()){
				return this->instr_struct_decl<true>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::StructDef>()){
				return this->instr_struct_def();

			}else if constexpr(std::is_same<InstrType, Instruction::TemplateStruct>()){
				return this->instr_template_struct(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncDecl<false>>()){
				return this->instr_func_decl<false>(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncDef>()){
				return this->instr_func_def(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TemplateFunc>()){
				return this->instr_template_func(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Return>()){
				return this->instr_return(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Error>()){
				return this->instr_error(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TypeToTerm>()){
				return this->instr_type_to_term(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::FuncCall>()){
				return this->instr_func_call(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Import>()){
				return this->instr_import(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TemplatedTerm>()){
				return this->instr_templated_term(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TemplatedTermWait>()){
				return this->instr_templated_term_wait(instr);

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

		const sema::GlobalVar::ID new_sema_var = this->context.sema_buffer.createGlobalVar(
			instr.var_decl.kind,
			instr.var_decl.ident,
			this->source.getID(),
			std::optional<sema::Expr>(),
			got_type_info_id.asTypeID(),
			var_attrs.value().is_pub
		);

		if(this->add_ident_to_scope(var_ident, instr.var_decl, new_sema_var) == false){ return Result::Error; }

		this->symbol_proc.extra_info.emplace<SymbolProc::GlobalVarInfo>(new_sema_var);

		this->propagate_finished_decl();
		return Result::Success;
	}


	auto SemanticAnalyzer::instr_var_def(const Instruction::VarDef& instr) -> Result {
		sema::GlobalVar& sema_var = this->context.sema_buffer.global_vars[
			this->symbol_proc.extra_info.as<SymbolProc::GlobalVarInfo>().sema_var_id
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

		if(
			instr.var_decl.kind != AST::VarDecl::Kind::Def &&
			value_term_info.value_category == TermInfo::ValueCategory::EphemeralFluid
		){
			this->emit_error(
				Diagnostic::Code::SemaCannotInferType,
				*instr.var_decl.value,
				"Cannot infer the type of a fluid literal",
				Diagnostic::Info("Did you mean this variable to be `def`? If not, give the variable an explicit type")
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

		const sema::GlobalVar::ID new_sema_var = this->context.sema_buffer.createGlobalVar(
			instr.var_decl.kind,
			instr.var_decl.ident,
			this->source.getID(),
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

				}else if constexpr(std::is_same<ExtraInfo, SymbolProc::GlobalVarInfo>()){
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
	auto SemanticAnalyzer::instr_struct_decl(const Instruction::StructDecl<IS_INSTANTIATION>& instr) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_struct_decl: {}", this->symbol_proc.ident); });

		const evo::Result<StructAttrs> struct_attrs =
			this->analyze_struct_attrs(instr.struct_decl, instr.attribute_params_info);
		if(struct_attrs.isError()){ return Result::Error; }


		///////////////////////////////////
		// create

		SymbolProc::StructInfo& struct_info = this->symbol_proc.extra_info.as<SymbolProc::StructInfo>();


		const BaseType::ID created_struct = this->context.type_manager.getOrCreateStruct(
			BaseType::Struct(
				this->source.getID(),
				instr.struct_decl.ident,
				instr.instantiation_id,
				struct_info.member_symbols,
				nullptr,
				struct_attrs.value().is_pub
			)
		);

		struct_info.struct_id = created_struct.structID();

		if constexpr(IS_INSTANTIATION == false){
			const std::string_view ident_str = this->source.getTokenBuffer()[instr.struct_decl.ident].getString();
			if(this->add_ident_to_scope(ident_str, instr.struct_decl, created_struct.structID()) == false){
				return Result::Error;
			}
		}


		///////////////////////////////////
		// setup member statements

		this->push_scope_level(nullptr, created_struct.structID());

		BaseType::Struct& created_struct_ref = this->context.type_manager.structs[created_struct.structID()];
		created_struct_ref.scopeLevel = &this->get_current_scope_level();


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



	auto SemanticAnalyzer::instr_template_struct(const Instruction::TemplateStruct& instr) -> Result {
		EVO_DEFER([&](){
			this->context.trace("SemanticAnalyzer::instr_template_struct: {}", this->symbol_proc.ident);
		});


		size_t minimum_num_template_args = 0;
		auto params = evo::SmallVector<BaseType::StructTemplate::Param>();

		for(const SymbolProc::Instruction::TemplateParamInfo& template_param_info : instr.template_param_infos){
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


		const BaseType::ID created_struct_type_id = this->context.type_manager.getOrCreateStructTemplate(
			BaseType::StructTemplate(
				this->source.getID(), instr.struct_decl.ident, std::move(params), minimum_num_template_args
			)
		);
		
		const sema::TemplatedStruct::ID new_templated_struct = this->context.sema_buffer.createTemplatedStruct(
			created_struct_type_id.structTemplateID(), this->symbol_proc
		);

		const std::string_view ident_str = this->source.getTokenBuffer()[instr.struct_decl.ident].getString();
		if(this->add_ident_to_scope(ident_str, instr.struct_decl, new_templated_struct) == false){
			return Result::Error;
		}

		this->propagate_finished_decl_def();

		return Result::Success;
	};



	template<bool IS_INSTANTIATION>
	auto SemanticAnalyzer::instr_func_decl(const Instruction::FuncDecl<IS_INSTANTIATION>& instr) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_func_decl: {}", this->symbol_proc.ident); });

		const evo::Result<FuncAttrs> func_attrs =
			this->analyze_func_attrs(instr.func_decl, instr.attribute_params_info);
		if(func_attrs.isError()){ return Result::Error; }


		///////////////////////////////////
		// create func type

		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		auto params = evo::SmallVector<BaseType::Function::Param>();
		auto sema_params = evo::SmallVector<sema::Func::Param>();
		uint32_t min_num_args = 0;
		bool has_in_param = false;

		for(size_t i = 0; const std::optional<SymbolProc::TypeID>& symbol_proc_param_type_id : instr.params()){
			EVO_DEFER([&](){ i += 1; });

			const AST::FuncDecl::Param& param = instr.func_decl.params[i];
			
			evo::debugAssert(
				symbol_proc_param_type_id.has_value() == (param.name.kind() != AST::Kind::This),
				"`this` is the only must not have a type, and everything else must have a type"
			);


			if(symbol_proc_param_type_id.has_value()){ // regular param
				const TypeInfo::VoidableID param_type_id = this->get_type(*symbol_proc_param_type_id);

				if(param_type_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SemaParamTypeVoid, *param.type, "Function parameter cannot be type `Void`"
					);
					return Result::Error;
				}

				const bool should_copy = [&](){
					if(param.kind != AST::FuncDecl::Param::Kind::Read){ return false; }
					return this->context.getTypeManager().isTriviallyCopyable(param_type_id.asTypeID());
				}();

				if(param.kind == AST::FuncDecl::Param::Kind::In){
					has_in_param = true;
				}

				params.emplace_back(param_type_id.asTypeID(), param.kind, should_copy);

				if(instr.default_param_values[i].has_value()){
					TermInfo default_param_value = this->get_term_info(*instr.default_param_values[i]);

					if(
						this->type_check<true>(
							param_type_id.asTypeID(),
							default_param_value,
							"Default value of function parameter",
							*instr.func_decl.params[i].defaultValue
						).ok == false
					){
						return Result::Error;
					}

					sema_params.emplace_back(ast_buffer.getIdent(param.name), default_param_value.getExpr());


				}else{
					sema_params.emplace_back(ast_buffer.getIdent(param.name), std::nullopt);
					min_num_args += 1;
				}

			}else{ // `this` param
				const std::optional<sema::ScopeManager::Scope::ObjectScope> current_type_scope = 
					this->scope.getCurrentTypeScopeIfExists();

				if(current_type_scope.has_value() == false){
					// TODO: better messaging
					this->emit_error(
						Diagnostic::Code::SemaInvalidScopeForThis,
						param.name,
						"`this` parameters are only valid inside type scope"
					);
					return Result::Error;
				}

				current_type_scope->visit([&](const auto& type_scope) -> void {
					using TypeScope = std::decay_t<decltype(type_scope)>;

					if constexpr(std::is_same<TypeScope, BaseType::Struct::ID>()){
						const TypeInfo::ID this_type = this->context.type_manager.getOrCreateTypeInfo(
							TypeInfo(BaseType::ID(type_scope))
						);
						params.emplace_back(this_type, param.kind);
					}else{
						evo::debugFatalBreak("Invalid object scope");
					}
				});

				sema_params.emplace_back(ast_buffer.getThis(param.name), std::nullopt);
				min_num_args += 1;
			}
		}


		auto return_params = evo::SmallVector<BaseType::Function::ReturnParam>();
		for(size_t i = 0; const SymbolProc::TypeID& symbol_proc_return_param_type_id : instr.returns()){
			EVO_DEFER([&](){ i += 1; });

			const TypeInfo::VoidableID type_id = this->get_type(symbol_proc_return_param_type_id);

			const AST::FuncDecl::Return& ast_return_param = instr.func_decl.returns[i];

			if(i == 0){
				if(type_id.isVoid() && ast_return_param.ident.has_value()){
					this->emit_error(
						Diagnostic::Code::SemaNamedVoidReturn,
						*ast_return_param.ident,
						"A function return parameter that is type `Void` cannot be named"
					);
					return Result::Error;
				}
			}else{
				if(type_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SemaNotFirstReturnVoid,
						ast_return_param.type,
						"Only the first function return parameter can be type `Void`"
					);
					return Result::Error;
				}
			}

			return_params.emplace_back(ast_return_param.ident, type_id);
		}


		auto error_return_params = evo::SmallVector<BaseType::Function::ReturnParam>();
		for(size_t i = 0; const SymbolProc::TypeID& symbol_proc_error_return_param_type_id : instr.errorReturns()){
			EVO_DEFER([&](){ i += 1; });

			const TypeInfo::VoidableID type_id = this->get_type(symbol_proc_error_return_param_type_id);

			const AST::FuncDecl::Return& ast_error_return_param = instr.func_decl.errorReturns[i];

			if(i == 0){
				if(type_id.isVoid() && ast_error_return_param.ident.has_value()){
					this->emit_error(
						Diagnostic::Code::SemaNamedVoidReturn,
						*ast_error_return_param.ident,
						"A function error return parameter that is type `Void` cannot be named"
					);
					return Result::Error;
				}
			}else{
				if(type_id.isVoid()){
					this->emit_error(
						Diagnostic::Code::SemaNotFirstReturnVoid,
						ast_error_return_param.type,
						"Only the first function error return parameter can be type `Void`"
					);
					return Result::Error;
				}
			}

			error_return_params.emplace_back(ast_error_return_param.ident, type_id);
		}


		const BaseType::ID created_func_base_type = this->context.type_manager.getOrCreateFunction(
			BaseType::Function(std::move(params), std::move(return_params), std::move(error_return_params))
		);


		///////////////////////////////////
		// create func

		const sema::Func::ID created_func = this->context.sema_buffer.createFunc(
			instr.func_decl.name,
			this->source.getID(),
			created_func_base_type.funcID(),
			std::move(sema_params),
			min_num_args,
			func_attrs.value().is_pub,
			!func_attrs.value().is_runtime,
			has_in_param,
			instr.instantiation_id
		);


		if constexpr(IS_INSTANTIATION == false){
			// TODO: manage overloads
			const Token::ID ident = this->source.getASTBuffer().getIdent(instr.func_decl.name);
			const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();
			if(this->add_ident_to_scope(ident_str, instr.func_decl, created_func, this->context) == false){
				return Result::Error;
			}
		}

		this->push_scope_level(&this->context.sema_buffer.funcs[created_func].stmtBlock, created_func);


		///////////////////////////////////
		// setup member statements


		this->propagate_finished_decl();

		return Result::Success;
	}


	auto SemanticAnalyzer::instr_func_def(const Instruction::FuncDef& instr) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_func_def: {}", this->symbol_proc.ident); });

		if(this->get_current_scope_level().isTerminated()){
			this->get_current_func().isTerminated = true;

		}else{
			const sema::Func& current_func = this->get_current_func();
			const BaseType::Function& func_type = this->context.getTypeManager().getFunction(current_func.typeID);

			if(func_type.returnsVoid() == false){
				this->emit_error(
					Diagnostic::Code::SemaFuncIsntTerminated,
					instr.func_decl,
					"Function isn't terminated",
					Diagnostic::Info(
						"A function is terminated when all control paths end in a [return], [error], [unreachable], "
						"or a function call that has the attribute `#noReturn`"
					)
				);
				return Result::Error;
			}
		}


		this->propagate_finished_def();
		this->get_current_func().defCompleted = true;

		this->pop_scope_level(); // TODO: needed?

		return Result::Success;
	}


	// TODO: condense this with template struct somehow?
	auto SemanticAnalyzer::instr_template_func(const Instruction::TemplateFunc& instr) -> Result {
		EVO_DEFER([&](){
			this->context.trace("SemanticAnalyzer::instr_template_func: {}", this->symbol_proc.ident);
		});


		size_t minimum_num_template_args = 0;
		auto params = evo::SmallVector<sema::TemplatedFunc::Param>();

		for(const SymbolProc::Instruction::TemplateParamInfo& template_param_info : instr.template_param_infos){
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

		evo::debugAssert(instr.func_decl.name.kind() == AST::Kind::Ident, "templated overloads are not allowed");
		const Token::ID ident = this->source.getASTBuffer().getIdent(instr.func_decl.name);
		const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();
		
		const sema::TemplatedFunc::ID new_templated_func = this->context.sema_buffer.createTemplatedFunc(
			this->symbol_proc, minimum_num_template_args, std::move(params)
		);

		if(this->add_ident_to_scope(ident_str, instr.func_decl, new_templated_func) == false){
			return Result::Error;
		}

		this->propagate_finished_decl_def();

		return Result::Success;
	}



	auto SemanticAnalyzer::instr_return(const Instruction::Return& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.return_stmt) == false){ return Result::Error; }

		if(instr.return_stmt.label.has_value()){
			this->emit_error(
				Diagnostic::Code::MiscUnimplementedFeature,
				instr.return_stmt.label.value(),
				"return statements with labels are currently unsupported"
			);
			return Result::Error;
		}

		const sema::Func& current_func = this->get_current_func();
		const BaseType::Function& current_func_type = this->context.getTypeManager().getFunction(current_func.typeID);


		auto return_value = std::optional<sema::Expr>();
		if(instr.return_stmt.value.is<std::monostate>()){ // return;
			if(current_func_type.returnsVoid() == false){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectReturnStmtKind,
					instr.return_stmt,
					"Functions that have a return type other than `Void` must return a value"
				);
				return Result::Error;
			}

			if(current_func_type.hasNamedReturns()){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectReturnStmtKind,
					instr.return_stmt,
					"Incorrect return statement kind for a function named return parameters",
					Diagnostic::Info("Set all return values and use \"return...;\" instead")
				);
				return Result::Error;
			}
			
		}else if(instr.return_stmt.value.is<AST::Node>()){ // return {EXPRESSION};
			evo::debugAssert(instr.value.has_value(), "Return value needs to have value analyzed");

			if(current_func_type.returnsVoid()){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectReturnStmtKind,
					instr.return_stmt,
					"Functions that have a return type of `Void` cannot return a value"
				);
				return Result::Error;
			}

			if(current_func_type.hasNamedReturns()){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectReturnStmtKind,
					instr.return_stmt,
					"Incorrect return statement kind for a function named return parameters",
					Diagnostic::Info("Set all return values and use \"return...;\" instead")
				);
				return Result::Error;
			}


			TermInfo& return_value_term = this->get_term_info(*instr.value);

			if(return_value_term.is_ephemeral() == false){
				this->emit_error(
					Diagnostic::Code::SemaReturnNotEphemeral,
					instr.return_stmt.value.as<AST::Node>(),
					"Value of return statement is not ephemeral"
				);
				return Result::Error;
			}

			if(this->type_check<true>(
				current_func_type.returnParams.front().typeID.asTypeID(),
				return_value_term,
				"Return",
				instr.return_stmt.value.as<AST::Node>()
			).ok == false){
				return Result::Error;
			}

			return_value = return_value_term.getExpr();
			
		}else{ // return...;
			evo::debugAssert(instr.return_stmt.value.is<Token::ID>(), "Unknown return kind");

			if(current_func_type.returnsVoid()){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectReturnStmtKind,
					instr.return_stmt,
					"Functions that have a return type of `Void` cannot return a value"
				);
				return Result::Error;
			}

			if(current_func_type.hasNamedReturns() == false){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectReturnStmtKind,
					instr.return_stmt,
					"Incorrect return statement kind for single unnamed return parameters",
					Diagnostic::Info("Use \"return {EXPRESSION};\" instead")
				);
				return Result::Error;
			}
		}

		const sema::Return::ID sema_return_id = this->context.sema_buffer.createReturn(return_value);

		this->get_current_scope_level().stmtBlock().emplace_back(sema_return_id);
		this->get_current_scope_level().setTerminated();

		return Result::Success;
	}




	auto SemanticAnalyzer::instr_error(const Instruction::Error& instr) -> Result {
		if(this->check_scope_isnt_terminated(instr.error_stmt) == false){ return Result::Error; }

		const sema::Func& current_func = this->get_current_func();
		const BaseType::Function& current_func_type = this->context.getTypeManager().getFunction(current_func.typeID);


		if(current_func_type.hasErrorReturn() == false){
			this->emit_error(
				Diagnostic::Code::SemaErrorInFuncWithoutErrors,
				instr.error_stmt,
				"Cannot error return in a function that does not have error returns"
			);
			return Result::Error;
		}


		auto error_value = std::optional<sema::Expr>();
		if(instr.error_stmt.value.is<std::monostate>()){ // error;
			if(current_func_type.hasErrorReturnParams()){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectReturnStmtKind,
					instr.error_stmt,
					"Incorrect error return statement kind for a function named error return parameters",
					Diagnostic::Info("Set all error return values and use \"error...;\" instead")
				);
				return Result::Error;
			}
			
		}else if(instr.error_stmt.value.is<AST::Node>()){ // error {EXPRESSION};
			evo::debugAssert(instr.value.has_value(), "error return value needs to have value analyzed");

			if(current_func_type.hasNamedErrorReturns()){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectReturnStmtKind,
					instr.error_stmt,
					"Incorrect error return statement kind for a function named error return parameters",
					Diagnostic::Info("Set all error return values and use \"error...;\" instead")
				);
				return Result::Error;
			}


			TermInfo& error_value_term = this->get_term_info(*instr.value);

			if(error_value_term.is_ephemeral() == false){
				this->emit_error(
					Diagnostic::Code::SemaReturnNotEphemeral,
					instr.error_stmt.value.as<AST::Node>(),
					"Value of error return statement is not ephemeral"
				);
				return Result::Error;
			}

			if(this->type_check<true>(
				current_func_type.errorParams.front().typeID.asTypeID(),
				error_value_term,
				"Error return",
				instr.error_stmt.value.as<AST::Node>()
			).ok == false){
				return Result::Error;
			}

			error_value = error_value_term.getExpr();
			
		}else{ // error...;
			evo::debugAssert(instr.error_stmt.value.is<Token::ID>(), "Unknown return kind");

			if(current_func_type.hasNamedErrorReturns() == false){
				this->emit_error(
					Diagnostic::Code::SemaIncorrectReturnStmtKind,
					instr.error_stmt,
					"Incorrect error return statement kind for single unnamed error return parameters",
					Diagnostic::Info("Use \"error {EXPRESSION};\" instead")
				);
				return Result::Error;
			}
		}

		const sema::Error::ID sema_error_id = this->context.sema_buffer.createError(error_value);

		this->get_current_scope_level().stmtBlock().emplace_back(sema_error_id);
		this->get_current_scope_level().setTerminated();

		return Result::Success;
	}





	auto SemanticAnalyzer::instr_type_to_term(const Instruction::TypeToTerm& instr) -> Result {
		this->return_term_info(instr.to,
			TermInfo::ValueCategory::Type, TermInfo::ValueStage::Constexpr, this->get_type(instr.from), std::nullopt
		);
		return Result::Success;
	}



	auto SemanticAnalyzer::instr_func_call(const Instruction::FuncCall& instr) -> Result {
		const TermInfo target_term_info = this->get_term_info(instr.target);

		if(target_term_info.value_category != TermInfo::ValueCategory::Function){
			this->emit_error(
				Diagnostic::Code::SemaCannotCallLikeFunction,
				instr.func_call.target,
				"Cannot call expression like a function"
			);
			return Result::Error;
		}


		auto func_infos = evo::SmallVector<SelectFuncOverloadFuncInfo>();
		using FuncOverload = evo::Variant<sema::Func::ID, sema::TemplatedFuncID>;
		for(const FuncOverload& func_overload : target_term_info.type_id.as<TermInfo::FuncOverloadList>()){
			if(func_overload.is<sema::Func::ID>()){
				const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(func_overload.as<sema::Func::ID>());
				const BaseType::Function& func_type = this->context.getTypeManager().getFunction(sema_func.typeID);
				func_infos.emplace_back(func_overload.as<sema::Func::ID>(), func_type);
			}
		}

		auto arg_infos = evo::SmallVector<SelectFuncOverloadArgInfo>();
		for(size_t i = 0; const SymbolProc::TermInfoID& arg : instr.args){
			arg_infos.emplace_back(this->get_term_info(arg), instr.func_call.args[i]);
			i += 1;
		}


		const evo::Result<size_t> selected_func_overload_index = this->select_func_overload(
			func_infos, arg_infos, instr.func_call.target
		);
		if(selected_func_overload_index.isError()){ return Result::Error; }


		const sema::Func::ID selected_func_id = func_infos[selected_func_overload_index.value()].func_id;
		const sema::Func& selected_func = this->context.sema_buffer.getFunc(selected_func_id);
		const BaseType::Function& selected_func_type = this->context.getTypeManager().getFunction(selected_func.typeID);



		auto sema_args = evo::SmallVector<sema::Expr>();
		for(const SymbolProc::TermInfoID& arg : instr.args){
			sema_args.emplace_back(this->get_term_info(arg).getExpr());
		}

		for(size_t i = sema_args.size(); i < selected_func.params.size(); i+=1){
			sema_args.emplace_back(*selected_func.params[i].defaultValue);
		}

		const sema::FuncCall::ID sema_func_call_id = this->context.sema_buffer.createFuncCall(
			selected_func_id, std::move(sema_args)
		);


		const TermInfo::ValueStage value_stage = selected_func.isConstexpr
			? TermInfo::ValueStage::Constexpr 
			: TermInfo::ValueStage::Runtime;

		if(selected_func_type.returnParams.size() == 1){ // single return
			this->return_term_info(instr.output,
				TermInfo(
					TermInfo::ValueCategory::Ephemeral,
					value_stage,
					selected_func_type.returnParams[0].typeID.asTypeID(),
					sema::Expr(sema_func_call_id)
				)
			);
			
		}else{ // multi-return
			auto return_types = evo::SmallVector<TypeInfo::ID>();
			return_types.reserve(selected_func_type.returnParams.size());
			for(const BaseType::Function::ReturnParam& return_param : selected_func_type.returnParams){
				return_types.emplace_back(return_param.typeID.asTypeID());
			}

			this->return_term_info(instr.output,
				TermInfo(
					TermInfo::ValueCategory::Ephemeral,
					value_stage,
					std::move(return_types),
					sema::Expr(sema_func_call_id)
				)
			);
		}


		return Result::Success;
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
					TermInfo::ValueCategory::Module,
					TermInfo::ValueStage::Constexpr,
					import_lookup.value(),
					std::nullopt
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

			const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
				&source_module.global_symbol_procs,
				instr.infix.rhs,
				rhs_ident_str,
				std::format("Module has no symbol named \"{}\"", rhs_ident_str)
			);

			if(wait_on_symbol_proc_result == WaitOnSymbolProcResult::Error){ return Result::Error; }
			if(wait_on_symbol_proc_result == WaitOnSymbolProcResult::NeedToWait){ return Result::NeedToWait; }

			evo::debugAssert(
				wait_on_symbol_proc_result == WaitOnSymbolProcResult::SemasReady, "Unknown WaitOnSymbolProcResult"
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
				case AnalyzeExprIdentInScopeLevelError::DoesntExist:
					evo::debugFatalBreak("Def is done, but can't find sema of symbol");

				case AnalyzeExprIdentInScopeLevelError::NeedsToWaitOnDef:
					evo::debugFatalBreak(
						"Sema doesn't have completed info for def despite SymbolProc saying it should"
					);

				case AnalyzeExprIdentInScopeLevelError::ErrorEmitted: return Result::Error;
			}

			evo::unreachable();


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

			const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
				&lhs_struct.memberSymbols,
				instr.infix.rhs,
				rhs_ident_str,
				std::format("Struct has no member named \"{}\"", rhs_ident_str)
			);

			if(wait_on_symbol_proc_result == WaitOnSymbolProcResult::Error){ return Result::Error; }
			if(wait_on_symbol_proc_result == WaitOnSymbolProcResult::NeedToWait){ return Result::NeedToWait; }

			evo::debugAssert(
				wait_on_symbol_proc_result == WaitOnSymbolProcResult::SemasReady, "Unknown WaitOnSymbolProcResult"
			);


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
		const TypeInfo::ID base_type_id = [&](){
			const TermInfo& term_info = this->get_term_info(instr.base_type);

			switch(term_info.value_category){
				case TermInfo::ValueCategory::Type: {
					evo::debugAssert(
						this->get_term_info(instr.base_type).type_id.as<TypeInfo::VoidableID>().isVoid() == false,
						"`Void` is not a user-type"
					);
					return term_info.type_id.as<TypeInfo::VoidableID>().asTypeID();
				} break;

				case TermInfo::ValueCategory::TemplateType: {
					const sema::TemplatedStruct& sema_templated_struct = this->context.sema_buffer.getTemplatedStruct(
						term_info.type_id.as<sema::TemplatedStruct::ID>()
					);

					return this->context.type_manager.getOrCreateTypeInfo(
						TypeInfo(BaseType::ID(sema_templated_struct.templateID))
					);
				} break;

				default: evo::debugFatalBreak("Invalid user type base");
			}
		}();

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

		const sema::TemplatedStruct& sema_templated_struct = this->context.sema_buffer.templated_structs[
			templated_type_term_info.type_id.as<sema::TemplatedStruct::ID>()
		];

		BaseType::StructTemplate& struct_template = 
			this->context.type_manager.struct_templates[sema_templated_struct.templateID];


		///////////////////////////////////
		// check args

		if(instr.arguments.size() < struct_template.minNumTemplateArgs){
			auto infos = evo::SmallVector<Diagnostic::Info>();

			if(struct_template.hasAnyDefaultParams()){
				infos.emplace_back(
					std::format(
						"This type requires at least {}, got {}",
						struct_template.minNumTemplateArgs, instr.arguments.size()
					)
				);
			}else{
				infos.emplace_back(
					std::format(
						"This type requires {}, got {}", struct_template.minNumTemplateArgs, instr.arguments.size()
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


		if(instr.arguments.size() > struct_template.params.size()){
			auto infos = evo::SmallVector<Diagnostic::Info>();

			if(struct_template.hasAnyDefaultParams()){
				infos.emplace_back(
					std::format(
						"This type requires at most {}, got {}",
						struct_template.params.size(), instr.arguments.size()
					)
				);
			}else{
				infos.emplace_back(
					std::format(
						"This type requires {}, got {}", struct_template.params.size(), instr.arguments.size()
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

		auto instantiation_lookup_args = evo::SmallVector<BaseType::StructTemplate::Arg>();
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
					if(struct_template.params[i].typeID.has_value()){
						const ASTBuffer& ast_buffer = this->source.getASTBuffer();
						const AST::StructDecl& ast_struct =
							ast_buffer.getStructDecl(sema_templated_struct.symbolProc.ast_node);
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

				if(struct_template.params[i].typeID.has_value() == false){
					const ASTBuffer& ast_buffer = this->source.getASTBuffer();
					const AST::StructDecl& ast_struct =
						ast_buffer.getStructDecl(sema_templated_struct.symbolProc.ast_node);
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
					*struct_template.params[i].typeID, arg_term_info, "Template argument", instr.templated_expr.args[i]
				);
				if(type_check_info.ok == false){
					return Result::Error;
				}

				const sema::Expr& arg_expr = arg_term_info.getExpr();
				instantiation_args.emplace_back(arg_expr);
				switch(arg_expr.kind()){
					case sema::Expr::Kind::IntValue: {
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
				if(struct_template.params[i].typeID.has_value()){
					const ASTBuffer& ast_buffer = this->source.getASTBuffer();
					const AST::StructDecl& ast_struct =
						ast_buffer.getStructDecl(sema_templated_struct.symbolProc.ast_node);
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


		for(size_t i = instr.arguments.size(); i < struct_template.params.size(); i+=1){
			struct_template.params[i].defaultValue.visit([&](const auto& default_value) -> void {
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


		const BaseType::StructTemplate::InstantiationInfo instantiation_info =
			struct_template.lookupInstantiation(std::move(instantiation_lookup_args));

		if(instantiation_info.needsToBeCompiled()){
			auto symbol_proc_builder = SymbolProcBuilder(
				this->context, this->context.source_manager[sema_templated_struct.symbolProc.source_id]
			);

			sema::ScopeManager& scope_manager = this->context.sema_buffer.scope_manager;

			const sema::ScopeManager::Scope::ID instantiation_sema_scope_id = 
				scope_manager.copyScope(*sema_templated_struct.symbolProc.sema_scope_id);


			///////////////////////////////////
			// build instantiation

			const evo::Result<SymbolProc::ID> instantiation_symbol_proc_id = symbol_proc_builder.buildTemplateInstance(
				sema_templated_struct.symbolProc,
				instantiation_info.instantiation,
				instantiation_sema_scope_id,
				*instantiation_info.instantiationID
			);
			if(instantiation_symbol_proc_id.isError()){ return Result::Error; }

			instantiation_info.instantiation.symbolProcID = instantiation_symbol_proc_id.value();


			///////////////////////////////////
			// add instantiation args to scope

			sema::ScopeManager::Scope& instantiation_sema_scope = scope_manager.getScope(instantiation_sema_scope_id);

			instantiation_sema_scope.pushLevel(scope_manager.createLevel());

			const AST::StructDecl& struct_template_decl = 
				this->source.getASTBuffer().getStructDecl(sema_templated_struct.symbolProc.ast_node);

			const AST::TemplatePack& ast_template_pack = this->source.getASTBuffer().getTemplatePack(
				*struct_template_decl.templatePack
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
							*struct_template.params[i].typeID,
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
		const BaseType::StructTemplate::Instantiation& instantiation =
			this->get_struct_instantiation(instr.instantiation);

		if(instantiation.errored.load()){ return Result::Error; }
		// if(instantiation.structID.load().has_value() == false){ return Result::NeedToWaitOnInstantiation; }
		evo::debugAssert(instantiation.structID.has_value(), "Should already be completed");

		this->return_term_info(instr.output,
			TermInfo::ValueCategory::Type,
			TermInfo::ValueStage::Constexpr,
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
					TermInfo::ValueStage::Constexpr,
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
					TermInfo::ValueStage::Constexpr,
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
					TermInfo::ValueStage::Constexpr,
					this->context.getTypeManager().getTypeBool(),
					sema::Expr(this->context.sema_buffer.createBoolValue(literal_token.getBool()))
				);
				return Result::Success;
			} break;

			case Token::Kind::LiteralString: {
				this->return_term_info(instr.output,
					TermInfo::ValueCategory::Ephemeral,
					TermInfo::ValueStage::Constexpr,
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
					TermInfo::ValueStage::Constexpr,
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
			TermInfo::ValueStage::Constexpr,
			TermInfo::InitializerType(),
			sema::Expr(this->context.sema_buffer.createUninit(instr.uninit_token))
		);
		return Result::Success;
	}

	auto SemanticAnalyzer::instr_zeroinit(const Instruction::Zeroinit& instr) -> Result {
		this->return_term_info(instr.output,
			TermInfo::ValueCategory::Initializer,
			TermInfo::ValueStage::Constexpr,
			TermInfo::InitializerType(),
			sema::Expr(this->context.sema_buffer.createZeroinit(instr.zeroinit_token))
		);
		return Result::Success;
	}



	//////////////////////////////////////////////////////////////////////
	// scope

	auto SemanticAnalyzer::get_current_scope_level() const -> sema::ScopeLevel& {
		return this->context.sema_buffer.scope_manager.getLevel(this->scope.getCurrentLevel());
	}


	auto SemanticAnalyzer::push_scope_level(sema::StmtBlock* stmt_block) -> void {
		if(this->scope.inObjectScope()){
			this->get_current_scope_level().addSubScope();
		}
		this->scope.pushLevel(this->context.sema_buffer.scope_manager.createLevel(stmt_block));
	}

	auto SemanticAnalyzer::push_scope_level(sema::StmtBlock* stmt_block, const auto& object_scope_id) -> void {
		// if(this->scope.inObjectScope()){
			this->get_current_scope_level().addSubScope();
		// }
		this->scope.pushLevel(this->context.sema_buffer.scope_manager.createLevel(stmt_block), object_scope_id);
	}

	auto SemanticAnalyzer::pop_scope_level() -> void {
		sema::ScopeLevel& current_scope_level = this->get_current_scope_level();
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


	auto SemanticAnalyzer::get_current_func() -> sema::Func& {
		return this->context.sema_buffer.funcs[this->scope.getCurrentObjectScope().as<sema::Func::ID>()];
	}




	//////////////////////////////////////////////////////////////////////
	// misc

	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::lookup_ident_impl(Token::ID ident) -> evo::Expected<TermInfo, Result> {
		const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();

		///////////////////////////////////
		// find symbol procs

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


		const WaitOnSymbolProcResult wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
			symbol_proc_namespaces,
			ident,
			ident_str,
			std::format("Identifier \"{}\" was not defined in this scope", ident_str)
		);

		if(wait_on_symbol_proc_result == WaitOnSymbolProcResult::Error){ return evo::Unexpected(Result::Error); }
		if(wait_on_symbol_proc_result == WaitOnSymbolProcResult::NeedToWait){
			return evo::Unexpected(Result::NeedToWait);
		}

		evo::debugAssert(
			wait_on_symbol_proc_result == WaitOnSymbolProcResult::SemasReady, "Unknown WaitOnSymbolProcResult"
		);


		///////////////////////////////////
		// find sema

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

		evo::debugFatalBreak("Failed to find semas for ident");
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
				return ReturnType(
					TermInfo(TermInfo::ValueCategory::Function, TermInfo::ValueStage::Constexpr, ident_id, std::nullopt)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::GlobalVar::ID>()){
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

				const sema::GlobalVar& sema_var = this->context.getSemaBuffer().getGlobalVar(ident_id);

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
							is_global_scope ? ValueStage::Runtime : ValueStage::Comptime,
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
							ValueCategory::ConcreteConst, ValueStage::Comptime, *sema_var.typeID, sema::Expr(ident_id)
						));
					} break;

					case AST::VarDecl::Kind::Def: {
						if(sema_var.typeID.has_value()){
							return ReturnType(TermInfo(
								ValueCategory::Ephemeral, ValueStage::Constexpr, *sema_var.typeID, *sema_var.expr.load()
							));
						}else{
							return ReturnType(TermInfo(
								ValueCategory::EphemeralFluid,
								ValueStage::Constexpr,
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
						TermInfo::ValueStage::Constexpr,
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
						TermInfo::ValueStage::Constexpr,
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
						TermInfo::ValueStage::Constexpr,
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
						TermInfo::ValueStage::Constexpr,
						ident_id,
						std::nullopt
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::TemplateTypeParam>()){
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::Type,
						TermInfo::ValueStage::Constexpr,
						ident_id.typeID,
						std::nullopt
					)
				);

			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::TemplateExprParam>()){
				return ReturnType(
					TermInfo(
						TermInfo::ValueCategory::Ephemeral,
						TermInfo::ValueStage::Constexpr,
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
		std::string&& error_msg_if_ident_doesnt_exist
	) -> WaitOnSymbolProcResult {
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
			return WaitOnSymbolProcResult::Error;
		}


		bool any_waiting = false;
		bool any_ready = false;
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
				break; case SymbolProc::WaitOnResult::NotNeeded:             any_ready = true;
				break; case SymbolProc::WaitOnResult::Waiting:               any_waiting = true;
				break; case SymbolProc::WaitOnResult::WasErrored:            return WaitOnSymbolProcResult::Error;
				break; case SymbolProc::WaitOnResult::WasPassedOnByWhenCond: // do nothing...
				break; case SymbolProc::WaitOnResult::CircularDepDetected:   return WaitOnSymbolProcResult::Error;
			}
		}

		if(any_waiting){ return WaitOnSymbolProcResult::NeedToWait; }
		if(any_ready){ return WaitOnSymbolProcResult::SemasReady; }

		this->emit_error(
			Diagnostic::Code::SemaNoSymbolInModuleWithThatIdent,
			ident,
			std::move(error_msg_if_ident_doesnt_exist),
			Diagnostic::Info("The identifier was declared in a when conditional block that wasn't taken")
		);
		return WaitOnSymbolProcResult::Error;
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




	auto SemanticAnalyzer::select_func_overload(
		evo::ArrayProxy<SelectFuncOverloadFuncInfo> func_infos,
		evo::SmallVector<SelectFuncOverloadArgInfo>& arg_infos,
		const auto& call_node
	) -> evo::Result<size_t> {
		evo::debugAssert(func_infos.empty() == false, "need at least 1 function");

		struct OverloadScore{
			using Success = std::monostate;
			struct TooFewArgs{ size_t min_num; size_t got_num; bool accepts_different_nums; };
			struct TooManyArgs{ size_t max_num; size_t got_num; bool accepts_different_nums; };
			struct TypeMismatch{ size_t arg_index; };
			struct ValueKindMismatch{ size_t arg_index; };
			struct IncorrectLabel{ size_t arg_index; };

			using Reason = evo::Variant<
				Success,
				TooFewArgs,
				TooManyArgs,
				TypeMismatch,
				ValueKindMismatch,
				IncorrectLabel
			>;
			
			unsigned score;
			Reason reason;

			OverloadScore(unsigned _score) : score(_score), reason(std::monostate()) {};
			OverloadScore(Reason _reason) : score(0), reason(_reason) {};
		};
		auto scores = evo::SmallVector<OverloadScore>();
		scores.reserve(func_infos.size());

		unsigned best_score = 0;
		size_t best_score_index = 0;
		bool found_matching_best_score = false;

		
		for(size_t func_i = 0; const SelectFuncOverloadFuncInfo& func_info : func_infos){
			EVO_DEFER([&](){ func_i += 1; });


			unsigned current_score = 0;

			const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(func_info.func_id);

			if(arg_infos.size() < sema_func.minNumArgs){
				scores.emplace_back(OverloadScore::TooFewArgs(
					sema_func.minNumArgs,
					arg_infos.size(),
					sema_func.minNumArgs != func_info.func_type.params.size())
				);
				continue;
			}

			if(arg_infos.size() > func_info.func_type.params.size()){
				scores.emplace_back(OverloadScore::TooManyArgs(
					func_info.func_type.params.size(),
					arg_infos.size(),
					sema_func.minNumArgs != func_info.func_type.params.size())
				);
				continue;
			}


			bool arg_checking_failed = false;
			for(size_t arg_i = 0; SelectFuncOverloadArgInfo& arg_info : arg_infos){
				EVO_DEFER([&](){ arg_i += 1; });


				///////////////////////////////////
				// check type mismatch

				const TypeCheckInfo& type_check_info = this->type_check<false>(
					func_info.func_type.params[arg_i].typeID,
					arg_info.term_info,
					"Function call argument",
					arg_info.ast_arg.value
				);

				if(type_check_info.ok == false){
					scores.emplace_back(OverloadScore::TypeMismatch(arg_i));
					arg_checking_failed = true;
					break;
				}

				if(type_check_info.requires_implicit_conversion == false){ current_score += 1; }


				///////////////////////////////////
				// value kind

				switch(func_info.func_type.params[arg_i].kind){
					case AST::FuncDecl::Param::Kind::Read: {
						// accepts any value kind
					} break;

					case AST::FuncDecl::Param::Kind::Mut: {
						if(arg_info.term_info.is_const()){
							scores.emplace_back(OverloadScore::ValueKindMismatch(arg_i));
							arg_checking_failed = true;
							break;
						}

						if(arg_info.term_info.is_concrete() == false){
							scores.emplace_back(OverloadScore::ValueKindMismatch(arg_i));
							arg_checking_failed = true;
							break;
						}

						current_score += 1; // add 1 to prefer mut over read
					} break;

					case AST::FuncDecl::Param::Kind::In: {
						if(arg_info.term_info.is_ephemeral() == false){
							scores.emplace_back(OverloadScore::ValueKindMismatch(arg_i));
							arg_checking_failed = true;
							break;
						}
					} break;
				}


				///////////////////////////////////
				// check label

				if(arg_info.ast_arg.label.has_value()){
					const std::string_view arg_label = 
						this->source.getTokenBuffer()[*arg_info.ast_arg.label].getString();

					const std::string_view param_name = this->context.getSourceManager()[sema_func.sourceID]
						.getTokenBuffer()[sema_func.params[arg_i].ident].getString();

					if(arg_label != param_name){
						scores.emplace_back(OverloadScore::IncorrectLabel(arg_i));
						arg_checking_failed = true;
						break;
					}
				}


				///////////////////////////////////
				// done checking arg

				current_score += 1;
			}
			if(arg_checking_failed){ continue; }

			current_score += 1;
			scores.emplace_back(current_score);
			if(best_score < current_score){
				best_score = current_score;
				best_score_index = func_i;
				found_matching_best_score = false;
			}else if(best_score == current_score){
				found_matching_best_score = true;
			}
		}

		if(best_score == 0){ // found no matches
			auto infos = evo::SmallVector<Diagnostic::Info>();

			for(size_t i = 0; const OverloadScore& score : scores){
				EVO_DEFER([&](){ i += 1; });
			
				score.reason.visit([&](const auto& reason) -> void {
					using ReasonT = std::decay_t<decltype(reason)>;
					
					if constexpr(std::is_same<ReasonT, OverloadScore::Success>()){
						evo::fatalBreak("Success should not have a score of 0");

					}else if constexpr(std::is_same<ReasonT, OverloadScore::TooFewArgs>()){
						if(reason.accepts_different_nums){
							infos.emplace_back(
								std::format(
									"Failed to match: too few arguments (requires at least {}, got {})",
									reason.min_num,
									reason.got_num
								),
								this->get_location(func_infos[i].func_id)
							);
							
						}else{
							infos.emplace_back(
								std::format(
									"Failed to match: too few arguments (requires {}, got {})",
									reason.min_num,
									reason.got_num
								),
								this->get_location(func_infos[i].func_id)
							);
						}

					}else if constexpr(std::is_same<ReasonT, OverloadScore::TooManyArgs>()){
						if(reason.accepts_different_nums){
							infos.emplace_back(
								std::format(
									"Failed to match: too many arguments (requires at most {}, got {})",
									reason.max_num,
									reason.got_num
								),
								this->get_location(func_infos[i].func_id)
							);
							
						}else{
							infos.emplace_back(
								std::format(
									"Failed to match: too many arguments (requires {}, got {})",
									reason.max_num,
									reason.got_num
								),
								this->get_location(func_infos[i].func_id)
							);
						}

					}else if constexpr(std::is_same<ReasonT, OverloadScore::TypeMismatch>()){
						const TypeInfo::ID expected_type_id = func_infos[i].func_type.params[reason.arg_index].typeID;
						const TermInfo& got_arg = arg_infos[reason.arg_index].term_info;

						infos.emplace_back(
							std::format("Failed to match: argument (index: {}) type mismatch", reason.arg_index),
							this->get_location(func_infos[i].func_id),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info(
									"This argument:", this->get_location(arg_infos[reason.arg_index].ast_arg.value)
								),
								Diagnostic::Info(std::format("Argument type:  {}", this->print_type(got_arg))),
								Diagnostic::Info(
									std::format(
										"Parameter type: {}",
										this->context.getTypeManager().printType(
											expected_type_id, this->context.getSourceManager()
										)
									)
								),
							}
						);

					}else if constexpr(std::is_same<ReasonT, OverloadScore::ValueKindMismatch>()){
						auto sub_infos = evo::SmallVector<Diagnostic::Info>();
						sub_infos.emplace_back(
							"This argument:", this->get_location(arg_infos[reason.arg_index].ast_arg.value)
						);

						switch(func_infos[i].func_type.params[reason.arg_index].kind){
							case AST::FuncDecl::Param::Kind::Read: {
								evo::debugFatalBreak("Read parameters should never fail to accept value kind");
							} break;

							case AST::FuncDecl::Param::Kind::Mut: {
								sub_infos.emplace_back(
									"`mut` parameters can only accept concrete values that are mutable"
								);
							} break;

							case AST::FuncDecl::Param::Kind::In: {
								sub_infos.emplace_back("`in` parameters can only accept ephemeral values");
							} break;
						}

						infos.emplace_back(
							std::format("Failed to match: argument (index: {}) value kind mismatch", reason.arg_index),
							this->get_location(func_infos[i].func_id),
							std::move(sub_infos)
						);

					}else if constexpr(std::is_same<ReasonT, OverloadScore::IncorrectLabel>()){
						const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(func_infos[i].func_id);

						infos.emplace_back(
							std::format("Failed to match: argument (index: {}) incorrect label", reason.arg_index),
							this->get_location(func_infos[i].func_id),
							evo::SmallVector<Diagnostic::Info>{
								Diagnostic::Info(
									"This label:", this->get_location(*arg_infos[reason.arg_index].ast_arg.label)
								),
								Diagnostic::Info(
									std::format(
										"Expected label: \"{}\"", 
										this->context.getSourceManager()[sema_func.sourceID]
											.getTokenBuffer()[sema_func.params[reason.arg_index].ident].getString()
									)
								),
							}
						);

					}else{
						static_assert(false, "Unsupported overload score reason");
					}
				});
			}

			this->emit_error(
				Diagnostic::Code::SemaNoMatchingFunction,
				call_node,
				"No matching function overload found",
				std::move(infos)
			);
			return evo::resultError;


		}else if(found_matching_best_score){ // found multiple matches
			auto infos = evo::SmallVector<Diagnostic::Info>();
			for(size_t i = 0; const OverloadScore& score : scores){
				EVO_DEFER([&](){ i += 1; });

				if(score.score == best_score){
					infos.emplace_back("Could be this one:", this->get_location(func_infos[i].func_id));
				}
			}

			this->emit_error(
				Diagnostic::Code::SemaMultipleMatchingFunctionOverloads,
				call_node,
				"Multiple matching function overloads found",
				std::move(infos)
			);
			return evo::resultError;
		}


		const SelectFuncOverloadFuncInfo& selected_func = func_infos[best_score_index];

		for(size_t i = 0; SelectFuncOverloadArgInfo& arg_info : arg_infos){
			if(this->type_check<true>( // this is to implicitly convert all the required args
				selected_func.func_type.params[i].typeID,
				arg_info.term_info,
				"Function call argument",
				arg_info.ast_arg.value
			).ok == false){
				evo::debugFatalBreak("This should not be able to fail");
			}
		
			i += 1;
		}

		return best_score_index;
	}



	//////////////////////////////////////////////////////////////////////
	// attributes


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
					std::format("Unknown struct attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}


		return StructAttrs(attr_pub.is_set());
	}


	auto SemanticAnalyzer::analyze_func_attrs(
		const AST::FuncDecl& func_decl, evo::ArrayProxy<Instruction::AttributeParams> attribute_params_info
	) -> evo::Result<FuncAttrs> {
		auto attr_pub = ConditionalAttribute(*this, "pub");
		auto attr_runtime = ConditionalAttribute(*this, "runtime");
		auto attr_entry = Attribute(*this, "entry");

		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(func_decl.attributeBlock);

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

			}else if(attribute_str == "runtime"){
				if(attribute_params_info[i].empty()){
					if(attr_runtime.set(attribute.attribute, true) == false){ return evo::resultError; } 

				}else if(attribute_params_info[i].size() == 1){
					TermInfo& cond_term_info = this->get_term_info(attribute_params_info[i][0]);
					if(this->check_term_isnt_type(cond_term_info, attribute.args[0]) == false){return evo::resultError;}

					if(this->type_check<true>(
						this->context.getTypeManager().getTypeBool(),
						cond_term_info,
						"Condition in #runtime",
						attribute.args[0]
					).ok == false){
						return evo::resultError;
					}

					const bool runtime_cond = this->context.sema_buffer
						.getBoolValue(cond_term_info.getExpr().boolValueID()).value;

					if(attr_runtime.set(attribute.attribute, runtime_cond) == false){ return evo::resultError; }

				}else{
					this->emit_error(
						Diagnostic::Code::SemaTooManyAttributeArgs,
						attribute.args[1],
						"Attribute #runtime does not accept more than 1 argument"
					);
					return evo::resultError;
				}

			}else if(attribute_str == "entry"){
				if(attribute_params_info.empty() == false){
					this->emit_error(
						Diagnostic::Code::SemaTooManyAttributeArgs,
						attribute.args.front(),
						"Attribute #entry does not accept any arguments"
					);
					return evo::resultError;
				}

				if(attr_entry.set(attribute.attribute) == false){ return evo::resultError; }
				attr_runtime.implicitly_set(attribute.attribute, true);


			}else{
				this->emit_error(
					Diagnostic::Code::SemaUnknownAttribute,
					attribute.attribute,
					std::format("Unknown function attribute #{}", attribute_str)
				);
				return evo::resultError;
			}
		}


		return FuncAttrs(attr_pub.is_set());
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
		evo::debugAssert(
			this->symbol_proc.type_ids[symbol_proc_type_id.get()].has_value(),
			"Symbol proc type wasn't set"
		);
		return *this->symbol_proc.type_ids[symbol_proc_type_id.get()];
	}

	auto SemanticAnalyzer::return_type(SymbolProc::TypeID symbol_proc_type_id, TypeInfo::VoidableID&& id) -> void {
		this->symbol_proc.type_ids[symbol_proc_type_id.get()] = std::move(id);
	}


	auto SemanticAnalyzer::get_term_info(SymbolProc::TermInfoID symbol_proc_term_info_id) -> TermInfo& {
		evo::debugAssert(
			this->symbol_proc.term_infos[symbol_proc_term_info_id.get()].has_value(),
			"Symbol proc term info wasn't set"
		);
		return *this->symbol_proc.term_infos[symbol_proc_term_info_id.get()];
	}

	auto SemanticAnalyzer::return_term_info(SymbolProc::TermInfoID symbol_proc_term_info_id, auto&&... args) -> void {
		this->symbol_proc.term_infos[symbol_proc_term_info_id.get()]
			.emplace(std::forward<decltype(args)>(args)...);
	}



	auto SemanticAnalyzer::get_struct_instantiation(SymbolProc::StructInstantiationID instantiation_id)
	-> const BaseType::StructTemplate::Instantiation& {
		evo::debugAssert(
			this->symbol_proc.struct_instantiations[instantiation_id.get()] != nullptr,
			"Symbol proc struct instantiation wasn't set"
		);
		return *this->symbol_proc.struct_instantiations[instantiation_id.get()];
	}

	auto SemanticAnalyzer::return_struct_instantiation(
		SymbolProc::StructInstantiationID instantiation_id,
		const BaseType::StructTemplate::Instantiation& instantiation
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


						const core::GenericFloat target_min = type_manager.getMin(expected_type_info.baseTypeID())
							.as<core::GenericFloat>().asF128();

						const core::GenericFloat target_max = type_manager.getMax(expected_type_info.baseTypeID())
							.as<core::GenericFloat>().asF128();


						const core::GenericFloat converted_literal = float_value.value.asF128();

						if(converted_literal.lt(target_min) || converted_literal.gt(target_max)){
							this->emit_error(
								Diagnostic::Code::SemaCannotConvertFluidValue,
								location,
								"Cannot implicitly convert this fluid value to the target type",
								Diagnostic::Info("Requires truncation (maybe use [as] operator)")
							);
							return TypeCheckInfo(false, false);
						}


						switch(expected_type_primitive.kind()){
							break; case Token::Kind::TypeF16:  float_value.value = float_value.value.asF16();
							break; case Token::Kind::TypeBF16: float_value.value = float_value.value.asBF16();
							break; case Token::Kind::TypeF32:  float_value.value = float_value.value.asF32();
							break; case Token::Kind::TypeF64:  float_value.value = float_value.value.asF64();
							break; case Token::Kind::TypeF80:  float_value.value = float_value.value.asF80();
							break; case Token::Kind::TypeF128: float_value.value = float_value.value.asF128();
							break; case Token::Kind::TypeCLongDouble: {
								if(this->context.getTypeManager().sizeOf(expected_type_info.baseTypeID()) == 8){
									float_value.value = float_value.value.asF64();
								}else{
									float_value.value = float_value.value.asF128();
								}
							}
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

		std::string expected_type_str = std::string("Expected type: ");
		auto got_type_str = std::string("Expression is type: ");

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
				"{} cannot accept an expression of a different type, "
					"and this expression cannot be implicitly converted to the correct type",
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
				this->error_already_defined<false>(
					ast_node,
					ident_str,
					*current_scope_level.lookupIdent(ident_str),
					std::forward<decltype(ident_id_info)>(ident_id_info)...
				);
			}

			return false;
		}


		for(auto iter = std::next(target_scope.begin()); iter != target_scope.end(); ++iter){
			sema::ScopeLevel& scope_level = this->context.sema_buffer.scope_manager.getLevel(*iter);
			if(scope_level.disallowIdentForShadowing(ident_str, add_ident_result.value()) == false){
				this->error_already_defined<true>(
					ast_node,
					ident_str,
					*scope_level.lookupIdent(ident_str),
					std::forward<decltype(ident_id_info)>(ident_id_info)...
				);
				return false;
			}
		}

		return true;
	}


	template<bool IS_SHADOWING>
	auto SemanticAnalyzer::error_already_defined_impl(
		const auto& redef_id,
		std::string_view ident_str,
		const sema::ScopeLevel::IdentID& first_defined_id,
		std::optional<sema::Func::ID> attempted_decl_func_id
	)  -> void {
		first_defined_id.visit([&](const auto& first_decl_ident_id) -> void {
			using IdentIDType = std::decay_t<decltype(first_decl_ident_id)>;

			static constexpr bool IS_FUNC_OVERLOAD_COLLISION = 
				std::is_same<std::remove_cvref_t<std::decay_t<decltype(redef_id)>>, pcit::panther::AST::FuncDecl>() 
				&& std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()
				&& !IS_SHADOWING;

			auto infos = evo::SmallVector<Diagnostic::Info>();

			if constexpr(IS_FUNC_OVERLOAD_COLLISION){
				const sema::Func& attempted_decl_func = this->context.getSemaBuffer().getFunc(*attempted_decl_func_id);

				for(const evo::Variant<sema::Func::ID, sema::TemplatedFuncID>& overload_id : first_decl_ident_id){
					if(overload_id.is<sema::TemplatedFuncID>()){ continue; }

					const sema::Func& overload = this->context.sema_buffer.getFunc(overload_id.as<sema::Func::ID>());
					if(attempted_decl_func.isEquivalentOverload(overload, this->context)){
						// TODO: better messaging
						infos.emplace_back(
							"Overload collided with:", this->get_location(overload_id.as<sema::Func::ID>())
						);
						break;
					}
				}
				
			}else if constexpr(std::is_same<IdentIDType, sema::ScopeLevel::FuncOverloadList>()){
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

			const std::string message = [&](){
				if constexpr(IS_FUNC_OVERLOAD_COLLISION){
					return std::format(
						"Function \"{}\" has an overload that collides with this declaration", ident_str
					);
				}else{
					return std::format("Identifier \"{}\" was already defined in this scope", ident_str);
				}
			}();


			this->emit_error(
				Diagnostic::Code::SemaIdentAlreadyInScope, redef_id, std::move(message), std::move(infos)
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

			}else if constexpr(std::is_same<TypeID, TermInfo::FuncOverloadList>()){
				// TODO: actual name
				return "{FUNCTION}";

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



	auto SemanticAnalyzer::check_scope_isnt_terminated(const auto& location) -> bool {
		if(this->get_current_scope_level().isTerminated() == false){ return true; }

		this->emit_error(
			Diagnostic::Code::SemaScopeIsAlreadyTerminated,
			location,
			"Scope is already terminated"
		);
		return false;
	}


}