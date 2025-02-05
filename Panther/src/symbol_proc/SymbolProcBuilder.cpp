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
	

	auto SymbolProcBuilder::build(const AST::Node& stmt) -> bool {
		const evo::Result<std::string_view> symbol_ident = this->get_symbol_ident(stmt);
		if(symbol_ident.isError()){ return false; }


		SymbolProc::ID symbol_proc_id = this->context.symbol_proc_manager.create_symbol_proc(
			stmt, this->source.getID(), symbol_ident.value()
		);
		SymbolProc& symbol_proc = this->context.symbol_proc_manager.getSymbolProc(symbol_proc_id);

		this->symbol_proc_infos.emplace_back(symbol_proc_id, symbol_proc);

		
		switch(stmt.kind()){
			break; case AST::Kind::VarDecl:         if(this->build_var_decl(stmt) == false){ return false; }
			break; case AST::Kind::FuncDecl:        if(this->build_func_decl(stmt) == false){ return false; }
			break; case AST::Kind::AliasDecl:       if(this->build_alias_decl(stmt) == false){ return false; }
			break; case AST::Kind::TypedefDecl:     if(this->build_typedef_decl(stmt) == false){ return false; }
			break; case AST::Kind::StructDecl:      if(this->build_struct_decl(stmt) == false){ return false; }
			break; case AST::Kind::WhenConditional: if(this->build_when_conditional(stmt) == false){ return false; }
			break; case AST::Kind::FuncCall:        if(this->build_func_call(stmt) == false){ return false; }

			break; default: evo::unreachable();
		}

		symbol_proc.expr_infos.resize(this->get_current_symbol().num_expr_infos);
		symbol_proc.type_ids.resize(this->get_current_symbol().num_type_ids);

		this->symbol_proc_infos.pop_back();

		return true;
	}


	auto SymbolProcBuilder::get_symbol_ident(const AST::Node& stmt) -> evo::Result<std::string_view> {
		const TokenBuffer& token_buffer = this->source.getTokenBuffer();
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(stmt.kind()){
			case AST::Kind::None: evo::debugFatalBreak("Not a valid AST node");

			case AST::Kind::VarDecl: {
				return token_buffer[ast_buffer.getVarDecl(stmt).ident].getString();
			} break;

			case AST::Kind::FuncDecl: {
				const AST::FuncDecl& func_decl = ast_buffer.getFuncDecl(stmt);
				if(func_decl.name.kind() == AST::Kind::Ident){
					return token_buffer[ast_buffer.getIdent(func_decl.name)].getString();
				}else{
					return std::string_view();
				}
			} break;

			case AST::Kind::AliasDecl: {
				return token_buffer[ast_buffer.getAliasDecl(stmt).ident].getString();
			} break;

			case AST::Kind::TypedefDecl: {
				return token_buffer[ast_buffer.getTypedefDecl(stmt).ident].getString();
			} break;

			case AST::Kind::StructDecl: {
				return token_buffer[ast_buffer.getStructDecl(stmt).ident].getString();
			} break;

			case AST::Kind::WhenConditional: {
				return std::string_view();
			} break;

			case AST::Kind::FuncCall: {
				return std::string_view();
			} break;


			case AST::Kind::Return:        case AST::Kind::Conditional:     case AST::Kind::While:
			case AST::Kind::Unreachable:   case AST::Kind::Block:           case AST::Kind::TemplatePack:
			case AST::Kind::TemplatedExpr: case AST::Kind::Prefix:          case AST::Kind::Infix:
			case AST::Kind::Postfix:       case AST::Kind::MultiAssign:     case AST::Kind::New:
			case AST::Kind::Type:          case AST::Kind::TypeIDConverter: case AST::Kind::AttributeBlock:
			case AST::Kind::Attribute:     case AST::Kind::PrimitiveType:   case AST::Kind::Ident:
			case AST::Kind::Intrinsic:     case AST::Kind::Literal:         case AST::Kind::Uninit:
			case AST::Kind::Zeroinit:      case AST::Kind::This:            case AST::Kind::Discard: {
				this->context.emitError(
					Diagnostic::Code::SymbolProcInvalidGlobalStmt,
					Diagnostic::Location::get(stmt, this->source),
					"Invalid global statement"
				);
				return evo::resultError;
			};
		}

		evo::unreachable();
	}



	auto SymbolProcBuilder::build_var_decl(const AST::Node& stmt) -> bool {
		const AST::VarDecl& var_decl = this->source.getASTBuffer().getVarDecl(stmt);

		auto attribute_exprs = evo::SmallVector<evo::StaticVector<SymbolProcExprInfoID, 2>>();

		const AST::AttributeBlock& attr_block = this->source.getASTBuffer().getAttributeBlock(var_decl.attributeBlock);
		for(const AST::AttributeBlock::Attribute& attribute : attr_block.attributes){
			attribute_exprs.emplace_back();

			for(const AST::Node& arg : attribute.args){
				const evo::Result<SymbolProc::ExprInfoID> arg_expr = this->analyze_expr<true>(arg);
				if(arg_expr.isError()){ return false; }

				attribute_exprs.back().emplace_back(arg_expr.value());
			}
		}

		auto type_id = std::optional<SymbolProc::TypeID>();
		if(var_decl.type.has_value()){
			const evo::Result<SymbolProc::TypeID> type_id_res = 
				this->analyze_type(this->source.getASTBuffer().getType(*var_decl.type));
			if(type_id_res.isError()){ return false; }

			type_id = type_id_res.value();

			this->add_instruction(SymbolProc::Instruction::FinishDecl());
		}


		if(var_decl.value.has_value() == false){
			this->emit_error(
				Diagnostic::Code::SemaVarWithNoValue, var_decl, "Variables need to be defined with a value"
			);
			return false;
		}

		const evo::Result<SymbolProc::ExprInfoID> value_id = this->analyze_expr<true>(*var_decl.value);
		if(value_id.isError()){ return false; }

		this->add_instruction(
			SymbolProc::Instruction::GlobalVarDecl(
				this->source.getASTBuffer().getVarDecl(stmt), std::move(attribute_exprs), type_id, value_id.value()
			)
		);

		if(var_decl.type.has_value() == false){ this->add_instruction(SymbolProc::Instruction::FinishDecl()); }

		SymbolProcInfo& current_symbol = this->get_current_symbol();

		if(this->is_child_symbol()){
			SymbolProcInfo& parent_symbol = this->get_parent_symbol();

			parent_symbol.symbol_proc.decl_waited_on_by.emplace_back(current_symbol.symbol_proc_id);
			current_symbol.symbol_proc.waiting_for.emplace_back(parent_symbol.symbol_proc_id);

			this->symbol_scopes.back()->emplace_back(current_symbol.symbol_proc_id);
		}

		this->source.global_symbol_procs.emplace(current_symbol.symbol_proc.getIdent(), current_symbol.symbol_proc_id);

		return true;
	}


	auto SymbolProcBuilder::build_func_decl(const AST::Node& stmt) -> bool {
		// const AST::FuncDecl& func_decl = this->source.getASTBuffer().getFuncDecl(stmt);
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			stmt,
			"Building symbol process of Func Decl is unimplemented"
		);
		return false;
	}

	auto SymbolProcBuilder::build_alias_decl(const AST::Node& stmt) -> bool {
		// const AST::AliasDecl& alias_decl = this->source.getASTBuffer().getAliasDecl(stmt);
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			stmt,
			"Building symbol process of Alias Decl is unimplemented"
		);
		return false;
	}

	auto SymbolProcBuilder::build_typedef_decl(const AST::Node& stmt) -> bool {
		// const AST::TypedefDecl& typedef_decl = this->source.getASTBuffer().getTypedefDecl(stmt);
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			stmt,
			"Building symbol process of Typedef Decl is unimplemented"
		);
		return false;
	}

	auto SymbolProcBuilder::build_struct_decl(const AST::Node& stmt) -> bool {
		// const AST::StructDecl& struct_decl = this->source.getASTBuffer().getStructDecl(stmt);
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			stmt,
			"Building symbol process of Struct Decl is unimplemented"
		);
		return false;
	}

	auto SymbolProcBuilder::build_when_conditional(const AST::Node& stmt) -> bool {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();
		const AST::WhenConditional& when_conditional = ast_buffer.getWhenConditional(stmt);

		const evo::Result<SymbolProc::ExprInfoID> cond_id = this->analyze_expr<true>(when_conditional.cond);
		if(cond_id.isError()){ return false; }

		auto then_symbol_scope = SymbolScope();
		this->symbol_scopes.emplace_back(&then_symbol_scope);
		for(const AST::Node& then_stmt : ast_buffer.getBlock(when_conditional.thenBlock).stmts){
			if(this->build(then_stmt) == false){ return false; }
		}
		this->symbol_scopes.pop_back();

		auto else_symbol_scope = SymbolScope();
		if(when_conditional.elseBlock.has_value()){
			this->symbol_scopes.emplace_back(&else_symbol_scope);
			if(when_conditional.elseBlock->kind() == AST::Kind::Block){
				for(const AST::Node& else_stmt : ast_buffer.getBlock(*when_conditional.elseBlock).stmts){
					if(this->build(else_stmt) == false){ return false; }
				}
			}else{
				if(this->build(*when_conditional.elseBlock) == false){ return false; }
			}
			this->symbol_scopes.pop_back();
		}
		

		this->add_instruction(SymbolProc::Instruction::GlobalWhenCond(when_conditional, cond_id.value()));

		this->source.global_symbol_procs.emplace("", this->get_current_symbol().symbol_proc_id);

		// TODO: address these directly instead of moving them in
		this->get_current_symbol().symbol_proc.extra_info.emplace<SymbolProc::WhenCondInfo>(
			std::move(then_symbol_scope), std::move(else_symbol_scope)
		);

		return true;
	}

	auto SymbolProcBuilder::build_func_call(const AST::Node& stmt) -> bool {
		// const AST::FuncCall& func_call = this->source.getASTBuffer().getFuncCall(stmt);
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature,
			stmt,
			"Building symbol process of Func Call is unimplemented"
		);
		return false;
	}




	auto SymbolProcBuilder::analyze_type(const AST::Type& ast_type) -> evo::Result<SymbolProc::TypeID> {
		const SymbolProc::TypeID created_type_id = this->create_type();

		switch(ast_type.base.kind()){
			case AST::Kind::PrimitiveType: { 
				this->add_instruction(SymbolProc::Instruction::PrimitiveType(ast_type, created_type_id));
				return created_type_id;
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
					Diagnostic::Code::SymbolProcInvalidBaseType, ast_type.base, "Invalid base type"
				);
				return evo::resultError;
			} break;
		}
	}



	template<bool IS_COMPTIME>
	auto SymbolProcBuilder::analyze_expr(const AST::Node& expr) -> evo::Result<SymbolProc::ExprInfoID> {
		const ASTBuffer& ast_buffer = this->source.getASTBuffer();

		switch(expr.kind()){
			case AST::Kind::None: {
				evo::debugFatalBreak("Invalid AST::Node");
			} break;

			case AST::Kind::Block:         return this->analyze_expr_block<IS_COMPTIME>(expr);
			case AST::Kind::FuncCall:      return this->analyze_expr_func_call<IS_COMPTIME>(expr);
			case AST::Kind::TemplatedExpr: return this->analyze_expr_templated<IS_COMPTIME>(expr);
			case AST::Kind::Prefix:        return this->analyze_expr_prefix<IS_COMPTIME>(expr);
			case AST::Kind::Infix:         return this->analyze_expr_infix<IS_COMPTIME>(expr);
			case AST::Kind::Postfix:       return this->analyze_expr_postfix<IS_COMPTIME>(expr);
			case AST::Kind::New:           return this->analyze_expr_new<IS_COMPTIME>(expr);
			case AST::Kind::Ident:         return this->analyze_expr_ident<IS_COMPTIME>(expr);
			case AST::Kind::Intrinsic:     return this->analyze_expr_intrinsic(expr);
			case AST::Kind::Literal:       return this->analyze_expr_literal(ast_buffer.getLiteral(expr));
			case AST::Kind::Uninit:        return this->analyze_expr_uninit(expr);
			case AST::Kind::Zeroinit:      return this->analyze_expr_zeroinit(expr);
			case AST::Kind::This:          return this->analyze_expr_this(expr);

			case AST::Kind::VarDecl:     case AST::Kind::FuncDecl:        case AST::Kind::AliasDecl:
			case AST::Kind::TypedefDecl: case AST::Kind::StructDecl:      case AST::Kind::Return:
			case AST::Kind::Conditional: case AST::Kind::WhenConditional: case AST::Kind::While:
			case AST::Kind::Unreachable: case AST::Kind::TemplatePack:    case AST::Kind::MultiAssign:
			case AST::Kind::Type:        case AST::Kind::TypeIDConverter: case AST::Kind::AttributeBlock:
			case AST::Kind::Attribute:   case AST::Kind::PrimitiveType:   case AST::Kind::Discard: {
				// TODO: better messaging (specify what kind)
				this->emit_fatal(
					Diagnostic::Code::SymbolProcInvalidExprKind,
					Diagnostic::Location::NONE,
					Diagnostic::createFatalMessage("Encountered expr of invalid AST kind")
				);
				return evo::resultError; 
			} break;
		}

		evo::unreachable();
	}



	template<bool IS_COMPTIME>
	auto SymbolProcBuilder::analyze_expr_block(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, node, "Building symbol proc of block is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_COMPTIME>
	auto SymbolProcBuilder::analyze_expr_func_call(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		const AST::FuncCall& func_call = this->source.getASTBuffer().getFuncCall(node);

		if(func_call.target.kind() == AST::Kind::Intrinsic){
			const Token::ID intrin_tok_id = this->source.getASTBuffer().getIntrinsic(func_call.target);
			if(this->source.getTokenBuffer()[intrin_tok_id].getString() == "import"){
				if(func_call.args.empty()){
					this->emit_error(
						Diagnostic::Code::SymbolProcImportRequiresOneArg,
						intrin_tok_id,
						"Calls to @import requires a path"
					);
					return evo::resultError;
				}

				if(func_call.args.size() > 1){
					this->emit_error(
						Diagnostic::Code::SymbolProcImportRequiresOneArg,
						func_call.args[1].value,
						"Calls to @import requires a path, and no other arguments"
					);
					return evo::resultError;	
				}

				const evo::Result<SymbolProc::ExprInfoID> path_value = this->analyze_expr<true>(
					func_call.args[0].value
				);
				if(path_value.isError()){ return evo::resultError; }

				const SymbolProc::ExprInfoID new_expr_info_id = this->create_expr_info();
				this->add_instruction(SymbolProc::Instruction::Import(func_call, path_value.value(), new_expr_info_id));
				return new_expr_info_id;
			}
		}

		const evo::Result<SymbolProc::ExprInfoID> target = this->analyze_expr<IS_COMPTIME>(func_call.target);
		if(target.isError()){ return evo::resultError; }

		auto args = evo::SmallVector<SymbolProc::ExprInfoID>();
		args.reserve(func_call.args.size());
		for(const AST::FuncCall::Arg& arg : func_call.args){
			const evo::Result<SymbolProc::ExprInfoID> arg_value = this->analyze_expr<IS_COMPTIME>(arg.value);
			if(arg_value.isError()){ return evo::resultError; }
			args.emplace_back(arg_value.value());
		}

		const SymbolProc::ExprInfoID new_expr_info_id = this->create_expr_info();
		this->add_instruction(
			SymbolProc::Instruction::FuncCall(func_call, target.value(), new_expr_info_id, std::move(args))
		);
		return new_expr_info_id;
	}

	template<bool IS_COMPTIME>
	auto SymbolProcBuilder::analyze_expr_templated(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, node, "Building symbol proc of templated expr is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_COMPTIME>
	auto SymbolProcBuilder::analyze_expr_prefix(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, node, "Building symbol proc of prefix is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_COMPTIME>
	auto SymbolProcBuilder::analyze_expr_infix(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		const AST::Infix& infix = this->source.getASTBuffer().getInfix(node);

		if(this->source.getTokenBuffer()[infix.opTokenID].kind() == Token::lookupKind(".")){
			const evo::Result<SymbolProc::ExprInfoID> lhs = this->analyze_expr<IS_COMPTIME>(infix.lhs);
			if(lhs.isError()){ return evo::resultError; }

			const Token::ID rhs = this->source.getASTBuffer().getIdent(infix.rhs);

			const SymbolProc::ExprInfoID new_expr_info_id = this->create_expr_info();
			if constexpr(IS_COMPTIME){
				this->add_instruction(
					SymbolProc::Instruction::ComptimeExprAccessor(infix, lhs.value(), rhs, new_expr_info_id)
				);
			}else{
				this->add_instruction(SymbolProc::Instruction::ExprAccessor(infix, lhs.value(), rhs, new_expr_info_id));
			}
			return new_expr_info_id;
		}

		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, node, "Building symbol proc of infix (not [.]) is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_COMPTIME>
	auto SymbolProcBuilder::analyze_expr_postfix(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, node, "Building symbol proc of postfix is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_COMPTIME>
	auto SymbolProcBuilder::analyze_expr_new(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, node, "Building symbol proc of new is unimplemented"
		);
		return evo::resultError;
	}

	template<bool IS_COMPTIME>
	auto SymbolProcBuilder::analyze_expr_ident(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		const SymbolProc::ExprInfoID new_expr_info_id = this->create_expr_info();
		if constexpr(IS_COMPTIME){
			this->add_instruction(
				SymbolProc::Instruction::ComptimeIdent(this->source.getASTBuffer().getIdent(node), new_expr_info_id)
			);
		}else{
			this->add_instruction(
				SymbolProc::Instruction::Ident(this->source.getASTBuffer().getIdent(node), new_expr_info_id)
			);
		}
		return new_expr_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_intrinsic(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		const SymbolProc::ExprInfoID new_expr_info_id = this->create_expr_info();
		this->add_instruction(
			SymbolProc::Instruction::Intrinsic(this->source.getASTBuffer().getIntrinsic(node), new_expr_info_id)
		);
		return new_expr_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_literal(const Token::ID& literal) -> evo::Result<SymbolProc::ExprInfoID> {
		const SymbolProc::ExprInfoID new_expr_info_id = this->create_expr_info();
		this->add_instruction(SymbolProc::Instruction::Literal(literal, new_expr_info_id));
		return new_expr_info_id;
	}

	auto SymbolProcBuilder::analyze_expr_uninit(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, node, "Building symbol proc of uninit is unimplemented"
		);
		return evo::resultError;
	}

	auto SymbolProcBuilder::analyze_expr_zeroinit(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, node, "Building symbol proc of zeroinit is unimplemented"
		);
		return evo::resultError;
	}

	auto SymbolProcBuilder::analyze_expr_this(const AST::Node& node) -> evo::Result<SymbolProc::ExprInfoID> {
		this->emit_error(
			Diagnostic::Code::MiscUnimplementedFeature, node, "Building symbol proc of this is unimplemented"
		);
		return evo::resultError;
	}


}