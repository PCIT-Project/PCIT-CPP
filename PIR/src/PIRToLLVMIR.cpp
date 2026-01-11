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
			this->lower_struct_type<true>(struct_type);
		}

		
		for(const ExternalFunction& external_function : this->module.getExternalFunctionIter()){
			this->lower_external_func<false>(external_function);
		}

		auto func_setups = std::vector<FuncLoweredSetup>();
		for(const Function& function : this->module.getFunctionIter()){
			func_setups.emplace_back(this->lower_function_setup<false>(function));
		}

		for(const GlobalVar& global_var : this->module.getGlobalVarIter()){
			this->lower_global_var_decl<false>(global_var);
		}

		for(const GlobalVar& global_var : this->module.getGlobalVarIter()){
			this->lower_global_var_def<false>(global_var);
		}

		for(const FuncLoweredSetup& func_setup : func_setups){
			this->lower_func_body<false>(func_setup.func, func_setup.llvm_func);
		}
	}



	auto PIRToLLVMIR::lowerSubset(const Subset& subset) -> void {
		this->lower_subset_impl<false>(subset);
	}

	auto PIRToLLVMIR::lowerSubsetWithWeakDependencies(const Subset& subset) -> void {
		this->lower_subset_impl<true>(subset);
	}



	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::lower_subset_impl(const Subset& subset) -> void {
		for(const Type& struct_type : subset.structs){
			this->lower_struct_type<ADD_WEAK_DEPS>(this->module.getStructType(struct_type));
		}

		for(const GlobalVar::ID global_var_id : subset.globalVars){
			this->lower_global_var_decl<ADD_WEAK_DEPS>(this->module.getGlobalVar(global_var_id));
		}

		for(const GlobalVar::ID global_var_decl_id : subset.globalVarDecls){
			this->lower_global_var_decl<ADD_WEAK_DEPS>(this->module.getGlobalVar(global_var_decl_id));
		}

		for(const GlobalVar::ID global_var_id : subset.globalVars){
			this->lower_global_var_def<ADD_WEAK_DEPS>(this->module.getGlobalVar(global_var_id));
		}

		for(const ExternalFunction::ID external_function_id : subset.externFuncs){
			this->lower_external_func<ADD_WEAK_DEPS>(this->module.getExternalFunction(external_function_id));
		}

		for(const Function::ID function_id : subset.funcDecls){
			this->lower_function_decl<ADD_WEAK_DEPS>(this->module.getFunction(function_id));
		}


		auto func_setups = std::vector<FuncLoweredSetup>();
		for(const Function::ID func_id : subset.funcs){
			func_setups.emplace_back(this->lower_function_setup<ADD_WEAK_DEPS>(this->module.getFunction(func_id)));
		}

		for(const FuncLoweredSetup& func_setup : func_setups){
			this->lower_func_body<ADD_WEAK_DEPS>(func_setup.func, func_setup.llvm_func);
		}
	}

	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::lower_struct_type(const StructType& struct_type) -> void {
		if constexpr(ADD_WEAK_DEPS){
			if(this->struct_types.contains(&struct_type)){ return; }
		}

		auto members = evo::SmallVector<llvmint::Type>();
		for(const Type& member : struct_type.members){
			members.emplace_back(this->get_type<ADD_WEAK_DEPS>(member));
		}

		const llvmint::StructType llvm_struct_type = this->builder.createStructType(
			members, struct_type.isPacked, struct_type.name
		);

		this->struct_types.emplace(&struct_type, llvm_struct_type);
	}


	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::lower_global_var_decl(const GlobalVar& global) -> void {
		if constexpr(ADD_WEAK_DEPS){
			if(this->global_vars.contains(&global)){ return; }
		}

		const llvmint::Type constant_type = this->get_type<ADD_WEAK_DEPS>(global.type);

		llvmint::GlobalVariable llvm_global_var = this->llvm_module.createGlobal(
			llvmint::Constant(nullptr), constant_type, llvmint::LinkageType::External, global.isConstant, global.name
		);

		llvm_global_var.setAlignment(unsigned(this->module.getAlignment(global.type)));

		this->global_vars.emplace(&global, llvm_global_var);
	}


	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::lower_global_var_def(const GlobalVar& global) -> void {
		llvmint::GlobalVariable& llvm_global = this->global_vars.at(&global);
		llvm_global.setInitializer(this->get_global_var_value<ADD_WEAK_DEPS>(global.value, global.type));
	}


	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::lower_external_func(const ExternalFunction& func_decl) -> void {
		if constexpr(ADD_WEAK_DEPS){
			if(this->funcs.contains(&func_decl)){ return; }
		}

		auto param_types = evo::SmallVector<llvmint::Type>();
		for(const Parameter& param : func_decl.parameters){
			param_types.emplace_back(this->get_type<ADD_WEAK_DEPS>(param.getType()));
		}

		const llvmint::FunctionType func_type = this->builder.getFuncProto(
			this->get_type<ADD_WEAK_DEPS>(func_decl.returnType), param_types, false
		);

		const llvmint::LinkageType linkage = this->get_linkage(func_decl.linkage);

		llvmint::Function llvm_func_decl = this->llvm_module.createFunction(func_decl.name, func_type, linkage);
		llvm_func_decl.setNoThrow();
		llvm_func_decl.setCallingConv(this->get_calling_conv(func_decl.callingConvention));

		this->funcs.emplace(&func_decl, llvm_func_decl);
	}

	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::lower_function_decl(const Function& func) -> void {
		if constexpr(ADD_WEAK_DEPS){
			if(this->funcs.contains(&func)){ return; }
		}

		auto param_types = evo::SmallVector<llvmint::Type>();
		for(const Parameter& param : func.getParameters()){
			param_types.emplace_back(this->get_type<ADD_WEAK_DEPS>(param.getType()));
		}

		const llvmint::FunctionType func_type = this->builder.getFuncProto(
			this->get_type<ADD_WEAK_DEPS>(func.getReturnType()), param_types, false
		);

		const llvmint::LinkageType linkage = this->get_linkage(func.getLinkage());

		llvmint::Function llvm_func_decl = this->llvm_module.createFunction(func.getName(), func_type, linkage);
		llvm_func_decl.setNoThrow();
		llvm_func_decl.setCallingConv(this->get_calling_conv(func.getCallingConvention()));

		this->funcs.emplace(&func, llvm_func_decl);
	}


	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::lower_function_setup(const Function& func) -> FuncLoweredSetup {
		auto param_types = evo::SmallVector<llvmint::Type>();
		for(const Parameter& param : func.getParameters()){
			param_types.emplace_back(this->get_type<ADD_WEAK_DEPS>(param.getType()));
		}

		const llvmint::FunctionType func_type = this->builder.getFuncProto(
			this->get_type<ADD_WEAK_DEPS>(func.getReturnType()), param_types, false
		);

		const llvmint::LinkageType linkage = this->get_linkage(func.getLinkage());

		llvmint::Function llvm_func = this->llvm_module.createFunction(func.getName(), func_type, linkage);
		llvm_func.setNoThrow();
		llvm_func.setCallingConv(this->get_calling_conv(func.getCallingConvention()));


		for(unsigned i = 0; const Parameter& param : func.getParameters()){
			llvmint::Argument arg = llvm_func.getArg(i);
			arg.setName(param.getName());

			for(const Parameter::Attribute& attribute_variant : param.attributes){
				attribute_variant.visit([&](const auto& attribute) -> void {
					using Attribute = std::decay_t<decltype(attribute)>;

					if constexpr(std::is_same<Attribute, Parameter::Attribute::Unsigned>()){
						arg.setZeroExt();

					}else if constexpr(std::is_same<Attribute, Parameter::Attribute::Signed>()){
						arg.setSignExt();

					}else if constexpr(std::is_same<Attribute, Parameter::Attribute::PtrNoAlias>()){
						arg.setNoAlias();

					}else if constexpr(std::is_same<Attribute, Parameter::Attribute::PtrNonNull>()){
						arg.setNonNull();

					}else if constexpr(std::is_same<Attribute, Parameter::Attribute::PtrDereferencable>()){
						arg.setDereferencable(attribute.size);

					}else if constexpr(std::is_same<Attribute, Parameter::Attribute::PtrReadOnly>()){
						arg.setReadOnly();

					}else if constexpr(std::is_same<Attribute, Parameter::Attribute::PtrWriteOnly>()){
						arg.setWriteOnly();

					}else if constexpr(std::is_same<Attribute, Parameter::Attribute::PtrWritable>()){
						arg.setWritable();

					}else if constexpr(std::is_same<Attribute, Parameter::Attribute::PtrRVO>()){
						arg.setStructRet(this->get_type<ADD_WEAK_DEPS>(attribute.type));
					}
				});
			}


			i += 1;
		}

		this->funcs.emplace(&func, llvm_func);
		return FuncLoweredSetup(func, llvm_func);
	}

	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::lower_func_body(const Function& func, const llvmint::Function& llvm_func) -> void {
		auto basic_block_map = std::unordered_map<BasicBlock::ID, llvmint::BasicBlock>();

		this->reader.setTargetFunction(func);
		
		if(func.getAllocasRange().empty() == false){
			const llvmint::BasicBlock setup_block = this->builder.createBasicBlock(llvm_func, "_ALLOCAS_");

			this->builder.setInsertionPoint(setup_block);

			for(const Alloca& alloca_info : func.getAllocasRange()){
				const llvmint::Alloca llvm_alloca = this->builder.createAlloca(
					this->get_type<ADD_WEAK_DEPS>(alloca_info.type), alloca_info.name
				);

				this->allocas.emplace(&alloca_info, llvm_alloca);
			}
		}

		for(unsigned i = 0; i < func.getParameters().size(); i+=1){
			this->args.emplace_back(llvm_func.getArg(i));
		}

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
				switch(stmt.kind()){
					case Expr::Kind::NONE:             evo::debugFatalBreak("Not a valid expr");
					case Expr::Kind::GLOBAL_VALUE:     evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::FUNCTION_POINTER: evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::NUMBER:           evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::BOOLEAN:          evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::NULLPTR:          evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::PARAM_EXPR:       evo::debugFatalBreak("Not a valid stmt");

					case Expr::Kind::CALL: {
						const Call& call = this->reader.getCall(stmt);

						auto call_args = evo::SmallVector<llvmint::Value>();
						for(const Expr& arg : call.args){
							call_args.emplace_back(this->get_value<ADD_WEAK_DEPS>(arg));
						}

						const llvmint::Value call_value = call.target.visit(
							[&](const auto& target) -> llvmint::Value {
								using TargetT = std::decay_t<decltype(target)>;


								if constexpr(std::is_same<TargetT, Function::ID>()){
									const Function& func_target = this->module.getFunction(target);

									return this->builder.createCall(
										this->get_func<ADD_WEAK_DEPS>(func_target), call_args, call.name
									).asValue();

								}else if constexpr(std::is_same<TargetT, ExternalFunction::ID>()){
									const ExternalFunction& func_target = this->module.getExternalFunction(target);

									llvmint::CallInst call_inst = this->builder.createCall(
										this->get_func<ADD_WEAK_DEPS>(func_target), call_args, call.name
									);
									call_inst.setCallingConv(
										this->get_calling_conv(func_target.callingConvention)
									);

									return call_inst.asValue();

								}else if constexpr(std::is_same<TargetT, PtrCall>()){
									const llvmint::FunctionType target_func_type = 
										this->get_func_type<ADD_WEAK_DEPS>(target.funcType);
									const llvmint::Value func_target = this->get_value<ADD_WEAK_DEPS>(target.location);

									llvmint::CallInst call_inst = 
										this->builder.createCall(func_target, target_func_type, call_args, call.name);
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

					case Expr::Kind::CALL_VOID: {
						const CallVoid& call_void = this->reader.getCallVoid(stmt);

						auto call_args = evo::SmallVector<llvmint::Value>();
						for(const Expr& arg : call_void.args){
							call_args.emplace_back(this->get_value<ADD_WEAK_DEPS>(arg));
						}

						call_void.target.visit([&](const auto& target) -> void {
							using TargetT = std::decay_t<decltype(target)>;

							if constexpr(std::is_same<TargetT, Function::ID>()){
								const Function& func_target = this->module.getFunction(target);
								this->builder.createCall(this->get_func<ADD_WEAK_DEPS>(func_target), call_args);

							}else if constexpr(std::is_same<TargetT, ExternalFunction::ID>()){
								const ExternalFunction& func_target = this->module.getExternalFunction(target);

								llvmint::CallInst call_inst = 
									this->builder.createCall(this->get_func<ADD_WEAK_DEPS>(func_target), call_args);
								call_inst.setCallingConv(this->get_calling_conv(func_target.callingConvention));

							}else if constexpr(std::is_same<TargetT, PtrCall>()){
								const llvmint::FunctionType target_func_type = 
									this->get_func_type<ADD_WEAK_DEPS>(target.funcType);
								const llvmint::Value func_target = this->get_value<ADD_WEAK_DEPS>(target.location);

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

					case Expr::Kind::ABORT: {
						// follows how clang lowers `__builtin_trap()` 
						// 	(which prevents it complaining about basic blocks not terminated)
						if(this->reader.getTargetFunction().getReturnType().kind() == pir::Type::Kind::VOID){
							this->builder.createIntrinsicCall(
								llvmint::IRBuilder::IntrinsicID::TRAP, this->builder.getTypeVoid(), nullptr
							);
							this->builder.createRet();

						}else{
							this->builder.createIntrinsicCall(
								llvmint::IRBuilder::IntrinsicID::TRAP, this->builder.getTypeVoid(), nullptr
							);
							this->builder.createIntrinsicCall(
								llvmint::IRBuilder::IntrinsicID::TRAP, this->builder.getTypeVoid(), nullptr
							);
							this->builder.createUnreachable();
						}
					} break;

					case Expr::Kind::BREAKPOINT: {
						this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::DEBUG_TRAP, this->builder.getTypeVoid(), nullptr
						);
					} break;

					case Expr::Kind::RET: {
						const Ret& ret = this->reader.getRet(stmt);
						if(ret.value.has_value()){
							this->builder.createRet(this->get_value<ADD_WEAK_DEPS>(*ret.value));
						}else{
							this->builder.createRet();
						}
					} break;

					case Expr::Kind::JUMP: {
						const Jump& jump = this->reader.getJump(stmt);
						this->builder.createBranch(basic_block_map.at(jump.target));
					} break;

					case Expr::Kind::BRANCH: {
						const Branch& branch = this->reader.getBranch(stmt);
						this->builder.createCondBranch(
							this->get_value<ADD_WEAK_DEPS>(branch.cond),
							basic_block_map.at(branch.thenBlock),
							basic_block_map.at(branch.elseBlock)
						);
					} break;

					case Expr::Kind::UNREACHABLE: {
						this->builder.createUnreachable();
					} break;

					case Expr::Kind::PHI: {
						const Phi& phi = this->reader.getPhi(stmt);

						const llvmint::Type value_type = this->get_type<ADD_WEAK_DEPS>(
							this->reader.getExprType(phi.predecessors[0].value)
						);

						auto incoming = evo::SmallVector<llvmint::IRBuilder::Incoming, 4>();
						for(const Phi::Predecessor& predecessor : phi.predecessors){
							incoming.emplace_back(
								this->get_value<ADD_WEAK_DEPS>(predecessor.value),
								basic_block_map.at(predecessor.block)
							);
						}

						const llvmint::Value created_phi = this->builder.createPhi(value_type, incoming, phi.name);
						this->stmt_values.emplace(stmt, created_phi);
					} break;

					case Expr::Kind::SWITCH: {
						const Switch& switch_stmt = this->reader.getSwitch(stmt);

						auto cases = evo::SmallVector<llvmint::IRBuilder::Case>();
						cases.reserve(switch_stmt.cases.size());

						for(const Switch::Case& switch_case : switch_stmt.cases){
							const Number& number = this->reader.getNumber(switch_case.value);

							cases.emplace_back(
								this->builder.getValueI_N(number.type.getWidth(), false, number.getInt()),
								basic_block_map.at(switch_case.block)
							);
						}

						this->builder.createSwitch(
							this->get_value<ADD_WEAK_DEPS>(switch_stmt.cond),
							basic_block_map.at(switch_stmt.defaultBlock),
							std::move(cases)
						);
					} break;

					case Expr::Kind::ALLOCA: evo::debugFatalBreak("Not a valid stmt");

					case Expr::Kind::LOAD: {
						const Load& load = this->reader.getLoad(stmt);

						const llvmint::LoadInst load_inst = this->builder.createLoad(
							this->get_value<ADD_WEAK_DEPS>(load.source),
							this->get_type<ADD_WEAK_DEPS>(load.type),
							load.isVolatile,
							this->get_atomic_ordering(load.atomicOrdering),
							load.name
						);

						this->stmt_values.emplace(stmt, load_inst);
					} break;

					case Expr::Kind::STORE: {
						const Store& store = this->reader.getStore(stmt);

						this->builder.createStore(
							this->get_value<ADD_WEAK_DEPS>(store.destination),
							this->get_value<ADD_WEAK_DEPS>(store.value),
							store.isVolatile,
							this->get_atomic_ordering(store.atomicOrdering)
						);
					} break;

					case Expr::Kind::CALC_PTR: {
						const CalcPtr& calc_ptr = this->reader.getCalcPtr(stmt);

						auto indices = evo::SmallVector<llvmint::Value>();
						for(const CalcPtr::Index& index : calc_ptr.indices){
							if(index.is<int64_t>()){
								indices.emplace_back(this->builder.getValueI32(int32_t(index.as<int64_t>())));
							}else{
								indices.emplace_back(this->get_value<ADD_WEAK_DEPS>(index.as<Expr>()));
							}
						}

						const llvmint::Value gep = this->builder.createGetElementPtr(
							this->get_type<ADD_WEAK_DEPS>(calc_ptr.ptrType),
							this->get_value<ADD_WEAK_DEPS>(calc_ptr.basePtr),
							indices,
							calc_ptr.name
						);

						this->stmt_values.emplace(stmt, gep);
					} break;

					case Expr::Kind::MEMCPY: {
						const Memcpy& memcpy = this->reader.getMemcpy(stmt);

						this->builder.createMemCpyInline(
							this->get_value<ADD_WEAK_DEPS>(memcpy.dst),
							this->get_value<ADD_WEAK_DEPS>(memcpy.src),
							this->get_value<ADD_WEAK_DEPS>(memcpy.numBytes),
							memcpy.isVolatile
						);
					} break;

					case Expr::Kind::MEMSET: {
						const Memset& memset = this->reader.getMemset(stmt);

						this->builder.createMemSetInline(
							this->get_value<ADD_WEAK_DEPS>(memset.dst),
							this->get_value<ADD_WEAK_DEPS>(memset.value),
							this->get_value<ADD_WEAK_DEPS>(memset.numBytes),
							memset.isVolatile
						);
					} break;

					case Expr::Kind::BIT_CAST: {
						const BitCast& bitcast = this->reader.getBitCast(stmt);

						const llvmint::Value from_value = this->get_value<ADD_WEAK_DEPS>(bitcast.fromValue);
						const llvmint::Type to_type = this->get_type<ADD_WEAK_DEPS>(bitcast.toType);

						const llvmint::Value llvm_bitcast = 
							this->builder.createBitCast(from_value, to_type, bitcast.name);
						this->stmt_values.emplace(stmt, llvm_bitcast);
					} break;

					case Expr::Kind::TRUNC: {
						const Trunc& trunc = this->reader.getTrunc(stmt);

						const llvmint::Value from_value = this->get_value<ADD_WEAK_DEPS>(trunc.fromValue);
						const llvmint::Type to_type = this->get_type<ADD_WEAK_DEPS>(trunc.toType);

						const llvmint::Value llvm_trunc = this->builder.createTrunc(from_value, to_type, trunc.name);
						this->stmt_values.emplace(stmt, llvm_trunc);
					} break;

					case Expr::Kind::FTRUNC: {
						const FTrunc& ftrunc = this->reader.getFTrunc(stmt);

						const llvmint::Value from_value = this->get_value<ADD_WEAK_DEPS>(ftrunc.fromValue);
						const llvmint::Type to_type = this->get_type<ADD_WEAK_DEPS>(ftrunc.toType);

						const llvmint::Value llvm_ftrunc = this->builder.createFTrunc(from_value, to_type, ftrunc.name);
						this->stmt_values.emplace(stmt, llvm_ftrunc);
					} break;

					case Expr::Kind::SEXT: {
						const SExt& sext = this->reader.getSExt(stmt);

						const llvmint::Value from_value = this->get_value<ADD_WEAK_DEPS>(sext.fromValue);
						const llvmint::Type to_type = this->get_type<ADD_WEAK_DEPS>(sext.toType);

						const llvmint::Value llvm_sext = this->builder.createSExt(from_value, to_type, sext.name);
						this->stmt_values.emplace(stmt, llvm_sext);
					} break;

					case Expr::Kind::ZEXT: {
						const ZExt& zext = this->reader.getZExt(stmt);

						const llvmint::Value from_value = this->get_value<ADD_WEAK_DEPS>(zext.fromValue);
						const llvmint::Type to_type = this->get_type<ADD_WEAK_DEPS>(zext.toType);

						const llvmint::Value llvm_zext = this->builder.createZExt(from_value, to_type, zext.name);
						this->stmt_values.emplace(stmt, llvm_zext);
					} break;

					case Expr::Kind::FEXT: {
						const FExt& fext = this->reader.getFExt(stmt);

						const llvmint::Value from_value = this->get_value<ADD_WEAK_DEPS>(fext.fromValue);
						const llvmint::Type to_type = this->get_type<ADD_WEAK_DEPS>(fext.toType);

						const llvmint::Value llvm_fext = this->builder.createFExt(from_value, to_type, fext.name);
						this->stmt_values.emplace(stmt, llvm_fext);
					} break;

					case Expr::Kind::ITOF: {
						const IToF& itof = this->reader.getIToF(stmt);

						const llvmint::Value from_value = this->get_value<ADD_WEAK_DEPS>(itof.fromValue);
						const llvmint::Type to_type = this->get_type<ADD_WEAK_DEPS>(itof.toType);

						const llvmint::Value llvm_itof = this->builder.createIToF(from_value, to_type, itof.name);
						this->stmt_values.emplace(stmt, llvm_itof);
					} break;

					case Expr::Kind::UITOF: {
						const UIToF& uitof = this->reader.getUIToF(stmt);

						const llvmint::Value from_value = this->get_value<ADD_WEAK_DEPS>(uitof.fromValue);
						const llvmint::Type to_type = this->get_type<ADD_WEAK_DEPS>(uitof.toType);

						const llvmint::Value llvm_uitof = this->builder.createUIToF(from_value, to_type, uitof.name);
						this->stmt_values.emplace(stmt, llvm_uitof);
					} break;

					case Expr::Kind::FTOI: {
						const FToI& ftoi = this->reader.getFToI(stmt);

						const llvmint::Value from_value = this->get_value<ADD_WEAK_DEPS>(ftoi.fromValue);
						const llvmint::Type to_type = this->get_type<ADD_WEAK_DEPS>(ftoi.toType);

						const llvmint::Value llvm_ftoi = this->builder.createFToI(from_value, to_type, ftoi.name);
						this->stmt_values.emplace(stmt, llvm_ftoi);
					} break;

					case Expr::Kind::FTOUI: {
						const FToUI& ftoui = this->reader.getFToUI(stmt);

						const llvmint::Value from_value = this->get_value<ADD_WEAK_DEPS>(ftoui.fromValue);
						const llvmint::Type to_type = this->get_type<ADD_WEAK_DEPS>(ftoui.toType);

						const llvmint::Value llvm_ftoui = this->builder.createFToUI(from_value, to_type, ftoui.name);
						this->stmt_values.emplace(stmt, llvm_ftoui);
					} break;


					case Expr::Kind::ADD: {
						const Add& add = this->reader.getAdd(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(add.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(add.rhs);

						const llvmint::Value add_value = this->builder.createAdd(lhs, rhs, add.nuw, add.nsw, add.name);
						this->stmt_values.emplace(stmt, add_value);
					} break;

					case Expr::Kind::SADD_WRAP: {
						const SAddWrap& sadd_wrap = this->reader.getSAddWrap(stmt);
						const Type& sadd_type = this->reader.getExprType(sadd_wrap.lhs);

						const llvmint::Type return_type = this->builder.getStructType(
							{this->get_type<ADD_WEAK_DEPS>(sadd_type), this->builder.getTypeBool().asType()}
						).asType();

						const llvmint::Value sadd_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::SADD_OVERFLOW,
							return_type,
							{
								this->get_value<ADD_WEAK_DEPS>(sadd_wrap.lhs),
								this->get_value<ADD_WEAK_DEPS>(sadd_wrap.rhs)
							},
							"ADD_WRAP"
						).asValue();

						this->stmt_values.emplace(
							this->reader.extractSAddWrapResult(stmt),
							this->builder.createExtractValue(sadd_value, {0}, sadd_wrap.resultName)
						);

						this->stmt_values.emplace(
							this->reader.extractSAddWrapWrapped(stmt),
							this->builder.createExtractValue(sadd_value, {1}, sadd_wrap.wrappedName)
						);
					} break;

					case Expr::Kind::SADD_WRAP_RESULT:  evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::SADD_WRAP_WRAPPED: evo::debugFatalBreak("Not a valid stmt");

					case Expr::Kind::UADD_WRAP: {
						const UAddWrap& uadd_wrap = this->reader.getUAddWrap(stmt);
						const Type& uadd_type = this->reader.getExprType(uadd_wrap.lhs);

						const llvmint::Type return_type = this->builder.getStructType(
							{this->get_type<ADD_WEAK_DEPS>(uadd_type), this->builder.getTypeBool().asType()}
						).asType();

						const llvmint::Value uadd_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::UADD_OVERFLOW,
							return_type,
							{
								this->get_value<ADD_WEAK_DEPS>(uadd_wrap.lhs),
								this->get_value<ADD_WEAK_DEPS>(uadd_wrap.rhs)
							},
							"ADD_WRAP"
						).asValue();

						this->stmt_values.emplace(
							this->reader.extractUAddWrapResult(stmt),
							this->builder.createExtractValue(uadd_value, {0}, uadd_wrap.resultName)
						);

						this->stmt_values.emplace(
							this->reader.extractUAddWrapWrapped(stmt),
							this->builder.createExtractValue(uadd_value, {1}, uadd_wrap.wrappedName)
						);
					} break;

					case Expr::Kind::UADD_WRAP_RESULT:  evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::UADD_WRAP_WRAPPED: evo::debugFatalBreak("Not a valid stmt");

					case Expr::Kind::SADD_SAT: {
						const SAddSat& sadd_sat = this->reader.getSAddSat(stmt);
						const Type& sadd_sat_type = this->reader.getExprType(sadd_sat.lhs);

						const llvmint::Value sadd_sat_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::SADD_SAT,
							this->get_type<ADD_WEAK_DEPS>(sadd_sat_type),
							{
								this->get_value<ADD_WEAK_DEPS>(sadd_sat.lhs),
								this->get_value<ADD_WEAK_DEPS>(sadd_sat.rhs)
							},
							sadd_sat.name
						).asValue();
						this->stmt_values.emplace(stmt, sadd_sat_value);
					} break;

					case Expr::Kind::UADD_SAT: {
						const UAddSat& uadd_sat = this->reader.getUAddSat(stmt);
						const Type& uadd_sat_type = this->reader.getExprType(uadd_sat.lhs);

						const llvmint::Value uadd_sat_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::UADD_SAT,
							this->get_type<ADD_WEAK_DEPS>(uadd_sat_type),
							{
								this->get_value<ADD_WEAK_DEPS>(uadd_sat.lhs),
								this->get_value<ADD_WEAK_DEPS>(uadd_sat.rhs)
							},
							uadd_sat.name
						).asValue();
						this->stmt_values.emplace(stmt, uadd_sat_value);
					} break;

					case Expr::Kind::FADD: {
						const FAdd& add = this->reader.getFAdd(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(add.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(add.rhs);

						const llvmint::Value add_value = this->builder.createFAdd(lhs, rhs, add.name);
						this->stmt_values.emplace(stmt, add_value);
					} break;


					case Expr::Kind::SUB: {
						const Sub& sub = this->reader.getSub(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(sub.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(sub.rhs);

						const llvmint::Value sub_value = this->builder.createSub(lhs, rhs, sub.nuw, sub.nsw, sub.name);
						this->stmt_values.emplace(stmt, sub_value);
					} break;

					case Expr::Kind::SSUB_WRAP: {
						const SSubWrap& ssub_wrap = this->reader.getSSubWrap(stmt);
						const Type& ssub_type = this->reader.getExprType(ssub_wrap.lhs);

						const llvmint::Type return_type = this->builder.getStructType(
							{this->get_type<ADD_WEAK_DEPS>(ssub_type), this->builder.getTypeBool().asType()}
						).asType();

						const llvmint::Value ssub_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::SSUB_OVERFLOW,
							return_type,
							{
								this->get_value<ADD_WEAK_DEPS>(ssub_wrap.lhs),
								this->get_value<ADD_WEAK_DEPS>(ssub_wrap.rhs)
							},
							"ADD_WRAP"
						).asValue();

						this->stmt_values.emplace(
							this->reader.extractSSubWrapResult(stmt),
							this->builder.createExtractValue(ssub_value, {0}, ssub_wrap.resultName)
						);

						this->stmt_values.emplace(
							this->reader.extractSSubWrapWrapped(stmt),
							this->builder.createExtractValue(ssub_value, {1}, ssub_wrap.wrappedName)
						);
					} break;

					case Expr::Kind::SSUB_WRAP_RESULT:  evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::SSUB_WRAP_WRAPPED: evo::debugFatalBreak("Not a valid stmt");

					case Expr::Kind::USUB_WRAP: {
						const USubWrap& usub_wrap = this->reader.getUSubWrap(stmt);
						const Type& usub_type = this->reader.getExprType(usub_wrap.lhs);

						const llvmint::Type return_type = this->builder.getStructType(
							{this->get_type<ADD_WEAK_DEPS>(usub_type), this->builder.getTypeBool().asType()}
						).asType();

						const llvmint::Value usub_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::USUB_OVERFLOW,
							return_type,
							{
								this->get_value<ADD_WEAK_DEPS>(usub_wrap.lhs),
								this->get_value<ADD_WEAK_DEPS>(usub_wrap.rhs)
							},
							"ADD_WRAP"
						).asValue();

						this->stmt_values.emplace(
							this->reader.extractUSubWrapResult(stmt),
							this->builder.createExtractValue(usub_value, {0}, usub_wrap.resultName)
						);

						this->stmt_values.emplace(
							this->reader.extractUSubWrapWrapped(stmt),
							this->builder.createExtractValue(usub_value, {1}, usub_wrap.wrappedName)
						);
					} break;

					case Expr::Kind::USUB_WRAP_RESULT:  evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::USUB_WRAP_WRAPPED: evo::debugFatalBreak("Not a valid stmt");

					case Expr::Kind::SSUB_SAT: {
						const SSubSat& ssub_sat = this->reader.getSSubSat(stmt);
						const Type& ssub_sat_type = this->reader.getExprType(ssub_sat.lhs);

						const llvmint::Value ssub_sat_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::SSUB_SAT,
							this->get_type<ADD_WEAK_DEPS>(ssub_sat_type),
							{
								this->get_value<ADD_WEAK_DEPS>(ssub_sat.lhs),
								this->get_value<ADD_WEAK_DEPS>(ssub_sat.rhs)
							},
							ssub_sat.name
						).asValue();
						this->stmt_values.emplace(stmt, ssub_sat_value);
					} break;

					case Expr::Kind::USUB_SAT: {
						const USubSat& usub_sat = this->reader.getUSubSat(stmt);
						const Type& usub_sat_type = this->reader.getExprType(usub_sat.lhs);

						const llvmint::Value usub_sat_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::USUB_SAT,
							this->get_type<ADD_WEAK_DEPS>(usub_sat_type),
							{
								this->get_value<ADD_WEAK_DEPS>(usub_sat.lhs),
								this->get_value<ADD_WEAK_DEPS>(usub_sat.rhs)
							},
							usub_sat.name
						).asValue();
						this->stmt_values.emplace(stmt, usub_sat_value);
					} break;

					case Expr::Kind::FSUB: {
						const FSub& add = this->reader.getFSub(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(add.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(add.rhs);

						const llvmint::Value add_value = this->builder.createFSub(lhs, rhs, add.name);
						this->stmt_values.emplace(stmt, add_value);
					} break;

					case Expr::Kind::MUL: {
						const Mul& mul = this->reader.getMul(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(mul.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(mul.rhs);

						const llvmint::Value mul_value = this->builder.createMul(lhs, rhs, mul.nuw, mul.nsw, mul.name);
						this->stmt_values.emplace(stmt, mul_value);
					} break;

					case Expr::Kind::SMUL_WRAP: {
						const SMulWrap& smul_wrap = this->reader.getSMulWrap(stmt);
						const Type& smul_type = this->reader.getExprType(smul_wrap.lhs);

						const llvmint::Type return_type = this->builder.getStructType(
							{this->get_type<ADD_WEAK_DEPS>(smul_type), this->builder.getTypeBool().asType()}
						).asType();

						const llvmint::Value smul_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::SMUL_OVERFLOW,
							return_type,
							{
								this->get_value<ADD_WEAK_DEPS>(smul_wrap.lhs),
								this->get_value<ADD_WEAK_DEPS>(smul_wrap.rhs)
							},
							"ADD_WRAP"
						).asValue();

						this->stmt_values.emplace(
							this->reader.extractSMulWrapResult(stmt),
							this->builder.createExtractValue(smul_value, {0}, smul_wrap.resultName)
						);

						this->stmt_values.emplace(
							this->reader.extractSMulWrapWrapped(stmt),
							this->builder.createExtractValue(smul_value, {1}, smul_wrap.wrappedName)
						);
					} break;

					case Expr::Kind::SMUL_WRAP_RESULT:  evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::SMUL_WRAP_WRAPPED: evo::debugFatalBreak("Not a valid stmt");

					case Expr::Kind::UMUL_WRAP: {
						const UMulWrap& umul_wrap = this->reader.getUMulWrap(stmt);
						const Type& umul_type = this->reader.getExprType(umul_wrap.lhs);

						const llvmint::Type return_type = this->builder.getStructType(
							{this->get_type<ADD_WEAK_DEPS>(umul_type), this->builder.getTypeBool().asType()}
						).asType();

						const llvmint::Value umul_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::UMUL_OVERFLOW,
							return_type,
							{
								this->get_value<ADD_WEAK_DEPS>(umul_wrap.lhs),
								this->get_value<ADD_WEAK_DEPS>(umul_wrap.rhs)
							},
							"ADD_WRAP"
						).asValue();

						this->stmt_values.emplace(
							this->reader.extractUMulWrapResult(stmt),
							this->builder.createExtractValue(umul_value, {0}, umul_wrap.resultName)
						);

						this->stmt_values.emplace(
							this->reader.extractUMulWrapWrapped(stmt),
							this->builder.createExtractValue(umul_value, {1}, umul_wrap.wrappedName)
						);
					} break;

					case Expr::Kind::UMUL_WRAP_RESULT:  evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::UMUL_WRAP_WRAPPED: evo::debugFatalBreak("Not a valid stmt");

					case Expr::Kind::SMUL_SAT: {
						const SMulSat& smul_sat = this->reader.getSMulSat(stmt);
						const Type& smul_sat_type = this->reader.getExprType(smul_sat.lhs);

						const llvmint::Value smul_sat_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::SMUL_FIX_SAT,
							this->get_type<ADD_WEAK_DEPS>(smul_sat_type),
							{
								this->get_value<ADD_WEAK_DEPS>(smul_sat.lhs),
								this->get_value<ADD_WEAK_DEPS>(smul_sat.rhs),
								this->builder.getValueI32(0).asValue()
							},
							smul_sat.name
						).asValue();
						this->stmt_values.emplace(stmt, smul_sat_value);
					} break;

					case Expr::Kind::UMUL_SAT: {
						const UMulSat& umul_sat = this->reader.getUMulSat(stmt);
						const Type& umul_sat_type = this->reader.getExprType(umul_sat.lhs);

						const llvmint::Value umul_sat_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::UMUL_FIX_SAT,
							this->get_type<ADD_WEAK_DEPS>(umul_sat_type),
							{
								this->get_value<ADD_WEAK_DEPS>(umul_sat.lhs),
								this->get_value<ADD_WEAK_DEPS>(umul_sat.rhs),
								this->builder.getValueI32(0).asValue()
							},
							umul_sat.name
						).asValue();
						this->stmt_values.emplace(stmt, umul_sat_value);
					} break;

					case Expr::Kind::FMUL: {
						const FMul& fmul = this->reader.getFMul(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(fmul.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(fmul.rhs);

						const llvmint::Value fmul_value = this->builder.createFMul(lhs, rhs, fmul.name);
						this->stmt_values.emplace(stmt, fmul_value);
					} break;

					case Expr::Kind::SDIV: {
						const SDiv& sdiv = this->reader.getSDiv(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(sdiv.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(sdiv.rhs);

						const llvmint::Value sdiv_value = this->builder.createSDiv(lhs, rhs, sdiv.isExact, sdiv.name);
						this->stmt_values.emplace(stmt, sdiv_value);
					} break;

					case Expr::Kind::UDIV: {
						const UDiv& udiv = this->reader.getUDiv(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(udiv.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(udiv.rhs);

						const llvmint::Value udiv_value = this->builder.createUDiv(lhs, rhs, udiv.isExact, udiv.name);
						this->stmt_values.emplace(stmt, udiv_value);
					} break;

					case Expr::Kind::FDIV: {
						const FDiv& fdiv = this->reader.getFDiv(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(fdiv.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(fdiv.rhs);

						const llvmint::Value fdiv_value = this->builder.createFDiv(lhs, rhs, fdiv.name);
						this->stmt_values.emplace(stmt, fdiv_value);
					} break;

					case Expr::Kind::SREM: {
						const SRem& srem = this->reader.getSRem(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(srem.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(srem.rhs);

						const llvmint::Value srem_value = this->builder.createSRem(lhs, rhs, srem.name);
						this->stmt_values.emplace(stmt, srem_value);
					} break;

					case Expr::Kind::UREM: {
						const URem& urem = this->reader.getURem(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(urem.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(urem.rhs);

						const llvmint::Value urem_value = this->builder.createURem(lhs, rhs, urem.name);
						this->stmt_values.emplace(stmt, urem_value);
					} break;

					case Expr::Kind::FREM: {
						const FRem& frem = this->reader.getFRem(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(frem.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(frem.rhs);

						const llvmint::Value frem_value = this->builder.createFRem(lhs, rhs, frem.name);
						this->stmt_values.emplace(stmt, frem_value);
					} break;

					case Expr::Kind::FNEG: {
						const FNeg& fneg = this->reader.getFNeg(stmt);

						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(fneg.rhs);

						const llvmint::Value fneg_value = this->builder.createFNeg(rhs, fneg.name);
						this->stmt_values.emplace(stmt, fneg_value);
					} break;


					case Expr::Kind::IEQ: {
						const IEq& ieq = this->reader.getIEq(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(ieq.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(ieq.rhs);

						const llvmint::Value ieq_value = this->builder.createICmpEQ(lhs, rhs, ieq.name);
						this->stmt_values.emplace(stmt, ieq_value);
					} break;
					
					case Expr::Kind::FEQ: {
						const FEq& feq = this->reader.getFEq(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(feq.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(feq.rhs);

						const llvmint::Value feq_value = this->builder.createFCmpEQ(lhs, rhs, feq.name);
						this->stmt_values.emplace(stmt, feq_value);
					} break;
					
					case Expr::Kind::INEQ: {
						const INeq& ineq = this->reader.getINeq(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(ineq.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(ineq.rhs);

						const llvmint::Value ineq_value = this->builder.createICmpNE(lhs, rhs, ineq.name);
						this->stmt_values.emplace(stmt, ineq_value);
					} break;
					
					case Expr::Kind::FNEQ: {
						const FNeq& fneq = this->reader.getFNeq(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(fneq.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(fneq.rhs);

						const llvmint::Value fneq_value = this->builder.createFCmpNE(lhs, rhs, fneq.name);
						this->stmt_values.emplace(stmt, fneq_value);
					} break;
					
					case Expr::Kind::SLT: {
						const SLT& slt = this->reader.getSLT(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(slt.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(slt.rhs);

						const llvmint::Value slt_value = this->builder.createICmpSLT(lhs, rhs, slt.name);
						this->stmt_values.emplace(stmt, slt_value);
					} break;
					
					case Expr::Kind::ULT: {
						const ULT& ult = this->reader.getULT(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(ult.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(ult.rhs);

						const llvmint::Value ult_value = this->builder.createICmpULT(lhs, rhs, ult.name);
						this->stmt_values.emplace(stmt, ult_value);
					} break;
					
					case Expr::Kind::FLT: {
						const FLT& flt = this->reader.getFLT(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(flt.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(flt.rhs);

						const llvmint::Value flt_value = this->builder.createFCmpLT(lhs, rhs, flt.name);
						this->stmt_values.emplace(stmt, flt_value);
					} break;
					
					case Expr::Kind::SLTE: {
						const SLTE& slte = this->reader.getSLTE(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(slte.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(slte.rhs);

						const llvmint::Value slte_value = this->builder.createICmpSLE(lhs, rhs, slte.name);
						this->stmt_values.emplace(stmt, slte_value);
					} break;
					
					case Expr::Kind::ULTE: {
						const ULTE& ulte = this->reader.getULTE(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(ulte.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(ulte.rhs);

						const llvmint::Value ulte_value = this->builder.createICmpULE(lhs, rhs, ulte.name);
						this->stmt_values.emplace(stmt, ulte_value);
					} break;
					
					case Expr::Kind::FLTE: {
						const FLTE& flte = this->reader.getFLTE(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(flte.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(flte.rhs);

						const llvmint::Value flte_value = this->builder.createFCmpLE(lhs, rhs, flte.name);
						this->stmt_values.emplace(stmt, flte_value);
					} break;
					
					case Expr::Kind::SGT: {
						const SGT& sgt = this->reader.getSGT(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(sgt.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(sgt.rhs);

						const llvmint::Value sgt_value = this->builder.createICmpSGT(lhs, rhs, sgt.name);
						this->stmt_values.emplace(stmt, sgt_value);
					} break;
					
					case Expr::Kind::UGT: {
						const UGT& ugt = this->reader.getUGT(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(ugt.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(ugt.rhs);

						const llvmint::Value ugt_value = this->builder.createICmpUGT(lhs, rhs, ugt.name);
						this->stmt_values.emplace(stmt, ugt_value);
					} break;
					
					case Expr::Kind::FGT: {
						const FGT& fgt = this->reader.getFGT(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(fgt.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(fgt.rhs);

						const llvmint::Value fgt_value = this->builder.createFCmpGT(lhs, rhs, fgt.name);
						this->stmt_values.emplace(stmt, fgt_value);
					} break;
					
					case Expr::Kind::SGTE: {
						const SGTE& sgte = this->reader.getSGTE(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(sgte.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(sgte.rhs);

						const llvmint::Value sgte_value = this->builder.createICmpSGE(lhs, rhs, sgte.name);
						this->stmt_values.emplace(stmt, sgte_value);
					} break;
					
					case Expr::Kind::UGTE: {
						const UGTE& ugte = this->reader.getUGTE(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(ugte.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(ugte.rhs);

						const llvmint::Value ugte_value = this->builder.createICmpUGE(lhs, rhs, ugte.name);
						this->stmt_values.emplace(stmt, ugte_value);
					} break;
					
					case Expr::Kind::FGTE: {
						const FGTE& fgte = this->reader.getFGTE(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(fgte.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(fgte.rhs);

						const llvmint::Value fgte_value = this->builder.createFCmpGE(lhs, rhs, fgte.name);
						this->stmt_values.emplace(stmt, fgte_value);
					} break;

					case Expr::Kind::AND: {
						const And& and_stmt = this->reader.getAnd(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(and_stmt.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(and_stmt.rhs);

						const llvmint::Value and_value = this->builder.createAnd(lhs, rhs, and_stmt.name);
						this->stmt_values.emplace(stmt, and_value);
					} break;

					case Expr::Kind::OR: {
						const Or& or_stmt = this->reader.getOr(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(or_stmt.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(or_stmt.rhs);

						const llvmint::Value or_value = this->builder.createOr(lhs, rhs, or_stmt.name);
						this->stmt_values.emplace(stmt, or_value);
					} break;

					case Expr::Kind::XOR: {
						const Xor& xor_stmt = this->reader.getXor(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(xor_stmt.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(xor_stmt.rhs);

						const llvmint::Value xor_value = this->builder.createXor(lhs, rhs, xor_stmt.name);
						this->stmt_values.emplace(stmt, xor_value);
					} break;

					case Expr::Kind::SHL: {
						const SHL& shl = this->reader.getSHL(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(shl.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(shl.rhs);

						const llvmint::Value shl_value = this->builder.createSHL(lhs, rhs, shl.nuw, shl.nsw, shl.name);
						this->stmt_values.emplace(stmt, shl_value);
					} break;

					case Expr::Kind::SSHL_SAT: {
						const SSHLSat& sshl_sat = this->reader.getSSHLSat(stmt);
						const Type& sshlsat_type = this->reader.getExprType(sshl_sat.lhs);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(sshl_sat.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(sshl_sat.rhs);

						const llvmint::Value sshl_sat_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::SSHL_SAT,
							this->get_type<ADD_WEAK_DEPS>(sshlsat_type),
							{
								this->get_value<ADD_WEAK_DEPS>(sshl_sat.lhs),
								this->get_value<ADD_WEAK_DEPS>(sshl_sat.rhs)
							},
							sshl_sat.name
						).asValue();
						this->stmt_values.emplace(stmt, sshl_sat_value);
					} break;

					case Expr::Kind::USHL_SAT: {
						const USHLSat& ushl_sat = this->reader.getUSHLSat(stmt);
						const Type& ushlsat_type = this->reader.getExprType(ushl_sat.lhs);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(ushl_sat.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(ushl_sat.rhs);

						const llvmint::Value ushl_sat_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::USHL_SAT,
							this->get_type<ADD_WEAK_DEPS>(ushlsat_type),
							{
								this->get_value<ADD_WEAK_DEPS>(ushl_sat.lhs),
								this->get_value<ADD_WEAK_DEPS>(ushl_sat.rhs)
							},
							ushl_sat.name
						).asValue();
						this->stmt_values.emplace(stmt, ushl_sat_value);
					} break;

					case Expr::Kind::SSHR: {
						const SSHR& sshr = this->reader.getSSHR(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(sshr.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(sshr.rhs);

						const llvmint::Value sshr_value = this->builder.createASHR(lhs, rhs, sshr.isExact, sshr.name);
						this->stmt_values.emplace(stmt, sshr_value);
					} break;

					case Expr::Kind::USHR: {
						const USHR& ushr = this->reader.getUSHR(stmt);

						const llvmint::Value lhs = this->get_value<ADD_WEAK_DEPS>(ushr.lhs);
						const llvmint::Value rhs = this->get_value<ADD_WEAK_DEPS>(ushr.rhs);

						const llvmint::Value ushr_value = this->builder.createLSHR(lhs, rhs, ushr.isExact, ushr.name);
						this->stmt_values.emplace(stmt, ushr_value);
					} break;

					case Expr::Kind::BIT_REVERSE: {
						const BitReverse& bit_reverse = this->reader.getBitReverse(stmt);

						const llvmint::Value arg = this->get_value<ADD_WEAK_DEPS>(bit_reverse.arg);

						const llvmint::Value bit_reverse_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::BIT_REVERSE,
							this->get_type<ADD_WEAK_DEPS>(this->reader.getExprType(bit_reverse.arg)),
							arg,
							bit_reverse.name
						).asValue();
						this->stmt_values.emplace(stmt, bit_reverse_value);
					} break;

					case Expr::Kind::BYTE_SWAP: {
						const ByteSwap& byte_swap = this->reader.getByteSwap(stmt);

						const llvmint::Value arg = this->get_value<ADD_WEAK_DEPS>(byte_swap.arg);

						const llvmint::Value byte_swap_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::BSWAP,
							this->get_type<ADD_WEAK_DEPS>(this->reader.getExprType(byte_swap.arg)),
							arg,
							byte_swap.name
						).asValue();
						this->stmt_values.emplace(stmt, byte_swap_value);
					} break;

					case Expr::Kind::CTPOP: {
						const CtPop& ctpop = this->reader.getCtPop(stmt);

						const llvmint::Value arg = this->get_value<ADD_WEAK_DEPS>(ctpop.arg);

						const llvmint::Value ctpop_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::CTPOP,
							this->get_type<ADD_WEAK_DEPS>(this->reader.getExprType(ctpop.arg)),
							arg,
							ctpop.name
						).asValue();
						this->stmt_values.emplace(stmt, ctpop_value);
					} break;

					case Expr::Kind::CTLZ: {
						const CTLZ& ctlz = this->reader.getCTLZ(stmt);

						const llvmint::Value arg = this->get_value<ADD_WEAK_DEPS>(ctlz.arg);

						const llvmint::Value ctlz_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::CTLZ,
							this->get_type<ADD_WEAK_DEPS>(this->reader.getExprType(ctlz.arg)),
							{arg, this->builder.getValueBool(false).asValue()},
							ctlz.name
						).asValue();
						this->stmt_values.emplace(stmt, ctlz_value);
					} break;

					case Expr::Kind::CTTZ: {
						const CTTZ& cttz = this->reader.getCTTZ(stmt);

						const llvmint::Value arg = this->get_value<ADD_WEAK_DEPS>(cttz.arg);

						const llvmint::Value cttz_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::CTTZ,
							this->get_type<ADD_WEAK_DEPS>(this->reader.getExprType(cttz.arg)),
							{arg, this->builder.getValueBool(false).asValue()},
							cttz.name
						).asValue();
						this->stmt_values.emplace(stmt, cttz_value);
					} break;

					case Expr::Kind::LIFETIME_START: {
						const LifetimeStart& lifetime_start = this->reader.getLifetimeStart(stmt);

						this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::LIFETIME_START,
							this->builder.getTypeVoid(),
							std::initializer_list<llvmint::Value>{
								static_cast<llvmint::Value>(this->builder.getValueI64(lifetime_start.size)),
								this->get_value<ADD_WEAK_DEPS>(lifetime_start.arg)
							}
						);
					} break;

					case Expr::Kind::LIFETIME_END: {
						const LifetimeEnd& lifetime_end = this->reader.getLifetimeEnd(stmt);

						this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::LIFETIME_END,
							this->builder.getTypeVoid(),
							std::initializer_list<llvmint::Value>{
								static_cast<llvmint::Value>(this->builder.getValueI64(lifetime_end.size)),
								this->get_value<ADD_WEAK_DEPS>(lifetime_end.arg)
							}
						);
					} break;
				}
			}
		}


		this->reader.clearTargetFunction();
		this->stmt_values.clear();
		this->allocas.clear();
		this->args.clear();
	}

	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::get_constant_value(const Expr& expr) -> llvmint::Constant {
		switch(expr.kind()){
			case Expr::Kind::NUMBER: {
				const Number& number = this->reader.getNumber(expr);

				switch(number.type.kind()){
					case Type::Kind::INTEGER: {
						return this->builder.getValueI_N(number.type.getWidth(), false, number.getInt()).asConstant();
					} break;

					case Type::Kind::FLOAT: {
						return this->builder.getValueFloat(this->get_type<false>(number.type), number.getFloat());
					} break;

					case Type::Kind::BFLOAT: {
						return this->builder.getValueFloat(this->builder.getTypeBF16(), number.getFloat());
					} break;


					default: evo::debugFatalBreak("Unknown or unsupported number kind");
				}
			} break;

			case Expr::Kind::BOOLEAN: {
				return this->builder.getValueBool(this->reader.getBoolean(expr)).asConstant();
			} break;

			case Expr::Kind::NULLPTR: {
				return this->builder.getValueNull();
			} break;

			case Expr::Kind::FUNCTION_POINTER: {
				return this->get_func<ADD_WEAK_DEPS>(this->reader.getFunctionPointer(expr)).asConstant();
			} break;

			case Expr::Kind::GLOBAL_VALUE: {
				const GlobalVar& global_var = this->reader.getGlobalValue(expr);
				return this->get_global_var<ADD_WEAK_DEPS>(global_var).asConstant();
			} break;

			default: {
				evo::debugFatalBreak("Not a valid constant");
			} break;
		}

	}

	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::get_global_var_value(const GlobalVar::Value& global_var_value, const Type& type)
	-> llvmint::Constant {
		return global_var_value.visit([&](const auto& value) -> llvmint::Constant {
			using ValueT = std::decay_t<decltype(value)>;

			if constexpr(std::is_same<ValueT, GlobalVar::NoValue>()){
				return llvmint::Constant(nullptr);

			}else if constexpr(std::is_same<ValueT, Expr>()){
				return this->get_constant_value<ADD_WEAK_DEPS>(value);

			}else if constexpr(std::is_same<ValueT, GlobalVar::Zeroinit>()){
				switch(type.kind()){
					case Type::Kind::INTEGER: {
						return this->builder.getValueI_N(type.getWidth(), 0).asConstant();
					} break;

					case Type::Kind::BOOL: {
						return this->builder.getValueI_N(8, 0).asConstant();
					} break;

					case Type::Kind::FLOAT: {
						switch(type.getWidth()){
							case 16:  return this->builder.getValueF16(0);
							case 32:  return this->builder.getValueF32(0);
							case 64:  return this->builder.getValueF64(0);
							case 80:  return this->builder.getValueF80(0);
							case 128: return this->builder.getValueF128(0);
						}

						evo::debugFatalBreak("Unknown float width");
					} break;

					case Type::Kind::BFLOAT: {
						return this->builder.getValueBF16(0);
					} break;

					case Type::Kind::PTR: {
						return this->builder.getValueI_N(64, 0).asConstant();
					} break;

					default: {
						return this->builder.getValueGlobalAggregateZero(this->get_type<ADD_WEAK_DEPS>(type));
					} break;
				}

			}else if constexpr(std::is_same<ValueT,GlobalVar::Uninit>()){
				return this->builder.getValueGlobalUndefValue(this->get_type<ADD_WEAK_DEPS>(type));

			}else if constexpr(std::is_same<ValueT, GlobalVar::String::ID>()){
				return this->builder.getValueGlobalStr(this->module.getGlobalString(value).value);

			}else if constexpr(std::is_same<ValueT, GlobalVar::Array::ID>()){
				const GlobalVar::Array& array = this->module.getGlobalArray(value);
				const Type& array_elem_type = this->module.getArrayType(type).elemType;

				auto values = std::vector<llvmint::Constant>();
				values.reserve(array.values.size());
				for(const GlobalVar::Value& arr_value : array.values){
					values.emplace_back(this->get_global_var_value<ADD_WEAK_DEPS>(arr_value, array_elem_type));
				}

				return this->builder.getValueGlobalArray(this->get_type<ADD_WEAK_DEPS>(array_elem_type), values);

			}else if constexpr(std::is_same<ValueT, GlobalVar::Struct::ID>()){
				const GlobalVar::Struct& global_struct = this->module.getGlobalStruct(value);
				const StructType& struct_type = this->module.getStructType(type);

				auto values = std::vector<llvmint::Constant>();
				values.reserve(global_struct.values.size());
				for(size_t i = 0; const GlobalVar::Value& arr_value : global_struct.values){
					values.emplace_back(this->get_global_var_value<ADD_WEAK_DEPS>(arr_value, struct_type.members[i]));

					i += 1;
				}

				return this->builder.getValueGlobalStruct(this->get_struct_type<ADD_WEAK_DEPS>(struct_type), values);

			}else{
				static_assert(false, "Unknown GlobalVar::Value");
			}
		});
	}


	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::get_value(const Expr& expr) -> llvmint::Value {
		switch(expr.kind()){
			case Expr::Kind::NONE: evo::debugFatalBreak("Not a valid expr");

			case Expr::Kind::GLOBAL_VALUE: {
				const GlobalVar& global_var = this->reader.getGlobalValue(expr);
				return this->get_global_var<ADD_WEAK_DEPS>(global_var).asValue();
			} break;

			case Expr::Kind::FUNCTION_POINTER: {
				const Function& func = this->reader.getFunctionPointer(expr);
				return this->get_func<ADD_WEAK_DEPS>(func).asValue();
			} break;

			case Expr::Kind::NUMBER: {
				const Number& number = this->reader.getNumber(expr);

				switch(number.type.kind()){
					case Type::Kind::INTEGER: {
						return this->builder.getValueI_N(number.type.getWidth(), true, number.getInt()).asValue();
					} break;

					case Type::Kind::FLOAT: {
						core::GenericFloat float_value = [&](){
							switch(number.type.getWidth()){
								case 16:  return number.getFloat().asF16();
								case 32:  return number.getFloat().asF32();
								case 64:  return number.getFloat().asF64();
								case 80:  return number.getFloat().asF80();
								case 128: return number.getFloat().asF128();
								default:  evo::debugFatalBreak("Unsupported float width ({})", number.type.getWidth());
							}
						}();
						return this->builder.getValueFloat(this->get_type<false>(number.type), float_value).asValue();
					} break;

					case Type::Kind::BFLOAT: {
						return this->builder.getValueFloat(
							this->builder.getTypeBF16(), number.getFloat().asBF16()
						).asValue();
					} break;

					default: evo::debugFatalBreak("Unknown or unsupported number kind");
				}
			} break;

			case Expr::Kind::BOOLEAN: {
				return this->builder.getValueBool(this->reader.getBoolean(expr)).asValue();
			} break;

			case Expr::Kind::NULLPTR: {
				return this->builder.getValueNull().asValue();
			} break;

			case Expr::Kind::PARAM_EXPR: {
				const ParamExpr& param = this->reader.getParamExpr(expr);
				return this->args[param.index].asValue();
			} break;

			case Expr::Kind::CALL: {
				return this->stmt_values.at(expr);
			} break;

			case Expr::Kind::CALL_VOID:   evo::debugFatalBreak("Not a value");
			case Expr::Kind::ABORT:       evo::debugFatalBreak("Not a value");
			case Expr::Kind::BREAKPOINT:  evo::debugFatalBreak("Not a value");
			case Expr::Kind::RET:         evo::debugFatalBreak("Not a value");
			case Expr::Kind::JUMP:        evo::debugFatalBreak("Not a value");
			case Expr::Kind::BRANCH:      evo::debugFatalBreak("Not a value");
			case Expr::Kind::UNREACHABLE: evo::debugFatalBreak("Not a value");
			case Expr::Kind::PHI:         return this->stmt_values.at(expr);

			case Expr::Kind::ALLOCA: {
				const Alloca& alloca_info = this->reader.getAlloca(expr);
				return this->allocas.at(&alloca_info).asValue();
			} break;

			case Expr::Kind::LOAD: {
				return this->stmt_values.at(expr);
			} break;

			case Expr::Kind::STORE: evo::debugFatalBreak("Not a value");

			case Expr::Kind::CALC_PTR: return this->stmt_values.at(expr);
			case Expr::Kind::MEMCPY: evo::debugFatalBreak("Not a value");
			case Expr::Kind::MEMSET: evo::debugFatalBreak("Not a value");
			case Expr::Kind::BIT_CAST: return this->stmt_values.at(expr);
			case Expr::Kind::TRUNC:   return this->stmt_values.at(expr);
			case Expr::Kind::FTRUNC:  return this->stmt_values.at(expr);
			case Expr::Kind::SEXT:    return this->stmt_values.at(expr);
			case Expr::Kind::ZEXT:    return this->stmt_values.at(expr);
			case Expr::Kind::FEXT:    return this->stmt_values.at(expr);
			case Expr::Kind::ITOF:    return this->stmt_values.at(expr);
			case Expr::Kind::UITOF:   return this->stmt_values.at(expr);
			case Expr::Kind::FTOI:    return this->stmt_values.at(expr);
			case Expr::Kind::FTOUI:   return this->stmt_values.at(expr);

			case Expr::Kind::ADD:     return this->stmt_values.at(expr);
			case Expr::Kind::SADD_WRAP: evo::debugFatalBreak("Not a value");
			case Expr::Kind::SADD_WRAP_RESULT: {
				return this->stmt_values.at(this->reader.extractSAddWrapResult(expr));
			} break;
			case Expr::Kind::SADD_WRAP_WRAPPED: {
				return this->stmt_values.at(this->reader.extractSAddWrapWrapped(expr));
			} break;
			case Expr::Kind::UADD_WRAP: evo::debugFatalBreak("Not a value");
			case Expr::Kind::UADD_WRAP_RESULT: {
				return this->stmt_values.at(this->reader.extractUAddWrapResult(expr));
			} break;
			case Expr::Kind::UADD_WRAP_WRAPPED: {
				return this->stmt_values.at(this->reader.extractUAddWrapWrapped(expr));
			} break;
			case Expr::Kind::SADD_SAT: return this->stmt_values.at(expr);
			case Expr::Kind::UADD_SAT: return this->stmt_values.at(expr);
			case Expr::Kind::FADD:     return this->stmt_values.at(expr);

			case Expr::Kind::SUB:      return this->stmt_values.at(expr);
			case Expr::Kind::SSUB_WRAP: evo::debugFatalBreak("Not a value");
			case Expr::Kind::SSUB_WRAP_RESULT: {
				return this->stmt_values.at(this->reader.extractSSubWrapResult(expr));
			} break;
			case Expr::Kind::SSUB_WRAP_WRAPPED: {
				return this->stmt_values.at(this->reader.extractSSubWrapWrapped(expr));
			} break;
			case Expr::Kind::USUB_WRAP: evo::debugFatalBreak("Not a value");
			case Expr::Kind::USUB_WRAP_RESULT: {
				return this->stmt_values.at(this->reader.extractUSubWrapResult(expr));
			} break;
			case Expr::Kind::USUB_WRAP_WRAPPED: {
				return this->stmt_values.at(this->reader.extractUSubWrapWrapped(expr));
			} break;
			case Expr::Kind::SSUB_SAT: return this->stmt_values.at(expr);
			case Expr::Kind::USUB_SAT: return this->stmt_values.at(expr);
			case Expr::Kind::FSUB:     return this->stmt_values.at(expr);

			case Expr::Kind::MUL:      return this->stmt_values.at(expr);
			case Expr::Kind::SMUL_WRAP: evo::debugFatalBreak("Not a value");
			case Expr::Kind::SMUL_WRAP_RESULT: {
				return this->stmt_values.at(this->reader.extractSMulWrapResult(expr));
			} break;
			case Expr::Kind::SMUL_WRAP_WRAPPED: {
				return this->stmt_values.at(this->reader.extractSMulWrapWrapped(expr));
			} break;
			case Expr::Kind::UMUL_WRAP: evo::debugFatalBreak("Not a value");
			case Expr::Kind::UMUL_WRAP_RESULT: {
				return this->stmt_values.at(this->reader.extractUMulWrapResult(expr));
			} break;
			case Expr::Kind::UMUL_WRAP_WRAPPED: {
				return this->stmt_values.at(this->reader.extractUMulWrapWrapped(expr));
			} break;
			case Expr::Kind::SMUL_SAT:       return this->stmt_values.at(expr);
			case Expr::Kind::UMUL_SAT:       return this->stmt_values.at(expr);
			case Expr::Kind::FMUL:           return this->stmt_values.at(expr);

			case Expr::Kind::SDIV:           return this->stmt_values.at(expr);
			case Expr::Kind::UDIV:           return this->stmt_values.at(expr);
			case Expr::Kind::FDIV:           return this->stmt_values.at(expr);
			case Expr::Kind::SREM:           return this->stmt_values.at(expr);
			case Expr::Kind::UREM:           return this->stmt_values.at(expr);
			case Expr::Kind::FREM:           return this->stmt_values.at(expr);
			case Expr::Kind::FNEG:           return this->stmt_values.at(expr);

			case Expr::Kind::IEQ:            return this->stmt_values.at(expr);
			case Expr::Kind::FEQ:            return this->stmt_values.at(expr);
			case Expr::Kind::INEQ:           return this->stmt_values.at(expr);
			case Expr::Kind::FNEQ:           return this->stmt_values.at(expr);
			case Expr::Kind::SLT:            return this->stmt_values.at(expr);
			case Expr::Kind::ULT:            return this->stmt_values.at(expr);
			case Expr::Kind::FLT:            return this->stmt_values.at(expr);
			case Expr::Kind::SLTE:           return this->stmt_values.at(expr);
			case Expr::Kind::ULTE:           return this->stmt_values.at(expr);
			case Expr::Kind::FLTE:           return this->stmt_values.at(expr);
			case Expr::Kind::SGT:            return this->stmt_values.at(expr);
			case Expr::Kind::UGT:            return this->stmt_values.at(expr);
			case Expr::Kind::FGT:            return this->stmt_values.at(expr);
			case Expr::Kind::SGTE:           return this->stmt_values.at(expr);
			case Expr::Kind::UGTE:           return this->stmt_values.at(expr);
			case Expr::Kind::FGTE:           return this->stmt_values.at(expr);
			case Expr::Kind::AND:            return this->stmt_values.at(expr);
			case Expr::Kind::OR:             return this->stmt_values.at(expr);
			case Expr::Kind::XOR:            return this->stmt_values.at(expr);
			case Expr::Kind::SHL:            return this->stmt_values.at(expr);
			case Expr::Kind::SSHL_SAT:       return this->stmt_values.at(expr);
			case Expr::Kind::USHL_SAT:       return this->stmt_values.at(expr);
			case Expr::Kind::SSHR:           return this->stmt_values.at(expr);
			case Expr::Kind::USHR:           return this->stmt_values.at(expr);

			case Expr::Kind::BIT_REVERSE:    return this->stmt_values.at(expr);
			case Expr::Kind::BYTE_SWAP:      return this->stmt_values.at(expr);
			case Expr::Kind::CTPOP:          return this->stmt_values.at(expr);
			case Expr::Kind::CTLZ:           return this->stmt_values.at(expr);
			case Expr::Kind::CTTZ:           return this->stmt_values.at(expr);

			case Expr::Kind::LIFETIME_START: evo::debugFatalBreak("Not a value");
			case Expr::Kind::LIFETIME_END:   evo::debugFatalBreak("Not a value");
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}

	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::get_type(const Type& type) -> llvmint::Type {
		switch(type.kind()){
			case Type::Kind::VOID:     return this->builder.getTypeVoid();
			case Type::Kind::INTEGER:  return this->builder.getTypeI_N(type.getWidth()).asType();
			case Type::Kind::BOOL:     return this->builder.getTypeBool().asType();
			case Type::Kind::FLOAT: {
				switch(type.getWidth()){
					case 16:  return this->builder.getTypeF16();
					case 32:  return this->builder.getTypeF32();
					case 64:  return this->builder.getTypeF64();
					case 80:  return this->builder.getTypeF80();
					case 128: return this->builder.getTypeF128();
				}
			} break;
			case Type::Kind::BFLOAT: return this->builder.getTypeBF16();
			case Type::Kind::PTR:    return this->builder.getTypePtr().asType();

			case Type::Kind::ARRAY: {
				const ArrayType& array_type = this->module.getArrayType(type);
				return this->builder.getArrayType(
					this->get_type<ADD_WEAK_DEPS>(array_type.elemType), array_type.length
				).asType();
			} break;

			case Type::Kind::STRUCT: {
				return this->get_struct_type<ADD_WEAK_DEPS>(this->module.getStructType(type)).asType();
			} break;

			case Type::Kind::FUNCTION: {
				return this->get_func_type<ADD_WEAK_DEPS>(type).asType();
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported Type::Kind");
	}


	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::get_func_type(const Type& type) -> llvmint::FunctionType {
		const FunctionType& func_type = this->module.getFunctionType(type);

		auto params = evo::SmallVector<llvmint::Type>();
		for(const Type& param : func_type.parameters){
			params.emplace_back(this->get_type<ADD_WEAK_DEPS>(param));
		}

		return this->builder.getFuncProto(this->get_type<ADD_WEAK_DEPS>(func_type.returnType), params, false);
	}


	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::get_struct_type(const StructType& type) -> llvmint::StructType {
		if constexpr(ADD_WEAK_DEPS){
			this->lower_struct_type<true>(type);
			evo::debugAssert(
				this->struct_types.contains(&type),
				"Struct \"{}\" was not lowered - somehow weak dep was not added",
				type.name
			);
		}else{
			evo::debugAssert(this->struct_types.contains(&type), "Struct \"{}\" was not lowered", type.name);
		}

		return this->struct_types.at(&type);
	}



	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::get_func(const Function& func) -> llvmint::Function {
		if constexpr(ADD_WEAK_DEPS){
			this->lower_function_decl<true>(func);
			evo::debugAssert(
				this->funcs.contains(&func),
				"Function \"{}\" was not lowered - somehow weak dep was not added",
				func.getName()
			);
		}else{
			evo::debugAssert(this->funcs.contains(&func), "Function \"{}\" was not lowered", func.getName());
		}

		return this->funcs.at(&func);
	}


	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::get_func(const ExternalFunction& func) -> llvmint::Function {
		if constexpr(ADD_WEAK_DEPS){
			this->lower_external_func<true>(func);
			evo::debugAssert(
				this->funcs.contains(&func),
				"ExternalFunction \"{}\" was not lowered - somehow weak dep was not added",
				func.name
			);
		}else{
			evo::debugAssert(this->funcs.contains(&func), "ExternalFunction \"{}\" was not lowered", func.name);
		}

		return this->funcs.at(&func);
	}



	template<bool ADD_WEAK_DEPS>
	auto PIRToLLVMIR::get_global_var(const GlobalVar& global_var) -> llvmint::GlobalVariable {
		if constexpr(ADD_WEAK_DEPS){
			this->lower_global_var_decl<true>(global_var);
			evo::debugAssert(
				this->global_vars.contains(&global_var),
				"Global var \"{}\" was not lowered - somehow weak dep was not added",
				global_var.name
			);
		}else{
			evo::debugAssert(
				this->global_vars.contains(&global_var), "Global var \"{}\" was not lowered", global_var.name
			);
		}

		return this->global_vars.at(&global_var);
	}



	auto PIRToLLVMIR::get_linkage(const Linkage& linkage) -> llvmint::LinkageType {
		switch(linkage){
			case Linkage::DEFAULT:       return llvmint::LinkageType::Internal;
			case Linkage::PRIVATE:       return llvmint::LinkageType::Private;
			case Linkage::INTERNAL:      return llvmint::LinkageType::Internal;
			case Linkage::EXTERNAL:      return llvmint::LinkageType::External;
			case Linkage::WEAK:          return llvmint::LinkageType::WeakODR;
			case Linkage::WEAK_EXTERNAL: return llvmint::LinkageType::ExternalWeak;
		}

		evo::debugFatalBreak("Unknown or unsupported linkage kind");
	}


	auto PIRToLLVMIR::get_calling_conv(const CallingConvention& calling_conv) -> llvmint::CallingConv {
		switch(calling_conv){
			case CallingConvention::DEFAULT: return llvmint::CallingConv::Fast;
			case CallingConvention::C:       return llvmint::CallingConv::C;
			case CallingConvention::FAST:    return llvmint::CallingConv::Fast;
			case CallingConvention::COLD:    return llvmint::CallingConv::Cold;
		}

		evo::debugFatalBreak("Unknown or unsupported linkage kind");
	}

	auto PIRToLLVMIR::get_atomic_ordering(const AtomicOrdering& atomic_ordering) -> llvmint::AtomicOrdering {
		switch(atomic_ordering){
			case AtomicOrdering::NONE:                   return llvmint::AtomicOrdering::NotAtomic;
			case AtomicOrdering::MONOTONIC:              return llvmint::AtomicOrdering::Monotonic;
			case AtomicOrdering::ACQUIRE:                return llvmint::AtomicOrdering::Acquire;
			case AtomicOrdering::RELEASE:                return llvmint::AtomicOrdering::Release;
			case AtomicOrdering::ACQUIRE_RELEASE:         return llvmint::AtomicOrdering::AcquireRelease;
			case AtomicOrdering::SEQUENTIALLY_CONSISTENT: return llvmint::AtomicOrdering::SequentiallyConsistent;
		}

		evo::debugFatalBreak("Unknown or unsupported atomic ordering");
	}

}