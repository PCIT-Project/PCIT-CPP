////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./ASGToPIR.h"

#include "../include/Context.h"
#include "../include/Source.h"
#include "../include/SourceManager.h"
#include "../include/get_source_location.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{

	
	auto ASGToPIR::lower() -> void {
		// lower variables
		for(const Source::ID& source_id : this->context.getSourceManager()){
			this->current_source = &this->context.getSourceManager()[source_id];

			for(const GlobalScope::Var& global_var : this->current_source->getGlobalScope().getVars()){
				this->lower_global_var(global_var.asg_var);
			}
		}

		// lower func decls
		for(const Source::ID& source_id : this->context.getSourceManager()){
			this->current_source = &this->context.getSourceManager()[source_id];

			for(const ASG::Func::ID func_id : this->current_source->getASGBuffer().getFuncs()){
				this->lower_func_decl(func_id);
			}
		}

		// lower func bodies
		for(const Source::ID& source_id : this->context.getSourceManager()){
			this->current_source = &this->context.getSourceManager()[source_id];

			for(const ASG::Func::ID func_id : this->current_source->getASGBuffer().getFuncs()){
				this->lower_func_body(func_id);
			}
		}

		this->current_source = nullptr;
	}


	auto ASGToPIR::addRuntime() -> void {
		const FuncInfo& entry_func_info = this->get_func_info(*this->context.getEntry());

		const pir::Type int_type = this->module.createIntegerType(
			this->context.getConfig().os == core::OS::Windows ? 32 : 64
		);

		const pir::Function::ID main_func_id = module.createFunction(
			"main", {}, pir::CallingConvention::Fast, pir::Linkage::Internal, int_type
		);

		this->agent.setTargetFunction(main_func_id);

		const pir::BasicBlock::ID begin_block = this->agent.createBasicBlock("begin");
		this->agent.setTargetBasicBlock(begin_block);

		const pir::Expr call_to_entry = this->agent.createCall(entry_func_info.func, {}, "");
		this->agent.createRet(this->agent.createZExt(call_to_entry, int_type));
	}



	auto ASGToPIR::lower_global_var(const ASG::Var::ID& var_id) -> void {
		const ASG::Var& asg_var = this->current_source->getASGBuffer().getVar(var_id);
		if(asg_var.kind == AST::VarDecl::Kind::Def){ return; } // make sure not to emit def variables

		const pir::Type var_type = this->get_type(*asg_var.typeID);

		const pir::GlobalVar::ID global_var = this->module.createGlobalVar(
			this->mangle_name(asg_var),
			var_type,
			pir::Linkage::Private,
			this->get_global_var_value(var_type, asg_var.expr),
			asg_var.isConst
		);

		this->var_infos.emplace(ASG::Var::LinkID(this->current_source->getID(), var_id), VarInfo(global_var));
	}



	auto ASGToPIR::lower_func_decl(const ASG::Func::ID& func_id) -> void {
		const ASG::Func& func = this->current_source->getASGBuffer().getFunc(func_id);
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.baseTypeID.funcID());

		auto params = evo::SmallVector<pir::Parameter>();
		params.reserve(func_type.params.size());
		for(const BaseType::Function::Param& param : func_type.params){
			std::string param_name = [&](){
				if(param.ident.is<Token::ID>()){
					return std::string(this->current_source->getTokenBuffer()[param.ident.as<Token::ID>()].getString());
				}else{
					return std::string(strings::toStringView(param.ident.as<strings::StringCode>()));
				}
			}();

			if(param.optimizeWithCopy){
				params.emplace_back(std::move(param_name), this->get_type(param.typeID));
			}else{
				params.emplace_back(std::move(param_name), this->module.createPtrType());
			}
		}

		if(func_type.hasNamedReturns()){
			for(const BaseType::Function::ReturnParam& return_param : func_type.returnParams){
				params.emplace_back(
					std::format("RET.{}", this->current_source->getTokenBuffer()[*return_param.ident].getString()),
					this->module.createPtrType()
				);
			}
		}

		const pir::Type return_type = [&](){
			if(func_type.hasNamedReturns()){
				return this->module.createVoidType();
			}else{
				return this->get_type(func_type.returnParams.front().typeID);
			}
		}();

		std::string mangled_name = this->mangle_name(func);

		const pir::Function::ID created_func_id = module.createFunction(
			std::string(mangled_name),
			std::move(params),
			pir::CallingConvention::Fast,
			pir::Linkage::Private,
			return_type
		);

		const auto asg_func_link_id = ASG::Func::LinkID(this->current_source->getID(), func_id);
		this->func_infos.emplace(asg_func_link_id, FuncInfo(created_func_id, std::move(mangled_name)));

		this->agent.setTargetFunction(created_func_id);


		if(func_type.hasNamedReturns()){
			const unsigned first_return_param_index = unsigned(func_type.params.size());
			unsigned i = first_return_param_index;
			for(const BaseType::Function::ReturnParam& ret_param : func_type.returnParams){
				const unsigned return_i = i - first_return_param_index;

				this->return_param_infos.emplace(
					ASG::ReturnParam::LinkID(asg_func_link_id, func.returnParams[return_i]),
					ReturnParamInfo(
						pir::Agent::createParamExpr(i), this->get_type(ret_param.typeID.typeID())/*, return_i*/
					)
				);

				i += 1;
			}			
		}

		if(func_type.params.empty()){
			this->agent.createBasicBlock("begin");

		}else{
			const pir::BasicBlock::ID setup_block = this->agent.createBasicBlock("setup");
			const pir::BasicBlock::ID begin_block = this->agent.createBasicBlock("begin");

			this->agent.setTargetBasicBlock(setup_block);

			for(uint32_t i = 0; const BaseType::Function::Param& param : func_type.params){
				const std::string param_name = [&](){
					if(param.ident.is<Token::ID>()){
						return std::string(
							this->current_source->getTokenBuffer()[param.ident.as<Token::ID>()].getString()
						);
					}else{
						return std::string(strings::toStringView(param.ident.as<strings::StringCode>()));
					}
				}();

				const pir::Type param_type = this->get_type(param.typeID);
				const pir::Type param_alloca_type = param.optimizeWithCopy ? param_type : this->module.createPtrType();
				const pir::Expr param_alloca = this->agent.createAlloca(
					param_alloca_type, this->stmt_name("{}.alloca", param_name)
				);

				this->param_infos.emplace(
					ASG::Param::LinkID(asg_func_link_id, func.params[i]), ParamInfo(param_alloca, param_type, i)
				);

				this->agent.createStore(param_alloca, this->agent.createParamExpr(i), false, pir::AtomicOrdering::None);

				i += 1;
			}

			this->agent.createBranch(begin_block);
		}
	}


	auto ASGToPIR::lower_func_body(const ASG::Func::ID& func_id) -> void {
		const ASG::Func& asg_func = this->current_source->getASGBuffer().getFunc(func_id);
		this->current_func = &asg_func;


		auto link_id = ASG::Func::LinkID(this->current_source->getID(), func_id);
		this->current_func_link_id = link_id;

		this->agent.setTargetFunction(this->get_current_func_info().func);
		this->agent.setTargetBasicBlockAtEnd();

		for(const ASG::Stmt& stmt : asg_func.stmts){
			this->lower_stmt(stmt);
		}

		if(asg_func.isTerminated == false){
			const BaseType::Function& func_type = 
				this->context.getTypeManager().getFunction(asg_func.baseTypeID.funcID());


			if(func_type.returnsVoid()){
				this->agent.createRet();
			}else{
				this->agent.createUnreachable();
			}
		}

		this->current_func_link_id = std::nullopt;
		this->current_func = nullptr;
	}


	auto ASGToPIR::lower_stmt(const ASG::Stmt& stmt) -> void {
		const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();

		switch(stmt.kind()){
			break; case ASG::Stmt::Kind::Var:         this->lower_var(stmt.varID());
			break; case ASG::Stmt::Kind::FuncCall:    this->lower_func_call(asg_buffer.getFuncCall(stmt.funcCallID()));
			break; case ASG::Stmt::Kind::Assign:      this->lower_assign(asg_buffer.getAssign(stmt.assignID()));
			break; case ASG::Stmt::Kind::MultiAssign:
				this->lower_multi_assign(asg_buffer.getMultiAssign(stmt.multiAssignID()));
			break; case ASG::Stmt::Kind::Return:      this->lower_return(asg_buffer.getReturn(stmt.returnID()));
			break; case ASG::Stmt::Kind::Unreachable: this->agent.createUnreachable();
			break; case ASG::Stmt::Kind::Conditional:
				this->lower_conditional(asg_buffer.getConditional(stmt.conditionalID()));
			break; case ASG::Stmt::Kind::While:       this->lower_while(asg_buffer.getWhile(stmt.whileID()));
		}
	}


	auto ASGToPIR::lower_var(const ASG::Var::ID& var_id) -> void {
		const ASG::Var& asg_var = this->current_source->getASGBuffer().getVar(var_id);
		if(asg_var.kind == AST::VarDecl::Kind::Def){ return; } // make sure not to emit def variables

		const pir::Expr var_alloca = this->agent.createAlloca(
			this->get_type(*asg_var.typeID),
			this->stmt_name("{}.alloca", this->current_source->getTokenBuffer()[asg_var.ident].getString())
		);

		switch(asg_var.expr.kind()){
			case ASG::Expr::Kind::Uninit: {
				// do nothing
			} break;

			case ASG::Expr::Kind::Zeroinit: {
				// TODO: 
				evo::log::fatal("UNIMPLEMENTED (var value of [zeroinit])");
				evo::breakpoint();
				// this->builder.createMemSetInline(
				// 	var_alloca.asValue(),
				// 	this->builder.getValueI8(0).asValue(),
				// 	this->get_value_size(this->context.getTypeManager().sizeOf(*asg_var.typeID)).asValue(),
				// 	false
				// );
			} break;

			default: {
				this->agent.createStore(
					var_alloca, this->get_value<false>(asg_var.expr), false, pir::AtomicOrdering::None
				);
			} break;
		}

		this->var_infos.emplace(ASG::Var::LinkID(this->current_source->getID(), var_id), VarInfo(var_alloca));
	}


	auto ASGToPIR::lower_func_call(const ASG::FuncCall& func_call) -> void {
		if(func_call.target.is<ASG::Func::LinkID>() == false){
			this->lower_intrinsic_call(func_call);
			return;
		}

		const ASG::Func::LinkID& func_link_id = func_call.target.as<ASG::Func::LinkID>();
		const FuncInfo& func_info = this->get_func_info(func_link_id);

		const Source& linked_source = this->context.getSourceManager()[func_link_id.sourceID()];
		const ASG::Func& asg_func = linked_source.getASGBuffer().getFunc(func_link_id.funcID());
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(asg_func.baseTypeID.funcID());


		auto args = evo::SmallVector<pir::Expr>();
		args.reserve(func_call.args.size());
		for(size_t i = 0; const ASG::Expr& arg : func_call.args){
			const bool optimize_with_copy = func_type.params[i].optimizeWithCopy;
			if(optimize_with_copy){
				args.emplace_back(this->get_value<false>(arg));
			}else{
				args.emplace_back(this->get_value<true>(arg));
			}

			i += 1;
		}

		this->agent.createCallVoid(func_info.func, args);
	}


	auto ASGToPIR::lower_intrinsic_call(const ASG::FuncCall& func_call) -> void {
		auto args = evo::SmallVector<pir::Expr>();
		args.reserve(func_call.args.size());
		for(size_t i = 0; const ASG::Expr& arg : func_call.args){
			args.emplace_back(this->get_value<false>(arg));

			i += 1;
		}


		if(func_call.target.is<Intrinsic::Kind>()){
			switch(func_call.target.as<Intrinsic::Kind>()){
				case Intrinsic::Kind::Breakpoint: {
					this->agent.createBreakpoint();
				} break;

				case Intrinsic::Kind::_printHelloWorld: {
					if(this->globals.hello_world_str.has_value() == false){
						const pir::GlobalVar::String::ID str = 
							this->module.createGlobalString("Hello World, I'm Panther!\n");

						this->globals.hello_world_str = this->module.createGlobalVar(
							"PTHR.print_hello_world_str",
							this->module.getGlobalString(str).type,
							pir::Linkage::Private,
							str,
							true
						);

					}

					if(this->config.isJIT){
						this->link_jit_std_out_if_needed();

						this->agent.createCallVoid(*this->jit_links.std_out, evo::SmallVector<pcit::pir::Expr>{
							this->agent.createGlobalValue(*this->globals.hello_world_str)
						});

					}else{
						this->link_libc_puts_if_needed();

						this->agent.createCallVoid(
							*this->libc_links.puts,
							evo::SmallVector<pcit::pir::Expr>{
								this->agent.createGlobalValue(*this->globals.hello_world_str)
							}
						);
					}
				} break;

				case Intrinsic::Kind::_max_: {
					evo::debugFatalBreak("Intrinsic::Kind::_max_ is not an actual intrinsic");
				} break;
			}
			
		}else{
			evo::debugAssert(
				func_call.target.is<ASG::TemplatedIntrinsicInstantiation::ID>(),
				"cannot lower intrinsic for FuncLink::ID"
			);

			const ASG::TemplatedIntrinsicInstantiation& instantiation = 
				this->current_source->getASGBuffer().getTemplatedIntrinsicInstantiation(
					func_call.target.as<ASG::TemplatedIntrinsicInstantiation::ID>()
				);

			switch(instantiation.kind){
				case TemplatedIntrinsic::Kind::IsSameType: 
				case TemplatedIntrinsic::Kind::IsTriviallyCopyable:
				case TemplatedIntrinsic::Kind::IsTriviallyDestructable: 
				case TemplatedIntrinsic::Kind::IsPrimitive: 
				case TemplatedIntrinsic::Kind::IsBuiltin:
				case TemplatedIntrinsic::Kind::IsIntegral:
				case TemplatedIntrinsic::Kind::IsFloatingPoint:
				case TemplatedIntrinsic::Kind::SizeOf:
				case TemplatedIntrinsic::Kind::GetTypeID:
				case TemplatedIntrinsic::Kind::BitCast:
				case TemplatedIntrinsic::Kind::Trunc:
				case TemplatedIntrinsic::Kind::FTrunc:
				case TemplatedIntrinsic::Kind::SExt:
				case TemplatedIntrinsic::Kind::ZExt:
				case TemplatedIntrinsic::Kind::FExt:
				case TemplatedIntrinsic::Kind::IToF:
				case TemplatedIntrinsic::Kind::UIToF:
				case TemplatedIntrinsic::Kind::FToI:
				case TemplatedIntrinsic::Kind::FToUI:
				case TemplatedIntrinsic::Kind::Add:
				case TemplatedIntrinsic::Kind::AddWrap:
				case TemplatedIntrinsic::Kind::AddSat:
				case TemplatedIntrinsic::Kind::FAdd:
				case TemplatedIntrinsic::Kind::Sub:
				case TemplatedIntrinsic::Kind::SubWrap:
				case TemplatedIntrinsic::Kind::SubSat:
				case TemplatedIntrinsic::Kind::FSub:
				case TemplatedIntrinsic::Kind::Mul:
				case TemplatedIntrinsic::Kind::MulWrap:
				case TemplatedIntrinsic::Kind::MulSat:
				case TemplatedIntrinsic::Kind::FMul:
				case TemplatedIntrinsic::Kind::Div:
				case TemplatedIntrinsic::Kind::FDiv:
				case TemplatedIntrinsic::Kind::Rem:
				case TemplatedIntrinsic::Kind::Eq:
				case TemplatedIntrinsic::Kind::NEq:
				case TemplatedIntrinsic::Kind::LT:
				case TemplatedIntrinsic::Kind::LTE:
				case TemplatedIntrinsic::Kind::GT:
				case TemplatedIntrinsic::Kind::GTE:
				case TemplatedIntrinsic::Kind::And:
				case TemplatedIntrinsic::Kind::Or:
				case TemplatedIntrinsic::Kind::Xor:
				case TemplatedIntrinsic::Kind::SHL:
				case TemplatedIntrinsic::Kind::SHLSat:
				case TemplatedIntrinsic::Kind::SHR: {
					evo::debugFatalBreak("This intrinsic returns a value");
				} break;

				case TemplatedIntrinsic::Kind::_max_: {
					evo::debugFatalBreak("Intrinsic::Kind::_max_ is not an actual intrinsic");
				} break;
			}

			evo::debugFatalBreak("Unknown or unsupported templated intrinsic kind");
		}
	}


	auto ASGToPIR::lower_assign(const ASG::Assign& assign) -> void {
		const pir::Expr lhs = this->get_value<true>(assign.lhs);
		const pir::Expr rhs = this->get_value<false>(assign.rhs);

		this->agent.createStore(lhs, rhs, false, pir::AtomicOrdering::None);
	}


	auto ASGToPIR::lower_multi_assign(const ASG::MultiAssign& multi_assign) -> void {
		auto assign_targets = evo::SmallVector<std::optional<pir::Expr>>();
		assign_targets.reserve(multi_assign.targets.size());

		for(const std::optional<ASG::Expr>& target : multi_assign.targets){
			if(target.has_value()){
				assign_targets.emplace_back(this->get_value<true>(*target));
			}else{
				assign_targets.emplace_back();
			}
		}

		const evo::SmallVector<pir::Expr> values = this->lower_returning_func_call<false>(
			this->current_source->getASGBuffer().getFuncCall(multi_assign.value.funcCallID())
		);

		for(size_t i = 0; const std::optional<pir::Expr>& assign_target : assign_targets){
			EVO_DEFER([&](){ i += 1; });

			if(assign_target.has_value() == false){ continue; }

			this->agent.createStore(assign_target.value(), values[i], false, pir::AtomicOrdering::None);
		}
	}


	auto ASGToPIR::lower_return(const ASG::Return& return_stmt) -> void {
		if(return_stmt.value.has_value() == false){
			this->agent.createRet();
			return;
		}

		this->agent.createRet(this->get_value<false>(*return_stmt.value));
	}


	auto ASGToPIR::lower_conditional(const ASG::Conditional& conditional_stmt) -> void {
		const pir::BasicBlock::ID then_block = this->agent.createBasicBlock("IF.THEN");
		auto end_block = std::optional<pir::BasicBlock::ID>();

		const pir::Expr cond_value = this->get_value<false>(conditional_stmt.cond);

		if(conditional_stmt.elseStmts.empty()){
			end_block = this->agent.createBasicBlock("IF.END");

			this->agent.createCondBranch(cond_value, then_block, *end_block);

			this->agent.setTargetBasicBlock(then_block);
			for(const ASG::Stmt& stmt : conditional_stmt.thenStmts){
				this->lower_stmt(stmt);
			}

			if(conditional_stmt.thenStmts.isTerminated() == false){
				// this->agent.setTargetBasicBlock(then_block);
				this->agent.createBranch(*end_block);
			}
		}else{
			const pir::BasicBlock::ID else_block = this->agent.createBasicBlock("IF.ELSE");

			this->agent.createCondBranch(cond_value, then_block, else_block);

			// then block
			this->agent.setTargetBasicBlock(then_block);
			for(const ASG::Stmt& stmt : conditional_stmt.thenStmts){
				this->lower_stmt(stmt);
			}

			// required because stuff in the then block might add basic blocks
			pir::BasicBlock& then_block_end = this->agent.getTargetBasicBlock();

			// else block
			this->agent.setTargetBasicBlock(else_block);
			for(const ASG::Stmt& stmt : conditional_stmt.elseStmts){
				this->lower_stmt(stmt);
			}

			// end block
			const bool then_terminated = conditional_stmt.thenStmts.isTerminated();
			const bool else_terminated = conditional_stmt.elseStmts.isTerminated();

			if(else_terminated && then_terminated){ return; }

			end_block = this->agent.createBasicBlock("IF.END");

			if(!else_terminated){
				this->agent.createBranch(*end_block);
			}

			if(!then_terminated){
				this->agent.setTargetBasicBlock(then_block_end);
				this->agent.createBranch(*end_block);
			}
		}


		this->agent.setTargetBasicBlock(*end_block);
	}


	auto ASGToPIR::lower_while(const ASG::While& while_loop) -> void {
		const pir::BasicBlock::ID cond_block = this->agent.createBasicBlock("WHILE.COND");
		this->agent.createBranch(cond_block);
		const pir::BasicBlock::ID body_block = this->agent.createBasicBlock("WHILE.BODY");
		const pir::BasicBlock::ID end_block = this->agent.createBasicBlock("WHILE.END");


		this->agent.setTargetBasicBlock(cond_block);
		const pir::Expr cond_value = this->get_value<false>(while_loop.cond);
		this->agent.createCondBranch(cond_value, body_block, end_block);

		this->agent.setTargetBasicBlock(body_block);
		for(const ASG::Stmt& stmt : while_loop.block){
			this->lower_stmt(stmt);
		}
		this->agent.createBranch(cond_block);

		this->agent.setTargetBasicBlock(end_block);
	}



	auto ASGToPIR::get_type(const TypeInfo::VoidableID& type_info_voidable_id) const -> pir::Type {
		if(type_info_voidable_id.isVoid()) [[unlikely]] {
			return this->module.createVoidType();
		}else{
			return this->get_type(this->context.getTypeManager().getTypeInfo(type_info_voidable_id.typeID()));
		}
	}

	auto ASGToPIR::get_type(const TypeInfo::ID& type_info_id) const -> pir::Type {
		return this->get_type(this->context.getTypeManager().getTypeInfo(type_info_id));
	}

	auto ASGToPIR::get_type(const TypeInfo& type_info) const -> pir::Type {
		const evo::ArrayProxy<AST::Type::Qualifier> type_qualifiers = type_info.qualifiers();
		if(type_qualifiers.empty() == false){
			if(type_qualifiers.back().isPtr){
				return this->module.createPtrType();
			}else{
				evo::fatalBreak("Optional is unsupported");	
			}
		}

		switch(type_info.baseTypeID().kind()){
			case BaseType::Kind::Primitive: {
				const BaseType::Primitive::ID primitive_id = type_info.baseTypeID().primitiveID();
				const BaseType::Primitive& primitive = this->context.getTypeManager().getPrimitive(primitive_id);
				return this->get_type(primitive);
			} break;

			case BaseType::Kind::Function: {
				return this->module.createPtrType();
			} break;

			case BaseType::Kind::Alias: {
				const BaseType::Alias::ID alias_id = type_info.baseTypeID().aliasID();
				const BaseType::Alias& alias = this->context.getTypeManager().getAlias(alias_id);
				return this->get_type(alias.aliasedType);
			} break;

			case BaseType::Kind::Dummy: evo::debugFatalBreak("Cannot get a dummy type");
		}

		evo::debugFatalBreak("Unknown or unsupported primitive kind");
	}


	auto ASGToPIR::get_type(const BaseType::Primitive& primitive) const -> pir::Type {
		switch(primitive.kind()){
			case Token::Kind::TypeInt: case Token::Kind::TypeUInt: {
				return this->module.createIntegerType(
					uint32_t(this->context.getTypeManager().sizeOfGeneralRegister() * 8)
				);
			} break;

			case Token::Kind::TypeISize: case Token::Kind::TypeUSize: {
				return this->module.createIntegerType(uint32_t(this->context.getTypeManager().sizeOfPtr() * 8));
			} break;

			case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N: {
				return this->module.createIntegerType(unsigned(primitive.bitWidth()));
			} break;

			case Token::Kind::TypeF16: return this->module.createFloatType(16);
			case Token::Kind::TypeBF16: return this->module.createBFloatType();
			case Token::Kind::TypeF32: return this->module.createFloatType(32);
			case Token::Kind::TypeF64: return this->module.createFloatType(64);
			case Token::Kind::TypeF80: return this->module.createFloatType(80);
			case Token::Kind::TypeF128: return this->module.createFloatType(128);
			case Token::Kind::TypeByte: return this->module.createIntegerType(8);
			case Token::Kind::TypeBool: return this->module.createBoolType();
			case Token::Kind::TypeChar: return this->module.createIntegerType(8);
			case Token::Kind::TypeRawPtr: return this->module.createPtrType();
			case Token::Kind::TypeTypeID: return this->module.createIntegerType(32);

			case Token::Kind::TypeCShort: case Token::Kind::TypeCUShort: 
				return this->module.createIntegerType(16);

			case Token::Kind::TypeCInt: case Token::Kind::TypeCUInt:  {
				if(this->context.getTypeManager().getOS() == core::OS::Windows){
					return this->module.createIntegerType(32);
				}else{
					return this->module.createIntegerType(64);
				}
			} break;

			case Token::Kind::TypeCLong: case Token::Kind::TypeCULong: 
				return this->module.createIntegerType(32);

			case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong:
				return this->module.createIntegerType(64);

			case Token::Kind::TypeCLongDouble: {
				if(this->context.getTypeManager().getOS() == core::OS::Windows){
					return this->module.createFloatType(64);
				}else{
					return this->module.createFloatType(80);
				}
			} break;

			default: evo::debugFatalBreak(
				"Unknown or unsupported primitive-type: {}", evo::to_underlying(primitive.kind())
			);
		}
	}



	template<bool GET_POINTER_TO_VALUE>
	auto ASGToPIR::get_value(const ASG::Expr& expr) -> pir::Expr {
		const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();

		switch(expr.kind()){
			case ASG::Expr::Kind::Uninit: {
				evo::debugFatalBreak("Cannot get value of [uninit]");
			} break;

			case ASG::Expr::Kind::Zeroinit: {
				evo::debugFatalBreak("Cannot get value of [zeroinit]");
			} break;

			case ASG::Expr::Kind::LiteralInt: {
				const ASG::LiteralInt& literal_int = asg_buffer.getLiteralInt(expr.literalIntID());

				const pir::Type int_type = this->get_type(*literal_int.typeID);
				const pir::Expr pir_num = this->agent.createNumber(int_type, literal_int.value);

				if constexpr(!GET_POINTER_TO_VALUE){
					return pir_num;
				}else{
					const pir::Expr alloca = this->agent.createAlloca(int_type);
					this->agent.createStore(alloca, pir_num, false, pir::AtomicOrdering::None);
					return alloca;
				}
			} break;

			case ASG::Expr::Kind::LiteralFloat: {
				const ASG::LiteralFloat& literal_float = asg_buffer.getLiteralFloat(expr.literalFloatID());

				const pir::Type float_type = this->get_type(*literal_float.typeID);
				const pir::Expr pir_num = this->agent.createNumber(float_type, literal_float.value);

				if constexpr(!GET_POINTER_TO_VALUE){
					return pir_num;
				}else{
					const pir::Expr alloca = this->agent.createAlloca(float_type);
					this->agent.createStore(alloca, pir_num, false, pir::AtomicOrdering::None);
					return alloca;
				}
			} break;

			case ASG::Expr::Kind::LiteralBool: {
				const ASG::LiteralBool& literal_bool = asg_buffer.getLiteralBool(expr.literalBoolID());

				const pir::Expr pir_bool = this->agent.createBoolean(literal_bool.value);

				if constexpr(!GET_POINTER_TO_VALUE){
					return pir_bool;
				}else{
					const pir::Expr alloca = this->agent.createAlloca(this->module.createBoolType());
					this->agent.createStore(alloca, pir_bool, false, pir::AtomicOrdering::None);
					return alloca;
				}
			} break;

			case ASG::Expr::Kind::LiteralChar: {
				const ASG::LiteralChar& literal_char = asg_buffer.getLiteralChar(expr.literalCharID());

				const pir::Type char_type = this->module.createIntegerType(8);
				const pir::Expr pir_char = this->agent.createNumber(
					char_type, core::GenericInt::create<char>(literal_char.value)
				);

				if constexpr(!GET_POINTER_TO_VALUE){
					return pir_char;
				}else{
					const pir::Expr alloca = this->agent.createAlloca(char_type);
					this->agent.createStore(alloca, pir_char, false, pir::AtomicOrdering::None);
					return alloca;
				}
			} break;

			case ASG::Expr::Kind::Intrinsic: {
				evo::debugFatalBreak("Cannot get value of intrinsic function");
			} break;

			case ASG::Expr::Kind::TemplatedIntrinsicInstantiation: {
				evo::debugFatalBreak("Cannot get value of template instantiated intrinsic function");
			} break;

			case ASG::Expr::Kind::Copy: {
				const ASG::Expr& copy_expr = asg_buffer.getCopy(expr.copyID());
				return this->get_value<GET_POINTER_TO_VALUE>(copy_expr);
			} break;

			case ASG::Expr::Kind::Move: {
				const ASG::Expr& move_expr = asg_buffer.getMove(expr.moveID());
				return this->get_value<GET_POINTER_TO_VALUE>(move_expr);
			} break;

			case ASG::Expr::Kind::FuncCall: {
				const ASG::FuncCall& func_call = asg_buffer.getFuncCall(expr.funcCallID());
				return this->lower_returning_func_call<GET_POINTER_TO_VALUE>(func_call).front();
			} break;

			case ASG::Expr::Kind::AddrOf: {
				const ASG::Expr& addr_of_expr = asg_buffer.getAddrOf(expr.addrOfID());
				const pir::Expr address = this->get_value<true>(addr_of_expr);

				if constexpr(!GET_POINTER_TO_VALUE){
					return address;

				}else{
					const pir::Expr alloca = this->agent.createAlloca(this->module.createPtrType());
					this->agent.createStore(alloca, address, false, pir::AtomicOrdering::None);
					return alloca;
				}

			} break;

			case ASG::Expr::Kind::Deref: {
				const ASG::Deref& deref_expr = asg_buffer.getDeref(expr.derefID());
				const pir::Expr value = this->get_value<false>(deref_expr.expr);


				if constexpr(GET_POINTER_TO_VALUE){
					return value;

				}else{
					return this->agent.createLoad(
						value,
						this->get_type(deref_expr.typeID),
						false,
						pir::AtomicOrdering::None,
						this->stmt_name("DEREF")
					);
				}
			} break;

			case ASG::Expr::Kind::Var: {
				const VarInfo& var_info = this->get_var_info(expr.varLinkID());

				if constexpr(GET_POINTER_TO_VALUE){
					return var_info.value.visit([&](auto& value) -> pir::Expr {
						using ValueT = std::decay_t<decltype(value)>;

						if constexpr(std::is_same<ValueT, pir::Expr>()){
							return value;

						}else if constexpr(std::is_same<ValueT, pir::GlobalVar::ID>()){
							return this->agent.createGlobalValue(value);

						}else{
							static_assert(false, "Unknown or unsupported var kind");
						}
					});

				}else{
					return var_info.value.visit([&](auto& value) -> pir::Expr {
						using ValueT = std::decay_t<decltype(value)>;

						if constexpr(std::is_same<ValueT, pir::Expr>()){
							return this->agent.createLoad(
								value,
								this->agent.getAlloca(value).type,
								false,
								pir::AtomicOrdering::None,
								this->stmt_name("VAR.load")
							);

						}else if constexpr(std::is_same<ValueT, pir::GlobalVar::ID>()){
							const pir::Expr global_value = this->agent.createGlobalValue(value);
							return this->agent.createLoad(
								global_value,
								this->agent.getExprType(global_value),
								false,
								pir::AtomicOrdering::None,
								this->stmt_name("GLOBAL.load")
							);

						}else{
							static_assert(false, "Unknown or unsupported var kind");
						}
					});
				}
			} break;

			case ASG::Expr::Kind::Func: {
				// TODO: 
				evo::log::fatal("UNIMPLEMENTED (ASG::Expr::Kind::Func)");
				evo::breakpoint();
			} break;

			case ASG::Expr::Kind::Param: {
				const ParamInfo& param_info = this->get_param_info(expr.paramLinkID());

				const BaseType::Function& func_type = this->context.getTypeManager().getFunction(
					this->current_func->baseTypeID.funcID()
				);

				const bool optimize_with_copy = func_type.params[param_info.index].optimizeWithCopy;

				if(optimize_with_copy){
					if constexpr(GET_POINTER_TO_VALUE){
						return param_info.alloca;
					}else{
						return this->agent.createLoad(
							param_info.alloca,
							param_info.type,
							false,
							pir::AtomicOrdering::None,
							this->stmt_name("PARAM.load")
						);
					}

				}else{
					const pir::Expr load = this->agent.createLoad(
						param_info.alloca,
						this->module.createPtrType(),
						false,
						pir::AtomicOrdering::None,
						this->stmt_name("PARAM.ptr_lookup")
					);

					if constexpr(GET_POINTER_TO_VALUE){
						return load;
					}else{
						return this->agent.createLoad(
							load, param_info.type, false, pir::AtomicOrdering::None, this->stmt_name("PARAM.load")
						);
					}
				}
			} break;

			case ASG::Expr::Kind::ReturnParam: {
				const ReturnParamInfo& ret_param_info = this->get_return_param_info(expr.returnParamLinkID());

				if constexpr(GET_POINTER_TO_VALUE){
					return ret_param_info.param;
				}else{
					return this->agent.createLoad(
						ret_param_info.param,
						ret_param_info.type,
						false,
						pir::AtomicOrdering::None,
						this->stmt_name("RET_PARAM.load")
					);
				}
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported expr kind");
	}


	auto ASGToPIR::get_global_var_value(const pir::Type& type, const ASG::Expr& expr) const -> pir::GlobalVar::Value {
		switch(expr.kind()){
			case ASG::Expr::Kind::Uninit: return pir::GlobalVar::Uninit();
			case ASG::Expr::Kind::Zeroinit: return pir::GlobalVar::Zeroinit();

			case ASG::Expr::Kind::LiteralInt: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralInt& literal_int = asg_buffer.getLiteralInt(expr.literalIntID());

				return this->agent.createNumber(type, literal_int.value);
			} break;

			case ASG::Expr::Kind::LiteralFloat: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralFloat& literal_float = asg_buffer.getLiteralFloat(expr.literalFloatID());

				return this->agent.createNumber(type, literal_float.value);
			} break;

			case ASG::Expr::Kind::LiteralBool: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralBool& literal_bool = asg_buffer.getLiteralBool(expr.literalBoolID());

				return this->agent.createBoolean(literal_bool.value);
			} break;

			case ASG::Expr::Kind::LiteralChar: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralChar& literal_char = asg_buffer.getLiteralChar(expr.literalCharID());

				return this->agent.createNumber(
					this->module.createIntegerType(8), core::GenericInt::create<char>(literal_char.value)
				);
			} break;

			case ASG::Expr::Kind::Intrinsic:     case ASG::Expr::Kind::TemplatedIntrinsicInstantiation:
			case ASG::Expr::Kind::Copy:          case ASG::Expr::Kind::Move:
			case ASG::Expr::Kind::FuncCall:      case ASG::Expr::Kind::AddrOf:
			case ASG::Expr::Kind::Deref:         case ASG::Expr::Kind::Var:
			case ASG::Expr::Kind::Func:          case ASG::Expr::Kind::Param:
			case ASG::Expr::Kind::ReturnParam: {
				evo::debugFatalBreak("Invalid global var value");
			}
		}

		evo::debugFatalBreak("Unknown or unsupported expr kind");
	}


	template<bool GET_POINTER_TO_VALUE>
	auto ASGToPIR::lower_returning_func_call(const ASG::FuncCall& func_call) -> evo::SmallVector<pir::Expr> {
		if(func_call.target.is<ASG::Func::LinkID>() == false){
			return this->lower_returning_intrinsic_call<GET_POINTER_TO_VALUE>(func_call);
		}

		const ASG::Func::LinkID& func_link_id = func_call.target.as<ASG::Func::LinkID>();
		const FuncInfo& func_info = this->get_func_info(func_link_id);

		const Source& linked_source = this->context.getSourceManager()[func_link_id.sourceID()];
		const ASG::Func& asg_func = linked_source.getASGBuffer().getFunc(func_link_id.funcID());
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(asg_func.baseTypeID.funcID());


		auto args = evo::SmallVector<pir::Expr>();
		args.reserve(func_call.args.size());
		for(size_t i = 0; const ASG::Expr& arg : func_call.args){
			const bool optimize_with_copy = func_type.params[i].optimizeWithCopy;
			if(optimize_with_copy){
				args.emplace_back(this->get_value<false>(arg));
			}else{
				args.emplace_back(this->get_value<true>(arg));
			}

			i += 1;
		}


		auto return_values = evo::SmallVector<pir::Expr>();
		if(func_type.hasNamedReturns()){
			for(const BaseType::Function::ReturnParam& return_param : func_type.returnParams){
				const pir::Expr alloca = this->agent.createAlloca(
					this->get_type(return_param.typeID),
					this->stmt_name(
						"RET_PARAM.{}.alloca",
						this->current_source->getTokenBuffer()[*return_param.ident].getString()
					)
				);

				args.emplace_back(alloca);
			}

			this->agent.createCallVoid(func_info.func, args);

			if constexpr(GET_POINTER_TO_VALUE){
				for(size_t i = func_type.params.size(); i < args.size(); i+=1){
					return_values.emplace_back(args[i]);
				}

				return return_values;
			}else{
				for(size_t i = func_type.params.size(); i < args.size(); i+=1){
					return_values.emplace_back(
						this->agent.createLoad(
							args[i],
							this->get_type(func_type.returnParams[i - func_type.params.size()].typeID),
							false,
							pir::AtomicOrdering::None
						)
					);
				}

				return return_values;
			}

		}else{
			const pir::Expr func_call_value = this->agent.createCall(func_info.func, args);
			if constexpr(GET_POINTER_TO_VALUE == false){
				return_values.emplace_back(func_call_value);
				return return_values;
			}else{
				const pir::Type return_type = this->get_type(func_type.returnParams[0].typeID);

				const pir::Expr alloca = this->agent.createAlloca(return_type);
				this->agent.createStore(alloca, func_call_value, false, pir::AtomicOrdering::None);

				return_values.emplace_back(alloca);
				return return_values;
			}
		}
	}


	template<bool GET_POINTER_TO_VALUE>
	auto ASGToPIR::lower_returning_intrinsic_call(const ASG::FuncCall& func_call) -> evo::SmallVector<pir::Expr> {
		auto args = evo::SmallVector<pir::Expr>();
		args.reserve(func_call.args.size());
		for(size_t i = 0; const ASG::Expr& arg : func_call.args){
			args.emplace_back(this->get_value<false>(arg));

			i += 1;
		}


		if(func_call.target.is<Intrinsic::Kind>()){
			switch(func_call.target.as<Intrinsic::Kind>()){
				case Intrinsic::Kind::Breakpoint: case Intrinsic::Kind::_printHelloWorld:  {
					evo::debugFatalBreak("This instrinsic does not return");
				} break;

				case Intrinsic::Kind::_max_: {
					evo::debugFatalBreak("Intrinsic::Kind::_max_ is not an actual intrinsic");
				} break;
			}

			evo::debugFatalBreak("Unknown or unsupported intrinsic kind");
			
		}else{
			evo::debugAssert(
				func_call.target.is<ASG::TemplatedIntrinsicInstantiation::ID>(),
				"cannot lower intrinsic for FuncLink::ID"
			);

			const ASG::TemplatedIntrinsicInstantiation& instantiation = 
				this->current_source->getASGBuffer().getTemplatedIntrinsicInstantiation(
					func_call.target.as<ASG::TemplatedIntrinsicInstantiation::ID>()
				);

			switch(instantiation.kind){
				case TemplatedIntrinsic::Kind::IsSameType: {
					return evo::SmallVector<pir::Expr>{
						this->agent.createBoolean(
							instantiation.templateArgs[0].as<TypeInfo::VoidableID>() == 
							instantiation.templateArgs[1].as<TypeInfo::VoidableID>()
						)
					};
				} break;

				case TemplatedIntrinsic::Kind::IsTriviallyCopyable: {
					return evo::SmallVector<pir::Expr>{
						this->agent.createBoolean(
							this->context.getTypeManager().isTriviallyCopyable(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
							)
						)
					};
				} break;

				case TemplatedIntrinsic::Kind::IsTriviallyDestructable: {
					return evo::SmallVector<pir::Expr>{
						this->agent.createBoolean(
							this->context.getTypeManager().isTriviallyDestructable(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
							)
						)
					};
				} break;

				case TemplatedIntrinsic::Kind::IsPrimitive: {
					return evo::SmallVector<pir::Expr>{
						this->agent.createBoolean(
							this->context.getTypeManager().isPrimitive(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
							)
						)
					};
				} break;

				case TemplatedIntrinsic::Kind::IsBuiltin: {
					return evo::SmallVector<pir::Expr>{
						this->agent.createBoolean(
							this->context.getTypeManager().isBuiltin(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
							)
						)
					};
				} break;

				case TemplatedIntrinsic::Kind::IsIntegral: {
					return evo::SmallVector<pir::Expr>{
						this->agent.createBoolean(
							this->context.getTypeManager().isIntegral(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>()
							)
						)
					};
				} break;

				case TemplatedIntrinsic::Kind::IsFloatingPoint: {
					return evo::SmallVector<pir::Expr>{
						this->agent.createBoolean(
							this->context.getTypeManager().isFloatingPoint(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>()
							)
						)
					};
				} break;

				case TemplatedIntrinsic::Kind::SizeOf: {
					const size_t ptr_bits = this->context.getTypeManager().sizeOfPtr() * 8;
					const size_t size_of_value = this->context.getTypeManager().sizeOf(
						instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
					);

					return evo::SmallVector<pir::Expr>{
						this->agent.createNumber(
							this->module.createIntegerType(uint32_t(ptr_bits)),
							core::GenericInt(uint32_t(ptr_bits), size_of_value, false)
						)
					};
				} break;

				case TemplatedIntrinsic::Kind::GetTypeID: {
					const pir::Type type_id_type = this->get_type(TypeManager::getTypeTypeID());
					const uint32_t type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID().get();

					return evo::SmallVector<pir::Expr>{
						this->agent.createNumber(
							this->module.createIntegerType(32), core::GenericInt(32, type_id, false)
						)
					};
				} break;

				case TemplatedIntrinsic::Kind::BitCast: {
					const TypeInfo::ID from_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const TypeInfo::ID to_type_id = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();

					if(from_type_id == to_type_id){ return args; }

					return evo::SmallVector<pir::Expr>{
						this->agent.createBitCast(args[0], this->get_type(to_type_id), this->stmt_name("BITCAST"))
					};
				} break;


				case TemplatedIntrinsic::Kind::Trunc: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<pir::Expr>{
						this->agent.createTrunc(args[0], this->get_type(to_type), this->stmt_name("TRUNC"))
					};
				} break;

				case TemplatedIntrinsic::Kind::FTrunc: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<pir::Expr>{
						this->agent.createFTrunc(args[0], this->get_type(to_type), this->stmt_name("FTRUNC"))
					};
				} break;

				case TemplatedIntrinsic::Kind::SExt: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<pir::Expr>{
						this->agent.createSExt(args[0], this->get_type(to_type), this->stmt_name("SEXT"))
					};
				} break;

				case TemplatedIntrinsic::Kind::ZExt: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<pir::Expr>{
						this->agent.createZExt(args[0], this->get_type(to_type), this->stmt_name("ZEXT"))
					};
				} break;

				case TemplatedIntrinsic::Kind::FExt: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<pir::Expr>{
						this->agent.createFExt(args[0], this->get_type(to_type), this->stmt_name("FEXT"))
					};
				} break;

				case TemplatedIntrinsic::Kind::IToF: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<pir::Expr>{
						this->agent.createIToF(args[0], this->get_type(to_type), this->stmt_name("ITOF"))
					};
				} break;

				case TemplatedIntrinsic::Kind::UIToF: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<pir::Expr>{
						this->agent.createUIToF(args[0], this->get_type(to_type), this->stmt_name("UITOF"))
					};
				} break;

				case TemplatedIntrinsic::Kind::FToI: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<pir::Expr>{
						this->agent.createFToI(args[0], this->get_type(to_type), this->stmt_name("FTOI"))
					};
				} break;

				case TemplatedIntrinsic::Kind::FToUI: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<pir::Expr>{
						this->agent.createFToUI(args[0], this->get_type(to_type), this->stmt_name("FTOUI"))
					};
				} break;


				///////////////////////////////////
				// addition

				case TemplatedIntrinsic::Kind::Add: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const bool may_wrap = instantiation.templateArgs[1].as<bool>();
					if(may_wrap || this->config.checkedMath == false){
						return evo::SmallVector<pir::Expr>{
							this->agent.createAdd(args[0], args[1], may_wrap, this->stmt_name("ADD"))
						};
					}

					if(is_unsigned){
						const pir::Expr add_wrap_inst = this->agent.createUAddWrap(
							args[0], args[1], this->stmt_name("ADD.VALUE"), this->stmt_name("ADD.WRAPPED")
						);

						this->add_fail_assertion(
							this->agent.extractUAddWrapWrapped(add_wrap_inst),
							"ADD_CHECK",
							"Addition wrapped",
							func_call.location
						);

						return evo::SmallVector<pir::Expr>{this->agent.extractUAddWrapResult(add_wrap_inst)};
					}else{
						const pir::Expr add_wrap_inst = this->agent.createSAddWrap(
							args[0], args[1], this->stmt_name("ADD.VALUE"), this->stmt_name("ADD.WRAPPED")
						);

						this->add_fail_assertion(
							this->agent.extractSAddWrapWrapped(add_wrap_inst),
							"ADD_CHECK",
							"Addition wrapped",
							func_call.location
						);

						return evo::SmallVector<pir::Expr>{this->agent.extractSAddWrapResult(add_wrap_inst)};
					}
				} break;

				case TemplatedIntrinsic::Kind::AddWrap: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					if(is_unsigned){
						const pir::Expr add_wrap_inst = this->agent.createUAddWrap(
							args[0], args[1], this->stmt_name("ADD.VALUE"), this->stmt_name("ADD.WRAPPED")
						);

						return evo::SmallVector<pir::Expr>{
							this->agent.extractUAddWrapResult(add_wrap_inst),
							this->agent.extractUAddWrapWrapped(add_wrap_inst)
						};
					}else{
						const pir::Expr add_wrap_inst = this->agent.createSAddWrap(
							args[0], args[1], this->stmt_name("ADD.VALUE"), this->stmt_name("ADD.WRAPPED")
						);

						return evo::SmallVector<pir::Expr>{
							this->agent.extractSAddWrapResult(add_wrap_inst),
							this->agent.extractSAddWrapWrapped(add_wrap_inst)
						};
					}
				} break;

				case TemplatedIntrinsic::Kind::AddSat: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					if(is_unsigned){
						return evo::SmallVector<pir::Expr>{
							this->agent.createUAddSat(args[0], args[1], this->stmt_name("ADD_SAT"))
						};
					}else{
						return evo::SmallVector<pir::Expr>{
							this->agent.createSAddSat(args[0], args[1], this->stmt_name("ADD_SAT"))
						};
					}
				} break;

				case TemplatedIntrinsic::Kind::FAdd: {
					return evo::SmallVector<pir::Expr>{
						this->agent.createFAdd(args[0], args[1], this->stmt_name("FADD"))
					};
				} break;


				///////////////////////////////////
				// subtraction

				case TemplatedIntrinsic::Kind::Sub: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const bool may_wrap = instantiation.templateArgs[1].as<bool>();
					if(may_wrap || this->config.checkedMath == false){
						return evo::SmallVector<pir::Expr>{
							this->agent.createSub(args[0], args[1], may_wrap, this->stmt_name("SUB"))
						};
					}

					if(is_unsigned){
						const pir::Expr sub_wrap_inst = this->agent.createUSubWrap(
							args[0], args[1], this->stmt_name("SUB.VALUE"), this->stmt_name("SUB.WRAPPED")
						);

						this->add_fail_assertion(
							this->agent.extractUSubWrapWrapped(sub_wrap_inst),
							"SUB_CHECK",
							"Subtraction wrapped",
							func_call.location
						);

						return evo::SmallVector<pir::Expr>{this->agent.extractUSubWrapResult(sub_wrap_inst)};
					}else{
						const pir::Expr sub_wrap_inst = this->agent.createSSubWrap(
							args[0], args[1], this->stmt_name("SUB.VALUE"), this->stmt_name("SUB.WRAPPED")
						);

						this->add_fail_assertion(
							this->agent.extractSSubWrapWrapped(sub_wrap_inst),
							"SUB_CHECK",
							"Subtraction wrapped",
							func_call.location
						);

						return evo::SmallVector<pir::Expr>{this->agent.extractSSubWrapResult(sub_wrap_inst)};
					}
				} break;

				case TemplatedIntrinsic::Kind::SubWrap: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					if(is_unsigned){
						const pir::Expr sub_wrap_inst = this->agent.createUSubWrap(
							args[0], args[1], this->stmt_name("SUB.VALUE"), this->stmt_name("SUB.WRAPPED")
						);

						return evo::SmallVector<pir::Expr>{
							this->agent.extractUSubWrapResult(sub_wrap_inst),
							this->agent.extractUSubWrapWrapped(sub_wrap_inst)
						};
					}else{
						const pir::Expr sub_wrap_inst = this->agent.createSSubWrap(
							args[0], args[1], this->stmt_name("SUB.VALUE"), this->stmt_name("SUB.WRAPPED")
						);

						return evo::SmallVector<pir::Expr>{
							this->agent.extractSSubWrapResult(sub_wrap_inst),
							this->agent.extractSSubWrapWrapped(sub_wrap_inst)
						};
					}
				} break;

				case TemplatedIntrinsic::Kind::SubSat: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					if(is_unsigned){
						return evo::SmallVector<pir::Expr>{
							this->agent.createUSubSat(args[0], args[1], this->stmt_name("SUB_SAT"))
						};
					}else{
						return evo::SmallVector<pir::Expr>{
							this->agent.createSSubSat(args[0], args[1], this->stmt_name("SUB_SAT"))
						};
					}
				} break;

				case TemplatedIntrinsic::Kind::FSub: {
					return evo::SmallVector<pir::Expr>{
						this->agent.createFSub(args[0], args[1], this->stmt_name("FSUB"))
					};
				} break;


				///////////////////////////////////
				// multiplication

				case TemplatedIntrinsic::Kind::Mul: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const bool may_wrap = instantiation.templateArgs[1].as<bool>();
					if(may_wrap || this->config.checkedMath == false){
						return evo::SmallVector<pir::Expr>{
							this->agent.createMul(args[0], args[1], may_wrap, this->stmt_name("MUL"))
						};
					}

					if(is_unsigned){
						const pir::Expr add_wrap_inst = this->agent.createUMulWrap(
							args[0], args[1], this->stmt_name("MUL.VALUE"), this->stmt_name("MUL.WRAPPED")
						);

						this->add_fail_assertion(
							this->agent.extractUMulWrapWrapped(add_wrap_inst),
							"MUL_CHECK",
							"Mulition wrapped",
							func_call.location
						);

						return evo::SmallVector<pir::Expr>{this->agent.extractUMulWrapResult(add_wrap_inst)};
					}else{
						const pir::Expr add_wrap_inst = this->agent.createSMulWrap(
							args[0], args[1], this->stmt_name("MUL.VALUE"), this->stmt_name("MUL.WRAPPED")
						);

						this->add_fail_assertion(
							this->agent.extractSMulWrapWrapped(add_wrap_inst),
							"MUL_CHECK",
							"Mulition wrapped",
							func_call.location
						);

						return evo::SmallVector<pir::Expr>{this->agent.extractSMulWrapResult(add_wrap_inst)};
					}
				} break;

				case TemplatedIntrinsic::Kind::MulWrap: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					if(is_unsigned){
						const pir::Expr add_wrap_inst = this->agent.createUMulWrap(
							args[0], args[1], this->stmt_name("MUL.VALUE"), this->stmt_name("MUL.WRAPPED")
						);

						return evo::SmallVector<pir::Expr>{
							this->agent.extractUMulWrapResult(add_wrap_inst),
							this->agent.extractUMulWrapWrapped(add_wrap_inst)
						};
					}else{
						const pir::Expr add_wrap_inst = this->agent.createSMulWrap(
							args[0], args[1], this->stmt_name("MUL.VALUE"), this->stmt_name("MUL.WRAPPED")
						);

						return evo::SmallVector<pir::Expr>{
							this->agent.extractSMulWrapResult(add_wrap_inst),
							this->agent.extractSMulWrapWrapped(add_wrap_inst)
						};
					}
				} break;

				case TemplatedIntrinsic::Kind::MulSat: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					if(is_unsigned){
						return evo::SmallVector<pir::Expr>{
							this->agent.createUMulSat(args[0], args[1], this->stmt_name("MUL_SAT"))
						};
					}else{
						return evo::SmallVector<pir::Expr>{
							this->agent.createSMulSat(args[0], args[1], this->stmt_name("MUL_SAT"))
						};
					}
				} break;

				case TemplatedIntrinsic::Kind::FMul: {
					return evo::SmallVector<pir::Expr>{
						this->agent.createFMul(args[0], args[1], this->stmt_name("FMUL"))
					};
				} break;


				///////////////////////////////////
				// division / remainder

				case TemplatedIntrinsic::Kind::Div: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_exact = instantiation.templateArgs[1].as<bool>();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					if(is_unsigned){
						return evo::SmallVector<pir::Expr>{
							this->agent.createUDiv(args[0], args[1], is_exact, this->stmt_name("DIV"))
						};
					}else{
						return evo::SmallVector<pir::Expr>{
							this->agent.createSDiv(args[0], args[1], is_exact, this->stmt_name("DIV"))
						};
					}
				} break;

				case TemplatedIntrinsic::Kind::FDiv: {
					return evo::SmallVector<pir::Expr>{
						this->agent.createFDiv(args[0], args[1], this->stmt_name("FDIV"))
					};
				} break;

				case TemplatedIntrinsic::Kind::Rem: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<pir::Expr>{
							this->agent.createFRem(args[0], args[1], this->stmt_name("REM"))
						};
					}

					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					if(is_unsigned){
						return evo::SmallVector<pir::Expr>{
							this->agent.createURem(args[0], args[1], this->stmt_name("REM"))
						};
					}else{
						return evo::SmallVector<pir::Expr>{
							this->agent.createSRem(args[0], args[1], this->stmt_name("REM"))
						};
					}
				} break;


				///////////////////////////////////
				// logical

				case TemplatedIntrinsic::Kind::Eq: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<pir::Expr>{
							this->agent.createFEq(args[0], args[1], this->stmt_name("EQ"))
						};
					}

					return evo::SmallVector<pir::Expr>{
						this->agent.createIEq(args[0], args[1], this->stmt_name("EQ"))
					};
				} break;

				case TemplatedIntrinsic::Kind::NEq: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<pir::Expr>{
							this->agent.createFNeq(args[0], args[1], this->stmt_name("NEQ"))
						};
					}

					return evo::SmallVector<pir::Expr>{
						this->agent.createINeq(args[0], args[1], this->stmt_name("NEQ"))
					};
				} break;

				case TemplatedIntrinsic::Kind::LT: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<pir::Expr>{
							this->agent.createFLT(args[0], args[1], this->stmt_name("LT"))
						};
					}

					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					if(is_unsigned){
						return evo::SmallVector<pir::Expr>{
							this->agent.createULT(args[0], args[1], this->stmt_name("LT"))
						};
					}else{
						return evo::SmallVector<pir::Expr>{
							this->agent.createSLT(args[0], args[1], this->stmt_name("LT"))
						};
					}
				} break;

				case TemplatedIntrinsic::Kind::LTE: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<pir::Expr>{
							this->agent.createFLT(args[0], args[1], this->stmt_name("LTE"))
						};
					}

					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					if(is_unsigned){
						return evo::SmallVector<pir::Expr>{
							this->agent.createULTE(args[0], args[1], this->stmt_name("LTE"))
						};
					}else{
						return evo::SmallVector<pir::Expr>{
							this->agent.createSLTE(args[0], args[1], this->stmt_name("LTE"))
						};
					}
				} break;

				case TemplatedIntrinsic::Kind::GT: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<pir::Expr>{
							this->agent.createFGT(args[0], args[1], this->stmt_name("GT"))
						};
					}

					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					if(is_unsigned){
						return evo::SmallVector<pir::Expr>{
							this->agent.createUGT(args[0], args[1], this->stmt_name("GT"))
						};
					}else{
						return evo::SmallVector<pir::Expr>{
							this->agent.createSGT(args[0], args[1], this->stmt_name("GT"))
						};
					}
				} break;

				case TemplatedIntrinsic::Kind::GTE: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<pir::Expr>{
							this->agent.createFGTE(args[0], args[1], this->stmt_name("GTE"))
						};
					}

					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					if(is_unsigned){
						return evo::SmallVector<pir::Expr>{
							this->agent.createUGTE(args[0], args[1], this->stmt_name("GTE"))
						};
					}else{
						return evo::SmallVector<pir::Expr>{
							this->agent.createSGTE(args[0], args[1], this->stmt_name("GTE"))
						};
					}
				} break;


				///////////////////////////////////
				// remainder

				case TemplatedIntrinsic::Kind::And: {
					// TODO: 
					evo::log::fatal("UNIMPLEMENTED (@and)");
					evo::breakpoint();

					// return evo::SmallVector<llvmint::Value>{
					// 	this->builder.createAnd(args[0], args[1], this->stmt_name("AND"))
					// };
				} break;

				case TemplatedIntrinsic::Kind::Or: {
					// TODO: 
					evo::log::fatal("UNIMPLEMENTED (@or)");
					evo::breakpoint();

					// return evo::SmallVector<llvmint::Value>{
					// 	this->builder.createOr(args[0], args[1], this->stmt_name("OR"))
					// };	
				} break;

				case TemplatedIntrinsic::Kind::Xor: {
					// TODO: 
					evo::log::fatal("UNIMPLEMENTED (@xor)");
					evo::breakpoint();

					// return evo::SmallVector<llvmint::Value>{
					// 	this->builder.createXor(args[0], args[1], this->stmt_name("XOR"))
					// };
				} break;

				case TemplatedIntrinsic::Kind::SHL: {
					// TODO: 
					evo::log::fatal("UNIMPLEMENTED (@shl)");
					evo::breakpoint();

					// const bool may_wrap = instantiation.templateArgs[2].as<bool>();
					// if(may_wrap){
					// 	return evo::SmallVector<llvmint::Value>{
					// 		this->builder.createSHL(args[0], args[1], false, false, this->stmt_name("SHL"))
					// 	};
					// }
						
					// const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					// const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);


					// const llvmint::Value shift_value = this->builder.createSHL(
					// 	args[0], args[1], is_unsigned, !is_unsigned, this->stmt_name("SHL")
					// );

					// if(this->config.checkedMath){
					// 	const llvmint::Value check_value = [&](){
					// 		if(is_unsigned){
					// 			return this->builder.createLSHR(
					// 				shift_value, args[1], true, this->stmt_name("SHL.CHECK")
					// 			);
					// 		}else{
					// 			return this->builder.createASHR(
					// 				shift_value, args[1], true, this->stmt_name("SHL.CHECK")
					// 			);
					// 		}
					// 	}();

					// 	this->add_fail_assertion(
					// 		this->builder.createINE(check_value, args[0], this->stmt_name("SHL.CHECK_NEQ")),
					// 		"SHL_CHECK",
					// 		"Bit-shift-left overflow",
					// 		func_call.location
					// 	);
					// }

					// return evo::SmallVector<llvmint::Value>{shift_value};
				} break;

				case TemplatedIntrinsic::Kind::SHLSat: {
					// TODO: 
					evo::log::fatal("UNIMPLEMENTED (@shlSat)");
					evo::breakpoint();

					// const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					// const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					// const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned 
					// 	? llvmint::IRBuilder::IntrinsicID::ushlSat
					// 	: llvmint::IRBuilder::IntrinsicID::sshlSat;

					// return evo::SmallVector<llvmint::Value>{
					// 	this->builder.createIntrinsicCall(
					// 		intrinsic_id, this->get_type(arg_type), args, this->stmt_name("SHL_SAT")
					// 	).asValue()
					// };
				} break;

				case TemplatedIntrinsic::Kind::SHR: {
					// TODO: 
					evo::log::fatal("UNIMPLEMENTED (@shr)");
					evo::breakpoint();
					
					// const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					// const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					// const bool is_exact = instantiation.templateArgs[2].as<bool>();

					// const llvmint::Value shift_value = [&](){
					// 	if(is_unsigned){
					// 		return this->builder.createLSHR(args[0], args[1], is_exact, this->stmt_name("SHR"));
					// 	}else{
					// 		return this->builder.createASHR(args[0], args[1], is_exact, this->stmt_name("SHR"));
					// 	}
					// }();

					// if(!is_exact && this->config.checkedMath){
					// 	const llvmint::Value check_value = this->builder.createSHL(
					// 		shift_value, args[1], true, false, this->stmt_name("SHR.CHECK")
					// 	);

					// 	this->add_fail_assertion(
					// 		this->builder.createINE(check_value, args[0], this->stmt_name("SHR.CHECK_NEQ")),
					// 		"SHR_CHECK",
					// 		"Bit-shift-right overflow",
					// 		func_call.location
					// 	);
					// }

					// return evo::SmallVector<llvmint::Value>{shift_value};
				} break;


				///////////////////////////////////
				// _max_

				case TemplatedIntrinsic::Kind::_max_: {
					evo::debugFatalBreak("Intrinsic::Kind::_max_ is not an actual intrinsic");
				} break;
			}

			evo::debugFatalBreak("Unknown or unsupported templated intrinsic kind");
		}
	}



	auto ASGToPIR::add_panic(std::string_view message, const ASG::Location& location) -> void {
		const pir::GlobalVar::ID panic_message = [&](){
			auto str_message = std::string(message);

			const auto message_find = this->globals.panic_messages.find(str_message);
			if(message_find != this->globals.panic_messages.end()){ return message_find->second; }

			const pir::GlobalVar::String::ID str_value = this->module.createGlobalString(std::move(str_message));

			const pir::GlobalVar::ID str_global = this->module.createGlobalVar(
				"PTHR.panic_message",
				this->module.getGlobalString(str_value).type,
				pir::Linkage::Private,
				str_value,
				true
			);

			this->globals.panic_messages.emplace(std::string(message), str_global);
			return str_global;
		}();

		if(this->config.addSourceLocations){
			auto args = evo::SmallVector<pir::Expr>{
				this->agent.createGlobalValue(panic_message),
				this->agent.createNumber(
					this->module.createIntegerType(32),
					core::GenericInt::create<uint32_t>(this->current_source->getID().get())
				),
				this->agent.createNumber(
					this->module.createIntegerType(32),
					core::GenericInt::create<uint32_t>(location.line)
				),
				this->agent.createNumber(
					this->module.createIntegerType(32),
					core::GenericInt::create<uint32_t>(location.collumn)
				),
			};

			if(this->config.isJIT){
				if(this->jit_links.panic.has_value() == false){
					this->jit_links.panic = this->module.createFunctionDecl(
						"PTHR.panic",
						evo::SmallVector<pcit::pir::Parameter>{
							pcit::pir::Parameter("str", this->module.createPtrType()),
							pcit::pir::Parameter("source_id", this->module.createIntegerType(32)),
							pcit::pir::Parameter("line", this->module.createIntegerType(32)),
							pcit::pir::Parameter("collumn", this->module.createIntegerType(32)),
						},
						pcit::pir::CallingConvention::C,
						pcit::pir::Linkage::External,
						this->module.createVoidType()
					);
				}

				this->agent.createCallVoid(*this->jit_links.panic, args);

			}else{
				// TODO: 
				evo::log::fatal("UNIMPLEMENTED (non-jit panic)");
				evo::breakpoint();

				// this->agent.createCallVoid(*this->non_jit_runtime.panic, args);
			}
		}else{
			if(this->config.isJIT){
				if(this->jit_links.panic.has_value() == false){
					this->jit_links.panic = this->module.createFunctionDecl(
						"PTHR.panic",
						evo::SmallVector<pcit::pir::Parameter>{
							pcit::pir::Parameter("str", this->module.createPtrType()),
						},
						pcit::pir::CallingConvention::C,
						pcit::pir::Linkage::External,
						this->module.createVoidType()
					);
				}

				this->agent.createCallVoid(*this->jit_links.panic, {this->agent.createGlobalValue(panic_message)});

			}else{
				// TODO: 
				evo::log::fatal("UNIMPLEMENTED (non-jit panic)");
				evo::breakpoint();

				// this->agent.createCallVoid(
				// 	*this->non_jit_runtime.panic, {this->agent.createGlobalValue(panic_message)}
				// );
			}
		}
	}

	auto ASGToPIR::add_fail_assertion(
		const pir::Expr& cond, std::string_view block_name, std::string_view message, const ASG::Location& location
	) -> void {
		const pir::BasicBlock::ID succeed_block = 
			this->agent.createBasicBlockInline(std::format("{}.SUCCEED", block_name));
		const pir::BasicBlock::ID fail_block = this->agent.createBasicBlockInline(std::format("{}.FAIL", block_name));


		this->agent.createCondBranch(cond, fail_block, succeed_block);

		this->agent.setTargetBasicBlock(fail_block);
		this->add_panic(message, location);
		this->agent.createBranch(succeed_block);

		this->agent.setTargetBasicBlock(succeed_block);
	}




	auto ASGToPIR::mangle_name(const ASG::Func& func) const -> std::string {
		return std::format(
			"PTHR.{}{}.{}",
			this->current_source->getID().get(),
			this->submangle_parent(func.parent),
			this->get_func_ident_name(func)
		);
	}


	auto ASGToPIR::mangle_name(const ASG::Var& var) const -> std::string {
		const std::string_view var_ident = this->current_source->getTokenBuffer()[var.ident].getString();

		return std::format("PTHR.{}.{}", this->current_source->getID().get(), var_ident);
	}


	auto ASGToPIR::submangle_parent(const ASG::Parent& parent) const -> std::string {
		return parent.visit([&](auto parent_id) -> std::string {
			using ParentID = std::decay_t<decltype(parent_id)>;

			if constexpr(std::is_same_v<ParentID, std::monostate>){
				return "";
			}else if constexpr(std::is_same_v<ParentID, ASG::Func::ID>){
				const ASG::Func& parent_func = this->current_source->getASGBuffer().getFunc(parent_id);
				return std::format(
					"{}.{}",
					this->submangle_parent(parent_func.parent),
					this->get_func_ident_name(parent_func)
				);
			}
		});
	}


	auto ASGToPIR::get_func_ident_name(const ASG::Func& func) const -> std::string {
		const Token::ID func_ident_token_id = this->current_source->getASTBuffer().getIdent(func.name);
		auto name = std::string(this->current_source->getTokenBuffer()[func_ident_token_id].getString());

		if(func.instanceID.has_value()){
			name += std::format("-i{}", func.instanceID.get());
		}

		return name;
	}


	auto ASGToPIR::stmt_name(std::string_view str) const -> std::string {
		if(this->config.useReadableRegisters) [[unlikely]] {
			return std::string(str);
		}else{
			return std::string();
		}
	}


	template<class... Args>
	auto ASGToPIR::stmt_name(std::format_string<Args...> fmt, Args&&... args) const -> std::string {
		if(this->config.useReadableRegisters) [[unlikely]] {
			return std::format(fmt, std::forward<Args...>(args)...);
		}else{
			return std::string();
		}
	}



	auto ASGToPIR::get_func_info(ASG::Func::LinkID link_id) const -> const FuncInfo& {
		const auto& info_find = this->func_infos.find(link_id);
		evo::debugAssert(info_find != this->func_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}

	auto ASGToPIR::get_current_func_info() const -> const FuncInfo& {
		evo::debugAssert(this->current_func_link_id.has_value(), "current_func_link_id is not set");
		return this->get_func_info(*this->current_func_link_id);
	}


	auto ASGToPIR::get_var_info(ASG::Var::LinkID link_id) const -> const VarInfo& {
		const auto& info_find = this->var_infos.find(link_id);
		evo::debugAssert(info_find != this->var_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}

	auto ASGToPIR::get_param_info(ASG::Param::LinkID link_id) const -> const ParamInfo& {
		const auto& info_find = this->param_infos.find(link_id);
		evo::debugAssert(info_find != this->param_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}

	auto ASGToPIR::get_return_param_info(ASG::ReturnParam::LinkID link_id) const -> const ReturnParamInfo& {
		const auto& info_find = this->return_param_infos.find(link_id);
		evo::debugAssert(info_find != this->return_param_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}



	auto ASGToPIR::link_jit_std_out_if_needed() -> void {
		if(this->jit_links.std_out.has_value() == false){
			this->jit_links.std_out = this->module.createFunctionDecl(
				"PTHR.stdout",
				evo::SmallVector<pcit::pir::Parameter>{
					pcit::pir::Parameter("str", this->module.createPtrType())
				},
				pcit::pir::CallingConvention::C,
				pcit::pir::Linkage::External,
				this->module.createVoidType()
			);
		}
	}

	auto ASGToPIR::link_jit_std_err_if_needed() -> void {
		if(this->jit_links.std_out.has_value() == false){
			this->jit_links.std_out = this->module.createFunctionDecl(
				"PTHR.stderr",
				evo::SmallVector<pcit::pir::Parameter>{
					pcit::pir::Parameter("str", this->module.createPtrType())
				},
				pcit::pir::CallingConvention::C,
				pcit::pir::Linkage::External,
				this->module.createVoidType()
			);
		}
	}


	auto ASGToPIR::link_libc_puts_if_needed() -> void {
		if(this->libc_links.puts.has_value() == false){
			this->libc_links.puts = this->module.createFunctionDecl(
				"puts",
				evo::SmallVector<pcit::pir::Parameter>{
					pcit::pir::Parameter("str", this->module.createPtrType())
				},
				pcit::pir::CallingConvention::C,
				pcit::pir::Linkage::External,
				this->module.createVoidType()
			);
		}
	}

	
}