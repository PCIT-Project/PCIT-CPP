//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./ASGToLLVMIR.h"

#include "../include/Context.h"
#include "../include/Source.h"
#include "../include/SourceManager.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{

	auto ASGToLLVMIR::lower() -> void {
		for(const Source::ID& source_id : this->context.getSourceManager()){
			this->current_source = &this->context.getSourceManager()[source_id];

			for(const ASG::Func::ID func_id : this->current_source->getASGBuffer().getFuncs()){
				this->lower_func_decl(func_id);
			}
		}

		for(const Source::ID& source_id : this->context.getSourceManager()){
			this->current_source = &this->context.getSourceManager()[source_id];

			for(const ASG::Func::ID func_id : this->current_source->getASGBuffer().getFuncs()){
				this->lower_func_body(func_id);
			}
		}

		this->current_source = nullptr;
	}


	auto ASGToLLVMIR::addRuntime() -> void {
		const FuncInfo& entry_func_info = this->get_func_info(*this->context.getEntry());

		const llvmint::FunctionType main_func_proto = this->builder.getFuncProto(
			this->builder.getTypeI32(), {}, false
		);

		llvmint::Function main_func = this->module.createFunction(
			"main", main_func_proto, llvmint::LinkageType::External
		);
		main_func.setNoThrow();
		main_func.setCallingConv(llvmint::CallingConv::Fast);

		const llvmint::BasicBlock begin_block = this->builder.createBasicBlock(main_func, "begin");
		this->builder.setInsertionPoint(begin_block);

		const llvmint::Value begin_ret = static_cast<llvmint::Value>(
			this->builder.createCall(entry_func_info.func, {}, '\0')
		);
		this->builder.createRet(this->builder.createZExt(begin_ret, this->builder.getTypeI32()));
	}


	auto ASGToLLVMIR::lower_func_decl(ASG::Func::ID func_id) -> void {
		const ASG::Func& func = this->current_source->getASGBuffer().getFunc(func_id);
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.baseTypeID.funcID());

		const bool has_named_returns = func_type.hasNamedReturns();

		auto param_types = evo::SmallVector<llvmint::Type>();
		param_types.reserve(func_type.params().size());
		for(const BaseType::Function::Param& param : func_type.params()){
			if(param.optimizeWithCopy){
				param_types.emplace_back(this->get_type(param.typeID));
			}else{
				param_types.emplace_back(this->builder.getTypePtr());
			}
		}

		if(has_named_returns){
			for(size_t i = 0; i < func_type.returnParams().size(); i+=1){
				param_types.emplace_back(this->builder.getTypePtr());
			}
		}

		const llvmint::Type return_type = [&](){
			if(has_named_returns){
				return this->builder.getTypeVoid();
			}else{
				return this->get_type(func_type.returnParams()[0].typeID);
			}
		}();

		const llvmint::FunctionType func_proto = this->builder.getFuncProto(return_type, param_types, false);
		const auto linkage = llvmint::LinkageType::Internal;

		llvmint::Function llvm_func = this->module.createFunction(this->mangle_name(func), func_proto, linkage);
		llvm_func.setNoThrow();
		llvm_func.setCallingConv(llvmint::CallingConv::Fast);

		const auto asg_func_link_id = ASG::Func::LinkID(this->current_source->getID(), func_id);
		this->func_infos.emplace(asg_func_link_id, FuncInfo(llvm_func));

		for(evo::uint i = 0; const BaseType::Function::Param& param : func_type.params()){
			llvm_func.getArg(i).setName(this->current_source->getTokenBuffer()[param.ident].getString());

			i += 1;
		}

		if(has_named_returns){
			const evo::uint first_return_param_index = evo::uint(func_type.params().size());
			evo::uint i = first_return_param_index;
			for(const BaseType::Function::ReturnParam& ret_param : func_type.returnParams()){
				llvm_func.getArg(i).setName(
					std::format("ret.{}", this->current_source->getTokenBuffer()[*ret_param.ident].getString())
				);

				const evo::uint return_i = i - first_return_param_index;

				this->return_param_infos.emplace(
					ASG::ReturnParam::LinkID(asg_func_link_id, func.returnParams[return_i]),
					ReturnParamInfo(
						llvm_func.getArg(i), this->get_type(ret_param.typeID.typeID()), return_i
					)
				);

				i += 1;
			}			
		}

		if(func_type.params().empty()){
			this->builder.createBasicBlock(llvm_func, "begin");
			
		}else{
			const llvmint::BasicBlock setup_block = this->builder.createBasicBlock(llvm_func, "setup");
			const llvmint::BasicBlock begin_block = this->builder.createBasicBlock(llvm_func, "begin");

			this->builder.setInsertionPoint(setup_block);

			for(evo::uint i = 0; const BaseType::Function::Param& param : func_type.params()){
				const std::string_view param_name = this->current_source->getTokenBuffer()[param.ident].getString();

				const llvmint::Alloca param_alloca = [&](){
					if(param.optimizeWithCopy){
						return this->builder.createAlloca(
							this->get_type(param.typeID), this->stmt_name("{}.alloca", param_name)
						);

					}else{
						return this->builder.createAlloca(
							this->builder.getTypePtr(), this->stmt_name("{}.alloca", param_name)
						);
					}
				}();

				this->builder.createStore(param_alloca, static_cast<llvmint::Value>(llvm_func.getArg(i)), false);

				this->param_infos.emplace(
					ASG::Param::LinkID(asg_func_link_id, func.params[i]),
					ParamInfo(param_alloca, this->get_type(param.typeID), i)
				);

				i += 1;
			}

			this->builder.createBranch(begin_block);
		}
	}


	auto ASGToLLVMIR::lower_func_body(ASG::Func::ID func_id) -> void {
		const ASG::Func& asg_func = this->current_source->getASGBuffer().getFunc(func_id);
		this->current_func = &asg_func;

		auto link_id = ASG::Func::LinkID(this->current_source->getID(), func_id);
		const FuncInfo& func_info = this->get_func_info(link_id);

		this->builder.setInsertionPointAtBack(func_info.func);

		for(const ASG::Stmt& stmt : asg_func.stmts){
			this->lower_stmt(stmt);
		}

		if(asg_func.isTerminated == false){
			this->builder.createRet();
		}

		this->current_func = nullptr;
	}



	auto ASGToLLVMIR::lower_stmt(const ASG::Stmt& stmt) -> void {
		const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();

		switch(stmt.kind()){
			break; case ASG::Stmt::Kind::Var:      this->lower_var(stmt.varID());
			break; case ASG::Stmt::Kind::FuncCall: this->lower_func_call(asg_buffer.getFuncCall(stmt.funcCallID()));
			break; case ASG::Stmt::Kind::Assign:   this->lower_assign(asg_buffer.getAssign(stmt.assignID()));
			break; case ASG::Stmt::Kind::Return:   this->lower_return(asg_buffer.getReturn(stmt.returnID()));
		}
	}


	auto ASGToLLVMIR::lower_var(const ASG::Var::ID var_id) -> void {
		const ASG::Var& var = this->current_source->getASGBuffer().getVar(var_id);

		const llvmint::Alloca var_alloca = this->builder.createAlloca(
			this->get_type(var.typeID),
			this->stmt_name("{}.alloca", this->current_source->getTokenBuffer()[var.ident].getString())
		);

		switch(var.expr.kind()){
			case ASG::Expr::Kind::Uninit: {
				// do nothing
			} break;

			case ASG::Expr::Kind::Zeroinit: {
				this->builder.createMemSetInline(
					var_alloca,
					this->builder.getValueI8(0),
					this->get_value_size(this->context.getTypeManager().sizeOf(var.typeID)),
					false
				);
			} break;

			default: {
				this->builder.createStore(var_alloca, this->get_value(var.expr), false);
			} break;
		}

		this->var_infos.emplace(ASG::Var::LinkID(this->current_source->getID(), var_id), VarInfo(var_alloca));
	}


	auto ASGToLLVMIR::lower_func_call(const ASG::FuncCall& func_call) -> void {
		const FuncInfo& func_info = this->get_func_info(func_call.target);
		const Source& target_source = this->context.getSourceManager()[func_call.target.sourceID()];
		const ASG::Func& asg_func_target = target_source.getASGBuffer().getFunc(func_call.target.funcID());
		const BaseType::Function& func_target_type = 
			this->context.getTypeManager().getFunction(asg_func_target.baseTypeID.funcID());

		auto args = evo::SmallVector<llvmint::Value>();
		args.reserve(func_call.args.size());
		for(size_t i = 0; const ASG::Expr& arg : func_call.args){
			const bool optimize_with_copy = func_target_type.params()[i].optimizeWithCopy;
			args.emplace_back(this->get_value(arg, !optimize_with_copy));

			i += 1;
		}

		this->builder.createCall(func_info.func, args);
	}


	auto ASGToLLVMIR::lower_assign(const ASG::Assign& assign) -> void {
		llvmint::Value lhs = this->get_concrete_value(assign.lhs);
		llvmint::Value rhs = this->get_value(assign.rhs);

		this->builder.createStore(lhs, rhs, false);
	}

	auto ASGToLLVMIR::lower_return(const ASG::Return& return_stmt) -> void {
		if(return_stmt.value.has_value() == false){
			this->builder.createRet();
			return;
		}

		this->builder.createRet(this->get_value(*return_stmt.value));
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
				return static_cast<llvmint::Type>(this->builder.getTypePtr());
			}else{
				evo::fatalBreak("Optional is unsupported");	
			}
		}

		switch(type_info.baseTypeID().kind()){
			case BaseType::Kind::Builtin: {
				const BaseType::Builtin::ID builtin_id = type_info.baseTypeID().builtinID();
				const BaseType::Builtin& builtin = this->context.getTypeManager().getBuiltin(builtin_id);

				// TODO: select correct type based on target platform / architecture
				switch(builtin.kind()){
					case Token::Kind::TypeInt: case Token::Kind::TypeUInt: {
						return static_cast<llvmint::Type>(
							this->builder.getTypeI_N(
								evo::uint(this->context.getTypeManager().sizeOfGeneralRegister() * 8)
							)
						);
					} break;

					case Token::Kind::TypeISize: case Token::Kind::TypeUSize:{
						return static_cast<llvmint::Type>(
							this->builder.getTypeI_N(evo::uint(this->context.getTypeManager().sizeOfPtr() * 8))
						);
					} break;

					case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N: {
						return static_cast<llvmint::Type>(this->builder.getTypeI_N(evo::uint(builtin.bitWidth())));
					} break;

					case Token::Kind::TypeF16: return static_cast<llvmint::Type>(this->builder.getTypeF16()); 
					case Token::Kind::TypeBF16: return static_cast<llvmint::Type>(this->builder.getTypeBF16()); 
					case Token::Kind::TypeF32: return static_cast<llvmint::Type>(this->builder.getTypeF32()); 
					case Token::Kind::TypeF64: return static_cast<llvmint::Type>(this->builder.getTypeF64()); 
					case Token::Kind::TypeF80: return static_cast<llvmint::Type>(this->builder.getTypeF80());
					case Token::Kind::TypeF128: return static_cast<llvmint::Type>(this->builder.getTypeF128());
					case Token::Kind::TypeByte: return static_cast<llvmint::Type>(this->builder.getTypeI8());
					case Token::Kind::TypeBool: return static_cast<llvmint::Type>(this->builder.getTypeBool()); 
					case Token::Kind::TypeChar: return static_cast<llvmint::Type>(this->builder.getTypeI8());
					case Token::Kind::TypeRawPtr: return static_cast<llvmint::Type>(this->builder.getTypePtr());

					case Token::Kind::TypeCShort: case Token::Kind::TypeCUShort: 
						return static_cast<llvmint::Type>(this->builder.getTypeI16()); 

					case Token::Kind::TypeCInt: case Token::Kind::TypeCUInt: {
						if(this->context.getTypeManager().platform() == core::Platform::Windows){
							return static_cast<llvmint::Type>(this->builder.getTypeI32());
						}else{
							return static_cast<llvmint::Type>(this->builder.getTypeI64());
						}
					} break;

					case Token::Kind::TypeCLong: case Token::Kind::TypeCULong:
						return static_cast<llvmint::Type>(this->builder.getTypeI32()); 

					case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong:
						return static_cast<llvmint::Type>(this->builder.getTypeI64());

					case Token::Kind::TypeCLongDouble: {
						if(this->context.getTypeManager().platform() == core::Platform::Windows){
							return static_cast<llvmint::Type>(this->builder.getTypeF64());
						}else{
							return static_cast<llvmint::Type>(this->builder.getTypeF80());
						}
					} break;

					default: evo::debugFatalBreak(
						"Unknown or unsupported builtin-type: {}", evo::to_underlying(builtin.kind())
					);
				}
			} break;

			case BaseType::Kind::Function: {
				evo::fatalBreak("Function types are unsupported");
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported builtin kind");
	}


	auto ASGToLLVMIR::get_concrete_value(const ASG::Expr& expr) -> llvmint::Value {
		switch(expr.kind()){
			case ASG::Expr::Kind::Uninit:       case ASG::Expr::Kind::Zeroinit:    case ASG::Expr::Kind::LiteralInt:
			case ASG::Expr::Kind::LiteralFloat: case ASG::Expr::Kind::LiteralBool: case ASG::Expr::Kind::LiteralChar:
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
				return static_cast<llvmint::Value>(var_info.alloca);
			} break;

			case ASG::Expr::Kind::Func: {
				evo::fatalBreak("Function values are unsupported");
			} break;

			case ASG::Expr::Kind::Param: {
				const ParamInfo& param_info = this->get_param_info(expr.paramLinkID());
				
				const BaseType::Function& func_type = this->context.getTypeManager().getFunction(
					this->current_func->baseTypeID.funcID()
				);

				const bool optimize_with_copy = func_type.params()[param_info.index].optimizeWithCopy;

				if(optimize_with_copy){
					return static_cast<llvmint::Value>(param_info.alloca);

				}else{
					return static_cast<llvmint::Value>(this->builder.createLoad(
						param_info.alloca, this->stmt_name("param.ptr_lookup")
					));
				}
			} break;

			case ASG::Expr::Kind::ReturnParam: {
				const ReturnParamInfo& ret_param_info = this->get_return_param_info(expr.returnParamLinkID());

				return static_cast<llvmint::Value>(ret_param_info.arg);
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
				const llvmint::ConstantInt value = this->builder.getValueIntegral(integer_type, literal_int.value);
				if(get_pointer_to_value == false){ return static_cast<llvmint::Value>(value); }

				const llvmint::Alloca alloca = this->builder.createAlloca(literal_type);
				this->builder.createStore(alloca, static_cast<llvmint::Value>(value), false);
				return static_cast<llvmint::Value>(alloca);
			} break;

			case ASG::Expr::Kind::LiteralFloat: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralFloat& literal_float = asg_buffer.getLiteralFloat(expr.literalFloatID());

				const llvmint::Type literal_type = this->get_type(*literal_float.typeID);
				const llvmint::Constant value = this->builder.getValueFloat(literal_type, literal_float.value);
				if(get_pointer_to_value == false){ return static_cast<llvmint::Value>(value); }

				const llvmint::Alloca alloca = this->builder.createAlloca(literal_type);
				this->builder.createStore(alloca, static_cast<llvmint::Value>(value), false);
				return static_cast<llvmint::Value>(alloca);
			} break;

			case ASG::Expr::Kind::LiteralBool: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const bool bool_value = asg_buffer.getLiteralBool(expr.literalBoolID()).value;

				const llvmint::ConstantInt value = this->builder.getValueBool(bool_value);
				if(get_pointer_to_value == false){ return static_cast<llvmint::Value>(value); }

				const llvmint::Alloca alloca = this->builder.createAlloca(this->builder.getTypeI8());
				this->builder.createStore(alloca, static_cast<llvmint::Value>(value), false);
				return static_cast<llvmint::Value>(alloca);
			} break;

			case ASG::Expr::Kind::LiteralChar: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const char char_value = asg_buffer.getLiteralChar(expr.literalCharID()).value;

				const llvmint::ConstantInt value = this->builder.getValueI8(uint8_t(char_value));
				if(get_pointer_to_value == false){ return static_cast<llvmint::Value>(value); }

				const llvmint::Alloca alloca = this->builder.createAlloca(this->builder.getTypeI8());
				this->builder.createStore(alloca, static_cast<llvmint::Value>(value), false);
				return static_cast<llvmint::Value>(alloca);
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

				return this->builder.createLoad(value, this->get_type(deref_expr.typeID));
			} break;

			case ASG::Expr::Kind::AddrOf: {
				const ASG::Expr& addr_of_expr = this->current_source->getASGBuffer().getAddrOf(expr.addrOfID());
				const llvmint::Value address = this->get_value(addr_of_expr, true);
				if(get_pointer_to_value == false){ return address; }

				const llvmint::Alloca alloca = this->builder.createAlloca(this->builder.getTypePtr());
				this->builder.createStore(alloca, address, false);
				return static_cast<llvmint::Value>(alloca);
			} break;

			case ASG::Expr::Kind::FuncCall: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::FuncCall& func_call = asg_buffer.getFuncCall(expr.funcCallID());
				const ASG::Func::LinkID& func_link_id = func_call.target;
				const FuncInfo& func_info = this->get_func_info(func_link_id);

				const Source& linked_source = this->context.getSourceManager()[func_link_id.sourceID()];
				const ASG::Func& asg_func = linked_source.getASGBuffer().getFunc(func_link_id.funcID());
				const BaseType::Function& func_type = this->context.getTypeManager().getFunction(
					asg_func.baseTypeID.funcID()
				);

				auto args = evo::SmallVector<llvmint::Value>();
				args.reserve(func_call.args.size());
				for(size_t i = 0; const ASG::Expr& arg : func_call.args){
					const bool optimize_with_copy = func_type.params()[i].optimizeWithCopy;
					args.emplace_back(this->get_value(arg, !optimize_with_copy));

					i += 1;
				}

				if(func_type.hasNamedReturns()){
					for(const BaseType::Function::ReturnParam& return_param : func_type.returnParams()){
						const llvmint::Alloca alloca = this->builder.createAlloca(
							this->get_type(return_param.typeID),
							this->stmt_name(
								"RET_PARAM.{}.alloca",
								this->current_source->getTokenBuffer()[*return_param.ident].getString()
							)
						);

						args.emplace_back(static_cast<llvmint::Value>(alloca));
					}

					this->builder.createCall(func_info.func, args);

					if(get_pointer_to_value){ return args.back(); }

					return static_cast<llvmint::Value>(
						this->builder.createLoad(args.back(), this->get_type(func_type.returnParams().front().typeID))
					);

				}else{
					const llvmint::Value func_call_value = this->builder.createCall(func_info.func, args);
					if(get_pointer_to_value == false){ return func_call_value; }

					
					const llvmint::Type return_type = this->get_type(func_type.returnParams()[0].typeID);

					const llvmint::Alloca alloca = this->builder.createAlloca(return_type);
					this->builder.createStore(alloca, func_call_value, false);
					return static_cast<llvmint::Value>(alloca);
				}

			} break;

			case ASG::Expr::Kind::Var: {
				const VarInfo& var_info = this->get_var_info(expr.varLinkID());
				if(get_pointer_to_value){
					return static_cast<llvmint::Value>(var_info.alloca);
				}

				const llvmint::LoadInst load_inst = this->builder.createLoad(
					var_info.alloca, this->stmt_name("VAR.load")
				);
				return static_cast<llvmint::Value>(load_inst);
			} break;

			case ASG::Expr::Kind::Func: {
				evo::fatalBreak("ASG::Expr::Kind::Func is unsupported");
			} break;

			case ASG::Expr::Kind::Param: {
				const ParamInfo& param_info = this->get_param_info(expr.paramLinkID());

				const BaseType::Function& func_type = this->context.getTypeManager().getFunction(
					this->current_func->baseTypeID.funcID()
				);

				const bool optimize_with_copy = func_type.params()[param_info.index].optimizeWithCopy;

				if(optimize_with_copy){
					if(get_pointer_to_value){
						return static_cast<llvmint::Value>(param_info.alloca);
					}else{
						return this->builder.createLoad(param_info.alloca, this->stmt_name("PARAM.load"));
					}

				}else{
					const llvmint::LoadInst load_inst = this->builder.createLoad(
						param_info.alloca, this->stmt_name("PARAM.ptr_lookup")
					);

					if(get_pointer_to_value) [[unlikely]] {
						return static_cast<llvmint::Value>(load_inst);
					}else{
						return this->builder.createLoad(
							static_cast<llvmint::Value>(load_inst), param_info.type, this->stmt_name("PARAM.load")
						);
					}
				}
			} break;

			case ASG::Expr::Kind::ReturnParam: {
				const ReturnParamInfo& ret_param_info = this->get_return_param_info(expr.returnParamLinkID());

				if(get_pointer_to_value) [[unlikely]] {
					return static_cast<llvmint::Value>(ret_param_info.arg);
				}else{
					const llvmint::LoadInst load_inst = this->builder.createLoad(
						ret_param_info.arg, ret_param_info.type, this->stmt_name("RET_PARAM.load")
					);
					return static_cast<llvmint::Value>(load_inst);
				}
			} break;

		}

		evo::debugFatalBreak("Unknown or unsupported expr kind");
	}


	auto ASGToLLVMIR::mangle_name(const ASG::Func& func) const -> std::string {
		return std::format(
			"PTHR.{}{}.{}",
			this->current_source->getID().get(),
			this->submangle_parent(func.parent),
			this->get_func_ident_name(func)
		);
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