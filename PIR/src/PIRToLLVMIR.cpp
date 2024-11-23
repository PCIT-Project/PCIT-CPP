////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./PIRToLLVMIR.h"




#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::pir{
	
	auto PIRToLLVMIR::lower() -> void {
		for(const StructType& struct_type : this->module.getStructTypeIter()){
			this->lower_struct_type(struct_type);
		}

		for(const GlobalVar& global_var : this->module.getGlobalVarIter()){
			this->lower_global_var(global_var);
		}

		for(const FunctionDecl& function_decl : this->module.getFunctionDeclIter()){
			this->lower_function_decl(function_decl);
		}


		auto func_setups = std::vector<FuncLoweredSetup>();
		for(const Function& function : this->module.getFunctionIter()){
			func_setups.emplace_back(this->lower_function_setup(function));
		}

		for(const FuncLoweredSetup& func_setup : func_setups){
			this->lower_func_body(func_setup.func, func_setup.llvm_func);
		}
	}


	auto PIRToLLVMIR::lower_struct_type(const StructType& struct_type) -> void {
		auto members = evo::SmallVector<llvmint::Type>();
		for(const Type& member : struct_type.members){
			members.emplace_back(this->get_type(member));
		}

		const llvmint::StructType llvm_struct_type = this->builder.createStructType(
			members, struct_type.isPacked, struct_type.name
		);

		this->struct_types.emplace(struct_type.name, llvm_struct_type);
	}

	auto PIRToLLVMIR::lower_global_var(const GlobalVar& global) -> void {
		const llvmint::Type global_type = this->get_type(global.type);
		const llvmint::LinkageType linkage = this->get_linkage(global.linkage);

		const llvmint::GlobalVariable llvm_global_var = global.value.visit(
			[&](const auto& value) -> llvmint::GlobalVariable {
				using ValueT = std::decay_t<decltype(value)>;

				if constexpr(std::is_same<ValueT, Expr>()){

					return this->llvm_module.createGlobal(
						this->get_constant_value(value), global_type, linkage, global.isConstant, global.name
					);

				}else if constexpr(std::is_same<ValueT, GlobalVar::Zeroinit>()){
					switch(global.type.getKind()){
						case Type::Kind::Signed: case Type::Kind::Unsigned: {
							return this->llvm_module.createGlobal(
								this->builder.getValueI_N(global.type.getWidth(), 0).asConstant(),
								global_type,
								linkage,
								global.isConstant,
								global.name
							);
						} break;

						case Type::Kind::Float: {
							const llvmint::Constant zero_value = [&](){
								switch(global.type.getWidth()){
									case 16:  return this->builder.getValueF16(0);
									case 32:  return this->builder.getValueF32(0);
									case 64:  return this->builder.getValueF64(0);
									case 80:  return this->builder.getValueF80(0);
									case 128: return this->builder.getValueF128(0);
								}

								evo::debugFatalBreak("Unknown float width");
							}();

							return this->llvm_module.createGlobal(
								zero_value, global_type, linkage, global.isConstant, global.name
							);
						} break;

						case Type::Kind::BFloat: {
							return this->llvm_module.createGlobal(
								this->builder.getValueBF16(0),
								global_type,
								linkage,
								global.isConstant,
								global.name
							);
						} break;

						default: {
							return this->llvm_module.createGlobalZeroinit(
								global_type, linkage, global.isConstant, global.name
							);
						} break;
					}
					
				}else if constexpr(std::is_same<ValueT, GlobalVar::Uninit>()){
					return this->llvm_module.createGlobalUninit(
						global_type, linkage, global.isConstant, global.name
					);

				}else{
					static_assert(false, "Unsupported global var value kind");
				}
			}
		);

		this->global_vars.emplace(global.name, llvm_global_var);
	}

	auto PIRToLLVMIR::lower_function_decl(const FunctionDecl& func_decl) -> void {
		auto param_types = evo::SmallVector<llvmint::Type>();
		for(const Parameter& param : func_decl.parameters){
			param_types.emplace_back(this->get_type(param.getType()));
		}

		const llvmint::FunctionType func_type = this->builder.getFuncProto(
			this->get_type(func_decl.returnType), param_types, false
		);

		const llvmint::LinkageType linkage = this->get_linkage(func_decl.linkage);

		llvmint::Function llvm_func_decl = this->llvm_module.createFunction(func_decl.name, func_type, linkage);
		llvm_func_decl.setNoThrow();
		llvm_func_decl.setCallingConv(this->get_calling_conv(func_decl.callingConvention));

		this->funcs.emplace(func_decl.name, llvm_func_decl);
	}


	
	auto PIRToLLVMIR::lower_function_setup(const Function& func) -> FuncLoweredSetup {
		auto param_types = evo::SmallVector<llvmint::Type>();
		for(const Parameter& param : func.getParameters()){
			param_types.emplace_back(this->get_type(param.getType()));
		}

		const llvmint::FunctionType func_type = this->builder.getFuncProto(
			this->get_type(func.getReturnType()), param_types, false
		);

		const llvmint::LinkageType linkage = this->get_linkage(func.getLinkage());

		llvmint::Function llvm_func = this->llvm_module.createFunction(func.getName(), func_type, linkage);
		llvm_func.setNoThrow();
		llvm_func.setCallingConv(this->get_calling_conv(func.getCallingConvention()));


		for(unsigned i = 0; const Parameter& param : func.getParameters()){
			llvmint::Argument arg = llvm_func.getArg(i);
			arg.setName(param.getName());
			this->args.emplace_back(arg);

			i += 1;
		}

		if(func.getAllocasRange().empty() == false){
			const llvmint::BasicBlock setup_block = this->builder.createBasicBlock(llvm_func, "_ALLOCAS_");

			this->builder.setInsertionPoint(setup_block);

			for(const Alloca& alloca_info : func.getAllocasRange()){
				const llvmint::Alloca llvm_alloca = this->builder.createAlloca(
					this->get_type(alloca_info.type), alloca_info.name
				);

				this->allocas.emplace(&alloca_info, llvm_alloca);
			}
		}

		this->funcs.emplace(func.getName(), llvm_func);

		return FuncLoweredSetup(func, llvm_func);
	}


	auto PIRToLLVMIR::lower_func_body(const Function& func, const llvmint::Function& llvm_func) -> void {
		auto basic_block_map = std::unordered_map<BasicBlock::ID, llvmint::BasicBlock>();

		this->reader.setTargetFunction(func);

		for(const BasicBlock::ID basic_block_id : func){
			const BasicBlock& basic_block = this->reader.getBasicBlock(basic_block_id);
			const llvmint::BasicBlock llvm_basic_block = 
				this->builder.createBasicBlock(llvm_func, basic_block.getName());

			basic_block_map.emplace(basic_block_id, llvm_basic_block);
		}

		if(func.getAllocasRange().empty() == false){
			this->builder.createBranch(basic_block_map.at(*func.begin()));
		}

		for(const BasicBlock::ID basic_block_id : func){
			const BasicBlock& basic_block = this->reader.getBasicBlock(basic_block_id);

			this->builder.setInsertionPoint(basic_block_map.at(basic_block_id));
			
			for(const Expr& stmt : basic_block){
				switch(stmt.getKind()){
					case Expr::Kind::None: evo::debugFatalBreak("Not a valid expr");
					case Expr::Kind::GlobalValue: evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::Number: evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::Boolean: evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::ParamExpr: evo::debugFatalBreak("Not a valid stmt");

					case Expr::Kind::Call: {
						const Call& call = this->reader.getCall(stmt);

						auto call_args = evo::SmallVector<llvmint::Value>();
						for(const Expr& arg : call.args){
							call_args.emplace_back(this->get_value(arg));
						}

						const llvmint::Value call_value = call.target.visit(
							[&](const auto& target) -> llvmint::Value {
								using TargetT = std::decay_t<decltype(target)>;

								if constexpr(std::is_same<TargetT, Function::ID>()){
									const Function& func_target = this->module.getFunction(target);
									return this->builder.createCall(
										this->funcs.at(func_target.getName()), call_args
									).asValue();

								}else if constexpr(std::is_same<TargetT, FunctionDecl::ID>()){
									const FunctionDecl& func_target = this->module.getFunctionDecl(target);

									llvmint::CallInst call_inst = 
										this->builder.createCall(this->funcs.at(func_target.name), call_args);
									call_inst.setCallingConv(
										this->get_calling_conv(func_target.callingConvention)
									);

									return call_inst.asValue();

								}else if constexpr(std::is_same<TargetT, PtrCall>()){
									const llvmint::FunctionType target_func_type = 
										this->get_func_type(target.funcType);
									const llvmint::Value func_target = this->get_value(target.location);

									llvmint::CallInst call_inst = 
										this->builder.createCall(func_target, target_func_type, call_args);
									call_inst.setCallingConv(
										this->get_calling_conv(
											this->module.getFunctionType(target.funcType).callingConvention
										)
									);

									return call_inst.asValue();

								}else{
									static_assert(false, "Unknown func call target");
								}
							}
						);

						this->stmt_values.emplace(stmt, call_value);
					} break;

					case Expr::Kind::CallVoid: {
						const CallVoid& call_void = this->reader.getCallVoid(stmt);

						auto call_args = evo::SmallVector<llvmint::Value>();
						for(const Expr& arg : call_void.args){
							call_args.emplace_back(this->get_value(arg));
						}

						call_void.target.visit([&](const auto& target) -> void {
							using TargetT = std::decay_t<decltype(target)>;

							if constexpr(std::is_same<TargetT, Function::ID>()){
								const Function& func_target = this->module.getFunction(target);
								this->builder.createCall(this->funcs.at(func_target.getName()), call_args);

							}else if constexpr(std::is_same<TargetT, FunctionDecl::ID>()){
								const FunctionDecl& func_target = this->module.getFunctionDecl(target);

								llvmint::CallInst call_inst = 
									this->builder.createCall(this->funcs.at(func_target.name), call_args);
								call_inst.setCallingConv(this->get_calling_conv(func_target.callingConvention));

							}else if constexpr(std::is_same<TargetT, PtrCall>()){
								const llvmint::FunctionType target_func_type = 
									this->get_func_type(target.funcType);
								const llvmint::Value func_target = this->get_value(target.location);

								llvmint::CallInst call_inst = 
									this->builder.createCall(func_target, target_func_type, call_args);
								call_inst.setCallingConv(
									this->get_calling_conv(
										this->module.getFunctionType(target.funcType).callingConvention
									)
								);
							}else{
								static_assert(false, "Unknown func call target");
							}
						});
					} break;

					case Expr::Kind::Ret: {
						const Ret& ret = this->reader.getRet(stmt);
						if(ret.value.has_value()){
							this->builder.createRet(this->get_value(*ret.value));
						}else{
							this->builder.createRet();
						}
					} break;

					case Expr::Kind::Branch: {
						const Branch& branch = this->reader.getBranch(stmt);
						this->builder.createBranch(basic_block_map.at(branch.target));
					} break;

					case Expr::Kind::Alloca: evo::debugFatalBreak("Not a valid stmt");

					case Expr::Kind::Load: {
						const Load& load = this->reader.getLoad(stmt);

						const llvmint::LoadInst load_inst = this->builder.createLoad(
							this->get_value(load.source),
							this->get_type(load.type),
							load.isVolatile,
							this->get_atomic_ordering(load.atomicOrdering),
							load.name
						);

						this->stmt_values.emplace(stmt, load_inst);
					} break;

					case Expr::Kind::Store: {
						const Store& store = this->reader.getStore(stmt);

						this->builder.createStore(
							this->get_value(store.destination),
							this->get_value(store.value),
							store.isVolatile,
							this->get_atomic_ordering(store.atomicOrdering)
						);
					} break;

					case Expr::Kind::Add: {
						const Add& add = this->reader.getAdd(stmt);
						const Type& add_type = this->reader.getExprType(add.lhs);

						const llvmint::Value lhs = this->get_value(add.lhs);
						const llvmint::Value rhs = this->get_value(add.rhs);

						const llvmint::Value add_value = [&](){
							switch(add_type.getKind()){
								case Type::Kind::Signed:
									return this->builder.createAdd(lhs, rhs, false, !add.mayWrap, add.name);
								case Type::Kind::Unsigned:
									return this->builder.createAdd(lhs, rhs, !add.mayWrap, false, add.name);
								case Type::Kind::Float:  return this->builder.createFAdd(lhs, rhs, add.name);
								case Type::Kind::BFloat: return this->builder.createFAdd(lhs, rhs, add.name);
								default: evo::debugFatalBreak("Unknown or unsupported add types");
							}
						}();

						this->stmt_values.emplace(stmt, add_value);
					} break;

					case Expr::Kind::AddWrap: {
						const AddWrap& add_wrap = this->reader.getAddWrap(stmt);
						const Type& add_type = this->reader.getExprType(add_wrap.lhs);
						const bool is_unsigned = add_type.getKind() == Type::Kind::Unsigned;

						const llvmint::IRBuilder::IntrinsicID intrinsic_id = is_unsigned
							? llvmint::IRBuilder::IntrinsicID::uaddOverflow
							: llvmint::IRBuilder::IntrinsicID::saddOverflow;

						const llvmint::Type return_type = this->builder.getStructType(
							{this->get_type(add_type), this->builder.getTypeBool().asType()}
						).asType();

						const llvmint::Value add_value = this->builder.createIntrinsicCall(
							intrinsic_id,
							return_type,
							{this->get_value(add_wrap.lhs), this->get_value(add_wrap.rhs)},
							"ADD_WRAP"
						).asValue();

						this->stmt_values.emplace(
							this->reader.extractAddWrapResult(stmt),
							this->builder.createExtractValue(add_value, {0}, add_wrap.resultName)
						);

						this->stmt_values.emplace(
							this->reader.extractAddWrapWrapped(stmt),
							this->builder.createExtractValue(add_value, {0}, add_wrap.wrappedName)
						);
					} break;

					case Expr::Kind::AddWrapResult:  evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::AddWrapWrapped: evo::debugFatalBreak("Not a valid stmt");
				}
			}
		}

		this->reader.clearTargetFunction();
		this->stmt_values.clear();
		this->allocas.clear();
		this->args.clear();
	}


	auto PIRToLLVMIR::get_constant_value(const Expr& expr) -> llvmint::Constant {
		evo::debugAssert(
			expr.getKind() == Expr::Kind::Number || expr.getKind() == Expr::Kind::Boolean, "Not a valid constant"
		);

		if(expr.getKind() == Expr::Kind::Boolean){
			return this->builder.getValueBool(this->reader.getBoolean(expr)).asConstant();
		}

		const Number& number = this->reader.getNumber(expr);

		switch(number.type.getKind()){
			case Type::Kind::Signed: {
				return this->builder.getValueI_N(number.type.getWidth(), false, number.getInt()).asConstant();
			} break;

			case Type::Kind::Unsigned: {
				return this->builder.getValueI_N(number.type.getWidth(), true, number.getInt()).asConstant();
			} break;

			case Type::Kind::Float: {
				return this->builder.getValueFloat(this->get_type(number.type), number.getFloat());
			} break;

			case Type::Kind::BFloat: {
				return this->builder.getValueFloat(this->builder.getTypeBF16(), number.getFloat());
			} break;


			default: evo::debugFatalBreak("Unknown or unsupported number kind");
		}
	}


	auto PIRToLLVMIR::get_value(const Expr& expr) -> llvmint::Value {
		switch(expr.getKind()){
			case Expr::Kind::None: evo::debugFatalBreak("Not a valid expr");

			case Expr::Kind::GlobalValue: {
				const GlobalVar& global_var = this->reader.getGlobalValue(expr);
				return this->global_vars.at(global_var.name).asValue();
			} break;

			case Expr::Kind::Number: {
				const Number& number = this->reader.getNumber(expr);

				switch(number.type.getKind()){
					case Type::Kind::Signed: {
						return this->builder.getValueI_N(
							number.type.getWidth(), false, number.getInt()
						).asValue();
					} break;

					case Type::Kind::Unsigned: {
						return this->builder.getValueI_N(
							number.type.getWidth(), true, number.getInt()
						).asValue();
					} break;

					case Type::Kind::Float: {
						return this->builder.getValueFloat(
							this->get_type(number.type), number.getFloat()
						).asValue();
					} break;

					case Type::Kind::BFloat: {
						return this->builder.getValueFloat(
							this->builder.getTypeBF16(), number.getFloat()
						).asValue();
					} break;

					default: evo::debugFatalBreak("Unknown or unsupported number kind");
				}
			} break;

			case Expr::Kind::Boolean: {
				return this->builder.getValueBool(this->reader.getBoolean(expr)).asValue();
			} break;

			case Expr::Kind::ParamExpr: {
				const ParamExpr& param = this->reader.getParamExpr(expr);
				return this->args[param.index].asValue();
			} break;

			case Expr::Kind::Call: {
				return this->stmt_values.at(expr);
			} break;

			case Expr::Kind::CallVoid: evo::debugFatalBreak("Not a value");
			case Expr::Kind::Ret: evo::debugFatalBreak("Not a value");
			case Expr::Kind::Branch: evo::debugFatalBreak("Not a value");

			case Expr::Kind::Alloca: {
				const Alloca& alloca_info = this->reader.getAlloca(expr);
				return this->allocas.at(&alloca_info).asValue();
			} break;

			case Expr::Kind::Load: {
				return this->stmt_values.at(expr);
			} break;

			case Expr::Kind::Store: evo::debugFatalBreak("Not a value");

			case Expr::Kind::Add: {
				return this->stmt_values.at(expr);
			} break;

			case Expr::Kind::AddWrap: evo::debugFatalBreak("Not a value");

			case Expr::Kind::AddWrapResult: {
				return this->stmt_values.at(this->reader.extractAddWrapResult(expr));
			} break;

			case Expr::Kind::AddWrapWrapped: {
				return this->stmt_values.at(this->reader.extractAddWrapWrapped(expr));
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto PIRToLLVMIR::get_type(const Type& type) -> llvmint::Type {
		switch(type.getKind()){
			case Type::Kind::Void:     return this->builder.getTypeVoid();
			case Type::Kind::Signed:   return this->builder.getTypeI_N(type.getWidth()).asType();
			case Type::Kind::Unsigned: return this->builder.getTypeI_N(type.getWidth()).asType();
			case Type::Kind::Bool:     return this->builder.getTypeBool().asType();
			case Type::Kind::Float: {
				switch(type.getWidth()){
					case 16:  return this->builder.getTypeF16();
					case 32:  return this->builder.getTypeF32();
					case 64:  return this->builder.getTypeF64();
					case 80:  return this->builder.getTypeF80();
					case 128: return this->builder.getTypeF128();
				}
			} break;
			case Type::Kind::BFloat: return this->builder.getTypeBF16();
			case Type::Kind::Ptr:    return this->builder.getTypePtr().asType();

			case Type::Kind::Array: evo::debugFatalBreak("Type::Kind::Array unsupported");

			case Type::Kind::Struct: {
				const StructType& struct_type = this->module.getStructType(type);
				return this->struct_types.at(struct_type.name);
			} break;

			case Type::Kind::Function: {
				return this->get_func_type(type).asType();
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported Type::Kind");
	}


	auto PIRToLLVMIR::get_func_type(const Type& type) -> llvmint::FunctionType {
		const FunctionType& func_type = this->module.getFunctionType(type);

		auto params = evo::SmallVector<llvmint::Type>();
		for(const Type& param : func_type.parameters){
			params.emplace_back(this->get_type(param));
		}

		return this->builder.getFuncProto(this->get_type(func_type.returnType), params, false);
	}


	auto PIRToLLVMIR::get_linkage(const Linkage& linkage) -> llvmint::LinkageType {
		switch(linkage){
			case Linkage::Default:  return llvmint::LinkageType::Internal;
			case Linkage::Private:  return llvmint::LinkageType::Private;
			case Linkage::Internal: return llvmint::LinkageType::Internal;
			case Linkage::External: return llvmint::LinkageType::External;
		}

		evo::debugFatalBreak("Unknown or unsupported linkage kind");
	}


	auto PIRToLLVMIR::get_calling_conv(const CallingConvention& calling_conv) -> llvmint::CallingConv {
		switch(calling_conv){
			case CallingConvention::Default: return llvmint::CallingConv::C;
			case CallingConvention::C:       return llvmint::CallingConv::C;
			case CallingConvention::Fast:    return llvmint::CallingConv::Fast;
			case CallingConvention::Cold:    return llvmint::CallingConv::Cold;
		}

		evo::debugFatalBreak("Unknown or unsupported linkage kind");
	}

	auto PIRToLLVMIR::get_atomic_ordering(const AtomicOrdering& atomic_ordering) -> llvmint::AtomicOrdering {
		switch(atomic_ordering){
			case AtomicOrdering::None:                   return llvmint::AtomicOrdering::NotAtomic;
			case AtomicOrdering::Monotonic:              return llvmint::AtomicOrdering::Monotonic;
			case AtomicOrdering::Acquire:                return llvmint::AtomicOrdering::Acquire;
			case AtomicOrdering::Release:                return llvmint::AtomicOrdering::Release;
			case AtomicOrdering::AcquireRelease:         return llvmint::AtomicOrdering::AcquireRelease;
			case AtomicOrdering::SequentiallyConsistent: return llvmint::AtomicOrdering::SequentiallyConsistent;
		}

		evo::debugFatalBreak("Unknown or unsupported atomic ordering");
	}

}