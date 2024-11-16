////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./ASGToLLVMIR.h"

#include "../include/Context.h"
#include "../include/Source.h"
#include "../include/SourceManager.h"
#include "../include/get_source_location.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{

	auto ASGToLLVMIR::lower() -> void {
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


	auto ASGToLLVMIR::lowerFunc(const ASG::Func::LinkID& func_link_id) -> void {
		this->current_source = &this->context.getSourceManager()[func_link_id.sourceID()];

		this->lower_func_decl(func_link_id.funcID());
		this->lower_func_body(func_link_id.funcID());

		this->current_source = nullptr;
	}



	auto ASGToLLVMIR::addRuntime() -> void {
		const FuncInfo& entry_func_info = this->get_func_info(*this->context.getEntry());

		const llvmint::FunctionType main_func_proto = this->builder.getFuncProto(
			this->builder.getTypeI32().asType(), {}, false
		);

		llvmint::Function main_func = this->module.createFunction(
			"main", main_func_proto, llvmint::LinkageType::External
		);
		main_func.setNoThrow();
		main_func.setCallingConv(llvmint::CallingConv::Fast);

		const llvmint::BasicBlock begin_block = this->builder.createBasicBlock(main_func, "begin");
		this->builder.setInsertionPoint(begin_block);

		const llvmint::Value begin_ret = this->builder.createCall(entry_func_info.func, {}, '\0').asValue();
		
		this->builder.createRet(this->builder.createZExt(begin_ret, this->builder.getTypeI32().asType()));
	}


	auto ASGToLLVMIR::getFuncMangledName(ASG::Func::LinkID link_id) -> std::string_view {
		const FuncInfo& func_info = this->get_func_info(link_id);
		return std::string_view(func_info.mangled_name);
	}




	auto ASGToLLVMIR::add_links() -> void {
		// _printHelloWorld
		linked_functions.print_hello_world = this->module.createFunction(
			"PTHR._printHelloWorld",
			this->builder.getFuncProto(this->builder.getTypeVoid(), {}, false),
			llvmint::LinkageType::External
		);
		linked_functions.print_hello_world->setNoThrow();
	}

	auto ASGToLLVMIR::add_links_for_JIT() -> void {
		if(this->config.addSourceLocations){
			linked_functions.panic_with_location = this->module.createFunction(
				"PTHR.panic_with_location",
				this->builder.getFuncProto(
					this->builder.getTypeVoid(),
					{
						this->builder.getTypePtr().asType(),
						this->builder.getTypeI32().asType(),
						this->builder.getTypeI32().asType(),
						this->builder.getTypeI32().asType()
					},
					false
				),
				llvmint::LinkageType::External
			);
			linked_functions.panic_with_location->setNoThrow();

		}else{
			linked_functions.panic = this->module.createFunction(
				"PTHR.panic",
				this->builder.getFuncProto(this->builder.getTypeVoid(), {this->builder.getTypePtr().asType()}, false),
				llvmint::LinkageType::External
			);
			linked_functions.panic->setNoThrow();
		}
	}



	auto ASGToLLVMIR::lower_global_var(const ASG::Var::ID& var_id) -> void {
		const ASG::Var& asg_var = this->current_source->getASGBuffer().getVar(var_id);

		if(asg_var.kind == AST::VarDecl::Kind::Def){ return; } // make sure not to emit def variables

		const llvmint::Type type = this->get_type(*asg_var.typeID);

		llvmint::GlobalVariable global_var = [&](){
			if(asg_var.expr.kind() == ASG::Expr::Kind::Zeroinit) [[unlikely]] {
				const TypeInfo& var_type_info = this->context.getTypeManager().getTypeInfo(*asg_var.typeID);
				evo::debugAssert(var_type_info.isPointer() == false, "cannot zeroinit a pointer");
				evo::debugAssert(var_type_info.isOptionalNotPointer() == false, "not supported");

				if(var_type_info.baseTypeID().kind() != BaseType::Kind::Primitive){
					return this->module.createGlobalZeroinit(
						type, llvmint::LinkageType::Internal, asg_var.isConst, this->mangle_name(asg_var)
					);
				}

				const BaseType::Primitive& var_primitive_type = this->context.getTypeManager().getPrimitive(
					var_type_info.baseTypeID().primitiveID()
				);
				
				llvmint::Constant constant_value = [&](){
					switch(var_primitive_type.kind()){
						case Token::Kind::TypeInt:       case Token::Kind::TypeISize:      case Token::Kind::TypeI_N:
						case Token::Kind::TypeUInt:      case Token::Kind::TypeUSize:      case Token::Kind::TypeUI_N:
						case Token::Kind::TypeCShort:    case Token::Kind::TypeCUShort:    case Token::Kind::TypeCInt:
						case Token::Kind::TypeCUInt:     case Token::Kind::TypeCLong:      case Token::Kind::TypeCULong:
						case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong: {
							const auto integer_type = llvmint::IntegerType(
								(llvm::IntegerType*)this->get_type(var_primitive_type).native()
							);
							return this->builder.getValueIntegral(integer_type, 0).asConstant();
						} break;

						case Token::Kind::TypeF16: case Token::Kind::TypeBF16: case Token::Kind::TypeF32:
						case Token::Kind::TypeF64: case Token::Kind::TypeF80:  case Token::Kind::TypeF128:
						case Token::Kind::TypeCLongDouble: {
							const llvmint::Type float_type = this->get_type(var_primitive_type);
							return this->builder.getValueFloat(float_type, 0.0);
						} break;

						case Token::Kind::TypeByte: return this->builder.getValueI8(0).asConstant();
						case Token::Kind::TypeBool: return this->builder.getValueBool(false).asConstant();
						case Token::Kind::TypeChar: return this->builder.getValueI8(0).asConstant();
						
						default: evo::debugFatalBreak("Unknown or unsupported base-type kind");
					}
				}();

				return this->module.createGlobal(
					constant_value, type, llvmint::LinkageType::Internal, asg_var.isConst, this->mangle_name(asg_var)
				);

			}else if(asg_var.expr.kind() == ASG::Expr::Kind::Uninit) [[unlikely]] {
				return this->module.createGlobalUninit(
					type, llvmint::LinkageType::Internal, asg_var.isConst, this->mangle_name(asg_var)
				);

			}else{
				const llvmint::Constant constant_value = this->get_constant_value(asg_var.expr);
				return this->module.createGlobal(
					constant_value, type, llvmint::LinkageType::Internal, asg_var.isConst, this->mangle_name(asg_var)
				);
			}
		}();


		this->var_infos.emplace(ASG::Var::LinkID(this->current_source->getID(), var_id), VarInfo(global_var));
	}


	auto ASGToLLVMIR::lower_func_decl(const ASG::Func::ID& func_id) -> void {
		const ASG::Func& func = this->current_source->getASGBuffer().getFunc(func_id);
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.baseTypeID.funcID());

		const llvmint::FunctionType func_proto = this->get_func_type(func_type);
		const auto linkage = llvmint::LinkageType::Internal;

		const std::string mangled_name = this->mangle_name(func);
		llvmint::Function llvm_func = this->module.createFunction(mangled_name, func_proto, linkage);
		llvm_func.setNoThrow();
		llvm_func.setCallingConv(llvmint::CallingConv::Fast);

		const auto asg_func_link_id = ASG::Func::LinkID(this->current_source->getID(), func_id);
		this->func_infos.emplace(asg_func_link_id, FuncInfo(llvm_func, std::move(mangled_name)));

		for(unsigned i = 0; const BaseType::Function::Param& param : func_type.params){
			const std::string_view param_name = param.ident.visit([&](const auto& param_ident_id) -> std::string_view {
				if constexpr(std::is_same_v<std::decay_t<decltype(param_ident_id)>, Token::ID>){
					return this->current_source->getTokenBuffer()[param_ident_id].getString();
				}else{
					return strings::toStringView(param_ident_id);
				}
			});
			llvm_func.getArg(i).setName(param_name);

			i += 1;
		}

		if(func_type.hasNamedReturns()){
			const unsigned first_return_param_index = unsigned(func_type.params.size());
			unsigned i = first_return_param_index;
			for(const BaseType::Function::ReturnParam& ret_param : func_type.returnParams){
				llvm_func.getArg(i).setName(
					std::format("ret.{}", this->current_source->getTokenBuffer()[*ret_param.ident].getString())
				);

				const unsigned return_i = i - first_return_param_index;

				this->return_param_infos.emplace(
					ASG::ReturnParam::LinkID(asg_func_link_id, func.returnParams[return_i]),
					ReturnParamInfo(
						llvm_func.getArg(i), this->get_type(ret_param.typeID.typeID()), return_i
					)
				);

				i += 1;
			}			
		}

		if(func_type.params.empty()){
			this->builder.createBasicBlock(llvm_func, "begin");
			
		}else{
			const llvmint::BasicBlock setup_block = this->builder.createBasicBlock(llvm_func, "setup");
			const llvmint::BasicBlock begin_block = this->builder.createBasicBlock(llvm_func, "begin");

			this->builder.setInsertionPoint(setup_block);

			for(unsigned i = 0; const BaseType::Function::Param& param : func_type.params){
				const std::string_view param_name = param.ident.visit(
					[&](const auto& param_ident_id) -> std::string_view {
						if constexpr(std::is_same_v<std::decay_t<decltype(param_ident_id)>, Token::ID>){
							return this->current_source->getTokenBuffer()[param_ident_id].getString();
						}else{
							return strings::toStringView(param_ident_id);
						}
					}
				);

				const llvmint::Alloca param_alloca = [&](){
					if(param.optimizeWithCopy){
						return this->builder.createAlloca(
							this->get_type(param.typeID), this->stmt_name("{}.alloca", param_name)
						);

					}else{
						return this->builder.createAlloca(
							this->builder.getTypePtr().asType(),
							this->stmt_name("{}.alloca", param_name)
						);
					}
				}();

				this->builder.createStore(param_alloca, llvm_func.getArg(i).asValue(), false);

				this->param_infos.emplace(
					ASG::Param::LinkID(asg_func_link_id, func.params[i]),
					ParamInfo(param_alloca, this->get_type(param.typeID), i)
				);

				i += 1;
			}

			this->builder.createBranch(begin_block);
		}
	}


	auto ASGToLLVMIR::lower_func_body(const ASG::Func::ID& func_id) -> void {
		const ASG::Func& asg_func = this->current_source->getASGBuffer().getFunc(func_id);
		this->current_func = &asg_func;

		auto link_id = ASG::Func::LinkID(this->current_source->getID(), func_id);
		this->current_func_link_id = link_id;
		const FuncInfo& func_info = this->get_func_info(link_id);

		this->builder.setInsertionPointAtBack(func_info.func);

		for(const ASG::Stmt& stmt : asg_func.stmts){
			this->lower_stmt(stmt);
		}

		if(asg_func.isTerminated == false){
			const BaseType::Function& func_type = 
				this->context.getTypeManager().getFunction(asg_func.baseTypeID.funcID());


			if(func_type.returnsVoid()){
				this->builder.createRet();
			}else{
				this->builder.createUnreachable();
			}
		}

		this->current_func_link_id = std::nullopt;
		this->current_func = nullptr;
	}



	auto ASGToLLVMIR::lower_stmt(const ASG::Stmt& stmt) -> void {
		const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();

		switch(stmt.kind()){
			break; case ASG::Stmt::Kind::Var:         this->lower_var(stmt.varID());
			break; case ASG::Stmt::Kind::FuncCall:    this->lower_func_call(asg_buffer.getFuncCall(stmt.funcCallID()));
			break; case ASG::Stmt::Kind::Assign:      this->lower_assign(asg_buffer.getAssign(stmt.assignID()));
			break; case ASG::Stmt::Kind::MultiAssign:
				this->lower_multi_assign(asg_buffer.getMultiAssign(stmt.multiAssignID()));
			break; case ASG::Stmt::Kind::Return:      this->lower_return(asg_buffer.getReturn(stmt.returnID()));
			break; case ASG::Stmt::Kind::Unreachable: this->builder.createUnreachable();
			break; case ASG::Stmt::Kind::Conditional:
				this->lower_conditional(asg_buffer.getConditional(stmt.conditionalID()));
			break; case ASG::Stmt::Kind::While:       this->lower_while(asg_buffer.getWhile(stmt.whileID()));
		}
	}


	auto ASGToLLVMIR::lower_var(const ASG::Var::ID& var_id) -> void {
		const ASG::Var& asg_var = this->current_source->getASGBuffer().getVar(var_id);

		if(asg_var.kind == AST::VarDecl::Kind::Def){ return; } // make sure not to emit def variables

		const llvmint::Alloca var_alloca = this->builder.createAlloca(
			this->get_type(*asg_var.typeID),
			this->stmt_name("{}.alloca", this->current_source->getTokenBuffer()[asg_var.ident].getString())
		);

		switch(asg_var.expr.kind()){
			case ASG::Expr::Kind::Uninit: {
				// do nothing
			} break;

			case ASG::Expr::Kind::Zeroinit: {
				this->builder.createMemSetInline(
					var_alloca.asValue(),
					this->builder.getValueI8(0).asValue(),
					this->get_value_size(this->context.getTypeManager().sizeOf(*asg_var.typeID)).asValue(),
					false
				);
			} break;

			default: {
				this->builder.createStore(var_alloca, this->get_value(asg_var.expr), false);
			} break;
		}

		this->var_infos.emplace(ASG::Var::LinkID(this->current_source->getID(), var_id), VarInfo(var_alloca));
	}


	auto ASGToLLVMIR::lower_func_call(const ASG::FuncCall& func_call) -> void {
		// TODO: optimize the generated ouput
		[[maybe_unused]] const evo::SmallVector<llvmint::Value> ret_vals = lower_returning_func_call(func_call, false);
	}


	auto ASGToLLVMIR::lower_assign(const ASG::Assign& assign) -> void {
		const llvmint::Value lhs = this->get_concrete_value(assign.lhs);
		const llvmint::Value rhs = this->get_value(assign.rhs);

		this->builder.createStore(lhs, rhs, false);
	}


	auto ASGToLLVMIR::lower_multi_assign(const ASG::MultiAssign& multi_assign) -> void {
		auto assign_targets = evo::SmallVector<std::optional<llvmint::Value>>();
		assign_targets.reserve(multi_assign.targets.size());

		for(const std::optional<ASG::Expr>& target : multi_assign.targets){
			if(target.has_value()){
				assign_targets.emplace_back(this->get_concrete_value(*target));
			}else{
				assign_targets.emplace_back();
			}
		}

		const evo::SmallVector<llvmint::Value> values = this->lower_returning_func_call(
			this->current_source->getASGBuffer().getFuncCall(multi_assign.value.funcCallID()), false
		);

		for(size_t i = 0; const std::optional<llvmint::Value>& assign_target : assign_targets){
			EVO_DEFER([&](){ i += 1; });

			if(assign_target.has_value() == false){ continue; }

			this->builder.createStore(assign_target.value(), values[i], false);
		}
	}


	auto ASGToLLVMIR::lower_return(const ASG::Return& return_stmt) -> void {
		if(return_stmt.value.has_value() == false){
			this->builder.createRet();
			return;
		}

		this->builder.createRet(this->get_value(*return_stmt.value));
	}



	auto ASGToLLVMIR::lower_conditional(const ASG::Conditional& conditional_stmt) -> void {
		const FuncInfo& current_func_info = this->get_current_func_info();

		const llvmint::BasicBlock then_block = this->builder.createBasicBlock(current_func_info.func, "IF.THEN");
		auto end_block = std::optional<llvmint::BasicBlock>();

		const llvmint::Value cond_value = this->get_value(conditional_stmt.cond);

		if(conditional_stmt.elseStmts.empty()){
			end_block = this->builder.createBasicBlock(current_func_info.func, "IF.END");

			this->builder.createCondBranch(cond_value, then_block, *end_block);

			this->builder.setInsertionPoint(then_block);
			for(const ASG::Stmt& stmt : conditional_stmt.thenStmts){
				this->lower_stmt(stmt);
			}

			if(conditional_stmt.thenStmts.isTerminated() == false){
				this->builder.setInsertionPoint(then_block);
				this->builder.createBranch(*end_block);
			}
		}else{
			const llvmint::BasicBlock else_block = this->builder.createBasicBlock(current_func_info.func, "IF.ELSE");

			this->builder.createCondBranch(cond_value, then_block, else_block);

			// then block
			this->builder.setInsertionPoint(then_block);
			for(const ASG::Stmt& stmt : conditional_stmt.thenStmts){
				this->lower_stmt(stmt);
			}

			// required because stuff in the then block might add basic blocks
			const llvmint::BasicBlock then_block_end = this->builder.getInsertionPoint();

			// else block
			this->builder.setInsertionPoint(else_block);
			for(const ASG::Stmt& stmt : conditional_stmt.elseStmts){
				this->lower_stmt(stmt);
			}

			// end block
			const bool then_terminated = conditional_stmt.thenStmts.isTerminated();
			const bool else_terminated = conditional_stmt.elseStmts.isTerminated();

			if(else_terminated && then_terminated){ return; }

			end_block = this->builder.createBasicBlock(current_func_info.func, "IF.END");

			if(!else_terminated){
				this->builder.createBranch(*end_block);
			}

			if(!then_terminated){
				this->builder.setInsertionPoint(then_block_end);
				this->builder.createBranch(*end_block);
			}
		}


		this->builder.setInsertionPoint(*end_block);
	}


	auto ASGToLLVMIR::lower_while(const ASG::While& while_loop) -> void {
		const FuncInfo& current_func_info = this->get_current_func_info();
		
		const llvmint::BasicBlock cond_block = this->builder.createBasicBlock(current_func_info.func, "WHILE.COND");
		const llvmint::BasicBlock body_block = this->builder.createBasicBlock(current_func_info.func, "WHILE.BODY");
		const llvmint::BasicBlock end_block = this->builder.createBasicBlock(current_func_info.func, "WHILE.END");

		this->builder.createBranch(cond_block);

		this->builder.setInsertionPoint(cond_block);
		const llvmint::Value cond_value = this->get_value(while_loop.cond);
		this->builder.createCondBranch(cond_value, body_block, end_block);

		this->builder.setInsertionPoint(body_block);
		for(const ASG::Stmt& stmt : while_loop.block){
			this->lower_stmt(stmt);
		}
		this->builder.createBranch(cond_block);

		this->builder.setInsertionPoint(end_block);
	}




	auto ASGToLLVMIR::get_type(const TypeInfo::VoidableID& type_info_voidable_id) const -> llvmint::Type {
		if(type_info_voidable_id.isVoid()) [[unlikely]] {
			return this->builder.getTypeVoid();
		}else{
			return this->get_type(this->context.getTypeManager().getTypeInfo(type_info_voidable_id.typeID()));
		}
	}

	auto ASGToLLVMIR::get_type(const TypeInfo::ID& type_info_id) const -> llvmint::Type {
		return this->get_type(this->context.getTypeManager().getTypeInfo(type_info_id));
	}

	auto ASGToLLVMIR::get_type(const TypeInfo& type_info) const -> llvmint::Type {
		const evo::ArrayProxy<AST::Type::Qualifier> type_qualifiers = type_info.qualifiers();
		if(type_qualifiers.empty() == false){
			if(type_qualifiers.back().isPtr){
				return this->builder.getTypePtr().asType();
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
				return this->builder.getTypePtr().asType();
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


	auto ASGToLLVMIR::get_type(const BaseType::Primitive& primitive) const -> llvmint::Type {
		switch(primitive.kind()){
			case Token::Kind::TypeInt: case Token::Kind::TypeUInt: {
				return this->builder.getTypeI_N(
					unsigned(this->context.getTypeManager().sizeOfGeneralRegister() * 8)
				).asType();
			} break;

			case Token::Kind::TypeISize: case Token::Kind::TypeUSize:{
				return this->builder.getTypeI_N(
					unsigned(this->context.getTypeManager().sizeOfPtr() * 8)
				).asType();
			} break;

			case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N: {
				return this->builder.getTypeI_N(unsigned(primitive.bitWidth())).asType();
			} break;

			case Token::Kind::TypeF16: return this->builder.getTypeF16();
			case Token::Kind::TypeBF16: return this->builder.getTypeBF16();
			case Token::Kind::TypeF32: return this->builder.getTypeF32();
			case Token::Kind::TypeF64: return this->builder.getTypeF64();
			case Token::Kind::TypeF80: return this->builder.getTypeF80();
			case Token::Kind::TypeF128: return this->builder.getTypeF128();
			case Token::Kind::TypeByte: return this->builder.getTypeI8().asType();
			case Token::Kind::TypeBool: return this->builder.getTypeBool().asType();
			case Token::Kind::TypeChar: return this->builder.getTypeI8().asType();
			case Token::Kind::TypeRawPtr: return this->builder.getTypePtr().asType();
			case Token::Kind::TypeTypeID: return this->builder.getTypeI32().asType();

			case Token::Kind::TypeCShort: case Token::Kind::TypeCUShort: 
				return this->builder.getTypeI16().asType();

			case Token::Kind::TypeCInt: case Token::Kind::TypeCUInt: {
				if(this->context.getTypeManager().getOS() == core::OS::Windows){
					return this->builder.getTypeI32().asType();
				}else{
					return this->builder.getTypeI64().asType();
				}
			} break;

			case Token::Kind::TypeCLong: case Token::Kind::TypeCULong:
				return this->builder.getTypeI32().asType();

			case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong:
				return this->builder.getTypeI64().asType();

			case Token::Kind::TypeCLongDouble: {
				if(this->context.getTypeManager().getOS() == core::OS::Windows){
					return this->builder.getTypeF64();
				}else{
					return this->builder.getTypeF80();
				}
			} break;

			default: evo::debugFatalBreak(
				"Unknown or unsupported primitive-type: {}", evo::to_underlying(primitive.kind())
			);
		}
	}




	auto ASGToLLVMIR::get_func_type(const BaseType::Function& func_type) const -> llvmint::FunctionType {
		auto param_types = evo::SmallVector<llvmint::Type>();
		param_types.reserve(func_type.params.size());
		for(const BaseType::Function::Param& param : func_type.params){
			if(param.optimizeWithCopy){
				param_types.emplace_back(this->get_type(param.typeID));
			}else{
				param_types.emplace_back(this->builder.getTypePtr());
			}
		}

		if(func_type.hasNamedReturns()){
			for(size_t i = 0; i < func_type.returnParams.size(); i+=1){
				param_types.emplace_back(this->builder.getTypePtr());
			}
		}

		const llvmint::Type return_type = [&](){
			if(func_type.hasNamedReturns()){
				return this->builder.getTypeVoid();
			}else{
				return this->get_type(func_type.returnParams.front().typeID);
			}
		}();

		return this->builder.getFuncProto(return_type, param_types, false);
	}


	auto ASGToLLVMIR::get_concrete_value(const ASG::Expr& expr) -> llvmint::Value {
		switch(expr.kind()){
			case ASG::Expr::Kind::Uninit:       case ASG::Expr::Kind::Zeroinit:    case ASG::Expr::Kind::LiteralInt:
			case ASG::Expr::Kind::LiteralFloat: case ASG::Expr::Kind::LiteralBool: case ASG::Expr::Kind::LiteralChar:
			case ASG::Expr::Kind::Intrinsic:    case ASG::Expr::Kind::TemplatedIntrinsicInstantiation:
			case ASG::Expr::Kind::Copy:         case ASG::Expr::Kind::Move:        case ASG::Expr::Kind::AddrOf:
			case ASG::Expr::Kind::FuncCall: {
				evo::debugFatalBreak("Cannot get concrete value this kind");
			} break;

			case ASG::Expr::Kind::Deref: {
				const ASG::Deref& deref_expr = this->current_source->getASGBuffer().getDeref(expr.derefID());
				return this->get_value(deref_expr.expr, false);
			} break;

			case ASG::Expr::Kind::Var: {
				const VarInfo& var_info = this->get_var_info(expr.varLinkID());
				return var_info.value.visit([&](auto value) -> llvmint::Value {
					using ValueT = std::decay_t<decltype(value)>;

					if constexpr(std::is_same_v<ValueT, llvmint::Alloca>){
						return value.asValue();
					}else if constexpr(std::is_same_v<ValueT, llvmint::GlobalVariable>){
						return value.asValue();
					}else{
						static_assert(false, "Unknown or unsupported var kind");
					}
				});
			} break;

			case ASG::Expr::Kind::Func: {
				evo::fatalBreak("Function values are unsupported");
			} break;

			case ASG::Expr::Kind::Param: {
				const ParamInfo& param_info = this->get_param_info(expr.paramLinkID());
				
				const BaseType::Function& func_type = this->context.getTypeManager().getFunction(
					this->current_func->baseTypeID.funcID()
				);

				const bool optimize_with_copy = func_type.params[param_info.index].optimizeWithCopy;

				if(optimize_with_copy){
					return param_info.alloca.asValue();

				}else{
					return this->builder.createLoad(
						param_info.alloca, false, this->stmt_name("param.ptr_lookup")
					).asValue();
				}
			} break;

			case ASG::Expr::Kind::ReturnParam: {
				const ReturnParamInfo& ret_param_info = this->get_return_param_info(expr.returnParamLinkID());

				return ret_param_info.arg.asValue();
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported expr kind");
	}


	auto ASGToLLVMIR::get_value(const ASG::Expr& expr, bool get_pointer_to_value) -> llvmint::Value {
		switch(expr.kind()){
			case ASG::Expr::Kind::Uninit: {
				evo::debugFatalBreak("Cannot get value of [uninit]");
			} break;

			case ASG::Expr::Kind::Zeroinit: {
				evo::debugFatalBreak("Cannot get value of [zeroinit]");
			} break;

			case ASG::Expr::Kind::LiteralInt: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralInt& literal_int = asg_buffer.getLiteralInt(expr.literalIntID());

				const llvmint::Type literal_type = this->get_type(*literal_int.typeID);
				const auto integer_type = llvmint::IntegerType((llvm::IntegerType*)literal_type.native());
				const llvmint::ConstantInt value = this->builder.getValueIntegral(
					integer_type, static_cast<uint64_t>(literal_int.value)
				);
				if(get_pointer_to_value == false){ return value.asValue(); }

				const llvmint::Alloca alloca = this->builder.createAlloca(literal_type);
				this->builder.createStore(alloca, value.asValue(), false);
				return alloca.asValue();
			} break;

			case ASG::Expr::Kind::LiteralFloat: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralFloat& literal_float = asg_buffer.getLiteralFloat(expr.literalFloatID());

				const llvmint::Type literal_type = this->get_type(*literal_float.typeID);
				const llvmint::Constant value = this->builder.getValueFloat(
					literal_type, static_cast<float64_t>(literal_float.value)
				);
				if(get_pointer_to_value == false){ return value.asValue(); }

				const llvmint::Alloca alloca = this->builder.createAlloca(literal_type);
				this->builder.createStore(alloca, value.asValue(), false);
				return alloca.asValue();
			} break;

			case ASG::Expr::Kind::LiteralBool: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const bool bool_value = asg_buffer.getLiteralBool(expr.literalBoolID()).value;

				const llvmint::ConstantInt value = this->builder.getValueBool(bool_value);
				if(get_pointer_to_value == false){ return value.asValue(); }

				const llvmint::Alloca alloca = this->builder.createAlloca(this->builder.getTypeI8().asType());
				this->builder.createStore(alloca, value.asValue(), false);
				return alloca.asValue();
			} break;

			case ASG::Expr::Kind::LiteralChar: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const char char_value = asg_buffer.getLiteralChar(expr.literalCharID()).value;

				const llvmint::ConstantInt value = this->builder.getValueI8(uint8_t(char_value));
				if(get_pointer_to_value == false){ return value.asValue(); }

				const llvmint::Alloca alloca = this->builder.createAlloca(this->builder.getTypeI8().asType());
				this->builder.createStore(alloca, value.asValue(), false);
				return alloca.asValue();
			} break;

			case ASG::Expr::Kind::Intrinsic: {
				evo::debugFatalBreak("Cannot get value of intrinsic function");
			} break;

			case ASG::Expr::Kind::TemplatedIntrinsicInstantiation: {
				evo::debugFatalBreak("Cannot get value of template instantiated intrinsic function");
			} break;

			case ASG::Expr::Kind::Copy: {
				const ASG::Expr& copy_expr = this->current_source->getASGBuffer().getCopy(expr.copyID());
				return this->get_value(copy_expr, get_pointer_to_value);
			} break;

			case ASG::Expr::Kind::Move: {
				const ASG::Expr& move_expr = this->current_source->getASGBuffer().getMove(expr.moveID());
				return this->get_value(move_expr, get_pointer_to_value);
			} break;

			case ASG::Expr::Kind::Deref: {
				const ASG::Deref& deref_expr = this->current_source->getASGBuffer().getDeref(expr.derefID());
				const llvmint::Value value = this->get_value(deref_expr.expr, false);
				if(get_pointer_to_value){ return value; }

				return this->builder.createLoad(
					value, this->get_type(deref_expr.typeID), false, this->stmt_name("DEREF")
				).asValue();
			} break;

			case ASG::Expr::Kind::AddrOf: {
				const ASG::Expr& addr_of_expr = this->current_source->getASGBuffer().getAddrOf(expr.addrOfID());
				const llvmint::Value address = this->get_value(addr_of_expr, true);
				if(get_pointer_to_value == false){ return address; }

				const llvmint::Alloca alloca = this->builder.createAlloca(this->builder.getTypePtr().asType());
				this->builder.createStore(alloca, address, false);
				return alloca.asValue();
			} break;

			case ASG::Expr::Kind::FuncCall: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::FuncCall& func_call = asg_buffer.getFuncCall(expr.funcCallID());
				
				return this->lower_returning_func_call(func_call, get_pointer_to_value).front();
			} break;

			case ASG::Expr::Kind::Var: {
				const VarInfo& var_info = this->get_var_info(expr.varLinkID());
				if(get_pointer_to_value){
					return var_info.value.visit([&](auto value) -> llvmint::Value {
						using ValueT = std::decay_t<decltype(value)>;

						if constexpr(std::is_same_v<ValueT, llvmint::Alloca>){
							return value.asValue();
						}else if constexpr(std::is_same_v<ValueT, llvmint::GlobalVariable>){
							return value.asValue();
						}else{
							static_assert(false, "Unknown or unsupported var kind");
						}
					});
				}

				const llvmint::LoadInst load_inst = var_info.value.visit([&](auto value) -> llvmint::LoadInst {
					return this->builder.createLoad(value, false, this->stmt_name("VAR.load"));
				});
				return load_inst.asValue();
			} break;

			case ASG::Expr::Kind::Func: {
				return this->get_func_info(expr.funcLinkID()).func.asValue();
			} break;

			case ASG::Expr::Kind::Param: {
				const ParamInfo& param_info = this->get_param_info(expr.paramLinkID());

				const BaseType::Function& func_type = this->context.getTypeManager().getFunction(
					this->current_func->baseTypeID.funcID()
				);

				const bool optimize_with_copy = func_type.params[param_info.index].optimizeWithCopy;

				if(optimize_with_copy){
					if(get_pointer_to_value){
						return param_info.alloca.asValue();
					}else{
						return this->builder.createLoad(
							param_info.alloca, false, this->stmt_name("PARAM.load")
						).asValue();
					}

				}else{
					const llvmint::LoadInst load_inst = this->builder.createLoad(
						param_info.alloca, false, this->stmt_name("PARAM.ptr_lookup")
					);

					if(get_pointer_to_value) [[unlikely]] {
						return load_inst.asValue();
					}else{
						return this->builder.createLoad(
							load_inst.asValue(), param_info.type, false, this->stmt_name("PARAM.load")
						).asValue();
					}
				}
			} break;

			case ASG::Expr::Kind::ReturnParam: {
				const ReturnParamInfo& ret_param_info = this->get_return_param_info(expr.returnParamLinkID());

				if(get_pointer_to_value) [[unlikely]] {
					return ret_param_info.arg.asValue();
				}else{
					const llvmint::LoadInst load_inst = this->builder.createLoad(
						ret_param_info.arg.asValue(), ret_param_info.type, false, this->stmt_name("RET_PARAM.load")
					);
					return load_inst.asValue();
				}
			} break;

		}

		evo::debugFatalBreak("Unknown or unsupported expr kind");
	}


	auto ASGToLLVMIR::get_constant_value(const ASG::Expr& expr) -> llvmint::Constant {
		switch(expr.kind()){
			case ASG::Expr::Kind::LiteralInt: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralInt& literal_int = asg_buffer.getLiteralInt(expr.literalIntID());

				const llvmint::Type literal_type = this->get_type(*literal_int.typeID);
				const auto integer_type = llvmint::IntegerType((llvm::IntegerType*)literal_type.native());
				return this->builder.getValueIntegral(
					integer_type, uint64_t(literal_int.value)
				).asConstant();
			} break;

			case ASG::Expr::Kind::LiteralFloat: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralFloat& literal_float = asg_buffer.getLiteralFloat(expr.literalFloatID());

				const llvmint::Type literal_type = this->get_type(*literal_float.typeID);
				return this->builder.getValueFloat(literal_type, static_cast<float64_t>(literal_float.value));
			} break;

			case ASG::Expr::Kind::LiteralBool: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const bool bool_value = asg_buffer.getLiteralBool(expr.literalBoolID()).value;

				return this->builder.getValueBool(bool_value).asConstant();
			} break;

			case ASG::Expr::Kind::LiteralChar: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const char char_value = asg_buffer.getLiteralChar(expr.literalCharID()).value;

				return this->builder.getValueI8(uint8_t(char_value)).asConstant();
			} break;

			case ASG::Expr::Kind::Uninit:
			case ASG::Expr::Kind::Zeroinit:
			case ASG::Expr::Kind::Intrinsic:
			case ASG::Expr::Kind::TemplatedIntrinsicInstantiation:
			case ASG::Expr::Kind::Copy:
			case ASG::Expr::Kind::Move:
			case ASG::Expr::Kind::FuncCall:
			case ASG::Expr::Kind::AddrOf:
			case ASG::Expr::Kind::Deref:
			case ASG::Expr::Kind::Var:
			case ASG::Expr::Kind::Func:
			case ASG::Expr::Kind::Param:
			case ASG::Expr::Kind::ReturnParam:
				evo::debugFatalBreak("Cannot get constant value from this ASG::Expr kind");
		}

		evo::debugFatalBreak("Unknown or unsupported ASG::Expr::Kind");
	}


	auto ASGToLLVMIR::lower_returning_func_call(const ASG::FuncCall& func_call, bool get_pointer_to_value)
	-> evo::SmallVector<llvmint::Value> {
		if(func_call.target.is<ASG::Func::LinkID>() == false){
			return this->lower_returning_intrinsic_call(func_call, get_pointer_to_value);
		}

		const ASG::Func::LinkID& func_link_id = func_call.target.as<ASG::Func::LinkID>();
		const FuncInfo& func_info = this->get_func_info(func_link_id);

		const Source& linked_source = this->context.getSourceManager()[func_link_id.sourceID()];
		const ASG::Func& asg_func = linked_source.getASGBuffer().getFunc(func_link_id.funcID());
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(
			asg_func.baseTypeID.funcID()
		);

		auto args = evo::SmallVector<llvmint::Value>();
		args.reserve(func_call.args.size());
		for(size_t i = 0; const ASG::Expr& arg : func_call.args){
			const bool optimize_with_copy = func_type.params[i].optimizeWithCopy;
			args.emplace_back(this->get_value(arg, !optimize_with_copy));

			i += 1;
		}


		auto return_values = evo::SmallVector<llvmint::Value>();
		if(func_type.hasNamedReturns()){
			for(const BaseType::Function::ReturnParam& return_param : func_type.returnParams){
				const llvmint::Alloca alloca = this->builder.createAlloca(
					this->get_type(return_param.typeID),
					this->stmt_name(
						"RET_PARAM.{}.alloca",
						this->current_source->getTokenBuffer()[*return_param.ident].getString()
					)
				);

				args.emplace_back(alloca.asValue());
			}

			this->builder.createCall(func_info.func, args);

			if(get_pointer_to_value){
				for(size_t i = func_type.params.size(); i < args.size(); i+=1){
					return_values.emplace_back(args[i]);
				}

				return return_values;
			}

			for(size_t i = func_type.params.size(); i < args.size(); i+=1){
				return_values.emplace_back(
					this->builder.createLoad(
						args[i], this->get_type(func_type.returnParams.front().typeID), false
					).asValue()
				);
			}

			return return_values;

		}else{
			const llvmint::Value func_call_value = this->builder.createCall(func_info.func, args).asValue();
			if(get_pointer_to_value == false){
				return_values.emplace_back(func_call_value);
				return return_values;
			}
			
			const llvmint::Type return_type = this->get_type(func_type.returnParams[0].typeID);

			const llvmint::Alloca alloca = this->builder.createAlloca(return_type);
			this->builder.createStore(alloca, func_call_value, false);

			return_values.emplace_back(alloca.asValue());
			return return_values;
		}
	}


	auto ASGToLLVMIR::lower_returning_intrinsic_call(const ASG::FuncCall& func_call, bool get_pointer_to_value)
	-> evo::SmallVector<llvmint::Value> {
		evo::debugAssert(get_pointer_to_value == false, "cannot get pointers to intrinsic func call values");

		auto args = evo::SmallVector<llvmint::Value>();
		args.reserve(func_call.args.size());
		for(size_t i = 0; const ASG::Expr& arg : func_call.args){
			args.emplace_back(this->get_value(arg, false));

			i += 1;
		}

		if(func_call.target.is<Intrinsic::Kind>()){
			switch(func_call.target.as<Intrinsic::Kind>()){
				case Intrinsic::Kind::Breakpoint: {
					this->builder.createIntrinsicCall(
						llvmint::IRBuilder::IntrinsicID::debugtrap, this->builder.getTypeVoid(), nullptr
					);

					return evo::SmallVector<llvmint::Value>();
				} break;

				case Intrinsic::Kind::_printHelloWorld: {
					this->builder.createCall(*this->linked_functions.print_hello_world, nullptr);

					return evo::SmallVector<llvmint::Value>();
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
					return evo::SmallVector<llvmint::Value>{
						this->builder.getValueBool(
							instantiation.templateArgs[0].as<TypeInfo::VoidableID>() == 
							instantiation.templateArgs[1].as<TypeInfo::VoidableID>()
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::IsTriviallyCopyable: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.getValueBool(
							this->context.getTypeManager().isTriviallyCopyable(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
							)
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::IsTriviallyDestructable: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.getValueBool(
							this->context.getTypeManager().isTriviallyDestructable(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
							)
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::IsPrimitive: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.getValueBool(
							this->context.getTypeManager().isPrimitive(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
							)
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::IsBuiltin: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.getValueBool(
							this->context.getTypeManager().isBuiltin(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
							)
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::IsIntegral: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.getValueBool(
							this->context.getTypeManager().isIntegral(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>()
							)
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::IsFloatingPoint: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.getValueBool(
							this->context.getTypeManager().isFloatingPoint(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>()
							)
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::SizeOf: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.getValueI_N(
							unsigned(this->context.getTypeManager().sizeOfPtr()),
							this->context.getTypeManager().sizeOf(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
							)
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::GetTypeID: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.getValueIntegral(
							llvmint::IntegerType( // This is safe because it is guaranteed to be an integer type
								(llvm::IntegerType*)this->get_type(TypeManager::getTypeTypeID()).native()
							),
							instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID().get()
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::BitCast: {
					const TypeInfo::ID from_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const TypeInfo::ID to_type_id = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();

					if(from_type_id == to_type_id){ return args; }

					return evo::SmallVector<llvmint::Value>{
						this->builder.createBitCast(args[0], this->get_type(to_type_id), this->stmt_name("BITCAST"))
					};
				} break;


				case TemplatedIntrinsic::Kind::Trunc: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<llvmint::Value>{
						this->builder.createTrunc(args[0], this->get_type(to_type), this->stmt_name("TRUNC"))
					};
				} break;

				case TemplatedIntrinsic::Kind::FTrunc: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<llvmint::Value>{
						this->builder.createFPTrunc(args[0], this->get_type(to_type), this->stmt_name("FPTRUNC"))
					};
				} break;

				case TemplatedIntrinsic::Kind::SExt: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<llvmint::Value>{
						this->builder.createSExt(args[0], this->get_type(to_type), this->stmt_name("SEXT"))
					};
				} break;

				case TemplatedIntrinsic::Kind::ZExt: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<llvmint::Value>{
						this->builder.createZExt(args[0], this->get_type(to_type), this->stmt_name("ZEXT"))
					};
				} break;

				case TemplatedIntrinsic::Kind::FExt: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<llvmint::Value>{
						this->builder.createFPExt(args[0], this->get_type(to_type), this->stmt_name("FPEXT"))
					};
				} break;

				case TemplatedIntrinsic::Kind::IToF: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<llvmint::Value>{
						this->builder.createSIToFP(args[0], this->get_type(to_type), this->stmt_name("SITOFP"))
					};
				} break;

				case TemplatedIntrinsic::Kind::UIToF: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<llvmint::Value>{
						this->builder.createUIToFP(args[0], this->get_type(to_type), this->stmt_name("UITOFP"))
					};
				} break;

				case TemplatedIntrinsic::Kind::FToI: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<llvmint::Value>{
						this->builder.createFPToSI(args[0], this->get_type(to_type), this->stmt_name("FPTOSI"))
					};
				} break;

				case TemplatedIntrinsic::Kind::FToUI: {
					const TypeInfo::ID to_type = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().typeID();
					return evo::SmallVector<llvmint::Value>{
						this->builder.createFPToUI(args[0], this->get_type(to_type), this->stmt_name("FPTOUI"))
					};
				} break;


				///////////////////////////////////
				// addition

				case TemplatedIntrinsic::Kind::Add: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const bool may_wrap = instantiation.templateArgs[1].as<bool>();
					if(may_wrap || this->config.checkedMath == false){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createAdd(
								args[0],
								args[1],
								is_unsigned & !may_wrap,
								!is_unsigned & !may_wrap,
								this->stmt_name("ADD")
							)
						};
					}

					const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned
						? llvmint::IRBuilder::IntrinsicID::uaddOverflow
						: llvmint::IRBuilder::IntrinsicID::saddOverflow;

					const llvmint::Type return_type = this->builder.getStructType(
						{this->get_type(arg_type), this->get_type(TypeManager::getTypeBool())}
					).asType();

					const llvmint::Value add_wrap_value = this->builder.createIntrinsicCall(
						intrinsic_id, return_type, args, this->stmt_name("ADD")
					).asValue();

					const llvmint::Value add_value = 
						this->builder.createExtractValue(add_wrap_value, {0}, this->stmt_name("ADD.VALUE"));
					const llvmint::Value wrapped_value = 
						this->builder.createExtractValue(add_wrap_value, {1}, this->stmt_name("ADD.WRAPPED"));

					this->add_fail_assertion(wrapped_value, "ADD_CHECK", "Addition wrapped", func_call.location);

					return evo::SmallVector<llvmint::Value>{add_value};

				} break;

				case TemplatedIntrinsic::Kind::AddWrap: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned
						? llvmint::IRBuilder::IntrinsicID::uaddOverflow
						: llvmint::IRBuilder::IntrinsicID::saddOverflow;

					const llvmint::Type return_type = this->builder.getStructType(
						{this->get_type(arg_type), this->get_type(TypeManager::getTypeBool())}
					).asType();

					const llvmint::Value add_value = this->builder.createIntrinsicCall(
						intrinsic_id, return_type, args, this->stmt_name("ADD_WRAP")
					).asValue();

					return evo::SmallVector<llvmint::Value>{
						this->builder.createExtractValue(add_value, {0}, this->stmt_name("ADD_WRAP.VALUE")),
						this->builder.createExtractValue(add_value, {1}, this->stmt_name("ADD_WRAP.WRAPPED")),
					};
				} break;

				case TemplatedIntrinsic::Kind::AddSat: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned
						? llvmint::IRBuilder::IntrinsicID::uaddSat
						: llvmint::IRBuilder::IntrinsicID::saddSat;

					return evo::SmallVector<llvmint::Value>{
						this->builder.createIntrinsicCall(
							intrinsic_id, this->get_type(arg_type), args, this->stmt_name("ADD_SAT")
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::FAdd: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.createFAdd(args[0], args[1], this->stmt_name("FADD"))
					};
				} break;


				///////////////////////////////////
				// subtraction

				case TemplatedIntrinsic::Kind::Sub: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const bool may_wrap = instantiation.templateArgs[1].as<bool>();
					if(may_wrap || this->config.checkedMath == false){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createSub(
								args[0],
								args[1],
								is_unsigned & !may_wrap,
								!is_unsigned & !may_wrap,
								this->stmt_name("SUB")
							)
						};
					}

					const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned
						? llvmint::IRBuilder::IntrinsicID::usubOverflow
						: llvmint::IRBuilder::IntrinsicID::ssubOverflow;

					const llvmint::Type return_type = this->builder.getStructType(
						{this->get_type(arg_type), this->get_type(TypeManager::getTypeBool())}
					).asType();

					const llvmint::Value sub_wrap_value = this->builder.createIntrinsicCall(
						intrinsic_id, return_type, args, this->stmt_name("SUB")
					).asValue();

					const llvmint::Value sub_value = 
						this->builder.createExtractValue(sub_wrap_value, {0}, this->stmt_name("SUB.VALUE"));
					const llvmint::Value wrapped_value = 
						this->builder.createExtractValue(sub_wrap_value, {1}, this->stmt_name("SUB.WRAPPED"));

					this->add_fail_assertion(wrapped_value, "SUB_CHECK", "Subtraction wrapped", func_call.location);

					return evo::SmallVector<llvmint::Value>{sub_value};

				} break;

				case TemplatedIntrinsic::Kind::SubWrap: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned
						? llvmint::IRBuilder::IntrinsicID::usubOverflow
						: llvmint::IRBuilder::IntrinsicID::ssubOverflow;

					const llvmint::Type return_type = this->builder.getStructType(
						{this->get_type(arg_type), this->get_type(TypeManager::getTypeBool())}
					).asType();

					const llvmint::Value sub_value = this->builder.createIntrinsicCall(
						intrinsic_id, return_type, args, this->stmt_name("SUB_WRAP")
					).asValue();

					return evo::SmallVector<llvmint::Value>{
						this->builder.createExtractValue(sub_value, {0}, this->stmt_name("SUB_WRAP.VALUE")),
						this->builder.createExtractValue(sub_value, {1}, this->stmt_name("SUB_WRAP.WRAPPED")),
					};
				} break;

				case TemplatedIntrinsic::Kind::SubSat: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned
						? llvmint::IRBuilder::IntrinsicID::usubSat
						: llvmint::IRBuilder::IntrinsicID::ssubSat;

					return evo::SmallVector<llvmint::Value>{
						this->builder.createIntrinsicCall(
							intrinsic_id, this->get_type(arg_type), args, this->stmt_name("SUB_SAT")
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::FSub: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.createFSub(args[0], args[1], this->stmt_name("FSUB"))
					};
				} break;


				///////////////////////////////////
				// multiplication

				case TemplatedIntrinsic::Kind::Mul: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const bool may_wrap = instantiation.templateArgs[1].as<bool>();
					if(may_wrap || this->config.checkedMath == false){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createMul(
								args[0],
								args[1],
								is_unsigned & !may_wrap,
								!is_unsigned & !may_wrap,
								this->stmt_name("MUL")
							)
						};
					}

					const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned
						? llvmint::IRBuilder::IntrinsicID::umulOverflow
						: llvmint::IRBuilder::IntrinsicID::smulOverflow;

					const llvmint::Type return_type = this->builder.getStructType(
						{this->get_type(arg_type), this->get_type(TypeManager::getTypeBool())}
					).asType();

					const llvmint::Value mul_wrap_value = this->builder.createIntrinsicCall(
						intrinsic_id, return_type, args, this->stmt_name("MUL")
					).asValue();

					const llvmint::Value mul_value = 
						this->builder.createExtractValue(mul_wrap_value, {0}, this->stmt_name("MUL.VALUE"));
					const llvmint::Value wrapped_value = 
						this->builder.createExtractValue(mul_wrap_value, {1}, this->stmt_name("MUL.WRAPPED"));

					this->add_fail_assertion(wrapped_value, "MUL_CHECK", "Multiplication wrapped", func_call.location);

					return evo::SmallVector<llvmint::Value>{mul_value};

				} break;

				case TemplatedIntrinsic::Kind::MulWrap: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned
						? llvmint::IRBuilder::IntrinsicID::umulOverflow
						: llvmint::IRBuilder::IntrinsicID::smulOverflow;

					const llvmint::Type return_type = this->builder.getStructType(
						{this->get_type(arg_type), this->get_type(TypeManager::getTypeBool())}
					).asType();

					const llvmint::Value mul_value = this->builder.createIntrinsicCall(
						intrinsic_id, return_type, args, this->stmt_name("MUL_WRAP")
					).asValue();

					return evo::SmallVector<llvmint::Value>{
						this->builder.createExtractValue(mul_value, {0}, this->stmt_name("MUL_WRAP.VALUE")),
						this->builder.createExtractValue(mul_value, {1}, this->stmt_name("MUL_WRAP.WRAPPED")),
					};
				} break;

				case TemplatedIntrinsic::Kind::MulSat: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned
						? llvmint::IRBuilder::IntrinsicID::umulFixSat
						: llvmint::IRBuilder::IntrinsicID::smulFixSat;

					return evo::SmallVector<llvmint::Value>{
						this->builder.createIntrinsicCall(
							intrinsic_id,
							this->get_type(arg_type),
							{args[0], args[1], this->builder.getValueI32(0).asValue()},
							this->stmt_name("MUL_SAT")
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::FMul: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.createFMul(args[0], args[1], this->stmt_name("FMUL"))
					};
				} break;


				///////////////////////////////////
				// division / remainder

				case TemplatedIntrinsic::Kind::Div: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_exact = instantiation.templateArgs[1].as<bool>();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					if(is_unsigned){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createUDiv(args[0], args[1], is_exact, this->stmt_name("DIV"))
						};
					}else{
						return evo::SmallVector<llvmint::Value>{
							this->builder.createSDiv(args[0], args[1], is_exact, this->stmt_name("DIV"))
						};
					}
				} break;

				case TemplatedIntrinsic::Kind::FDiv: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.createFDiv(args[0], args[1], this->stmt_name("FDIV"))
					};
				} break;

				case TemplatedIntrinsic::Kind::Rem: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createFRem(args[0], args[1], this->stmt_name("REM"))
						};
					}

					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					if(is_unsigned){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createURem(args[0], args[1], this->stmt_name("REM"))
						};
					}else{
						return evo::SmallVector<llvmint::Value>{
							this->builder.createSRem(args[0], args[1], this->stmt_name("REM"))
						};
					}
				} break;


				///////////////////////////////////
				// logical

				case TemplatedIntrinsic::Kind::Eq: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createFCmpEQ(args[0], args[1], this->stmt_name("EQ"))
						};
					}

					return evo::SmallVector<llvmint::Value>{
						this->builder.createICmpEQ(args[0], args[1], this->stmt_name("EQ"))
					};
				} break;

				case TemplatedIntrinsic::Kind::NEq: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createFCmpNE(args[0], args[1], this->stmt_name("NEQ"))
						};
					}

					return evo::SmallVector<llvmint::Value>{
						this->builder.createICmpNE(args[0], args[1], this->stmt_name("NEQ"))
					};
				} break;

				case TemplatedIntrinsic::Kind::LT: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createFCmpLT(args[0], args[1], this->stmt_name("LT"))
						};
					}

					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					if(is_unsigned){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createICmpULT(args[0], args[1], this->stmt_name("LT"))
						};
					}else{
						return evo::SmallVector<llvmint::Value>{
							this->builder.createICmpSLT(args[0], args[1], this->stmt_name("LT"))
						};
					}	
				} break;

				case TemplatedIntrinsic::Kind::LTE: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createFCmpLT(args[0], args[1], this->stmt_name("LTE"))
						};
					}

					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					if(is_unsigned){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createICmpULT(args[0], args[1], this->stmt_name("LTE"))
						};
					}else{
						return evo::SmallVector<llvmint::Value>{
							this->builder.createICmpSLT(args[0], args[1], this->stmt_name("LTE"))
						};
					}	
				} break;

				case TemplatedIntrinsic::Kind::GT: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createFCmpGT(args[0], args[1], this->stmt_name("GT"))
						};
					}

					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					if(is_unsigned){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createICmpUGT(args[0], args[1], this->stmt_name("GT"))
						};
					}else{
						return evo::SmallVector<llvmint::Value>{
							this->builder.createICmpSGT(args[0], args[1], this->stmt_name("GT"))
						};
					}	
				} break;

				case TemplatedIntrinsic::Kind::GTE: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();

					const bool is_floating_point = this->context.getTypeManager().isFloatingPoint(arg_type);
					if(is_floating_point){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createFCmpGT(args[0], args[1], this->stmt_name("GTE"))
						};
					}

					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					if(is_unsigned){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createICmpUGT(args[0], args[1], this->stmt_name("GTE"))
						};
					}else{
						return evo::SmallVector<llvmint::Value>{
							this->builder.createICmpSGT(args[0], args[1], this->stmt_name("GTE"))
						};
					}	
				} break;


				///////////////////////////////////
				// remainder

				case TemplatedIntrinsic::Kind::And: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.createAnd(args[0], args[1], this->stmt_name("AND"))
					};
				} break;

				case TemplatedIntrinsic::Kind::Or: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.createOr(args[0], args[1], this->stmt_name("OR"))
					};	
				} break;

				case TemplatedIntrinsic::Kind::Xor: {
					return evo::SmallVector<llvmint::Value>{
						this->builder.createXor(args[0], args[1], this->stmt_name("XOR"))
					};
				} break;

				case TemplatedIntrinsic::Kind::SHL: {
					const bool may_wrap = instantiation.templateArgs[2].as<bool>();
					if(may_wrap){
						return evo::SmallVector<llvmint::Value>{
							this->builder.createSHL(args[0], args[1], false, false, this->stmt_name("SHL"))
						};
					}
						
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);


					const llvmint::Value shift_value = this->builder.createSHL(
						args[0], args[1], is_unsigned, !is_unsigned, this->stmt_name("SHL")
					);

					if(this->config.checkedMath){
						const llvmint::Value check_value = [&](){
							if(is_unsigned){
								return this->builder.createLSHR(
									shift_value, args[1], true, this->stmt_name("SHL.CHECK")
								);
							}else{
								return this->builder.createASHR(
									shift_value, args[1], true, this->stmt_name("SHL.CHECK")
								);
							}
						}();

						this->add_fail_assertion(
							this->builder.createICmpNE(check_value, args[0], this->stmt_name("SHL.CHECK_NEQ")),
							"SHL_CHECK",
							"Bit-shift-left overflow",
							func_call.location
						);
					}

					return evo::SmallVector<llvmint::Value>{shift_value};
				} break;

				case TemplatedIntrinsic::Kind::SHLSat: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);

					const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned 
						? llvmint::IRBuilder::IntrinsicID::ushlSat
						: llvmint::IRBuilder::IntrinsicID::sshlSat;

					return evo::SmallVector<llvmint::Value>{
						this->builder.createIntrinsicCall(
							intrinsic_id, this->get_type(arg_type), args, this->stmt_name("SHL_SAT")
						).asValue()
					};
				} break;

				case TemplatedIntrinsic::Kind::SHR: {
					const TypeInfo::ID arg_type = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID();
					const bool is_unsigned = this->context.getTypeManager().isUnsignedIntegral(arg_type);
					const bool is_exact = instantiation.templateArgs[2].as<bool>();

					const llvmint::Value shift_value = [&](){
						if(is_unsigned){
							return this->builder.createLSHR(args[0], args[1], is_exact, this->stmt_name("SHR"));
						}else{
							return this->builder.createASHR(args[0], args[1], is_exact, this->stmt_name("SHR"));
						}
					}();

					if(!is_exact && this->config.checkedMath){
						const llvmint::Value check_value = this->builder.createSHL(
							shift_value, args[1], true, false, this->stmt_name("SHR.CHECK")
						);

						this->add_fail_assertion(
							this->builder.createICmpNE(check_value, args[0], this->stmt_name("SHR.CHECK_NEQ")),
							"SHR_CHECK",
							"Bit-shift-right overflow",
							func_call.location
						);
					}

					return evo::SmallVector<llvmint::Value>{shift_value};
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



	auto ASGToLLVMIR::add_panic(std::string_view message, const ASG::Location& location) -> void {
		const llvmint::Constant panic_message = this->builder.getValueGlobalStrPtr(message);

		if(this->config.addSourceLocations){
			this->builder.createCall(
				*this->linked_functions.panic_with_location,
				{
					panic_message.asValue(),
					this->builder.getValueI32(this->current_source->getID().get()).asValue(),
					this->builder.getValueI32(location.line).asValue(),
					this->builder.getValueI32(location.collumn).asValue(),
				}
			);
		}else{
			this->builder.createCall(*this->linked_functions.panic, {panic_message.asValue()});
		}
	}

	auto ASGToLLVMIR::add_fail_assertion(
		const llvmint::Value& cond, std::string_view block_name, std::string_view message, const ASG::Location& location
	) -> void {
		const FuncInfo& current_func_info = this->get_current_func_info();

		const llvmint::BasicBlock fail_block = this->builder.createBasicBlock(
			current_func_info.func, std::format("{}.FAIL", block_name)
		);

		const llvmint::BasicBlock succeed_block = this->builder.createBasicBlock(
			current_func_info.func, std::format("{}.SUCCEED", block_name)
		);


		this->builder.createCondBranch(cond, fail_block, succeed_block);

		this->builder.setInsertionPoint(fail_block);
		this->add_panic(message, location);
		this->builder.createBranch(succeed_block);

		this->builder.setInsertionPoint(succeed_block);
	}



	auto ASGToLLVMIR::mangle_name(const ASG::Func& func) const -> std::string {
		return std::format(
			"PTHR.{}{}.{}",
			this->current_source->getID().get(),
			this->submangle_parent(func.parent),
			this->get_func_ident_name(func)
		);
	}

	auto ASGToLLVMIR::mangle_name(const ASG::Var& var) const -> std::string {
		const std::string_view var_ident = this->current_source->getTokenBuffer()[var.ident].getString();

		return std::format("PTHR.{}.{}", this->current_source->getID().get(), var_ident);
	}


	auto ASGToLLVMIR::submangle_parent(const ASG::Parent& parent) const -> std::string {
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


	auto ASGToLLVMIR::get_func_ident_name(const ASG::Func& func) const -> std::string {
		const Token::ID func_ident_token_id = this->current_source->getASTBuffer().getIdent(func.name);
		auto name = std::string(this->current_source->getTokenBuffer()[func_ident_token_id].getString());

		if(func.instanceID.has_value()){
			name += std::format("-i{}", func.instanceID.get());
		}

		return name;
	}


	auto ASGToLLVMIR::stmt_name(std::string_view str) const -> std::string {
		if(this->config.useReadableRegisters) [[unlikely]] {
			return std::string(str);
		}else{
			return std::string();
		}
	}


	template<class... Args>
	auto ASGToLLVMIR::stmt_name(std::format_string<Args...> fmt, Args&&... args) const -> std::string {
		if(this->config.useReadableRegisters) [[unlikely]] {
			return std::format(fmt, std::forward<Args...>(args)...);
		}else{
			return std::string();
		}
	}



	auto ASGToLLVMIR::get_func_info(ASG::Func::LinkID link_id) const -> const FuncInfo& {
		const auto& info_find = this->func_infos.find(link_id);
		evo::debugAssert(info_find != this->func_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}

	auto ASGToLLVMIR::get_current_func_info() const -> const FuncInfo& {
		evo::debugAssert(this->current_func_link_id.has_value(), "current_func_link_id is not set");
		return this->get_func_info(*this->current_func_link_id);
	}


	auto ASGToLLVMIR::get_var_info(ASG::Var::LinkID link_id) const -> const VarInfo& {
		const auto& info_find = this->var_infos.find(link_id);
		evo::debugAssert(info_find != this->var_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}

	auto ASGToLLVMIR::get_param_info(ASG::Param::LinkID link_id) const -> const ParamInfo& {
		const auto& info_find = this->param_infos.find(link_id);
		evo::debugAssert(info_find != this->param_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}

	auto ASGToLLVMIR::get_return_param_info(ASG::ReturnParam::LinkID link_id) const -> const ReturnParamInfo& {
		const auto& info_find = this->return_param_infos.find(link_id);
		evo::debugAssert(info_find != this->return_param_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}



	auto ASGToLLVMIR::get_value_size(uint64_t val) const -> llvmint::ConstantInt {
		return this->builder.getValueI64(val);
	}
	
}