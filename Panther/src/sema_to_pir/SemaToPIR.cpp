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
		for(uint32_t i = 0; i < this->context.getTypeManager().getNumStructs(); i+=1){
			this->lowerStructAndDependencies(BaseType::Struct::ID(i));
		}

		for(uint32_t i = 0; i < this->context.getTypeManager().getNumUnions(); i+=1){
			this->lowerUnionAndDependencies(BaseType::Union::ID(i));
		}

		for(const sema::GlobalVar::ID& global_var_id : this->context.getSemaBuffer().getGlobalVars()){
			this->lowerGlobalDecl(global_var_id);
			this->lowerGlobalDef(global_var_id);
		}

		for(const sema::Func::ID& func_id : this->context.getSemaBuffer().getFuncs()){
			const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);
			if(func.status == sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT){ continue; }
			if(func.status == sema::Func::Status::SUSPENDED){ continue; }

			this->lowerFuncDecl(func_id);
		}

		for(uint32_t i = 0; i < this->context.getTypeManager().getNumInterfaces(); i+=1){
			const auto interface_id = BaseType::Interface::ID(i);

			const BaseType::Interface& interface = this->context.getTypeManager().getInterface(interface_id);

			for(const auto& [target_type_id, impl] : interface.impls){
				this->lowerInterfaceVTable(interface_id, target_type_id, impl.methods);
			}
		}

		for(const sema::Func::ID& func_id : this->context.getSemaBuffer().getFuncs()){
			const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);
			if(func.status == sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT){ continue; }
			if(func.status == sema::Func::Status::SUSPENDED){ continue; }
			if(func.isClangFunc()){ continue; }

			this->lowerFuncDef(func_id);
		}
	}



	
	auto SemaToPIR::lowerStruct(BaseType::Struct::ID struct_id) -> pir::Type {
		return this->lower_struct<false>(struct_id);
	}

	auto SemaToPIR::lowerStructAndDependencies(BaseType::Struct::ID struct_id) -> pir::Type {
		return this->lower_struct<true>(struct_id);
	}


	auto SemaToPIR::lowerUnion(BaseType::Union::ID union_id) -> pir::Type {
		return this->lower_union<false>(union_id);
	}

	auto SemaToPIR::lowerUnionAndDependencies(BaseType::Union::ID union_id) -> pir::Type {
		return this->lower_union<true>(union_id);
	}


	auto SemaToPIR::lowerGlobalDecl(sema::GlobalVar::ID global_var_id) -> std::optional<pir::GlobalVar::ID> {
		const sema::GlobalVar& sema_global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);

		if(sema_global_var.kind == AST::VarDecl::Kind::DEF){ return std::nullopt; }

		const pir::GlobalVar::ID new_global_var = this->module.createGlobalVar(
			this->mangle_name(global_var_id),
			this->get_type<false>(*sema_global_var.typeID),
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



	auto SemaToPIR::lowerFuncDeclConstexpr(sema::Func::ID func_id) -> pir::Function::ID {
		return *this->lower_func_decl(func_id);
	}

	auto SemaToPIR::lowerFuncDecl(sema::Func::ID func_id) -> void {
		this->lower_func_decl(func_id);
	}


	// This is a separete function as the return for a non-constexpr func decl may be not useful as functions
	// 	 with in-params will have multiple funcs created, so the one returned is the one used for constexpr
	// Returns nullopt if is a clang func
	auto SemaToPIR::lower_func_decl(sema::Func::ID func_id) -> std::optional<pir::Function::ID> {
		const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);

		evo::debugAssert(
			func.status != sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT, "Incorrect status for lowering func decl"
		);

		if(func.isClangFunc() == false){
			this->current_source = &this->context.getSourceManager()[func.sourceID.as<Source::ID>()];
		}
		EVO_DEFER([&](){
			if(func.isClangFunc() == false){
				this->current_source = nullptr;
			}
		});

		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.typeID);

		auto params = evo::SmallVector<pir::Parameter>();
		params.reserve(func_type.params.size() + func_type.returnParams.size() + func_type.errorParams.size());

		uint32_t in_param_index = 0;
		auto param_infos = evo::SmallVector<Data::FuncInfo::Param>();
		param_infos.reserve(func_type.params.size());

		for(size_t i = 0; const BaseType::Function::Param& param : func_type.params){
			EVO_DEFER([&](){ i += 1; });

			std::string param_name = [&](){
				if(this->data.getConfig().useReadableNames == false){
					return std::format(".{}", params.size());
				}else{
					if(func.isClangFunc()){
						const std::string_view param_name =
							func.getParamName(func.params[i], this->context.getSourceManager());

						if(param_name.empty()){
							return std::format(".{}", params.size());
						}else{
							return std::string(param_name);
						}

					}else{
						const Token& token =
							this->current_source->getTokenBuffer()[func.params[i].ident.as<Token::ID>()];

						if(token.kind() == Token::Kind::KEYWORD_THIS){
							return std::string("this");
						}else{
							return std::string(token.getString());
						}
					}
				}
			}();

			auto attributes = evo::SmallVector<pir::Parameter::Attribute>();

			if(param.shouldCopy){
				const pir::Type param_type = this->get_type<false>(param.typeID);

				if(param_type.kind() == pir::Type::Kind::INTEGER){
					const TypeInfo::ID underlying_id = this->context.type_manager.getUnderlyingType(param.typeID);
					if(this->context.getTypeManager().isUnsignedIntegral(underlying_id)){
						attributes.emplace_back(pir::Parameter::Attribute::Unsigned());
					}else{
						attributes.emplace_back(pir::Parameter::Attribute::Signed());
					}
				}


				params.emplace_back(std::move(param_name), param_type, std::move(attributes));

				if(param.kind == AST::FuncDecl::Param::Kind::IN){
					param_infos.emplace_back(std::nullopt, in_param_index);
					in_param_index += 1;
				}else{
					param_infos.emplace_back(std::nullopt, std::nullopt);
				}

			}else{
				attributes.emplace_back(pir::Parameter::Attribute::PtrNonNull());
				attributes.emplace_back(
					pir::Parameter::Attribute::PtrDereferencable(this->context.getTypeManager().numBytes(param.typeID))
				);

				if(param.kind == AST::FuncDecl::Param::Kind::READ){
					attributes.emplace_back(pir::Parameter::Attribute::PtrReadOnly());
				}else{
					attributes.emplace_back(pir::Parameter::Attribute::PtrWritable());
					attributes.emplace_back(pir::Parameter::Attribute::PtrNoAlias());
				}

				params.emplace_back(std::move(param_name), this->module.createPtrType(), std::move(attributes));

				if(param.kind == AST::FuncDecl::Param::Kind::IN){
					param_infos.emplace_back(this->get_type<false>(param.typeID), in_param_index);
					in_param_index += 1;
				}else{
					param_infos.emplace_back(this->get_type<false>(param.typeID), std::nullopt);
				}
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
						this->context.getTypeManager().numBytes(return_param.typeID.asTypeID())
					)),
					pir::Parameter::Attribute(pir::Parameter::Attribute::PtrWritable()),
				};

				if(func_type.returnParams.size() == 1 && func_type.hasErrorReturn() == false){
					attributes.emplace_back(
						pir::Parameter::Attribute::PtrRVO(this->get_type<false>(return_param.typeID.asTypeID()))
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
					this->context.getTypeManager().numBytes(func_type.returnParams[0].typeID.asTypeID())
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
				error_return_param_types.emplace_back(this->get_type<false>(error_param.typeID));
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
				return this->get_type<false>(func_type.returnParams.front().typeID);
			}
		}();


		auto pir_funcs = evo::SmallVector<evo::Variant<pir::Function::ID, pir::ExternalFunction::ID>>();


		const pir::CallingConvention calling_conv = [&](){
			if(this->data.getConfig().isJIT || func.isExport){ return pir::CallingConvention::C; }
			return pir::CallingConvention::FAST;
		}();

		const pir::Linkage linkage = [&](){
			if(this->data.getConfig().isJIT || func.isExport){ return pir::Linkage::EXTERNAL; }
			return pir::Linkage::PRIVATE;
		}();


		if(func.hasInParam == false){
			if(func.isClangFunc()){
				std::string mangled_name = this->mangle_name(func_id);

				if(this->data.add_extern_func_if_needed(mangled_name)){ // prevent ODR violation
					const pir::ExternalFunction::ID created_external_func_id = this->module.createExternalFunction(
						std::move(mangled_name), std::move(params), pir::CallingConvention::C, linkage, return_type
					);

					pir_funcs.emplace_back(created_external_func_id);

					this->data.create_func(
						func_id,
						std::move(pir_funcs), // first arg of FuncInfo construction
						return_type,
						std::move(param_infos),
						std::move(return_params),
						error_return_param,
						error_return_type
					);
				}

				return std::nullopt;
				
			}else{
				const pir::Function::ID new_func_id = this->module.createFunction(
					this->mangle_name(func_id), std::move(params), calling_conv, linkage, return_type
				);

				pir_funcs.emplace_back(new_func_id);

				this->agent.setTargetFunction(new_func_id);
				this->agent.createBasicBlock(this->name("begin"));
				this->agent.removeTargetFunction();
			}


		}else{
			const size_t num_instantiations = 1ull << size_t(in_param_index);
			pir_funcs.reserve(num_instantiations);

			for(size_t i = 0; i < num_instantiations; i+=1){
				std::string name = this->mangle_name(func_id);

				if(this->data.getConfig().useReadableNames){
					name += ".in_";
					for(size_t j = 0; j < in_param_index; j+=1){
						name += bool((i >> j) & 1) ? 'C' : 'M';
					}
				}else{
					name += ".in";
					name += std::to_string(i);
				}

				const pir::Function::ID new_func_id = this->module.createFunction(
					std::move(name), evo::copy(params), calling_conv, linkage, return_type
				);

				this->agent.setTargetFunction(new_func_id);
				this->agent.createBasicBlock(this->name("begin"));
				this->agent.removeTargetFunction();

				pir_funcs.emplace_back(new_func_id);
			}
		}


		const pir::Function::ID new_func_id = pir_funcs.back().as<pir::Function::ID>();

		this->data.create_func(
			func_id,
			std::move(pir_funcs), // first arg of FuncInfo construction
			return_type,
			std::move(param_infos),
			std::move(return_params),
			error_return_param,
			error_return_type
		);

		return new_func_id;
	}



	auto SemaToPIR::lowerFuncDef(sema::Func::ID func_id) -> void {
		const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(func_id);
		evo::debugAssert(sema_func.status == sema::Func::Status::DEF_DONE, "Incorrect status for lowering func def");
		evo::debugAssert(sema_func.isClangFunc() == false, "cannot lower def of clang func");

		this->current_source = &this->context.getSourceManager()[sema_func.sourceID.as<Source::ID>()];
		EVO_DEFER([&](){ this->current_source = nullptr; });

		this->current_func_type = &this->context.getTypeManager().getFunction(sema_func.typeID);

		const SemaToPIRData::FuncInfo& func_info = this->data.get_func(func_id);

		this->in_param_bitmap = 0;
		for(const evo::Variant<pir::Function::ID, pir::ExternalFunction::ID> pir_id : func_info.pir_ids){
			pir::Function& func = this->module.getFunction(pir_id.as<pir::Function::ID>());

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

			this->pop_scope_level();

			this->local_func_exprs.clear();

			this->in_param_bitmap += 1;
		}

		this->current_func_type = nullptr;
	}




	auto SemaToPIR::lowerInterfaceVTable(
		BaseType::Interface::ID interface_id, BaseType::ID type, const evo::SmallVector<sema::Func::ID>& funcs
	) -> void {
		std::string vtable_name = [&](){
			switch(type.kind()){
				case BaseType::Kind::PRIMITIVE:
					return std::format("PTHR.vtable.i{}.p{}", interface_id.get(), type.primitiveID().get());

				case BaseType::Kind::ARRAY:
					return std::format("PTHR.vtable.i{}.a{}", interface_id.get(), type.arrayID().get());

				case BaseType::Kind::DISTINCT_ALIAS:
					return std::format("PTHR.vtable.i{}.t{}", interface_id.get(), type.distinctAliasID().get());

				case BaseType::Kind::STRUCT:
					return std::format("PTHR.vtable.i{}.s{}", interface_id.get(), type.structID().get());

				case BaseType::Kind::UNION:
					return std::format("PTHR.vtable.i{}.u{}", interface_id.get(), type.unionID().get());

				case BaseType::Kind::DUMMY:        case BaseType::Kind::FUNCTION:
				case BaseType::Kind::ALIAS:        case BaseType::Kind::STRUCT_TEMPLATE:
				case BaseType::Kind::TYPE_DEDUCER: case BaseType::Kind::INTERFACE: {
					evo::debugFatalBreak("Not valid base type for VTable");
				} break;
			}

			evo::unreachable();
		}();


		auto vtable_values = evo::SmallVector<pir::GlobalVar::Value>();
		vtable_values.reserve(funcs.size());
		for(sema::Func::ID func_id : funcs){
			vtable_values.emplace_back(
				this->agent.createFunctionPointer(this->data.get_func(func_id).pir_ids[0].as<pir::Function::ID>())
			);
		}

		const pir::GlobalVar::ID vtable = this->module.createGlobalVar(
			std::move(vtable_name),
			this->module.createArrayType(this->module.createPtrType(), uint64_t(funcs.size())),
			pir::Linkage::EXTERNAL,
			this->module.createGlobalArray(this->module.createPtrType(), std::move(vtable_values)),
			true
		);

		this->data.create_vtable(SemaToPIRData::VTableID(interface_id, type), vtable);
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

		const pir::Expr entry_call = 
			this->agent.createCall(target_entry_func_info.pir_ids[0].as<pir::Function::ID>(), {});
		this->agent.createRet(entry_call);

		return entry_func_id;
	}


	auto SemaToPIR::createConsoleExecutableEntry(sema::Func::ID target_entry_func) -> pir::Function::ID {
		const Data::FuncInfo& target_entry_func_info = this->data.get_func(target_entry_func);


		const pir::Function::ID entry_func_id = this->module.createFunction(
			"main",
			{
				pir::Parameter("argc", this->module.createIntegerType(32)),
				pir::Parameter("argv", this->module.createPtrType()),
			},
			pir::CallingConvention::C,
			pir::Linkage::EXTERNAL,
			this->module.createIntegerType(32)
		);

		pir::Function& entry_func = this->module.getFunction(entry_func_id);

		this->agent.setTargetFunction(entry_func);

		this->agent.createBasicBlock();
		this->agent.setTargetBasicBlockAtEnd();

		const pir::Expr entry_call =
			this->agent.createCall(target_entry_func_info.pir_ids[0].as<pir::Function::ID>(), {});
		const pir::Expr zext = this->agent.createZExt(entry_call, this->module.createIntegerType(32));
		this->agent.createRet(zext);

		return entry_func_id;
	}


	auto SemaToPIR::createWindowedExecutableEntry(sema::Func::ID target_entry_func) -> pir::Function::ID {
		switch(this->context.getConfig().target.platform){
			case core::Target::Platform::WINDOWS: {
				const Data::FuncInfo& target_entry_func_info = this->data.get_func(target_entry_func);

				const pir::Function::ID entry_func_id = this->module.createFunction(
					"WinMain",
					{
						pir::Parameter("hInstance", this->module.createPtrType()),
						pir::Parameter("hPrevInstance", this->module.createPtrType()),
						pir::Parameter("lpCmdLine", this->module.createPtrType()),
						pir::Parameter("nShowCmd", this->module.createIntegerType(32)),
					},
					pir::CallingConvention::C,
					pir::Linkage::EXTERNAL,
					this->module.createIntegerType(32)
				);

				pir::Function& entry_func = this->module.getFunction(entry_func_id);

				this->agent.setTargetFunction(entry_func);

				this->agent.createBasicBlock();
				this->agent.setTargetBasicBlockAtEnd();

				std::ignore = this->agent.createCall(target_entry_func_info.pir_ids[0].as<pir::Function::ID>(), {});

				this->agent.createRet(
					this->agent.createNumber(this->module.createIntegerType(32), core::GenericInt::create<uint32_t>(0))
				);

				return entry_func_id;
			} break;

			case core::Target::Platform::LINUX: case core::Target::Platform::UNKNOWN: {
				return this->createConsoleExecutableEntry(target_entry_func);
			} break;
		}

		evo::debugFatalBreak("Unknown platform OS");
	}





	auto SemaToPIR::createFuncJITInterface(sema::Func::ID func_id, pir::Function::ID pir_func_id) -> pir::Function::ID {
		const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(func_id);
		this->current_source = &this->context.getSourceManager()[sema_func.sourceID.as<Source::ID>()];
		EVO_DEFER([&](){ this->current_source = nullptr; });

		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(sema_func.typeID);

		const pir::Function& target_pir_func = this->module.getFunction(pir_func_id);


		auto params = evo::SmallVector<pir::Parameter>{
			pir::Parameter("ARGS", this->module.createPtrType()),
			pir::Parameter("RET", this->module.createPtrType()),
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

		size_t param_i = 0;

		for(const BaseType::Function::Param& param : func_type.params){
			const pir::Expr param_calc_ptr = this->agent.createCalcPtr(
				this->agent.createParamExpr(0),
				this->module.createPtrType(),
				evo::SmallVector<pir::CalcPtr::Index>{int64_t(param_i)}
			);

			if(target_pir_func.getParameters()[param_i].getType().kind() == pir::Type::Kind::PTR){
				if(this->context.getTypeManager().getTypeInfo(param.typeID).isPointer()){
					args.emplace_back(
						this->agent.createLoad(
							this->agent.createLoad(param_calc_ptr, this->module.createPtrType()),
							target_pir_func.getParameters()[param_i].getType()
						)
					);
				}else{
					args.emplace_back(this->agent.createLoad(param_calc_ptr, this->module.createPtrType()));
				}

			}else{
				args.emplace_back(
					this->agent.createLoad(
						this->agent.createLoad(param_calc_ptr, this->module.createPtrType()),
						target_pir_func.getParameters()[param_i].getType()
					)
				);
			}

			param_i += 1;
		}

		if(func_type.hasNamedReturns()){
			for(const BaseType::Function::ReturnParam& ret_param : func_type.returnParams){
				std::ignore = ret_param;

				const pir::Expr param_calc_ptr = this->agent.createCalcPtr(
					this->agent.createParamExpr(0),
					this->module.createPtrType(),
					evo::SmallVector<pir::CalcPtr::Index>{int64_t(param_i)}
				);

				args.emplace_back(this->agent.createLoad(param_calc_ptr, this->module.createPtrType()));

				param_i += 1;
			}
		}

		if(target_pir_func.getReturnType().kind() == pir::Type::Kind::VOID){
			this->agent.createCallVoid(pir_func_id, std::move(args));
		}else{
			const pir::Expr func_return = this->agent.createCall(pir_func_id, std::move(args));
			this->agent.createStore(this->agent.createParamExpr(1), func_return);
		}

		

		///////////////////////////////////
		// done
		
		this->agent.createRet();

		return jit_interface_func_id;
	}




	template<bool MAY_LOWER_DEPENDENCY>
	auto SemaToPIR::lower_struct(BaseType::Struct::ID struct_id) -> pir::Type {
		const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(struct_id);

		if constexpr(MAY_LOWER_DEPENDENCY){
			if(this->data.has_struct(struct_id)){
				return this->data.get_struct(struct_id);
			}
		}


		auto member_var_types = evo::SmallVector<pir::Type>();

		if(struct_type.memberVarsABI.empty()){
			member_var_types.emplace_back(this->module.createIntegerType(1));
		}else{
			member_var_types.reserve(struct_type.memberVarsABI.size());
			for(const BaseType::Struct::MemberVar* member_var : struct_type.memberVarsABI){
				member_var_types.emplace_back(this->get_type<MAY_LOWER_DEPENDENCY>(member_var->typeID));
			}
		}

		const pir::Type new_type = this->module.createStructType(
			this->mangle_name(struct_id), std::move(member_var_types), struct_type.isPacked
		);

		this->data.create_struct(struct_id, new_type);

		return new_type;
	}


	template<bool MAY_LOWER_DEPENDENCY>
	auto SemaToPIR::lower_union(BaseType::Union::ID union_id) -> pir::Type {
		const BaseType::Union& union_info = this->context.getTypeManager().getUnion(union_id);

		const pir::Type new_type = [&](){
			if(union_info.isUntagged){
				return this->module.createArrayType(
					this->module.createIntegerType(8), this->context.getTypeManager().numBytes(BaseType::ID(union_id))
				);
			}else{
				evo::unimplemented();
			}
		}();

		this->data.create_union(union_id, new_type);

		return new_type;
	}



	auto SemaToPIR::lower_stmt(const sema::Stmt& stmt) -> void {
		switch(stmt.kind()){
			case sema::Stmt::Kind::VAR: {
				const sema::Var& var = this->context.getSemaBuffer().getVar(stmt.varID());

				if(var.kind == AST::VarDecl::Kind::DEF){ return; }

				const pir::Expr var_alloca = this->agent.createAlloca(
					this->get_type<false>(*var.typeID),
					this->name("{}.ALLOCA", this->current_source->getTokenBuffer()[var.ident].getString())
				);

				this->local_func_exprs.emplace(sema::Expr(stmt.varID()), var_alloca);

				this->get_expr_store(var.expr, var_alloca);
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

				const BaseType::Function& target_type = this->context.getTypeManager().getFunction(
					this->context.getSemaBuffer().getFunc(func_call.target.as<sema::Func::ID>()).typeID
				);

				const uint32_t target_in_param_bitmap = this->calc_in_param_bitmap(target_type, func_call.args);

				if(target_func_info.return_type.kind() == pir::Type::Kind::VOID){
					this->create_call_void(target_func_info.pir_ids[target_in_param_bitmap], std::move(args));
				}else{
					std::ignore = this->create_call(target_func_info.pir_ids[target_in_param_bitmap], std::move(args));
				}
			} break;

			case sema::Stmt::Kind::INTERFACE_CALL: {
				const sema::InterfaceCall& interface_call =
					this->context.getSemaBuffer().getInterfaceCall(stmt.interfaceCallID());


				///////////////////////////////////
				// create target func type

				const BaseType::Function& target_func_type =
					this->context.getTypeManager().getFunction(interface_call.funcTypeID);

				const pir::Type return_type = [&](){
					if(target_func_type.hasNamedReturns()){ return this->module.createVoidType(); }
					if(target_func_type.returnsVoid()){ return this->module.createVoidType(); }
					return this->get_type<false>(target_func_type.returnParams[0].typeID.asTypeID());
				}();

				auto param_types = evo::SmallVector<pir::Type>();
				for(const BaseType::Function::Param& param : target_func_type.params){
					if(param.shouldCopy){
						param_types.emplace_back(this->get_type<false>(param.typeID));
					}else{
						param_types.emplace_back(this->module.createPtrType());
					}
				}
				if(target_func_type.hasNamedReturns()){
					for(size_t i = 0; i < target_func_type.returnParams.size(); i+=1){
						param_types.emplace_back(this->module.createPtrType());
					}
				}


				const pir::Type func_pir_type = this->module.createFunctionType(
					std::move(param_types),
					this->data.getConfig().isJIT ? pir::CallingConvention::C : pir::CallingConvention::FAST,
					return_type
				);


				///////////////////////////////////
				// get func pointer

				const pir::Expr target_interface_ptr = this->get_expr_pointer(interface_call.value);
				const pir::Type interface_ptr_type = this->data.getInterfacePtrType(this->module);

				const pir::Expr vtable_ptr = this->agent.createCalcPtr(
					target_interface_ptr,
					interface_ptr_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".VTABLE.PTR")
				);
				const pir::Expr vtable = this->agent.createLoad(
					vtable_ptr,
					this->module.createPtrType(),
					false,
					pir::AtomicOrdering::NONE,
					this->name(".VTABLE")
				);

				const pir::Expr target_func_ptr = this->agent.createCalcPtr(
					vtable,
					this->module.createPtrType(),
					evo::SmallVector<pir::CalcPtr::Index>{interface_call.index},
					this->name(".VTABLE.FUNC.PTR")
				);
				const pir::Expr target_func = this->agent.createLoad(
					target_func_ptr,
					this->module.createPtrType(),
					false,
					pir::AtomicOrdering::NONE,
					this->name(".VTABLE.FUNC")
				);


				///////////////////////////////////
				// make call

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : interface_call.args){
					if(target_func_type.params[i].shouldCopy){
						args.emplace_back(this->get_expr_register(arg));
					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}

				if(return_type.kind() == pir::Type::Kind::VOID){
					this->agent.createCallVoid(target_func, func_pir_type, std::move(args));
				}else{
					std::ignore = this->agent.createCall(target_func, func_pir_type, std::move(args));
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
								this->get_type<false>(target.as<TypeInfo::ID>()), this->name(".DISCARD")
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

					const std::optional<pir::Expr> ret_value = [&]() -> std::optional<pir::Expr> {
						if(return_stmt.value.has_value()){
							return this->get_expr_register(*return_stmt.value);
						}else{
							return std::nullopt;
						}
					}();

					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						if(scope_level.label != label){
							this->output_defers_for_scope_level<false>(scope_level);
							continue;
						}

						if(return_stmt.value.has_value()){
							this->agent.createStore(scope_level.label_output_locations[0], *ret_value);
						}

						this->output_defers_for_scope_level<false>(scope_level);

						this->agent.createJump(*scope_level.end_block);
						break;
					}

				}else{
					if(this->current_func_type->hasErrorReturn()){
						if(return_stmt.value.has_value()){
							const pir::Expr ret_value = this->get_expr_register(*return_stmt.value);

							for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
								this->output_defers_for_scope_level<false>(scope_level);
							}

							this->agent.createStore(this->current_func_info->return_params.front(), ret_value);
						}

						this->agent.createRet(this->agent.createBoolean(true));

					}else{
						if(return_stmt.value.has_value()){
							const pir::Expr ret_value = this->get_expr_register(*return_stmt.value);

							for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
								this->output_defers_for_scope_level<false>(scope_level);
							}

							this->agent.createRet(ret_value);

						}else{
							for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
								this->output_defers_for_scope_level<false>(scope_level);
							}
							this->agent.createRet();
						}
					}
				}

			} break;

			case sema::Stmt::Kind::ERROR: {
				const sema::Error& error_stmt = this->context.getSemaBuffer().getError(stmt.errorID());

				if(error_stmt.value.has_value()){
					this->agent.createStore(
						*this->current_func_info->error_return_param, this->get_expr_register(*error_stmt.value)
					);
					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<true>(scope_level);
					}
					this->agent.createRet(this->agent.createBoolean(false));

				}else{
					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<true>(scope_level);
					}
					this->agent.createRet(this->agent.createBoolean(false));
				}
				
			} break;

			case sema::Stmt::Kind::UNREACHABLE: {
				if(this->data.getConfig().useDebugUnreachables){
					this->agent.createBreakpoint();
					// TODO(FUTURE): proper panic
					this->agent.createAbort();
				}else{
					this->agent.createUnreachable();
				}
			} break;

			case sema::Stmt::Kind::BREAK: {
				const sema::Break& break_stmt = this->context.getSemaBuffer().getBreak(stmt.breakID());

				if(break_stmt.label.has_value()){
					const std::string_view label =
						this->current_source->getTokenBuffer()[*break_stmt.label].getString();

					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<true>(scope_level);

						if(scope_level.label == label){
							this->agent.createJump(*scope_level.end_block);
							break;
						}
					}
					
				}else{
					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<true>(scope_level);

						if(scope_level.is_loop){
							this->agent.createJump(*scope_level.end_block);
							break;
						}
					}
				}
			} break;

			case sema::Stmt::Kind::CONTINUE: {
				const sema::Continue& continue_stmt = this->context.getSemaBuffer().getContinue(stmt.continueID());

				if(continue_stmt.label.has_value()){
					const std::string_view label =
						this->current_source->getTokenBuffer()[*continue_stmt.label].getString();

					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<true>(scope_level);

						if(scope_level.label == label){
							this->agent.createJump(*scope_level.begin_block);
							break;
						}
					}
					
				}else{
					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<true>(scope_level);

						if(scope_level.is_loop){
							this->agent.createJump(*scope_level.begin_block);
							break;
						}
					}
				}
			} break;

			case sema::Stmt::Kind::CONDITIONAL: {
				const sema::Conditional& conditional_stmt = 
					this->context.getSemaBuffer().getConditional(stmt.conditionalID());

				const pir::BasicBlock::ID then_block = this->agent.createBasicBlock("IF.THEN");
				auto end_block = std::optional<pir::BasicBlock::ID>();

				const pir::Expr cond_value = this->get_expr_register(conditional_stmt.cond);

				if(conditional_stmt.elseStmts.empty()){
					end_block = this->agent.createBasicBlock("IF.END");

					this->agent.createBranch(cond_value, then_block, *end_block);

					this->agent.setTargetBasicBlock(then_block);
					this->push_scope_level();
					for(const sema::Stmt& block_stmt : conditional_stmt.thenStmts){
						this->lower_stmt(block_stmt);
					}
					const bool then_terminated = conditional_stmt.thenStmts.isTerminated();
					if(then_terminated == false){
						this->output_defers_for_scope_level<false>(this->scope_levels.back());
					}
					this->pop_scope_level();
					if(then_terminated == false){
						this->agent.createJump(*end_block);
					}
				}else{
					const pir::BasicBlock::ID else_block = this->agent.createBasicBlock("IF.ELSE");

					const bool then_terminated = conditional_stmt.thenStmts.isTerminated();
					const bool else_terminated = conditional_stmt.elseStmts.isTerminated();

					this->agent.createBranch(cond_value, then_block, else_block);

					// then block
					this->agent.setTargetBasicBlock(then_block);
					this->push_scope_level();
					for(const sema::Stmt& block_stmt : conditional_stmt.thenStmts){
						this->lower_stmt(block_stmt);
					}
					if(then_terminated == false){
						this->output_defers_for_scope_level<false>(this->scope_levels.back());
					}
					this->pop_scope_level();

					// required because stuff in the then block might add basic blocks
					pir::BasicBlock& then_block_end = this->agent.getTargetBasicBlock();

					// else block
					this->push_scope_level();
					this->agent.setTargetBasicBlock(else_block);
					for(const sema::Stmt& block_stmt : conditional_stmt.elseStmts){
						this->lower_stmt(block_stmt);
					}
					if(else_terminated == false){
						this->output_defers_for_scope_level<false>(this->scope_levels.back());
					}
					this->pop_scope_level();

					// end block

					if(else_terminated && then_terminated){ return; }

					end_block = this->agent.createBasicBlock("IF.END");

					if(else_terminated == false){
						this->agent.createJump(*end_block);
					}

					if(then_terminated == false){
						this->agent.setTargetBasicBlock(then_block_end);
						this->agent.createJump(*end_block);
					}
				}


				this->agent.setTargetBasicBlock(*end_block);
			} break;

			case sema::Stmt::Kind::WHILE: {
				const sema::While& while_stmt = this->context.getSemaBuffer().getWhile(stmt.whileID());

				const pir::BasicBlock::ID cond_block = this->agent.createBasicBlock(this->name("WHILE.COND"));
				const pir::BasicBlock::ID body_block = this->agent.createBasicBlock(this->name("WHILE.BODY"));
				const pir::BasicBlock::ID end_block = this->agent.createBasicBlock(this->name("WHILE.END"));


				this->agent.createJump(cond_block);

				this->agent.setTargetBasicBlock(cond_block);
				const pir::Expr cond_value = this->get_expr_register(while_stmt.cond);
				this->agent.createBranch(cond_value, body_block, end_block);

				this->agent.setTargetBasicBlock(body_block);

				if(while_stmt.label.has_value()){
					const std::string_view label = 
						this->current_source->getTokenBuffer()[*while_stmt.label].getString();
					this->push_scope_level(label, evo::SmallVector<pir::Expr>(), cond_block, end_block, true);
				}else{
					this->push_scope_level("", evo::SmallVector<pir::Expr>(), cond_block, end_block, true);
				}

				for(const sema::Stmt& block_stmt : while_stmt.block){
					this->lower_stmt(block_stmt);
				}

				if(while_stmt.block.isTerminated() == false){
					this->output_defers_for_scope_level<false>(this->scope_levels.back());
					this->agent.createJump(cond_block);
				}

				this->pop_scope_level();
				this->agent.setTargetBasicBlock(end_block);
			} break;

			case sema::Stmt::Kind::DEFER: {
				const sema::Defer& defer_stmt = this->context.getSemaBuffer().getDefer(stmt.deferID());
				this->get_current_scope_level().defers.emplace_back(stmt.deferID(), defer_stmt.isErrorDefer);
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
				evo::debugFatalBreak("Not a valid sema::Expr to be lowered");
			} break;

			case sema::Expr::Kind::UNINIT: {
				if constexpr(MODE == GetExprMode::REGISTER){
					evo::debugFatalBreak("Cannot get register of [uninit]");

				}else if constexpr(MODE == GetExprMode::POINTER){
					evo::debugFatalBreak("Cannot get pointer of [uninit]");

				}else if constexpr(MODE == GetExprMode::STORE){
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::ZEROINIT: {
				if constexpr(MODE == GetExprMode::REGISTER){
					evo::debugFatalBreak("Cannot get register of [zeroinit]");

				}else if constexpr(MODE == GetExprMode::POINTER){
					evo::debugFatalBreak("Cannot get pointer of [zeroinit]");

				}else if constexpr(MODE == GetExprMode::STORE){
					const pir::Expr zero = this->agent.createNumber(
						this->module.createIntegerType(8), core::GenericInt::create<evo::byte>(0)
					);

					this->agent.createMemset(store_locations[0], zero, this->agent.getAlloca(store_locations[0]).type);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::NULL_VALUE: {
				const sema::Null& null_value = this->context.getSemaBuffer().getNull(expr.nullID());
				const TypeInfo& target_type_info = this->context.getTypeManager().getTypeInfo(*null_value.targetTypeID);

				if(target_type_info.isNormalPointer()){
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->agent.createNullptr();

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr ptr_alloca = this->agent.createAlloca(
							this->module.createPtrType(), this->name("NULL")
						);
						this->agent.createStore(ptr_alloca, this->agent.createNullptr());
						return ptr_alloca;

					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						this->agent.createStore(store_locations[0], this->agent.createNullptr());
						return std::nullopt;

					}else{
						return std::nullopt;
					}

				}else if(target_type_info.isInterfacePointer()){
					const pir::Type interface_ptr_type = this->data.getInterfacePtrType(this->module);

					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Expr interface_ptr_alloca = this->agent.createAlloca(interface_ptr_type);
						const pir::Expr calc_ptr = this->agent.createCalcPtr(
							interface_ptr_alloca, interface_ptr_type, evo::SmallVector<pir::CalcPtr::Index>{0, 0}
						);
						this->agent.createStore(calc_ptr, this->agent.createNullptr());

						return this->agent.createLoad(
							interface_ptr_alloca,
							interface_ptr_type,
							false,
							pir::AtomicOrdering::NONE,
							this->name("NULL")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr interface_ptr_alloca = this->agent.createAlloca(interface_ptr_type);
						const pir::Expr calc_ptr = this->agent.createCalcPtr(
							interface_ptr_alloca, interface_ptr_type, evo::SmallVector<pir::CalcPtr::Index>{0, 0}
						);
						this->agent.createStore(calc_ptr, this->agent.createNullptr());

						return interface_ptr_alloca;

					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						const pir::Expr calc_ptr = this->agent.createCalcPtr(
							store_locations[0], interface_ptr_type, evo::SmallVector<pir::CalcPtr::Index>{0, 0}
						);
						this->agent.createStore(calc_ptr, this->agent.createNullptr());
						return std::nullopt;

					}else{
						return std::nullopt;
					}

				}else{
					const pir::Type optional_type = this->get_type<false>(*null_value.targetTypeID);

					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Expr optional_alloca = this->agent.createAlloca(optional_type);
						const pir::Expr calc_ptr = this->agent.createCalcPtr(
							optional_alloca, optional_type, evo::SmallVector<pir::CalcPtr::Index>{0, 1}
						);
						this->agent.createStore(calc_ptr, this->agent.createBoolean(false));

						return this->agent.createLoad(
							optional_alloca,
							optional_type,
							false,
							pir::AtomicOrdering::NONE,
							this->name("NULL")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr optional_alloca = this->agent.createAlloca(optional_type);
						const pir::Expr calc_ptr = this->agent.createCalcPtr(
							optional_alloca, optional_type, evo::SmallVector<pir::CalcPtr::Index>{0, 1}
						);
						this->agent.createStore(calc_ptr, this->agent.createBoolean(false));

						return optional_alloca;

					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						const pir::Expr calc_ptr = this->agent.createCalcPtr(
							store_locations[0], optional_type, evo::SmallVector<pir::CalcPtr::Index>{0, 1}
						);
						this->agent.createStore(calc_ptr, this->agent.createBoolean(false));

						return std::nullopt;

					}else{
						return std::nullopt;
					}
				}

			} break;

			case sema::Expr::Kind::INT_VALUE: {
				const sema::IntValue& int_value = this->context.getSemaBuffer().getIntValue(expr.intValueID());
				const pir::Type value_type = this->get_type<false>(*int_value.typeID);
				const pir::Expr number = this->agent.createNumber(value_type, int_value.value);

				if constexpr(MODE == GetExprMode::REGISTER){
					return number;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(value_type, this->name(".NUMBER.ALLOCA"));
					this->agent.createStore(alloca, number);
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], number);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::FLOAT_VALUE: {
				const sema::FloatValue& float_value = this->context.getSemaBuffer().getFloatValue(expr.floatValueID());
				const pir::Type value_type = this->get_type<false>(*float_value.typeID);
				const pir::Expr number = this->agent.createNumber(value_type, float_value.value);

				if constexpr(MODE == GetExprMode::REGISTER){
					return number;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(value_type, this->name(".NUMBER.ALLOCA"));
					this->agent.createStore(alloca, number);
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], number);
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
						this->module.createBoolType(), this->name(".BOOLEAN.ALLOCA")
					);
					this->agent.createStore(alloca, boolean);
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], boolean);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::STRING_VALUE: {
				const sema::StringValue& string_value =
					this->context.getSemaBuffer().getStringValue(expr.stringValueID());

				const pir::GlobalVar::String::ID string_value_id = 
					this->module.createGlobalString(evo::copy(string_value.value));

				const pir::GlobalVar::ID string_id = this->module.createGlobalVar(
					std::format("PTHR.str{}", this->data.get_string_literal_id()),
					this->module.getGlobalString(string_value_id).type,
					pir::Linkage::PRIVATE,
					string_value_id,
					true
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->agent.createGlobalValue(string_id);

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(
						this->module.getGlobalString(string_value_id).type, this->name(".STR.ALLOCA")
					);
					this->agent.createStore(alloca, this->agent.createGlobalValue(string_id));
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], this->agent.createGlobalValue(string_id));
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::AGGREGATE_VALUE: {
				const sema::AggregateValue& aggregate =
					this->context.getSemaBuffer().getAggregateValue(expr.aggregateValueID());


				if constexpr(MODE != GetExprMode::DISCARD){
					const pir::Type pir_type = this->get_type<false>(aggregate.typeID);

					const pir::Expr initialization_target = [&](){
						if constexpr(MODE == GetExprMode::REGISTER || MODE == GetExprMode::POINTER){
							return this->agent.createAlloca(pir_type, ".AGGREGATE");
						}else{
							evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
							return store_locations[0];
						}
					}();


					if(aggregate.typeID.kind() == BaseType::Kind::STRUCT){
						const BaseType::Struct& struct_info =
							this->context.getTypeManager().getStruct(aggregate.typeID.structID());


						const std::string_view struct_name = struct_info.getName(this->context.getSourceManager());

						for(uint32_t i = 0; const sema::Expr& value : aggregate.values){
							const std::string_view memebr_name = struct_info.getMemberName(
								*struct_info.memberVarsABI[i], this->context.getSourceManager()
							);

							const pir::Expr calc_ptr = this->agent.createCalcPtr(
								initialization_target,
								pir_type,
								evo::SmallVector<pir::CalcPtr::Index>{0, i},
								this->name(".NEW.{}.{}", struct_name, memebr_name)
							);

							this->get_expr_store(value, calc_ptr);

							i += 1;
						}

					}else{
						for(uint32_t i = 0; const sema::Expr& value : aggregate.values){
							const pir::Expr calc_ptr = this->agent.createCalcPtr(
								initialization_target, pir_type, evo::SmallVector<pir::CalcPtr::Index>{0, i}
							);

							this->get_expr_store(value, calc_ptr);

							i += 1;
						}
					}
					



					if constexpr(MODE == GetExprMode::REGISTER){
						return this->agent.createLoad(initialization_target, pir_type);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return initialization_target;

					}else{
						return std::nullopt;
					}

				}else{
					for(const sema::Expr& value : aggregate.values){
						this->get_expr_discard(value);
					}
					return std::nullopt;
				}
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
					const pir::Expr alloca = this->agent.createAlloca(value_type, this->name(".NUMBER.ALLOCA"));
					this->agent.createStore(alloca, number);
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], number);
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
				const sema::Copy& copy_expr = this->context.getSemaBuffer().getCopy(expr.copyID());
				return this->expr_copy<MODE>(copy_expr.expr, copy_expr.exprTypeID, store_locations);
			} break;

			case sema::Expr::Kind::MOVE: {
				const sema::Move& move_expr = this->context.getSemaBuffer().getMove(expr.moveID());
				return this->expr_move<MODE>(move_expr.expr, move_expr.exprTypeID, store_locations);
			} break;

			case sema::Expr::Kind::FORWARD: {
				const sema::Forward& forward_expr = this->context.getSemaBuffer().getForward(expr.forwardID());

				if constexpr(MODE == GetExprMode::REGISTER || MODE == GetExprMode::STORE){
					const sema::Param& target_param = 
						this->context.getSemaBuffer().getParam(forward_expr.expr.paramID());
					const uint32_t in_param_index = *this->current_func_info->params[target_param.index].in_param_index;
					const bool param_is_copy = bool((this->in_param_bitmap >> in_param_index) & 1);

					if(param_is_copy){
						return this->expr_copy<MODE>(forward_expr.expr, forward_expr.exprTypeID, store_locations);
					}else{
						return this->expr_move<MODE>(forward_expr.expr, forward_expr.exprTypeID, store_locations);
					}

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->get_expr_pointer(forward_expr.expr);
					
				}else{
					return std::nullopt;
				}
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

				const uint32_t target_in_param_bitmap = this->calc_in_param_bitmap(target_type, func_call.args);

				if(target_type.hasNamedReturns()){
					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Type return_type =
							this->get_type<false>(target_type.returnParams[0].typeID.asTypeID());

						const pir::Expr return_alloc = this->agent.createAlloca(return_type);
						args.emplace_back(return_alloc);
						this->create_call_void(target_func_info.pir_ids[target_in_param_bitmap], std::move(args));

						return this->agent.createLoad(return_alloc, return_type);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Type return_type =
							this->get_type<false>(target_type.returnParams[0].typeID.asTypeID());
						
						const pir::Expr return_alloc = this->agent.createAlloca(return_type);
						args.emplace_back(return_alloc);
						this->create_call_void(
							target_func_info.pir_ids[target_in_param_bitmap], std::move(args)
						);

						return return_alloc;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						for(pir::Expr store_location : store_locations){
							args.emplace_back(store_location);
						}
						this->create_call_void(
							target_func_info.pir_ids[target_in_param_bitmap], std::move(args)
						);
						return std::nullopt;

					}else{
						const pir::Function& target_func = this->module.getFunction(
							target_func_info.pir_ids[target_in_param_bitmap].as<pir::Function::ID>()
						);

						const size_t current_num_args = args.size();
						for(size_t i = current_num_args; i < target_func.getParameters().size(); i+=1){
							args.emplace_back(this->agent.createAlloca(target_func.getParameters()[i].getType()));
						}

						this->create_call_void(
							target_func_info.pir_ids[target_in_param_bitmap], std::move(args)
						);
						return std::nullopt;
					}

				}else{
					const pir::Expr call_return  = this->create_call(
						target_func_info.pir_ids[target_in_param_bitmap],
						std::move(args),
						this->name("{}.CALL", this->mangle_name(func_call.target.as<sema::Func::ID>()))
					);

					if constexpr(MODE == GetExprMode::REGISTER){
						return call_return;

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr alloca = this->agent.createAlloca(target_func_info.return_type);
						this->agent.createStore(alloca, call_return);
						return alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						this->agent.createStore(store_locations.front(), call_return);
						return std::nullopt;

					}else{
						return std::nullopt;
					}
				}
				
			} break;

			case sema::Expr::Kind::ADDR_OF: {
				const sema::Expr& target = this->context.getSemaBuffer().getAddrOf(expr.addrOfID());
				const pir::Expr address = this->get_expr_pointer(target);

				if constexpr(MODE == GetExprMode::REGISTER){
					return address;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(this->module.createPtrType());
					this->agent.createStore(alloca, address);
					return alloca;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations.front(), address);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::IMPLICIT_CONVERSION_TO_OPTIONAL: {
				const sema::ImplicitConversionToOptional& implicit_conversion_to_optionals = 
					this->context.getSemaBuffer().getImplicitConversionToOptional(
						expr.implicitConversionToOptionalID()
					);

				const pir::Type target_type = this->get_type<false>(implicit_conversion_to_optionals.targetTypeID);

				const pir::Expr target = [&](){
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->agent.createAlloca(target_type, ".IMPLICIT_CONVERSION_TO_OPTIONAL.alloca");

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->agent.createAlloca(target_type, ".IMPLICIT_CONVERSION_TO_OPTIONAL.alloca");
						
					}else if constexpr(MODE == GetExprMode::STORE){
						return store_locations[0];

					}else{
						return this->agent.createAlloca(target_type, ".DISCARD.IMPLICIT_CONVERSION_TO_OPTIONAL.alloca");
					}
				}();


				const pir::Expr held_calc_ptr = this->agent.createCalcPtr(
					target, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 0}
				);\
				this->get_expr_store(implicit_conversion_to_optionals.expr, held_calc_ptr);


				const pir::Expr flag_calc_ptr = this->agent.createCalcPtr(
					target,
					target_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".IMPLICIT_CONVERSION_TO_OPTIONAL.flag")
				);
				this->agent.createStore(flag_calc_ptr, this->agent.createBoolean(true));

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->agent.createLoad(
						target,
						target_type,
						false,
						pir::AtomicOrdering::NONE,
						this->name("IMPLICIT_CONVERSION_TO_OPTIONAL")
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return target;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::OPTIONAL_NULL_CHECK: {
				const sema::OptionalNullCheck& optional_null_check = 
					this->context.getSemaBuffer().getOptionalNullCheck(expr.optionalNullCheckID());

				const pir::Expr cmp = [&]() -> pir::Expr {
					const TypeInfo& target_type_info =
						this->context.getTypeManager().getTypeInfo(optional_null_check.targetTypeID);

					if(target_type_info.isNormalPointer()){
						const pir::Expr lhs = this->get_expr_register(optional_null_check.expr);

						if(optional_null_check.equal){
							return this->agent.createIEq(
								lhs, this->agent.createNullptr(), this->name("OPT_IS_NULL")
							);
						}else{
							return this->agent.createINeq(
								lhs, this->agent.createNullptr(), this->name("OPT_ISNT_NULL")
							);
						}

					}else if(target_type_info.isInterfacePointer()){
						const pir::Expr lhs = this->get_expr_pointer(optional_null_check.expr);

						const pir::Type interface_ptr_type = this->data.getInterfacePtrType(this->module);
						const pir::Expr calc_ptr = this->agent.createCalcPtr(
							lhs, interface_ptr_type, evo::SmallVector<pir::CalcPtr::Index>{0, 0}
						);

						if(optional_null_check.equal){
							return this->agent.createIEq(
								calc_ptr, this->agent.createNullptr(), this->name("OPT_IS_NULL")
							);
						}else{
							return this->agent.createINeq(
								calc_ptr, this->agent.createNullptr(), this->name("OPT_ISNT_NULL")
							);
						}

					}else{
						const pir::Expr lhs = this->get_expr_pointer(optional_null_check.expr);

						const pir::Type target_type = this->get_type<false>(optional_null_check.targetTypeID);
						const pir::Expr calc_ptr = this->agent.createCalcPtr(
							lhs, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 1}
						);
						const pir::Expr flag = this->agent.createLoad(calc_ptr, this->module.createBoolType());

						if(optional_null_check.equal){
							return this->agent.createIEq(
								flag, this->agent.createBoolean(false), this->name("OPT_IS_NULL")
							);
						}else{
							return this->agent.createINeq(
								flag, this->agent.createBoolean(false), this->name("OPT_ISNT_NULL")
							);
						}
					}
				}();
				
				if constexpr(MODE == GetExprMode::REGISTER){
					return cmp;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(this->module.createBoolType());
					this->agent.createStore(alloca, cmp);
					return alloca;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], cmp);
					return std::nullopt;

				}else{
					return std::nullopt;
				}

			} break;

			case sema::Expr::Kind::DEREF: {
				const sema::Deref& deref = this->context.getSemaBuffer().getDeref(expr.derefID());

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->agent.createLoad(
						this->get_expr_register(deref.expr),
						this->get_type<false>(deref.targetTypeID),
						false,
						pir::AtomicOrdering::NONE,
						"DEREF"
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->get_expr_register(deref.expr);
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					this->agent.createMemcpy(
						store_locations.front(),
						this->get_expr_register(deref.expr),
						this->get_type<false>(deref.targetTypeID)
					);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::UNWRAP: {
				const sema::Unwrap& unwrap = this->context.getSemaBuffer().getUnwrap(expr.unwrapID());
				const TypeInfo& target_type_info = this->context.getTypeManager().getTypeInfo(unwrap.targetTypeID);

				if(target_type_info.isPointer()){
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->get_expr_register(unwrap.expr);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->get_expr_pointer(unwrap.expr);
						
					}else if constexpr(MODE == GetExprMode::STORE){
						this->get_expr_store(unwrap.expr, store_locations);
						return std::nullopt;

					}else{
						this->get_expr_discard(unwrap.expr);
						return std::nullopt;
					}

				}else{
					const pir::Type target_pir_type = this->get_type<false>(unwrap.targetTypeID);

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->agent.createCalcPtr(
							this->get_expr_pointer(unwrap.expr),
							target_pir_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name("UNWRAP")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr unwrap_alloca = this->agent.createAlloca(this->module.createPtrType());

						const pir::Expr calc_ptr = this->agent.createCalcPtr(
							this->get_expr_pointer(unwrap.expr),
							target_pir_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".UNWRAP")
						);

						this->agent.createStore(unwrap_alloca, calc_ptr);
						return unwrap_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						const pir::Expr calc_ptr = this->agent.createCalcPtr(
							this->get_expr_pointer(unwrap.expr),
							target_pir_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".UNWRAP")
						);

						this->agent.createStore(store_locations[0], calc_ptr);
						return std::nullopt;

					}else{
						this->get_expr_discard(unwrap.expr);
						return std::nullopt;
					}
				} 
			} break;

			case sema::Expr::Kind::ACCESSOR: {
				const sema::Accessor& accessor = this->context.getSemaBuffer().getAccessor(expr.accessorID());

				const pir::Type target_pir_type = this->get_type<false>(accessor.targetTypeID);

				const TypeInfo& target_type = this->context.getTypeManager().getTypeInfo(accessor.targetTypeID);
				const BaseType::Struct& target_struct_type = this->context.getTypeManager().getStruct(
					target_type.baseTypeID().structID()
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Expr calc_ptr = this->agent.createCalcPtr(
						this->get_expr_pointer(accessor.target),
						target_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, int64_t(accessor.memberABIIndex)},
						this->name("ACCESSOR")
					);

					return this->agent.createLoad(
						calc_ptr,
						this->get_type<false>(target_struct_type.memberVars[size_t(accessor.memberABIIndex)].typeID)
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->agent.createCalcPtr(
						this->get_expr_pointer(accessor.target),
						target_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, int64_t(accessor.memberABIIndex)},
						this->name("ACCESSOR")
					);

				}else if constexpr(MODE == GetExprMode::STORE){
					const pir::Expr calc_ptr = this->agent.createCalcPtr(
						this->get_expr_pointer(accessor.target),
						target_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, int64_t(accessor.memberABIIndex)},
						this->name(".ACCESSOR")
					);

					this->agent.createMemcpy(
						store_locations[0],
						calc_ptr,
						this->get_type<false>(target_struct_type.memberVars[size_t(accessor.memberABIIndex)].typeID)
					);
					return std::nullopt;

				}else{
					this->get_expr_discard(accessor.target);
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::PTR_ACCESSOR: {
				const sema::PtrAccessor& accessor = this->context.getSemaBuffer().getPtrAccessor(expr.ptrAccessorID());

				const pir::Type target_pir_type = this->get_type<false>(accessor.targetTypeID);

				const TypeInfo& target_type = this->context.getTypeManager().getTypeInfo(accessor.targetTypeID);
				const BaseType::Struct& target_struct_type = this->context.getTypeManager().getStruct(
					target_type.baseTypeID().structID()
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Expr calc_ptr = this->agent.createCalcPtr(
						this->get_expr_register(accessor.target),
						target_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, int64_t(accessor.memberABIIndex)},
						this->name("ACCESSOR")
					);

					return this->agent.createLoad(
						calc_ptr,
						this->get_type<false>(target_struct_type.memberVars[size_t(accessor.memberABIIndex)].typeID)
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->agent.createCalcPtr(
						this->get_expr_register(accessor.target),
						target_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, int64_t(accessor.memberABIIndex)},
						this->name("ACCESSOR")
					);

				}else if constexpr(MODE == GetExprMode::STORE){
					const pir::Expr calc_ptr = this->agent.createCalcPtr(
						this->get_expr_register(accessor.target),
						target_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, int64_t(accessor.memberABIIndex)},
						this->name(".ACCESSOR")
					);

					this->agent.createMemcpy(
						store_locations[0],
						calc_ptr,
						this->get_type<false>(target_struct_type.memberVars[size_t(accessor.memberABIIndex)].typeID)
					);
					return std::nullopt;

				}else{
					this->get_expr_discard(accessor.target);
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::UNION_ACCESSOR: {
				const sema::UnionAccessor& union_accessor = 
					this->context.getSemaBuffer().getUnionAccessor(expr.unionAccessorID());

				if constexpr(MODE == GetExprMode::REGISTER){
					const TypeInfo& target_type = 
						this->context.getTypeManager().getTypeInfo(union_accessor.targetTypeID);
					const BaseType::Union& target_union_type = this->context.getTypeManager().getUnion(
						target_type.baseTypeID().unionID()
					);

					return this->agent.createLoad(
						this->get_expr_pointer(union_accessor.target),
						this->get_type<false>(target_union_type.fields[union_accessor.fieldIndex].typeID.asTypeID()),
						false,
						pir::AtomicOrdering::NONE,
						this->name("UNION_ACCESSOR")
					);
					
				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->get_expr_pointer(union_accessor.target);
					
				}else if constexpr(MODE == GetExprMode::STORE){
					const TypeInfo& target_type = 
						this->context.getTypeManager().getTypeInfo(union_accessor.targetTypeID);
					const BaseType::Union& target_union_type = this->context.getTypeManager().getUnion(
						target_type.baseTypeID().unionID()
					);

					return this->agent.createMemcpy(
						store_locations[0],
						this->get_expr_pointer(union_accessor.target),
						this->get_type<false>(target_union_type.fields[union_accessor.fieldIndex].typeID.asTypeID())
					);
					
				}else{
					this->get_expr_discard(union_accessor.target);
					return std::nullopt;
				}
				
			} break;

			case sema::Expr::Kind::PTR_UNION_ACCESSOR: {
				const sema::PtrUnionAccessor& ptr_union_accessor = 
					this->context.getSemaBuffer().getPtrUnionAccessor(expr.ptrUnionAccessorID());

				if constexpr(MODE == GetExprMode::REGISTER){
					const TypeInfo& target_type = 
						this->context.getTypeManager().getTypeInfo(ptr_union_accessor.targetTypeID);
					const BaseType::Union& target_union_type = this->context.getTypeManager().getUnion(
						target_type.baseTypeID().unionID()
					);

					return this->agent.createLoad(
						this->get_expr_register(ptr_union_accessor.target),
						this->get_type<false>(
							target_union_type.fields[ptr_union_accessor.fieldIndex].typeID.asTypeID()
						),
						false,
						pir::AtomicOrdering::NONE,
						this->name("UNION_ACCESSOR")
					);
					
				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->get_expr_register(ptr_union_accessor.target);
					
				}else if constexpr(MODE == GetExprMode::STORE){
					const TypeInfo& target_type = 
						this->context.getTypeManager().getTypeInfo(ptr_union_accessor.targetTypeID);
					const BaseType::Union& target_union_type = this->context.getTypeManager().getUnion(
						target_type.baseTypeID().unionID()
					);

					return this->agent.createMemcpy(
						store_locations[0],
						this->get_expr_register(ptr_union_accessor.target),
						this->get_type<false>(target_union_type.fields[ptr_union_accessor.fieldIndex].typeID.asTypeID())
					);
					
				}else{
					this->get_expr_discard(ptr_union_accessor.target);
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::BLOCK_EXPR: {
				const sema::BlockExpr& block_expr = this->context.getSemaBuffer().getBlockExpr(expr.blockExprID());

				const std::string_view label = this->current_source->getTokenBuffer()[block_expr.label].getString();

				const pir::BasicBlock::ID end_block = this->agent.createBasicBlock("BLOCK_EXPR.END");

				if constexpr(MODE == GetExprMode::REGISTER || MODE == GetExprMode::POINTER){
					auto label_output_locations = evo::SmallVector<pir::Expr>();
					const pir::Type output_type = this->get_type<false>(block_expr.outputs[0].typeID);
					label_output_locations.emplace_back(
						this->agent.createAlloca(output_type, this->name(".BLOCK_EXPR.OUTPUT.ALLOCA"))
					);

					this->push_scope_level(label, std::move(label_output_locations), std::nullopt, end_block, false);

				}else{
					this->push_scope_level(
						label,
						evo::SmallVector<pir::Expr>(store_locations.begin(), store_locations.end()),
						std::nullopt,
						end_block,
						false
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
						this->name("BLOCK_EXPR.OUTPUT")
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

			case sema::Expr::Kind::FAKE_TERM_INFO: {
				evo::debugFatalBreak("Should never lower fake term info");
			} break;

			case sema::Expr::Kind::MAKE_INTERFACE_PTR: {
				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;
					
				}else{
					const sema::MakeInterfacePtr& make_interface_ptr =
						this->context.getSemaBuffer().getMakeInterfacePtr(expr.makeInterfacePtrID());

					const pir::Type interface_ptr_type = this->data.getInterfacePtrType(this->module);

					const pir::GlobalVar::ID vtable = this->data.get_vtable(
						SemaToPIRData::VTableID(make_interface_ptr.interfaceID, make_interface_ptr.implTypeID)
					);


					const pir::Expr target = [&](){
						if constexpr(MODE == GetExprMode::STORE){
							return store_locations[0];
						}else{
							return this->agent.createAlloca(interface_ptr_type);
						}
					}();


					const pir::Expr value_ptr = this->agent.createCalcPtr(
						target,
						interface_ptr_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 0},
						this->name(".MAKE_INTERFACE_PTR.VALUE")
					);
					this->get_expr_store(make_interface_ptr.expr, value_ptr);

					const pir::Expr vtable_ptr = this->agent.createCalcPtr(
						target,
						interface_ptr_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 1},
						this->name(".MAKE_INTERFACE_PTR.VTABLE")
					);
					this->agent.createStore(vtable_ptr, this->agent.createGlobalValue(vtable));

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->agent.createLoad(target, interface_ptr_type);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return target;

					}else if constexpr(MODE == GetExprMode::STORE){
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::INTERFACE_CALL: {
				const sema::InterfaceCall& interface_call =
					this->context.getSemaBuffer().getInterfaceCall(expr.interfaceCallID());


				///////////////////////////////////
				// create target func type

				const BaseType::Function& target_func_type =
					this->context.getTypeManager().getFunction(interface_call.funcTypeID);

				const pir::Type return_type = [&](){
					if(target_func_type.hasNamedReturns()){ return this->module.createVoidType(); }

					return this->get_type<false>(target_func_type.returnParams[0].typeID.asTypeID());
				}();

				auto param_types = evo::SmallVector<pir::Type>();
				for(const BaseType::Function::Param& param : target_func_type.params){
					if(param.shouldCopy){
						param_types.emplace_back(this->get_type<false>(param.typeID));
					}else{
						param_types.emplace_back(this->module.createPtrType());
					}
				}
				if(target_func_type.hasNamedReturns()){
					for(size_t i = 0; i < target_func_type.returnParams.size(); i+=1){
						param_types.emplace_back(this->module.createPtrType());
					}
				}


				const pir::Type func_pir_type = this->module.createFunctionType(
					std::move(param_types),
					this->data.getConfig().isJIT ? pir::CallingConvention::C : pir::CallingConvention::FAST,
					return_type
				);


				///////////////////////////////////
				// get func pointer

				const pir::Expr target_interface_ptr = this->get_expr_pointer(interface_call.value);
				const pir::Type interface_ptr_type = this->data.getInterfacePtrType(this->module);

				const pir::Expr vtable_ptr = this->agent.createCalcPtr(
					target_interface_ptr,
					interface_ptr_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".VTABLE.PTR")
				);
				const pir::Expr vtable = this->agent.createLoad(
					vtable_ptr,
					this->module.createPtrType(),
					false,
					pir::AtomicOrdering::NONE,
					this->name(".VTABLE")
				);

				const pir::Expr target_func_ptr = this->agent.createCalcPtr(
					vtable,
					this->module.createPtrType(),
					evo::SmallVector<pir::CalcPtr::Index>{interface_call.index},
					this->name(".VTABLE.FUNC.PTR")
				);
				const pir::Expr target_func = this->agent.createLoad(
					target_func_ptr,
					this->module.createPtrType(),
					false,
					pir::AtomicOrdering::NONE,
					this->name(".VTABLE.FUNC")
				);


				///////////////////////////////////
				// make call

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : interface_call.args){
					if(target_func_type.params[i].shouldCopy){
						args.emplace_back(this->get_expr_register(arg));
					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}


				if(target_func_type.hasNamedReturns()){
					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Type actual_return_type =
							this->get_type<false>(target_func_type.returnParams[0].typeID.asTypeID());

						const pir::Expr return_alloc = this->agent.createAlloca(
							actual_return_type, this->name(".INTERFACE_CALL.ALLOCA")
						);
						args.emplace_back(return_alloc);
						this->agent.createCallVoid(target_func, func_pir_type, std::move(args));

						return this->agent.createLoad(
							return_alloc,
							actual_return_type,
							false,
							pir::AtomicOrdering::NONE,
							this->name("INTERFACE_CALL")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Type actual_return_type =
							this->get_type<false>(target_func_type.returnParams[0].typeID.asTypeID());

						const pir::Expr return_alloc = this->agent.createAlloca(
							actual_return_type, this->name("INTERFACE_CALL.ALLOCA")
						);
						args.emplace_back(return_alloc);
						this->agent.createCallVoid(target_func, func_pir_type, std::move(args));

						return return_alloc;
						
					}else if constexpr(MODE == GetExprMode::STORE || MODE == GetExprMode::DISCARD){
						for(pir::Expr store_location : store_locations){
							args.emplace_back(store_location);
						}
						this->agent.createCallVoid(target_func, func_pir_type, std::move(args));
						return std::nullopt;
					}

				}else{
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->agent.createCall(
							target_func, func_pir_type, std::move(args), this->name("INTERFACE_CALL")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr call_return = this->agent.createCall(
							target_func, func_pir_type, std::move(args), this->name(".INTERFACE_CALL")
						);

						const pir::Expr alloca = this->agent.createAlloca(
							return_type, this->name("INTERFACE_CALL.ALLOCA")
						);
						this->agent.createStore(alloca, call_return);
						return alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						const pir::Expr call_return = this->agent.createCall(
							target_func, func_pir_type, std::move(args), this->name(".INTERFACE_CALL")
						);
						this->agent.createStore(store_locations.front(), call_return);
						return std::nullopt;

					}else{
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::INDEXER: {
				const sema::Indexer& indexer = this->context.getSemaBuffer().getIndexer(expr.indexerID());

				const pir::Expr target = this->get_expr_pointer(indexer.target);

				const pir::Type type_usize = this->get_type<false>(TypeManager::getTypeUSize());

				auto indices = evo::SmallVector<pir::CalcPtr::Index>();
				indices.reserve(indexer.indices.size() + 1);
				indices.emplace_back(
					pir::CalcPtr::Index(this->agent.createNumber(type_usize, core::GenericInt::create<uint64_t>(0)))
				);
				for(const sema::Expr& index : indexer.indices){
					indices.emplace_back(this->get_expr_register(index));
				}

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->agent.createCalcPtr(
						target, this->get_type<false>(indexer.targetTypeID), std::move(indices), this->name("INDEXER")
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr indexer_alloca = this->agent.createAlloca(
						this->module.createPtrType(), this->name(".INDEXER.alloca")
					);

					const pir::Expr calc_ptr = this->agent.createCalcPtr(
						target, this->get_type<false>(indexer.targetTypeID), std::move(indices), this->name(".INDEXER")
					);
					this->agent.createStore(indexer_alloca, calc_ptr);

					return indexer_alloca;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::Expr calc_ptr = this->agent.createCalcPtr(
						target, this->get_type<false>(indexer.targetTypeID), std::move(indices), this->name(".INDEXER")
					);

					this->agent.createStore(store_locations[0], calc_ptr);
					return std::nullopt;

				}else{
					return std::nullopt;
				}

			} break;

			case sema::Expr::Kind::PTR_INDEXER: {
				const sema::PtrIndexer& ptr_indexer = this->context.getSemaBuffer().getPtrIndexer(expr.ptrIndexerID());

				const pir::Expr target = this->get_expr_register(ptr_indexer.target);

				const pir::Type type_usize = this->get_type<false>(TypeManager::getTypeUSize());

				auto indices = evo::SmallVector<pir::CalcPtr::Index>();
				indices.reserve(ptr_indexer.indices.size() + 1);
				indices.emplace_back(
					pir::CalcPtr::Index(this->agent.createNumber(type_usize, core::GenericInt::create<uint64_t>(0)))
				);
				for(const sema::Expr& index : ptr_indexer.indices){
					indices.emplace_back(this->get_expr_register(index));
				}

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->agent.createCalcPtr(
						target,
						this->get_type<false>(ptr_indexer.targetTypeID),
						std::move(indices),
						this->name("PTR_INDEXER")
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr ptr_indexer_alloca = this->agent.createAlloca(
						this->module.createPtrType(), this->name(".PTR_INDEXER.alloca")
					);

					const pir::Expr calc_ptr = this->agent.createCalcPtr(
						target,
						this->get_type<false>(ptr_indexer.targetTypeID),
						std::move(indices),
						this->name(".PTR_INDEXER")
					);
					this->agent.createStore(ptr_indexer_alloca, calc_ptr);

					return ptr_indexer_alloca;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::Expr calc_ptr = this->agent.createCalcPtr(
						target,
						this->get_type<false>(ptr_indexer.targetTypeID),
						std::move(indices),
						this->name(".PTR_INDEXER")
					);

					this->agent.createStore(store_locations[0], calc_ptr);
					return std::nullopt;

				}else{
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
						return this->agent.createAlloca(this->get_type<false>(target_type.returnParams[0].typeID));
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

				const uint32_t target_in_param_bitmap = this->calc_in_param_bitmap(target_type, attempt_func_call.args);

				const pir::Expr attempt = this->create_call(
					target_func_info.pir_ids[target_in_param_bitmap], std::move(args)
				);

				const pir::BasicBlock::ID if_error_block = this->agent.createBasicBlock(this->name("TRY.ERROR"));
				const pir::BasicBlock::ID end_block = this->agent.createBasicBlock(this->name("TRY.END"));

				this->agent.createBranch(attempt, end_block, if_error_block);

				this->agent.setTargetBasicBlock(if_error_block);
				this->get_expr_store(try_else.except, return_address);
				this->agent.createJump(end_block);

				this->agent.setTargetBasicBlock(end_block);

				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Type return_type = this->get_type<false>(target_type.returnParams[0].typeID);
					return this->agent.createLoad(return_address, return_type);

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
						const pir::Function& current_func =
							this->module.getFunction(this->current_func_info->pir_ids[0].as<pir::Function::ID>());
						const pir::Expr alloca = this->agent.createAlloca(
							current_func.getParameters()[sema_param.index].getType()
						);
						this->agent.createStore(alloca, output);
						return alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						this->agent.createStore(store_locations.front(), output);
						return std::nullopt;

					}else{
						return std::nullopt;
					}

				}else{
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->agent.createLoad(
							this->agent.createParamExpr(sema_param.abiIndex),
							*this->current_func_info->params[sema_param.index].reference_type
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->agent.createParamExpr(sema_param.abiIndex);
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						const size_t num_bytes = this->context.getTypeManager().numBytes(
							this->current_func_type->params[sema_param.index].typeID
						);

						this->agent.createMemcpy(
							store_locations[0], this->agent.createParamExpr(sema_param.abiIndex), num_bytes	
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
						*this->current_func_info->params[sema_return_param.index].reference_type
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->agent.createParamExpr(sema_return_param.abiIndex);
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::Function& current_func = 
						this->module.getFunction(this->current_func_info->pir_ids[0].as<pir::Function::ID>());

					this->agent.createMemcpy(
						store_locations[0],
						this->agent.createParamExpr(sema_return_param.abiIndex),
						current_func.getParameters()[sema_return_param.index].getType()
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
						this->get_type<false>(
							this->current_func_type->errorParams[sema_error_param.index].typeID.asTypeID()
						)
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return calc_ptr;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					this->agent.createMemcpy(
						store_locations[0],
						calc_ptr,
						this->get_type<false>(
							this->current_func_type->errorParams[sema_error_param.index].typeID.asTypeID()
						)
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
							this->get_type<false>(block_expr_output_param.typeID),
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
							this->get_type<false>(block_expr_output_param.typeID)
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
						this->get_type<false>(
							this->context.getSemaBuffer().getExceptParam(expr.exceptParamID()).typeID
						)
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->local_func_exprs.at(expr);

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					this->agent.createMemcpy(
						store_locations[0],
						this->local_func_exprs.at(expr),
						this->get_type<false>(
							this->context.getSemaBuffer().getExceptParam(expr.exceptParamID()).typeID
						)
					);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::VAR: {
				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Expr var_alloca = this->local_func_exprs.at(expr);

					if(this->data.getConfig().useReadableNames){
						const pir::Alloca& var_actual_alloca = this->agent.getAlloca(var_alloca);
						std::string_view alloca_name = static_cast<std::string_view>(var_actual_alloca.name);
						alloca_name.remove_suffix(sizeof(".ALLOCA") - 1);

						return this->agent.createLoad(
							var_alloca,
							this->agent.getAlloca(var_alloca).type,
							false,
							pir::AtomicOrdering::NONE,
							std::string(alloca_name)
						);

					}else{
						return this->agent.createLoad(var_alloca, this->agent.getAlloca(var_alloca).type);	
					}

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->local_func_exprs.at(expr);

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::Expr var_alloca = this->local_func_exprs.at(expr);
					this->agent.createMemcpy(store_locations[0], var_alloca, this->agent.getAlloca(var_alloca).type);
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
						this->name("{}.LOAD", this->mangle_name(expr.globalVarID()))
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->agent.createGlobalValue(pir_var_id);
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::GlobalVar& pir_var = this->module.getGlobalVar(pir_var_id);
					this->agent.createMemcpy(
						store_locations[0], this->agent.createGlobalValue(pir_var_id), pir_var.type
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
	auto SemaToPIR::expr_copy(
		const sema::Expr& expr, TypeInfo::ID expr_type_id, evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
		if(this->context.getTypeManager().isTriviallyCopyable(expr_type_id)){
			if constexpr(MODE == GetExprMode::REGISTER){
				return this->get_expr_register(expr);

			}else if constexpr(MODE == GetExprMode::POINTER){
				return this->get_expr_pointer(expr);
				
			}else if constexpr(MODE == GetExprMode::STORE){
				this->get_expr_store(expr, store_locations[0]);
				return std::nullopt;

			}else{
				return std::nullopt;
			}
		}else{
			evo::unimplemented("Non-trivially-copyable copy");
		}
	}


	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::expr_move(
		const sema::Expr& expr, TypeInfo::ID expr_type_id, evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
		if(this->context.getTypeManager().isTriviallyCopyable(expr_type_id)){
			if constexpr(MODE == GetExprMode::REGISTER){
				return this->get_expr_register(expr);

			}else if constexpr(MODE == GetExprMode::POINTER){
				return this->get_expr_pointer(expr);
				
			}else if constexpr(MODE == GetExprMode::STORE){
				this->get_expr_store(expr, store_locations[0]);
				return std::nullopt;

			}else{
				return std::nullopt;
			}
		}else{
			evo::unimplemented("Non-trivially-movable move");
		}
	}




	auto SemaToPIR::deinit_expr(pir::Expr expr, TypeInfo::ID expr_type_id) -> void {
		evo::debugAssert(this->agent.getExprType(expr).kind() == pir::Type::Kind::PTR, "expr must be a pointer");

		if(this->context.getTypeManager().isTriviallyDeletable(expr_type_id)){ return; }

		const TypeInfo& expr_type = this->context.getTypeManager().getTypeInfo(expr_type_id);

		if(expr_type.isOptionalNotPointer()){
			if(this->context.getTypeManager().isTriviallyDeletable(expr_type_id) == false){
				auto held_qualifiers = evo::SmallVector<AST::Type::Qualifier>(
					expr_type.qualifiers().begin(), std::prev(expr_type.qualifiers().end())
				);
				const TypeInfo::ID held_type = this->context.type_manager.getOrCreateTypeInfo(
					TypeInfo(expr_type.baseTypeID(), std::move(held_qualifiers))
				);

				const pir::Expr held_data = this->agent.createCalcPtr(
					expr, this->get_type<false>(expr_type_id), evo::SmallVector<pir::CalcPtr::Index>{0, 0}
				);

				this->deinit_expr(held_data, held_type);

				// const pir::Expr opt_flag = this->agent.createCalcPtr(
				// 	expr, this->get_type<false>(expr_type_id), evo::SmallVector<pir::CalcPtr::Index>{0, 1}
				// );
				// this->agent.createStore(opt_flag, this->agent.createBoolean(false));
			}

			return;
		}

		evo::unimplemented("non-trivially destruct");
	}




	auto SemaToPIR::calc_in_param_bitmap(
		const BaseType::Function& target_func_type, evo::ArrayProxy<sema::Expr> args
	) const -> uint32_t {
		uint32_t output_in_param_bitmap = 0;

		uint32_t num_in_params = 0;

		for(size_t i = 0; const sema::Expr& arg : args){
			EVO_DEFER([&](){ i += 1; });

			if(target_func_type.params[i].kind != AST::FuncDecl::Param::Kind::IN){ continue; }

			if(arg.kind() == sema::Expr::Kind::COPY){
				output_in_param_bitmap |= 1 << num_in_params;

			}else if(arg.kind() == sema::Expr::Kind::FORWARD){
				const sema::Forward& arg_forward = this->context.getSemaBuffer().getForward(arg.forwardID());
				const sema::Param& target_param = this->context.getSemaBuffer().getParam(arg_forward.expr.paramID());
				const uint32_t in_param_index = *this->current_func_info->params[target_param.index].in_param_index;
				const bool in_param_is_copy = bool((this->in_param_bitmap >> in_param_index) & 1);

				if(in_param_is_copy){
					output_in_param_bitmap |= 1 << num_in_params;
				}
			}

			num_in_params += 1;
		}

		return output_in_param_bitmap;
	}





	auto SemaToPIR::create_call(
		evo::Variant<pir::Function::ID, pir::ExternalFunction::ID> func_id,
		evo::SmallVector<pir::Expr>&& args,
		std::string&& name
	) -> pir::Expr {
		if(func_id.is<pir::Function::ID>()){
			return this->agent.createCall(func_id.as<pir::Function::ID>(), std::move(args), std::move(name));
		}else{
			return this->agent.createCall(func_id.as<pir::ExternalFunction::ID>(), std::move(args), std::move(name));
		}
	}

	auto SemaToPIR::create_call_void(
		evo::Variant<pir::Function::ID, pir::ExternalFunction::ID> func_id, evo::SmallVector<pir::Expr>&& args
	) -> void {
		if(func_id.is<pir::Function::ID>()){
			this->agent.createCallVoid(func_id.as<pir::Function::ID>(), std::move(args));
		}else{
			this->agent.createCallVoid(func_id.as<pir::ExternalFunction::ID>(), std::move(args));
		}
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
			case TemplateIntrinsicFunc::Kind::GET_TYPE_ID: {
				evo::debugFatalBreak("@getTypeID is constexpr evaluated");
			} break;

			case TemplateIntrinsicFunc::Kind::NUM_BYTES: {
				evo::debugFatalBreak("@numBytes is constexpr evaluated");
			} break;

			case TemplateIntrinsicFunc::Kind::NUM_BITS: {
				evo::debugFatalBreak("@numBits is constexpr evaluated");
			} break;

			case TemplateIntrinsicFunc::Kind::BIT_CAST: {
				const pir::Type from_type =
					this->get_type<false>(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type =
					this->get_type<false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value =
					this->agent.createBitCast(from_value, to_type, this->name("BIT_CAST"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;


			case TemplateIntrinsicFunc::Kind::TRUNC: {
				const pir::Type from_type =
					this->get_type<false>(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type =
					this->get_type<false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createTrunc(from_value, to_type, this->name("TRUNC"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FTRUNC: {
				const pir::Type from_type =
					this->get_type<false>(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type =
					this->get_type<false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createFTrunc(from_value, to_type, this->name("FTRUNC"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SEXT: {
				const pir::Type from_type =
					this->get_type<false>(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type =
					this->get_type<false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createSExt(from_value, to_type, this->name("SEXT"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::ZEXT: {
				const pir::Type from_type =
					this->get_type<false>(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type =
					this->get_type<false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createZExt(from_value, to_type, this->name("ZEXT"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FEXT: {
				const pir::Type from_type =
					this->get_type<false>(instantiation.templateArgs[0].as<TypeInfo::VoidableID>());
				const pir::Type to_type =
					this->get_type<false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>());

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->agent.createFExt(from_value, to_type, this->name("FEXT"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::I_TO_F: {
				const pir::Type to_type = this->get_type<false>(
					instantiation.templateArgs[1].as<TypeInfo::VoidableID>().asTypeID()
				);
				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = [&](){
					const TypeInfo::ID from_type_id = 
						instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();

					if(this->context.type_manager.isUnsignedIntegral(from_type_id)){
						return this->agent.createUIToF(from_value, to_type, this->name("UI_TO_F"));
					}else{
						return this->agent.createIToF(from_value, to_type, this->name("I_TO_F"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::F_TO_I: {
				const TypeInfo::VoidableID to_type_id =
					instantiation.templateArgs[1].as<TypeInfo::VoidableID>().asTypeID();
				const pir::Type to_type = this->get_type<false>(to_type_id);
				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = [&](){
					if(this->context.getTypeManager().isUnsignedIntegral(to_type_id)){
						return this->agent.createFToUI(from_value, to_type, this->name("F_TO_UI"));
					}else{
						return this->agent.createFToI(from_value, to_type, this->name("F_TO_I"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(to_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::ADD: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const bool may_wrap = instantiation.templateArgs[1].as<core::GenericValue>().getBool();

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->agent.createAdd(
					lhs, rhs, !is_unsigned & !may_wrap, is_unsigned & !may_wrap, this->name("ADD")
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false>(arg_type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::ADD_WRAP: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr result = [&](){
					if(is_unsigned){
						return this->agent.createUAddWrap(
							lhs, rhs, this->name("UADD_WRAP.VALUE"), this->name("UADD_WRAP.WRAPPED")
						);
					}else{
						return this->agent.createSAddWrap(
							lhs, rhs, this->name("SADD_WRAP.VALUE"), this->name("SADD_WRAP.WRAPPED")
						);
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					evo::debugFatalBreak("@addWrap returns multiple values");

				}else if constexpr(MODE == GetExprMode::POINTER){
					evo::debugFatalBreak("@addWrap returns multiple values");

				}else if constexpr(MODE == GetExprMode::STORE){
					if(is_unsigned){
						this->agent.createStore(store_locations[0], this->agent.extractUAddWrapResult(result));
						this->agent.createStore(store_locations[1], this->agent.extractUAddWrapWrapped(result));
					}else{
						this->agent.createStore(store_locations[0], this->agent.extractSAddWrapResult(result));
						this->agent.createStore(store_locations[1], this->agent.extractSAddWrapWrapped(result));
					}

					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::ADD_SAT: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = [&](){
					if(is_unsigned){
						return this->agent.createUAddSat(lhs, rhs, this->name("UADD_SAT"));
					}else{
						return this->agent.createSAddSat(lhs, rhs, this->name("SADD_SAT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false>(arg_type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FADD: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->agent.createFAdd(lhs, rhs, this->name("FADD"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SUB: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const bool may_wrap = instantiation.templateArgs[1].as<core::GenericValue>().getBool();

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->agent.createSub(
					lhs, rhs, !is_unsigned & !may_wrap, is_unsigned & !may_wrap, this->name("SUB")
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false>(arg_type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SUB_WRAP: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr result = [&](){
					if(is_unsigned){
						return this->agent.createUSubWrap(
							lhs, rhs, this->name("USUB_WRAP.VALUE"), this->name("USUB_WRAP.WRAPPED")
						);
					}else{
						return this->agent.createSSubWrap(
							lhs, rhs, this->name("SSUB_WRAP.VALUE"), this->name("SSUB_WRAP.WRAPPED")
						);
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					evo::debugFatalBreak("@addWrap returns multiple values");

				}else if constexpr(MODE == GetExprMode::POINTER){
					evo::debugFatalBreak("@addWrap returns multiple values");

				}else if constexpr(MODE == GetExprMode::STORE){
					if(is_unsigned){
						this->agent.createStore(store_locations[0], this->agent.extractUSubWrapResult(result));
						this->agent.createStore(store_locations[1], this->agent.extractUSubWrapWrapped(result));
					}else{
						this->agent.createStore(store_locations[0], this->agent.extractSSubWrapResult(result));
						this->agent.createStore(store_locations[1], this->agent.extractSSubWrapWrapped(result));
					}

					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SUB_SAT: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = [&](){
					if(is_unsigned){
						return this->agent.createUSubSat(lhs, rhs, this->name("USUB_SAT"));
					}else{
						return this->agent.createSSubSat(lhs, rhs, this->name("SSUB_SAT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false>(arg_type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FSUB: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->agent.createFSub(lhs, rhs, this->name("FSUB"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::MUL: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const bool may_wrap = instantiation.templateArgs[1].as<core::GenericValue>().getBool();

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->agent.createMul(
					lhs, rhs, !is_unsigned & !may_wrap, is_unsigned & !may_wrap, this->name("MUL")
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false>(arg_type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::MUL_WRAP: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr result = [&](){
					if(is_unsigned){
						return this->agent.createUMulWrap(
							lhs, rhs, this->name("UMUL_WRAP.VALUE"), this->name("UMUL_WRAP.WRAPPED")
						);
					}else{
						return this->agent.createSMulWrap(
							lhs, rhs, this->name("SMUL_WRAP.VALUE"), this->name("SMUL_WRAP.WRAPPED")
						);
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					evo::debugFatalBreak("@addWrap returns multiple values");

				}else if constexpr(MODE == GetExprMode::POINTER){
					evo::debugFatalBreak("@addWrap returns multiple values");

				}else if constexpr(MODE == GetExprMode::STORE){
					if(is_unsigned){
						this->agent.createStore(store_locations[0], this->agent.extractUMulWrapResult(result));
						this->agent.createStore(store_locations[1], this->agent.extractUMulWrapWrapped(result));
					}else{
						this->agent.createStore(store_locations[0], this->agent.extractSMulWrapResult(result));
						this->agent.createStore(store_locations[1], this->agent.extractSMulWrapWrapped(result));
					}

					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::MUL_SAT: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = [&](){
					if(is_unsigned){
						return this->agent.createUMulSat(lhs, rhs, this->name("UMUL_SAT"));
					}else{
						return this->agent.createSMulSat(lhs, rhs, this->name("SMUL_SAT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false>(arg_type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FMUL: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->agent.createFMul(lhs, rhs, this->name("FMUL"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::DIV: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const bool is_exact = instantiation.templateArgs[1].as<core::GenericValue>().getBool();

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = [&](){
					if(is_unsigned){
						return this->agent.createUDiv(lhs, rhs, is_exact, this->name("UDIV"));
					}else{
						return this->agent.createSDiv(lhs, rhs, is_exact, this->name("SDIV"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false>(arg_type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FDIV: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->agent.createFDiv(lhs, rhs, this->name("FDIV"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::REM: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_float = this->context.type_manager.isFloatingPoint(arg_type_id);
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = [&](){
					if(is_float){
						return this->agent.createFRem(lhs, rhs, this->name("FREM"));
					}else if(is_unsigned){
						return this->agent.createURem(lhs, rhs, this->name("UREM"));
					}else{
						return this->agent.createSRem(lhs, rhs, this->name("SREM"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FNEG: {
				const pir::Expr rhs = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->agent.createFNeg(rhs, this->name("FNEG"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::EQ: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_float = this->context.type_manager.isFloatingPoint(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = [&](){
					if(is_float){
						return this->agent.createFEq(lhs, rhs, this->name("FEQ"));
					}else{
						return this->agent.createIEq(lhs, rhs, this->name("IEQ"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::NEQ: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_float = this->context.type_manager.isFloatingPoint(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = [&](){
					if(is_float){
						return this->agent.createFNeq(lhs, rhs, this->name("FNEQ"));
					}else{
						return this->agent.createINeq(lhs, rhs, this->name("INEQ"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::LT: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_float = this->context.type_manager.isFloatingPoint(arg_type_id);
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = [&](){
					if(is_float){
						return this->agent.createFLT(lhs, rhs, this->name("FLT"));
					}else if(is_unsigned){
						return this->agent.createULT(lhs, rhs, this->name("ULT"));
					}else{
						return this->agent.createSLT(lhs, rhs, this->name("SLT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::LTE: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_float = this->context.type_manager.isFloatingPoint(arg_type_id);
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = [&](){
					if(is_float){
						return this->agent.createFLTE(lhs, rhs, this->name("FLTE"));
					}else if(is_unsigned){
						return this->agent.createULTE(lhs, rhs, this->name("ULTE"));
					}else{
						return this->agent.createSLTE(lhs, rhs, this->name("SLTE"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::GT: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_float = this->context.type_manager.isFloatingPoint(arg_type_id);
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = [&](){
					if(is_float){
						return this->agent.createFGT(lhs, rhs, this->name("FGT"));
					}else if(is_unsigned){
						return this->agent.createUGT(lhs, rhs, this->name("UGT"));
					}else{
						return this->agent.createSGT(lhs, rhs, this->name("SGT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::GTE: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const bool is_float = this->context.type_manager.isFloatingPoint(arg_type_id);
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = [&](){
					if(is_float){
						return this->agent.createFGTE(lhs, rhs, this->name("FGTE"));
					}else if(is_unsigned){
						return this->agent.createUGTE(lhs, rhs, this->name("UGTE"));
					}else{
						return this->agent.createSGTE(lhs, rhs, this->name("SGTE"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;


			case TemplateIntrinsicFunc::Kind::AND: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->agent.createAnd(lhs, rhs, this->name("AND"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::OR: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->agent.createOr(lhs, rhs, this->name("OR"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::XOR: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->agent.createXor(lhs, rhs, this->name("XOR"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SHL: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const pir::Type arg_pir_type = this->get_type<false>(arg_type_id);
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const bool is_exact = instantiation.templateArgs[2].as<core::GenericValue>().getBool();

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->agent.createZExt(
					this->get_expr_register(func_call.args[1]), arg_pir_type, this->name("SHL.AMMOUNT_ZEXT")
				);

				const pir::Expr register_value = this->agent.createSHL(
					lhs, rhs, !is_unsigned & !is_exact, is_unsigned & !is_exact, this->name("SHL")
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SHL_SAT: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const pir::Type arg_pir_type = this->get_type<false>(arg_type_id);
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				

				const pir::Expr register_value = [&](){
					if(is_unsigned){
						const pir::Expr rhs = this->agent.createZExt(
							this->get_expr_register(func_call.args[1]),
							arg_pir_type,
							this->name("USHL_SAT.AMMOUNT_ZEXT")
						);
						return this->agent.createUSHLSat(lhs, rhs, this->name("USHL_SAT"));
					}else{
						const pir::Expr rhs = this->agent.createZExt(
							this->get_expr_register(func_call.args[1]),
							arg_pir_type,
							this->name("SSHL_SAT.AMMOUNT_ZEXT")
						);
						return this->agent.createSSHLSat(lhs, rhs, this->name("SSHL_SAT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SHR: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const pir::Type arg_pir_type = this->get_type<false>(arg_type_id);
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);
				const bool is_exact = instantiation.templateArgs[2].as<core::GenericValue>().getBool();

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = [&](){
					if(is_unsigned){
						const pir::Expr rhs = this->agent.createZExt(
							this->get_expr_register(func_call.args[1]), arg_pir_type, this->name("USHR.AMMOUNT_ZEXT")
						);
						return this->agent.createUSHR(lhs, rhs, is_exact, this->name("USHR"));
					}else{
						const pir::Expr rhs = this->agent.createZExt(
							this->get_expr_register(func_call.args[1]), arg_pir_type, this->name("SSHR.AMMOUNT_ZEXT")
						);
						return this->agent.createSSHR(lhs, rhs, is_exact, this->name("SSHR"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::BIT_REVERSE: {
				const pir::Expr rhs = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->agent.createBitReverse(rhs, this->name("BIT_REVERSE"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::BSWAP: {
				const pir::Expr rhs = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->agent.createBSwap(rhs, this->name("BSWAP"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::CTPOP: {
				const pir::Expr rhs = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->agent.createCtPop(rhs, this->name("CTPOP"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::CTLZ: {
				const pir::Expr rhs = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->agent.createCTLZ(rhs, this->name("CTLZ"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::CTTZ: {
				const pir::Expr rhs = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->agent.createCTTZ(rhs, this->name("CTTZ"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false>(type_id);
					const pir::Expr pointer_alloca = this->agent.createAlloca(arg_pir_type);
					this->agent.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->agent.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
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

			case sema::Expr::Kind::NULL_VALUE: {
				return this->agent.createNullptr();
			} break;

			case sema::Expr::Kind::INT_VALUE: {
				const sema::IntValue& int_value = this->context.getSemaBuffer().getIntValue(expr.intValueID());
				return this->agent.createNumber(this->get_type<false>(*int_value.typeID), int_value.value);
			} break;

			case sema::Expr::Kind::FLOAT_VALUE: {
				const sema::FloatValue& float_value = this->context.getSemaBuffer().getFloatValue(expr.floatValueID());
				return this->agent.createNumber(this->get_type<false>(*float_value.typeID), float_value.value);
			} break;

			case sema::Expr::Kind::BOOL_VALUE: {
				const sema::BoolValue& bool_value = this->context.getSemaBuffer().getBoolValue(expr.boolValueID());
				return this->agent.createBoolean(bool_value.value);
			} break;

			case sema::Expr::Kind::STRING_VALUE: {
				const sema::StringValue& string_value =
					this->context.getSemaBuffer().getStringValue(expr.stringValueID());

				const pir::GlobalVar::String::ID string_value_id = 
					this->module.createGlobalString(evo::copy(string_value.value));

				const pir::GlobalVar::ID string_id = this->module.createGlobalVar(
					std::format("PTHR.str{}", this->data.get_string_literal_id()),
					this->module.getGlobalString(string_value_id).type,
					pir::Linkage::PRIVATE,
					string_value_id,
					true
				);

				return this->agent.createGlobalValue(string_id);
			} break;

			case sema::Expr::Kind::AGGREGATE_VALUE: {
				const sema::AggregateValue& aggregate_value = 
					this->context.getSemaBuffer().getAggregateValue(expr.aggregateValueID());

				auto values = evo::SmallVector<pir::GlobalVar::Value>();
				values.reserve(aggregate_value.values.size());
				for(const sema::Expr value : aggregate_value.values){
					values.emplace_back(this->get_global_var_value(value));
				}

				const pir::Type aggregate_type = this->get_type<false>(aggregate_value.typeID);

				if(aggregate_type.kind() == pir::Type::Kind::STRUCT){
					// for empty structs
					if(values.empty()){
						values.emplace_back(
							this->agent.createNumber(this->module.createIntegerType(1), core::GenericInt(1, 0))
						);
					}

					return this->module.createGlobalStruct(aggregate_type, std::move(values));

				}else{
					evo::debugAssert(aggregate_type.kind() == pir::Type::Kind::ARRAY, "Unknown aggregate type");

					const pir::Type elem_type = this->module.getArrayType(aggregate_type).elemType;
					return this->module.createGlobalArray(elem_type, std::move(values));
				}

			} break;

			case sema::Expr::Kind::CHAR_VALUE: {
				const sema::CharValue& char_value = this->context.getSemaBuffer().getCharValue(expr.charValueID());
				return this->agent.createNumber(
					this->module.createIntegerType(8), core::GenericInt(8, uint64_t(char_value.value))
				);
			} break;

			case sema::Expr::Kind::MODULE_IDENT:        case sema::Expr::Kind::INTRINSIC_FUNC:
			case sema::Expr::Kind::TEMPLATED_INTRINSIC_FUNC_INSTANTIATION:
			case sema::Expr::Kind::COPY:                case sema::Expr::Kind::MOVE:
			case sema::Expr::Kind::FORWARD:             case sema::Expr::Kind::FUNC_CALL:
			case sema::Expr::Kind::ADDR_OF:             case sema::Expr::Kind::IMPLICIT_CONVERSION_TO_OPTIONAL:
			case sema::Expr::Kind::OPTIONAL_NULL_CHECK: case sema::Expr::Kind::DEREF:
			case sema::Expr::Kind::UNWRAP:              case sema::Expr::Kind::ACCESSOR:
			case sema::Expr::Kind::PTR_ACCESSOR:        case sema::Expr::Kind::UNION_ACCESSOR:
			case sema::Expr::Kind::PTR_UNION_ACCESSOR:  case sema::Expr::Kind::TRY_ELSE:
			case sema::Expr::Kind::BLOCK_EXPR:          case sema::Expr::Kind::FAKE_TERM_INFO:
			case sema::Expr::Kind::MAKE_INTERFACE_PTR:  case sema::Expr::Kind::INTERFACE_CALL:
			case sema::Expr::Kind::INDEXER:             case sema::Expr::Kind::PTR_INDEXER:
			case sema::Expr::Kind::PARAM:               case sema::Expr::Kind::RETURN_PARAM:
			case sema::Expr::Kind::ERROR_RETURN_PARAM:  case sema::Expr::Kind::BLOCK_EXPR_OUTPUT:
			case sema::Expr::Kind::EXCEPT_PARAM:        case sema::Expr::Kind::VAR:
			case sema::Expr::Kind::GLOBAL_VAR:          case sema::Expr::Kind::FUNC: {
				evo::debugFatalBreak("Not valid global var value");
			} break;
		}

		evo::unreachable();
	}



	//////////////////////////////////////////////////////////////////////
	// get type


	template<bool MAY_LOWER_DEPENDENCY>
	auto SemaToPIR::get_type(const TypeInfo::VoidableID voidable_type_id) -> pir::Type {
		if(voidable_type_id.isVoid()){ return this->module.createVoidType(); }
		return this->get_type<MAY_LOWER_DEPENDENCY>(voidable_type_id.asTypeID());
	}


	template<bool MAY_LOWER_DEPENDENCY>
	auto SemaToPIR::get_type(const TypeInfo::ID type_id) -> pir::Type {
		const TypeInfo& type_info = this->context.getTypeManager().getTypeInfo(type_id);

		if(type_info.isInterfacePointer()){ return this->data.getInterfacePtrType(this->module); }
		if(type_info.isPointer()){ return this->module.createPtrType(); }

		if(type_info.isOptionalNotPointer()){
			const auto lock = std::scoped_lock(this->data.optional_types_lock);
			const auto find = this->data.optional_types.find(&type_info);
			if(find != this->data.optional_types.end()){ return find->second; }


			auto target_qualifiers = evo::SmallVector<AST::Type::Qualifier>();
			target_qualifiers.reserve(type_info.qualifiers().size() - 1);
			for(size_t i = 0; i < type_info.qualifiers().size() - 1; i+=1){
				target_qualifiers.emplace_back(type_info.qualifiers()[i]);
			}

			const TypeInfo::ID target_type_id = this->context.type_manager.getOrCreateTypeInfo(
				TypeInfo(type_info.baseTypeID(), std::move(target_qualifiers))
			);

			const pir::Type created_struct = this->module.createStructType(
				std::format("PTHR.optional_{}", type_id.get()),
				evo::SmallVector<pir::Type>{
					this->get_type<MAY_LOWER_DEPENDENCY>(target_type_id), this->module.createBoolType()
				},
				true
			);

			this->data.optional_types.emplace(&type_info, created_struct);

			return created_struct;
		}

		return this->get_type<MAY_LOWER_DEPENDENCY>(type_info.baseTypeID());
	}



	template<bool MAY_LOWER_DEPENDENCY>
	auto SemaToPIR::get_type(const BaseType::ID base_type_id) -> pir::Type {
		switch(base_type_id.kind()){
			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Not a valid base type");
			
			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = 
					this->context.getTypeManager().getPrimitive(base_type_id.primitiveID());

				switch(primitive.kind()){
					case Token::Kind::TYPE_INT:         case Token::Kind::TYPE_ISIZE:    case Token::Kind::TYPE_UINT:
					case Token::Kind::TYPE_USIZE:       case Token::Kind::TYPE_TYPEID:   case Token::Kind::TYPE_C_WCHAR:
					case Token::Kind::TYPE_C_SHORT:     case Token::Kind::TYPE_C_USHORT: case Token::Kind::TYPE_C_INT:
					case Token::Kind::TYPE_C_UINT:      case Token::Kind::TYPE_C_LONG:   case Token::Kind::TYPE_C_ULONG:
					case Token::Kind::TYPE_C_LONG_LONG: case Token::Kind::TYPE_C_ULONG_LONG:
						return this->module.createIntegerType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
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
						return this->module.createFloatType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
						);

					default: evo::debugFatalBreak("Unknown builtin type");
				}
			} break;
			
			case BaseType::Kind::FUNCTION: {
				evo::unimplemented("BaseType::Kind::FUNCTION");
			} break;
			
			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->context.getTypeManager().getArray(base_type_id.arrayID());
				const pir::Type elem_type = this->get_type<false>(array.elementTypeID);


				pir::Type array_type = this->module.createArrayType(elem_type, array.lengths[0]);

				for(size_t i = 1; i < array.lengths.size(); i+=1){
					array_type = this->module.createArrayType(array_type, array.lengths[i]);
				}

				return array_type;
			} break;
			
			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->context.getTypeManager().getAlias(base_type_id.aliasID());
				return this->get_type<false>(*alias.aliasedType.load());
			} break;
			
			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& distinct_alias_type = 
					this->context.getTypeManager().getDistinctAlias(base_type_id.distinctAliasID());
				return this->get_type<false>(*distinct_alias_type.underlyingType.load());
			} break;
			
			case BaseType::Kind::STRUCT: {
				if constexpr(MAY_LOWER_DEPENDENCY){
					if(this->data.has_struct(base_type_id.structID()) == false){
						this->lowerStructAndDependencies(base_type_id.structID());
					}
				}

				return this->data.get_struct(base_type_id.structID());
			} break;


			case BaseType::Kind::UNION: {
				if constexpr(MAY_LOWER_DEPENDENCY){
					if(this->data.has_union(base_type_id.unionID()) == false){
						this->lowerUnionAndDependencies(base_type_id.unionID());
					}
				}

				return this->data.get_union(base_type_id.unionID());
			} break;
			
			case BaseType::Kind::STRUCT_TEMPLATE: {
				evo::debugFatalBreak("Cannot get type of struct template");
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				evo::debugFatalBreak("Cannot get type of type deducer");
			} break;

			case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Cannot get type of interface");
			} break;
		}

		return this->module.createIntegerType(12);
	}



	//////////////////////////////////////////////////////////////////////
	// name mangling

	auto SemaToPIR::mangle_name(const BaseType::Struct::ID struct_id) const -> std::string {
		if(this->data.getConfig().useReadableNames){
			const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(struct_id);

			if(struct_type.isClangType()){
				return std::format("struct.{}", struct_type.getName(this->context.getSourceManager()));

			}else{
				return std::format(
					"PTHR.s{}.{}", struct_id.get(), struct_type.getName(this->context.getSourceManager())
				);
			}

		}else{
			return std::format("PTHR.s{}", struct_id.get());
		}
	}

	auto SemaToPIR::mangle_name(const sema::GlobalVar::ID global_var_id) const -> std::string {
		const sema::GlobalVar& global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);
		const Source& source = this->context.getSourceManager()[global_var.sourceID];

		if(this->data.getConfig().useReadableNames){
			return std::format(
				"PTHR.g{}.{}", global_var_id.get(), source.getTokenBuffer()[global_var.ident].getString()
			);
			
		}else{
			return std::format("PTHR.g{}", global_var_id.get());
		}
	}


	auto SemaToPIR::mangle_name(const sema::Func::ID func_id) const -> std::string {
		const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);


		if(func.isExport || func.isClangFunc()){
			return std::string(func.getName(this->context.getSourceManager()));
			
		}else{
			const Source& source = this->context.getSourceManager()[func.sourceID.as<Source::ID>()];
			const Token& name_token = source.getTokenBuffer()[func.name.as<Token::ID>()];
			if(name_token.kind() == Token::Kind::IDENT){
				if(this->data.getConfig().useReadableNames){
					return std::format("PTHR.f{}.{}", func_id.get(), name_token.getString());
				}else{
					return std::format("PTHR.f{}", func_id.get());
				}

			}else{
				// TODO(FUTURE): better naming of overloads
				return std::format("PTHR.f{}.OP.{}", func_id.get(), Token::printKind(name_token.kind()));
			}
		}
	}


	auto SemaToPIR::mangle_name(const BaseType::Union::ID union_id) const -> std::string {
		const BaseType::Union& union_type = this->context.getTypeManager().getUnion(union_id);


		if(union_type.isClangType()){
			return std::format("union.{}", union_type.getName(this->context.getSourceManager()));
			
		}else if(this->data.getConfig().useReadableNames){
			return std::format("PTHR.u{}.{}", union_id.get(), union_type.getName(this->context.getSourceManager()));
			
		}else{
			return std::format("PTHR.u{}", union_id.get());
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
			return std::format(fmt, std::forward<decltype(args)>(args)...);
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



	template<bool INCLUDE_ERRORS>
	auto SemaToPIR::output_defers_for_scope_level(const ScopeLevel& scope_level) -> void {
		for(const DeferItem& defer_item : scope_level.defers | std::views::reverse){
			if constexpr(INCLUDE_ERRORS == false){
				if(defer_item.error_only){
					continue;
				}		
			}

			const sema::Defer& sema_defer = this->context.getSemaBuffer().getDefer(defer_item.defer_id);

			for(const sema::Stmt& stmt : sema_defer.block){
				this->lower_stmt(stmt);
			}
		}
	}



}