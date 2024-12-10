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
		const llvmint::Constant constant_value = this->get_global_var_value(global.value, global.type);
		const llvmint::Type constant_type = constant_value.getType();

		llvmint::GlobalVariable llvm_global_var = this->llvm_module.createGlobal(
			constant_value, constant_type, this->get_linkage(global.linkage), global.isConstant, global.name
		);

		llvm_global_var.setAlignment(unsigned(this->module.getAlignment(global.type)));

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

			i += 1;
		}

		this->funcs.emplace(func.getName(), llvm_func);
		return FuncLoweredSetup(func, llvm_func);
	}


	auto PIRToLLVMIR::lower_func_body(const Function& func, const llvmint::Function& llvm_func) -> void {
		auto basic_block_map = std::unordered_map<BasicBlock::ID, llvmint::BasicBlock>();

		this->reader.setTargetFunction(func);
		
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

					case Expr::Kind::Breakpoint: {
						this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::debugtrap, this->builder.getTypeVoid(), nullptr
						);
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

					case Expr::Kind::CondBranch: {
						const CondBranch& branch = this->reader.getCondBranch(stmt);
						this->builder.createCondBranch(
							this->get_value(branch.cond),
							basic_block_map.at(branch.thenBlock),
							basic_block_map.at(branch.elseBlock)
						);
					} break;

					case Expr::Kind::Unreachable: {
						this->builder.createUnreachable();
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

					case Expr::Kind::CalcPtr: {
						const CalcPtr& calc_ptr = this->reader.getCalcPtr(stmt);

						auto indices = evo::SmallVector<llvmint::Value>();
						for(const CalcPtr::Index& index : calc_ptr.indices){
							if(index.is<int64_t>()){
								indices.emplace_back(this->builder.getValueI32(int32_t(index.as<int64_t>())));
							}else{
								indices.emplace_back(this->get_value(index.as<Expr>()));
							}
						}

						const llvmint::Value gep = this->builder.createGetElementPtr(
							this->get_type(calc_ptr.ptrType),
							this->get_value(calc_ptr.basePtr),
							indices,
							calc_ptr.name
						);

						this->stmt_values.emplace(stmt, gep);
					} break;

					case Expr::Kind::BitCast:{
						const BitCast& bitcast = this->reader.getBitCast(stmt);

						const llvmint::Value from_value = this->get_value(bitcast.fromValue);
						const llvmint::Type to_type = this->get_type(bitcast.toType);

						const llvmint::Value llvm_bitcast = 
							this->builder.createBitCast(from_value, to_type, bitcast.name);
						this->stmt_values.emplace(stmt, llvm_bitcast);
					} break;

					case Expr::Kind::Trunc:{
						const Trunc& trunc = this->reader.getTrunc(stmt);

						const llvmint::Value from_value = this->get_value(trunc.fromValue);
						const llvmint::Type to_type = this->get_type(trunc.toType);

						const llvmint::Value llvm_trunc = this->builder.createTrunc(from_value, to_type, trunc.name);
						this->stmt_values.emplace(stmt, llvm_trunc);
					} break;

					case Expr::Kind::FTrunc:{
						const FTrunc& ftrunc = this->reader.getFTrunc(stmt);

						const llvmint::Value from_value = this->get_value(ftrunc.fromValue);
						const llvmint::Type to_type = this->get_type(ftrunc.toType);

						const llvmint::Value llvm_ftrunc = this->builder.createFTrunc(from_value, to_type, ftrunc.name);
						this->stmt_values.emplace(stmt, llvm_ftrunc);
					} break;

					case Expr::Kind::SExt:{
						const SExt& sext = this->reader.getSExt(stmt);

						const llvmint::Value from_value = this->get_value(sext.fromValue);
						const llvmint::Type to_type = this->get_type(sext.toType);

						const llvmint::Value llvm_sext = this->builder.createSExt(from_value, to_type, sext.name);
						this->stmt_values.emplace(stmt, llvm_sext);
					} break;

					case Expr::Kind::ZExt:{
						const ZExt& zext = this->reader.getZExt(stmt);

						const llvmint::Value from_value = this->get_value(zext.fromValue);
						const llvmint::Type to_type = this->get_type(zext.toType);

						const llvmint::Value llvm_zext = this->builder.createZExt(from_value, to_type, zext.name);
						this->stmt_values.emplace(stmt, llvm_zext);
					} break;

					case Expr::Kind::FExt:{
						const FExt& fext = this->reader.getFExt(stmt);

						const llvmint::Value from_value = this->get_value(fext.fromValue);
						const llvmint::Type to_type = this->get_type(fext.toType);

						const llvmint::Value llvm_fext = this->builder.createFExt(from_value, to_type, fext.name);
						this->stmt_values.emplace(stmt, llvm_fext);
					} break;

					case Expr::Kind::IToF:{
						const IToF& itof = this->reader.getIToF(stmt);

						const llvmint::Value from_value = this->get_value(itof.fromValue);
						const llvmint::Type to_type = this->get_type(itof.toType);

						const llvmint::Value llvm_itof = this->builder.createIToF(from_value, to_type, itof.name);
						this->stmt_values.emplace(stmt, llvm_itof);
					} break;

					case Expr::Kind::UIToF:{
						const UIToF& uitof = this->reader.getUIToF(stmt);

						const llvmint::Value from_value = this->get_value(uitof.fromValue);
						const llvmint::Type to_type = this->get_type(uitof.toType);

						const llvmint::Value llvm_uitof = this->builder.createUIToF(from_value, to_type, uitof.name);
						this->stmt_values.emplace(stmt, llvm_uitof);
					} break;

					case Expr::Kind::FToI:{
						const FToI& ftoi = this->reader.getFToI(stmt);

						const llvmint::Value from_value = this->get_value(ftoi.fromValue);
						const llvmint::Type to_type = this->get_type(ftoi.toType);

						const llvmint::Value llvm_ftoi = this->builder.createFToI(from_value, to_type, ftoi.name);
						this->stmt_values.emplace(stmt, llvm_ftoi);
					} break;

					case Expr::Kind::FToUI:{
						const FToUI& ftoui = this->reader.getFToUI(stmt);

						const llvmint::Value from_value = this->get_value(ftoui.fromValue);
						const llvmint::Type to_type = this->get_type(ftoui.toType);

						const llvmint::Value llvm_ftoui = this->builder.createFToUI(from_value, to_type, ftoui.name);
						this->stmt_values.emplace(stmt, llvm_ftoui);
					} break;


					case Expr::Kind::Add: {
						const Add& add = this->reader.getAdd(stmt);

						const llvmint::Value lhs = this->get_value(add.lhs);
						const llvmint::Value rhs = this->get_value(add.rhs);

						bool no_wrap = !add.mayWrap;

						const llvmint::Value add_value = this->builder.createAdd(lhs, rhs, no_wrap, no_wrap, add.name);
						this->stmt_values.emplace(stmt, add_value);
					} break;

					case Expr::Kind::FAdd: {
						const FAdd& add = this->reader.getFAdd(stmt);

						const llvmint::Value lhs = this->get_value(add.lhs);
						const llvmint::Value rhs = this->get_value(add.rhs);

						const llvmint::Value add_value = this->builder.createFAdd(lhs, rhs, add.name);
						this->stmt_values.emplace(stmt, add_value);
					} break;

					case Expr::Kind::SAddWrap: {
						const SAddWrap& sadd_wrap = this->reader.getSAddWrap(stmt);
						const Type& sadd_type = this->reader.getExprType(sadd_wrap.lhs);

						const llvmint::Type return_type = this->builder.getStructType(
							{this->get_type(sadd_type), this->builder.getTypeBool().asType()}
						).asType();

						const llvmint::Value sadd_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::saddOverflow,
							return_type,
							{this->get_value(sadd_wrap.lhs), this->get_value(sadd_wrap.rhs)},
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

					case Expr::Kind::SAddWrapResult:  evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::SAddWrapWrapped: evo::debugFatalBreak("Not a valid stmt");

					case Expr::Kind::UAddWrap: {
						const UAddWrap& uadd_wrap = this->reader.getUAddWrap(stmt);
						const Type& uadd_type = this->reader.getExprType(uadd_wrap.lhs);

						const llvmint::Type return_type = this->builder.getStructType(
							{this->get_type(uadd_type), this->builder.getTypeBool().asType()}
						).asType();

						const llvmint::Value uadd_value = this->builder.createIntrinsicCall(
							llvmint::IRBuilder::IntrinsicID::uaddOverflow,
							return_type,
							{this->get_value(uadd_wrap.lhs), this->get_value(uadd_wrap.rhs)},
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

					case Expr::Kind::UAddWrapResult:  evo::debugFatalBreak("Not a valid stmt");
					case Expr::Kind::UAddWrapWrapped: evo::debugFatalBreak("Not a valid stmt");
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
			case Type::Kind::Integer: {
				return this->builder.getValueI_N(number.type.getWidth(), false, number.getInt()).asConstant();
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


	auto PIRToLLVMIR::get_global_var_value(const GlobalVar::Value& global_var_value, const Type& type)
	-> llvmint::Constant {
		return global_var_value.visit([&](const auto& value) -> llvmint::Constant {
			using ValueT = std::decay_t<decltype(value)>;

			if constexpr(std::is_same<ValueT, Expr>()){
				return this->get_constant_value(value);

			}else if constexpr(std::is_same<ValueT, GlobalVar::Zeroinit>()){
				switch(type.getKind()){
					case Type::Kind::Integer: {
						return this->builder.getValueI_N(type.getWidth(), 0).asConstant();
					} break;

					case Type::Kind::Float: {
						switch(type.getWidth()){
							case 16:  return this->builder.getValueF16(0);
							case 32:  return this->builder.getValueF32(0);
							case 64:  return this->builder.getValueF64(0);
							case 80:  return this->builder.getValueF80(0);
							case 128: return this->builder.getValueF128(0);
						}

						evo::debugFatalBreak("Unknown float width");
					} break;

					case Type::Kind::BFloat: {
						return this->builder.getValueBF16(0);
					} break;

					default: {
						return this->builder.getValueGlobalAggregateZero(this->get_type(type));
					} break;
				}

			}else if constexpr(std::is_same<ValueT,GlobalVar::Uninit>()){
				return this->builder.getValueGlobalUndefValue(this->get_type(type));

			}else if constexpr(std::is_same<ValueT, GlobalVar::String::ID>()){
				return this->builder.getValueGlobalStr(this->module.getGlobalString(value).value);

			}else if constexpr(std::is_same<ValueT, GlobalVar::Array::ID>()){
				const GlobalVar::Array& array = this->module.getGlobalArray(value);
				const Type& array_elem_type = this->module.getArrayType(type).elemType;

				auto values = std::vector<llvmint::Constant>();
				values.reserve(array.values.size());
				for(const GlobalVar::Value& arr_value : array.values){
					values.emplace_back(this->get_global_var_value(arr_value, array_elem_type));
				}

				return this->builder.getValueGlobalArray(values);

			}else if constexpr(std::is_same<ValueT, GlobalVar::Struct::ID>()){
				const GlobalVar::Struct& global_struct = this->module.getGlobalStruct(value);
				const StructType& struct_type = this->module.getStructType(type);

				auto values = std::vector<llvmint::Constant>();
				values.reserve(global_struct.values.size());
				for(size_t i = 0; const GlobalVar::Value& arr_value : global_struct.values){
					values.emplace_back(this->get_global_var_value(arr_value, struct_type.members[i]));

					i += 1;
				}

				return this->builder.getValueGlobalStruct(this->get_struct_type(type), values);

			}else{
				static_assert(false, "Unknown GlobalVar::Value");
			}
		});
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
					case Type::Kind::Integer: {
						return this->builder.getValueI_N(number.type.getWidth(), true, number.getInt()).asValue();
					} break;

					case Type::Kind::Float: {
						core::GenericFloat float_value = [&](){
							switch(number.type.getWidth()){
								case 16: return number.getFloat().asF16();
								case 32: return number.getFloat().asF32();
								case 64: return number.getFloat().asF64();
								case 80: return number.getFloat().asF80();
								case 128: return number.getFloat().asF128();
								default: evo::debugFatalBreak("Unsupported float width ({})", number.type.getWidth());
							}
						}();
						return this->builder.getValueFloat(this->get_type(number.type), float_value).asValue();
					} break;

					case Type::Kind::BFloat: {
						return this->builder.getValueFloat(
							this->builder.getTypeBF16(), number.getFloat().asBF16()
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

			case Expr::Kind::CallVoid:    evo::debugFatalBreak("Not a value");
			case Expr::Kind::Breakpoint:  evo::debugFatalBreak("Not a value");
			case Expr::Kind::Ret:         evo::debugFatalBreak("Not a value");
			case Expr::Kind::Branch:      evo::debugFatalBreak("Not a value");
			case Expr::Kind::CondBranch:  evo::debugFatalBreak("Not a value");
			case Expr::Kind::Unreachable: evo::debugFatalBreak("Not a value");

			case Expr::Kind::Alloca: {
				const Alloca& alloca_info = this->reader.getAlloca(expr);
				return this->allocas.at(&alloca_info).asValue();
			} break;

			case Expr::Kind::Load: {
				return this->stmt_values.at(expr);
			} break;

			case Expr::Kind::Store: evo::debugFatalBreak("Not a value");

			case Expr::Kind::CalcPtr: return this->stmt_values.at(expr);
			case Expr::Kind::BitCast: return this->stmt_values.at(expr);
			case Expr::Kind::Trunc:   return this->stmt_values.at(expr);
			case Expr::Kind::FTrunc:  return this->stmt_values.at(expr);
			case Expr::Kind::SExt:    return this->stmt_values.at(expr);
			case Expr::Kind::ZExt:    return this->stmt_values.at(expr);
			case Expr::Kind::FExt:    return this->stmt_values.at(expr);
			case Expr::Kind::IToF:    return this->stmt_values.at(expr);
			case Expr::Kind::UIToF:   return this->stmt_values.at(expr);
			case Expr::Kind::FToI:    return this->stmt_values.at(expr);
			case Expr::Kind::FToUI:   return this->stmt_values.at(expr);
			case Expr::Kind::Add:     return this->stmt_values.at(expr);
			case Expr::Kind::FAdd:    return this->stmt_values.at(expr);

			case Expr::Kind::SAddWrap: evo::debugFatalBreak("Not a value");

			case Expr::Kind::SAddWrapResult: {
				return this->stmt_values.at(this->reader.extractSAddWrapResult(expr));
			} break;

			case Expr::Kind::SAddWrapWrapped: {
				return this->stmt_values.at(this->reader.extractSAddWrapWrapped(expr));
			} break;

			case Expr::Kind::UAddWrap: evo::debugFatalBreak("Not a value");

			case Expr::Kind::UAddWrapResult: {
				return this->stmt_values.at(this->reader.extractUAddWrapResult(expr));
			} break;

			case Expr::Kind::UAddWrapWrapped: {
				return this->stmt_values.at(this->reader.extractUAddWrapWrapped(expr));
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported Expr::Kind");
	}


	auto PIRToLLVMIR::get_type(const Type& type) -> llvmint::Type {
		switch(type.getKind()){
			case Type::Kind::Void:     return this->builder.getTypeVoid();
			case Type::Kind::Integer:  return this->builder.getTypeI_N(type.getWidth()).asType();
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

			case Type::Kind::Array: {
				const ArrayType& array_type = this->module.getArrayType(type);
				return this->builder.getArrayType(this->get_type(array_type.elemType), array_type.length).asType();
			} break;

			case Type::Kind::Struct: {
				return this->get_struct_type(type).asType();
			} break;

			case Type::Kind::Function: {
				return this->get_func_type(type).asType();
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported Type::Kind");
	}


	auto PIRToLLVMIR::get_struct_type(const Type& type) -> llvmint::StructType {
		const StructType& struct_type = this->module.getStructType(type);
		return this->struct_types.at(struct_type.name);
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