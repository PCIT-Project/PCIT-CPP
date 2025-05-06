////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SemaToPIR.h"

#include <ranges>

#include "../../include/Context.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{
	

	auto SemaToPIR::lower() -> void {
		for(uint32_t i = 0; i < this->context.getTypeManager().structs.size(); i+=1){
			this->lowerStruct(BaseType::Struct::ID(i));
		}

		for(const sema::GlobalVar::ID& global_var_id : this->context.getSemaBuffer().getGlobalVars()){
			this->lowerGlobalDecl(global_var_id);
			this->lowerGlobalDef(global_var_id);
		}

		for(const sema::Func::ID& func_id : this->context.getSemaBuffer().getFuncs()){
			this->lowerFuncDecl(func_id);
		}
		for(const sema::Func::ID& func_id : this->context.getSemaBuffer().getFuncs()){
			this->lowerFuncDef(func_id);
		}
	}



	auto SemaToPIR::lowerStruct(BaseType::Struct::ID) -> void {
		// const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(struct_id);

		// const pir::Type new_type = this->module.createStructType(
		// 	this->mangle_name(struct_id),
		// 	evo::SmallVector<pir::Type>(),
		// 	false
		// );

		// this->data.create_struct(struct_id, new_type);
	}



	auto SemaToPIR::lowerGlobalDecl(sema::GlobalVar::ID global_var_id) -> std::optional<pir::GlobalVar::ID> {
		const sema::GlobalVar& sema_global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);

		if(sema_global_var.kind == AST::VarDecl::Kind::DEF){ return std::nullopt; }

		const pir::GlobalVar::ID new_global_var = this->module.createGlobalVar(
			this->mangle_name(global_var_id),
			this->get_type(*sema_global_var.typeID),
			this->data.getConfig().isJIT ? pir::Linkage::EXTERNAL : pir::Linkage::PRIVATE,
			pir::GlobalVar::NoValue{},
			sema_global_var.kind == AST::VarDecl::Kind::CONST
		);

		this->data.create_global_var(global_var_id, new_global_var);

		return new_global_var;
	}


	auto SemaToPIR::lowerGlobalDef(sema::GlobalVar::ID global_var_id) -> void {
		const sema::GlobalVar& sema_global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);

		if(sema_global_var.kind == AST::VarDecl::Kind::DEF){ return; }

		const pir::GlobalVar::ID pir_var_id = this->data.get_global_var(global_var_id);
		this->module.getGlobalVar(pir_var_id).value = this->get_global_var_value(*sema_global_var.expr.load());
	}


	auto SemaToPIR::lowerFuncDecl(sema::Func::ID func_id) -> pir::Function::ID {
		const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);
		this->current_source = &this->context.getSourceManager()[func.sourceID];
		EVO_DEFER([&](){ this->current_source = nullptr; });

		if(func.hasInParam){ evo::unimplemented("functions with `in` parameter"); }

		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.typeID);


		auto params = evo::SmallVector<pir::Parameter>();
		params.reserve(func_type.params.size() + func_type.returnParams.size() + func_type.errorParams.size());

		auto param_infos = evo::SmallVector<Data::FuncInfo::Param>();
		param_infos.reserve(func_type.params.size());

		bool no_params_need_alloca = true;
		for(size_t i = 0; const BaseType::Function::Param& param : func_type.params){
			EVO_DEFER([&](){ i += 1; });

			if(this->context.getTypeManager().sizeOf(param.typeID) == 0){ continue; }

			std::string param_name = [&](){
				if(this->data.getConfig().useReadableNames == false){
					return std::format(".{}", params.size());
				}else{
					return std::string(this->current_source->getTokenBuffer()[func.params[i].ident].getString());
				}
			}();

			auto attributes = evo::SmallVector<pir::Parameter::Attribute>();

			if(param.shouldCopy){
				const pir::Type param_type = this->get_type(param.typeID);

				if(param_type.kind() == pir::Type::Kind::INTEGER){
					if(this->context.getTypeManager().isUnsignedIntegral(param.typeID)){
						attributes.emplace_back(pir::Parameter::Attribute::Unsigned());
					}else{
						attributes.emplace_back(pir::Parameter::Attribute::Signed());
					}
				}


				params.emplace_back(std::move(param_name), param_type, std::move(attributes));
				param_infos.emplace_back(std::nullopt);

				if(param.kind == AST::FuncDecl::Param::Kind::IN){
					no_params_need_alloca = false;
				}
			}else{
				attributes.emplace_back(pir::Parameter::Attribute::PtrNonNull());
				attributes.emplace_back(
					pir::Parameter::Attribute::PtrDereferencable(this->context.getTypeManager().sizeOf(param.typeID))
				);

				if(param.kind == AST::FuncDecl::Param::Kind::READ){
					attributes.emplace_back(pir::Parameter::Attribute::PtrReadOnly());
				}else{
					attributes.emplace_back(pir::Parameter::Attribute::PtrWritable());
					attributes.emplace_back(pir::Parameter::Attribute::PtrNoAlias());
				}

				params.emplace_back(std::move(param_name), this->module.createPtrType(), std::move(attributes));
				param_infos.emplace_back(this->get_type(param.typeID));
			}

		}

		auto return_params = evo::SmallVector<pir::Expr>();
		if(func_type.hasNamedReturns()){
			for(const BaseType::Function::ReturnParam& return_param : func_type.returnParams){
				return_params.emplace_back(this->agent.createParamExpr(uint32_t(params.size())));

				auto attributes = evo::SmallVector<pir::Parameter::Attribute>{
					pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNoAlias()),
					pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNonNull()),
					pir::Parameter::Attribute(pir::Parameter::Attribute::PtrDereferencable(
						this->context.getTypeManager().sizeOf(return_param.typeID.asTypeID())
					)),
					pir::Parameter::Attribute(pir::Parameter::Attribute::PtrWritable()),
				};

				if(func_type.returnParams.size() == 1 && func_type.hasErrorReturn() == false){
					attributes.emplace_back(
						pir::Parameter::Attribute::PtrRVO(this->get_type(return_param.typeID.asTypeID()))
					);
				}

				if(this->data.getConfig().useReadableNames){
					params.emplace_back(
						std::format("RET.{}", this->current_source->getTokenBuffer()[*return_param.ident].getString()),
						this->module.createPtrType(),
						std::move(attributes)
					);
				}else{
					params.emplace_back(
						std::format(".{}", params.size()), this->module.createPtrType(), std::move(attributes)
					);
				}
			}

		}else if(func_type.hasErrorReturn() && func_type.returnsVoid() == false){
			return_params.emplace_back(this->agent.createParamExpr(uint32_t(params.size())));

			auto attributes = evo::SmallVector<pir::Parameter::Attribute>{
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNoAlias()),
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNonNull()),
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrDereferencable(
					this->context.getTypeManager().sizeOf(func_type.returnParams[0].typeID.asTypeID())
				)),
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrWritable()),
			};

			if(this->data.getConfig().useReadableNames){
				params.emplace_back("RET", this->module.createPtrType(), std::move(attributes));
			}else{
				params.emplace_back(
					std::format(".{}", params.size()), this->module.createPtrType(), std::move(attributes)
				);
			}
		}

		auto error_return_param = std::optional<pir::Expr>();
		auto error_return_type = std::optional<pir::Type>();
		if(func_type.hasErrorReturnParams()){
			error_return_param = this->agent.createParamExpr(uint32_t(params.size()));


			auto error_return_param_types = evo::SmallVector<pir::Type>();
			for(const BaseType::Function::ReturnParam& error_param : func_type.errorParams){
				error_return_param_types.emplace_back(this->get_type(error_param.typeID));
			}

			error_return_type = this->module.createStructType(
				this->mangle_name(func_id) + ".ERR", std::move(error_return_param_types), true
			);

			auto attributes = evo::SmallVector<pir::Parameter::Attribute>{
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNoAlias()),
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNonNull()),
				pir::Parameter::Attribute(
					pir::Parameter::Attribute::PtrDereferencable(this->module.getSize(*error_return_type))
				),
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrWritable())
			};

			if(this->data.getConfig().useReadableNames){
				params.emplace_back("ERR", this->module.createPtrType(), std::move(attributes));
			}else{
				params.emplace_back(
					std::format(".{}", params.size()), this->module.createPtrType(), std::move(attributes)
				);
			}
		}

		const pir::Type return_type = [&](){
			if(func_type.hasErrorReturn()){
				return this->module.createBoolType();

			}else if(func_type.hasNamedReturns()){
				return this->module.createVoidType();

			}else{
				return this->get_type(func_type.returnParams.front().typeID);
			}
		}();

		const pir::Function::ID new_func_id = this->module.createFunction(
			this->mangle_name(func_id),
			std::move(params),
			this->data.getConfig().isJIT ? pir::CallingConvention::C : pir::CallingConvention::FAST,
			this->data.getConfig().isJIT ? pir::Linkage::EXTERNAL : pir::Linkage::PRIVATE,
			return_type
		);

		this->agent.setTargetFunction(new_func_id);

		this->data.create_func(
			func_id,
			new_func_id, // first arg of FuncInfo construction
			return_type,
			std::move(param_infos),
			std::move(return_params),
			error_return_param,
			error_return_type
		);


		if(func_type.params.empty() | no_params_need_alloca){
			this->agent.createBasicBlock(this->name("begin"));

		}else{
			const pir::BasicBlock::ID setup_block = this->agent.createBasicBlock(this->name("setup"));
			const pir::BasicBlock::ID begin_block = this->agent.createBasicBlock(this->name("begin"));

			this->agent.setTargetBasicBlock(setup_block);

			for(uint32_t i = 0; const BaseType::Function::Param& param : func_type.params){
				if(param.shouldCopy == false){ continue; }

				const pir::Type param_type = this->get_type(param.typeID);
				const pir::Expr param_alloca = this->agent.createAlloca(
					param_type,
					this->name("ALLOCA.{}", this->current_source->getTokenBuffer()[func.params[i].ident].getString())
				);

				this->agent.createStore(param_alloca, this->agent.createParamExpr(i), false, pir::AtomicOrdering::NONE);

				i += 1;
			}

			this->agent.createBranch(begin_block);
		}


		return new_func_id;
	}



	auto SemaToPIR::lowerFuncDef(sema::Func::ID func_id) -> void {
		const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(func_id);
		this->current_source = &this->context.getSourceManager()[sema_func.sourceID];
		EVO_DEFER([&](){ this->current_source = nullptr; });

		this->current_func_type = &this->context.getTypeManager().getFunction(sema_func.typeID);

		pir::Function& func = this->module.getFunction(this->data.get_func(func_id).pir_id);

		this->current_func_info = &this->data.get_func(func_id);
		EVO_DEFER([&](){ this->current_func_info = nullptr; });

		this->agent.setTargetFunction(func);
		this->agent.setTargetBasicBlockAtEnd();

		this->push_scope_level();

		for(const sema::Stmt& stmt : sema_func.stmtBlock){
			this->lower_stmt(stmt);
		}


		if(sema_func.isTerminated == false){
			if(this->current_func_type->returnsVoid()){
				if(this->current_func_type->hasErrorReturn()){
					this->agent.createRet(this->agent.createBoolean(true));
				}else{
					this->agent.createRet();
				}
				
			}else{
				this->agent.createUnreachable();
			}
		}


		this->current_func_type = nullptr;

		this->pop_scope_level();
		this->local_func_exprs.clear();
	}



	auto SemaToPIR::createJITEntry(sema::Func::ID target_entry_func) -> pir::Function::ID {
		const Data::FuncInfo& target_entry_func_info = this->data.get_func(target_entry_func);


		const pir::Function::ID entry_func_id = this->module.createFunction(
			"PTHR.entry", {}, pir::CallingConvention::C, pir::Linkage::EXTERNAL, this->module.createIntegerType(8)
		);

		pir::Function& entry_func = this->module.getFunction(entry_func_id);

		this->agent.setTargetFunction(entry_func);

		this->agent.createBasicBlock();
		this->agent.setTargetBasicBlockAtEnd();

		const pir::Expr entry_call = this->agent.createCall(target_entry_func_info.pir_id, {});
		this->agent.createRet(entry_call);

		return entry_func_id;
	}


	auto SemaToPIR::createFuncJITInterface(sema::Func::ID func_id, pir::Function::ID pir_func_id) -> pir::Function::ID {
		const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(func_id);
		this->current_source = &this->context.getSourceManager()[sema_func.sourceID];
		EVO_DEFER([&](){ this->current_source = nullptr; });

		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(sema_func.typeID);

		const pir::Function& target_pir_func = this->module.getFunction(pir_func_id);


		auto params = evo::SmallVector<pir::Parameter>{
			pir::Parameter("RET", this->module.createPtrType()),
			pir::Parameter("ARGS", this->module.createPtrType()),
		};

		const pir::Function::ID jit_interface_func_id = this->module.createFunction(
			"JIT_INT." + this->mangle_name(func_id),
			std::move(params),
			pir::CallingConvention::C,
			pir::Linkage::EXTERNAL,
			this->module.createVoidType()
		);


		pir::Function& jit_interface_func = this->module.getFunction(jit_interface_func_id);

		this->agent.setTargetFunction(jit_interface_func);

		this->agent.createBasicBlock();
		this->agent.setTargetBasicBlockAtEnd();

		auto args = evo::SmallVector<pir::Expr>();
		args.reserve(target_pir_func.getParameters().size());


		///////////////////////////////////
		// getters for parameters

		for(size_t i = 0; const pir::Parameter& param : target_pir_func.getParameters()){
			EVO_DEFER([&](){ i += 1; });

			bool param_is_ptr_rvo = false;
			for(const pir::Parameter::Attribute& attribute : param.attributes){
				if(attribute.is<pir::Parameter::Attribute::PtrRVO>()){
					param_is_ptr_rvo = true;
					break;
				}
			}

			
			if(param_is_ptr_rvo){
				switch(param.getType().kind()){
					case pir::Type::Kind::VOID: {
						evo::debugFatalBreak("Function parameter cannot be type void");
					} break;

					case pir::Type::Kind::INTEGER: {
						args.emplace_back(
							this->agent.createNumber(param.getType(), core::GenericInt(param.getType().getWidth(), 0))
						);
					} break;

					case pir::Type::Kind::BOOL: {
						args.emplace_back(this->agent.createBoolean(false));
					} break;

					case pir::Type::Kind::FLOAT: {
						// TODO(FUTURE): separate out creating GenericFloats for each type?
						args.emplace_back(
							this->agent.createNumber(param.getType(), core::GenericFloat::createF128(0.0))
						);
					} break;

					case pir::Type::Kind::BFLOAT: {
						args.emplace_back(
							this->agent.createNumber(param.getType(), core::GenericFloat::createBF16(0))
						);
					} break;

					case pir::Type::Kind::PTR: {
						const TypeInfo::ID param_type_id = [&](){
							if(i < func_type.params.size()){ return func_type.params[i].typeID; }
							return func_type.returnParams[i - func_type.params.size()].typeID.asTypeID();
						}();

						const pir::Expr alloca = this->agent.createAlloca(this->get_type(param_type_id));
						args.emplace_back(alloca);
					} break;

					case pir::Type::Kind::ARRAY: {
						evo::unimplemented();
					} break;

					case pir::Type::Kind::STRUCT: {
						evo::unimplemented();
					} break;

					case pir::Type::Kind::FUNCTION: {
						evo::unimplemented();
					} break;
				}

			}else{
				const pir::Expr arg_ptr = this->agent.createCalcPtr(
					this->agent.createParamExpr(1),
					this->module.createIntegerType(8),
					evo::SmallVector<pir::CalcPtr::Index>{int64_t(i * sizeof(core::GenericValue))}
				);

				switch(param.getType().kind()){
					case pir::Type::Kind::VOID: {
						evo::debugFatalBreak("Function parameter cannot be type void");
					} break;

					case pir::Type::Kind::INTEGER: {
						if(func_type.params[i].typeID == TypeManager::getTypeChar()) [[unlikely]] {
							args.emplace_back(
								this->agent.createCall(this->data.getJITInterfaceFuncs().get_generic_char, {arg_ptr})
							);

						}else{
							const pir::Expr alloca = this->agent.createAlloca(param.getType());
							this->agent.createCallVoid(
								this->data.getJITInterfaceFuncs().get_generic_int,
								{
									arg_ptr,
									alloca,
									this->agent.createNumber(
										this->module.createIntegerType(64),
										core::GenericInt(8, param.getType().getWidth() / 8)
									)
								}
							);
							args.emplace_back(
								this->agent.createLoad(alloca, param.getType(), false, pir::AtomicOrdering::NONE)
							);
						}
					} break;

					case pir::Type::Kind::BOOL: {
						args.emplace_back(
							this->agent.createCall(this->data.getJITInterfaceFuncs().get_generic_bool, {arg_ptr})
						);
					} break;

					case pir::Type::Kind::FLOAT: {
						const pir::Expr alloca = this->agent.createAlloca(param.getType());
						this->agent.createCallVoid(
							this->data.getJITInterfaceFuncs().get_generic_float, {arg_ptr, alloca}
						);
						args.emplace_back(
							this->agent.createLoad(alloca, param.getType(), false, pir::AtomicOrdering::NONE)
						);
					} break;

					case pir::Type::Kind::BFLOAT: {
						const pir::Expr alloca = this->agent.createAlloca(param.getType());
						this->agent.createCallVoid(
							this->data.getJITInterfaceFuncs().get_generic_float, {arg_ptr, alloca}
						);
						args.emplace_back(
							this->agent.createLoad(alloca, param.getType(), false, pir::AtomicOrdering::NONE)
						);
					} break;

					case pir::Type::Kind::PTR: {
						const TypeInfo::ID param_type_id = [&](){
							if(i < func_type.params.size()){ return func_type.params[i].typeID; }
							return func_type.returnParams[i - func_type.params.size()].typeID.asTypeID();
						}();

						if(this->context.getTypeManager().isIntegral(param_type_id)){
							const pir::Type param_pir_type = this->get_type(param_type_id);
							const pir::Expr alloca = this->agent.createAlloca(param_pir_type);
							this->agent.createCallVoid(
								this->data.getJITInterfaceFuncs().get_generic_int,
								{
									arg_ptr,
									alloca,
									this->agent.createNumber(
										this->module.createIntegerType(64),
										core::GenericInt(8, param_pir_type.getWidth() / 8)
									)
								}
							);
							args.emplace_back(alloca);
						}else{
							evo::debugAssert(this->context.getTypeManager().isFloat(param_type_id));
							const pir::Expr alloca = this->agent.createAlloca(this->get_type(param_type_id));
							this->agent.createCallVoid(
								this->data.getJITInterfaceFuncs().get_generic_float, {arg_ptr, alloca}
							);
							args.emplace_back(alloca);
						}
					} break;

					case pir::Type::Kind::ARRAY: {
						evo::unimplemented();
					} break;

					case pir::Type::Kind::STRUCT: {
						evo::unimplemented();
					} break;

					case pir::Type::Kind::FUNCTION: {
						evo::unimplemented();
					} break;
				}
			}
		}


		///////////////////////////////////
		// return the return value

		switch(target_pir_func.getReturnType().kind()){
			case pir::Type::Kind::VOID: {
				this->agent.createCallVoid(pir_func_id, evo::copy(args));
			} break;
			
			case pir::Type::Kind::INTEGER: {
				const bool returns_char = func_type.returnParams.size() == 1 
					&& func_type.hasErrorReturn() == false
					&& func_type.returnParams[0].typeID.asTypeID() == this->context.getTypeManager().getTypeChar();

				if(returns_char){
					this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_char, {
						this->agent.createParamExpr(0),
						this->agent.createCall(pir_func_id, evo::copy(args))
					});

				}else{
					const uint32_t bit_width = target_pir_func.getReturnType().getWidth();

					const pir::Expr return_alloca = this->agent.createAlloca(target_pir_func.getReturnType());
					const pir::Expr target_call = this->agent.createCall(pir_func_id, evo::copy(args));
					this->agent.createStore(return_alloca, target_call, false, pir::AtomicOrdering::NONE);

					this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_int, {
						this->agent.createParamExpr(0),
						return_alloca,
						this->agent.createNumber(
							this->module.createIntegerType(64), core::GenericInt::create<uint64_t>(uint64_t(bit_width))
						)
					});
				}
			} break;
			
			case pir::Type::Kind::BOOL: {
				this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_bool, {
					this->agent.createParamExpr(0),
					this->agent.createCall(pir_func_id, evo::copy(args))
				});
			} break;
			
			case pir::Type::Kind::FLOAT: {				
				switch(target_pir_func.getReturnType().getWidth()){
					case 16: {
						const pir::Expr return_alloca = this->agent.createAlloca(target_pir_func.getReturnType());
						const pir::Expr target_call = this->agent.createCall(pir_func_id, evo::copy(args));
						this->agent.createStore(return_alloca, target_call, false, pir::AtomicOrdering::NONE);

						this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_f16, {
							this->agent.createParamExpr(0), return_alloca,
						});
					} break;

					case 32: {
						this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_f32, {
							this->agent.createParamExpr(0),
							this->agent.createCall(pir_func_id, evo::copy(args))
						});

					} break;

					case 64: {
						this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_f64, {
							this->agent.createParamExpr(0),
							this->agent.createCall(pir_func_id, evo::copy(args))
						});

					} break;

					case 80: {
						const pir::Expr return_alloca = this->agent.createAlloca(target_pir_func.getReturnType());
						const pir::Expr target_call = this->agent.createCall(pir_func_id, evo::copy(args));
						this->agent.createStore(return_alloca, target_call, false, pir::AtomicOrdering::NONE);

						this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_f80, {
							this->agent.createParamExpr(0), return_alloca,
						});
					} break;

					case 128: {
						const pir::Expr return_alloca = this->agent.createAlloca(target_pir_func.getReturnType());
						const pir::Expr target_call = this->agent.createCall(pir_func_id, evo::copy(args));
						this->agent.createStore(return_alloca, target_call, false, pir::AtomicOrdering::NONE);

						this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_f128, {
							this->agent.createParamExpr(0), return_alloca,
						});
					} break;
				}
			} break;
			
			case pir::Type::Kind::BFLOAT: {
				const pir::Expr return_alloca = this->agent.createAlloca(target_pir_func.getReturnType());
				const pir::Expr target_call = this->agent.createCall(pir_func_id, evo::copy(args));
				this->agent.createStore(return_alloca, target_call, false, pir::AtomicOrdering::NONE);

				this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_bf16, {
					this->agent.createParamExpr(0), return_alloca,
				});
			} break;
			
			case pir::Type::Kind::PTR: {
				evo::unimplemented();
			} break;
			
			case pir::Type::Kind::ARRAY: {
				evo::unimplemented();
			} break;
			
			case pir::Type::Kind::STRUCT: {
				evo::unimplemented();
			} break;
			
			case pir::Type::Kind::FUNCTION: {
				evo::unimplemented();
			} break;
		}


		///////////////////////////////////
		// return RVO parameters

		for(size_t i = 0; const pir::Parameter& param : target_pir_func.getParameters()){
			EVO_DEFER([&](){ i += 1; });

			{
				bool param_is_rvo = false;
				for(const pir::Parameter::Attribute& attribute : param.attributes){
					if(attribute.is<pir::Parameter::Attribute::PtrRVO>()){
						param_is_rvo = true;
						break;
					}
				}

				if(param_is_rvo == false){ continue; }
			}

			const pir::Expr arg_ptr = this->agent.createCalcPtr(
				this->agent.createParamExpr(1),
				this->module.createIntegerType(8),
				evo::SmallVector<pir::CalcPtr::Index>{int64_t(i * sizeof(core::GenericValue))}
			);



			evo::debugAssert(
				param.getType().kind() == pir::Type::Kind::PTR,
				"mut parameters on a constexpr function call are not supported"
			);

			const TypeInfo::ID param_type_id = func_type.returnParams[0].typeID.asTypeID();
			const pir::Type pir_type_id = this->get_type(param_type_id);

			switch(pir_type_id.kind()){
				case pir::Type::Kind::VOID: {
					evo::debugFatalBreak("Function parameter cannot be type void");
				} break;

				case pir::Type::Kind::INTEGER: {
					if(param_type_id == TypeManager::getTypeChar()) [[unlikely]] {
						this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_char, {
							this->agent.createParamExpr(0),
							this->agent.createLoad(args[i], pir_type_id, false, pir::AtomicOrdering::NONE)
						});

					}else{
						const uint64_t bit_width = uint64_t(pir_type_id.getWidth());

						this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_int, {
							this->agent.createParamExpr(0),
							args[i],
							this->agent.createNumber(
								this->module.createIntegerType(64), core::GenericInt::create<uint64_t>(bit_width)
							)
						});
					}
				} break;

				case pir::Type::Kind::BOOL: {
					this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_bool, {
						this->agent.createParamExpr(0),
						this->agent.createLoad(args[i], pir_type_id, false, pir::AtomicOrdering::NONE)
					});
				} break;

				case pir::Type::Kind::FLOAT: {
					switch(pir_type_id.getWidth()){
						case 16: {
							this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_f16, {
								this->agent.createParamExpr(0), args[i]
							});
						} break;

						case 32: {
							this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_f32, {
								this->agent.createParamExpr(0),
								this->agent.createLoad(args[i], pir_type_id, false, pir::AtomicOrdering::NONE)
							});

						} break;

						case 64: {
							this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_f64, {
								this->agent.createParamExpr(0),
								this->agent.createLoad(args[i], pir_type_id, false, pir::AtomicOrdering::NONE)
							});

						} break;

						case 80: {
							this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_f80, {
								this->agent.createParamExpr(0), args[i]
							});
						} break;

						case 128: {
							this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_f128, {
								this->agent.createParamExpr(0), args[i]
							});
						} break;
					}
				} break;

				case pir::Type::Kind::BFLOAT: {
					this->agent.createCallVoid(this->data.getJITInterfaceFuncs().return_generic_bf16, {
						this->agent.createParamExpr(0), args[i]
					});
				} break;

				case pir::Type::Kind::PTR: {
					evo::unimplemented();
				} break;

				case pir::Type::Kind::ARRAY: {
					evo::unimplemented();
				} break;

				case pir::Type::Kind::STRUCT: {
					evo::unimplemented();
				} break;

				case pir::Type::Kind::FUNCTION: {
					evo::unimplemented();
				} break;
			}
		}



		///////////////////////////////////
		// done
		
		this->agent.createRet();

		return jit_interface_func_id;
	}






	auto SemaToPIR::lower_stmt(const sema::Stmt& stmt) -> void {
		switch(stmt.kind()){
			case sema::Stmt::Kind::GLOBAL_VAR: {
				evo::unimplemented("To PIR of sema::Stmt::Kind::GLOBAL_VAR");
			} break;

			case sema::Stmt::Kind::FUNC_CALL: {
				const sema::FuncCall& func_call = this->context.getSemaBuffer().getFuncCall(stmt.funcCallID());

				if(func_call.target.is<IntrinsicFunc::Kind>()){ 
					this->intrinsic_func_call(func_call);
					return;
				}

				const Data::FuncInfo& target_func_info = this->data.get_func(func_call.target.as<sema::Func::ID>());

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : func_call.args){
					if(target_func_info.params[i].is_copy()){
						args.emplace_back(this->get_expr_register(arg));
					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}

				if(target_func_info.return_type.kind() == pir::Type::Kind::VOID){
					this->agent.createCallVoid(target_func_info.pir_id, std::move(args));
				}else{
					std::ignore = this->agent.createCall(target_func_info.pir_id, std::move(args));
				}
			} break;

			case sema::Stmt::Kind::ASSIGN: {
				const sema::Assign& assignment = this->context.getSemaBuffer().getAssign(stmt.assignID());

				if(assignment.lhs.has_value()){
					this->get_expr_store(assignment.rhs, this->get_expr_pointer(*assignment.lhs));
				}else{
					this->get_expr_discard(assignment.rhs);
				}
			} break;

			case sema::Stmt::Kind::MULTI_ASSIGN: {
				const sema::MultiAssign& multi_assign = 
					this->context.getSemaBuffer().getMultiAssign(stmt.multiAssignID());

				auto targets = evo::SmallVector<pir::Expr>();
				targets.reserve(multi_assign.targets.size());
				for(const evo::Variant<sema::Expr, TypeInfo::ID>& target : multi_assign.targets){
					if(target.is<sema::Expr>()){
						targets.emplace_back(this->get_expr_pointer(target.as<sema::Expr>()));
					}else{
						targets.emplace_back(
							this->agent.createAlloca(
								this->get_type(target.as<TypeInfo::ID>()), this->name("DISCARD")
							)
						);
					}
				}

				this->get_expr_store(multi_assign.value, targets);
			} break;

			case sema::Stmt::Kind::RETURN: {
				const sema::Return& return_stmt = this->context.getSemaBuffer().getReturn(stmt.returnID());

				if(return_stmt.targetLabel.has_value()) [[unlikely]] {
					const std::string_view label =
						this->current_source->getTokenBuffer()[*return_stmt.targetLabel].getString();

					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						if(scope_level.label != label){ continue; }

						if(return_stmt.value.has_value()){
							this->get_expr_store(*return_stmt.value, scope_level.label_output_locations);	
						}

						this->agent.createBranch(*scope_level.end_block);
						break;
					}

				}else{
					if(this->current_func_type->hasErrorReturn()){
						if(return_stmt.value.has_value()){
							this->agent.createStore(
								this->current_func_info->return_params.front(),
								this->get_expr_register(*return_stmt.value),
								false,
								pir::AtomicOrdering::NONE
							);
						}

						this->agent.createRet(this->agent.createBoolean(true));

					}else{
						if(return_stmt.value.has_value()){
							this->agent.createRet(this->get_expr_register(*return_stmt.value));

						}else{
							this->agent.createRet();
						}
					}
				}

			} break;

			case sema::Stmt::Kind::ERROR: {
				const sema::Error& error_stmt = this->context.getSemaBuffer().getError(stmt.errorID());

				if(error_stmt.value.has_value()){
					this->agent.createStore(
						*this->current_func_info->error_return_param,
						this->get_expr_register(*error_stmt.value),
						false,
						pir::AtomicOrdering::NONE
					);
					this->agent.createRet(this->agent.createBoolean(false));

				}else{
					this->agent.createRet(this->agent.createBoolean(false));
				}
				
			} break;

			case sema::Stmt::Kind::UNREACHABLE: {
				evo::unimplemented("To PIR of sema::Stmt::Kind::UNREACHABLE");
			} break;

			case sema::Stmt::Kind::CONDITIONAL: {
				evo::unimplemented("To PIR of sema::Stmt::Kind::CONDITIONAL");
			} break;

			case sema::Stmt::Kind::WHILE: {
				evo::unimplemented("To PIR of sema::Stmt::Kind::WHILE");
			} break;
		}
	}




	//////////////////////////////////////////////////////////////////////
	// get expr


	auto SemaToPIR::get_expr_register(const sema::Expr expr) -> pir::Expr {
		return *this->get_expr_impl<GetExprMode::REGISTER>(expr, nullptr);
	}

	auto SemaToPIR::get_expr_pointer(const sema::Expr expr) -> pir::Expr {
		return *this->get_expr_impl<GetExprMode::POINTER>(expr, nullptr);
	}

	auto SemaToPIR::get_expr_store(const sema::Expr expr, evo::ArrayProxy<pir::Expr> store_locations) -> void {
		this->get_expr_impl<GetExprMode::STORE>(expr, store_locations);
	}

	auto SemaToPIR::get_expr_discard(const sema::Expr expr) -> void {
		this->get_expr_impl<GetExprMode::DISCARD>(expr, nullptr);
	}



	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::get_expr_impl(const sema::Expr expr, evo::ArrayProxy<pir::Expr> store_locations)
	-> std::optional<pir::Expr> {
		if constexpr(MODE == GetExprMode::STORE){
			evo::debugAssert(!store_locations.empty(), "Must have store location(s) if `MODE == GetExprMode::STORE`");

		}else{
			evo::debugAssert(store_locations.empty(), "Cannot have store location(s) if `MODE != GetExprMode::STORE`");
		}

		switch(expr.kind()){
			case sema::Expr::Kind::NONE: {
				evo::debugFatalBreak("Not a valid sema::Expr");
			} break;

			case sema::Expr::Kind::MODULE_IDENT: {
				evo::unimplemented("lower sema::Expr::Kind::ModuleIdent");
			} break;

			case sema::Expr::Kind::UNINIT: {
				evo::unimplemented("lower sema::Expr::Kind::Uninit");
			} break;

			case sema::Expr::Kind::ZEROINIT: {
				evo::unimplemented("lower sema::Expr::Kind::Zeroinit");
			} break;

			case sema::Expr::Kind::INT_VALUE: {
				const sema::IntValue& int_value = this->context.getSemaBuffer().getIntValue(expr.intValueID());
				const pir::Type value_type = this->get_type(*int_value.typeID);
				const pir::Expr number = this->agent.createNumber(value_type, int_value.value);

				if constexpr(MODE == GetExprMode::REGISTER){
					return number;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(value_type, this->name("NUMBER.ALLOCA"));
					this->agent.createStore(alloca, number, false, pir::AtomicOrdering::NONE);
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], number, false, pir::AtomicOrdering::NONE);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::FLOAT_VALUE: {
				const sema::FloatValue& float_value = this->context.getSemaBuffer().getFloatValue(expr.floatValueID());
				const pir::Type value_type = this->get_type(*float_value.typeID);
				const pir::Expr number = this->agent.createNumber(value_type, float_value.value);

				if constexpr(MODE == GetExprMode::REGISTER){
					return number;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(value_type, this->name("NUMBER.ALLOCA"));
					this->agent.createStore(alloca, number, false, pir::AtomicOrdering::NONE);
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], number, false, pir::AtomicOrdering::NONE);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::BOOL_VALUE: {
				const sema::BoolValue& bool_value = this->context.getSemaBuffer().getBoolValue(expr.boolValueID());
				const pir::Expr boolean = this->agent.createBoolean(bool_value.value);

				if constexpr(MODE == GetExprMode::REGISTER){
					return boolean;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(
						this->module.createBoolType(), this->name("BOOLEAN.ALLOCA")
					);
					this->agent.createStore(alloca, boolean, false, pir::AtomicOrdering::NONE);
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], boolean, false, pir::AtomicOrdering::NONE);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::STRING_VALUE: {
				evo::unimplemented("lower sema::Expr::Kind::StringValue");
			} break;

			case sema::Expr::Kind::CHAR_VALUE: {
				const sema::CharValue& char_value = this->context.getSemaBuffer().getCharValue(expr.charValueID());
				const pir::Type value_type = this->module.createIntegerType(8);
				const pir::Expr number = this->agent.createNumber(
					value_type, core::GenericInt(8, uint64_t(char_value.value))
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return number;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(value_type, this->name("NUMBER.ALLOCA"));
					this->agent.createStore(alloca, number, false, pir::AtomicOrdering::NONE);
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], number, false, pir::AtomicOrdering::NONE);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::INTRINSIC_FUNC: {
				evo::debugFatalBreak("sema::Expr::Kind::INTRINSIC_FUNC should be target of func call");
			} break;

			case sema::Expr::Kind::TEMPLATED_INTRINSIC_FUNC_INSTANTIATION: {
				evo::debugFatalBreak(
					"sema::Expr::Kind::TEMPLATED_INTRINSIC_FUNC_INSTANTIATION should be target of func call"
				);
			} break;

			case sema::Expr::Kind::COPY: {
				const sema::Expr& copy_expr = this->context.getSemaBuffer().getCopy(expr.copyID());

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->get_expr_register(copy_expr);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->get_expr_pointer(copy_expr);
					
				}else if constexpr(MODE == GetExprMode::STORE){
					this->get_expr_store(copy_expr, store_locations);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::MOVE: {
				const sema::Expr& move_expr = this->context.getSemaBuffer().getMove(expr.moveID());

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->get_expr_register(move_expr);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->get_expr_pointer(move_expr);
					
				}else if constexpr(MODE == GetExprMode::STORE){
					this->get_expr_store(move_expr, store_locations);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::FORWARD: {
				evo::unimplemented("lower sema::Expr::Kind::Forward");
			} break;

			case sema::Expr::Kind::FUNC_CALL: {
				const sema::FuncCall& func_call = this->context.getSemaBuffer().getFuncCall(expr.funcCallID());

				if(func_call.target.is<sema::TemplateIntrinsicFuncInstantiation::ID>()){
					return this->template_intrinsic_func_call<MODE>(func_call, store_locations);
				}

				const Data::FuncInfo& target_func_info = this->data.get_func(func_call.target.as<sema::Func::ID>());

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : func_call.args){
					if(target_func_info.params[i].is_copy()){
						args.emplace_back(this->get_expr_register(arg));
					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}


				const BaseType::Function& target_type = this->context.getTypeManager().getFunction(
					this->context.getSemaBuffer().getFunc(func_call.target.as<sema::Func::ID>()).typeID
				);

				if(target_type.hasNamedReturns()){
					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Type return_type = this->get_type(target_type.returnParams[0].typeID.asTypeID());

						const pir::Expr retunr_alloc = this->agent.createAlloca(return_type);
						args.emplace_back(retunr_alloc);
						this->agent.createCallVoid(target_func_info.pir_id, std::move(args));

						return this->agent.createLoad(
							retunr_alloc, return_type, false, pir::AtomicOrdering::NONE
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Type return_type = this->get_type(target_type.returnParams[0].typeID.asTypeID());
						
						const pir::Expr retunr_alloc = this->agent.createAlloca(return_type);
						args.emplace_back(retunr_alloc);
						this->agent.createCallVoid(target_func_info.pir_id, std::move(args));

						return retunr_alloc;
						
					}else if constexpr(MODE == GetExprMode::STORE || MODE == GetExprMode::DISCARD){
						for(pir::Expr store_location : store_locations){
							args.emplace_back(store_location);
						}
						this->agent.createCallVoid(target_func_info.pir_id, std::move(args));
						return std::nullopt;
					}

				}else{
					const pir::Expr call_return  = this->agent.createCall(
						target_func_info.pir_id,
						std::move(args),
						this->name("CALL.{}", this->mangle_name(func_call.target.as<sema::Func::ID>()))
					);

					if constexpr(MODE == GetExprMode::REGISTER){
						return call_return;

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr alloca = this->agent.createAlloca(target_func_info.return_type);
						this->agent.createStore(alloca, call_return, false, pir::AtomicOrdering::NONE);
						return alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						this->agent.createStore(store_locations.front(), call_return, false, pir::AtomicOrdering::NONE);
						return std::nullopt;

					}else{
						return std::nullopt;
					}
				}
				
			} break;

			case sema::Expr::Kind::ADDR_OF: {
				evo::unimplemented("lower sema::Expr::Kind::ADDR_OF");
			} break;

			case sema::Expr::Kind::DEREF: {
				evo::unimplemented("lower sema::Expr::Kind::DEREF");
			} break;

			case sema::Expr::Kind::BLOCK_EXPR: {
				const sema::BlockExpr& block_expr = this->context.getSemaBuffer().getBlockExpr(expr.blockExprID());

				const std::string_view label = this->current_source->getTokenBuffer()[block_expr.label].getString();

				const pir::BasicBlock::ID end_block = this->agent.createBasicBlock("BLOCK_EXPR.END");

				if constexpr(MODE == GetExprMode::REGISTER || MODE == GetExprMode::POINTER){
					auto label_output_locations = evo::SmallVector<pir::Expr>();
					const pir::Type output_type = this->get_type(block_expr.outputs[0].typeID);
					label_output_locations.emplace_back(
						this->agent.createAlloca(output_type, this->name("BLOCK_EXPR.OUTPUT"))
					);

					this->push_scope_level(label, std::move(label_output_locations), end_block);

				}else{
					this->push_scope_level(
						label, evo::SmallVector<pir::Expr>(store_locations.begin(), store_locations.end()), end_block
					);
				}
				

				for(const sema::Stmt& stmt : block_expr.block){
					this->lower_stmt(stmt);
				}

				this->agent.setTargetBasicBlock(end_block);

				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Expr output = this->get_current_scope_level().label_output_locations[0];
					this->pop_scope_level();
					return this->agent.createLoad(
						output,
						this->agent.getAlloca(output).type,
						false,
						pir::AtomicOrdering::NONE,
						this->name("LOAD.BLOCK_EXPR.OUTPUT")
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr output = this->get_current_scope_level().label_output_locations[0];
					this->pop_scope_level();
					return output;

				}else{
					this->pop_scope_level();
					return std::nullopt;
				}

			} break;

			case sema::Expr::Kind::TRY_ELSE: {
				const sema::TryElse& try_else = this->context.getSemaBuffer().getTryElse(expr.tryElseID());
				
				const sema::FuncCall& attempt_func_call =
					this->context.getSemaBuffer().getFuncCall(try_else.attempt.funcCallID());

				const Data::FuncInfo& target_func_info = this->data.get_func(
					attempt_func_call.target.as<sema::Func::ID>()
				);

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : attempt_func_call.args){
					if(target_func_info.params[i].is_copy()){
						args.emplace_back(this->get_expr_register(arg));
					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}

				const BaseType::Function& target_type = this->context.getTypeManager().getFunction(
					this->context.getSemaBuffer().getFunc(attempt_func_call.target.as<sema::Func::ID>()).typeID
				);

				const pir::Expr return_address = [&](){
					if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						return store_locations[0];
					}else{
						return this->agent.createAlloca(this->get_type(target_type.returnParams[0].typeID));
					}
				}();


				args.emplace_back(return_address);

				if(target_type.errorParams[0].typeID.isVoid() == false){
					const pir::Expr error_value = this->agent.createAlloca(
						*target_func_info.error_return_type, this->name("ERR.ALLOCA")
					);

					for(const sema::ExceptParam::ID except_param_id : try_else.exceptParams){
						const sema::ExceptParam& except_param =
							this->context.getSemaBuffer().getExceptParam(except_param_id);

						const pir::Expr except_param_pir_expr = this->agent.createCalcPtr(
							error_value,
							*target_func_info.error_return_type,
							evo::SmallVector<pir::CalcPtr::Index>{
								pir::CalcPtr::Index(0), pir::CalcPtr::Index(except_param.index)
							},
							this->name(
								"EXCEPT_PARAM.{}",
								this->current_source->getTokenBuffer()[
									this->context.getSemaBuffer().getExceptParam(except_param_id).ident
								].getString()
							)
						);
						this->local_func_exprs.emplace(sema::Expr(except_param_id), except_param_pir_expr);
					}

					args.emplace_back(error_value);
				}

				const pir::Expr attempt = this->agent.createCall(target_func_info.pir_id, std::move(args));

				const pir::BasicBlock::ID if_error_block = this->agent.createBasicBlock(this->name("TRY.ERROR"));
				const pir::BasicBlock::ID end_block = this->agent.createBasicBlock(this->name("TRY.END"));

				this->agent.createCondBranch(attempt, end_block, if_error_block);

				this->agent.setTargetBasicBlock(if_error_block);
				this->get_expr_store(try_else.except, return_address);
				this->agent.createBranch(end_block);

				this->agent.setTargetBasicBlock(end_block);

				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Type return_type = this->get_type(target_type.returnParams[0].typeID);
					return this->agent.createLoad(
						return_address, return_type, false, pir::AtomicOrdering::NONE
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return return_address;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::PARAM: {
				const sema::Param& sema_param = this->context.getSemaBuffer().getParam(expr.paramID());

				if(this->current_func_info->params[sema_param.abiIndex].is_copy()){
					const pir::Expr output = this->agent.createParamExpr(sema_param.abiIndex);

					if constexpr(MODE == GetExprMode::REGISTER){
						return output;

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Function& current_func = this->module.getFunction(this->current_func_info->pir_id);
						const pir::Expr alloca = this->agent.createAlloca(
							current_func.getParameters()[sema_param.index].getType()
						);
						this->agent.createStore(alloca, output, false, pir::AtomicOrdering::NONE);
						return alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						this->agent.createStore(store_locations.front(), output, false, pir::AtomicOrdering::NONE);
						return std::nullopt;

					}else{
						return std::nullopt;
					}

				}else{
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->agent.createLoad(
							this->agent.createParamExpr(sema_param.abiIndex),
							*this->current_func_info->params[sema_param.index].reference_type,
							false,
							pir::AtomicOrdering::NONE
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->agent.createParamExpr(sema_param.abiIndex);
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						const pir::Function& current_func = this->module.getFunction(this->current_func_info->pir_id);
						this->agent.createMemcpy(
							store_locations[0],
							this->agent.createParamExpr(sema_param.abiIndex),
							current_func.getParameters()[sema_param.index].getType(),
							false
						);
						return std::nullopt;

					}else{
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::RETURN_PARAM: {
				const sema::ReturnParam& sema_return_param =
					this->context.getSemaBuffer().getReturnParam(expr.returnParamID());

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->agent.createLoad(
						this->agent.createParamExpr(sema_return_param.abiIndex),
						*this->current_func_info->params[sema_return_param.index].reference_type,
						false,
						pir::AtomicOrdering::NONE
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->agent.createParamExpr(sema_return_param.abiIndex);
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::Function& current_func = this->module.getFunction(this->current_func_info->pir_id);

					this->agent.createMemcpy(
						store_locations[0],
						this->agent.createParamExpr(sema_return_param.abiIndex),
						current_func.getParameters()[sema_return_param.index].getType(),
						false
					);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;


			case sema::Expr::Kind::ERROR_RETURN_PARAM: {
				const sema::ErrorReturnParam& sema_error_param =
					this->context.getSemaBuffer().getErrorReturnParam(expr.errorReturnParamID());

				const pir::Expr calc_ptr = this->agent.createCalcPtr(
					this->agent.createParamExpr(sema_error_param.abiIndex),
					*this->current_func_info->error_return_type,
					evo::SmallVector<pir::CalcPtr::Index>{
						pir::CalcPtr::Index(0),
						pir::CalcPtr::Index(sema_error_param.index)
					}
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->agent.createLoad(
						calc_ptr,
						this->get_type(this->current_func_type->errorParams[sema_error_param.index].typeID.asTypeID()),
						false,
						pir::AtomicOrdering::NONE
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return calc_ptr;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					this->agent.createMemcpy(
						store_locations[0],
						calc_ptr,
						this->get_type(this->current_func_type->errorParams[sema_error_param.index].typeID.asTypeID()),
						false
					);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::BLOCK_EXPR_OUTPUT: {
				const sema::BlockExprOutput& block_expr_output_param =
					this->context.getSemaBuffer().getBlockExprOutput(expr.blockExprOutputID());

				const std::string_view label_str =
					this->current_source->getTokenBuffer()[block_expr_output_param.label].getString();

				for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
					if(scope_level.label != label_str){ continue; }

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->agent.createLoad(
							scope_level.label_output_locations[block_expr_output_param.index],
							this->get_type(block_expr_output_param.typeID),
							false,
							pir::AtomicOrdering::NONE,
							"LOAD.BLOCK_EXPR_OUTPUT"
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return scope_level.label_output_locations[block_expr_output_param.index];
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						this->agent.createMemcpy(
							store_locations[0],
							scope_level.label_output_locations[block_expr_output_param.index],
							this->get_type(block_expr_output_param.typeID),
							false
						);
						return std::nullopt;

					}else{
						return std::nullopt;
					}

					break;
				}
			} break;


			case sema::Expr::Kind::EXCEPT_PARAM: {
				if constexpr(MODE == GetExprMode::REGISTER){
					return this->agent.createLoad(
						this->local_func_exprs.at(expr),
						this->get_type(this->context.getSemaBuffer().getExceptParam(expr.exceptParamID()).typeID),
						false,
						pir::AtomicOrdering::NONE
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->local_func_exprs.at(expr);

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					this->agent.createMemcpy(
						store_locations[0],
						this->local_func_exprs.at(expr),
						this->get_type(this->context.getSemaBuffer().getExceptParam(expr.exceptParamID()).typeID),
						false
					);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::GLOBAL_VAR: {
				const pir::GlobalVar::ID pir_var_id = this->data.get_global_var(expr.globalVarID());
				
				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::GlobalVar& pir_var = this->module.getGlobalVar(pir_var_id);
					return this->agent.createLoad(
						this->agent.createGlobalValue(pir_var_id),
						pir_var.type,
						false,
						pir::AtomicOrdering::NONE,
						this->name("LOAD.{}", this->mangle_name(expr.globalVarID()))
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->agent.createGlobalValue(pir_var_id);
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::GlobalVar& pir_var = this->module.getGlobalVar(pir_var_id);
					this->agent.createMemcpy(
						store_locations[0], this->agent.createGlobalValue(pir_var_id), pir_var.type, false
					);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::FUNC: {
				evo::unimplemented("lower sema::Expr::Kind::FUNC");
			} break;
		}

		evo::unreachable();
	}



	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::template_intrinsic_func_call(
		const sema::FuncCall& func_call, evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
		const sema::TemplateIntrinsicFuncInstantiation& instantiation = 
			this->context.getSemaBuffer().getTemplateIntrinsicFuncInstantiation(
				func_call.target.as<sema::TemplateIntrinsicFuncInstantiation::ID>()
			);

		switch(instantiation.kind){
			case TemplateIntrinsicFunc::Kind::SIZE_OF: {
				evo::debugFatalBreak("@sizeOf is constexpr only");
			} break;

			case TemplateIntrinsicFunc::Kind::BIT_CAST: {
				const pir::Type from_type = this->get_type(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type = this->get_type(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				if(from_type == to_type){
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->get_expr_register(func_call.args[0]);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->get_expr_pointer(func_call.args[0]);

					}else{
						this->get_expr_store(func_call.args[0], store_locations);
						return std::nullopt;
					}
				}


				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createBitCast(from_value, to_type);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value, false, pir::AtomicOrdering::NONE);
					return pointer_alloca;

				}else{
					this->agent.createStore(store_locations[0], register_value, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;


			case TemplateIntrinsicFunc::Kind::TRUNC: {
				const pir::Type from_type = this->get_type(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type = this->get_type(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				if(from_type == to_type){
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->get_expr_register(func_call.args[0]);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->get_expr_pointer(func_call.args[0]);

					}else{
						this->get_expr_store(func_call.args[0], store_locations);
						return std::nullopt;
					}
				}


				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createTrunc(from_value, to_type);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value, false, pir::AtomicOrdering::NONE);
					return pointer_alloca;

				}else{
					this->agent.createStore(store_locations[0], register_value, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FTRUNC: {
				const pir::Type from_type = this->get_type(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type = this->get_type(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				if(from_type == to_type){
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->get_expr_register(func_call.args[0]);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->get_expr_pointer(func_call.args[0]);

					}else{
						this->get_expr_store(func_call.args[0], store_locations);
						return std::nullopt;
					}
				}

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createFTrunc(from_value, to_type);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value, false, pir::AtomicOrdering::NONE);
					return pointer_alloca;

				}else{
					this->agent.createStore(store_locations[0], register_value, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SEXT: {
				const pir::Type from_type = this->get_type(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type = this->get_type(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				if(from_type == to_type){
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->get_expr_register(func_call.args[0]);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->get_expr_pointer(func_call.args[0]);

					}else{
						this->get_expr_store(func_call.args[0], store_locations);
						return std::nullopt;
					}
				}

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createSExt(from_value, to_type);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value, false, pir::AtomicOrdering::NONE);
					return pointer_alloca;

				}else{
					this->agent.createStore(store_locations[0], register_value, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::ZEXT: {
				const pir::Type from_type = this->get_type(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type = this->get_type(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				if(from_type == to_type){
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->get_expr_register(func_call.args[0]);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->get_expr_pointer(func_call.args[0]);

					}else{
						this->get_expr_store(func_call.args[0], store_locations);
						return std::nullopt;
					}
				}

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createZExt(from_value, to_type);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value, false, pir::AtomicOrdering::NONE);
					return pointer_alloca;

				}else{
					this->agent.createStore(store_locations[0], register_value, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FEXT: {
				const pir::Type from_type = this->get_type(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type = this->get_type(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				if(from_type == to_type){
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->get_expr_register(func_call.args[0]);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->get_expr_pointer(func_call.args[0]);

					}else{
						this->get_expr_store(func_call.args[0], store_locations);
						return std::nullopt;
					}
				}

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createFExt(from_value, to_type);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value, false, pir::AtomicOrdering::NONE);
					return pointer_alloca;

				}else{
					this->agent.createStore(store_locations[0], register_value, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::I_TO_F: {
				const pir::Type to_type = this->get_type(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());
				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createIToF(from_value, to_type);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value, false, pir::AtomicOrdering::NONE);
					return pointer_alloca;

				}else{
					this->agent.createStore(store_locations[0], register_value, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::UI_TO_F: {
				const pir::Type to_type = this->get_type(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());
				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createUIToF(from_value, to_type);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value, false, pir::AtomicOrdering::NONE);
					return pointer_alloca;

				}else{
					this->agent.createStore(store_locations[0], register_value, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::F_TO_I: {
				const pir::Type to_type = this->get_type(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());
				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createFToI(from_value, to_type);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value, false, pir::AtomicOrdering::NONE);
					return pointer_alloca;

				}else{
					this->agent.createStore(store_locations[0], register_value, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::F_TO_UI: {
				const pir::Type to_type = this->get_type(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());
				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createFToUI(from_value, to_type);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value, false, pir::AtomicOrdering::NONE);
					return pointer_alloca;

				}else{
					this->agent.createStore(store_locations[0], register_value, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;


			case TemplateIntrinsicFunc::Kind::_MAX_: {
				evo::debugFatalBreak("not a valid template intrinsic func");
			} break;
		}

		evo::unreachable();
	}



	auto SemaToPIR::intrinsic_func_call(const sema::FuncCall& func_call) -> void {
		const IntrinsicFunc::Kind intrinsic_func_kind = func_call.target.as<IntrinsicFunc::Kind>();

		const auto get_args = [&](evo::SmallVector<pir::Expr>& args) -> void {
			const TypeManager& type_manager = this->context.getTypeManager();

			const TypeInfo::ID intrinsic_type_id = this->context.getIntrinsicFuncInfo(intrinsic_func_kind).typeID;
			const TypeInfo& intrinsic_type = type_manager.getTypeInfo(intrinsic_type_id);
			const BaseType::Function& func_type = type_manager.getFunction(intrinsic_type.baseTypeID().funcID());

			for(size_t i = 0; const sema::Expr& arg : func_call.args){
				if(func_type.params[i].shouldCopy){
					args.emplace_back(this->get_expr_register(arg));
				}else{
					args.emplace_back(this->get_expr_pointer(arg));
				}

				i += 1;
			}
		};

		const auto get_context_ptr = [&]() -> pir::Expr {
			return this->agent.createNumber(
				this->module.createIntegerType(sizeof(size_t) * 8),
				core::GenericInt::create<size_t>(size_t(&this->context))
			);
		};

		switch(intrinsic_func_kind){
			case IntrinsicFunc::Kind::ABORT: {
				this->agent.createAbort();
			} break;

			case IntrinsicFunc::Kind::BREAKPOINT: {
				this->agent.createBreakpoint();
			} break;

			case IntrinsicFunc::Kind::BUILD_SET_NUM_THREADS: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->agent.createCallVoid(this->data.getJITBuildFuncs().build_set_num_threads, std::move(args));
			} break;

			case IntrinsicFunc::Kind::BUILD_SET_OUTPUT: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->agent.createCallVoid(this->data.getJITBuildFuncs().build_set_output, std::move(args));
			} break;

			case IntrinsicFunc::Kind::BUILD_SET_USE_STD_LIB: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->agent.createCallVoid(this->data.getJITBuildFuncs().build_set_use_std_lib, std::move(args));
			} break;

			case IntrinsicFunc::Kind::_MAX_: {
				evo::debugFatalBreak("Invalid intrinsic func");
			} break;
		}
	}



	auto SemaToPIR::get_global_var_value(const sema::Expr expr) -> pir::GlobalVar::Value {
		switch(expr.kind()){
			case sema::Expr::Kind::NONE: evo::debugFatalBreak("Invalid Expr");

			case sema::Expr::Kind::UNINIT: {
				return pir::GlobalVar::Uninit();
			} break;

			case sema::Expr::Kind::ZEROINIT: {
				return pir::GlobalVar::Zeroinit();
			} break;

			case sema::Expr::Kind::INT_VALUE: {
				const sema::IntValue& int_value = this->context.getSemaBuffer().getIntValue(expr.intValueID());
				return this->agent.createNumber(this->get_type(*int_value.typeID), int_value.value);
			} break;

			case sema::Expr::Kind::FLOAT_VALUE: {
				const sema::FloatValue& float_value = this->context.getSemaBuffer().getFloatValue(expr.floatValueID());
				return this->agent.createNumber(this->get_type(*float_value.typeID), float_value.value);
			} break;

			case sema::Expr::Kind::BOOL_VALUE: {
				const sema::BoolValue& bool_value = this->context.getSemaBuffer().getBoolValue(expr.boolValueID());
				return this->agent.createBoolean(bool_value.value);
			} break;

			case sema::Expr::Kind::STRING_VALUE: {
				const sema::StringValue& string_value =
					this->context.getSemaBuffer().getStringValue(expr.stringValueID());

				return this->module.createGlobalString(evo::copy(string_value.value));
			} break;

			case sema::Expr::Kind::CHAR_VALUE: {
				const sema::CharValue& char_value = this->context.getSemaBuffer().getCharValue(expr.charValueID());
				return this->agent.createNumber(
					this->module.createIntegerType(8), core::GenericInt(8, uint64_t(char_value.value))
				);
			} break;

			case sema::Expr::Kind::MODULE_IDENT:       case sema::Expr::Kind::INTRINSIC_FUNC:
			case sema::Expr::Kind::TEMPLATED_INTRINSIC_FUNC_INSTANTIATION:
			case sema::Expr::Kind::COPY:               case sema::Expr::Kind::MOVE:
			case sema::Expr::Kind::FORWARD:            case sema::Expr::Kind::FUNC_CALL:
			case sema::Expr::Kind::ADDR_OF:            case sema::Expr::Kind::DEREF:
			case sema::Expr::Kind::TRY_ELSE:           case sema::Expr::Kind::BLOCK_EXPR:
			case sema::Expr::Kind::PARAM:              case sema::Expr::Kind::RETURN_PARAM:
			case sema::Expr::Kind::ERROR_RETURN_PARAM: case sema::Expr::Kind::BLOCK_EXPR_OUTPUT:
			case sema::Expr::Kind::EXCEPT_PARAM:       case sema::Expr::Kind::GLOBAL_VAR:
			case sema::Expr::Kind::FUNC: {
				evo::debugFatalBreak("Not valid global var value");
			} break;
		}

		evo::unreachable();
	}



	//////////////////////////////////////////////////////////////////////
	// get type


	auto SemaToPIR::get_type(const TypeInfo::VoidableID voidable_type_id) -> pir::Type {
		if(voidable_type_id.isVoid()){ return this->module.createVoidType(); }
		return this->get_type(voidable_type_id.asTypeID());
	}


	auto SemaToPIR::get_type(const TypeInfo::ID type_id) -> pir::Type {
		return this->get_type(this->context.getTypeManager().getTypeInfo(type_id));
	}

	auto SemaToPIR::get_type(const TypeInfo& type_info) -> pir::Type {
		if(type_info.isPointer()){ return this->module.createPtrType(); }

		if(type_info.isOptionalNotPointer()){
			evo::unimplemented("Optional type");
		}

		return this->get_type(type_info.baseTypeID());
	}


	auto SemaToPIR::get_type(const BaseType::ID base_type_id) -> pir::Type {
		switch(base_type_id.kind()){
			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Not a valid base type");
			
			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = 
					this->context.getTypeManager().getPrimitive(base_type_id.primitiveID());

				switch(primitive.kind()){
					case Token::Kind::TYPE_INT:      case Token::Kind::TYPE_ISIZE:   case Token::Kind::TYPE_UINT:
					case Token::Kind::TYPE_USIZE:    case Token::Kind::TYPE_TYPEID:  case Token::Kind::TYPE_C_SHORT:
					case Token::Kind::TYPE_C_USHORT: case Token::Kind::TYPE_C_INT:   case Token::Kind::TYPE_C_UINT:
					case Token::Kind::TYPE_C_LONG:   case Token::Kind::TYPE_C_ULONG: case Token::Kind::TYPE_C_LONG_LONG:
					case Token::Kind::TYPE_C_ULONG_LONG:
						return this->module.createIntegerType(
							uint32_t(this->context.getTypeManager().sizeOf(base_type_id) * 8)
						);

					case Token::Kind::TYPE_I_N:
					case Token::Kind::TYPE_UI_N:
						return this->module.createIntegerType(primitive.bitWidth());

					case Token::Kind::TYPE_F16:    return this->module.createFloatType(16);
					case Token::Kind::TYPE_BF16:   return this->module.createBFloatType();
					case Token::Kind::TYPE_F32:    return this->module.createFloatType(32);
					case Token::Kind::TYPE_F64:    return this->module.createFloatType(64);
					case Token::Kind::TYPE_F80:    return this->module.createFloatType(80);
					case Token::Kind::TYPE_F128:   return this->module.createFloatType(128);

					case Token::Kind::TYPE_BYTE:   return this->module.createIntegerType(8);
					case Token::Kind::TYPE_BOOL:   return this->module.createBoolType();
					case Token::Kind::TYPE_CHAR:   return this->module.createIntegerType(8);

					case Token::Kind::TYPE_RAWPTR: return this->module.createPtrType();

					case Token::Kind::TYPE_C_LONG_DOUBLE: 
						return this->context.getTypeManager().sizeOf(base_type_id) 
							? this->module.createFloatType(64)
							: this->module.createFloatType(128);

					default: evo::debugFatalBreak("Unknown builtin type");
				}
			} break;
			
			case BaseType::Kind::FUNCTION: {
				evo::unimplemented("BaseType::Kind::FUNCTION");
			} break;
			
			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->context.getTypeManager().getArray(base_type_id.arrayID());
				const pir::Type elem_type = this->get_type(array.elementTypeID);

				uint64_t length = array.lengths.front();
				for(size_t i = 1; i < array.lengths.size(); i+=1){
					length *= array.lengths[i];
				}

				if(array.terminator.has_value()){ length += 1; }

				return this->module.createArrayType(elem_type, length);
			} break;
			
			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->context.getTypeManager().getAlias(base_type_id.aliasID());
				return this->get_type(*alias.aliasedType.load());
			} break;
			
			case BaseType::Kind::TYPEDEF: {
				const BaseType::Typedef& typedef_type = 
					this->context.getTypeManager().getTypedef(base_type_id.typedefID());
				return this->get_type(*typedef_type.underlyingType.load());
			} break;
			
			case BaseType::Kind::STRUCT: {
				evo::unimplemented("BaseType::Kind::STRUCT");
			} break;
			
			case BaseType::Kind::STRUCT_TEMPLATE: {
				evo::debugFatalBreak("Cannot get type of struct template");
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				evo::debugFatalBreak("Cannot get type of type deducer");
			} break;
		}

		return this->module.createIntegerType(12);
	}



	//////////////////////////////////////////////////////////////////////
	// name mangling

	auto SemaToPIR::mangle_name(const BaseType::Struct::ID struct_id) const -> std::string {
		const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(struct_id);
		const Source& source = this->context.getSourceManager()[struct_type.sourceID];

		if(this->data.getConfig().useReadableNames){
			return std::format(
				"PTHR.s{}.{}", struct_id.get(), source.getTokenBuffer()[struct_type.identTokenID].getString()
			);
			
		}else{
			return std::format("PTHR.s{}", struct_id.get());
		}
	}

	auto SemaToPIR::mangle_name(const sema::GlobalVar::ID global_var_id) const -> std::string {
		const sema::GlobalVar& global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);
		const Source& source = this->context.getSourceManager()[global_var.sourceID];

		if(this->data.getConfig().useReadableNames){
			return std::format(
				"PTHR.v{}.{}", global_var_id.get(), source.getTokenBuffer()[global_var.ident].getString()
			);
			
		}else{
			return std::format("PTHR.v{}", global_var_id.get());
		}
	}


	auto SemaToPIR::mangle_name(const sema::Func::ID func_id) const -> std::string {
		const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);
		const Source& source = this->context.getSourceManager()[func.sourceID];

		if(func.name.kind() == AST::Kind::IDENT){
			if(this->data.getConfig().useReadableNames){
				return std::format(
					"PTHR.f{}.{}",
					func_id.get(),
					source.getTokenBuffer()[source.getASTBuffer().getIdent(func.name)].getString()
				);
			}else{
				return std::format("PTHR.f{}", func_id.get());
			}

		}else{
			evo::unimplemented("Name mangling of operator overload func name");
		}
	}





	auto SemaToPIR::name(std::string_view str) const -> std::string {
		if(this->data.getConfig().useReadableNames) [[unlikely]] {
			return std::string(str);
		}else{
			return std::string();
		}
	}


	template<class... Args>
	auto SemaToPIR::name(std::format_string<Args...> fmt, Args&&... args) const -> std::string {
		if(this->data.getConfig().useReadableNames) [[unlikely]] {
			return std::format(fmt, std::forward<Args...>(args)...);
		}else{
			return std::string();
		}
	}



	auto SemaToPIR::push_scope_level(auto&&... scope_level_args) -> void {
		this->scope_levels.emplace_back(std::forward<decltype(scope_level_args)>(scope_level_args)...);
	}

	auto SemaToPIR::pop_scope_level() -> void {
		evo::debugAssert(this->scope_levels.empty() == false, "No scope levels to pop");
		this->scope_levels.pop_back();
	}

	auto SemaToPIR::get_current_scope_level() -> ScopeLevel& {
		return this->scope_levels.back();
	}



}