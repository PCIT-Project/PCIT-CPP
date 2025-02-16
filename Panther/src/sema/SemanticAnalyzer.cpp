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
				case Result::Success: {
					this->symbol_proc.nextInstruction();
				} break;

				case Result::Error: {
					this->context.symbol_proc_manager.symbol_proc_done();
					return;
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

			}else if constexpr(std::is_same<InstrType, Instruction::StructDecl>()){
				return this->instr_struct_decl(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::StructDef>()){
				return this->instr_struct_def();

			}else if constexpr(std::is_same<InstrType, Instruction::FuncCall>()){
				return this->instr_func_call(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::Import>()){
				return this->instr_import(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::TypeAccessor>()){
				return this->instr_expr_accessor<true, false>(instr.infix, instr.lhs, instr.rhs_ident, instr.output);

			}else if constexpr(std::is_same<InstrType, Instruction::ComptimeExprAccessor>()){
				return this->instr_expr_accessor<true, true>(instr.infix, instr.lhs, instr.rhs_ident, instr.output);

			}else if constexpr(std::is_same<InstrType, Instruction::ExprAccessor>()){
				return this->instr_expr_accessor<false, true>(instr.infix, instr.lhs, instr.rhs_ident, instr.output);

			}else if constexpr(std::is_same<InstrType, Instruction::PrimitiveType>()){
				return this->instr_primitive_type(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::UserType>()){
				return this->instr_user_type(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::BaseTypeIdent>()){
				return this->instr_base_type_ident(instr);

			}else if constexpr(std::is_same<InstrType, Instruction::ComptimeIdent>()){
				return this->instr_ident<true>(instr.ident, instr.output);

			}else if constexpr(std::is_same<InstrType, Instruction::Ident>()){
				return this->instr_ident<false>(instr.ident, instr.output);

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

		ExprInfo& value_expr_info = this->get_expr_info(instr.value_id);

		if(value_expr_info.value_category == ExprInfo::ValueCategory::Initializer){
			if(instr.var_decl.kind != AST::VarDecl::Kind::Var){
				this->emit_error(
					Diagnostic::Code::SemaVarInitializerOnNonVar,
					instr.var_decl,
					"Only `var` variables can be defined with an initializer value"
				);
				return Result::Error;
			}

		}else{
			if(value_expr_info.is_ephemeral() == false){
				if(value_expr_info.value_category == ExprInfo::ValueCategory::Module){
					this->error_type_mismatch(
						*sema_var.typeID, value_expr_info, "Variable definition", *instr.var_decl.value
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
				*sema_var.typeID, value_expr_info, "Variable definition", *instr.var_decl.value
			).ok == false){
				return Result::Error;
			}
		}

		sema_var.expr = value_expr_info.getExpr();

		this->propagate_finished_def();
		return Result::Success;
	}


	auto SemanticAnalyzer::instr_var_decl_def(const Instruction::VarDeclDef& instr) -> Result {
		const std::string_view var_ident = this->source.getTokenBuffer()[instr.var_decl.ident].getString();

		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_var_decl_def: {}", this->symbol_proc.ident); });

		const evo::Result<VarAttrs> var_attrs = this->analyze_var_attrs(instr.var_decl, instr.attribute_exprs);
		if(var_attrs.isError()){ return Result::Error; }


		ExprInfo& value_expr_info = this->get_expr_info(instr.value_id);
		if(value_expr_info.value_category == ExprInfo::ValueCategory::Module){
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
				value_expr_info.type_id.as<Source::ID>(),
				instr.var_decl.ident,
				var_attrs.value().is_pub
			);

			this->propagate_finished_decl_def();
			return is_redef ? Result::Error : Result::Success;
		}


		if(value_expr_info.value_category == ExprInfo::ValueCategory::Initializer){
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


		if(instr.type_id.has_value()){
			const TypeInfo::VoidableID got_type_info_id = this->get_type(*instr.type_id);

			if(got_type_info_id.isVoid()){
				this->emit_error(
					Diagnostic::Code::SemaVarTypeVoid, *instr.var_decl.type, "Variables cannot be type `Void`"
				);
				return Result::Error;
			}

			if(this->type_check<true>(
				got_type_info_id.asTypeID(), value_expr_info, "Variable definition", *instr.var_decl.value
			).ok == false){
				return Result::Error;
			}
		}

		const std::optional<TypeInfo::ID> type_id = [&](){
			if(value_expr_info.type_id.is<TypeInfo::ID>()){
				return std::optional<TypeInfo::ID>(value_expr_info.type_id.as<TypeInfo::ID>());
			}
			return std::optional<TypeInfo::ID>();
		}();

		const sema::Var::ID new_sema_var = this->context.sema_buffer.createVar(
			instr.var_decl.kind,
			instr.var_decl.ident,
			std::optional<sema::Expr>(value_expr_info.getExpr()),
			type_id,
			var_attrs.value().is_pub
		);

		if(this->add_ident_to_scope(var_ident, instr.var_decl, new_sema_var) == false){ return Result::Error; }

		this->propagate_finished_decl_def();
		return Result::Success;
	}



	auto SemanticAnalyzer::instr_when_cond(const Instruction::WhenCond& instr) -> Result {
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
				const auto lock = std::scoped_lock(passed_symbol.decl_waited_on_lock, passed_symbol.def_waited_on_lock);
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
				if(instr.attribute_exprs[i].empty()){
					if(attr_pub.set(attribute.attribute, true) == false){ return Result::Error; } 

				}else if(instr.attribute_exprs[i].size() == 1){
					ExprInfo cond_expr_info = this->get_expr_info(instr.attribute_exprs[i][0]);

					if(this->type_check<true>(
						this->context.getTypeManager().getTypeBool(),
						cond_expr_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return Result::Error;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_expr_info.getExpr().boolValueID()).value;

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



	auto SemanticAnalyzer::instr_struct_decl(const Instruction::StructDecl& instr) -> Result {
		EVO_DEFER([&](){ this->context.trace("SemanticAnalyzer::instr_struct_decl: {}", this->symbol_proc.ident); });

		auto attr_pub = ConditionalAttribute(*this, "pub");

		const AST::AttributeBlock& attribute_block = 
			this->source.getASTBuffer().getAttributeBlock(instr.struct_decl.attributeBlock);

		for(size_t i = 0; const AST::AttributeBlock::Attribute& attribute : attribute_block.attributes){
			EVO_DEFER([&](){ i += 1; });
			
			const std::string_view attribute_str = this->source.getTokenBuffer()[attribute.attribute].getString();

			if(attribute_str == "pub"){
				if(instr.attribute_exprs[i].empty()){
					if(attr_pub.set(attribute.attribute, true) == false){ return Result::Error; } 

				}else if(instr.attribute_exprs[i].size() == 1){
					ExprInfo cond_expr_info = this->get_expr_info(instr.attribute_exprs[i][0]);

					if(this->type_check<true>(
						this->context.getTypeManager().getTypeBool(),
						cond_expr_info,
						"Condition in #pub",
						attribute.args[0]
					).ok == false){
						return Result::Error;
					}

					const bool pub_cond = this->context.sema_buffer
						.getBoolValue(cond_expr_info.getExpr().boolValueID()).value;

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

		SymbolProc::StructInfo& struct_info = this->symbol_proc.extra_info.as<SymbolProc::StructInfo>();

		const BaseType::ID created_struct = this->context.type_manager.getOrCreateStruct(
			BaseType::Struct(
				this->source.getID(),
				instr.struct_decl.ident,
				struct_info.member_symbols,
				nullptr,
				attr_pub.is_set()
			)
		);

		struct_info.struct_id = created_struct.structID();

		const std::string_view ident_str = this->source.getTokenBuffer()[instr.struct_decl.ident].getString();
		if(this->add_ident_to_scope(ident_str, instr.struct_decl, created_struct.structID()) == false){
			return Result::Error;
		}

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


	template<bool NEEDS_DEF, bool IS_EXPR>
	auto SemanticAnalyzer::instr_expr_accessor(
		const AST::Infix& infix, SymbolProcExprInfoID lhs_id, Token::ID rhs_ident, SymbolProcExprInfoID output
	) -> Result {
		const std::string_view rhs_ident_str = this->source.getTokenBuffer()[rhs_ident].getString();
		const ExprInfo& lhs = this->get_expr_info(lhs_id);

		if(lhs.type_id.is<Source::ID>()){
			const Source& source_module = this->context.getSourceManager()[lhs.type_id.as<Source::ID>()];

			const sema::ScopeManager::Scope& source_module_sema_scope = 
				this->context.sema_buffer.scope_manager.getScope(*source_module.sema_scope_id);


			const sema::ScopeLevel& scope_level = this->context.sema_buffer.scope_manager.getLevel(
				source_module_sema_scope.getGlobalLevel()
			);

			const evo::Result<std::optional<ExprInfo>> expr_ident = 
				this->analyze_expr_ident_in_scope_level<NEEDS_DEF, IS_EXPR, true>(
					rhs_ident, rhs_ident_str, scope_level, true, true, &source_module
				);

			if(expr_ident.isError()){ return Result::Error; }

			if(expr_ident.value().has_value()){
				this->return_expr_info(output, std::move(*expr_ident.value()));
				return Result::Success;
			}

			return this->wait_on_symbol_proc<NEEDS_DEF>(
				source_module.global_symbol_procs,
				infix.rhs,
				rhs_ident_str,
				std::format("Module has no symbol named \"{}\"", rhs_ident_str),
				[&]() -> Result {
					const evo::Result<std::optional<ExprInfo>> expr_ident = 
						this->analyze_expr_ident_in_scope_level<NEEDS_DEF, IS_EXPR, true>(
							rhs_ident, rhs_ident_str, scope_level, true, true, &source_module
						);

					if(expr_ident.isError()){ return Result::Error; }

					if(expr_ident.value().has_value()){
						this->return_expr_info(output, std::move(*expr_ident.value()));
						return Result::Success;
					}

					evo::debugFatalBreak("Def is done, but can't find sema of symbol");
				}
			);

		}else if(lhs.type_id.is<TypeInfo::ID>()){
			const TypeInfo::ID actual_lhs_type_id = this->get_actual_type<true>(lhs.type_id.as<TypeInfo::ID>());
			const TypeInfo& actual_lhs_type = this->context.getTypeManager().getTypeInfo(actual_lhs_type_id);

			if(actual_lhs_type.qualifiers().empty() == false){
				// TODO: better message
				this->emit_error(
					Diagnostic::Code::SemaInvalidAccessorRHS, infix.lhs, "Accessor operator of this LHS is unsupported"
				);
				return Result::Error;
			}

			if(actual_lhs_type.baseTypeID().kind() != BaseType::Kind::Struct){
				// TODO: better message
				this->emit_error(
					Diagnostic::Code::SemaInvalidAccessorRHS, infix.lhs, "Accessor operator of this LHS is unsupported"
				);
				return Result::Error;	
			}


			const BaseType::Struct& lhs_struct = this->context.getTypeManager().getStruct(
				actual_lhs_type.baseTypeID().structID()
			);

			const Source& struct_source = this->context.getSourceManager()[lhs_struct.sourceID];


			const evo::Result<std::optional<ExprInfo>> expr_ident = 
				this->analyze_expr_ident_in_scope_level<NEEDS_DEF, IS_EXPR, false>(
					rhs_ident, rhs_ident_str, *lhs_struct.scopeLevel, true, true, &struct_source
				);

			if(expr_ident.isError()){ return Result::Error; }

			if(expr_ident.value().has_value()){
				this->return_expr_info(output, std::move(*expr_ident.value()));
				return Result::Success;
			}

			return this->wait_on_symbol_proc<NEEDS_DEF>(
				lhs_struct.memberSymbols,
				infix.rhs,
				rhs_ident_str,
				std::format("Struct has no member named \"{}\"", rhs_ident_str),
				[&]() -> Result {
					const evo::Result<std::optional<ExprInfo>> expr_ident = 
						this->analyze_expr_ident_in_scope_level<NEEDS_DEF, IS_EXPR, true>(
							rhs_ident, rhs_ident_str, *lhs_struct.scopeLevel, true, true, &struct_source
						);

					if(expr_ident.isError()){ return Result::Error; }

					if(expr_ident.value().has_value()){
						this->return_expr_info(output, std::move(*expr_ident.value()));
						return Result::Success;
					}

					evo::debugFatalBreak("Def is done, but can't find sema of member");
				}
			);
		}

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			infix.lhs,
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
		evo::debugAssert(this->get_expr_info(instr.base_type).value_category == ExprInfo::ValueCategory::Type);
		const TypeInfo::ID base_type_id = this->get_expr_info(instr.base_type).type_id.as<TypeInfo::ID>();

		const TypeInfo& base_type = this->context.getTypeManager().getTypeInfo(base_type_id);

		// switch(base_type.baseTypeID().kind()){
		// 	case BaseType::Kind::Dummy: evo::debugFatalBreak("Not a valid base type kind");
		// 	case BaseType::Kind::Primitive: evo::debugFatalBreak("Not a user type");
		// 	case BaseType::Kind::Function: {
		// 		this->emit_error(
		// 			Diagnostic::Code::MiscUnimplementedFeature, instr.ast_type.base, "Function types are unimplemented"
		// 		);
		// 		return Result::Error;
		// 	} break;

		// 	case BaseType::Kind::Array: {
		// 		this->emit_error(
		// 			Diagnostic::Code::MiscUnimplementedFeature, instr.ast_type.base, "Array types are unimplemented"
		// 		);
		// 		return Result::Error;
		// 	} break;

		// 	case BaseType::Kind::Alias: {
		// 		const Alias&
		// 	} break;

		// 	case BaseType::Kind::Typedef: {
		// 		this->emit_error(
		// 			Diagnostic::Code::MiscUnimplementedFeature, instr.ast_type.base, "Typedef types are unimplemented"
		// 		);
		// 		return Result::Error;
		// 	} break;
		// }

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
		const evo::Expected<ExprInfo, Result> lookup_ident_result = this->lookup_ident_impl<true, false>(instr.ident);
		if(lookup_ident_result.has_value() == false){ return lookup_ident_result.error(); }

		this->return_expr_info(instr.output, lookup_ident_result.value());
		return Result::Success;
	}



	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::instr_ident(Token::ID ident, SymbolProc::ExprInfoID output) -> Result {
		const evo::Expected<ExprInfo, Result> lookup_ident_result = this->lookup_ident_impl<NEEDS_DEF, true>(ident);
		if(lookup_ident_result.has_value() == false){ return lookup_ident_result.error(); }

		this->return_expr_info(output, std::move(lookup_ident_result.value()));
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


	auto SemanticAnalyzer::instr_uninit(const Instruction::Uninit& instr) -> Result {
		this->return_expr_info(instr.output,
			ExprInfo::ValueCategory::Initializer,
			ExprInfo::ValueStage::Comptime,
			ExprInfo::InitializerType(),
			sema::Expr(this->context.sema_buffer.createUninit(instr.uninit_token))
		);
		return Result::Success;
	}

	auto SemanticAnalyzer::instr_zeroinit(const Instruction::Zeroinit& instr) -> Result {
		this->return_expr_info(instr.output,
			ExprInfo::ValueCategory::Initializer,
			ExprInfo::ValueStage::Comptime,
			ExprInfo::InitializerType(),
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




	template<bool NEEDS_DEF, bool IS_EXPR>
	auto SemanticAnalyzer::lookup_ident_impl(Token::ID ident) -> evo::Expected<ExprInfo, Result> {
		const std::string_view ident_str = this->source.getTokenBuffer()[ident].getString();

		for(size_t i = this->scope.size() - 1; sema::ScopeLevel::ID scope_level_id : this->scope){
			const evo::Result<std::optional<ExprInfo>> scope_level_lookup = 
				this->analyze_expr_ident_in_scope_level<NEEDS_DEF, IS_EXPR, false>(
					ident,
					ident_str,
					this->context.sema_buffer.scope_manager.getLevel(scope_level_id),
					i >= this->scope.getCurrentObjectScopeIndex() || i == 0,
					i == 0,
					nullptr
				);

			if(scope_level_lookup.isError()){ return evo::Unexpected(Result::Error); }

			if(scope_level_lookup.value().has_value()){
				return *scope_level_lookup.value();
			}

			i -= 1;
		}


		auto maybe_output = std::optional<ExprInfo>();

		const Result wait_on_symbol_proc_result = this->wait_on_symbol_proc<NEEDS_DEF>(
			this->source.global_symbol_procs,
			ident,
			ident_str,
			std::format("Identifier \"{}\" was not defined in this scope", ident_str),
			[&]() -> Result {
				for(size_t i = this->scope.size() - 1; sema::ScopeLevel::ID scope_level_id : this->scope){
					const evo::Result<std::optional<ExprInfo>> scope_level_lookup = 
						this->analyze_expr_ident_in_scope_level<NEEDS_DEF, IS_EXPR, false>(
							ident,
							ident_str,
							this->context.sema_buffer.scope_manager.getLevel(scope_level_id),
							i >= this->scope.getCurrentObjectScopeIndex() || i == 0,
							i == 0,
							nullptr
						);

					if(scope_level_lookup.isError()){ return Result::Error; }

					if(scope_level_lookup.value().has_value()){
						maybe_output = *scope_level_lookup.value();
						return Result::Success;
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




	template<bool NEEDS_DEF, bool IS_EXPR, bool PUB_REQUIRED>
	auto SemanticAnalyzer::analyze_expr_ident_in_scope_level(
		const Token::ID& ident,
		std::string_view ident_str,
		const sema::ScopeLevel& scope_level,
		bool variables_in_scope, // TODO: make this template argument?
		bool is_global_scope, // TODO: make this template argumnet?
		const Source* source_module
	) -> evo::Result<std::optional<ExprInfo>> {
		if constexpr(PUB_REQUIRED){
			evo::debugAssert(variables_in_scope, "IF `PUB_REQUIRED`, `variables_in_scope` should be true");
			evo::debugAssert(is_global_scope, "IF `PUB_REQUIRED`, `is_global_scope` should be true");
		}

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
						std::format("Variable \"{}\" is not accessable in this scope", ident_str),
						Diagnostic::Info(
							"Local variables, parameters, and members cannot be accessed inside a sub-object scope. "
								"Defined here:",
							this->get_location(ident_id)
						)
					);
					return evo::resultError;
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
						return evo::resultError;
					}

				}

				using ValueCategory = ExprInfo::ValueCategory;
				using ValueStage = ExprInfo::ValueStage;

				switch(sema_var.kind){
					case AST::VarDecl::Kind::Var: {
						if constexpr(NEEDS_DEF){
							if(sema_var.expr.load().has_value() == false){ return std::optional<ExprInfo>(); }
						}
						
						return std::optional<ExprInfo>(ExprInfo(
							ValueCategory::ConcreteMut,
							is_global_scope ? ValueStage::Runtime : ValueStage::Constexpr,
							*sema_var.typeID,
							sema::Expr(ident_id)
						));
					} break;

					case AST::VarDecl::Kind::Const: {
						if constexpr(NEEDS_DEF){
							if(sema_var.expr.load().has_value() == false){ return std::optional<ExprInfo>(); }
						}

						return std::optional<ExprInfo>(ExprInfo(
							ValueCategory::ConcreteConst, ValueStage::Constexpr, *sema_var.typeID, sema::Expr(ident_id)
						));
					} break;

					case AST::VarDecl::Kind::Def: {
						if(sema_var.typeID.has_value()){
							return std::optional<ExprInfo>(ExprInfo(
								ValueCategory::Ephemeral, ValueStage::Comptime, *sema_var.typeID, *sema_var.expr.load()
							));
						}else{
							return std::optional<ExprInfo>(ExprInfo(
								ValueCategory::EphemeralFluid,
								ValueStage::Comptime,
								ExprInfo::FluidType{},
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
				return evo::resultError;

			}else if constexpr(std::is_same<IdentIDType, sema::ReturnParamID>()){
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ident,
					"return parameter identifiers are unimplemented"
				);
				return evo::resultError;

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
						return evo::resultError;
					}
				}

				return std::optional<ExprInfo>(
					ExprInfo(
						ExprInfo::ValueCategory::Module,
						ExprInfo::ValueStage::Comptime,
						ident_id.sourceID,
						sema::Expr::createModuleIdent(ident_id.tokenID)
					)
				);

			}else if constexpr(std::is_same<IdentIDType, BaseType::Alias::ID>()){
				if constexpr(IS_EXPR){
					this->emit_error(
						Diagnostic::Code::SemaTypeUsedAsExpr,
						ident,
						"Type alias cannot be used as an expression"
					);
					return evo::resultError;
				}else{
					const BaseType::Alias& alias = this->context.getTypeManager().getAlias(ident_id);

					if constexpr(NEEDS_DEF){
						if(alias.defCompleted() == false){ return std::optional<ExprInfo>(); }
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
							return evo::resultError;
						}
					}

					return std::optional<ExprInfo>(
						ExprInfo(
							ExprInfo::ValueCategory::Type,
							ExprInfo::ValueStage::Comptime,
							this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(ident_id))),
							std::nullopt
						)
					);
				}

			}else if constexpr(std::is_same<IdentIDType, BaseType::Typedef::ID>()){
				this->emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					ident,
					"Using typedefs is currently unimplemented"
				);
				return evo::resultError;

			}else if constexpr(std::is_same<IdentIDType, BaseType::Struct::ID>()){
				if constexpr(IS_EXPR){
					this->emit_error(
						Diagnostic::Code::SemaTypeUsedAsExpr,
						ident,
						"Struct cannot be used as an expression"
					);
					return evo::resultError;
				}else{
					const BaseType::Struct& struct_info = this->context.getTypeManager().getStruct(ident_id);

					if constexpr(NEEDS_DEF){
						if(struct_info.defCompleted == false){
							// TODO: optimize this to signify exists but not ready
							// def not ready
							return std::optional<ExprInfo>();
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
							return evo::resultError;
						}
					}

					return std::optional<ExprInfo>(
						ExprInfo(
							ExprInfo::ValueCategory::Type,
							ExprInfo::ValueStage::Comptime,
							this->context.type_manager.getOrCreateTypeInfo(TypeInfo(BaseType::ID(ident_id))),
							std::nullopt
						)
					);
				}

			}else{
				static_assert(false, "Unsupported IdentID");
			}
		});
	}


	template<bool NEEDS_DEF>
	auto SemanticAnalyzer::wait_on_symbol_proc(
		const SymbolProc::Namespace& symbol_proc_namespace,
		const auto& ident,
		std::string_view ident_str,
		std::string&& error_msg_if_ident_doesnt_exist,
		std::function<Result()> func_if_def_completed
	) -> Result {
		const auto find = symbol_proc_namespace.equal_range(ident_str);

		if(find.first == symbol_proc_namespace.end()){
			this->emit_error(
				Diagnostic::Code::SemaNoSymbolInModuleWithThatIdent,
				ident,
				std::move(error_msg_if_ident_doesnt_exist)
			);
			return Result::Error;
		}

		const auto found_range = core::IterRange(find.first, find.second);


		bool found_was_passed_by_when_cond = false;
		for(auto& pair : found_range){
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

		if(target.waiting_for.empty()){
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

			if(waited_on.waiting_for.empty()){
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

		TypeInfo::ID actual_expected_type_id = this->get_actual_type<false>(expected_type_id);

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

				TypeInfo::ID actual_got_type_id = this->get_actual_type<false>(got_expr.type_id.as<TypeInfo::ID>());

				// if types are not exact, check if implicit conversion is valid
				if(actual_expected_type_id != actual_got_type_id){
					const TypeInfo& expected_type = this->context.getTypeManager().getTypeInfo(actual_expected_type_id);
					const TypeInfo& got_type      = this->context.getTypeManager().getTypeInfo(actual_got_type_id);

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
				const TypeInfo& expected_type_info = 
					this->context.getTypeManager().getTypeInfo(actual_expected_type_id);

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