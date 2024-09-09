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





	auto ASGToLLVMIR::lower_global_var(const ASG::Var::ID& var_id) -> void {
		const ASG::Var& asg_var = this->current_source->getASGBuffer().getVar(var_id);

		if(asg_var.kind == AST::VarDecl::Kind::Def){ return; } // make sure not to emit def variables

		const llvmint::Type type = this->get_type(asg_var.typeID);

		llvmint::GlobalVariable global_var = [&](){
			if(asg_var.expr.kind() == ASG::Expr::Kind::Zeroinit) [[unlikely]] {
				const TypeInfo& var_type_info = this->context.getTypeManager().getTypeInfo(asg_var.typeID);
				evo::debugAssert(var_type_info.isPointer() == false, "cannot zeroinit a pointer");
				evo::debugAssert(var_type_info.isOptionalNotPointer() == false, "not supported");

				if(var_type_info.baseTypeID().kind() != BaseType::Kind::Builtin){
					return this->module.createGlobalZeroinit(
						type, llvmint::LinkageType::Internal, asg_var.isConst, this->mangle_name(asg_var)
					);
				}

				const BaseType::Builtin& var_builtin_type = this->context.getTypeManager().getBuiltin(
					var_type_info.baseTypeID().builtinID()
				);
				
				llvmint::Constant constant_value = [&](){
					switch(var_builtin_type.kind()){
						case Token::Kind::TypeInt:       case Token::Kind::TypeISize:      case Token::Kind::TypeI_N:
						case Token::Kind::TypeUInt:      case Token::Kind::TypeUSize:      case Token::Kind::TypeUI_N:
						case Token::Kind::TypeCShort:    case Token::Kind::TypeCUShort:    case Token::Kind::TypeCInt:
						case Token::Kind::TypeCUInt:     case Token::Kind::TypeCLong:      case Token::Kind::TypeCULong:
						case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong: 
						case Token::Kind::TypeCLongDouble: {
							const auto integer_type = llvmint::IntegerType(
								(llvm::IntegerType*)this->get_type(var_builtin_type).native()
							);
							return this->builder.getValueIntegral(integer_type, 0).asConstant();
						} break;

						case Token::Kind::TypeF16: case Token::Kind::TypeBF16: case Token::Kind::TypeF32:
						case Token::Kind::TypeF64: case Token::Kind::TypeF80:  case Token::Kind::TypeF128: {
							const llvmint::Type float_type = this->get_type(var_builtin_type);
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

		llvmint::Function llvm_func = this->module.createFunction(this->mangle_name(func), func_proto, linkage);
		llvm_func.setNoThrow();
		llvm_func.setCallingConv(llvmint::CallingConv::Fast);

		const auto asg_func_link_id = ASG::Func::LinkID(this->current_source->getID(), func_id);
		this->func_infos.emplace(asg_func_link_id, FuncInfo(llvm_func));

		for(evo::uint i = 0; const BaseType::Function::Param& param : func_type.params()){
			llvm_func.getArg(i).setName(this->current_source->getTokenBuffer()[param.ident].getString());

			i += 1;
		}

		if(func_type.hasNamedReturns()){
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
			break; case ASG::Stmt::Kind::Var:         this->lower_var(stmt.varID());
			break; case ASG::Stmt::Kind::FuncCall:    this->lower_func_call(asg_buffer.getFuncCall(stmt.funcCallID()));
			break; case ASG::Stmt::Kind::Assign:      this->lower_assign(asg_buffer.getAssign(stmt.assignID()));
			break; case ASG::Stmt::Kind::MultiAssign:
				this->lower_multi_assign(asg_buffer.getMultiAssign(stmt.multiAssignID()));
			break; case ASG::Stmt::Kind::Return:      this->lower_return(asg_buffer.getReturn(stmt.returnID()));
			break; default: evo::debugFatalBreak("Unknown or unsupported stmt kind");
		}
	}


	auto ASGToLLVMIR::lower_var(const ASG::Var::ID& var_id) -> void {
		const ASG::Var& asg_var = this->current_source->getASGBuffer().getVar(var_id);

		const llvmint::Alloca var_alloca = this->builder.createAlloca(
			this->get_type(asg_var.typeID),
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
					this->get_value_size(this->context.getTypeManager().sizeOf(asg_var.typeID)).asValue(),
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
			case BaseType::Kind::Builtin: {
				const BaseType::Builtin::ID builtin_id = type_info.baseTypeID().builtinID();
				const BaseType::Builtin& builtin = this->context.getTypeManager().getBuiltin(builtin_id);
				return this->get_type(builtin);
			} break;

			case BaseType::Kind::Function: {
				return this->builder.getTypePtr().asType();
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported builtin kind");
	}


	auto ASGToLLVMIR::get_type(const BaseType::Builtin& builtin) const -> llvmint::Type {
		switch(builtin.kind()){
			case Token::Kind::TypeInt: case Token::Kind::TypeUInt: {
				return this->builder.getTypeI_N(
					evo::uint(this->context.getTypeManager().sizeOfGeneralRegister() * 8)
				).asType();
			} break;

			case Token::Kind::TypeISize: case Token::Kind::TypeUSize:{
				return this->builder.getTypeI_N(
					evo::uint(this->context.getTypeManager().sizeOfPtr() * 8)
				).asType();
			} break;

			case Token::Kind::TypeI_N: case Token::Kind::TypeUI_N: {
				return this->builder.getTypeI_N(evo::uint(builtin.bitWidth())).asType();
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

			case Token::Kind::TypeCShort: case Token::Kind::TypeCUShort: 
				return this->builder.getTypeI16().asType();

			case Token::Kind::TypeCInt: case Token::Kind::TypeCUInt: {
				if(this->context.getTypeManager().platform() == core::Platform::Windows){
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
				if(this->context.getTypeManager().platform() == core::Platform::Windows){
					return this->builder.getTypeF64();
				}else{
					return this->builder.getTypeF80();
				}
			} break;

			default: evo::debugFatalBreak(
				"Unknown or unsupported builtin-type: {}", evo::to_underlying(builtin.kind())
			);
		}
	}




	auto ASGToLLVMIR::get_func_type(const BaseType::Function& func_type) const -> llvmint::FunctionType {
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

		return this->builder.getFuncProto(return_type, param_types, false);
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

				const bool optimize_with_copy = func_type.params()[param_info.index].optimizeWithCopy;

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
				const llvmint::ConstantInt value = this->builder.getValueIntegral(integer_type, literal_int.value);
				if(get_pointer_to_value == false){ return value.asValue(); }

				const llvmint::Alloca alloca = this->builder.createAlloca(literal_type);
				this->builder.createStore(alloca, value.asValue(), false);
				return alloca.asValue();
			} break;

			case ASG::Expr::Kind::LiteralFloat: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralFloat& literal_float = asg_buffer.getLiteralFloat(expr.literalFloatID());

				const llvmint::Type literal_type = this->get_type(*literal_float.typeID);
				const llvmint::Constant value = this->builder.getValueFloat(literal_type, literal_float.value);
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

				const bool optimize_with_copy = func_type.params()[param_info.index].optimizeWithCopy;

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
				return this->builder.getValueIntegral(integer_type, literal_int.value).asConstant();
			} break;

			case ASG::Expr::Kind::LiteralFloat: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralFloat& literal_float = asg_buffer.getLiteralFloat(expr.literalFloatID());

				const llvmint::Type literal_type = this->get_type(*literal_float.typeID);
				return this->builder.getValueFloat(literal_type, literal_float.value);
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

		// to statisfy MSVC warning C4715
		evo::unreachable();
	}


	auto ASGToLLVMIR::lower_returning_func_call(const ASG::FuncCall& func_call, bool get_pointer_to_value)
	-> evo::SmallVector<llvmint::Value> {
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


		auto return_values = evo::SmallVector<llvmint::Value>();
		if(func_type.hasNamedReturns()){
			for(const BaseType::Function::ReturnParam& return_param : func_type.returnParams()){
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
				for(size_t i = func_type.params().size(); i < args.size(); i+=1){
					return_values.emplace_back(args[i]);
				}

				return return_values;
			}

			for(size_t i = func_type.params().size(); i < args.size(); i+=1){
				return_values.emplace_back(
					this->builder.createLoad(
						args[i], this->get_type(func_type.returnParams().front().typeID), false
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
			
			const llvmint::Type return_type = this->get_type(func_type.returnParams()[0].typeID);

			const llvmint::Alloca alloca = this->builder.createAlloca(return_type);
			this->builder.createStore(alloca, func_call_value, false);

			return_values.emplace_back(alloca.asValue());
			return return_values;
		}
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