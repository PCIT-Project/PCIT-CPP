////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SemaToPIR.hpp"

#include <ranges>

#include "../../include/Context.hpp"
#include "../../include/sema/conversion.hpp"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{

	[[nodiscard]] static constexpr auto ceil_to_multiple(size_t num, size_t multiple) -> size_t {
		return (num + (multiple - 1)) & ~(multiple - 1);
	}


	[[nodiscard]] static auto remove_alloca_from_name(std::string_view str) -> std::string {
		const size_t alloca_loc = str.find(".ALLOCA");

		if(alloca_loc == std::string_view::npos){ return std::string(str); }

		auto output = std::string(str.substr(0, alloca_loc));
		output += str.substr(alloca_loc + evo::stringSize(".ALLOCA"));
		return output;
	}



	auto SemaToPIR::lowerRuntime() -> void {
		for(uint32_t i = 0; i < this->context.getTypeManager().getNumStructs(); i+=1){
			const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(BaseType::Struct::ID(i));
			if(struct_type.isClangType() == false){ continue; }

			this->lowerStructAndDepsIfNeeded(BaseType::Struct::ID(i));
		}

		for(uint32_t i = 0; i < this->context.getTypeManager().getNumUnions(); i+=1){
			const BaseType::Union& union_type = this->context.getTypeManager().getUnion(BaseType::Union::ID(i));
			if(union_type.isClangType() == false){ continue; }

			this->lowerUnionAndDepsIfNeeded(BaseType::Union::ID(i));
		}

		for(const sema::GlobalVar::ID& global_var_id : this->context.getSemaBuffer().getGlobalVars()){
			const sema::GlobalVar& global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);
			if(global_var.isClangVar() == false && global_var.kind != AST::VarDef::Kind::VAR){ continue; }

			this->lowerGlobalDecl(global_var_id);

			if(global_var.expr.load().has_value()){
				this->lowerGlobalDef(global_var_id);
			}
		}


		for(const sema::Func::ID& func_id : this->context.getSemaBuffer().getFuncs()){
			const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);
			if(func.status == sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT){ continue; }
			if(func.status == sema::Func::Status::SUSPENDED){ continue; }
			if(func.attributes.isComptime){ continue; }

			this->lowerFuncDecl(func_id);
		}

		for(uint32_t i = 0; i < this->context.getTypeManager().getNumInterfaces(); i+=1){
			const auto interface_id = BaseType::Interface::ID(i);
			const BaseType::Interface& interface = this->context.getTypeManager().getInterface(interface_id);
			
			if(interface.isPolymorphic == false){ continue; }

			// this->lowerInterface(interface_id);

			for(const auto& [target_type_id, impl] : interface.impls){
				this->lowerInterfaceVTableDef(interface_id, target_type_id, impl.methods);
			}
		}
		
		for(const sema::Func::ID& func_id : this->context.getSemaBuffer().getFuncs()){
			const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);
			if(func.status == sema::Func::Status::INTERFACE_METHOD_NO_DEFAULT){ continue; }
			if(func.status == sema::Func::Status::SUSPENDED){ continue; }
			if(func.isClangFunc()){ continue; }

			if(func.attributes.isRuntime == false){
				// TODO(FUTURE): delete function
				continue;
			}

			if(func.attributes.isRTDiff){
				const Data::FuncInfo& func_info = this->data.get_func(func_id);

				using PIRFuncID = evo::Variant<std::monostate, pir::Function::ID, pir::ExternalFunction::ID>;
				for(const PIRFuncID pir_id : func_info.pir_ids){
					if(pir_id.is<pir::Function::ID>()){
						this->module.deleteBodyOfFunction(pir_id.as<pir::Function::ID>());

						this->handler.setTargetFunction(pir_id.as<pir::Function::ID>());
						this->handler.createBasicBlock(this->name("begin"));
						this->handler.removeTargetFunction();
					}
				}

			}else if(func.attributes.isComptime){
				continue;
			}

			this->lowerFuncDef(func_id);
		}
	}



	
	auto SemaToPIR::lowerStruct(BaseType::Struct::ID struct_id) -> pir::Type {
		return this->lower_struct<false>(struct_id);
	}

	auto SemaToPIR::lowerStructAndDepsIfNeeded(BaseType::Struct::ID struct_id) -> pir::Type {
		return this->lower_struct<true>(struct_id);
	}


	auto SemaToPIR::lowerUnion(BaseType::Union::ID union_id) -> pir::Type {
		return this->lower_union<false>(union_id);
	}

	auto SemaToPIR::lowerUnionAndDepsIfNeeded(BaseType::Union::ID union_id) -> pir::Type {
		return this->lower_union<true>(union_id);
	}


	auto SemaToPIR::lowerEnum(BaseType::Enum::ID enum_id) -> void {
		this->lower_enum<false>(enum_id);
	}

	auto SemaToPIR::lowerEnumAndDepsIfNeeded(BaseType::Enum::ID enum_id) -> void {
		this->lower_enum<true>(enum_id);
	}





	auto SemaToPIR::lowerGlobalDecl(sema::GlobalVar::ID global_var_id) -> std::optional<pir::GlobalVar::ID> {
		const sema::GlobalVar& sema_global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);

		if(sema_global_var.kind == AST::VarDef::Kind::DEF){ return std::nullopt; }

		const PIRType pir_type = this->get_type<false, false>(*sema_global_var.typeID);

		const pir::GlobalVar::ID new_global_var = this->module.createGlobalVar(
			this->mangle_name(global_var_id),
			pir_type.type,
			pir::Linkage::INTERNAL,
			pir::GlobalVar::NoValue{},
			sema_global_var.kind == AST::VarDef::Kind::CONST
		);

		this->data.create_global_var(global_var_id, new_global_var);

		return new_global_var;
	}


	auto SemaToPIR::lowerGlobalDef(sema::GlobalVar::ID global_var_id) -> void {
		const sema::GlobalVar& sema_global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);

		if(sema_global_var.kind == AST::VarDef::Kind::DEF){ return; }

		const pir::GlobalVar::ID pir_var_id = this->data.get_global_var(global_var_id);
		this->module.getGlobalVar(pir_var_id).value = this->get_global_var_value(*sema_global_var.expr.load());
	}



	auto SemaToPIR::lowerFuncDeclComptime(sema::Func::ID func_id) -> pir::Function::ID {
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
		params.reserve(func_type.params.size() + func_type.returnTypes.size() + size_t(!func_type.errorTypes.empty()));

		uint32_t in_param_index = 0;
		auto param_infos = evo::SmallVector<Data::FuncInfo::Param>();
		param_infos.reserve(func_type.params.size());

		auto meta_params = evo::SmallVector<pir::meta::Type>();
		if(this->data.config.includeDebugInfo){
			meta_params.reserve(
				func_type.params.size() + func_type.returnTypes.size() + size_t(!func_type.errorTypes.empty())
			);
		}


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
							return std::format("PARAM.{}.{}", i, token.getString());
						}
					}
				}
			}();

			auto attributes = evo::SmallVector<pir::Parameter::Attribute>();

			const pir::Type param_type = [&]() -> pir::Type {
				if(this->data.config.includeDebugInfo){
					const PIRType param_pir_type = this->get_type<false, true>(param.typeID);
					meta_params.emplace_back(*param_pir_type.meta_type_id);
					return param_pir_type.type;
				}else{
					return this->get_type<false, false>(param.typeID).type;
				}
			}();

			if(param.shouldCopy){
				params.emplace_back(std::move(param_name), param_type, std::move(attributes));

				if(param.kind == BaseType::Function::Param::Kind::IN){
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

				if(param.kind == BaseType::Function::Param::Kind::READ){
					attributes.emplace_back(pir::Parameter::Attribute::PtrReadOnly());
				}else{
					attributes.emplace_back(pir::Parameter::Attribute::PtrWritable());
					attributes.emplace_back(pir::Parameter::Attribute::PtrNoAlias());
				}

				params.emplace_back(std::move(param_name), this->module.createPtrType(), std::move(attributes));

				if(param.kind == BaseType::Function::Param::Kind::IN){
					param_infos.emplace_back(param_type, in_param_index);
					in_param_index += 1;
				}else{
					param_infos.emplace_back(param_type, std::nullopt);
				}
			}
		}

		bool is_implicit_rvo = false;
		auto return_params = evo::SmallVector<SemaToPIRData::FuncInfo::ReturnParam>();
		if(func_type.hasNamedReturns){
			for(const TypeInfo::VoidableID return_type_id : func_type.returnTypes){
				const pir::Type reference_type = [&]() -> pir::Type {
					if(this->data.config.includeDebugInfo){
						const PIRType reference_pir_type = this->get_type<false, true>(return_type_id.asTypeID());
						meta_params.emplace_back(*reference_pir_type.meta_type_id);
						return reference_pir_type.type;
					}else{
						return this->get_type<false, false>(return_type_id.asTypeID()).type;
					}
				}();


				return_params.emplace_back(this->handler.createParamExpr(uint32_t(params.size())), reference_type);

				auto attributes = evo::SmallVector<pir::Parameter::Attribute>{
					pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNoAlias()),
					pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNonNull()),
					pir::Parameter::Attribute(pir::Parameter::Attribute::PtrDereferencable(
						this->context.getTypeManager().numBytes(return_type_id.asTypeID())
					)),
					pir::Parameter::Attribute(pir::Parameter::Attribute::PtrWritable()),
				};

				if(func_type.returnTypes.size() == 1 && func_type.hasErrorReturn() == false){
					attributes.emplace_back(pir::Parameter::Attribute::PtrRVO(reference_type));
				}

				if(this->data.getConfig().useReadableNames){
					const Token::ID param_ident = func.returnParamIdents[return_params.size() - 1];

					params.emplace_back(
						std::format("RET.{}", this->current_source->getTokenBuffer()[param_ident].getString()),
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
			const pir::Type ret_type = [&]() -> pir::Type {
				if(this->data.config.includeDebugInfo){
					const PIRType ret_pir_type = this->get_type<false, true>(func_type.returnTypes[0].asTypeID());
					meta_params.emplace_back(*ret_pir_type.meta_type_id);
					return ret_pir_type.type;
				}else{
					return this->get_type<false, false>(func_type.returnTypes[0].asTypeID()).type;
				}
			}();


			return_params.emplace_back(this->handler.createParamExpr(uint32_t(params.size())), ret_type);

			auto attributes = evo::SmallVector<pir::Parameter::Attribute>{
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNoAlias()),
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNonNull()),
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrDereferencable(
					this->context.getTypeManager().numBytes(func_type.returnTypes[0].asTypeID())
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

		}else if(func.isClangFunc() == false && func_type.isImplicitRVO(this->context.getTypeManager())){
			is_implicit_rvo = true;

			const pir::Type ret_type = [&]() -> pir::Type {
				if(this->data.config.includeDebugInfo){
					const PIRType ret_pir_type = this->get_type<false, true>(func_type.returnTypes[0].asTypeID());
					meta_params.emplace_back(*ret_pir_type.meta_type_id);
					return ret_pir_type.type;
				}else{
					return this->get_type<false, false>(func_type.returnTypes[0].asTypeID()).type;
				}
			}();

			return_params.emplace_back(this->handler.createParamExpr(uint32_t(params.size())), ret_type);

			auto attributes = evo::SmallVector<pir::Parameter::Attribute>{
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNoAlias()),
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNonNull()),
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrDereferencable(
					this->context.getTypeManager().numBytes(func_type.returnTypes[0].asTypeID())
				)),
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrWritable()),
			};

			if(this->data.getConfig().useReadableNames){
				params.emplace_back("RET_RVO", this->module.createPtrType(), std::move(attributes));
			}else{
				params.emplace_back(
					std::format(".{}", params.size()), this->module.createPtrType(), std::move(attributes)
				);
			}
		}

		auto error_return_type = std::optional<pir::Type>();
		if(func_type.hasErrorReturnValue()){
			auto debug_members = evo::SmallVector<pir::meta::StructType::Member>();
			if(this->data.config.includeDebugInfo){
				debug_members.reserve(func_type.errorTypes.size());
			}


			auto error_return_param_types = evo::SmallVector<pir::Type>();
			error_return_param_types.reserve(func_type.errorTypes.size());
			for(size_t i = 0; TypeInfo::VoidableID error_type_id : func_type.errorTypes){
				EVO_DEFER([&](){ i += 1; });

				const PIRType error_type = [&]() -> PIRType {
					if(this->data.config.includeDebugInfo){
						return this->get_type<false, true>(error_type_id.asTypeID());
					}else{
						return this->get_type<false, false>(error_type_id.asTypeID());
					}
				}();

				error_return_param_types.emplace_back(error_type.type);

				if(this->data.config.includeDebugInfo){
					std::string member_name = [&]() -> std::string {
						if(func_type.hasNamedErrorReturns){
							return std::string(
								this->current_source->getTokenBuffer()[func.errorParamIdents[i]].getString()
							);	
						}else{
							return "__UNNAMED__";
						}
					}();

					debug_members.emplace_back(*error_type.meta_type_id, std::move(member_name));
				}
			}



			error_return_type = this->module.createStructType(
				this->mangle_name(func_id) + ".ERR", std::move(error_return_param_types), true
			);

			auto attributes = evo::SmallVector<pir::Parameter::Attribute>{
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNoAlias()),
				pir::Parameter::Attribute(pir::Parameter::Attribute::PtrNonNull()),
				pir::Parameter::Attribute(
					pir::Parameter::Attribute::PtrDereferencable(this->module.numBytes(*error_return_type))
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

			if(this->data.config.includeDebugInfo){
				const Location location = this->get_location(Diagnostic::Location::get(func, this->context));

				const pir::meta::StructType::ID struct_type_id = this->module.createMetaStructType(
					*error_return_type,
					this->mangle_name(func_id) + ".ERR",
					this->mangle_name(func_id) + ".ERR",
					std::move(debug_members),
					location.meta_file_id,
					location.meta_file_id, // TODO(FUTURE): get proper scope
					location.line_number
				);

				meta_params.emplace_back(struct_type_id);
			}
		}

		auto return_meta_type = std::optional<pir::meta::Type>();
		const pir::Type return_type = [&](){
			if(func_type.hasErrorReturn()){
				if(this->data.config.includeDebugInfo){
					return_meta_type = this->data.get_or_create_meta_basic_type(
						TypeManager::getTypeBool(), this->module, "Bool", this->module.createBoolType()
					);
				}
				return this->module.createBoolType();

			}else if(func_type.hasNamedReturns || is_implicit_rvo){
				return this->module.createVoidType();

			}else{
				const PIRType pir_type = this->get_type<false, true>(func_type.returnTypes[0]);
				return_meta_type = pir_type.meta_type_id;
				return pir_type.type;
			}
		}();


		auto pir_funcs = evo::SmallVector<evo::Variant<std::monostate, pir::Function::ID, pir::ExternalFunction::ID>>();


		const pir::CallingConvention calling_conv = [&](){
			if(func.attributes.isExport){ return pir::CallingConvention::C; }
			return pir::CallingConvention::FAST;
		}();

		const pir::Linkage linkage = [&](){
			if(func.attributes.isExport){ return pir::Linkage::EXTERNAL; }
			return pir::Linkage::PRIVATE;
		}();



		std::string mangled_name = this->mangle_name(func_id);

		if(func.hasInParam == false){
			auto meta_id = std::optional<pir::meta::Function::ID>();
			if(this->data.config.includeDebugInfo){
				const Location location = this->get_location(Diagnostic::Location::get(func, this->context));

				meta_id = this->module.createMetaFunction(
					evo::copy(mangled_name),
					this->get_unmangled_func_name(func),
					return_meta_type,
					std::move(meta_params),
					location.meta_file_id,
					location.meta_file_id, // TODO(FUTURE): get proper scope
					location.line_number
				);
			}

			if(func.isClangFunc()){
				if(this->data.add_extern_func_if_needed(mangled_name)){ // prevent ODR violation
					const pir::ExternalFunction::ID created_external_func_id = this->module.createExternalFunction(
						std::move(mangled_name),
						std::move(params),
						pir::CallingConvention::C,
						linkage,
						return_type,
						func.attributes.isNoReturn,
						meta_id
					);

					pir_funcs.emplace_back(created_external_func_id);

					this->data.create_func(
						func_id,
						std::move(pir_funcs), // first arg of FuncInfo construction
						return_type,
						is_implicit_rvo,
						func.attributes.isNoReturn,
						std::move(param_infos),
						std::move(return_params),
						error_return_type
					);
				}

				return std::nullopt;
				
			}else{
				const pir::Function::ID new_func_id = this->module.createFunction(
					std::move(mangled_name),
					std::move(params),
					calling_conv,
					linkage,
					return_type,
					func.attributes.isNoReturn,
					meta_id
				);

				pir_funcs.emplace_back(new_func_id);

				this->handler.setTargetFunction(new_func_id);
				this->handler.createBasicBlock(this->name("begin"));
				this->handler.removeTargetFunction();
			}


		}else{
			const size_t num_instantiations = 1ull << size_t(in_param_index);
			pir_funcs.reserve(num_instantiations);

			size_t non_copyable_bitmap = 0;
			size_t non_movable_bitmap = 0;

			for(size_t i = 0; const BaseType::Function::Param& param : func_type.params){
				if(param.kind != BaseType::Function::Param::Kind::IN){ continue; }

				if(this->context.getTypeManager().isCopyable(param.typeID) == false){
					non_copyable_bitmap |= size_t(1) << i;
				}

				if(this->context.getTypeManager().isMovable(param.typeID) == false){
					non_movable_bitmap |= size_t(1) << i;
				}

				i += 1;
			}


			for(size_t i = 0; i < num_instantiations; i+=1){
				if(((i & non_copyable_bitmap) != 0) || ((~i & non_movable_bitmap) != 0)){
					pir_funcs.emplace_back(std::monostate());
					continue;
				}

				std::string name = mangled_name;

				if(this->data.getConfig().useReadableNames){
					name += ".in_";
					for(size_t j = 0; j < in_param_index; j+=1){
						name += bool((i >> j) & 1) ? 'C' : 'M';
					}
				}else{
					name += ".in";
					name += std::to_string(i);
				}

				auto meta_id = std::optional<pir::meta::Function::ID>();
				if(this->data.config.includeDebugInfo){
					const Location location = this->get_location(Diagnostic::Location::get(func, this->context));

					meta_id = this->module.createMetaFunction(
						evo::copy(name),
						this->get_unmangled_func_name(func),
						return_meta_type,
						evo::copy(meta_params),
						location.meta_file_id,
						location.meta_file_id, // TODO(FUTURE): get proper scope
						location.line_number
					);
				}

				const pir::Function::ID new_func_id = this->module.createFunction(
					std::move(name),
					evo::copy(params),
					calling_conv,
					linkage,
					return_type,
					func.attributes.isNoReturn,
					meta_id
				);

				this->handler.setTargetFunction(new_func_id);
				this->handler.createBasicBlock(this->name("begin"));
				this->handler.removeTargetFunction();

				pir_funcs.emplace_back(new_func_id);
			}
		}


		const pir::Function::ID new_func_id = [&](){
			using PIRFuncID = evo::Variant<std::monostate, pir::Function::ID, pir::ExternalFunction::ID>;
			for(const PIRFuncID& pir_func : pir_funcs | std::views::reverse){
				if(pir_func.is<pir::Function::ID>()){ return pir_func.as<pir::Function::ID>(); }
			}
			evo::unreachable();
		}();

		this->data.create_func(
			func_id,
			std::move(pir_funcs), // first arg of FuncInfo construction
			return_type,
			is_implicit_rvo,
			func.attributes.isNoReturn,
			std::move(param_infos),
			std::move(return_params),
			error_return_type
		);

		return new_func_id;
	}


	auto SemaToPIR::lowerFuncDefComptime(sema::Func::ID func_id) -> void { this->lower_func_def_detail(func_id, true); }
	auto SemaToPIR::lowerFuncDef(sema::Func::ID func_id) -> void { this->lower_func_def_detail(func_id, false); }



	auto SemaToPIR::lower_func_def_detail(sema::Func::ID func_id, bool lower_comptime) -> void {
		const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(func_id);
		evo::debugAssert(sema_func.status == sema::Func::Status::DEF_DONE, "Incorrect status for lowering func def");
		evo::debugAssert(sema_func.isSrcFunc(), "cannot lower def of non src func");

		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(sema_func.typeID);

		this->current_source = &this->context.getSourceManager()[sema_func.sourceID.as<Source::ID>()];
		EVO_DEFER([&](){ this->current_source = nullptr; });

		this->current_func_type = &this->context.getTypeManager().getFunction(sema_func.typeID);

		const Data::FuncInfo& func_info = this->data.get_func(func_id);

		auto error_ret_debug_meta_type = std::optional<pir::meta::Type>();
		if(this->data.config.includeDebugInfo && sema_func.hasNamedErrors()){
			const pir::StructType& err_ret_struct_type = this->module.getStructType(*func_info.error_return_type);

			error_ret_debug_meta_type = this->module.createMetaQualifiedType(
				std::format("{}&", err_ret_struct_type.name),
				std::format("{}&", err_ret_struct_type.name),
				*this->module.lookupMetaStructType(*func_info.error_return_type),
				pir::meta::QualifiedType::Qualifier::MUT_REFERENCE
			);
		}


		this->in_param_bitmap = 0;
		using PIRFuncID = evo::Variant<std::monostate, pir::Function::ID, pir::ExternalFunction::ID>;
		for(const PIRFuncID pir_id : func_info.pir_ids){
			EVO_DEFER([&](){ this->in_param_bitmap += 1; });

			if(pir_id.is<std::monostate>()){ continue; }

			pir::Function& func = this->module.getFunction(pir_id.as<pir::Function::ID>());

			this->current_func_info = &this->data.get_func(func_id);
			EVO_DEFER([&](){ this->current_func_info = nullptr; });

			if(this->data.config.includeDebugInfo){
				this->local_scopes.emplace(*func.getMetaID());
			}

			EVO_DEFER([&](){
				if(this->data.config.includeDebugInfo){
					this->local_scopes.pop();
				}
			});

			this->handler.setTargetFunction(func);
			this->handler.setTargetBasicBlockAtEnd();

			this->push_scope_level();

			if(this->current_func_type->hasNamedReturns){
				for(size_t i = 0; TypeInfo::VoidableID ret_type : this->current_func_type->returnTypes){
					this->get_current_scope_level().defers.emplace_back(
						AutoDeleteManagedLifetimeTarget(func_info.return_params[i].expr, ret_type.asTypeID()),
						DeferItem::Targets{
							.on_scope_end = false,
							.on_return    = false,
							.on_error     = true,
							.on_continue  = false,
							.on_break     = false,
						}
					);

					i += 1;
				}
			}

			if(this->current_func_type->hasNamedErrorReturns){
				for(uint32_t i = 0; TypeInfo::VoidableID error_type : this->current_func_type->errorTypes){
					this->get_current_scope_level().defers.emplace_back(
						AutoDeleteManagedLifetimeTarget(ManagedLifetimeErrorParam(i), error_type.asTypeID()),
						DeferItem::Targets{
							.on_scope_end = false,
							.on_return    = false,
							.on_error     = true,
							.on_continue  = false,
							.on_break     = false,
						}
					);

					i += 1;
				}
			}


			if(
				sema_func.name.is<Token::ID>() &&
				this->current_source->getTokenBuffer()[sema_func.name.as<Token::ID>()].kind()
					== Token::Kind::KEYWORD_DELETE
			){
				const TypeInfo& this_type =
					this->context.getTypeManager().getTypeInfo(this->current_func_type->params[0].typeID);
				const BaseType::Struct& this_struct_type =
					this->context.getTypeManager().getStruct(this_type.baseTypeID().structID());

				for(const BaseType::Struct::MemberVar& member_var : this_struct_type.memberVars){
					const uint32_t abi_index = [&]() -> uint32_t {
						for(
							uint32_t i = 0;
							const BaseType::Struct::MemberVar* abi_member_var : this_struct_type.memberVarsABI
						){
							if(abi_member_var == &member_var){ return i; }
							i += 1;
						}
						evo::debugFatalBreak("Didn't find abi member");
					}();

					this->get_current_scope_level().defers.emplace_back(
						AutoDeleteManagedLifetimeTarget(OpDeleteThisAccessor(abi_index), member_var.typeID),
						DeferItem::Targets{
							.on_scope_end = true,
							.on_return    = true,
							.on_error     = false,
							.on_continue  = false,
							.on_break     = false,
						}
					);
				}
			}


			this->param_allocas.reserve(func.getParameters().size());
			for(size_t i = 0; const sema::Func::Param& param : sema_func.params){
				const pir::Parameter& pir_param = func.getParameters()[i];

				const std::string_view param_name = sema_func.getParamName(param, this->context.getSourceManager());

				const pir::Expr param_alloca = this->handler.createAlloca(
					pir_param.getType(), this->name("{}.ALLOCA", param_name)
				);

				this->handler.createStore(param_alloca, this->handler.createParamExpr(uint32_t(i)));

				this->param_allocas.emplace_back(param_alloca);

				if(this->data.config.includeDebugInfo){
					const Source::Location param_location = 
						Diagnostic::Location::get(param.ident.as<Token::ID>(), *this->current_source)
							.as<Source::Location>();

					const auto ssl =
						this->create_scoped_source_location(param_location.lineStart, param_location.collumnStart);

					const pir::meta::Type param_semantic_meta_type =
						*this->get_type<false, true>(func_type.params[i].typeID).meta_type_id;

					const pir::meta::Type param_meta_type = [&]() -> pir::meta::Type {
						if(this->current_func_info->params[i].is_copy()){
							return param_semantic_meta_type;

						}else{
							return this->data.get_or_create_meta_reference_qualified_type(
								func_type.params[i].typeID,
								module,
								std::format(
									"{}&",
									this->context.getTypeManager().printType(func_type.params[i].typeID, this->context)
								),
								param_semantic_meta_type,
								pir::meta::QualifiedType::Qualifier::MUT_REFERENCE
							);
						}
					}();

					this->handler.createMetaParam(std::string(param_name), param_alloca, uint32_t(i), param_meta_type);
				}

				i += 1;
			}

			for(size_t i = 0; Token::ID ret_param_token_id : sema_func.returnParamIdents){
				const size_t abi_index = i + sema_func.params.size();
				const pir::Parameter& pir_param = func.getParameters()[abi_index];

				const std::string_view param_name =
					this->current_source->getTokenBuffer()[ret_param_token_id].getString();

				const pir::Expr param_alloca = this->handler.createAlloca(
					pir_param.getType(), this->name("{}.ALLOCA", param_name)
				);

				this->handler.createStore(param_alloca, this->handler.createParamExpr(uint32_t(abi_index)));

				this->param_allocas.emplace_back(param_alloca);

				if(this->data.config.includeDebugInfo){
					const Source::Location param_location = 
						Diagnostic::Location::get(ret_param_token_id, *this->current_source).as<Source::Location>();

					const auto ssl =
						this->create_scoped_source_location(param_location.lineStart, param_location.collumnStart);

					const pir::meta::Type param_semantic_meta_type =
						*this->get_type<false, true>(func_type.returnTypes[i].asTypeID()).meta_type_id;

					const pir::meta::Type param_meta_type = this->data.get_or_create_meta_reference_qualified_type(
						func_type.returnTypes[i].asTypeID(),
						module,
						std::format(
							"{}&",
							this->context.getTypeManager().printType(func_type.returnTypes[i].asTypeID(), this->context)
						),
						param_semantic_meta_type,
						pir::meta::QualifiedType::Qualifier::MUT_REFERENCE
					);

					this->handler.createMetaParam(
						std::string(param_name), param_alloca, uint32_t(abi_index), param_meta_type
					);
				}
			
				i += 1;
			}

			// RET param
			if(func_type.hasErrorReturn() && func_type.returnsVoid() == false && func_type.hasNamedReturns == false){
				const pir::Expr ret_alloca = this->handler.createAlloca(
					this->module.createPtrType(), this->name("RET.ALLOCA")
				);

				this->handler.createStore(
					ret_alloca, this->handler.createParamExpr(uint32_t(func.getParameters().size() - 2))
				);

				this->param_allocas.emplace_back(ret_alloca);
			}


			if(sema_func.hasNamedErrors()){
				const size_t abi_index = func.getParameters().size() - 1;
				const pir::Parameter& pir_param = func.getParameters()[abi_index];

				const std::string_view param_name = "__ERR";

				const pir::Expr param_alloca = this->handler.createAlloca(
					pir_param.getType(), this->name("__ERR.ALLOCA")
				);

				this->handler.createStore(param_alloca, this->handler.createParamExpr(uint32_t(abi_index)));

				this->param_allocas.emplace_back(param_alloca);

				if(this->data.config.includeDebugInfo){
					const Source::Location param_location = Diagnostic::Location::get(
						sema_func.errorParamIdents.front(), *this->current_source
					).as<Source::Location>();

					const auto ssl =
						this->create_scoped_source_location(param_location.lineStart, param_location.collumnStart);

					this->handler.createMetaParam(
						std::string(param_name), param_alloca, uint32_t(abi_index), *error_ret_debug_meta_type
					);
				}
			}


			const sema::StmtBlock& stmt_block = [&]() -> const sema::StmtBlock& {
				if(lower_comptime == false && sema_func.attributes.isRTDiff){
					return sema_func.stmtBlockRT;
				}else{
					return sema_func.stmtBlock;
				}
			}();

			for(const sema::Stmt& stmt : stmt_block){
				this->lower_stmt(stmt);
			}


			if(stmt_block.isTerminated() == false){
				const auto ssl = this->create_scoped_source_location(sema_func);

				if(this->current_func_type->returnsVoid()){
					this->output_defers_for_scope_level<DeferTarget::RETURN>(this->scope_levels.back());

					if(this->current_func_type->hasErrorReturn()){
						this->handler.createRet(this->handler.createBoolean(false));
					}else{
						this->handler.createRet();
					}
					
				}else{
					this->handler.createUnreachable();
				}
			}

			this->pop_scope_level();

			this->local_func_exprs.clear();
			this->param_allocas.clear();
		}

		this->current_func_type = nullptr;
	}




	auto SemaToPIR::lowerInterface(BaseType::Interface::ID interface_id) -> void {
		const BaseType::Interface& interface = this->context.getTypeManager().getInterface(interface_id);

		evo::debugAssert(interface.isPolymorphic, "Only polymorphic interfaces can be lowered");

		auto error_return_types = evo::SmallVector<std::optional<pir::Type>>();
		error_return_types.reserve(interface.methods.size());
		for(size_t i = 0; const sema::Func::ID method_id : interface.methods){
			const sema::Func& method = this->context.getSemaBuffer().getFunc(method_id);
			const BaseType::Function method_type = this->context.getTypeManager().getFunction(method.typeID);

			if(method_type.hasNamedErrorReturns){
				auto error_return_param_types = evo::SmallVector<pir::Type>();
				for(TypeInfo::VoidableID error_type : method_type.errorTypes){
					error_return_param_types.emplace_back(this->get_type<false, false>(error_type.asTypeID()).type);
				}

				error_return_types.emplace_back(
					this->module.createStructType(
						std::format("{}.method_{}.ERR", this->mangle_name(interface_id), i),
						std::move(error_return_param_types),
						true
					)
				);

			}else{
				error_return_types.emplace_back(std::nullopt);
			}

			i += 1;
		}

		this->data.create_interface(interface_id, std::move(error_return_types));
	}



	auto SemaToPIR::lowerInterfaceVTableComptime(
		BaseType::Interface::ID interface_id, TypeInfo::ID type_id, const evo::SmallVector<sema::Func::ID>& funcs
	) -> void {
		const BaseType::Interface& interface_type = this->context.getTypeManager().getInterface(interface_id);

		evo::debugAssert(interface_type.isPolymorphic, "Only polymorphic interfaces can have vtables");

		if(funcs.size() == 1){
			this->data.create_single_method_vtable(
				Data::VTableID(interface_id, type_id), this->data.get_func(funcs[0]).pir_ids[0].as<pir::Function::ID>()
			);
			return;
		}


		std::string vtable_name = [&](){
			const TypeInfo& type_info = this->context.getTypeManager().getTypeInfo(type_id);

			if(type_info.qualifiers().empty() == false){
				return std::format("PTHR.vtable.i{}.qt{}", interface_id.get(), type_id.get());
			}

			switch(type_info.baseTypeID().kind()){
				case BaseType::Kind::PRIMITIVE:
					return std::format(
						"PTHR.vtable.i{}.p{}", interface_id.get(), type_info.baseTypeID().primitiveID().get()
					);

				case BaseType::Kind::ARRAY:
					return std::format(
						"PTHR.vtable.i{}.a{}", interface_id.get(), type_info.baseTypeID().arrayID().get()
					);

				case BaseType::Kind::DISTINCT_ALIAS:
					return std::format(
						"PTHR.vtable.i{}.da{}", interface_id.get(), type_info.baseTypeID().distinctAliasID().get()
					);

				case BaseType::Kind::STRUCT:
					return std::format(
						"PTHR.vtable.i{}.s{}", interface_id.get(), type_info.baseTypeID().structID().get()
					);

				case BaseType::Kind::UNION:
					return std::format(
						"PTHR.vtable.i{}.u{}", interface_id.get(), type_info.baseTypeID().unionID().get()
					);

				case BaseType::Kind::DUMMY:             case BaseType::Kind::FUNCTION:
				case BaseType::Kind::ARRAY_DEDUCER:     case BaseType::Kind::ARRAY_REF:
				case BaseType::Kind::ARRAY_REF_DEDUCER: case BaseType::Kind::ALIAS:
				case BaseType::Kind::STRUCT_TEMPLATE:   case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER:
				case BaseType::Kind::TYPE_DEDUCER:      case BaseType::Kind::ENUM:
				case BaseType::Kind::INTERFACE:         case BaseType::Kind::POLY_INTERFACE_REF:
				case BaseType::Kind::INTERFACE_MAP: {
					evo::debugFatalBreak("Not valid base type for VTable");
				} break;
			}

			evo::unreachable();
		}();


		auto vtable_values = evo::SmallVector<pir::GlobalVar::Value>();
		vtable_values.reserve(funcs.size());
		for(sema::Func::ID func_id : funcs){
			const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);

			if(func.attributes.isComptime){
				vtable_values.emplace_back(
					this->handler.createFunctionPointer(
						this->data.get_func(func_id).pir_ids[0].as<pir::Function::ID>()
					)
				);
			}else{
				vtable_values.emplace_back(this->handler.createNullptr());
			}
		}


		const pir::GlobalVar::ID vtable = this->module.createGlobalVar(
			std::move(vtable_name),
			this->module.getOrCreateArrayType(this->module.createPtrType(), uint64_t(interface_type.methods.size())),
			pir::Linkage::EXTERNAL,
			this->module.createGlobalArray(this->module.createPtrType(), std::move(vtable_values)),
			true
		);

		this->data.create_vtable(Data::VTableID(interface_id, type_id), vtable);
	}





	auto SemaToPIR::lowerInterfaceVTableDef(
		BaseType::Interface::ID interface_id, TypeInfo::ID type_id, const evo::SmallVector<sema::Func::ID>& funcs
	) -> void {
		evo::debugAssert(
			this->context.getTypeManager().getInterface(interface_id).isPolymorphic,
			"Only polymorphic interfaces can have vtables"
		);


		if(funcs.size() == 1){
			const sema::Func& func = this->context.getSemaBuffer().getFunc(funcs[0]);

			if(func.attributes.isComptime == false){
				this->data.create_single_method_vtable(
					Data::VTableID(interface_id, type_id),
					this->data.get_func(funcs[0]).pir_ids[0].as<pir::Function::ID>()
				);
			}

			return;
		}


		auto vtable_values = evo::SmallVector<pir::GlobalVar::Value>();
		vtable_values.reserve(funcs.size());
		for(sema::Func::ID func_id : funcs){
			vtable_values.emplace_back(
				this->handler.createFunctionPointer(this->data.get_func(func_id).pir_ids[0].as<pir::Function::ID>())
			);
		}


		const pir::GlobalVar::ID pir_var_id = this->data.get_vtable(Data::VTableID(interface_id, type_id));
		this->module.getGlobalVar(pir_var_id).value =
			this->module.createGlobalArray(this->module.createPtrType(), std::move(vtable_values));
	}




	auto SemaToPIR::createJITEntry(sema::Func::ID target_entry_func) -> pir::Function::ID {
		const Data::FuncInfo& target_entry_func_info = this->data.get_func(target_entry_func);

		const pir::Function::ID entry_func_id = this->module.createFunction(
			"PTHR.entry",
			evo::SmallVector<pir::Parameter>{},
			pir::CallingConvention::C,
			pir::Linkage::EXTERNAL,
			this->module.createUnsignedType(8)
		);

		pir::Function& entry_func = this->module.getFunction(entry_func_id);

		this->handler.setTargetFunction(entry_func);

		this->handler.createBasicBlock();
		this->handler.setTargetBasicBlockAtEnd();

		const pir::Expr entry_call = 
			this->handler.createCall(target_entry_func_info.pir_ids[0].as<pir::Function::ID>(), {});
		this->handler.createRet(entry_call);

		return entry_func_id;
	}


	auto SemaToPIR::createConsoleExecutableEntry(sema::Func::ID target_entry_func) -> pir::Function::ID {
		const Data::FuncInfo& target_entry_func_info = this->data.get_func(target_entry_func);

		auto meta_id = std::optional<pir::meta::Function::ID>();
		if(this->data.config.includeDebugInfo){
			const pir::Function& entry_func =
				this->module.getFunction(target_entry_func_info.pir_ids[0].as<pir::Function::ID>());

			const pir::meta::Function& entry_meta_func = this->module.getMetaFunction(*entry_func.getMetaID());

			const pir::meta::BasicType::ID int_meta_type = 
				this->module.createMetaBasicType("int", "int", this->module.createSignedType(32));

			meta_id = this->module.createMetaFunction(
				"main",
				"main",
				int_meta_type,
				evo::SmallVector<pir::meta::Type>{
					int_meta_type,
					this->module.createMetaQualifiedType(
						"const char*[]",
						"const char*[]",
						this->module.createMetaQualifiedType(
							"const char*",
							"const char*",
							this->module.createMetaBasicType("char", "char", this->module.createSignedType(8)),
							pir::meta::QualifiedType::Qualifier::POINTER
						),
						pir::meta::QualifiedType::Qualifier::MUT_POINTER 
					)
				},
				entry_meta_func.fileID,
				entry_meta_func.scopeWhereDefined,
				0
			);
		}

		const pir::Function::ID main_func_id = this->module.createFunction(
			"main",
			evo::SmallVector<pir::Parameter>{
				pir::Parameter("argc", this->module.createSignedType(32)),
				pir::Parameter("argv", this->module.createPtrType()),
			},
			pir::CallingConvention::C,
			pir::Linkage::EXTERNAL,
			this->module.createSignedType(32),
			false,
			meta_id
		);

		pir::Function& main_func = this->module.getFunction(main_func_id);

		this->handler.setTargetFunction(main_func);

		this->handler.createBasicBlock();
		this->handler.setTargetBasicBlockAtEnd();

		const auto ssl = this->create_scoped_source_location(*meta_id, 0, 0);

		const pir::Expr entry_call = this->handler.createCall(
			target_entry_func_info.pir_ids[0].as<pir::Function::ID>(), {}
		);

		const pir::Expr entry_call_conv = this->handler.createBitCast(entry_call, this->module.createSignedType(8));
		const pir::Expr sext = this->handler.createSExt(entry_call_conv, this->module.createSignedType(32));

		this->handler.createRet(sext);

		return main_func_id;
	}


	auto SemaToPIR::createWindowedExecutableEntry(sema::Func::ID target_entry_func) -> pir::Function::ID {
		switch(this->context.getConfig().target.platform){
			case core::Target::Platform::WINDOWS: {
				const Data::FuncInfo& target_entry_func_info = this->data.get_func(target_entry_func);

				auto meta_id = std::optional<pir::meta::Function::ID>();
				if(this->data.config.includeDebugInfo){
					const pir::Function& entry_func =
						this->module.getFunction(target_entry_func_info.pir_ids[0].as<pir::Function::ID>());

					const pir::meta::Function& entry_meta_func = this->module.getMetaFunction(*entry_func.getMetaID());

					const pir::meta::BasicType::ID int_meta_type = 
						this->module.createMetaBasicType("int", "int", this->module.createSignedType(32));

					const pir::meta::QualifiedType::ID h_instance_meta_type = this->module.createMetaQualifiedType(
						"HINSTANCE", "HINSTANCE", std::nullopt, pir::meta::QualifiedType::Qualifier::MUT_POINTER
					);

					const pir::meta::QualifiedType::ID lpstr_type = this->module.createMetaQualifiedType(
						"LPSTR",
						"LPSTR",
						this->module.createMetaBasicType("char", "char", this->module.createSignedType(8)),
						pir::meta::QualifiedType::Qualifier::MUT_POINTER
					);

					meta_id = this->module.createMetaFunction(
						"WinMain",
						"WinMain",
						int_meta_type,
						evo::SmallVector<pir::meta::Type>{
							h_instance_meta_type,
							h_instance_meta_type,
							lpstr_type,
							int_meta_type,
						},
						entry_meta_func.fileID,
						entry_meta_func.scopeWhereDefined,
						0
					);
				}


				const pir::Function::ID win_main_func_id = this->module.createFunction(
					"WinMain",
					{
						pir::Parameter("hInstance", this->module.createPtrType()),
						pir::Parameter("hPrevInstance", this->module.createPtrType()),
						pir::Parameter("lpCmdLine", this->module.createPtrType()),
						pir::Parameter("nShowCmd", this->module.createSignedType(32)),
					},
					pir::CallingConvention::C,
					pir::Linkage::EXTERNAL,
					this->module.createSignedType(32),
					false,
					meta_id
				);

				pir::Function& win_main_entry_func = this->module.getFunction(win_main_func_id);

				this->handler.setTargetFunction(win_main_entry_func);

				this->handler.createBasicBlock();
				this->handler.setTargetBasicBlockAtEnd();

				const auto ssl = this->create_scoped_source_location(*meta_id, 0, 0);

				std::ignore = this->handler.createCall(target_entry_func_info.pir_ids[0].as<pir::Function::ID>(), {});

				this->handler.createRet(
					this->handler.createNumber(this->module.createSignedType(32), core::GenericInt::create<int32_t>(0))
				);

				return win_main_func_id;
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
			this->module.createVoidType(),
			false,
			std::nullopt
		);


		pir::Function& jit_interface_func = this->module.getFunction(jit_interface_func_id);

		this->handler.setTargetFunction(jit_interface_func);

		this->handler.createBasicBlock();
		this->handler.setTargetBasicBlockAtEnd();


		auto args = evo::SmallVector<pir::Expr>();
		args.reserve(target_pir_func.getParameters().size());

		size_t param_i = 0;

		for(const BaseType::Function::Param& param : func_type.params){
			const pir::Expr param_calc_ptr = this->handler.createCalcPtr(
				this->handler.createParamExpr(0),
				this->module.createPtrType(),
				evo::SmallVector<pir::CalcPtr::Index>{int64_t(param_i)}
			);

			if(target_pir_func.getParameters()[param_i].getType().kind() == pir::Type::Kind::PTR){
				if(this->context.getTypeManager().getTypeInfo(param.typeID).isPointer()){
					args.emplace_back(
						this->handler.createLoad(
							this->handler.createLoad(param_calc_ptr, this->module.createPtrType()),
							target_pir_func.getParameters()[param_i].getType()
						)
					);
				}else{
					args.emplace_back(this->handler.createLoad(param_calc_ptr, this->module.createPtrType()));
				}

			}else{
				args.emplace_back(
					this->handler.createLoad(
						this->handler.createLoad(param_calc_ptr, this->module.createPtrType()),
						target_pir_func.getParameters()[param_i].getType()
					)
				);
			}

			param_i += 1;
		}

		if(func_type.hasNamedReturns || this->data.get_func(func_id).isImplicitRVO){
			for(size_t i = 0; i < func_type.returnTypes.size(); i+=1){
				const pir::Expr param_calc_ptr = this->handler.createCalcPtr(
					this->handler.createParamExpr(0),
					this->module.createPtrType(),
					evo::SmallVector<pir::CalcPtr::Index>{int64_t(param_i)}
				);

				args.emplace_back(this->handler.createLoad(param_calc_ptr, this->module.createPtrType()));

				param_i += 1;
			}
		}

		if(func_type.hasErrorReturn() && func_type.returnsVoid() == false){
			const pir::Expr param_calc_ptr = this->handler.createCalcPtr(
				this->handler.createParamExpr(0),
				this->module.createPtrType(),
				evo::SmallVector<pir::CalcPtr::Index>{int64_t(param_i)}
			);

			args.emplace_back(this->handler.createLoad(param_calc_ptr, this->module.createPtrType()));

			param_i += 1;
		}


		if(target_pir_func.getReturnType().kind() == pir::Type::Kind::VOID){
			this->handler.createCallVoid(pir_func_id, std::move(args));
		}else{
			const pir::Expr func_return = this->handler.createCall(pir_func_id, std::move(args));
			this->handler.createStore(this->handler.createParamExpr(1), func_return);
		}

		

		///////////////////////////////////
		// done
		
		this->handler.createRet();

		return jit_interface_func_id;
	}




	template<bool MAY_LOWER_DEPENDENCY>
	auto SemaToPIR::lower_struct(BaseType::Struct::ID struct_id) -> pir::Type {
		if constexpr(MAY_LOWER_DEPENDENCY){
			if(this->data.has_struct(struct_id)){
				return this->data.get_struct(struct_id);
			}
		}

		const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(struct_id);

		this->current_source = struct_type.sourceID.visit([&](const auto& source) -> Source* {
			using SourceType = std::decay_t<decltype(source)>;
		
			if constexpr(std::is_same<SourceType, Source::ID>()){
				return &this->context.getSourceManager()[source];
		
			}else if constexpr(std::is_same<SourceType, ClangSource::ID>()){
				return nullptr;

			}else if constexpr(std::is_same<SourceType, BuiltinModule::ID>()){
				return nullptr;
		
			}else{
				static_assert(false, "Unknown source type");
			}
		});


		auto member_var_types = evo::SmallVector<pir::Type>();

		if(struct_type.memberVarsABI.empty()){
			if(struct_type.isClangType()){
				member_var_types.emplace_back(this->module.createSignedType(8));
			}else{
				member_var_types.emplace_back(this->module.createUnsignedType(1));
			}

		}else{
			member_var_types.reserve(struct_type.memberVarsABI.size());
			for(const BaseType::Struct::MemberVar* member_var : struct_type.memberVarsABI){
				member_var_types.emplace_back(this->get_type<MAY_LOWER_DEPENDENCY, false>(member_var->typeID).type);
			}
		}



		const pir::Type new_type = this->module.createStructType(
			this->mangle_name(struct_id), std::move(member_var_types), struct_type.isPacked
		);

		this->data.create_struct(struct_id, new_type);


		if(this->data.config.includeDebugInfo && this->current_source != nullptr){
			const Location location = this->get_location(Diagnostic::Location::get(struct_type, this->context));

			auto debug_members = evo::SmallVector<pir::meta::StructType::Member>();

			if(struct_type.memberVarsABI.empty() == false){
				debug_members.reserve(struct_type.memberVarsABI.size());
				for(const BaseType::Struct::MemberVar* member_var : struct_type.memberVarsABI){
					debug_members.emplace_back(
						*this->get_type<MAY_LOWER_DEPENDENCY, true>(member_var->typeID).meta_type_id,
						std::string(struct_type.getMemberName(*member_var, this->context.getSourceManager()))
					);
				}
			}else{
				const pir::Type nothing_type = [&]() -> pir::Type {
					if(struct_type.isClangType()){
						return this->module.createSignedType(8);
					}else{
						return this->module.createUnsignedType(1);
					}
				}();

				debug_members.reserve(1);
				debug_members.emplace_back(
					this->data.get_or_create_meta_basic_type(
						TypeManager::getTypeUI1(), this->module, "UI1", nothing_type
					),
					"__UNNAMED__"
				);
			}


			std::string unmangled_struct_name = this->get_unmangled_struct_name(struct_id);

			std::ignore = this->module.createMetaStructType(
				new_type,
				evo::copy(unmangled_struct_name),
				std::move(unmangled_struct_name),
				std::move(debug_members),
				location.meta_file_id,
				this->get_current_meta_scope(),
				location.line_number
			);
		}


		return new_type;
	}


	template<bool MAY_LOWER_DEPENDENCY>
	auto SemaToPIR::lower_union(BaseType::Union::ID union_id) -> pir::Type {
		if constexpr(MAY_LOWER_DEPENDENCY){
			if(this->data.has_union(union_id)){
				return this->data.get_union(union_id);
			}
		}

		const BaseType::Union& union_type = this->context.getTypeManager().getUnion(union_id);

		this->current_source = union_type.sourceID.visit([&](const auto& source) -> Source* {
			using SourceType = std::decay_t<decltype(source)>;
		
			if constexpr(std::is_same<SourceType, Source::ID>()){
				return &this->context.getSourceManager()[source];
		
			}else if constexpr(std::is_same<SourceType, ClangSource::ID>()){
				return nullptr;

			}else if constexpr(std::is_same<SourceType, BuiltinModule::ID>()){
				return nullptr;
		
			}else{
				static_assert(false, "Unknown source type");
			}
		});


		const pir::Type underlying_data_type = this->module.getOrCreateArrayType(
			this->module.createUnsignedType(8), this->context.getTypeManager().numBytes(BaseType::ID(union_id))
		);

		const pir::Type union_pir_type = [&](){
			if(union_type.isUntagged){
				return underlying_data_type;

			}else{
				return this->module.createStructType(
					this->mangle_name(union_id),
					evo::SmallVector<pir::Type>{
						underlying_data_type,
						this->module.createUnsignedType(
							unsigned(ceil_to_multiple(std::bit_width(union_type.fields.size() - 1), 8))
						)
					},
					false
				);
			}
		}();


		if(this->data.config.includeDebugInfo){
			const Location location = this->get_location(Diagnostic::Location::get(union_type, this->context));

			auto fields = evo::SmallVector<pir::meta::UnionType::Field>();
			for(const BaseType::Union::Field& field : union_type.fields){
				if(field.typeID.isVoid()){ continue; }

				const PIRType field_type = this->get_type<false, true>(field.typeID.asTypeID());

				fields.emplace_back(
					field_type.type,
					*field_type.meta_type_id,
					std::string(union_type.getFieldName(field, this->context.getSourceManager()))
				);
			}

			std::string unmangled_union_name = this->get_unmangled_union_name(union_id);

			if(union_type.isUntagged){
				const pir::meta::UnionType::ID meta_union_id = this->module.createMetaUnionType(
					underlying_data_type,
					evo::copy(unmangled_union_name),
					std::move(unmangled_union_name),
					std::move(fields),
					location.meta_file_id,
					this->get_current_meta_scope(),
					location.line_number
				);

				this->data.create_meta_union(union_id, meta_union_id);

			}else{
				const pir::meta::UnionType::ID meta_union_id = this->module.createMetaUnionType(
					underlying_data_type,
					unmangled_union_name + "-data",
					unmangled_union_name + "-data",
					std::move(fields),
					location.meta_file_id,
					this->get_current_meta_scope(),
					location.line_number
				);


				//////////////////
				// tag enum

				auto enumerators = evo::SmallVector<pir::meta::EnumType::Enumerator>();

				for(size_t i = 0; const BaseType::Union::Field& field : union_type.fields){
					EVO_DEFER([&](){ i += 1; });
					if(field.typeID.isVoid()){ continue; }

					enumerators.emplace_back(
						std::string(union_type.getFieldName(field, this->context.getSourceManager())),
						core::GenericInt::create<size_t>(i)
					);
				}

				const uint32_t tag_bit_width =
					uint32_t(ceil_to_multiple(std::bit_width(union_type.fields.size() - 1), 8));
				const BaseType::ID tag_base_type_id =
					this->context.getTypeManager().getOrCreatePrimitiveBaseType(Token::Kind::TYPE_UI_N, tag_bit_width);
				const TypeInfo::ID tag_type_info_id =
					this->context.getTypeManager().getOrCreateTypeInfo(TypeInfo(tag_base_type_id));

				const pir::meta::BasicType::ID tag_meta_type = this->data.get_or_create_meta_basic_type(
					tag_type_info_id,
					this->module,
					std::format("UI{}", tag_bit_width),
					this->module.getStructType(union_pir_type).members[1]
				);

				const pir::meta::EnumType::ID tag_enum_id = this->module.createMetaEnumType(
					unmangled_union_name + "-tag",
					unmangled_union_name + "-tag",
					tag_meta_type,
					std::move(enumerators),
					location.meta_file_id,
					this->get_current_meta_scope(),
					location.line_number
				);


				//////////////////
				// actual type

				auto debug_members = evo::SmallVector<pir::meta::StructType::Member>{
					pir::meta::StructType::Member(meta_union_id, "data"),
					pir::meta::StructType::Member(tag_enum_id, "tag"),
				};

				const pir::meta::StructType::ID meta_tagged_union_id = this->module.createMetaStructType(
					union_pir_type,
					evo::copy(unmangled_union_name),
					std::move(unmangled_union_name),
					std::move(debug_members),
					location.meta_file_id,
					this->get_current_meta_scope(),
					location.line_number
				);

				this->data.create_meta_union(union_id, meta_tagged_union_id);
			}
		}


		this->data.create_union(union_id, union_pir_type);

		return union_pir_type;
	}


	template<bool MAY_LOWER_DEPENDENCY>
	auto SemaToPIR::lower_enum(BaseType::Enum::ID enum_id) -> void {
		if(this->data.config.includeDebugInfo == false){ return; }

		if constexpr(MAY_LOWER_DEPENDENCY){
			if(this->data.has_meta_enum(enum_id)){ return; }
		}

		const BaseType::Enum& enum_type = this->context.getTypeManager().getEnum(enum_id);

		this->current_source = enum_type.sourceID.visit([&](const auto& source) -> Source* {
			using SourceType = std::decay_t<decltype(source)>;
		
			if constexpr(std::is_same<SourceType, Source::ID>()){
				return &this->context.getSourceManager()[source];
		
			}else if constexpr(std::is_same<SourceType, ClangSource::ID>()){
				return nullptr;

			}else if constexpr(std::is_same<SourceType, BuiltinModule::ID>()){
				return nullptr;
		
			}else{
				static_assert(false, "Unknown source type");
			}
		});


		auto enumerators = evo::SmallVector<pir::meta::EnumType::Enumerator>();
		enumerators.reserve(enum_type.enumerators.size());
		for(const BaseType::Enum::Enumerator& enumerator : enum_type.enumerators){
			enumerators.emplace_back(
				std::string(enum_type.getEnumeratorName(enumerator, this->context.getSourceManager())), enumerator.value
			);
		}

		const Location location = this->get_location(Diagnostic::Location::get(enum_type, this->context));
		
		const pir::meta::EnumType::ID meta_enum_type_id = this->module.createMetaEnumType(
			this->context.getTypeManager().printType(BaseType::ID(enum_id), this->context),
			this->context.getTypeManager().printType(BaseType::ID(enum_id), this->context),
			*this->get_type<false, true>(BaseType::ID(enum_type.underlyingTypeID)).meta_type_id,
			std::move(enumerators),
			location.meta_file_id,
			this->get_current_meta_scope(),
			location.line_number
		);

		this->data.create_meta_enum(enum_id, meta_enum_type_id);
	}




	template<bool IS_COMPTIME>
	auto SemaToPIR::lower_interface_vtable_def_impl(
		BaseType::Interface::ID interface_id, TypeInfo::ID type_id, const evo::SmallVector<sema::Func::ID>& funcs
	) -> void {
		evo::debugAssert(
			this->context.getTypeManager().getInterface(interface_id).isPolymorphic,
			"Only polymorphic interfaces can have vtables"
		);


		auto vtable_values = evo::SmallVector<pir::GlobalVar::Value>();
		vtable_values.reserve(funcs.size());
		for(sema::Func::ID func_id : funcs){
			if constexpr(IS_COMPTIME){
				const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);

				if(func.attributes.isComptime){
					vtable_values.emplace_back(
						this->handler.createFunctionPointer(
							this->data.get_func(func_id).pir_ids[0].as<pir::Function::ID>()
						)
					);
				}else{
					vtable_values.emplace_back(this->handler.createNullptr());
				}

			}else{
				vtable_values.emplace_back(
					this->handler.createFunctionPointer(this->data.get_func(func_id).pir_ids[0].as<pir::Function::ID>())
				);
			}
		}


		const pir::GlobalVar::ID pir_var_id = this->data.get_vtable(Data::VTableID(interface_id, type_id));
		this->module.getGlobalVar(pir_var_id).value =
			this->module.createGlobalArray(this->module.createPtrType(), std::move(vtable_values));
	}



	auto SemaToPIR::lower_stmt(const sema::Stmt& stmt) -> void {
		switch(stmt.kind()){
			case sema::Stmt::Kind::VAR: {
				const sema::Var& var = this->context.getSemaBuffer().getVar(stmt.varID());

				if(var.kind == AST::VarDef::Kind::DEF){ break; }

				const std::string_view var_ident = this->current_source->getTokenBuffer()[var.ident].getString();

				const PIRType alloca_type = [&]() -> PIRType {
					if(this->data.config.includeDebugInfo){
						return this->get_type<false, true>(*var.typeID);
					}else{
						return this->get_type<false, false>(*var.typeID);
					}
				}();

				const auto ssl = this->create_scoped_source_location(var.line, var.collumn);

				const pir::Expr var_alloca = this->handler.createAlloca(
					alloca_type.type, this->name("{}.ALLOCA", var_ident)
				);

				if(this->data.config.includeDebugInfo){
					this->handler.createMetaLocalVar(std::string(var_ident), var_alloca, *alloca_type.meta_type_id);
				}

				this->local_func_exprs.emplace(sema::Expr(stmt.varID()), var_alloca);

				this->get_expr_store(var.expr, var_alloca);

				this->get_current_scope_level().defers.emplace_back(
					AutoDeleteManagedLifetimeTarget(var_alloca, *var.typeID),
					DeferItem::Targets{
						.on_scope_end = true,
						.on_return    = true,
						.on_error     = true,
						.on_continue  = true,
						.on_break     = true,
					}
				);
			} break;

			case sema::Stmt::Kind::FUNC_CALL: {
				const sema::FuncCall& func_call = this->context.getSemaBuffer().getFuncCall(stmt.funcCallID());

				if(func_call.target.is<IntrinsicFunc::Kind>()){
					this->intrinsic_func_call(func_call);
					break;
				}

				if(func_call.target.is<sema::TemplateIntrinsicFuncInstantiation::ID>()){
					this->template_intrinsic_func_call(func_call);
					break;
				}

				const auto ssl = this->create_scoped_source_location(func_call.line, func_call.collumn);

				const Data::FuncInfo& target_func_info = this->data.get_func(func_call.target.as<sema::Func::ID>());

				const BaseType::Function& target_type = this->context.getTypeManager().getFunction(
					this->context.getSemaBuffer().getFunc(func_call.target.as<sema::Func::ID>()).typeID
				);

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : func_call.args){
					if(target_func_info.params[i].is_copy()){
						args.emplace_back(this->get_expr_register(arg));

					}else if(target_type.params[i].kind == BaseType::Function::Param::Kind::IN){
						if(arg.kind() == sema::Expr::Kind::COPY){
							args.emplace_back(
								this->get_expr_pointer(this->context.getSemaBuffer().getCopy(arg.copyID()).expr)
							);

						}else if(arg.kind() == sema::Expr::Kind::MOVE){
							args.emplace_back(
								this->get_expr_pointer(this->context.getSemaBuffer().getMove(arg.moveID()).expr)
							);
							
						}else{
							args.emplace_back(this->get_expr_pointer(arg));
						}

					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}

				const uint32_t target_in_param_bitmap = this->calc_in_param_bitmap(target_type, func_call.args);

				if(target_func_info.return_type.kind() == pir::Type::Kind::VOID){
					if(target_func_info.isNoReturn){
						this->create_call_no_return(target_func_info.pir_ids[target_in_param_bitmap], std::move(args));
					}else{	
						this->create_call_void(target_func_info.pir_ids[target_in_param_bitmap], std::move(args));
					}

				}else{
					std::ignore = this->create_call(target_func_info.pir_ids[target_in_param_bitmap], std::move(args));
				}
			} break;

			case sema::Stmt::Kind::TRY_ELSE: {
				const sema::TryElse& try_else = this->context.getSemaBuffer().getTryElse(stmt.tryElseID());


				const Data::FuncInfo& target_func_info = this->data.get_func(try_else.target);

				const BaseType::Function& target_type = this->context.getTypeManager().getFunction(
					this->context.getSemaBuffer().getFunc(try_else.target).typeID
				);

				const auto ssl = this->create_scoped_source_location(try_else.line, try_else.collumn);

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : try_else.args){
					if(target_func_info.params[i].is_copy()){
						args.emplace_back(this->get_expr_register(arg));

					}else if(target_type.params[i].kind == BaseType::Function::Param::Kind::IN){
						if(arg.kind() == sema::Expr::Kind::COPY){
							args.emplace_back(
								this->get_expr_pointer(this->context.getSemaBuffer().getCopy(arg.copyID()).expr)
							);

						}else if(arg.kind() == sema::Expr::Kind::MOVE){
							args.emplace_back(
								this->get_expr_pointer(this->context.getSemaBuffer().getMove(arg.moveID()).expr)
							);
							
						}else{
							args.emplace_back(this->get_expr_pointer(arg));
						}

					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}


				if(target_type.errorTypes[0].isVoid() == false){
					const pir::Expr error_value = this->handler.createAlloca(
						*target_func_info.error_return_type, this->name("ERR.ALLOCA")
					);

					for(const sema::ExceptParam::ID except_param_id : try_else.exceptParams){
						const sema::ExceptParam& except_param =
							this->context.getSemaBuffer().getExceptParam(except_param_id);

						const pir::Expr except_param_pir_expr = this->handler.createCalcPtr(
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

				const uint32_t target_in_param_bitmap = this->calc_in_param_bitmap(target_type, try_else.args);


				const pir::Expr err_occurred = this->create_call(
					target_func_info.pir_ids[target_in_param_bitmap], std::move(args)
				);

				const pir::BasicBlock::ID start_block = this->handler.getTargetBasicBlock().getID();

				const pir::BasicBlock::ID if_error_block = this->handler.createBasicBlock(this->name("TRY.ERROR"));

				this->handler.setTargetBasicBlock(if_error_block);

				this->push_scope_level();

				for(const sema::Stmt& else_block_stmt : try_else.elseBlock){
					this->lower_stmt(else_block_stmt);
				}

				this->pop_scope_level();

				const pir::BasicBlock::ID if_error_block_end = this->handler.getTargetBasicBlock().getID();

				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("TRY.END"));


				this->handler.setTargetBasicBlock(start_block);
				this->handler.createBranch(err_occurred, if_error_block, end_block);

				this->handler.setTargetBasicBlock(if_error_block_end);
				this->handler.createJump(end_block);

				this->handler.setTargetBasicBlock(end_block);
			} break;

			case sema::Stmt::Kind::TRY_ELSE_INTERFACE: {
				const sema::TryElseInterface& try_else_interface = 
					this->context.getSemaBuffer().getTryElseInterface(stmt.tryElseInterfaceID());

				const auto ssl =
					this->create_scoped_source_location(try_else_interface.line, try_else_interface.collumn);


				///////////////////////////////////
				// create target func type

				const BaseType::Function& target_func_type =
					this->context.getTypeManager().getFunction(try_else_interface.funcTypeID);


				auto param_types = evo::SmallVector<pir::Type>();
				for(const BaseType::Function::Param& param : target_func_type.params){
					if(param.shouldCopy){
						param_types.emplace_back(this->get_type<false, false>(param.typeID).type);
					}else{
						param_types.emplace_back(this->module.createPtrType());
					}
				}

				if(target_func_type.errorTypes[0].isVoid() == false){
					param_types.emplace_back(this->module.createPtrType());
				}


				const pir::Type func_pir_type = this->module.getOrCreateFunctionType(
					std::move(param_types), pir::CallingConvention::FAST, this->module.createBoolType()
				);


				///////////////////////////////////
				// get func pointer

				const pir::Expr target_interface_ptr = this->get_expr_pointer(try_else_interface.value);
				const pir::Type interface_ptr_type =
					this->data.getInterfacePtrType(this->module, this->context.getSourceManager()).pir_type;

				const pir::Expr vtable_ptr = this->handler.createCalcPtr(
					target_interface_ptr,
					interface_ptr_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".VTABLE.PTR")
				);
				const pir::Expr vtable = this->handler.createLoad(
					vtable_ptr, this->module.createPtrType(), this->name(".VTABLE")
				);

				const pir::Expr target_func_ptr = this->handler.createCalcPtr(
					vtable,
					this->module.createPtrType(),
					evo::SmallVector<pir::CalcPtr::Index>{try_else_interface.vtableFuncIndex},
					this->name(".VTABLE.FUNC.PTR")
				);
				const pir::Expr target_func = this->handler.createLoad(
					target_func_ptr, this->module.createPtrType(), this->name(".VTABLE.FUNC")
				);


				///////////////////////////////////
				// make call

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : try_else_interface.args){
					if(target_func_type.params[i].shouldCopy){
						args.emplace_back(this->get_expr_register(arg));
					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}

				if(target_func_type.errorTypes[0].isVoid() == false){
					const Data::InterfaceInfo& interface_info =
						this->data.get_interface(try_else_interface.interfaceID);

					const pir::Type error_return_type =
						*interface_info.error_return_types[try_else_interface.vtableFuncIndex];

					const pir::Expr error_value = this->handler.createAlloca(error_return_type, this->name("ERR.ALLOCA"));

					for(const sema::ExceptParam::ID except_param_id : try_else_interface.exceptParams){
						const sema::ExceptParam& except_param =
							this->context.getSemaBuffer().getExceptParam(except_param_id);

						const pir::Expr except_param_pir_expr = this->handler.createCalcPtr(
							error_value,
							error_return_type,
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


				const pir::Expr err_occurred = this->handler.createCall(
					target_func,
					func_pir_type,
					std::move(args),
					this->name(".TRY_ELSE_INTERFACE.ERR_OCCURRED")
				);

				const pir::BasicBlock::ID start_block = this->handler.getTargetBasicBlock().getID();

				const pir::BasicBlock::ID if_error_block = this->handler.createBasicBlock(this->name("TRY.ERROR"));

				this->handler.setTargetBasicBlock(if_error_block);

				this->push_scope_level();

				for(const sema::Stmt& else_block_stmt : try_else_interface.elseBlock){
					this->lower_stmt(else_block_stmt);
				}

				this->pop_scope_level();

				const pir::BasicBlock::ID if_error_block_end = this->handler.getTargetBasicBlock().getID();

				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("TRY.END"));


				this->handler.setTargetBasicBlock(start_block);
				this->handler.createBranch(err_occurred, if_error_block, end_block);

				this->handler.setTargetBasicBlock(if_error_block_end);
				this->handler.createJump(end_block);

				this->handler.setTargetBasicBlock(end_block);
			} break;

			case sema::Stmt::Kind::INTERFACE_CALL: {
				const sema::InterfaceCall& interface_call =
					this->context.getSemaBuffer().getInterfaceCall(stmt.interfaceCallID());


				///////////////////////////////////
				// create target func type

				const BaseType::Function& target_func_type =
					this->context.getTypeManager().getFunction(interface_call.funcTypeID);

				const pir::Type return_type = [&](){
					if(target_func_type.hasNamedReturns){ return this->module.createVoidType(); }
					if(target_func_type.returnsVoid()){ return this->module.createVoidType(); }
					return this->get_type<false, false>(target_func_type.returnTypes[0].asTypeID()).type;
				}();

				auto param_types = evo::SmallVector<pir::Type>();
				for(const BaseType::Function::Param& param : target_func_type.params){
					if(param.shouldCopy){
						param_types.emplace_back(this->get_type<false, false>(param.typeID).type);
					}else{
						param_types.emplace_back(this->module.createPtrType());
					}
				}
				if(target_func_type.hasNamedReturns){
					for(size_t i = 0; i < target_func_type.returnTypes.size(); i+=1){
						param_types.emplace_back(this->module.createPtrType());
					}
				}


				const pir::Type func_pir_type = this->module.getOrCreateFunctionType(
					std::move(param_types), pir::CallingConvention::FAST, return_type
				);


				///////////////////////////////////
				// get func pointer

				const pir::Expr target_interface_ptr = this->get_expr_pointer(interface_call.value);
				const pir::Type interface_ptr_type =
					this->data.getInterfacePtrType(this->module, this->context.getSourceManager()).pir_type;

				const pir::Expr vtable_ptr = this->handler.createCalcPtr(
					target_interface_ptr,
					interface_ptr_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".VTABLE.PTR")
				);
				const pir::Expr vtable = this->handler.createLoad(
					vtable_ptr, this->module.createPtrType(), this->name(".VTABLE")
				);

				const pir::Expr target_func_ptr = this->handler.createCalcPtr(
					vtable,
					this->module.createPtrType(),
					evo::SmallVector<pir::CalcPtr::Index>{interface_call.vtableFuncIndex},
					this->name(".VTABLE.FUNC.PTR")
				);
				const pir::Expr target_func = this->handler.createLoad(
					target_func_ptr, this->module.createPtrType(), this->name(".VTABLE.FUNC")
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
					this->handler.createCallVoid(target_func, func_pir_type, std::move(args));
				}else{
					std::ignore = this->handler.createCall(target_func, func_pir_type, std::move(args));
				}
			} break;

			case sema::Stmt::Kind::ASSIGN: {
				const sema::Assign& assignment = this->context.getSemaBuffer().getAssign(stmt.assignID());

				const auto ssl = this->create_scoped_source_location(assignment.line, assignment.collumn);

				if(assignment.lhs.has_value()){
					this->get_expr_store(assignment.rhs, this->get_expr_pointer(*assignment.lhs));
				}else{
					this->get_expr_discard(assignment.rhs);
				}
			} break;

			case sema::Stmt::Kind::MULTI_ASSIGN: {
				const sema::MultiAssign& multi_assign = 
					this->context.getSemaBuffer().getMultiAssign(stmt.multiAssignID());

				const auto ssl = this->create_scoped_source_location(multi_assign.line, multi_assign.collumn);

				auto targets = evo::SmallVector<pir::Expr>();
				targets.reserve(multi_assign.targets.size());
				for(const evo::Variant<sema::Expr, TypeInfo::ID>& target : multi_assign.targets){
					if(target.is<sema::Expr>()){
						targets.emplace_back(this->get_expr_pointer(target.as<sema::Expr>()));
					}else{
						const pir::Expr discard_alloca = this->handler.createAlloca(
							this->get_type<false, false>(target.as<TypeInfo::ID>()).type, this->name(".DISCARD")
						);

						targets.emplace_back(discard_alloca);

						this->add_auto_delete_target(discard_alloca, target.as<TypeInfo::ID>());
					}
				}

				this->get_expr_store(multi_assign.value, targets);
			} break;

			case sema::Stmt::Kind::RETURN: {
				const sema::Return& return_stmt = this->context.getSemaBuffer().getReturn(stmt.returnID());

				const auto ssl = this->create_scoped_source_location(return_stmt.line, return_stmt.collumn);

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
							this->output_defers_for_scope_level<DeferTarget::SCOPE_END>(scope_level);
							continue;
						}

						if(return_stmt.value.has_value()){
							this->handler.createStore(scope_level.label_output_locations[0], *ret_value);
						}

						this->output_defers_for_scope_level<DeferTarget::SCOPE_END>(scope_level);

						this->handler.createJump(*scope_level.end_block);
						break;
					}

				}else{
					if(this->current_func_type->hasErrorReturn()){
						if(return_stmt.value.has_value()){
							const pir::Expr ret_value = this->get_expr_register(*return_stmt.value);

							for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
								this->output_defers_for_scope_level<DeferTarget::RETURN>(scope_level);
							}

							this->handler.createStore(this->current_func_info->return_params.front().expr, ret_value);
						}

						for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
							this->output_defers_for_scope_level<DeferTarget::RETURN>(scope_level);
						}

						this->handler.createRet(this->handler.createBoolean(false));

					}else{
						if(return_stmt.value.has_value()){
							if(this->current_func_info->isImplicitRVO){
								this->get_expr_store(
									*return_stmt.value, this->current_func_info->return_params.front().expr
								);

								for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
									this->output_defers_for_scope_level<DeferTarget::RETURN>(scope_level);
								}

								this->handler.createRet();

							}else{
								const pir::Expr ret_value = this->get_expr_register(*return_stmt.value);

								for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
									this->output_defers_for_scope_level<DeferTarget::RETURN>(scope_level);
								}

								this->handler.createRet(ret_value);
							}

						}else{
							for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
								this->output_defers_for_scope_level<DeferTarget::RETURN>(scope_level);
							}
							this->handler.createRet();
						}
					}
				}

			} break;

			case sema::Stmt::Kind::ERROR: {
				const sema::Error& error_stmt = this->context.getSemaBuffer().getError(stmt.errorID());

				const auto ssl = this->create_scoped_source_location(error_stmt.line, error_stmt.collumn);

				if(error_stmt.value.has_value()){
					this->handler.createStore(
						this->handler.createLoad(
							this->param_allocas.back(), this->module.createPtrType()
						),
						this->get_expr_register(*error_stmt.value)
					);
					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<DeferTarget::ERROR>(scope_level);
					}
					this->handler.createRet(this->handler.createBoolean(true));

				}else{
					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<DeferTarget::ERROR>(scope_level);
					}
					this->handler.createRet(this->handler.createBoolean(true));
				}
				
			} break;

			case sema::Stmt::Kind::UNREACHABLE: {
				if(this->data.getConfig().useDebugUnreachables){
					this->create_panic("Attempted to execute unreachable");
				}else{
					this->handler.createUnreachable();
				}
			} break;

			case sema::Stmt::Kind::BREAK: {
				const sema::Break& break_stmt = this->context.getSemaBuffer().getBreak(stmt.breakID());

				if(break_stmt.label.has_value()){
					const std::string_view label =
						this->current_source->getTokenBuffer()[*break_stmt.label].getString();

					#if defined(PCIT_CONFIG_DEBUG)
						bool found_loop = false;
					#endif

					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<DeferTarget::BREAK>(scope_level);

						if(scope_level.label == label){
							#if defined(PCIT_CONFIG_DEBUG)
								found_loop = true;
							#endif

							this->handler.createJump(*scope_level.end_block);
							break;
						}
					}

					#if defined(PCIT_CONFIG_DEBUG)
						evo::debugAssert(found_loop, "Labeled loop block not found");
					#endif
					
				}else{
					#if defined(PCIT_CONFIG_DEBUG)
						bool found_loop = false;
					#endif

					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<DeferTarget::BREAK>(scope_level);

						if(scope_level.is_loop){
							#if defined(PCIT_CONFIG_DEBUG)
								found_loop = true;
							#endif

							this->handler.createJump(*scope_level.end_block);
							break;
						}
					}

					#if defined(PCIT_CONFIG_DEBUG)
						evo::debugAssert(found_loop, "No loop block to break to found");
					#endif
				}
			} break;

			case sema::Stmt::Kind::CONTINUE: {
				const sema::Continue& continue_stmt = this->context.getSemaBuffer().getContinue(stmt.continueID());

				if(continue_stmt.label.has_value()){
					const std::string_view label =
						this->current_source->getTokenBuffer()[*continue_stmt.label].getString();

					#if defined(PCIT_CONFIG_DEBUG)
						bool found_loop = false;
					#endif

					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<DeferTarget::CONTINUE>(scope_level);

						if(scope_level.label == label){
							#if defined(PCIT_CONFIG_DEBUG)
								found_loop = true;
							#endif

							this->handler.createJump(*scope_level.begin_block);
							break;
						}
					}

					#if defined(PCIT_CONFIG_DEBUG)
						evo::debugAssert(found_loop, "Labeled loop block not found");
					#endif
					
				}else{
					#if defined(PCIT_CONFIG_DEBUG)
						bool found_loop = false;
					#endif

					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						this->output_defers_for_scope_level<DeferTarget::CONTINUE>(scope_level);

						if(scope_level.is_loop){
							#if defined(PCIT_CONFIG_DEBUG)
								found_loop = true;
							#endif

							this->handler.createJump(*scope_level.begin_block);
							break;
						}
					}

					#if defined(PCIT_CONFIG_DEBUG)
						evo::debugAssert(found_loop, "No loop block to continue to found");
					#endif
				}
			} break;

			case sema::Stmt::Kind::DELETE: {
				const sema::Delete& delete_stmt = this->context.getSemaBuffer().getDelete(stmt.deleteID());
				this->delete_expr(delete_stmt.expr, delete_stmt.exprTypeID);
			} break;

			case sema::Stmt::Kind::BLOCK_SCOPE: {
				const sema::BlockScope& block_scope = this->context.getSemaBuffer().getBlockScope(stmt.blockScopeID());

				if(this->data.getConfig().includeDebugInfo){
					const Location location =
						this->get_location(Diagnostic::Location::get(block_scope.openBrace, *this->current_source));

					const pir::meta::Subscope::ID block_meta_subscope = this->module.createMetaSubscope(
						std::format("meta.subscope.{}", this->data.get_meta_subscope_id()),
						this->get_current_meta_local_scope(),
						*this->current_source->getPIRMetaFileID(),
						location.line_number,
						location.collumn_number
					);

					this->local_scopes.emplace(block_meta_subscope);
				}

				this->push_scope_level();

				for(const sema::Stmt& block_stmt : block_scope.block){
					this->lower_stmt(block_stmt);
				}

				if(block_scope.block.isTerminated() == false){
					this->output_defers_for_scope_level<DeferTarget::SCOPE_END>(this->scope_levels.back());
				}

				this->pop_scope_level();

				if(this->data.getConfig().includeDebugInfo){
					this->local_scopes.pop();
				}
			} break;

			case sema::Stmt::Kind::CONDITIONAL: {
				const sema::Conditional& conditional_stmt = 
					this->context.getSemaBuffer().getConditional(stmt.conditionalID());

				const pir::BasicBlock::ID then_block = this->handler.createBasicBlock("IF.THEN");
				auto end_block = std::optional<pir::BasicBlock::ID>();


				if(this->data.getConfig().includeDebugInfo){
					const Location location =
						this->get_location(Diagnostic::Location::get(conditional_stmt.ifToken, *this->current_source));

					this->handler.pushSourceLocation(
						pir::meta::SourceLocation(
							this->get_current_meta_local_scope(), location.line_number, location.collumn_number
						)
					);
				}

				const pir::Expr cond_value = this->get_expr_register(conditional_stmt.cond);

				if(conditional_stmt.elseStmts.empty()){
					end_block = this->handler.createBasicBlock("IF.END");

					this->handler.createBranch(cond_value, then_block, *end_block);

					if(this->data.getConfig().includeDebugInfo){
						this->handler.popSourceLocation();

						const Location location = this->get_location(
							Diagnostic::Location::get(conditional_stmt.ifToken, *this->current_source)
						);

						const pir::meta::Subscope::ID created_meta_subscope = this->module.createMetaSubscope(
							std::format("meta.subscope.{}", this->data.get_meta_subscope_id()),
							this->get_current_meta_local_scope(),
							*this->current_source->getPIRMetaFileID(),
							location.line_number,
							location.collumn_number
						);

						this->local_scopes.emplace(created_meta_subscope);
					}

					this->handler.setTargetBasicBlock(then_block);
					this->push_scope_level();
					for(const sema::Stmt& block_stmt : conditional_stmt.thenStmts){
						this->lower_stmt(block_stmt);
					}
					const bool then_terminated = conditional_stmt.thenStmts.isTerminated();
					if(then_terminated == false){
						if(this->data.getConfig().includeDebugInfo){
							const Location location = this->get_location(
								Diagnostic::Location::get(conditional_stmt.closeBraceToken, *this->current_source)
							);

							this->handler.pushSourceLocation(
								pir::meta::SourceLocation(
									this->get_current_meta_local_scope(), location.line_number, location.collumn_number
								)
							);
						}

						this->output_defers_for_scope_level<DeferTarget::SCOPE_END>(this->scope_levels.back());
						this->handler.createJump(*end_block);

						if(this->data.getConfig().includeDebugInfo){
							this->handler.popSourceLocation();
						}
					}
					this->pop_scope_level();

					if(this->data.getConfig().includeDebugInfo){
						this->local_scopes.pop();
					}

				}else{
					const pir::BasicBlock::ID else_block = this->handler.createBasicBlock("IF.ELSE");

					const bool then_terminated = conditional_stmt.thenStmts.isTerminated();
					const bool else_terminated = conditional_stmt.elseStmts.isTerminated();

					this->handler.createBranch(cond_value, then_block, else_block);

					// then block
					auto then_meta_subscope = std::optional<pir::meta::Subscope::ID>();
					if(this->data.getConfig().includeDebugInfo){
						this->handler.popSourceLocation();

						const Location location = this->get_location(
							Diagnostic::Location::get(conditional_stmt.ifToken, *this->current_source)
						);

						then_meta_subscope = this->module.createMetaSubscope(
							std::format("meta.subscope.{}", this->data.get_meta_subscope_id()),
							this->get_current_meta_local_scope(),
							*this->current_source->getPIRMetaFileID(),
							location.line_number,
							location.collumn_number
						);

						this->local_scopes.emplace(*then_meta_subscope);
					}
					this->handler.setTargetBasicBlock(then_block);
					this->push_scope_level();
					for(const sema::Stmt& block_stmt : conditional_stmt.thenStmts){
						this->lower_stmt(block_stmt);
					}
					if(then_terminated == false){
						this->output_defers_for_scope_level<DeferTarget::SCOPE_END>(this->scope_levels.back());
					}
					this->pop_scope_level();
					if(this->data.getConfig().includeDebugInfo){
						this->local_scopes.pop();
					}

					// required because stuff in the then block might add basic blocks
					pir::BasicBlock& then_block_end = this->handler.getTargetBasicBlock();

					// else block
					auto else_meta_subscope = std::optional<pir::meta::Subscope::ID>();
					if(this->data.getConfig().includeDebugInfo){
						const Location location = this->get_location(
							Diagnostic::Location::get(*conditional_stmt.elseToken, *this->current_source)
						);

						else_meta_subscope = this->module.createMetaSubscope(
							std::format("meta.subscope.{}", this->data.get_meta_subscope_id()),
							this->get_current_meta_local_scope(),
							*this->current_source->getPIRMetaFileID(),
							location.line_number,
							location.collumn_number
						);

						this->local_scopes.emplace(*else_meta_subscope);
					}
					this->push_scope_level();
					this->handler.setTargetBasicBlock(else_block);
					for(const sema::Stmt& block_stmt : conditional_stmt.elseStmts){
						this->lower_stmt(block_stmt);
					}
					if(else_terminated == false){
						this->output_defers_for_scope_level<DeferTarget::SCOPE_END>(this->scope_levels.back());
					}
					this->pop_scope_level();
					if(this->data.getConfig().includeDebugInfo){
						this->local_scopes.pop();
					}

					// end block

					if(else_terminated && then_terminated){ break; }

					end_block = this->handler.createBasicBlock("IF.END");

					if(else_terminated == false){
						if(this->data.getConfig().includeDebugInfo){
							const Location location = this->get_location(
								Diagnostic::Location::get(conditional_stmt.closeBraceToken, *this->current_source)
							);

							this->handler.pushSourceLocation(
								pir::meta::SourceLocation(
									*else_meta_subscope, location.line_number, location.collumn_number
								)
							);
						}

						this->handler.createJump(*end_block);

						if(this->data.getConfig().includeDebugInfo){
							this->handler.popSourceLocation();
						}
					}

					if(then_terminated == false){
						if(this->data.getConfig().includeDebugInfo){
							const Location location = this->get_location(
								Diagnostic::Location::get(*conditional_stmt.elseToken, *this->current_source)
							);

							this->handler.pushSourceLocation(
								pir::meta::SourceLocation(
									*then_meta_subscope, location.line_number, location.collumn_number
								)
							);
						}

						this->handler.setTargetBasicBlock(then_block_end);
						this->handler.createJump(*end_block);

						if(this->data.getConfig().includeDebugInfo){
							this->handler.popSourceLocation();
						}
					}
				}


				this->handler.setTargetBasicBlock(*end_block);
			} break;

			case sema::Stmt::Kind::WHILE: {
				const sema::While& while_stmt = this->context.getSemaBuffer().getWhile(stmt.whileID());

				const pir::BasicBlock::ID cond_block = this->handler.createBasicBlock(this->name("WHILE.COND"));
				const pir::BasicBlock::ID body_block = this->handler.createBasicBlock(this->name("WHILE.BODY"));
				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("WHILE.END"));


				if(this->data.getConfig().includeDebugInfo){
					const Location location =
						this->get_location(Diagnostic::Location::get(while_stmt.whileToken, *this->current_source));

					const pir::meta::Subscope::ID while_meta_subscope = this->module.createMetaSubscope(
						std::format("meta.subscope.{}", this->data.get_meta_subscope_id()),
						this->get_current_meta_local_scope(),
						*this->current_source->getPIRMetaFileID(),
						location.line_number,
						location.collumn_number
					);

					this->local_scopes.emplace(while_meta_subscope);

					this->handler.pushSourceLocation(
						pir::meta::SourceLocation(
							while_meta_subscope, location.line_number, location.collumn_number
						)
					);
				}


				this->handler.createJump(cond_block);

				this->handler.setTargetBasicBlock(cond_block);
				const pir::Expr cond_value = this->get_expr_register(while_stmt.cond);
				this->handler.createBranch(cond_value, body_block, end_block);

				if(this->data.getConfig().includeDebugInfo){
					this->handler.popSourceLocation();
				}

				this->handler.setTargetBasicBlock(body_block);

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
					this->output_defers_for_scope_level<DeferTarget::SCOPE_END>(this->scope_levels.back());
					this->handler.createJump(cond_block);
				}

				this->pop_scope_level();
				this->handler.setTargetBasicBlock(end_block);

				if(this->data.getConfig().includeDebugInfo){
					this->local_scopes.pop();
				}
			} break;

			case sema::Stmt::Kind::FOR: {
				const sema::For& for_stmt = this->context.getSemaBuffer().getFor(stmt.forID());

				if(this->data.getConfig().includeDebugInfo){
					const Location location =
						this->get_location(Diagnostic::Location::get(for_stmt.forToken, *this->current_source));

					const pir::meta::Subscope::ID for_meta_subscope = this->module.createMetaSubscope(
						std::format("meta.subscope.{}", this->data.get_meta_subscope_id()),
						this->get_current_meta_local_scope(),
						*this->current_source->getPIRMetaFileID(),
						location.line_number,
						location.collumn_number
					);

					this->local_scopes.emplace(for_meta_subscope);

					this->handler.pushSourceLocation(
						pir::meta::SourceLocation(
							for_meta_subscope, location.line_number, location.collumn_number
						)
					);
				}


				auto index = std::optional<pir::Expr>();
				auto index_type = std::optional<pir::Type>();
				auto value_params = evo::SmallVector<pir::Expr>();

				if(for_stmt.hasIndex){
					index_type = this->get_type<false, false>(for_stmt.params[0].typeID).type;

					index = this->handler.createAlloca(
						*index_type,
						this->name(
							"FOR_PARAM_INDEX.{}.ALLOCA",
							this->current_source->getTokenBuffer()[for_stmt.params[0].ident].getString()
						)
					);

					this->local_func_exprs.emplace(for_stmt.params[0].expr, *index);


					this->handler.createStore(
						*index, this->handler.createNumber(*index_type, core::GenericInt(index_type->getWidth(), 0))
					);

					for(size_t i = 1; i < for_stmt.params.size(); i+=1){
						const pir::Expr pir_expr = this->handler.createAlloca(
							this->module.createPtrType(),
							this->name(
								"FOR_PARAM.{}.ALLOCA",
								this->current_source->getTokenBuffer()[for_stmt.params[i].ident].getString()
							)
						);

						value_params.emplace_back(pir_expr);
						this->local_func_exprs.emplace(for_stmt.params[i].expr, pir_expr);
					}

				}else{
					for(const sema::For::Param& param : for_stmt.params){
						const pir::Expr pir_expr = this->handler.createAlloca(
							this->module.createPtrType(),
							this->name(
								"FOR_PARAM.{}.ALLOCA", this->current_source->getTokenBuffer()[param.ident].getString()
							)
						);

						value_params.emplace_back(pir_expr);
						this->local_func_exprs.emplace(param.expr, pir_expr);
					}
				}


				auto iterator_type_ids = evo::SmallVector<TypeInfo::ID>();
				auto iterator_allocas = evo::SmallVector<pir::Expr>();
				iterator_allocas.reserve(for_stmt.iterables.size());
				for(const sema::For::Iterable& iterable : for_stmt.iterables){
					const sema::Func& create_iterable_func =
						this->context.getSemaBuffer().getFunc(iterable.createIteratorFunc);

					const BaseType::Function& create_iterable_func_type =
						this->context.getTypeManager().getFunction(create_iterable_func.typeID);

					const TypeInfo::ID iterator_type_id = create_iterable_func_type.returnTypes[0].asTypeID();
					iterator_type_ids.emplace_back(iterator_type_id);
					
					const pir::Expr iterator_alloca = this->handler.createAlloca(
						this->get_type<false, false>(iterator_type_id).type,
						this->name("FOR.iterator_{}", iterator_allocas.size())
					);

					const Data::FuncInfo& create_iterable_func_info = 
						this->data.get_func(iterable.createIteratorFunc);


					if(create_iterable_func_info.isImplicitRVO){
						this->create_call_void(
							create_iterable_func_info.pir_ids[0],
							evo::SmallVector<pir::Expr>{this->get_expr_pointer(iterable.expr), iterator_alloca}
						);

					}else{
						const pir::Expr iterator_value = this->create_call(
							create_iterable_func_info.pir_ids[0],
							evo::SmallVector<pir::Expr>{this->get_expr_pointer(iterable.expr)}
						);
						this->handler.createStore(iterator_alloca, iterator_value);
					}

					iterator_allocas.emplace_back(iterator_alloca);
				}


				const pir::BasicBlock::ID cond_block = this->handler.createBasicBlock(this->name("FOR.COND"));
				const pir::BasicBlock::ID body_block = this->handler.createBasicBlock(this->name("FOR.BODY"));
				const pir::BasicBlock::ID end_block  = this->handler.createBasicBlock(this->name("FOR.END"));


				this->handler.createJump(cond_block);

				this->handler.setTargetBasicBlock(cond_block);

				static constexpr size_t ITERATOR_NEXT   = 0;
				static constexpr size_t ITERATOR_GET    = 1;
				static constexpr size_t ITERATOR_AT_END = 2;

				{
					const Data::FuncInfo& at_end_func_info = 
						this->data.get_func(for_stmt.iterables[0].iteratorImpl.methods[ITERATOR_AT_END]);

					const pir::Expr cond_value = this->create_call(
						at_end_func_info.pir_ids[0],
						evo::SmallVector<pir::Expr>{iterator_allocas[0]},
						this->name("FOR.at_end")
					);

					this->handler.createBranch(cond_value, end_block, body_block);
				}


				this->handler.setTargetBasicBlock(body_block);

				if(for_stmt.label.has_value()){
					const std::string_view label = 
						this->current_source->getTokenBuffer()[*for_stmt.label].getString();
					this->push_scope_level(label, evo::SmallVector<pir::Expr>(), cond_block, end_block, true);
				}else{
					this->push_scope_level("", evo::SmallVector<pir::Expr>(), cond_block, end_block, true);
				}

				if(for_stmt.hasIndex){
					const bool index_type_is_unsigned =
						this->context.getTypeManager().isUnsignedIntegral(for_stmt.params[0].typeID);

					this->get_current_scope_level().defers.emplace_back(
						[this, index, index_type, index_type_is_unsigned]() -> void {
							const pir::Expr index_load =
								this->handler.createLoad(*index, *index_type, this->name("FOR.index"));

							const pir::Expr index_inc = this->handler.createAdd(
								index_load,
								this->handler.createNumber(*index_type, core::GenericInt(index_type->getWidth(), 1)),
								!index_type_is_unsigned,
								index_type_is_unsigned,
								this->name("FOR.index_inc")
							);

							this->handler.createStore(*index, index_inc);
						},
						DeferItem::Targets{
							.on_scope_end = true,
							.on_return    = false,
							.on_error     = false,
							.on_continue  = true,
							.on_break     = false,
						}
					);


					if(this->data.getConfig().includeDebugInfo){
						const sema::For::Param& param = for_stmt.params[0];

						this->handler.createMetaLocalVar(
							std::string(this->current_source->getTokenBuffer()[param.ident].getString()),
							*index,
							*this->get_type<false, true>(param.typeID).meta_type_id
						);
					}
				}

				for(size_t i = 0; const sema::For::Iterable& iterable : for_stmt.iterables){
					EVO_DEFER([&](){ i += 1; });


					if(this->data.getConfig().includeDebugInfo){
						const sema::For::Param& param = for_stmt.params[i + size_t(for_stmt.hasIndex)];

						this->handler.createMetaLocalVar(
							std::string(this->current_source->getTokenBuffer()[param.ident].getString()),
							value_params[i],
							this->data.get_or_create_meta_reference_qualified_type(
								param.typeID,
								this->module,
								this->context.getTypeManager().printType(param.typeID, this->context) + "&",
								*this->get_type<false, true>(param.typeID).meta_type_id,
								pir::meta::QualifiedType::Qualifier::MUT_REFERENCE
							)
						);
					}


					//////////////////
					// get

					const Data::FuncInfo& get_func_info =
						this->data.get_func(iterable.iteratorImpl.methods[ITERATOR_GET]);

					const pir::Expr get_value = this->create_call(
						get_func_info.pir_ids[0],
						evo::SmallVector<pir::Expr>{iterator_allocas[i]},
						this->name("FOR.GET_{}", i)
					);

					this->handler.createStore(value_params[i], get_value);


					//////////////////
					// next

					const Data::FuncInfo& next_func_info =
						this->data.get_func(iterable.iteratorImpl.methods[ITERATOR_NEXT]);

					this->get_current_scope_level().defers.emplace_back(
						[this, pir_func = next_func_info.pir_ids[0], iterator_alloca = iterator_allocas[i]]() -> void {
							this->create_call_void(pir_func, evo::SmallVector<pir::Expr>{iterator_alloca});
						},
						DeferItem::Targets{
							.on_scope_end = true,
							.on_return    = false,
							.on_error     = false,
							.on_continue  = true,
							.on_break     = false,
						}
					);
				}

				for(const sema::Stmt& block_stmt : for_stmt.block){
					this->lower_stmt(block_stmt);
				}

				if(for_stmt.block.isTerminated() == false){
					this->output_defers_for_scope_level<DeferTarget::SCOPE_END>(this->scope_levels.back());
					this->handler.createJump(cond_block);
				}

				this->pop_scope_level();
				this->handler.setTargetBasicBlock(end_block);

				for(size_t i = 0; const pir::Expr& iterator_alloca : iterator_allocas){
					this->delete_expr(iterator_alloca, iterator_type_ids[i]);
				}


				if(this->data.getConfig().includeDebugInfo){
					this->handler.popSourceLocation();
					this->local_scopes.pop();
				}
			} break;

			case sema::Stmt::Kind::FOR_UNROLL: {
				const sema::ForUnroll& for_unroll_stmt = this->context.getSemaBuffer().getForUnroll(stmt.forUnrollID());

				if(for_unroll_stmt.stmtBlocks.empty()){ break; }

				auto basic_blocks = evo::SmallVector<pir::BasicBlock::ID>();
				basic_blocks.reserve(for_unroll_stmt.stmtBlocks.size());
				for(size_t i = 1; i < for_unroll_stmt.stmtBlocks.size(); i+=1){ // yes, skip index 0
					basic_blocks.emplace_back(this->handler.createBasicBlock(this->name("FOR_UNROLL.LOOP_{}", i)));
				}
				basic_blocks.emplace_back(this->handler.createBasicBlock(this->name("FOR_UNROLL.END")));

				const std::string_view label_name_str = [&]() -> std::string_view {
					if(for_unroll_stmt.label.has_value()){
						return this->current_source->getTokenBuffer()[*for_unroll_stmt.label].getString();
					}else{
						return "";
					}
				}();


				const std::optional<Location> location = [&]() -> std::optional<Location> {
					if(this->data.getConfig().includeDebugInfo){
						return this->get_location(
							Diagnostic::Location::get(for_unroll_stmt.forToken, *this->current_source)
						);
					}else{
						return std::nullopt;
					}
				}();

				for(size_t i = 0; const sema::StmtBlock& stmt_block : for_unroll_stmt.stmtBlocks){
					EVO_DEFER([&](){ i += 1; });

					if(this->data.getConfig().includeDebugInfo){
						const pir::meta::Subscope::ID for_meta_subscope = this->module.createMetaSubscope(
							std::format("meta.subscope.{}", this->data.get_meta_subscope_id()),
							this->get_current_meta_local_scope(),
							*this->current_source->getPIRMetaFileID(),
							location->line_number,
							location->collumn_number
						);

						this->local_scopes.emplace(for_meta_subscope);
					}


					this->push_scope_level(
						label_name_str, evo::SmallVector<pir::Expr>(), basic_blocks[i], basic_blocks.back(), true
					);

					for(const sema::Stmt& block_stmt : stmt_block){
						this->lower_stmt(block_stmt);
					}

					if(stmt_block.isTerminated() == false){
						this->output_defers_for_scope_level<DeferTarget::SCOPE_END>(this->scope_levels.back());
						this->handler.createJump(basic_blocks[i]);
					}

					this->pop_scope_level();

					this->handler.setTargetBasicBlock(basic_blocks[i]);

					if(this->data.getConfig().includeDebugInfo){
						this->local_scopes.pop();
					}
				}
			} break;

			case sema::Stmt::Kind::SWITCH: {
				const sema::Switch& switch_stmt = this->context.getSemaBuffer().getSwitch(stmt.switchID());


				const pir::BasicBlock::ID start_basic_block = this->handler.getTargetBasicBlock().getID();

				if(switch_stmt.kind == sema::Switch::Kind::NO_JUMP){
					// const pir::Expr cond = this->get_expr_register(switch_stmt.cond);
					evo::unimplemented("lowering no jump switch");

				}else{
					const pir::Expr cond = [&]() -> pir::Expr {
						TypeInfo::ID target_cond_type_id = switch_stmt.condTypeID;
						while(true){
							const TypeInfo& target_cond_type =
								this->context.getTypeManager().getTypeInfo(target_cond_type_id);

							evo::debugAssert(target_cond_type.qualifiers().empty(), "unexpected switch cond type");

							switch(target_cond_type.baseTypeID().kind()){
								case BaseType::Kind::ALIAS: {
									target_cond_type_id = this->context.getTypeManager().getAlias(
										target_cond_type.baseTypeID().aliasID()
									).aliasedType;
								} break;

								case BaseType::Kind::DISTINCT_ALIAS: {
									target_cond_type_id = this->context.getTypeManager().getDistinctAlias(
										target_cond_type.baseTypeID().distinctAliasID()
									).underlyingType;
								} break;

								case BaseType::Kind::UNION: {
									const pir::Type union_pir_type = this->get_type<false, false>(
										target_cond_type_id, target_cond_type.baseTypeID()
									).type;

									const pir::StructType union_pir_struct_type =
										this->module.getStructType(union_pir_type);

									const pir::Expr calc_ptr = this->handler.createCalcPtr(
										this->get_expr_pointer(switch_stmt.cond),
										union_pir_type,
										evo::SmallVector<pir::CalcPtr::Index>{0, 1},
										this->name(".SWITCH.UNION_TAG_PTR")
									);

									return this->handler.createLoad(
										calc_ptr, union_pir_struct_type.members[1], this->name(".SWITCH.UNION_TAG")
									);
								} break;

								default: {
									return this->get_expr_register(switch_stmt.cond);
								} break;
							}
						}
					}();

					bool all_cases_terminated = switch_stmt.kind == sema::Switch::Kind::COMPLETE;
					auto else_index = std::optional<size_t>();

					auto basic_blocks = evo::SmallVector<pir::BasicBlock::ID>();
					basic_blocks.reserve(switch_stmt.cases.size() + 1);
					for(size_t i = 0; i < switch_stmt.cases.size(); i+=1){
						basic_blocks.emplace_back(this->handler.createBasicBlock(this->name("SWITCH.CASE_{}", i)));

						if(switch_stmt.cases[i].stmtBlock.isTerminated() == false){
							all_cases_terminated = false;
						}

						if(switch_stmt.cases[i].values.empty()){
							else_index = i;
						}
					}

					const std::optional<pir::BasicBlock::ID> end_block = [&]() -> std::optional<pir::BasicBlock::ID> {
						if(all_cases_terminated){
							return std::nullopt;
						}else{
							const pir::BasicBlock::ID created_end_block =
								this->handler.createBasicBlock(this->name("SWITCH.END"));
							basic_blocks.emplace_back(created_end_block);
							return created_end_block;
						}
					}();

					pir::BasicBlock::ID default_block = [&]() -> pir::BasicBlock::ID {
						if(else_index.has_value()){
							return basic_blocks[*else_index];

						}else if(switch_stmt.kind == sema::Switch::Kind::COMPLETE){
							const pir::BasicBlock::ID invalid_block =
								this->handler.createBasicBlock(this->name("SWITCH.INVALID"));

							this->handler.setTargetBasicBlock(invalid_block);
							this->create_panic("Invalid switch value");

							// probably not needed, but just in case
							this->handler.setTargetBasicBlock(start_basic_block);

							return invalid_block;
							
						}else{
							return *end_block;
						}
					}();

					auto cases = evo::SmallVector<pir::Switch::Case>();
					for(size_t i = 0; const sema::Switch::Case& switch_case : switch_stmt.cases){
						if(switch_stmt.cases.empty() == false){
							for(const sema::Expr value : switch_case.values){
								cases.emplace_back(this->get_expr_register(value), basic_blocks[i]);
							}
						}else{
							default_block = basic_blocks[i];
						}

						this->handler.setTargetBasicBlock(basic_blocks[i]);

						this->push_scope_level();

						for(const sema::Stmt& block_stmt : switch_case.stmtBlock){
							this->lower_stmt(block_stmt);
						}

						if(switch_case.stmtBlock.isTerminated() == false){
							this->output_defers_for_scope_level<DeferTarget::SCOPE_END>(this->scope_levels.back());
							this->handler.createJump(*end_block);
						}

						this->pop_scope_level();

						i += 1;
					}
					

					this->handler.setTargetBasicBlock(start_basic_block);
					this->handler.createSwitch(cond, std::move(cases), default_block);

					if(end_block.has_value()){
						this->handler.setTargetBasicBlock(*end_block);
					}
				}
			} break;

			case sema::Stmt::Kind::DEFER: {
				const sema::Defer& defer_stmt = this->context.getSemaBuffer().getDefer(stmt.deferID());
				this->get_current_scope_level().defers.emplace_back(
					stmt.deferID(),
					DeferItem::Targets{
						.on_scope_end = true,
						.on_return    = true,
						.on_error     = defer_stmt.isErrorDefer,
						.on_continue  = true,
						.on_break     = true,
					}
				);
			} break;

			case sema::Stmt::Kind::LIFETIME_START: {
				const sema::LifetimeStart& lifetime_start =
					this->context.getSemaBuffer().getLifetimeStart(stmt.lifetimeStartID());

				if(lifetime_start.target.is<sema::Expr>()){
					const sema::Expr target_expr = lifetime_start.target.as<sema::Expr>();

					switch(target_expr.kind()){
						case sema::Expr::Kind::ERROR_RETURN_PARAM: {
							const sema::ErrorReturnParam& err_ret_param = 
								this->context.getSemaBuffer().getErrorReturnParam(
									target_expr.errorReturnParamID()
								);

							this->get_current_scope_level().value_states[ManagedLifetimeErrorParam(err_ret_param.index)]
								= true;
						} break;

						case sema::Expr::Kind::BLOCK_EXPR_OUTPUT: {
							const pir::Expr pir_target_expr = this->get_expr_pointer(target_expr);
							this->get_current_scope_level().value_states[pir_target_expr] = true;
						} break;

						default: {
							const pir::Expr pir_target_expr = this->get_expr_pointer(target_expr);

							this->get_current_scope_level().value_states[pir_target_expr] = true;
						} break;
					}
				}else{
					this->get_current_scope_level().value_states[
						OpDeleteThisAccessor(lifetime_start.target.as<sema::OpDeleteThisAccessor>().abiIndex)
					] = true;
				}
			} break;

			case sema::Stmt::Kind::LIFETIME_END: {
				const sema::LifetimeEnd& lifetime_end =
					this->context.getSemaBuffer().getLifetimeEnd(stmt.lifetimeEndID());

				if(lifetime_end.target.is<sema::Expr>()){
					const sema::Expr target_expr = lifetime_end.target.as<sema::Expr>();

					switch(target_expr.kind()){
						case sema::Expr::Kind::ERROR_RETURN_PARAM: {
							const sema::ErrorReturnParam& err_ret_param =
								this->context.getSemaBuffer().getErrorReturnParam(
									target_expr.errorReturnParamID()
								);

							this->get_current_scope_level().value_states[ManagedLifetimeErrorParam(err_ret_param.index)]
								= false;
						} break;

						case sema::Expr::Kind::BLOCK_EXPR_OUTPUT: {
							const pir::Expr pir_target_expr = this->get_expr_pointer(target_expr);
							this->get_current_scope_level().value_states[pir_target_expr] = false;
						} break;

						default: {
							const pir::Expr pir_target_expr = this->get_expr_pointer(target_expr);

							this->get_current_scope_level().value_states[pir_target_expr] = false;
						} break;
					}

				}else{
					this->get_current_scope_level().value_states[
						OpDeleteThisAccessor(lifetime_end.target.as<sema::OpDeleteThisAccessor>().abiIndex)
					] = false;
				}

			} break;

			case sema::Stmt::Kind::UNUSED_EXPR: {
				const sema::UnusedExpr& unused_expr = this->context.getSemaBuffer().getUnusedExpr(stmt.unusedExprID());

				std::ignore = this->get_expr_pointer(unused_expr.expr);

				return; // skip end of stmt deletes to make sure expr it isn't deleted early
			} break;
		}


		for(const AutoDeleteTarget& expr_to_delete : this->end_of_stmt_deletes){
			this->delete_expr(expr_to_delete.expr, expr_to_delete.typeID);
		}
		this->end_of_stmt_deletes.clear();
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
					const pir::Expr zero = this->handler.createNumber(
						this->module.createUnsignedType(8), core::GenericInt::create<evo::byte>(0)
					);

					this->handler.createMemset(
						store_locations[0], zero, this->handler.getAlloca(store_locations[0]).type
					);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::NULL_VALUE: {
				evo::debugFatalBreak("Can't lower `null`");
			} break;

			case sema::Expr::Kind::INT_VALUE: {
				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;

				}else{
					const sema::IntValue& int_value = this->context.getSemaBuffer().getIntValue(expr.intValueID());
					const pir::Type value_type = this->get_type<false, false>(*int_value.typeID).type;
					const pir::Expr number = this->handler.createNumber(value_type, int_value.value);

					if constexpr(MODE == GetExprMode::REGISTER){
						return number;

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr alloca = this->handler.createAlloca(value_type, this->name(".NUMBER.ALLOCA"));
						this->handler.createStore(alloca, number);
						return alloca;

					}else{
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						this->handler.createStore(store_locations[0], number);
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::FLOAT_VALUE: {
				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;

				}else{
					const sema::FloatValue& float_value =
						this->context.getSemaBuffer().getFloatValue(expr.floatValueID());
					const pir::Type value_type = this->get_type<false, false>(*float_value.typeID).type;
					const pir::Expr number = this->handler.createNumber(value_type, float_value.value);

					if constexpr(MODE == GetExprMode::REGISTER){
						return number;

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr alloca = this->handler.createAlloca(value_type, this->name(".NUMBER.ALLOCA"));
						this->handler.createStore(alloca, number);
						return alloca;

					}else{
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						this->handler.createStore(store_locations[0], number);
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::BOOL_VALUE: {
				const sema::BoolValue& bool_value = this->context.getSemaBuffer().getBoolValue(expr.boolValueID());
				const pir::Expr boolean = this->handler.createBoolean(bool_value.value);

				if constexpr(MODE == GetExprMode::REGISTER){
					return boolean;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->handler.createAlloca(
						this->module.createBoolType(), this->name(".BOOLEAN.ALLOCA")
					);
					this->handler.createStore(alloca, boolean);
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->handler.createStore(store_locations[0], boolean);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::STRING_VALUE: {
				const sema::StringValue& string_value =
					this->context.getSemaBuffer().getStringValue(expr.stringValueID());

				const pir::GlobalVar::String::ID string_value_id = 
					this->module.createGlobalString(string_value.value + '\0');

				const pir::GlobalVar::ID string_id = this->module.createGlobalVar(
					std::format("PTHR.str{}", this->data.get_string_literal_id()),
					this->module.getGlobalString(string_value_id).type,
					pir::Linkage::PRIVATE,
					string_value_id,
					true
				);

				this->data.create_global_string(expr.stringValueID(), string_id);

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createGlobalValue(string_id);

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->handler.createAlloca(
						this->module.getGlobalString(string_value_id).type, this->name(".STR.ALLOCA")
					);
					this->handler.createStore(alloca, this->handler.createGlobalValue(string_id));
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->handler.createStore(store_locations[0], this->handler.createGlobalValue(string_id));
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::AGGREGATE_VALUE: {
				const sema::AggregateValue& aggregate =
					this->context.getSemaBuffer().getAggregateValue(expr.aggregateValueID());


				if constexpr(MODE != GetExprMode::DISCARD){
					const pir::Type pir_type = this->get_type<false, false>(aggregate.typeID).type;

					const pir::Expr initialization_target = [&](){
						if constexpr(MODE == GetExprMode::REGISTER || MODE == GetExprMode::POINTER){
							return this->handler.createAlloca(pir_type, ".AGGREGATE");
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

							const pir::Expr calc_ptr = this->handler.createCalcPtr(
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
							const pir::Expr calc_ptr = this->handler.createCalcPtr(
								initialization_target, pir_type, evo::SmallVector<pir::CalcPtr::Index>{0, i}
							);

							this->get_expr_store(value, calc_ptr);

							i += 1;
						}
					}
					



					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createLoad(initialization_target, pir_type);

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
				const pir::Type value_type = this->module.createUnsignedType(8);
				const pir::Expr number = this->handler.createNumber(
					value_type, core::GenericInt(8, uint64_t(char_value.value))
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return number;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->handler.createAlloca(value_type, this->name(".NUMBER.ALLOCA"));
					this->handler.createStore(alloca, number);
					return alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->handler.createStore(store_locations[0], number);
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
				return this->expr_copy<MODE>(
					copy_expr.expr, copy_expr.exprTypeID, copy_expr.isInitialization, store_locations
				);
			} break;

			case sema::Expr::Kind::MOVE: {
				const sema::Move& move_expr = this->context.getSemaBuffer().getMove(expr.moveID());
				return this->expr_move<MODE>(
					move_expr.expr, move_expr.exprTypeID, move_expr.isInitialization, store_locations
				);
			} break;

			case sema::Expr::Kind::FORWARD: {
				const sema::Forward& forward_expr = this->context.getSemaBuffer().getForward(expr.forwardID());

				if constexpr(MODE == GetExprMode::REGISTER || MODE == GetExprMode::STORE){
					const sema::Param& target_param = 
						this->context.getSemaBuffer().getParam(forward_expr.expr.paramID());
					const uint32_t in_param_index = *this->current_func_info->params[target_param.index].in_param_index;
					const bool param_is_copy = bool((this->in_param_bitmap >> in_param_index) & 1);

					if(param_is_copy){
						return this->expr_copy<MODE>(
							forward_expr.expr, forward_expr.exprTypeID, forward_expr.isInitialization, store_locations
						);
					}else{
						return this->expr_move<MODE>(
							forward_expr.expr, forward_expr.exprTypeID, forward_expr.isInitialization, store_locations
						);
					}

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->get_expr_pointer(forward_expr.expr);
					
				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::FUNC_CALL: {
				const sema::FuncCall& func_call = this->context.getSemaBuffer().getFuncCall(expr.funcCallID());

				if(func_call.target.is<IntrinsicFunc::Kind>()){
					return this->intrinsic_func_call_expr<MODE>(func_call, store_locations);

				}else if(func_call.target.is<sema::TemplateIntrinsicFuncInstantiation::ID>()){
					return this->template_intrinsic_func_call_expr<MODE>(func_call, store_locations);
				}

				const auto ssl = this->create_scoped_source_location(func_call.line, func_call.collumn);

				const Data::FuncInfo& target_func_info = this->data.get_func(func_call.target.as<sema::Func::ID>());

				const BaseType::Function& target_type = this->context.getTypeManager().getFunction(
					this->context.getSemaBuffer().getFunc(func_call.target.as<sema::Func::ID>()).typeID
				);


				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : func_call.args){
					if(target_func_info.params[i].is_copy()){
						args.emplace_back(this->get_expr_register(arg));

					}else if(target_type.params[i].kind == BaseType::Function::Param::Kind::IN){
						if(arg.kind() == sema::Expr::Kind::COPY){
							args.emplace_back(
								this->get_expr_pointer(this->context.getSemaBuffer().getCopy(arg.copyID()).expr)
							);

						}else if(arg.kind() == sema::Expr::Kind::MOVE){
							args.emplace_back(
								this->get_expr_pointer(this->context.getSemaBuffer().getMove(arg.moveID()).expr)
							);
							
						}else{
							args.emplace_back(this->get_expr_pointer(arg));
						}

					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}

				const uint32_t target_in_param_bitmap = this->calc_in_param_bitmap(target_type, func_call.args);


				if(target_type.hasNamedReturns || target_func_info.isImplicitRVO){
					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Type return_type =
							this->get_type<false, false>(target_type.returnTypes[0].asTypeID()).type;

						const pir::Expr return_alloc = this->handler.createAlloca(return_type);
						args.emplace_back(return_alloc);
						this->create_call_void(target_func_info.pir_ids[target_in_param_bitmap], std::move(args));

						return this->handler.createLoad(return_alloc, return_type);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Type return_type =
							this->get_type<false, false>(target_type.returnTypes[0].asTypeID()).type;
						
						const pir::Expr return_alloc = this->handler.createAlloca(return_type);
						args.emplace_back(return_alloc);
						this->create_call_void(target_func_info.pir_ids[target_in_param_bitmap], std::move(args));

						this->end_of_stmt_deletes.emplace_back(return_alloc, target_type.returnTypes[0].asTypeID());

						return return_alloc;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						for(pir::Expr store_location : store_locations){
							args.emplace_back(store_location);
						}
						this->create_call_void(target_func_info.pir_ids[target_in_param_bitmap], std::move(args));
						return std::nullopt;

					}else{
						const pir::Function& target_func = this->module.getFunction(
							target_func_info.pir_ids[target_in_param_bitmap].as<pir::Function::ID>()
						);

						const size_t current_num_args = args.size();
						for(size_t i = current_num_args; i < target_func.getParameters().size(); i+=1){
							const pir::Expr ret_alloca = this->handler.createAlloca(
								target_func_info.return_params[i - current_num_args].reference_type,
								this->name(".DISCARD")
							);
							args.emplace_back(ret_alloca);

							this->add_auto_delete_target(
								ret_alloca, target_type.returnTypes[i - current_num_args].asTypeID()
							);
						}

						this->create_call_void(target_func_info.pir_ids[target_in_param_bitmap], std::move(args));
						return std::nullopt;
					}

				}else{
					const pir::Expr call_return = this->create_call(
						target_func_info.pir_ids[target_in_param_bitmap],
						std::move(args),
						this->name("CALL.{}", this->mangle_name<true>(func_call.target.as<sema::Func::ID>()))
					);

					if constexpr(MODE == GetExprMode::REGISTER){
						return call_return;

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr return_alloca = this->handler.createAlloca(target_func_info.return_type);
						this->handler.createStore(return_alloca, call_return);
						this->end_of_stmt_deletes.emplace_back(return_alloca, target_type.returnTypes[0].asTypeID());
						return return_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						this->handler.createStore(store_locations.front(), call_return);
						return std::nullopt;

					}else{
						const pir::Expr discard_alloca =
							this->handler.createAlloca(target_func_info.return_type, this->name(".DISCARD"));
						this->handler.createStore(discard_alloca, call_return);

						this->add_auto_delete_target(discard_alloca, target_type.returnTypes[0].asTypeID());

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
					const pir::Expr alloca = this->handler.createAlloca(this->module.createPtrType());
					this->handler.createStore(alloca, address);
					return alloca;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->handler.createStore(store_locations.front(), address);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::CONVERSION_TO_OPTIONAL: {
				const sema::ConversionToOptional& conversion_to_optional = 
					this->context.getSemaBuffer().getConversionToOptional(expr.conversionToOptionalID());

				const TypeInfo& target_type =
					this->context.getTypeManager().getTypeInfo(conversion_to_optional.targetTypeID);

				if(target_type.isPointer()){
					return this->get_expr_impl<MODE>(conversion_to_optional.expr, store_locations);
				}


				const pir::Type target_pir_type =
					this->get_type<false, false>(conversion_to_optional.targetTypeID).type;


				const pir::Expr target = [&](){
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createAlloca(target_pir_type, ".CONVERSION_TO_OPTIONAL.ALLOCA");

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->handler.createAlloca(target_pir_type, ".CONVERSION_TO_OPTIONAL.ALLOCA");
						
					}else if constexpr(MODE == GetExprMode::STORE){
						return store_locations[0];

					}else{
						return this->handler.createAlloca(target_pir_type, ".DISCARD.CONVERSION_TO_OPTIONAL.ALLOCA");
					}
				}();


				const pir::Expr value_calc_ptr = this->handler.createCalcPtr(
					target,
					target_pir_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 0},
					this->name(".CONVERSION_TO_OPTIONAL.value_ptr")
				);

				const pir::Expr flag_calc_ptr = this->handler.createCalcPtr(
					target,
					target_pir_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".CONVERSION_TO_OPTIONAL.flag_ptr")
				);


				const auto create_copy_expr = [&](
					const sema::Expr& expr, TypeInfo::ID expr_type_id, bool is_initialization
				) -> void {
					if(is_initialization){
						std::ignore = this->expr_copy<GetExprMode::STORE>(expr, expr_type_id, true, value_calc_ptr);
					}else{
						const pir::BasicBlock::ID init_block =
							this->handler.createBasicBlock(this->name("CONVERSION_TO_OPTIONAL.INIT"));

						const pir::BasicBlock::ID assign_block =
							this->handler.createBasicBlock(this->name("CONVERSION_TO_OPTIONAL.ASSIGN"));

						const pir::BasicBlock::ID end_block =
							this->handler.createBasicBlock(this->name("CONVERSION_TO_OPTIONAL.END"));

						const pir::Expr flag_value = this->handler.createLoad(
							flag_calc_ptr,
							this->module.createBoolType(),
							this->name("CONVERSION_TO_OPTIONAL.flag")
						);

						this->handler.createBranch(flag_value, assign_block, init_block);

						this->handler.setTargetBasicBlock(init_block);
						std::ignore = this->expr_copy<GetExprMode::STORE>(expr, expr_type_id, true, value_calc_ptr);
						this->handler.createJump(end_block);

						this->handler.setTargetBasicBlock(assign_block);
						std::ignore = this->expr_copy<GetExprMode::STORE>(expr, expr_type_id, false, value_calc_ptr);
						this->handler.createJump(end_block);

						this->handler.setTargetBasicBlock(end_block);
					}
				};


				const auto create_move_expr = [&](
					const sema::Expr& expr, TypeInfo::ID expr_type_id, bool is_initialization
				) -> void {
					if(is_initialization){
						std::ignore = this->expr_move<GetExprMode::STORE>(expr, expr_type_id, true, value_calc_ptr);
					}else{
						const pir::BasicBlock::ID init_block =
							this->handler.createBasicBlock(this->name("CONVERSION_TO_OPTIONAL.INIT"));

						const pir::BasicBlock::ID assign_block =
							this->handler.createBasicBlock(this->name("CONVERSION_TO_OPTIONAL.ASSIGN"));

						const pir::BasicBlock::ID end_block =
							this->handler.createBasicBlock(this->name("CONVERSION_TO_OPTIONAL.END"));

						const pir::Expr flag_value = this->handler.createLoad(
							flag_calc_ptr,
							this->module.createBoolType(),
							this->name("CONVERSION_TO_OPTIONAL.flag")
						);

						this->handler.createBranch(flag_value, assign_block, init_block);

						this->handler.setTargetBasicBlock(init_block);
						std::ignore = this->expr_move<GetExprMode::STORE>(expr, expr_type_id, true, value_calc_ptr);
						this->handler.createJump(end_block);

						this->handler.setTargetBasicBlock(assign_block);
						std::ignore = this->expr_move<GetExprMode::STORE>(expr, expr_type_id, false, value_calc_ptr);
						this->handler.createJump(end_block);

						this->handler.setTargetBasicBlock(end_block);
					}
				};


				switch(conversion_to_optional.expr.kind()){
					case sema::Expr::Kind::COPY: {
						const sema::Copy& copy_expr =
							this->context.getSemaBuffer().getCopy(conversion_to_optional.expr.copyID());

						create_copy_expr(copy_expr.expr, copy_expr.exprTypeID, copy_expr.isInitialization);
					} break;

					case sema::Expr::Kind::MOVE: {
						const sema::Move& move_expr =
							this->context.getSemaBuffer().getMove(conversion_to_optional.expr.moveID());

						create_move_expr(move_expr.expr, move_expr.exprTypeID, move_expr.isInitialization);
					} break;

					case sema::Expr::Kind::FORWARD: {
						const sema::Forward& forward_expr =
							this->context.getSemaBuffer().getForward(conversion_to_optional.expr.forwardID());

						const sema::Param& target_param = 
							this->context.getSemaBuffer().getParam(forward_expr.expr.paramID());
						const uint32_t in_param_index =
							*this->current_func_info->params[target_param.index].in_param_index;
						const bool param_is_copy = bool((this->in_param_bitmap >> in_param_index) & 1);

						if(param_is_copy){
							create_copy_expr(forward_expr.expr, forward_expr.exprTypeID, forward_expr.isInitialization);
						}else{
							create_move_expr(forward_expr.expr, forward_expr.exprTypeID, forward_expr.isInitialization);
						}
					} break;
					
					default: {
						this->get_expr_store(conversion_to_optional.expr, value_calc_ptr);
					} break;
				}

				this->handler.createStore(flag_calc_ptr, this->handler.createBoolean(true));


				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createLoad(target, target_pir_type, this->name("CONVERSION_TO_OPTIONAL"));

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

					if(target_type_info.isPointer()){
						const pir::Expr lhs = this->get_expr_register(optional_null_check.expr);

						if(optional_null_check.equal){
							return this->handler.createIEq(
								lhs, this->handler.createNullptr(), this->name("OPT_IS_NULL")
							);
						}else{
							return this->handler.createINeq(
								lhs, this->handler.createNullptr(), this->name("OPT_ISNT_NULL")
							);
						}

					}else{
						evo::debugAssert(target_type_info.isOptionalNotPointer(), "Unknown expr to opt null check");

						const pir::Expr lhs = this->get_expr_pointer(optional_null_check.expr);

						const pir::Type target_type =
							this->get_type<false, false>(optional_null_check.targetTypeID).type;
						const pir::Expr calc_ptr = this->handler.createCalcPtr(
							lhs,
							target_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 1},
							this->name(".OPT_NULL_CHECK.flag_ptr")
						);
						const pir::Expr flag = this->handler.createLoad(
							calc_ptr, this->module.createBoolType(), this->name(".OPT_NULL_CHECK.flag")
						);

						if(optional_null_check.equal){
							return this->handler.createIEq(
								flag, this->handler.createBoolean(false), this->name("OPT_IS_NULL")
							);
						}else{
							return this->handler.createINeq(
								flag, this->handler.createBoolean(false), this->name("OPT_ISNT_NULL")
							);
						}

					}
				}();
				
				if constexpr(MODE == GetExprMode::REGISTER){
					return cmp;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->handler.createAlloca(this->module.createBoolType());
					this->handler.createStore(alloca, cmp);
					return alloca;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->handler.createStore(store_locations[0], cmp);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::OPTIONAL_EXTRACT: {
				const sema::OptionalExtract& optional_extract =
					this->context.getSemaBuffer().getOptionalExtract(expr.optionalExtractID());

				const TypeInfo& target_type_info =
					this->context.getTypeManager().getTypeInfo(optional_extract.targetTypeID);

				if(target_type_info.isPointer()){
					EVO_DEFER([&](){
						this->handler.createStore(
							this->get_expr_pointer(optional_extract.expr), this->handler.createNullptr()
						);
					});

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->get_expr_register(optional_extract.expr);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr pointer_alloca = this->handler.createAlloca(this->module.createPtrType());
						this->get_expr_store(optional_extract.expr, pointer_alloca);
						return pointer_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						this->get_expr_store(optional_extract.expr, store_locations[0]);
						return std::nullopt;

					}else{
						return std::nullopt;
					}

				}else if(target_type_info.isOptionalNotPointer()){
					const pir::Expr lhs = this->get_expr_pointer(optional_extract.expr);
					const pir::Type target_type = this->get_type<false, false>(optional_extract.targetTypeID).type;

					const pir::Expr held_value = this->handler.createCalcPtr(
						lhs, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 0}, this->name(".EXTRACT_OPT.value")
					);
					const pir::Expr flag = this->handler.createCalcPtr(
						lhs, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 1}, this->name(".EXTRACT_OPT.flag")
					);

					const TypeInfo::ID held_type_id = this->context.type_manager.getOrCreateTypeInfo(
						this->context.getTypeManager().getTypeInfo(optional_extract.targetTypeID)
							.copyWithPoppedQualifier()
					);
					const std::optional<pir::Expr> output = 
						this->expr_move<MODE>(held_value, held_type_id, true, store_locations);

					this->handler.createStore(flag, this->handler.createBoolean(false));

					return output;

				}else{
					EVO_DEFER([&](){
						const pir::Type interface_ptr_type =
							this->data.getInterfacePtrType(this->module, this->context.getSourceManager()).pir_type;
						const pir::Expr calc_ptr = this->handler.createCalcPtr(
							this->get_expr_pointer(optional_extract.expr),
							interface_ptr_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".EXTRACT_OPT.interface_pointer")
						);
						this->handler.createStore(calc_ptr, this->handler.createNullptr());
					});

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->get_expr_register(optional_extract.expr);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr pointer_alloca = this->handler.createAlloca(this->module.createPtrType());
						this->get_expr_store(optional_extract.expr, pointer_alloca);
						return pointer_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						this->get_expr_store(optional_extract.expr, store_locations[0]);
						return std::nullopt;

					}else{
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::DEREF: {
				const sema::Deref& deref = this->context.getSemaBuffer().getDeref(expr.derefID());

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createLoad(
						this->get_expr_register(deref.expr),
						this->get_type<false, false>(deref.targetTypeID).type,
						"DEREF"
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->get_expr_register(deref.expr);
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					this->handler.createMemcpy(
						store_locations.front(),
						this->get_expr_register(deref.expr),
						this->get_type<false, false>(deref.targetTypeID).type
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
					const pir::Type target_pir_type = this->get_type<false, false>(unwrap.targetTypeID).type;

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createCalcPtr(
							this->get_expr_pointer(unwrap.expr),
							target_pir_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name("UNWRAP")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr unwrap_alloca = this->handler.createAlloca(this->module.createPtrType());

						const pir::Expr calc_ptr = this->handler.createCalcPtr(
							this->get_expr_pointer(unwrap.expr),
							target_pir_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".UNWRAP")
						);

						this->handler.createStore(unwrap_alloca, calc_ptr);
						return unwrap_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						const pir::Expr calc_ptr = this->handler.createCalcPtr(
							this->get_expr_pointer(unwrap.expr),
							target_pir_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".UNWRAP")
						);

						this->handler.createStore(store_locations[0], calc_ptr);
						return std::nullopt;

					}else{
						this->get_expr_discard(unwrap.expr);
						return std::nullopt;
					}
				} 
			} break;

			case sema::Expr::Kind::ACCESSOR: {
				const sema::Accessor& accessor = this->context.getSemaBuffer().getAccessor(expr.accessorID());

				const pir::Type target_pir_type = this->get_type<false, false>(accessor.targetTypeID).type;

				const TypeInfo& target_type = this->context.getTypeManager().getTypeInfo(accessor.targetTypeID);
				const BaseType::Struct& target_struct_type = this->context.getTypeManager().getStruct(
					target_type.baseTypeID().structID()
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Expr calc_ptr = this->handler.createCalcPtr(
						this->get_expr_pointer(accessor.target),
						target_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, int64_t(accessor.memberABIIndex)},
						this->name("ACCESSOR")
					);

					return this->handler.createLoad(
						calc_ptr,
						this->get_type<false, false>(
							target_struct_type.memberVars[size_t(accessor.memberABIIndex)].typeID
						).type
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->handler.createCalcPtr(
						this->get_expr_pointer(accessor.target),
						target_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, int64_t(accessor.memberABIIndex)},
						this->name("ACCESSOR")
					);

				}else if constexpr(MODE == GetExprMode::STORE){
					const pir::Expr calc_ptr = this->handler.createCalcPtr(
						this->get_expr_pointer(accessor.target),
						target_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, int64_t(accessor.memberABIIndex)},
						this->name(".ACCESSOR")
					);

					this->handler.createMemcpy(
						store_locations[0],
						calc_ptr,
						this->get_type<false, false>(
							target_struct_type.memberVars[size_t(accessor.memberABIIndex)].typeID
						).type
					);
					return std::nullopt;

				}else{
					this->get_expr_discard(accessor.target);
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::LOGICAL_AND: {
				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;

				}else{
					const sema::LogicalAnd& logical_and =
						this->context.getSemaBuffer().getLogicalAnd(expr.logicalAndID());


					const pir::BasicBlock::ID end_block = this->handler.createBasicBlockInline("LOGICAL_AND.END");
					const pir::BasicBlock::ID rhs_block = this->handler.createBasicBlockInline("LOGICAL_AND.RHS");

					const pir::Expr lhs_expr = this->get_expr_register(logical_and.lhs);
					const pir::BasicBlock::ID lhs_end_block = this->handler.getTargetBasicBlock().getID();
					this->handler.createBranch(lhs_expr, rhs_block, end_block);

					this->handler.setTargetBasicBlock(rhs_block);
					const pir::Expr rhs_expr = this->get_expr_register(logical_and.rhs);
					const pir::BasicBlock::ID rhs_end_block = this->handler.getTargetBasicBlock().getID();
					this->handler.createJump(end_block);

					this->handler.setTargetBasicBlock(end_block);


					std::string output_expr_name = [&](){
						if constexpr(MODE == GetExprMode::STORE){
							return this->name(".LOGICAL_AND");
						}else{
							return this->name("LOGICAL_AND");
						}
					}();
					const pir::Expr output_expr = this->handler.createPhi(
						evo::SmallVector<pir::Phi::Predecessor>{
							pir::Phi::Predecessor(lhs_end_block, this->handler.createBoolean(false)),
							pir::Phi::Predecessor(rhs_end_block, rhs_expr)
						},
						std::move(output_expr_name)
					);

					if constexpr(MODE == GetExprMode::REGISTER){
						return output_expr;

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr output_alloca =
							this->handler.createAlloca(this->module.createBoolType(), this->name("LOGICAL_AND"));
						this->handler.createStore(output_alloca, output_expr);
						return output_alloca;
						
					}else{
						this->handler.createStore(store_locations[0], output_expr);
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::LOGICAL_OR: {
				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;

				}else{
					const sema::LogicalOr& logical_or =
						this->context.getSemaBuffer().getLogicalOr(expr.logicalOrID());

					const pir::BasicBlock::ID end_block = this->handler.createBasicBlockInline("LOGICAL_OR.END");
					const pir::BasicBlock::ID rhs_block = this->handler.createBasicBlockInline("LOGICAL_OR.RHS");

					const pir::Expr lhs_expr = this->get_expr_register(logical_or.lhs);
					const pir::BasicBlock::ID lhs_end_block = this->handler.getTargetBasicBlock().getID();
					this->handler.createBranch(lhs_expr, end_block, rhs_block);

					this->handler.setTargetBasicBlock(rhs_block);
					const pir::Expr rhs_expr = this->get_expr_register(logical_or.rhs);
					const pir::BasicBlock::ID rhs_end_block = this->handler.getTargetBasicBlock().getID();
					this->handler.createJump(end_block);

					this->handler.setTargetBasicBlock(end_block);

					this->handler.setTargetBasicBlock(end_block);
					std::string output_expr_name = [&](){
						if constexpr(MODE == GetExprMode::STORE){
							return this->name(".LOGICAL_OR");
						}else{
							return this->name("LOGICAL_OR");
						}
					}();
					const pir::Expr output_expr = this->handler.createPhi(
						evo::SmallVector<pir::Phi::Predecessor>{
							pir::Phi::Predecessor(lhs_end_block, this->handler.createBoolean(true)),
							pir::Phi::Predecessor(rhs_end_block, rhs_expr)
						},
						std::move(output_expr_name)
					);

					if constexpr(MODE == GetExprMode::REGISTER){
						return output_expr;

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr output_alloca = 
							this->handler.createAlloca(this->module.createBoolType(), this->name("LOGICAL_OR"));
						this->handler.createStore(output_alloca, output_expr);
						return output_alloca;
						
					}else{
						this->handler.createStore(store_locations[0], output_expr);
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::UNION_ACCESSOR: {
				const sema::UnionAccessor& union_accessor = 
					this->context.getSemaBuffer().getUnionAccessor(expr.unionAccessorID());

				if constexpr(MODE == GetExprMode::DISCARD){
					this->get_expr_discard(union_accessor.target);
					return std::nullopt;

				}else{
					const TypeInfo& target_type = 
						this->context.getTypeManager().getTypeInfo(union_accessor.targetTypeID);

					const BaseType::Union& target_union_type = this->context.getTypeManager().getUnion(
						target_type.baseTypeID().unionID()
					);


					const pir::Expr data_ptr = [&](){
						if(target_union_type.isUntagged){
							return this->get_expr_pointer(union_accessor.target);

						}else{
							return this->handler.createCalcPtr(
								this->get_expr_pointer(union_accessor.target),
								this->get_type<false, false>(target_type.baseTypeID()).type,
								evo::SmallVector<pir::CalcPtr::Index>{0, 0},
								this->name(".UNION_DATA")
							);
						}
					}();

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createLoad(
							data_ptr,
							this->get_type<false, false>(
								target_union_type.fields[union_accessor.fieldIndex].typeID.asTypeID()
							).type,
							this->name("UNION_ACCESSOR")
						);
						
					}else if constexpr(MODE == GetExprMode::POINTER){
						return data_ptr;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						return this->handler.createMemcpy(
							store_locations[0],
							data_ptr,
							this->get_type<false, false>(
								target_union_type.fields[union_accessor.fieldIndex].typeID.asTypeID()
							).type
						);
					}
				}
			} break;

			case sema::Expr::Kind::BLOCK_EXPR: {
				const sema::BlockExpr& block_expr = this->context.getSemaBuffer().getBlockExpr(expr.blockExprID());

				const std::string_view label = this->current_source->getTokenBuffer()[block_expr.label].getString();

				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock("BLOCK_EXPR.END");

				if constexpr(MODE == GetExprMode::REGISTER || MODE == GetExprMode::POINTER){
					auto label_output_locations = evo::SmallVector<pir::Expr>();
					const pir::Type output_type = this->get_type<false, false>(block_expr.outputs[0].typeID).type;
					label_output_locations.emplace_back(
						this->handler.createAlloca(output_type, this->name(".BLOCK_EXPR.OUTPUT.ALLOCA"))
					);

					this->get_current_scope_level().defers.emplace_back(
						AutoDeleteManagedLifetimeTarget(label_output_locations[0], block_expr.outputs[0].typeID),
						DeferItem::Targets{
							.on_scope_end = false,
							.on_return    = true,
							.on_error     = true,
							.on_continue  = false,
							.on_break     = false,
						}
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

					for(size_t i = 0; const pir::Expr store_location : store_locations){
						this->get_current_scope_level().defers.emplace_back(
							AutoDeleteManagedLifetimeTarget(store_location, block_expr.outputs[i].typeID),
							DeferItem::Targets{
								.on_scope_end = false,
								.on_return    = true,
								.on_error     = true,
								.on_continue  = false,
								.on_break     = false,
							}
						);
					
						i += 1;
					}
				}
				

				for(const sema::Stmt& stmt : block_expr.block){
					this->lower_stmt(stmt);
				}

				this->handler.setTargetBasicBlock(end_block);

				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Expr output = this->get_current_scope_level().label_output_locations[0];
					this->pop_scope_level();
					return this->handler.createLoad(
						output, this->handler.getAlloca(output).type, this->name("BLOCK_EXPR.OUTPUT")
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

					const pir::Type interface_ptr_type =
						this->data.getInterfacePtrType(this->module, this->context.getSourceManager()).pir_type;


					const pir::Expr vtable_value = [&]() -> pir::Expr {
						const BaseType::Interface& interface_type =
							this->context.getTypeManager().getInterface(make_interface_ptr.interfaceID);
						

						if(interface_type.methods.size() == 1){
							const pir::Function::ID vtable_method = this->data.get_single_method_vtable(
								Data::VTableID(make_interface_ptr.interfaceID, make_interface_ptr.implTypeID)
							);

							return this->handler.createFunctionPointer(vtable_method);

						}else{
							const pir::GlobalVar::ID vtable = this->data.get_vtable(
								Data::VTableID(make_interface_ptr.interfaceID, make_interface_ptr.implTypeID)
							);

							return this->handler.createGlobalValue(vtable);
						}
					}();


					const pir::Expr target = [&](){
						if constexpr(MODE == GetExprMode::STORE){
							return store_locations[0];
						}else{
							return this->handler.createAlloca(interface_ptr_type);
						}
					}();


					const pir::Expr value_ptr = this->handler.createCalcPtr(
						target,
						interface_ptr_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 0},
						this->name(".MAKE_INTERFACE_PTR.VALUE")
					);
					this->handler.createStore(value_ptr, this->get_expr_pointer(make_interface_ptr.expr));

					const pir::Expr vtable_ptr = this->handler.createCalcPtr(
						target,
						interface_ptr_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 1},
						this->name(".MAKE_INTERFACE_PTR.VTABLE")
					);
					this->handler.createStore(vtable_ptr, vtable_value);

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createLoad(target, interface_ptr_type);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return target;

					}else if constexpr(MODE == GetExprMode::STORE){
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::INTERFACE_PTR_EXTRACT_THIS: {
				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;

				}else{
					const sema::InterfacePtrExtractThis& interface_ptr_extract_this =
						this->context.getSemaBuffer().getInterfacePtrExtractThis(expr.interfacePtrExtractThisID());

					const pir::Type interface_ptr_type =
						this->data.getInterfacePtrType(this->module, this->context.getSourceManager()).pir_type;

					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Expr ptr = this->handler.createCalcPtr(
							this->get_expr_pointer(interface_ptr_extract_this.expr),
							interface_ptr_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".INTERFACE_PTR.this")
						);

						return this->handler.createLoad(
							ptr, this->module.createPtrType(), this->name("INTERFACE_PTR.this")
						);
						
					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->handler.createCalcPtr(
							this->get_expr_pointer(interface_ptr_extract_this.expr),
							interface_ptr_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name("INTERFACE_PTR.this")
						);

					}else{
						const pir::Expr ptr = this->handler.createCalcPtr(
							this->get_expr_pointer(interface_ptr_extract_this.expr),
							interface_ptr_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".INTERFACE_PTR.this")
						);

						this->handler.createMemcpy(store_locations[0], ptr, this->module.createPtrType());
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
					if(target_func_type.hasNamedReturns){ return this->module.createVoidType(); }

					return this->get_type<false, false>(target_func_type.returnTypes[0].asTypeID()).type;
				}();

				auto param_types = evo::SmallVector<pir::Type>();
				for(const BaseType::Function::Param& param : target_func_type.params){
					if(param.shouldCopy){
						param_types.emplace_back(this->get_type<false, false>(param.typeID).type);
					}else{
						param_types.emplace_back(this->module.createPtrType());
					}
				}
				if(target_func_type.hasNamedReturns){
					for(size_t i = 0; i < target_func_type.returnTypes.size(); i+=1){
						param_types.emplace_back(this->module.createPtrType());
					}
				}


				const pir::Type func_pir_type = this->module.getOrCreateFunctionType(
					std::move(param_types), pir::CallingConvention::FAST, return_type
				);


				///////////////////////////////////
				// get func pointer

				const pir::Expr target_interface_ptr = this->get_expr_pointer(interface_call.value);
				const pir::Type interface_ptr_type =
					this->data.getInterfacePtrType(this->module, this->context.getSourceManager()).pir_type;

				const pir::Expr vtable_ptr = this->handler.createCalcPtr(
					target_interface_ptr,
					interface_ptr_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".VTABLE.PTR")
				);
				const pir::Expr vtable = this->handler.createLoad(
					vtable_ptr, this->module.createPtrType(), this->name(".VTABLE")
				);


				const pir::Expr target_func = [&]() -> pir::Expr {
					const BaseType::Interface& interface_type =
						this->context.getTypeManager().getInterface(interface_call.interfaceID);

					if(interface_type.methods.size() == 1){
						return vtable;

					}else{
						const pir::Expr target_func_ptr = this->handler.createCalcPtr(
							vtable,
							this->module.createPtrType(),
							evo::SmallVector<pir::CalcPtr::Index>{interface_call.vtableFuncIndex},
							this->name(".VTABLE.FUNC.PTR")
						);
						return this->handler.createLoad(
							target_func_ptr, this->module.createPtrType(), this->name(".VTABLE.FUNC")
						);
					}
				}();


				///////////////////////////////////
				// make call

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : interface_call.args){
					EVO_DEFER([&](){ i += 1; });

					if(i == 0 && arg.kind() == sema::Expr::Kind::DEREF){
						const sema::Deref& deref = this->context.getSemaBuffer().getDeref(arg.derefID());

						if(deref.expr.kind() == sema::Expr::Kind::INTERFACE_PTR_EXTRACT_THIS){
							const sema::InterfacePtrExtractThis& interface_ptr_extract_this = 
								this->context
									.getSemaBuffer()
									.getInterfacePtrExtractThis(deref.expr.interfacePtrExtractThisID());

							if(interface_ptr_extract_this.expr == interface_call.value){
								const pir::Expr this_ptr = this->handler.createCalcPtr(
									target_interface_ptr,
									interface_ptr_type,
									evo::SmallVector<pir::CalcPtr::Index>{0, 0},
									this->name(".INTERFACE_PTR.this_ptr")
								);

								args.emplace_back(
									this->handler.createLoad(
										this_ptr, this->module.createPtrType(), this->name(".INTERFACE_PTR.this")
									)
								);

								continue;
							}
						}
					}


					if(target_func_type.params[i].shouldCopy){
						args.emplace_back(this->get_expr_register(arg));
					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}
				}


				if(target_func_type.hasNamedReturns){
					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Type actual_return_type =
							this->get_type<false, false>(target_func_type.returnTypes[0].asTypeID()).type;

						const pir::Expr return_alloc = this->handler.createAlloca(
							actual_return_type, this->name(".INTERFACE_CALL.ALLOCA")
						);
						args.emplace_back(return_alloc);
						this->handler.createCallVoid(target_func, func_pir_type, std::move(args));

						return this->handler.createLoad(return_alloc, actual_return_type, this->name("INTERFACE_CALL"));

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Type actual_return_type =
							this->get_type<false, false>(target_func_type.returnTypes[0].asTypeID()).type;

						const pir::Expr return_alloc = this->handler.createAlloca(
							actual_return_type, this->name("INTERFACE_CALL.ALLOCA")
						);
						args.emplace_back(return_alloc);
						this->handler.createCallVoid(target_func, func_pir_type, std::move(args));

						return return_alloc;
						
					}else if constexpr(MODE == GetExprMode::STORE || MODE == GetExprMode::DISCARD){
						for(pir::Expr store_location : store_locations){
							args.emplace_back(store_location);
						}
						this->handler.createCallVoid(target_func, func_pir_type, std::move(args));
						return std::nullopt;
					}

				}else{
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createCall(
							target_func, func_pir_type, std::move(args), this->name("INTERFACE_CALL")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr call_return = this->handler.createCall(
							target_func, func_pir_type, std::move(args), this->name(".INTERFACE_CALL")
						);

						const pir::Expr alloca = this->handler.createAlloca(
							return_type, this->name("INTERFACE_CALL.ALLOCA")
						);
						this->handler.createStore(alloca, call_return);
						return alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						const pir::Expr call_return = this->handler.createCall(
							target_func, func_pir_type, std::move(args), this->name(".INTERFACE_CALL")
						);
						this->handler.createStore(store_locations.front(), call_return);
						return std::nullopt;

					}else{
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::INDEXER: {
				const sema::Indexer& indexer = this->context.getSemaBuffer().getIndexer(expr.indexerID());

				const pir::Expr target = this->get_expr_pointer(indexer.target);

				const pir::Type type_usize = this->get_type<false, false>(TypeManager::getTypeUSize()).type;

				auto indices = evo::SmallVector<pir::CalcPtr::Index>();
				indices.reserve(indexer.indices.size() + 1);
				indices.emplace_back(
					pir::CalcPtr::Index(this->handler.createNumber(type_usize, core::GenericInt::create<uint64_t>(0)))
				);
				for(const sema::Expr& index : indexer.indices){
					indices.emplace_back(this->get_expr_register(index));
				}

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createCalcPtr(
						target,
						this->get_type<false, false>(indexer.targetTypeID).type,
						std::move(indices),
						this->name("INDEXER")
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr indexer_alloca = this->handler.createAlloca(
						this->module.createPtrType(), this->name(".INDEXER.ALLOCA")
					);

					const pir::Expr calc_ptr = this->handler.createCalcPtr(
						target,
						this->get_type<false, false>(indexer.targetTypeID).type,
						std::move(indices),
						this->name(".INDEXER")
					);
					this->handler.createStore(indexer_alloca, calc_ptr);

					return indexer_alloca;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::Expr calc_ptr = this->handler.createCalcPtr(
						target,
						this->get_type<false, false>(indexer.targetTypeID).type,
						std::move(indices),
						this->name(".INDEXER")
					);

					this->handler.createStore(store_locations[0], calc_ptr);
					return std::nullopt;

				}else{
					return std::nullopt;
				}

			} break;

			case sema::Expr::Kind::DEFAULT_NEW: {
				const sema::DefaultNew& default_new =
					this->context.getSemaBuffer().getDefaultNew(expr.defaultNewID());

				return this->default_new_expr<MODE>(
					default_new.targetTypeID, default_new.isInitialization, store_locations
				);
			} break;

			case sema::Expr::Kind::INIT_ARRAY_REF: {
				const sema::InitArrayRef& init_array_ref = 
					this->context.getSemaBuffer().getInitArrayRef(expr.initArrayRefID());


				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;

				}else{
					const uint64_t num_bits_ptr = this->context.getTypeManager().numBitsOfPtr();

					const pir::Type array_ref_type = this->data.getArrayRefType(
						this->module,
						this->context,
						init_array_ref.targetTypeID,
						[&](TypeInfo::ID id){ return *this->get_type<false, true>(id).meta_type_id; }
					).pir_type;

					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Expr array_ref_alloca =
							this->handler.createAlloca(array_ref_type, this->name(".ARRAY_REF.ALLOCA"));

						const pir::Expr data_ptr = this->handler.createCalcPtr(
							array_ref_alloca,
							array_ref_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".ARRAY_REF.ARRAY_PTR")
						);
						this->get_expr_store(init_array_ref.expr, data_ptr);


						for(uint32_t i = 1; evo::Variant<uint64_t, sema::Expr> dimension : init_array_ref.dimensions){
							if(dimension.is<uint64_t>()){
								const pir::Expr dimension_expr = this->handler.createNumber(
									this->module.createUnsignedType(uint32_t(num_bits_ptr)),
									core::GenericInt(unsigned(num_bits_ptr), dimension.as<uint64_t>())
								);

								const pir::Expr dimension_ptr = this->handler.createCalcPtr(
									array_ref_alloca,
									array_ref_type,
									evo::SmallVector<pir::CalcPtr::Index>{0, i},
									this->name(".ARRAY_REF.DIMENSION_{}", i - 1)
								);
								this->handler.createStore(dimension_ptr, dimension_expr);

							}else{
								const pir::Expr dimension_ptr = this->handler.createCalcPtr(
									array_ref_alloca,
									array_ref_type,
									evo::SmallVector<pir::CalcPtr::Index>{0, i},
									this->name(".ARRAY_REF.DIMENSION_{}", i - 1)
								);
								this->get_expr_store(dimension.as<sema::Expr>(), dimension_ptr);
							}

							i += 1;
						}

						return this->handler.createLoad(array_ref_alloca, array_ref_type, this->name("ARRAY_REF"));

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr array_ref_alloca =
							this->handler.createAlloca(array_ref_type, this->name("ARRAY_REF"));

						const pir::Expr data_ptr = this->handler.createCalcPtr(
							array_ref_alloca,
							array_ref_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".ARRAY_REF.ARRAY_PTR")
						);
						this->get_expr_store(init_array_ref.expr, data_ptr);


						for(uint32_t i = 1; evo::Variant<uint64_t, sema::Expr> dimension : init_array_ref.dimensions){
							if(dimension.is<uint64_t>()){
								const pir::Expr dimension_expr = this->handler.createNumber(
									this->module.createUnsignedType(uint32_t(num_bits_ptr)),
									core::GenericInt(unsigned(num_bits_ptr), dimension.as<uint64_t>())
								);

								const pir::Expr dimension_ptr = this->handler.createCalcPtr(
									array_ref_alloca,
									array_ref_type,
									evo::SmallVector<pir::CalcPtr::Index>{0, i},
									this->name(".ARRAY_REF.DIMENSION_{}", i - 1)
								);
								this->handler.createStore(dimension_ptr, dimension_expr);


							}else{
								const pir::Expr dimension_ptr = this->handler.createCalcPtr(
									array_ref_alloca,
									array_ref_type,
									evo::SmallVector<pir::CalcPtr::Index>{0, i},
									this->name(".ARRAY_REF.DIMENSION_{}", i - 1)
								);
								this->get_expr_store(dimension.as<sema::Expr>(), dimension_ptr);
							}

							i += 1;
						}

						return array_ref_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						const pir::Expr data_ptr = this->handler.createCalcPtr(
							store_locations[0],
							array_ref_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".ARRAY_REF.ARRAY_PTR")
						);
						this->get_expr_store(init_array_ref.expr, data_ptr);

						for(uint32_t i = 1; evo::Variant<uint64_t, sema::Expr> dimension : init_array_ref.dimensions){
							if(dimension.is<uint64_t>()){
								const pir::Expr dimension_expr = this->handler.createNumber(
									this->module.createUnsignedType(uint32_t(num_bits_ptr)),
									core::GenericInt(unsigned(num_bits_ptr), dimension.as<uint64_t>())
								);

								const pir::Expr dimension_ptr = this->handler.createCalcPtr(
									store_locations[0],
									array_ref_type,
									evo::SmallVector<pir::CalcPtr::Index>{0, i},
									this->name(".ARRAY_REF.DIMENSION_{}", i - 1)
								);
								this->handler.createStore(dimension_ptr, dimension_expr);

							}else{
								const pir::Expr dimension_ptr = this->handler.createCalcPtr(
									store_locations[0],
									array_ref_type,
									evo::SmallVector<pir::CalcPtr::Index>{0, i},
									this->name(".ARRAY_REF.DIMENSION_{}", i - 1)
								);
								this->get_expr_store(dimension.as<sema::Expr>(), dimension_ptr);
							}

							i += 1;
						}

						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::ARRAY_REF_INDEXER: {
				const sema::ArrayRefIndexer& array_ref_indexer =
					this->context.getSemaBuffer().getArrayRefIndexer(expr.arrayRefIndexerID());

				const BaseType::ArrayRef& array_ref_type =
					this->context.getTypeManager().getArrayRef(array_ref_indexer.targetTypeID);


				const pir::Type pir_array_ref_type = this->data.getArrayRefType(
					this->module,
					this->context,
					array_ref_indexer.targetTypeID,
					[&](TypeInfo::ID id){ return *this->get_type<false, true>(id).meta_type_id; }
				).pir_type;

				const pir::Expr target_array_ref = this->get_expr_pointer(array_ref_indexer.target);

				const pir::Expr get_arr_calc_ptr = this->handler.createCalcPtr(
					target_array_ref,
					pir_array_ref_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 0},
					this->name(".ARRAY_REF.PTR_CALC")
				);

				const pir::Expr target = this->handler.createLoad(
					get_arr_calc_ptr, this->module.createPtrType(), this->name(".ARRAY_REF.PTR")
				);

				const pir::Type type_usize = this->get_type<false, false>(TypeManager::getTypeUSize()).type;


				uint32_t ref_length_index = uint32_t(array_ref_type.getNumRefPtrs());

				pir::Expr index = this->get_expr_register(array_ref_indexer.indices.back());
				auto sub_array_width = std::optional<pir::Expr>();

				for(size_t i = array_ref_indexer.indices.size() - 1; i >= 1; i-=1){
					const pir::Expr length_num = [&](){
						if(array_ref_type.dimensions[i].isPtr()){
							const pir::Expr length_load = this->handler.createLoad(
								this->handler.createCalcPtr(
									target_array_ref,
									pir_array_ref_type,
									evo::SmallVector<pir::CalcPtr::Index>{0, ref_length_index}
								),
								type_usize
							);

							ref_length_index -= 1;

							return length_load;

						}else{
							return this->handler.createNumber(
								type_usize,
								core::GenericInt(
									unsigned(this->context.getTypeManager().numBitsOfPtr()),
									array_ref_type.dimensions[i].length()
								)
							);
						}
					}();


					if(sub_array_width.has_value()){
						sub_array_width = this->handler.createMul(*sub_array_width, length_num, false, true);
					}else{
						sub_array_width = length_num;
					}

					index = this->handler.createAdd(
						index,
						this->handler.createMul(
							*sub_array_width,
							this->get_expr_register(array_ref_indexer.indices[i - 1]),
							false,
							true
						),
						false,
						true
					);
				}


				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createCalcPtr(
						target,
						this->get_type<false, false>(array_ref_type.elementTypeID).type,
						evo::SmallVector<pir::CalcPtr::Index>{index},
						this->name("ARRAY_REF_INDEXER")
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr array_ref_indexer_alloca = this->handler.createAlloca(
						this->module.createPtrType(), this->name(".ARRAY_REF_INDEXER.ALLOCA")
					);

					const pir::Expr calc_ptr = this->handler.createCalcPtr(
						target,
						this->get_type<false, false>(array_ref_type.elementTypeID).type,
						evo::SmallVector<pir::CalcPtr::Index>{index},
						this->name(".ARRAY_REF_INDEXER")
					);
					this->handler.createStore(array_ref_indexer_alloca, calc_ptr);

					return array_ref_indexer_alloca;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::Expr calc_ptr = this->handler.createCalcPtr(
						target,
						this->get_type<false, false>(array_ref_type.elementTypeID).type,
						evo::SmallVector<pir::CalcPtr::Index>{index},
						this->name(".ARRAY_REF_INDEXER")
					);

					this->handler.createStore(store_locations[0], calc_ptr);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::ARRAY_REF_SIZE: {
				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;

				}else{
					const sema::ArrayRefSize& array_ref_size =
						this->context.getSemaBuffer().getArrayRefSize(expr.arrayRefSizeID());

					const BaseType::ArrayRef& array_ref_type =
						this->context.getTypeManager().getArrayRef(array_ref_size.targetTypeID);


					const pir::Type pir_array_ref_type = this->data.getArrayRefType(
						this->module,
						this->context,
						array_ref_size.targetTypeID,
						[&](TypeInfo::ID id){ return *this->get_type<false, true>(id).meta_type_id; }
					).pir_type;


					const pir::Expr target_array_ref = this->get_expr_pointer(array_ref_size.target);

					const pir::Type type_usize = this->get_type<false, false>(TypeManager::getTypeUSize()).type;


					uint32_t ref_length_index = 0;
					auto size_expr = std::optional<pir::Expr>();

					for(const BaseType::ArrayRef::Dimension& dimension : array_ref_type.dimensions){
						const pir::Expr length_num = [&](){
							if(dimension.isPtr()){
								const pir::Expr length_load = this->handler.createLoad(
									this->handler.createCalcPtr(
										target_array_ref,
										pir_array_ref_type,
										evo::SmallVector<pir::CalcPtr::Index>{0, ref_length_index + 1}
									),
									type_usize
								);

								ref_length_index += 1;

								return length_load;

							}else{
								return this->handler.createNumber(
									type_usize,
									core::GenericInt(
										unsigned(this->context.getTypeManager().numBitsOfPtr()), dimension.length()
									)
								);
							}
						}();

						if(size_expr.has_value()){
							*size_expr = this->handler.createMul(*size_expr, length_num, true, false);
						}else{
							size_expr = length_num;
						}
					}

					if constexpr(MODE == GetExprMode::REGISTER){
						return *size_expr;

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr size_alloca = 
							this->handler.createAlloca(type_usize, this->name("ARRAY_REF_SIZE"));
						this->handler.createStore(size_alloca, *size_expr);
						return size_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						this->handler.createStore(store_locations[0], *size_expr);
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::ARRAY_REF_DIMENSIONS: {
				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;
				}else{
					const sema::ArrayRefDimensions& array_ref_dimensions =
						this->context.getSemaBuffer().getArrayRefDimensions(expr.arrayRefDimensionsID());

					const BaseType::ArrayRef& array_ref_type =
						this->context.getTypeManager().getArrayRef(array_ref_dimensions.targetTypeID);

					const pir::Type pir_array_ref_type = this->data.getArrayRefType(
						this->module,
						this->context,
						array_ref_dimensions.targetTypeID,
						[&](TypeInfo::ID id){ return *this->get_type<false, true>(id).meta_type_id; }
					).pir_type;


					const pir::Expr target_array_ref = this->get_expr_pointer(array_ref_dimensions.target);

					const pir::Type type_usize = this->get_type<false, false>(TypeManager::getTypeUSize()).type;

					const pir::Type return_type =
						this->module.getOrCreateArrayType(type_usize, array_ref_type.dimensions.size());


					const pir::Expr output_memory = [&](){
						if constexpr(MODE == GetExprMode::REGISTER){
							return this->handler.createAlloca(return_type, this->name(".ARRAY_REF_DIMENSIONS.ALLOCA"));

						}else if constexpr(MODE == GetExprMode::POINTER){
							return this->handler.createAlloca(return_type, this->name("ARRAY_REF_DIMENSIONS"));

						}else{
							return store_locations[0];
						}
					}();

					uint32_t ref_length_index = 0;
					for(uint32_t i = 0; const BaseType::ArrayRef::Dimension& dimension : array_ref_type.dimensions){
						const pir::Expr length_num = [&](){
							if(dimension.isPtr()){
								const pir::Expr length_load = this->handler.createLoad(
									this->handler.createCalcPtr(
										target_array_ref,
										pir_array_ref_type,
										evo::SmallVector<pir::CalcPtr::Index>{0, ref_length_index + 1}
									),
									type_usize
								);

								ref_length_index += 1;

								return length_load;

							}else{
								return this->handler.createNumber(
									type_usize,
									core::GenericInt(
										unsigned(this->context.getTypeManager().numBitsOfPtr()), dimension.length()
									)
								);
							}
						}();

						const pir::Expr calc_ptr = this->handler.createCalcPtr(
							output_memory, return_type, evo::SmallVector<pir::CalcPtr::Index>{0, i}
						);

						this->handler.createStore(calc_ptr, length_num);

						i += 1;
					}

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createLoad(output_memory, return_type, this->name("ARRAY_REF_DIMENSIONS"));

					}else if constexpr(MODE == GetExprMode::POINTER){
						return output_memory;

					}else{
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::ARRAY_REF_DATA: {
				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;

				}else{
					const sema::ArrayRefData& array_ref_data =
						this->context.getSemaBuffer().getArrayRefData(expr.arrayRefDataID());


					const pir::Type pir_array_ref_type = this->data.getArrayRefType(
						this->module,
						this->context,
						array_ref_data.targetTypeID,
						[&](TypeInfo::ID id){ return *this->get_type<false, true>(id).meta_type_id; }
					).pir_type;


					const pir::Expr target_array_ref = this->get_expr_pointer(array_ref_data.target);

					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Expr data_value = this->handler.createCalcPtr(
							target_array_ref,
							pir_array_ref_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".ARRAY_REF_DATA")
						);

						return this->handler.createLoad(
							data_value, this->module.createPtrType(), this->name("ARRAY_REF_DATA")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->handler.createCalcPtr(
							target_array_ref,
							pir_array_ref_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name("ARRAY_REF_DATA")
						);
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						const pir::Expr data_value = this->handler.createCalcPtr(
							target_array_ref,
							pir_array_ref_type,
							evo::SmallVector<pir::CalcPtr::Index>{0, 0},
							this->name(".ARRAY_REF_DATA")
						);

						this->handler.createMemcpy(store_locations[0], data_value, this->module.createPtrType());
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::UNION_DESIGNATED_INIT_NEW: {
				const sema::UnionDesignatedInitNew& union_designated_init_new = 
					this->context.getSemaBuffer().getUnionDesignatedInitNew(expr.unionDesignatedInitNewID());

				const BaseType::Union& union_info =
					this->context.getTypeManager().getUnion(union_designated_init_new.unionTypeID);

				if(union_info.isUntagged){
					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Type union_pir_type =
							this->get_type<false, false>(BaseType::ID(union_designated_init_new.unionTypeID)).type;

						const pir::Expr storage_alloca =
							this->handler.createAlloca(union_pir_type, this->name(".UNION_DESIGNATED_INIT_NEW"));

						this->get_expr_store(union_designated_init_new.value, storage_alloca);

						return this->handler.createLoad(
							storage_alloca, union_pir_type, this->name("UNION_DESIGNATED_INIT_NEW")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Type union_pir_type =
							this->get_type<false, false>(BaseType::ID(union_designated_init_new.unionTypeID)).type;

						const pir::Expr storage_alloca =
							this->handler.createAlloca(union_pir_type, this->name("UNION_DESIGNATED_INIT_NEW"));

						this->get_expr_store(union_designated_init_new.value, storage_alloca);

						return storage_alloca;

					}else if constexpr(MODE == GetExprMode::STORE){
						this->get_expr_store(union_designated_init_new.value, store_locations);
						return std::nullopt;

					}else{
						this->get_expr_discard(union_designated_init_new.value);
						return std::nullopt;
					}

				}else{
					if constexpr(MODE == GetExprMode::DISCARD){
						this->get_expr_discard(union_designated_init_new.value);
						return std::nullopt;

					}else{
						const pir::Type union_pir_type =
							this->get_type<false, false>(BaseType::ID(union_designated_init_new.unionTypeID)).type;

						const pir::Expr target = [&](){
							if constexpr(MODE == GetExprMode::REGISTER){
								return this->handler.createAlloca(
									union_pir_type, this->name(".UNION_DESIGNATED_INIT_NEW")
								);

							}else if constexpr(MODE == GetExprMode::POINTER){
								return this->handler.createAlloca(
									union_pir_type, this->name("UNION_DESIGNATED_INIT_NEW")
								);

							}else{
								return store_locations[0];
							}
						}();

						if(union_designated_init_new.value.kind() != sema::Expr::Kind::NULL_VALUE){
							const pir::Expr data_ptr = this->handler.createCalcPtr(
								target,
								union_pir_type,
								evo::SmallVector<pir::CalcPtr::Index>{0, 0},
								this->name(".UNION_DESIGNATED_INIT_NEW.data")
							);

							const pir::Expr tag_ptr = this->handler.createCalcPtr(
								target,
								union_pir_type,
								evo::SmallVector<pir::CalcPtr::Index>{0, 1},
								this->name(".UNION_DESIGNATED_INIT_NEW.tag")
							);


							this->get_expr_store(union_designated_init_new.value, data_ptr);

							const pir::Type tag_type = this->module.getStructType(union_pir_type).members[1];
							const pir::Expr tag_value = this->handler.createNumber(
								tag_type,
								core::GenericInt(unsigned(tag_type.getWidth()), union_designated_init_new.fieldIndex)
							);
							this->handler.createStore(tag_ptr, tag_value);

						}else{
							const pir::Expr tag_ptr = this->handler.createCalcPtr(
								target,
								union_pir_type,
								evo::SmallVector<pir::CalcPtr::Index>{0, 1},
								this->name(".UNION_DESIGNATED_INIT_NEW.tag")
							);


							const pir::Type tag_type = this->module.getStructType(union_pir_type).members[1];
							const pir::Expr tag_value = this->handler.createNumber(
								tag_type,
								core::GenericInt(unsigned(tag_type.getWidth()), union_designated_init_new.fieldIndex)
							);
							this->handler.createStore(tag_ptr, tag_value);
						}

						if constexpr(MODE == GetExprMode::REGISTER || MODE == GetExprMode::POINTER){
							return target;
						}else{
							return std::nullopt;
						}
					}
				}
			} break;

			case sema::Expr::Kind::UNION_TAG_CMP: {
				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;
				}else{
					const sema::UnionTagCmp& union_tag_cmp =
						this->context.getSemaBuffer().getUnionTagCmp(expr.unionTagCmpID());
					
					const pir::Type union_pir_type =
						this->get_type<false, false>(BaseType::ID(union_tag_cmp.unionTypeID)).type;
					const pir::Type tag_type = this->module.getStructType(union_pir_type).members[1];

					const pir::Expr tag_ptr = this->handler.createCalcPtr(
						this->get_expr_pointer(union_tag_cmp.value),
						union_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 1},
						this->name(".UNION_TAG_CMP.tag_ptr")
					);

					const pir::Expr tag = this->handler.createLoad(tag_ptr, tag_type, this->name(".UNION_TAG_CMP.tag"));

					const pir::Expr tag_cmp_value = this->handler.createNumber(
						tag_type, core::GenericInt(unsigned(tag_type.getWidth()), union_tag_cmp.fieldIndex)
					);

					const pir::Expr cmp_result = [&](){
						if(union_tag_cmp.isEqual){
							return this->handler.createIEq(tag, tag_cmp_value);
						}else{
							return this->handler.createINeq(tag, tag_cmp_value);
						}
					}();

					if constexpr(MODE == GetExprMode::REGISTER){
						return cmp_result;
						
					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr cmp_result_alloca =
							this->handler.createAlloca(tag_type, this->name("UNION_TAG_CMP"));

						this->handler.createStore(cmp_result_alloca, cmp_result);

						return cmp_result_alloca;

					}else{
						this->handler.createStore(store_locations[0], cmp_result);
						return std::nullopt;
					}
				}
			} break;

			case sema::Expr::Kind::SAME_TYPE_CMP: {
				const sema::SameTypeCmp& same_type_cmp =
					this->context.getSemaBuffer().getSameTypeCmp(expr.sameTypeCmpID());
				
				return this->expr_cmp<MODE>(
					same_type_cmp.typeID, same_type_cmp.lhs, same_type_cmp.rhs, same_type_cmp.isEqual, store_locations
				);
			} break;

			case sema::Expr::Kind::TRY_ELSE_EXPR: {
				const sema::TryElseExpr& try_else_expr =
					this->context.getSemaBuffer().getTryElseExpr(expr.tryElseExprID());
				
				const auto ssl = this->create_scoped_source_location(try_else_expr.line, try_else_expr.collumn);

				const sema::FuncCall& attempt_func_call =
					this->context.getSemaBuffer().getFuncCall(try_else_expr.attempt.funcCallID());

				const Data::FuncInfo& target_func_info = this->data.get_func(
					attempt_func_call.target.as<sema::Func::ID>()
				);


				const BaseType::Function& target_type = this->context.getTypeManager().getFunction(
					this->context.getSemaBuffer().getFunc(attempt_func_call.target.as<sema::Func::ID>()).typeID
				);

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : attempt_func_call.args){
					if(target_func_info.params[i].is_copy()){
						args.emplace_back(this->get_expr_register(arg));

					}else if(target_type.params[i].kind == BaseType::Function::Param::Kind::IN){
						if(arg.kind() == sema::Expr::Kind::COPY){
							args.emplace_back(
								this->get_expr_pointer(this->context.getSemaBuffer().getCopy(arg.copyID()).expr)
							);

						}else if(arg.kind() == sema::Expr::Kind::MOVE){
							args.emplace_back(
								this->get_expr_pointer(this->context.getSemaBuffer().getMove(arg.moveID()).expr)
							);
							
						}else{
							args.emplace_back(this->get_expr_pointer(arg));
						}

					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}

				const pir::Expr return_address = [&](){
					if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						return store_locations[0];
					}else{
						return this->handler.createAlloca(
							this->get_type<false, false>(target_type.returnTypes[0]).type
						);
					}
				}();


				args.emplace_back(return_address);

				if(target_type.errorTypes[0].isVoid() == false){
					const pir::Expr error_value = this->handler.createAlloca(
						*target_func_info.error_return_type, this->name("ERR.ALLOCA")
					);

					for(const sema::ExceptParam::ID except_param_id : try_else_expr.exceptParams){
						const sema::ExceptParam& except_param =
							this->context.getSemaBuffer().getExceptParam(except_param_id);

						const pir::Expr except_param_pir_expr = this->handler.createCalcPtr(
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


				const pir::Expr err_occurred = this->create_call(
					target_func_info.pir_ids[target_in_param_bitmap], std::move(args)
				);

				const pir::BasicBlock::ID if_error_block = this->handler.createBasicBlock(this->name("TRY.ERROR"));
				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("TRY.END"));

				this->handler.createBranch(err_occurred, if_error_block, end_block);

				this->handler.setTargetBasicBlock(if_error_block);
				this->get_expr_store(try_else_expr.except, return_address);
				this->handler.createJump(end_block);

				this->handler.setTargetBasicBlock(end_block);

				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Type return_type = this->get_type<false, false>(target_type.returnTypes[0]).type;
					return this->handler.createLoad(return_address, return_type);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return return_address;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::TRY_ELSE_INTERFACE_EXPR: {
				const sema::TryElseInterfaceExpr& try_else_interface_expr =
					this->context.getSemaBuffer().getTryElseInterfaceExpr(expr.tryElseInterfaceExprID());
				
				const sema::InterfaceCall& attempt_func_interface_call =
					this->context.getSemaBuffer().getInterfaceCall(try_else_interface_expr.attempt.interfaceCallID());


				const auto ssl =
					this->create_scoped_source_location(try_else_interface_expr.line, try_else_interface_expr.collumn);


				///////////////////////////////////
				// create target func type

				const BaseType::Function& target_func_type =
					this->context.getTypeManager().getFunction(attempt_func_interface_call.funcTypeID);

				auto param_types = evo::SmallVector<pir::Type>();
				for(const BaseType::Function::Param& param : target_func_type.params){
					if(param.shouldCopy){
						param_types.emplace_back(this->get_type<false, false>(param.typeID).type);
					}else{
						param_types.emplace_back(this->module.createPtrType());
					}
				}
				for(size_t i = 0; i < target_func_type.returnTypes.size(); i+=1){
					param_types.emplace_back(this->module.createPtrType());
				}
				if(target_func_type.hasErrorReturnValue()){
					param_types.emplace_back(this->module.createPtrType());
				}


				const pir::Type func_pir_type = this->module.getOrCreateFunctionType(
					std::move(param_types), pir::CallingConvention::FAST, this->module.createBoolType()
				);


				///////////////////////////////////
				// get func pointer

				const pir::Expr target_interface_ptr = this->get_expr_pointer(attempt_func_interface_call.value);
				const pir::Type interface_ptr_type =
					this->data.getInterfacePtrType(this->module, this->context.getSourceManager()).pir_type;

				const pir::Expr vtable_ptr = this->handler.createCalcPtr(
					target_interface_ptr,
					interface_ptr_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".VTABLE.PTR")
				);
				const pir::Expr vtable = this->handler.createLoad(
					vtable_ptr, this->module.createPtrType(), this->name(".VTABLE")
				);

				const pir::Expr target_func_ptr = this->handler.createCalcPtr(
					vtable,
					this->module.createPtrType(),
					evo::SmallVector<pir::CalcPtr::Index>{attempt_func_interface_call.vtableFuncIndex},
					this->name(".VTABLE.FUNC.PTR")
				);
				const pir::Expr target_func = this->handler.createLoad(
					target_func_ptr, this->module.createPtrType(), this->name(".VTABLE.FUNC")
				);


				///////////////////////////////////
				// make call

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : attempt_func_interface_call.args){
					if(target_func_type.params[i].shouldCopy){
						args.emplace_back(this->get_expr_register(arg));
					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}

				const pir::Expr return_address = [&](){
					if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
						return store_locations[0];
					}else{
						return this->handler.createAlloca(
							this->get_type<false, false>(target_func_type.returnTypes[0]).type
						);
					}
				}();

				args.emplace_back(return_address);


				if(target_func_type.errorTypes[0].isVoid() == false){
					const Data::InterfaceInfo& interface_info =
						this->data.get_interface(attempt_func_interface_call.interfaceID);

					const pir::Type error_return_type =
						*interface_info.error_return_types[attempt_func_interface_call.vtableFuncIndex];

					const pir::Expr error_value =
						this->handler.createAlloca(error_return_type, this->name("ERR.ALLOCA"));

					for(const sema::ExceptParam::ID except_param_id : try_else_interface_expr.exceptParams){
						const sema::ExceptParam& except_param =
							this->context.getSemaBuffer().getExceptParam(except_param_id);

						const pir::Expr except_param_pir_expr = this->handler.createCalcPtr(
							error_value,
							error_return_type,
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



				const pir::Expr err_occurred = this->handler.createCall(
					target_func, func_pir_type, std::move(args), this->name(".TRY.ERRORED")
				);

				const pir::BasicBlock::ID if_error_block = this->handler.createBasicBlock(this->name("TRY.ERROR"));
				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("TRY.END"));

				this->handler.createBranch(err_occurred, if_error_block, end_block);

				this->handler.setTargetBasicBlock(if_error_block);
				this->get_expr_store(try_else_interface_expr.except, return_address);
				this->handler.createJump(end_block);

				this->handler.setTargetBasicBlock(end_block);

				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Type return_type = this->get_type<false, false>(target_func_type.returnTypes[0]).type;
					return this->handler.createLoad(return_address, return_type);

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

				const pir::Expr param_alloca = this->param_allocas[sema_param.abiIndex];

				if(this->current_func_info->params[sema_param.abiIndex].is_copy()){
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createLoad(param_alloca, this->handler.getAlloca(param_alloca).type);
						
					}else if constexpr(MODE == GetExprMode::POINTER){
						return param_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						this->handler.createMemcpy(
							store_locations[0], param_alloca, this->handler.getAlloca(param_alloca).type
						);
						return std::nullopt;
						
					}else{
						return std::nullopt;
					}

				}else{
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createLoad(
							this->handler.createLoad(param_alloca, this->handler.getAlloca(param_alloca).type),
							*this->current_func_info->params[sema_param.index].reference_type
						);
						
					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->handler.createLoad(param_alloca, this->handler.getAlloca(param_alloca).type);
						
					}else if constexpr(MODE == GetExprMode::STORE){
						this->handler.createMemcpy(
							store_locations[0],
							this->handler.createLoad(param_alloca, this->handler.getAlloca(param_alloca).type),
							*this->current_func_info->params[sema_param.index].reference_type
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

				const pir::Expr return_param_alloca = this->param_allocas[sema_return_param.abiIndex];
				const pir::Expr return_param_ptr =
					this->handler.createLoad(return_param_alloca, this->module.createPtrType());

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createLoad(
						return_param_ptr,
						this->current_func_info->return_params[sema_return_param.index].reference_type
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return return_param_ptr;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::Function& current_func = 
						this->module.getFunction(this->current_func_info->pir_ids[0].as<pir::Function::ID>());

					this->handler.createMemcpy(
						store_locations[0],
						return_param_ptr,
						this->current_func_info->return_params[sema_return_param.index].reference_type
					);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;


			case sema::Expr::Kind::ERROR_RETURN_PARAM: {
				const sema::ErrorReturnParam& sema_error_param =
					this->context.getSemaBuffer().getErrorReturnParam(expr.errorReturnParamID());

				const pir::Expr calc_ptr = this->handler.createCalcPtr(
					this->handler.createLoad(
						this->param_allocas[sema_error_param.abiIndex], this->module.createPtrType()
					),
					*this->current_func_info->error_return_type,
					evo::SmallVector<pir::CalcPtr::Index>{
						pir::CalcPtr::Index(0),
						pir::CalcPtr::Index(sema_error_param.index)
					}
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createLoad(
						calc_ptr,
						this->get_type<false, false>(
							this->current_func_type->errorTypes[sema_error_param.index].asTypeID()
						).type
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return calc_ptr;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					this->handler.createMemcpy(
						store_locations[0],
						calc_ptr,
						this->get_type<false, false>(
							this->current_func_type->errorTypes[sema_error_param.index].asTypeID()
						).type
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
						return this->handler.createLoad(
							scope_level.label_output_locations[block_expr_output_param.index],
							this->get_type<false, false>(block_expr_output_param.typeID).type,
							"LOAD.BLOCK_EXPR_OUTPUT"
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						return scope_level.label_output_locations[block_expr_output_param.index];
						
					}else if constexpr(MODE == GetExprMode::STORE){
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						this->handler.createMemcpy(
							store_locations[0],
							scope_level.label_output_locations[block_expr_output_param.index],
							this->get_type<false, false>(block_expr_output_param.typeID).type
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
					return this->handler.createLoad(
						this->local_func_exprs.at(expr),
						this->get_type<false, false>(
							this->context.getSemaBuffer().getExceptParam(expr.exceptParamID()).typeID
						).type
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->local_func_exprs.at(expr);

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					this->handler.createMemcpy(
						store_locations[0],
						this->local_func_exprs.at(expr),
						this->get_type<false, false>(
							this->context.getSemaBuffer().getExceptParam(expr.exceptParamID()).typeID
						).type
					);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::FOR_PARAM: {
				const sema::ForParam& for_param = this->context.getSemaBuffer().getForParam(expr.forParamID());

				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Expr for_param_expr = this->local_func_exprs.at(expr);
					const pir::Alloca& for_param_alloca = this->handler.getAlloca(for_param_expr);

					if(for_param.isIndex){
						return this->handler.createLoad(
							for_param_expr,
							for_param_alloca.type,
							this->name(remove_alloca_from_name(for_param_alloca.name))
						);

					}else{
						const pir::Expr for_param_load = this->handler.createLoad(
							for_param_expr,
							this->module.createPtrType(),
							this->name("." + remove_alloca_from_name(for_param_alloca.name))
						);

						return this->handler.createLoad(
							for_param_load,
							this->get_type<false, false>(for_param.typeID).type,
							this->name(remove_alloca_from_name(for_param_alloca.name) + ".LOAD")
						);
					}

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr for_param_expr = this->local_func_exprs.at(expr);
					const pir::Alloca& for_param_alloca = this->handler.getAlloca(for_param_expr);

					if(for_param.isIndex){
						return for_param_expr;
					}else{
						return this->handler.createLoad(
							for_param_expr,
							this->module.createPtrType(),
							this->name(remove_alloca_from_name(for_param_alloca.name))
						);
					}

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::Expr for_param_expr = this->local_func_exprs.at(expr);
					const pir::Alloca& for_param_alloca = this->handler.getAlloca(for_param_expr);

					if(for_param.isIndex){
						this->handler.createMemcpy(store_locations[0], for_param_expr, for_param_alloca.type);
					}else{
						const pir::Expr for_param_load = this->handler.createLoad(
							for_param_expr,
							this->module.createPtrType(),
							this->name("." + remove_alloca_from_name(for_param_alloca.name))
						);

						this->handler.createMemcpy(
							store_locations[0], for_param_load, this->get_type<false, false>(for_param.typeID).type
						);
					}

					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::VAR: {
				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Expr var_alloca = this->local_func_exprs.at(expr);

					if(this->data.getConfig().useReadableNames){
						const pir::Alloca& var_actual_alloca = this->handler.getAlloca(var_alloca);

						return this->handler.createLoad(
							var_alloca,
							this->handler.getAlloca(var_alloca).type,
							remove_alloca_from_name(var_actual_alloca.name)
						);

					}else{
						return this->handler.createLoad(var_alloca, this->handler.getAlloca(var_alloca).type);	
					}

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->local_func_exprs.at(expr);

				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::Expr var_alloca = this->local_func_exprs.at(expr);
					this->handler.createMemcpy(
						store_locations[0], var_alloca, this->handler.getAlloca(var_alloca).type
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
					return this->handler.createLoad(
						this->handler.createGlobalValue(pir_var_id),
						pir_var.type,
						this->name("{}.LOAD", this->mangle_name<true>(expr.globalVarID()))
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->handler.createGlobalValue(pir_var_id);
					
				}else if constexpr(MODE == GetExprMode::STORE){
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

					const pir::GlobalVar& pir_var = this->module.getGlobalVar(pir_var_id);
					this->handler.createMemcpy(
						store_locations[0], this->handler.createGlobalValue(pir_var_id), pir_var.type
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
	auto SemaToPIR::get_expr_from_generic_value(
		const core::GenericValue& generic_value,
		TypeInfo::ID expr_type_id,
		evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
		const TypeInfo& expr_type = this->context.getTypeManager().getTypeInfo(expr_type_id);

		if(expr_type.qualifiers().empty()){
			switch(expr_type.baseTypeID().kind()){
				case BaseType::Kind::DUMMY: evo::debugFatalBreak("Invalid base type");

				case BaseType::Kind::PRIMITIVE: {
					const BaseType::Primitive& primitive = 
						this->context.getTypeManager().getPrimitive(expr_type.baseTypeID().primitiveID());

					switch(primitive.kind()){
						case Token::Kind::TYPE_F16: {
							const pir::Type pir_type = this->module.createFloatType(16);

							if constexpr(MODE == GetExprMode::REGISTER){
								return this->handler.createNumber(pir_type, generic_value.getF16());

							}else if constexpr(MODE == GetExprMode::POINTER){
								const pir::Expr output_alloca = this->handler.createAlloca(pir_type);

								this->handler.createStore(
									output_alloca, this->handler.createNumber(pir_type, generic_value.getF16())
								);

								return output_alloca;
								
							}else if constexpr(MODE == GetExprMode::STORE){
								this->handler.createStore(
									store_locations[0],
									this->handler.createNumber(pir_type, generic_value.getF16())
								);
								return std::nullopt;
								
							}else{
								return std::nullopt;
							}
						} break;

						case Token::Kind::TYPE_F32: {
							const pir::Type pir_type = this->module.createFloatType(32);

							if constexpr(MODE == GetExprMode::REGISTER){
								return this->handler.createNumber(pir_type, generic_value.getF32());

							}else if constexpr(MODE == GetExprMode::POINTER){
								const pir::Expr output_alloca = this->handler.createAlloca(pir_type);

								this->handler.createStore(
									output_alloca, this->handler.createNumber(pir_type, generic_value.getF32())
								);

								return output_alloca;
								
							}else if constexpr(MODE == GetExprMode::STORE){
								this->handler.createStore(
									store_locations[0],
									this->handler.createNumber(pir_type, generic_value.getF32())
								);
								return std::nullopt;
								
							}else{
								return std::nullopt;
							}
						} break;

						case Token::Kind::TYPE_F64: {
							const pir::Type pir_type = this->module.createFloatType(64);

							if constexpr(MODE == GetExprMode::REGISTER){
								return this->handler.createNumber(pir_type, generic_value.getF64());

							}else if constexpr(MODE == GetExprMode::POINTER){
								const pir::Expr output_alloca = this->handler.createAlloca(pir_type);

								this->handler.createStore(
									output_alloca, this->handler.createNumber(pir_type, generic_value.getF64())
								);

								return output_alloca;
								
							}else if constexpr(MODE == GetExprMode::STORE){
								this->handler.createStore(
									store_locations[0],
									this->handler.createNumber(pir_type, generic_value.getF64())
								);
								return std::nullopt;
								
							}else{
								return std::nullopt;
							}
						} break;

						case Token::Kind::TYPE_F80: {
							const pir::Type pir_type = this->module.createFloatType(80);

							if constexpr(MODE == GetExprMode::REGISTER){
								return this->handler.createNumber(pir_type, generic_value.getF80());

							}else if constexpr(MODE == GetExprMode::POINTER){
								const pir::Expr output_alloca = this->handler.createAlloca(pir_type);

								this->handler.createStore(
									output_alloca, this->handler.createNumber(pir_type, generic_value.getF80())
								);

								return output_alloca;
								
							}else if constexpr(MODE == GetExprMode::STORE){
								this->handler.createStore(
									store_locations[0],
									this->handler.createNumber(pir_type, generic_value.getF80())
								);
								return std::nullopt;
								
							}else{
								return std::nullopt;
							}
						} break;

						case Token::Kind::TYPE_F128: {
							const pir::Type pir_type = this->module.createFloatType(128);

							if constexpr(MODE == GetExprMode::REGISTER){
								return this->handler.createNumber(pir_type, generic_value.getF128());

							}else if constexpr(MODE == GetExprMode::POINTER){
								const pir::Expr output_alloca = this->handler.createAlloca(pir_type);

								this->handler.createStore(
									output_alloca, this->handler.createNumber(pir_type, generic_value.getF128())
								);

								return output_alloca;
								
							}else if constexpr(MODE == GetExprMode::STORE){
								this->handler.createStore(
									store_locations[0],
									this->handler.createNumber(pir_type, generic_value.getF128())
								);
								return std::nullopt;
								
							}else{
								return std::nullopt;
							}
						} break;

						case Token::Kind::TYPE_C_LONG_DOUBLE: {
							const pir::Type pir_type =  this->module.createFloatType(
								uint32_t(this->context.getTypeManager().numBits(expr_type.baseTypeID()))
							);

							if constexpr(MODE == GetExprMode::REGISTER){
								return this->handler.createNumber(pir_type, generic_value.getF128());

							}else if constexpr(MODE == GetExprMode::POINTER){
								const pir::Expr output_alloca = this->handler.createAlloca(pir_type);

								this->handler.createStore(
									output_alloca, this->handler.createNumber(pir_type, generic_value.getF128())
								);

								return output_alloca;
								
							}else if constexpr(MODE == GetExprMode::STORE){
								this->handler.createStore(
									store_locations[0],
									this->handler.createNumber(pir_type, generic_value.getF128())
								);
								return std::nullopt;
								
							}else{
								return std::nullopt;
							}
						} break;

						case Token::Kind::TYPE_BOOL: {
							if constexpr(MODE == GetExprMode::REGISTER){
								return this->handler.createBoolean(generic_value.getBool());

							}else if constexpr(MODE == GetExprMode::POINTER){
								const pir::Expr output_alloca =
									this->handler.createAlloca(this->module.createBoolType());

								this->handler.createStore(
									output_alloca, this->handler.createBoolean(generic_value.getBool())
								);

								return output_alloca;
								
							}else if constexpr(MODE == GetExprMode::STORE){
								this->handler.createStore(
									store_locations[0],
									this->handler.createBoolean(generic_value.getBool())
								);
								return std::nullopt;
								
							}else{
								return std::nullopt;
							}
						} break;

						case Token::Kind::TYPE_RAWPTR: evo::unimplemented("Generic value to RawPtr");

						case Token::Kind::TYPE_UINT:         case Token::Kind::TYPE_USIZE:
						case Token::Kind::TYPE_UI_N:         case Token::Kind::TYPE_BYTE:
						case Token::Kind::TYPE_TYPEID:       case Token::Kind::TYPE_C_USHORT:
						case Token::Kind::TYPE_C_UINT:       case Token::Kind::TYPE_C_ULONG:
						case Token::Kind::TYPE_C_ULONG_LONG: {
							const unsigned bit_width = 
								unsigned(this->context.getTypeManager().numBits(expr_type.baseTypeID(), false));

							const pir::Type pir_type = this->module.createUnsignedType(bit_width);

							if constexpr(MODE == GetExprMode::REGISTER){
								return this->handler.createNumber(pir_type, generic_value.getInt(bit_width));

							}else if constexpr(MODE == GetExprMode::POINTER){
								const pir::Expr output_alloca = this->handler.createAlloca(pir_type);

								this->handler.createStore(
									output_alloca, this->handler.createNumber(pir_type, generic_value.getInt(bit_width))
								);

								return output_alloca;
								
							}else if constexpr(MODE == GetExprMode::STORE){
								this->handler.createStore(
									store_locations[0],
									this->handler.createNumber(pir_type, generic_value.getInt(bit_width))
								);
								return std::nullopt;
								
							}else{
								return std::nullopt;
							}
						} break;

						case Token::Kind::TYPE_INT:         case Token::Kind::TYPE_ISIZE:
						case Token::Kind::TYPE_I_N:         case Token::Kind::TYPE_CHAR:
						case Token::Kind::TYPE_C_WCHAR:     case Token::Kind::TYPE_C_SHORT:
						case Token::Kind::TYPE_C_INT:       case Token::Kind::TYPE_C_LONG:
						case Token::Kind::TYPE_C_LONG_LONG: {
							const unsigned bit_width = 
								unsigned(this->context.getTypeManager().numBits(expr_type.baseTypeID(), false));

							const pir::Type pir_type = this->module.createSignedType(bit_width);

							if constexpr(MODE == GetExprMode::REGISTER){
								return this->handler.createNumber(pir_type, generic_value.getInt(bit_width));

							}else if constexpr(MODE == GetExprMode::POINTER){
								const pir::Expr output_alloca = this->handler.createAlloca(pir_type);

								this->handler.createStore(
									output_alloca, this->handler.createNumber(pir_type, generic_value.getInt(bit_width))
								);

								return output_alloca;
								
							}else if constexpr(MODE == GetExprMode::STORE){
								this->handler.createStore(
									store_locations[0],
									this->handler.createNumber(pir_type, generic_value.getInt(bit_width))
								);
								return std::nullopt;
								
							}else{
								return std::nullopt;
							}
						} break;
					}

					evo::debugFatalBreak("Unkonwn primitive type");
				} break;

				case BaseType::Kind::FUNCTION: {
					evo::unimplemented("Lowering function core::GenericValue to pir::Expr");
				} break;

				case BaseType::Kind::ARRAY: case BaseType::Kind::STRUCT: case BaseType::Kind::UNION: {
					if constexpr(MODE == GetExprMode::DISCARD){
						return std::nullopt;

					}else{
						const pir::GlobalVar::ByteArray::ID byte_array_value_id =
							this->module.createGlobalByteArray(generic_value.dataRange());

						const pir::GlobalVar::ByteArray& byte_array_value =
							this->module.getGlobalByteArray(byte_array_value_id);

						const pir::GlobalVar::ID byte_array = this->module.createGlobalVar(
							std::format("PTHR.byteArr{}", this->data.get_byte_array_id()),
							byte_array_value.type,
							pir::Linkage::PRIVATE,
							byte_array_value_id,
							true
						);

						if constexpr(MODE == GetExprMode::REGISTER){
							return this->handler.createLoad(
								this->handler.createGlobalValue(byte_array), byte_array_value.type
							);
							
						}else if constexpr(MODE == GetExprMode::POINTER){
							return this->handler.createGlobalValue(byte_array);
							
						}else{ // store
							this->handler.createMemcpy(
								store_locations[0], this->handler.createGlobalValue(byte_array), byte_array_value.type
							);
							return std::nullopt;
						}
					}
				} break;

				case BaseType::Kind::ARRAY_REF: {
					evo::unimplemented("Lowering array ref core::GenericValue to pir::Expr");
				} break;

				case BaseType::Kind::ALIAS: {
					const BaseType::Alias& alias_type =
						this->context.getTypeManager().getAlias(expr_type.baseTypeID().aliasID());

					return this->get_expr_from_generic_value<MODE>(
						generic_value, alias_type.aliasedType, store_locations
					);
				} break;

				case BaseType::Kind::DISTINCT_ALIAS: {
					const BaseType::DistinctAlias& distinct_alias_type =
						this->context.getTypeManager().getDistinctAlias(expr_type.baseTypeID().distinctAliasID());

					return this->get_expr_from_generic_value<MODE>(
						generic_value, distinct_alias_type.underlyingType, store_locations
					);
				} break;

				case BaseType::Kind::ENUM: {
					const unsigned bit_width = 
						unsigned(this->context.getTypeManager().numBits(expr_type.baseTypeID(), false));

					const pir::Type pir_type = [&]() -> pir::Type {
						if(this->context.getTypeManager().isUnsignedIntegral(expr_type.baseTypeID())){
							return this->module.createUnsignedType(bit_width);
						}else{
							return this->module.createSignedType(bit_width);
						}
					}();


					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createNumber(pir_type, generic_value.getInt(bit_width));

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr output_alloca = this->handler.createAlloca(pir_type);

						this->handler.createStore(
							output_alloca, this->handler.createNumber(pir_type, generic_value.getInt(bit_width))
						);

						return output_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						this->handler.createStore(
							store_locations[0],
							this->handler.createNumber(pir_type, generic_value.getInt(bit_width))
						);
						return std::nullopt;
						
					}else{
						return std::nullopt;
					}
				} break;

				case BaseType::Kind::INTERFACE_MAP: {
					const BaseType::InterfaceMap& interface_map_type =
						this->context.getTypeManager().getInterfaceMap(expr_type.baseTypeID().interfaceMapID());

					return this->get_expr_from_generic_value<MODE>(
						generic_value, interface_map_type.underlyingTypeID, store_locations
					);
				} break;

				case BaseType::Kind::ARRAY_DEDUCER: 
				case BaseType::Kind::ARRAY_REF_DEDUCER: 
				case BaseType::Kind::STRUCT_TEMPLATE: 
				case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: 
				case BaseType::Kind::TYPE_DEDUCER: 
				case BaseType::Kind::INTERFACE:
				case BaseType::Kind::POLY_INTERFACE_REF: {
					evo::debugFatalBreak("Invalid generic value type");
				} break;
			}

			evo::debugFatalBreak("Unknown BaseType");

		}else{
			evo::unimplemented("Lowering qualified type core::GenericValue to pir::Expr");
		}
	}




	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::default_new_expr(
		TypeInfo::ID expr_type_id, bool is_initialization, evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
		evo::debugAssert(
			this->context.getTypeManager().isDefaultInitializable(expr_type_id), "this type isn't default initializable"
		);

		if constexpr(MODE != GetExprMode::STORE){
			evo::debugAssert(is_initialization, "this mode shouldn't be assignment");
		}

		if(this->context.getTypeManager().isTriviallyDefaultInitializable(expr_type_id)){
			if constexpr(MODE == GetExprMode::STORE){
				return std::nullopt;

			}else{
				const TypeInfo& expr_type = this->context.getTypeManager().getTypeInfo(expr_type_id);

				if(expr_type.qualifiers().empty() && expr_type.baseTypeID().kind() == BaseType::Kind::PRIMITIVE){
					const pir::Type primitive_type = this->get_type<false, false>(expr_type_id).type;

					if constexpr(MODE == GetExprMode::REGISTER){
						if(primitive_type.isIntegral()){
							return this->handler.createNumber(
								primitive_type, core::GenericInt(primitive_type.getWidth(), 0)
							);

						}else{
							switch(primitive_type.getWidth()){
								case 16: {
									return this->handler.createNumber(
										primitive_type, core::GenericFloat::createF32(0.0f).asF16()
									);
								} break;

								case 32: {
									return this->handler.createNumber(
										primitive_type, core::GenericFloat::createF32(0.0f)
									);
								} break;

								case 64: {
									return this->handler.createNumber(
										primitive_type, core::GenericFloat::createF64(0.0)
									);
								} break;

								case 80: {
									return this->handler.createNumber(
										primitive_type, core::GenericFloat::createF64(0.0).asF80()
									);
								} break;

								case 128: {
									return this->handler.createNumber(
										primitive_type, core::GenericFloat::createF64(0.0).asF128()
									);
								} break;
							}
						}
						
					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->handler.createAlloca(primitive_type);

					}else if constexpr(MODE == GetExprMode::DISCARD){
						return std::nullopt;

					}else{
						static_assert(false, "Unknown GetExprMode");
					}
					
				}else{
					const pir::Type expr_pir_type = this->get_type<false, false>(expr_type_id).type;

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createLoad(
							this->handler.createAlloca(expr_pir_type, this->name(".DEFAULT_NEW.ptr")),
							expr_pir_type,
							this->name("DEFAULT_NEW")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr default_new_alloca =
							this->handler.createAlloca(expr_pir_type, this->name("DEFAULT_NEW"));

						this->end_of_stmt_deletes.emplace_back(default_new_alloca, expr_type_id);

						return default_new_alloca;

					}else if constexpr(MODE == GetExprMode::DISCARD){
						const pir::Expr default_new_alloca =
							this->handler.createAlloca(expr_pir_type, this->name(".DISCARD.DEFAULT_NEW"));

						this->add_auto_delete_target(default_new_alloca, expr_type_id);

						return std::nullopt;

					}else{
						static_assert(false, "Unkonwn GetExprMode");
					}

					
				}
			}
		}
		
		const TypeInfo& expr_type = this->context.getTypeManager().getTypeInfo(expr_type_id);

		if(expr_type.qualifiers().empty() == false){
			if(expr_type.qualifiers().back().isPtr){
				evo::debugAssert(expr_type.isOptional(), "Unknown non-trivial default-initializable qualifier");

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createNullptr();

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr ptr_alloca =
						this->handler.createAlloca(this->module.createPtrType(), "DEFAULT_NEW_OPT_PTR");
						
					this->handler.createStore(ptr_alloca, this->handler.createNullptr());

					return ptr_alloca;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], this->handler.createNullptr());
					return std::nullopt;
					
				}else{
					return std::nullopt;
				}
			}

			const pir::Type opt_pir_type = this->get_type<false, false>(expr_type_id).type;

			const pir::Expr target = [&]() -> pir::Expr {
				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createAlloca(opt_pir_type, this->name(".DEFAULT_NEW_OPT"));

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->handler.createAlloca(opt_pir_type, this->name("DEFAULT_NEW_OPT"));
					
				}else if constexpr(MODE == GetExprMode::STORE){
					return store_locations[0];
					
				}else{
					return this->handler.createAlloca(opt_pir_type, this->name(".DISCARD"));
				}
			}();


			const pir::Expr flag_ptr = this->handler.createCalcPtr(
				target,
				opt_pir_type,
				evo::SmallVector<pir::CalcPtr::Index>{0, 1},
				this->name(".DEFAULT_NEW_OPT.flag_ptr")
			);

			const TypeInfo::ID data_type_id =
				this->context.type_manager.getOrCreateTypeInfo(expr_type.copyWithPoppedQualifier());

			if(is_initialization || this->context.getTypeManager().isTriviallyDeletable(data_type_id)){
				this->handler.createStore(flag_ptr, this->handler.createBoolean(false));

			}else{
				const pir::BasicBlock::ID has_flag_block =
					this->handler.createBasicBlock(this->name("DEFAULT_NEW_OPT.has_flag"));
				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("DEFAULT_NEW_OPT.end"));

				const pir::Expr flag = this->handler.createLoad(
					flag_ptr, this->module.createBoolType(), this->name(".DEFAULT_NEW_OPT.flag")
				);

				this->handler.createBranch(flag, has_flag_block, end_block);

				this->handler.setTargetBasicBlock(has_flag_block);
				const pir::Expr data_ptr = this->handler.createCalcPtr(
					target,
					opt_pir_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".DEFAULT_NEW_OPT.data_ptr")
				);
				this->delete_expr(data_ptr, data_type_id);
				this->handler.createStore(flag_ptr, this->handler.createBoolean(false));
				this->handler.createJump(end_block);

				this->handler.setTargetBasicBlock(end_block);

			}

			if constexpr(MODE == GetExprMode::REGISTER){
				return this->handler.createLoad(target, opt_pir_type, this->name("DEFAULT_NEW_OPT"));

			}else if constexpr(MODE == GetExprMode::POINTER){
				this->end_of_stmt_deletes.emplace_back(target, expr_type_id);
				return target;
				
			}else if constexpr(MODE == GetExprMode::STORE){
				return std::nullopt;
				
			}else{
				this->add_auto_delete_target(target, expr_type_id);
				return std::nullopt;
			}
		}

		switch(expr_type.baseTypeID().kind()){
			case BaseType::Kind::PRIMITIVE: {
				evo::debugAssert("Unknown non-trivial default-initializable type");
			} break;
			
			case BaseType::Kind::FUNCTION: {
				evo::debugFatalBreak("Invalid type to default initialize");
			} break;
			
			case BaseType::Kind::ARRAY: {
				const pir::Type array_pir_type = this->get_type<false, false>(expr_type_id).type;

				const pir::Expr target = [&]() -> pir::Expr {
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createAlloca(array_pir_type, this->name(".DEFAULT_NEW_ARR"));

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->handler.createAlloca(array_pir_type, this->name("DEFAULT_NEW_ARR"));
						
					}else if constexpr(MODE == GetExprMode::STORE){
						return store_locations[0];
						
					}else{
						return this->handler.createAlloca(array_pir_type, this->name(".DISCARD"));
					}
				}();


				const BaseType::Array& array_type =
					this->context.getTypeManager().getArray(expr_type.baseTypeID().arrayID());

				if(this->context.getTypeManager().isTriviallyDefaultInitializable(array_type.elementTypeID) == false){
					this->iterate_array(array_type, false, "DEFAULT_NEW_ARR", [&](pir::Expr index) -> void {
						const pir::Expr target_elem = this->handler.createCalcPtr(
							target, array_pir_type, evo::SmallVector<pir::CalcPtr::Index>{0, index}
						);
						this->default_new_expr<GetExprMode::STORE>(
							array_type.elementTypeID, is_initialization, target_elem
						);
					});
				}

				if(array_type.terminator.has_value()){
					const uint64_t terminator_index = [&](){
						uint64_t output_num_elems = 1;

						for(uint64_t dimension : array_type.dimensions){
							output_num_elems *= dimension;
						}

						return output_num_elems;
					}();

					const pir::Expr terminator_ptr = this->handler.createCalcPtr(
						target, array_pir_type, evo::SmallVector<pir::CalcPtr::Index>{0, int64_t(terminator_index)}
					);

					std::ignore = this->get_expr_from_generic_value<GetExprMode::STORE>(
						*array_type.terminator, array_type.elementTypeID, terminator_ptr
					);
				}

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createLoad(target, array_pir_type, this->name("DEFAULT_NEW_ARR"));

				}else if constexpr(MODE == GetExprMode::POINTER){
					this->end_of_stmt_deletes.emplace_back(target, expr_type_id);
					return target;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					return std::nullopt;
					
				}else{
					this->add_auto_delete_target(target, expr_type_id);
					return std::nullopt;
				}
			} break;
			
			case BaseType::Kind::ARRAY_DEDUCER: {
				evo::debugFatalBreak("Invalid type to default initialize");
			} break;
			
			case BaseType::Kind::ARRAY_REF: {
				if constexpr(MODE == GetExprMode::DISCARD){
					return std::nullopt;

				}else{
					const pir::Type pir_array_ref_type = this->data.getArrayRefType(
						this->module,
						this->context,
						expr_type.baseTypeID().arrayRefID(),
						[&](TypeInfo::ID id){ return *this->get_type<false, true>(id).meta_type_id; }
					).pir_type;

					if constexpr(MODE == GetExprMode::REGISTER){
						const pir::Expr output_alloca = this->handler.createAlloca(
							pir_array_ref_type, this->name(".DEFAULT_NEW_ARRAY_REF")
						);

						this->handler.createMemset(
							output_alloca,
							this->handler.createNumber(
								this->module.createUnsignedType(8), core::GenericInt::create<uint8_t>(0)
							),
							pir_array_ref_type
						);

						return this->handler.createLoad(
							output_alloca, pir_array_ref_type, this->name("DEFAULT_NEW_ARRAY_REF")
						);
						
					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr output_alloca = this->handler.createAlloca(
							pir_array_ref_type, this->name("DEFAULT_NEW_ARRAY_REF")
						);

						this->handler.createMemset(
							output_alloca,
							this->handler.createNumber(
								this->module.createUnsignedType(8), core::GenericInt::create<uint8_t>(0)
							),
							pir_array_ref_type
						);

						return output_alloca;
						
					}else{
						evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");

						this->handler.createMemset(
							store_locations[0],
							this->handler.createNumber(
								this->module.createUnsignedType(8), core::GenericInt::create<uint8_t>(0)
							),
							pir_array_ref_type
						);
						return std::nullopt;
					}
				}
			} break;
			
			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias_type =
					this->context.getTypeManager().getAlias(expr_type.baseTypeID().aliasID());

				return this->default_new_expr<MODE>(
					alias_type.aliasedType, is_initialization, store_locations
				);
			} break;
			
			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& distinct_alias_type =
					this->context.getTypeManager().getDistinctAlias(expr_type.baseTypeID().distinctAliasID());

				return this->default_new_expr<MODE>(
					distinct_alias_type.underlyingType, is_initialization, store_locations
				);
			} break;
			
			case BaseType::Kind::STRUCT: {
				const pir::Type struct_pir_type = this->get_type<false, false>(expr_type.baseTypeID()).type;

				const pir::Expr target = [&]() -> pir::Expr {
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createAlloca(struct_pir_type, this->name(".DEFAULT_NEW_STRUCT"));

					}else if constexpr(MODE == GetExprMode::POINTER){
						return this->handler.createAlloca(struct_pir_type, this->name("DEFAULT_NEW_STRUCT"));
						
					}else if constexpr(MODE == GetExprMode::STORE){
						return store_locations[0];
						
					}else{
						return this->handler.createAlloca(struct_pir_type, this->name(".DISCARD"));
					}
				}();

				const BaseType::Struct& struct_type =
					this->context.getTypeManager().getStruct(expr_type.baseTypeID().structID());

				if(is_initialization){
					for(const sema::Func::ID& new_init_overload_id : struct_type.newInitOverloads){
						if(this->context.getSemaBuffer().getFunc(new_init_overload_id).minNumArgs > 0){ continue; }

						const Data::FuncInfo& target_func_info = this->data.get_func(new_init_overload_id);
						const pir::Function::ID pir_id = target_func_info.pir_ids[0].as<pir::Function::ID>();

						this->create_call_void(pir_id, evo::SmallVector<pir::Expr>{target});

						break;
					}
				}else{
					bool found_assign_overload = false;

					for(const sema::Func::ID& new_assign_overload_id : struct_type.newAssignOverloads){
						if(this->context.getSemaBuffer().getFunc(new_assign_overload_id).minNumArgs > 0){ continue; }

						const Data::FuncInfo& target_func_info = this->data.get_func(new_assign_overload_id);
						const pir::Function::ID pir_id = target_func_info.pir_ids[0].as<pir::Function::ID>();

						this->create_call_void(pir_id, evo::SmallVector<pir::Expr>{target});

						found_assign_overload = true;
						break;
					}

					if(found_assign_overload == false){
						for(const sema::Func::ID& new_init_overload_id : struct_type.newInitOverloads){
							if(this->context.getSemaBuffer().getFunc(new_init_overload_id).minNumArgs > 0){ continue; }

							const Data::FuncInfo& target_func_info = this->data.get_func(new_init_overload_id);
							const pir::Function::ID pir_id = target_func_info.pir_ids[0].as<pir::Function::ID>();

							this->delete_expr(target, expr_type_id);
							this->create_call_void(pir_id, evo::SmallVector<pir::Expr>{target});

							break;
						}
					}
				}

				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createLoad(target, struct_pir_type, this->name("DEFAULT_NEW_STRUCT"));

				}else if constexpr(MODE == GetExprMode::POINTER){
					this->end_of_stmt_deletes.emplace_back(target, expr_type_id);
					return target;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					return std::nullopt;
					
				}else{
					this->add_auto_delete_target(target, expr_type_id);
					return std::nullopt;
				}
			} break;
			
			case BaseType::Kind::STRUCT_TEMPLATE: {
				evo::debugFatalBreak("Invalid type to default initialize");
			} break;
			
			case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: {
				evo::debugFatalBreak("Invalid type to default initialize");
			} break;
			
			case BaseType::Kind::UNION: {
				const BaseType::Union& union_type =
					this->context.getTypeManager().getUnion(expr_type.baseTypeID().unionID());

				evo::debugAssert(union_type.isUntagged == false, "Tagged union should be trivially-initable");

				const pir::Type union_pir_type = this->get_type<false, false>(expr_type.baseTypeID()).type;

				const pir::Expr target = [&]() -> pir::Expr {
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createAlloca(union_pir_type, this->name(".DEFAULT_NEW_UNION"));

					}else if constexpr(MODE == GetExprMode::POINTER){
						if(is_initialization == false){
							this->delete_expr(store_locations[0], expr_type_id);
						}
						return this->handler.createAlloca(union_pir_type, this->name("DEFAULT_NEW_UNION"));
						
					}else if constexpr(MODE == GetExprMode::STORE){
						return store_locations[0];
						
					}else{
						return this->handler.createAlloca(union_pir_type, this->name(".DISCARD"));
					}
				}();


				const TypeInfo::VoidableID default_field_type = union_type.fields[0].typeID;
				if(
					default_field_type.isVoid() == false
					&& !this->context.getTypeManager().isTriviallyDefaultInitializable(default_field_type.asTypeID())
				){
					const pir::Expr data_ptr = this->handler.createCalcPtr(
						target,
						union_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 0},
						this->name(".DEFAULT_NEW_UNION.data")
					);

					this->default_new_expr<GetExprMode::STORE>(default_field_type.asTypeID(), true, data_ptr);
				}


				const pir::Expr tag_ptr = this->handler.createCalcPtr(
					target,
					union_pir_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".DEFAULT_NEW_UNION.tag")
				);

				const pir::Type tag_type = this->module.getStructType(union_pir_type).members[1];

				this->handler.createStore(
					tag_ptr, this->handler.createNumber(tag_type, core::GenericInt(tag_type.getWidth(), 0))
				);


				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createLoad(target, union_pir_type, this->name("DEFAULT_NEW_UNION"));

				}else if constexpr(MODE == GetExprMode::POINTER){
					this->end_of_stmt_deletes.emplace_back(target, expr_type_id);
					return target;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					return std::nullopt;
					
				}else{
					this->add_auto_delete_target(target, expr_type_id);
					return std::nullopt;
				}
			} break;
			
			case BaseType::Kind::ENUM: {
				evo::debugFatalBreak("Invalid type to default initialize");
			} break;
			
			case BaseType::Kind::TYPE_DEDUCER: {
				evo::debugFatalBreak("Invalid type to default initialize");
			} break;
			
			case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Invalid type to default initialize");
			} break;
			
			case BaseType::Kind::POLY_INTERFACE_REF: {
				evo::debugFatalBreak("Invalid type to default initialize");
			} break;
			
			case BaseType::Kind::INTERFACE_MAP: {
				const BaseType::InterfaceMap& interface_map =
					this->context.getTypeManager().getInterfaceMap(expr_type.baseTypeID().interfaceMapID());

				return this->default_new_expr<MODE>(interface_map.underlyingTypeID, is_initialization, store_locations);
			} break;
		}

		evo::debugFatalBreak("Unknown base type");
	}



	auto SemaToPIR::delete_expr(const sema::Expr& expr, TypeInfo::ID expr_type_id) -> void {
		if(this->context.getTypeManager().isTriviallyDeletable(expr_type_id)){ return; }

		this->delete_expr(this->get_expr_pointer(expr), expr_type_id);
	}


	auto SemaToPIR::delete_expr(pir::Expr expr, TypeInfo::ID expr_type_id) -> void {
		evo::debugAssert(this->handler.getExprType(expr).kind() == pir::Type::Kind::PTR, "Expr must be a pointer");

		if(this->context.getTypeManager().isTriviallyDeletable(expr_type_id)){ return; }

		const TypeInfo& expr_type = this->context.getTypeManager().getTypeInfo(expr_type_id);

		if(expr_type.qualifiers().size() > 0){
			evo::debugAssert(
				expr_type.qualifiers().back().isOptional, "Unknown non-trivially-deletable type with qualifiers"
			);

			const pir::Type target_type = this->get_type<false, false>(expr_type_id).type;
			const pir::Expr flag_ptr = this->handler.createCalcPtr(
				expr, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 1}, this->name(".DELETE_OPT.flag_ptr")
			);
			const pir::Expr flag =
				this->handler.createLoad(flag_ptr, this->module.createBoolType(), this->name(".DELETE_OPT.flag"));

			const pir::Expr flag_is_true =
				this->handler.createIEq(flag, this->handler.createBoolean(true), this->name(".DELETE_OPT.flag_true"));

			const pir::BasicBlock::ID delete_block = this->handler.createBasicBlock(this->name("DELETE_OPT.DELETE"));
			const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("DELETE_OPT.END"));

			this->handler.createBranch(flag_is_true, delete_block, end_block);

			this->handler.setTargetBasicBlock(delete_block);

			const pir::Expr data_ptr = this->handler.createCalcPtr(
				expr, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 0}, this->name(".DELETE_OPT.data_ptr")
			);
			this->delete_expr(
				data_ptr, this->context.type_manager.getOrCreateTypeInfo(expr_type.copyWithPoppedQualifier())
			);

			this->handler.createJump(end_block);

			this->handler.setTargetBasicBlock(end_block);

			return;
		}

		switch(expr_type.baseTypeID().kind()){
			case BaseType::Kind::DUMMY: {
				evo::debugFatalBreak("Not a valid type");
			} break;

			case BaseType::Kind::PRIMITIVE: {
				evo::debugFatalBreak("Not non-trivially-deletable");
			} break;

			case BaseType::Kind::FUNCTION: {
				evo::debugFatalBreak("Not non-trivially-deletable");
			} break;

			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array_type =
					this->context.getTypeManager().getArray(expr_type.baseTypeID().arrayID());

				const pir::Type array_pir_type = this->get_type<false, false>(expr_type_id).type;

				this->iterate_array(array_type, true, "DELETE_ARR", [&](pir::Expr index) -> void {
					const pir::Expr target_elem = this->handler.createCalcPtr(
						expr, array_pir_type, evo::SmallVector<pir::CalcPtr::Index>{0, index}
					);
					this->delete_expr(target_elem, array_type.elementTypeID);
				});
			} break;

			case BaseType::Kind::ARRAY_DEDUCER: {
				evo::debugFatalBreak("Not deletable");
			} break;

			case BaseType::Kind::ARRAY_REF: {
				evo::debugFatalBreak("Not non-trivially-deletable");
			} break;

			case BaseType::Kind::ARRAY_REF_DEDUCER: {
				evo::debugFatalBreak("Not deletable");
			} break;

			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias =
					this->context.getTypeManager().getAlias(expr_type.baseTypeID().aliasID());

				this->delete_expr(expr, alias.aliasedType);
			} break;

			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& distinct_alias =
					this->context.getTypeManager().getDistinctAlias(expr_type.baseTypeID().distinctAliasID());

				this->delete_expr(expr, distinct_alias.underlyingType);
			} break;

			case BaseType::Kind::STRUCT: {
				const BaseType::Struct& struct_type =
					this->context.getTypeManager().getStruct(expr_type.baseTypeID().structID());

				evo::debugAssert(struct_type.deleteOverload.load().has_value(), "Not non-trivially-deletable");

				const Data::FuncInfo& target_func_info = this->data.get_func(*struct_type.deleteOverload.load());
				const pir::Function::ID pir_id = target_func_info.pir_ids[0].as<pir::Function::ID>();

				this->create_call_void(pir_id, evo::SmallVector<pir::Expr>{expr});
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE: {
				evo::debugFatalBreak("Not deletable");
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: {
				evo::debugFatalBreak("Not deletable");
			} break;

			case BaseType::Kind::UNION: {
				const BaseType::Union& union_type =
					this->context.getTypeManager().getUnion(expr_type.baseTypeID().unionID());

				const pir::Type union_pir_type = this->get_type<false, false>(expr_type.baseTypeID()).type;
				const pir::StructType union_struct_type = this->module.getStructType(union_pir_type);

				evo::debugAssert(union_type.isUntagged == false, "untagged union types aren't non-trivially-deletable");

				const pir::BasicBlock::ID current_block = this->handler.getTargetBasicBlock().getID();
				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("UNION_DELETE.end"));

				const pir::Expr data_ptr = this->handler.createCalcPtr(
					expr,
					union_pir_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 0},
					this->name(".UNION_DELETE.data")
				);

				auto cases = evo::SmallVector<pir::Switch::Case>();

				for(size_t i = 0; const BaseType::Union::Field& field : union_type.fields){
					EVO_DEFER([&](){ i += 1; });

					if(field.typeID.isVoid()){ continue; }
					if(this->context.getTypeManager().isTriviallyDeletable(field.typeID.asTypeID())){ continue; }

					const pir::BasicBlock::ID field_delete_block =
						this->handler.createBasicBlockInline(this->name("UNION_DELETE.field_{}", i));

					cases.emplace_back(
						this->handler.createNumber(
							union_struct_type.members[1],
							core::GenericInt(union_struct_type.members[1].getWidth(), i)
						),
						field_delete_block
					);

					this->handler.setTargetBasicBlock(field_delete_block);

					this->delete_expr(data_ptr, field.typeID.asTypeID());

					this->handler.createJump(end_block);
				}

				this->handler.setTargetBasicBlock(current_block);
				const pir::Expr tag_ptr = this->handler.createCalcPtr(
					expr,
					union_pir_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".UNION_DELETE.tag_ptr")
				);
				const pir::Expr tag = this->handler.createLoad(
					tag_ptr, union_struct_type.members[1], this->name(".UNION_DELETE.tag")
				);
				this->handler.createSwitch(tag, std::move(cases), end_block);

				this->handler.setTargetBasicBlock(end_block);
			} break;

			case BaseType::Kind::ENUM: {
				evo::debugFatalBreak("Not non-trivially-deletable");
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				evo::debugFatalBreak("Not deletable");
			} break;

			case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Not deletable");
			} break;

			case BaseType::Kind::POLY_INTERFACE_REF: {
				evo::debugFatalBreak("Not non-trivially-deletable");
			} break;

			case BaseType::Kind::INTERFACE_MAP: {
				const BaseType::InterfaceMap& interface_map =
					this->context.getTypeManager().getInterfaceMap(expr_type.baseTypeID().interfaceMapID());
				return this->delete_expr(expr, interface_map.underlyingTypeID);
			} break;
		}
	}



	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::expr_copy(
		const sema::Expr& expr,
		TypeInfo::ID expr_type_id,
		bool is_initialization,
		evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
		if(this->context.getTypeManager().isTriviallyCopyable(expr_type_id)){
			if constexpr(MODE == GetExprMode::REGISTER){
				return this->get_expr_register(expr);

			}else if constexpr(MODE == GetExprMode::POINTER){
				const pir::Expr copy_alloca = 
					this->handler.createAlloca(this->get_type<false, false>(expr_type_id).type, this->name("COPY"));
				this->get_expr_store(expr, copy_alloca);
				return copy_alloca;
				
			}else if constexpr(MODE == GetExprMode::STORE){
				this->get_expr_store(expr, store_locations[0]);
				return std::nullopt;

			}else{
				return std::nullopt;
			}
		}

		return this->expr_copy<MODE>(this->get_expr_pointer(expr), expr_type_id, is_initialization, store_locations);
	}

	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::expr_copy(
		pir::Expr expr,
		TypeInfo::ID expr_type_id,
		bool is_initialization,
		evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
		evo::debugAssert(this->handler.getExprType(expr).kind() == pir::Type::Kind::PTR, "Expr must be a pointer");

		if constexpr(MODE != GetExprMode::STORE){
			evo::debugAssert(is_initialization, "this expr mode cannot be assignment");
		}

		const TypeInfo& expr_type = this->context.getTypeManager().getTypeInfo(expr_type_id);

		const pir::Type target_type = this->get_type<false, false>(expr_type_id).type;


		if(this->context.getTypeManager().isTriviallyCopyable(expr_type_id)){
			if constexpr(MODE == GetExprMode::REGISTER){
				return this->handler.createLoad(expr, target_type);

			}else if constexpr(MODE == GetExprMode::POINTER){
				const pir::Expr copy_alloca = this->handler.createAlloca(target_type, this->name("COPY"));
				this->handler.createMemcpy(copy_alloca, expr, target_type);
				return copy_alloca;
				
			}else if constexpr(MODE == GetExprMode::STORE){
				this->handler.createMemcpy(store_locations[0], expr, target_type);
				return std::nullopt;

			}else{
				return std::nullopt;
			}
		}


		const pir::Expr target = [&](){
			if constexpr(MODE == GetExprMode::REGISTER || MODE == GetExprMode::DISCARD){
				return this->handler.createAlloca(target_type, this->name(".COPY"));

			}else if constexpr(MODE == GetExprMode::POINTER){
				return this->handler.createAlloca(target_type, this->name("COPY"));

			}else if constexpr(MODE == GetExprMode::STORE){
				return store_locations[0];
			}
		}();


		if(expr_type.qualifiers().size() > 0){
			evo::debugAssert(expr_type.isOptionalNotPointer(), "Not non-trivially-copyable");

			if(is_initialization){
				const pir::Expr flag_ptr = this->handler.createCalcPtr(
					expr,
					target_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".COPY_OPT_INIT.flag_ptr")
				);
				const pir::Expr flag = this->handler.createLoad(
					flag_ptr, this->module.createBoolType(), this->name(".COPY_OPT_INIT.flag")
				);

				const pir::Expr flag_is_true = this->handler.createIEq(
					flag, this->handler.createBoolean(true), this->name(".COPY_OPT_INIT.flag_true")
				);

				const pir::BasicBlock::ID true_copy_block =
					this->handler.createBasicBlock(this->name("COPY_OPT_INIT.HAS"));
				const pir::BasicBlock::ID false_copy_block =
					this->handler.createBasicBlock(this->name("COPY_OPT_INIT.NULL"));
				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("COPY_OPT_INIT.END"));

				this->handler.createBranch(flag_is_true, true_copy_block, false_copy_block);


				//////////////////
				// has value

				this->handler.setTargetBasicBlock(true_copy_block);

				{
					const pir::Expr src_held_ptr = this->handler.createCalcPtr(
						expr, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 0}
					);
					const pir::Expr target_held_ptr = this->handler.createCalcPtr(
						target, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 0}
					);
					this->expr_copy<GetExprMode::STORE>(
						src_held_ptr,
						this->context.type_manager.getOrCreateTypeInfo(expr_type.copyWithPoppedQualifier()),
						is_initialization,
						target_held_ptr
					);

					const pir::Expr target_flag = this->handler.createCalcPtr(
						target, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 1}
					);
					this->handler.createStore(target_flag, this->handler.createBoolean(true));

					this->handler.createJump(end_block);
				}



				//////////////////
				// doesn't have value

				this->handler.setTargetBasicBlock(false_copy_block);

				{
					const pir::Expr target_flag = this->handler.createCalcPtr(
						target, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 1}
					);
					this->handler.createStore(target_flag, this->handler.createBoolean(false));

					this->handler.createJump(end_block);
				}

				//////////////////
				// end

				this->handler.setTargetBasicBlock(end_block);

			}else{
				const TypeInfo::ID held_type_id =
					this->context.type_manager.getOrCreateTypeInfo(expr_type.copyWithPoppedQualifier());


				const pir::Expr src_data_ptr = this->handler.createCalcPtr(
					expr,
					target_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 0},
					this->name(".COPY_OPT_ASSIGN.src_data_ptr")
				);


				const pir::Expr dst_data_ptr = this->handler.createCalcPtr(
					store_locations[0],
					target_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 0},
					this->name(".COPY_OPT_ASSIGN.dst_data_ptr")
				);

				const pir::Expr src_flag_ptr = this->handler.createCalcPtr(
					expr,
					target_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".COPY_OPT_ASSIGN.src_flag_ptr")
				);

				const pir::Expr src_flag =this->handler.createLoad(
					src_flag_ptr, this->module.createBoolType(), this->name(".COPY_OPT_ASSIGN.src_flag")
				);

				const pir::Expr dst_flag_ptr = this->handler.createCalcPtr(
					store_locations[0],
					target_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".COPY_OPT_ASSIGN.dst_flag_ptr")
				);

				const pir::Expr dst_flag = this->handler.createLoad(
					dst_flag_ptr, this->module.createBoolType(), this->name(".COPY_OPT_ASSIGN.dst_flag")
				);


				const pir::BasicBlock::ID flags_eq_block = this->handler.createBasicBlock("COPY_OPT_ASSIGN.FLAGS_EQ");
				const pir::BasicBlock::ID flags_true_block =
					this->handler.createBasicBlock("COPY_OPT_ASSIGN.FLAGS_TRUE");
				const pir::BasicBlock::ID flags_neq_block = this->handler.createBasicBlock("COPY_OPT_ASSIGN.FLAGS_NEQ");
				const pir::BasicBlock::ID src_flag_true_block =
					this->handler.createBasicBlock("COPY_OPT_ASSIGN.SRC_FLAG_TRUE");
				const pir::BasicBlock::ID dst_flag_true_block =
					this->handler.createBasicBlock("COPY_OPT_ASSIGN.DST_FLAG_TRUE");
				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock("COPY_OPT_ASSIGN.END");

				const pir::Expr flags_eq = 
					this->handler.createIEq(src_flag, dst_flag, this->name(".COPY_OPT_ASSIGN.flags_eq"));
				this->handler.createBranch(flags_eq, flags_eq_block, flags_neq_block);


				this->handler.setTargetBasicBlock(flags_eq_block);
				const pir::Expr flags_true = this->handler.createIEq(
					src_flag, this->handler.createBoolean(true), this->name(".COPY_OPT_ASSIGN.flags_true")
				);
				this->handler.createBranch(flags_true, flags_true_block, end_block);


				this->handler.setTargetBasicBlock(flags_true_block);
				this->expr_copy<GetExprMode::STORE>(src_data_ptr, held_type_id, false, dst_data_ptr);
				this->handler.createJump(end_block);


				this->handler.setTargetBasicBlock(flags_neq_block);
				const pir::Expr src_flag_true = this->handler.createIEq(
					src_flag, this->handler.createBoolean(true), this->name(".COPY_OPT_ASSIGN.src_flag_true")
				);
				this->handler.createBranch(src_flag_true, src_flag_true_block, dst_flag_true_block);


				this->handler.setTargetBasicBlock(src_flag_true_block);
				this->expr_copy<GetExprMode::STORE>(src_data_ptr, held_type_id, true, dst_data_ptr);
				this->handler.createStore(dst_flag_ptr, this->handler.createBoolean(true));
				this->handler.createJump(end_block);


				this->handler.setTargetBasicBlock(dst_flag_true_block);
				this->delete_expr(dst_data_ptr, held_type_id);
				this->handler.createStore(dst_flag_ptr, this->handler.createBoolean(false));
				this->handler.createJump(end_block);

				this->handler.setTargetBasicBlock(end_block);
			}

		}else{
			switch(expr_type.baseTypeID().kind()){
				case BaseType::Kind::DUMMY: {
					evo::debugFatalBreak("Not a valid type");
				} break;

				case BaseType::Kind::PRIMITIVE: {
					evo::debugFatalBreak("Not non-trivially-copyable");
				} break;

				case BaseType::Kind::FUNCTION: {
					evo::debugFatalBreak("Not non-trivially-copyable");
				} break;

				case BaseType::Kind::ARRAY: {
					const BaseType::Array& array_type =
						this->context.getTypeManager().getArray(expr_type.baseTypeID().arrayID());

					this->iterate_array(array_type, true, "COPY_ARR", [&](pir::Expr index) -> void {
						const pir::Expr src_elem = this->handler.createCalcPtr(
							expr, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, index}
						);

						const pir::Expr target_elem = this->handler.createCalcPtr(
							target, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, index}
						);

						this->expr_copy<GetExprMode::STORE>(
							src_elem, array_type.elementTypeID, is_initialization, target_elem
						);
					});
				} break;

				case BaseType::Kind::ARRAY_DEDUCER: {
					evo::debugFatalBreak("Not copyable");
				} break;

				case BaseType::Kind::ARRAY_REF: {
					evo::debugFatalBreak("Not non-trivially-copyable");
				} break;

				case BaseType::Kind::ALIAS: {
					const BaseType::Alias& alias =
						this->context.getTypeManager().getAlias(expr_type.baseTypeID().aliasID());

					this->expr_copy<MODE>(expr, alias.aliasedType, is_initialization, target);
				} break;

				case BaseType::Kind::DISTINCT_ALIAS: {
					const BaseType::DistinctAlias& distinct_alias =
						this->context.getTypeManager().getDistinctAlias(expr_type.baseTypeID().distinctAliasID());

					this->expr_copy<MODE>(expr, distinct_alias.underlyingType, is_initialization, target);
				} break;

				case BaseType::Kind::STRUCT: {
					const BaseType::Struct& struct_type =
						this->context.getTypeManager().getStruct(expr_type.baseTypeID().structID());


					const sema::Func::ID target_func_id = [&](){
						if(is_initialization){
							return *struct_type.copyInitOverload.load().funcID;
						}else{
							const std::optional<sema::Func::ID> copy_assign_overload = 
								struct_type.copyAssignOverload.load();

							if(copy_assign_overload.has_value()){
								return *copy_assign_overload;
							}else{
								this->delete_expr(target, expr_type_id);
								return *struct_type.copyInitOverload.load().funcID;
							}
						}
					}();	

					const pir::Function::ID pir_id = 
						this->data.get_func(target_func_id).pir_ids[0].as<pir::Function::ID>();

					this->create_call_void(pir_id, evo::SmallVector<pir::Expr>{expr, target});
				} break;

				case BaseType::Kind::STRUCT_TEMPLATE: {
					evo::debugFatalBreak("Not copyable");
				} break;

				case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: {
					evo::debugFatalBreak("Not copyable");
				} break;

				case BaseType::Kind::UNION: {
					const BaseType::Union& union_type =
						this->context.getTypeManager().getUnion(expr_type.baseTypeID().unionID());

					const pir::Type union_pir_type = this->get_type<false, false>(expr_type.baseTypeID()).type;
					const pir::StructType union_struct_type = this->module.getStructType(union_pir_type);

					evo::debugAssert(
						union_type.isUntagged == false, "untagged union types aren't non-trivially-copyable"
					);

					const pir::BasicBlock::ID current_block = this->handler.getTargetBasicBlock().getID();
					const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("UNION_COPY.end"));

					const pir::Expr src_data_ptr = this->handler.createCalcPtr(
						expr,
						union_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 0},
						this->name(".UNION_COPY.src_data")
					);

					const pir::Expr src_tag_ptr = this->handler.createCalcPtr(
						expr,
						union_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 1},
						this->name(".UNION_COPY.src_tag_ptr")
					);
					const pir::Expr src_tag = this->handler.createLoad(
						src_tag_ptr, union_struct_type.members[1], this->name(".UNION_COPY.src_tag")
					);

					const pir::Expr dst = [&](){
						if constexpr(MODE == GetExprMode::REGISTER){
							return this->handler.createAlloca(union_pir_type, this->name(".UNION_COPY.ALLOCA"));
							
						}else if constexpr(MODE == GetExprMode::POINTER){
							return this->handler.createAlloca(union_pir_type, this->name("UNION_COPY"));
							
						}else if constexpr(MODE == GetExprMode::STORE){
							return store_locations[0];

						}else{
							return this->handler.createAlloca(union_pir_type, this->name(".UNION_COPY_DISCARD.ALLOCA"));
						}
					}();

					const pir::Expr dst_data_ptr = this->handler.createCalcPtr(
						dst,
						union_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 0},
						this->name(".UNION_COPY.dst_data")
					);

					const pir::Expr dst_tag_ptr = this->handler.createCalcPtr(
						dst,
						union_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 1},
						this->name(".UNION_COPY.dst_tag_ptr")
					);

					auto cases = evo::SmallVector<pir::Switch::Case>();
					cases.reserve(union_type.fields.size());

					for(size_t i = 0; const BaseType::Union::Field& field : union_type.fields){
						EVO_DEFER([&](){ i += 1; });

						const pir::BasicBlock::ID field_delete_block =
							this->handler.createBasicBlockInline(this->name("UNION_COPY.field_{}", i));

						const pir::Expr case_number = this->handler.createNumber(
							union_struct_type.members[1], core::GenericInt(union_struct_type.members[1].getWidth(), i)
						);

						cases.emplace_back(case_number, field_delete_block);

						this->handler.setTargetBasicBlock(field_delete_block);

						if(field.typeID.isVoid() == false){
							this->expr_copy<GetExprMode::STORE>(
								src_data_ptr, field.typeID.asTypeID(), is_initialization, dst_data_ptr
							);
						}
						this->handler.createStore(dst_tag_ptr, case_number);

						this->handler.createJump(end_block);
					}

					this->handler.setTargetBasicBlock(current_block);
					this->handler.createSwitch(src_tag, std::move(cases), end_block);

					this->handler.setTargetBasicBlock(end_block);

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createLoad(dst, union_pir_type, this->name("UNION_COPY"));

					}else if constexpr(MODE == GetExprMode::POINTER){
						return dst;

					}else if constexpr(MODE == GetExprMode::STORE || MODE == GetExprMode::DISCARD){
						return std::nullopt;
					}
				} break;

				case BaseType::Kind::ENUM: {
					evo::debugFatalBreak("Not non-trivially-copyable");
				} break;

				case BaseType::Kind::TYPE_DEDUCER: {
					evo::debugFatalBreak("Not copyable");
				} break;

				case BaseType::Kind::INTERFACE: {
					evo::debugFatalBreak("Not copyable");
				} break;

				case BaseType::Kind::POLY_INTERFACE_REF: {
					evo::debugFatalBreak("Not non-trivially-copyable");
				} break;

				case BaseType::Kind::INTERFACE_MAP: {
					const BaseType::InterfaceMap& interface_map =
						this->context.getTypeManager().getInterfaceMap(expr_type.baseTypeID().interfaceMapID());

					return this->expr_copy<MODE>(
						expr,
						interface_map.underlyingTypeID,
						is_initialization,
						store_locations
					);
				} break;
			}
		}


		if constexpr(MODE == GetExprMode::REGISTER){
			return this->handler.createLoad(target, target_type, this->name("COPY"));
			
		}else if constexpr(MODE == GetExprMode::POINTER){
			return target;
			
		}else if constexpr(MODE == GetExprMode::STORE || MODE == GetExprMode::DISCARD){
			return std::nullopt;
		}
	}




	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::expr_move(
		const sema::Expr& expr,
		TypeInfo::ID expr_type_id,
		bool is_initialization,
		evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
		if(this->context.getTypeManager().isTriviallyCopyable(expr_type_id)){
			if constexpr(MODE == GetExprMode::REGISTER){
				return this->get_expr_register(expr);

			}else if constexpr(MODE == GetExprMode::POINTER){
				const pir::Expr move_alloca = 
					this->handler.createAlloca(this->get_type<false, false>(expr_type_id).type, this->name("MOVE"));

				this->get_expr_store(expr, move_alloca);
				return move_alloca;
				
			}else if constexpr(MODE == GetExprMode::STORE){
				this->get_expr_store(expr, store_locations[0]);
				return std::nullopt;

			}else{
				return std::nullopt;
			}
		}

		return this->expr_move<MODE>(this->get_expr_pointer(expr), expr_type_id, is_initialization, store_locations);
	}

	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::expr_move(
		pir::Expr expr,
		TypeInfo::ID expr_type_id,
		bool is_initialization,
		evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
		evo::debugAssert(this->handler.getExprType(expr).kind() == pir::Type::Kind::PTR, "Expr must be a pointer");

		if constexpr(MODE != GetExprMode::STORE){
			evo::debugAssert(is_initialization, "this expr mode cannot be assignment");
		}

		const TypeInfo& expr_type = this->context.getTypeManager().getTypeInfo(expr_type_id);

		const pir::Type target_type = this->get_type<false, false>(expr_type_id).type;


		if(this->context.getTypeManager().isTriviallyCopyable(expr_type_id)){
			if constexpr(MODE == GetExprMode::REGISTER){
				return this->handler.createLoad(expr, target_type);

			}else if constexpr(MODE == GetExprMode::POINTER){
				const pir::Expr move_alloca = this->handler.createAlloca(target_type, this->name("MOVE"));
				this->handler.createMemcpy(move_alloca, expr, target_type);
				return move_alloca;
				
			}else if constexpr(MODE == GetExprMode::STORE){
				this->handler.createMemcpy(store_locations[0], expr, target_type);
				return std::nullopt;

			}else{
				return std::nullopt;
			}
		}

		const pir::Expr target = [&](){
			if constexpr(MODE == GetExprMode::REGISTER || MODE == GetExprMode::DISCARD){
				return this->handler.createAlloca(target_type, this->name(".MOVE"));

			}else if constexpr(MODE == GetExprMode::POINTER){
				return this->handler.createAlloca(target_type, this->name("MOVE"));

			}else if constexpr(MODE == GetExprMode::STORE){
				return store_locations[0];
			}
		}();


		if(expr_type.qualifiers().size() > 0){
			evo::debugAssert(expr_type.isOptionalNotPointer(), "Not non-trivially-movable");

			if(is_initialization){
				const pir::Expr flag_ptr = this->handler.createCalcPtr(
					expr, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 1}
				);
				const pir::Expr flag = this->handler.createLoad(flag_ptr, this->module.createBoolType());

				const pir::Expr flag_is_true = this->handler.createIEq(flag, this->handler.createBoolean(true));

				const pir::BasicBlock::ID true_move_block = this->handler.createBasicBlock(this->name("MOVE_OPT.HAS"));
				const pir::BasicBlock::ID false_move_block =
					this->handler.createBasicBlock(this->name("MOVE_OPT.NULL"));
				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("MOVE_OPT.END"));

				this->handler.createBranch(flag_is_true, true_move_block, false_move_block);


				//////////////////
				// has value

				this->handler.setTargetBasicBlock(true_move_block);

				{
					const pir::Expr src_held_ptr = this->handler.createCalcPtr(
						expr, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 0}
					);
					const pir::Expr target_held_ptr = this->handler.createCalcPtr(
						target, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 0}
					);
					this->expr_move<GetExprMode::STORE>(
						src_held_ptr,
						this->context.type_manager.getOrCreateTypeInfo(expr_type.copyWithPoppedQualifier()),
						is_initialization,
						target_held_ptr
					);

					const pir::Expr target_flag = this->handler.createCalcPtr(
						target, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 1}
					);
					this->handler.createStore(target_flag, this->handler.createBoolean(true));

					this->handler.createJump(end_block);
				}


				//////////////////
				// doesn't have value

				this->handler.setTargetBasicBlock(false_move_block);

				{
					const pir::Expr target_flag = this->handler.createCalcPtr(
						target, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, 1}
					);
					this->handler.createStore(target_flag, this->handler.createBoolean(false));

					this->handler.createJump(end_block);
				}

				//////////////////
				// end

				this->handler.setTargetBasicBlock(end_block);

			}else{
				const TypeInfo::ID held_type_id =
					this->context.type_manager.getOrCreateTypeInfo(expr_type.copyWithPoppedQualifier());


				const pir::Expr src_data_ptr = this->handler.createCalcPtr(
					expr,
					target_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 0},
					this->name(".MOVE_OPT_ASSIGN.src_data_ptr")
				);


				const pir::Expr dst_data_ptr = this->handler.createCalcPtr(
					store_locations[0],
					target_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 0},
					this->name(".MOVE_OPT_ASSIGN.dst_data_ptr")
				);

				const pir::Expr src_flag_ptr = this->handler.createCalcPtr(
					expr,
					target_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".MOVE_OPT_ASSIGN.src_flag_ptr")
				);

				const pir::Expr src_flag =this->handler.createLoad(
					src_flag_ptr, this->module.createBoolType(), this->name(".MOVE_OPT_ASSIGN.src_flag")
				);

				const pir::Expr dst_flag_ptr = this->handler.createCalcPtr(
					store_locations[0],
					target_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					this->name(".MOVE_OPT_ASSIGN.dst_flag_ptr")
				);

				const pir::Expr dst_flag = this->handler.createLoad(
					dst_flag_ptr, this->module.createBoolType(), this->name(".MOVE_OPT_ASSIGN.dst_flag")
				);


				const pir::BasicBlock::ID flags_eq_block = this->handler.createBasicBlock("MOVE_OPT_ASSIGN.FLAGS_EQ");
				const pir::BasicBlock::ID flags_true_block =
					this->handler.createBasicBlock("MOVE_OPT_ASSIGN.FLAGS_TRUE");
				const pir::BasicBlock::ID flags_neq_block = this->handler.createBasicBlock("MOVE_OPT_ASSIGN.FLAGS_NEQ");
				const pir::BasicBlock::ID src_flag_true_block =
					this->handler.createBasicBlock("MOVE_OPT_ASSIGN.SRC_FLAG_TRUE");
				const pir::BasicBlock::ID dst_flag_true_block =
					this->handler.createBasicBlock("MOVE_OPT_ASSIGN.DST_FLAG_TRUE");
				const pir::BasicBlock::ID end_block = this->handler.createBasicBlock("MOVE_OPT_ASSIGN.END");

				const pir::Expr flags_eq = 
					this->handler.createIEq(src_flag, dst_flag, this->name(".MOVE_OPT_ASSIGN.flags_eq"));
				this->handler.createBranch(flags_eq, flags_eq_block, flags_neq_block);


				this->handler.setTargetBasicBlock(flags_eq_block);
				const pir::Expr flags_true = this->handler.createIEq(
					src_flag, this->handler.createBoolean(true), this->name(".MOVE_OPT_ASSIGN.flags_true")
				);
				this->handler.createBranch(flags_true, flags_true_block, end_block);


				this->handler.setTargetBasicBlock(flags_true_block);
				this->expr_move<GetExprMode::STORE>(src_data_ptr, held_type_id, false, dst_data_ptr);
				this->handler.createStore(src_flag_ptr, this->handler.createBoolean(false));
				this->handler.createJump(end_block);


				this->handler.setTargetBasicBlock(flags_neq_block);
				const pir::Expr src_flag_true = this->handler.createIEq(
					src_flag, this->handler.createBoolean(true), this->name(".MOVE_OPT_ASSIGN.src_flag_true")
				);
				this->handler.createBranch(src_flag_true, src_flag_true_block, dst_flag_true_block);


				this->handler.setTargetBasicBlock(src_flag_true_block);
				this->expr_move<GetExprMode::STORE>(src_data_ptr, held_type_id, true, dst_data_ptr);
				this->handler.createStore(src_flag_ptr, this->handler.createBoolean(false));
				this->handler.createStore(dst_flag_ptr, this->handler.createBoolean(true));
				this->handler.createJump(end_block);


				this->handler.setTargetBasicBlock(dst_flag_true_block);
				this->delete_expr(dst_data_ptr, held_type_id);
				this->handler.createStore(dst_flag_ptr, this->handler.createBoolean(false));
				this->handler.createJump(end_block);

				this->handler.setTargetBasicBlock(end_block);
			}

		}else{
			switch(expr_type.baseTypeID().kind()){
				case BaseType::Kind::DUMMY: {
					evo::debugFatalBreak("Not a valid type");
				} break;

				case BaseType::Kind::PRIMITIVE: {
					evo::debugFatalBreak("Not non-trivially-movable");
				} break;

				case BaseType::Kind::FUNCTION: {
					evo::debugFatalBreak("Not non-trivially-movable");
				} break;

				case BaseType::Kind::ARRAY: {
					const BaseType::Array& array_type =
						this->context.getTypeManager().getArray(expr_type.baseTypeID().arrayID());

					this->iterate_array(array_type, false, "MOVE_ARR", [&](pir::Expr index) -> void {
						const pir::Expr src_elem = this->handler.createCalcPtr(
							expr, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, index}
						);

						const pir::Expr target_elem = this->handler.createCalcPtr(
							target, target_type, evo::SmallVector<pir::CalcPtr::Index>{0, index}
						);

						this->expr_move<GetExprMode::STORE>(
							src_elem, array_type.elementTypeID, is_initialization, target_elem
						);
					});
				} break;

				case BaseType::Kind::ARRAY_DEDUCER: {
					evo::debugFatalBreak("Not movable");
				} break;

				case BaseType::Kind::ARRAY_REF: {
					evo::debugFatalBreak("Not non-trivially-movable");
				} break;

				case BaseType::Kind::ALIAS: {
					const BaseType::Alias& alias =
						this->context.getTypeManager().getAlias(expr_type.baseTypeID().aliasID());

					this->expr_move<MODE>(expr, alias.aliasedType, is_initialization, target);
				} break;

				case BaseType::Kind::DISTINCT_ALIAS: {
					const BaseType::DistinctAlias& distinct_alias =
						this->context.getTypeManager().getDistinctAlias(expr_type.baseTypeID().distinctAliasID());

					this->expr_move<MODE>(expr, distinct_alias.underlyingType, is_initialization, target);
				} break;

				case BaseType::Kind::STRUCT: {
					const BaseType::Struct& struct_type =
						this->context.getTypeManager().getStruct(expr_type.baseTypeID().structID());

					const sema::Func::ID target_func_id = [&](){
						if(is_initialization){
							return *struct_type.moveInitOverload.load().funcID;
						}else{
							const std::optional<sema::Func::ID> move_assign_overload = 
								struct_type.moveAssignOverload.load();

							if(move_assign_overload.has_value()){
								return *move_assign_overload;
							}else{
								return *struct_type.moveInitOverload.load().funcID;
							}
						}
					}();	

					const pir::Function::ID pir_id = 
						this->data.get_func(target_func_id).pir_ids[0].as<pir::Function::ID>();

					this->create_call_void(pir_id, evo::SmallVector<pir::Expr>{expr, target});
				} break;

				case BaseType::Kind::STRUCT_TEMPLATE: {
					evo::debugFatalBreak("Not movable");
				} break;

				case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: {
					evo::debugFatalBreak("Not movable");
				} break;

				case BaseType::Kind::UNION: {
					const BaseType::Union& union_type =
						this->context.getTypeManager().getUnion(expr_type.baseTypeID().unionID());

					const pir::Type union_pir_type = this->get_type<false, false>(expr_type.baseTypeID()).type;
					const pir::StructType union_struct_type = this->module.getStructType(union_pir_type);

					evo::debugAssert(
						union_type.isUntagged == false, "untagged union types aren't non-trivially-movable"
					);

					const pir::BasicBlock::ID current_block = this->handler.getTargetBasicBlock().getID();
					const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("UNION_MOVE.end"));

					const pir::Expr src_data_ptr = this->handler.createCalcPtr(
						expr,
						union_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 0},
						this->name(".UNION_MOVE.src_data")
					);

					const pir::Expr src_flag_ptr = this->handler.createCalcPtr(
						expr,
						union_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 1},
						this->name(".UNION_MOVE.src_flag_ptr")
					);
					const pir::Expr src_flag = this->handler.createLoad(
						src_flag_ptr, union_struct_type.members[1], this->name(".UNION_MOVE.src_flag")
					);

					const pir::Expr dst = [&](){
						if constexpr(MODE == GetExprMode::REGISTER){
							return this->handler.createAlloca(union_pir_type, this->name(".UNION_MOVE.ALLOCA"));
							
						}else if constexpr(MODE == GetExprMode::POINTER){
							return this->handler.createAlloca(union_pir_type, this->name("UNION_MOVE"));
							
						}else if constexpr(MODE == GetExprMode::STORE){
							return store_locations[0];

						}else{
							return this->handler.createAlloca(union_pir_type, this->name(".UNION_MOVE_DISCARD.ALLOCA"));
						}
					}();

					const pir::Expr dst_data_ptr = this->handler.createCalcPtr(
						dst,
						union_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 0},
						this->name(".UNION_MOVE.dst_data")
					);

					const pir::Expr dst_flag_ptr = this->handler.createCalcPtr(
						dst,
						union_pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 1},
						this->name(".UNION_MOVE.dst_flag_ptr")
					);

					auto cases = evo::SmallVector<pir::Switch::Case>();
					cases.reserve(union_type.fields.size());

					for(size_t i = 0; const BaseType::Union::Field& field : union_type.fields){
						EVO_DEFER([&](){ i += 1; });

						const pir::BasicBlock::ID field_delete_block =
							this->handler.createBasicBlockInline(this->name("UNION_MOVE.field_{}", i));

						const pir::Expr case_number = this->handler.createNumber(
							union_struct_type.members[1], core::GenericInt(union_struct_type.members[1].getWidth(), i)
						);

						cases.emplace_back(case_number, field_delete_block);

						this->handler.setTargetBasicBlock(field_delete_block);

						if(field.typeID.isVoid() == false){
							this->expr_move<GetExprMode::STORE>(
								src_data_ptr, field.typeID.asTypeID(), is_initialization, dst_data_ptr
							);
						}
						this->handler.createStore(dst_flag_ptr, case_number);

						this->handler.createJump(end_block);
					}

					this->handler.setTargetBasicBlock(current_block);
					this->handler.createSwitch(src_flag, std::move(cases), end_block);

					this->handler.setTargetBasicBlock(end_block);

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createLoad(dst, union_pir_type, this->name("UNION_MOVE"));

					}else if constexpr(MODE == GetExprMode::POINTER){
						return dst;

					}else if constexpr(MODE == GetExprMode::STORE || MODE == GetExprMode::DISCARD){
						return std::nullopt;
					}
				} break;

				case BaseType::Kind::ENUM: {
					evo::debugFatalBreak("Not non-trivially-movable");
				} break;

				case BaseType::Kind::TYPE_DEDUCER: {
					evo::debugFatalBreak("Not movable");
				} break;

				case BaseType::Kind::INTERFACE: {
					evo::debugFatalBreak("Not movable");
				} break;

				case BaseType::Kind::POLY_INTERFACE_REF: {
					evo::debugFatalBreak("Not non-trivially-movable");
				} break;

				case BaseType::Kind::INTERFACE_MAP: {
					const BaseType::InterfaceMap& interface_map =
						this->context.getTypeManager().getInterfaceMap(expr_type.baseTypeID().interfaceMapID());
					return this->expr_move<MODE>(
						expr,
						interface_map.underlyingTypeID,
						is_initialization,
						store_locations
					);
				} break;
			}
		}


		if constexpr(MODE == GetExprMode::REGISTER){
			return this->handler.createLoad(target, target_type, this->name("MOVE"));
			
		}else if constexpr(MODE == GetExprMode::POINTER){
			return target;
			
		}else if constexpr(MODE == GetExprMode::STORE || MODE == GetExprMode::DISCARD){
			return std::nullopt;
		}
	}





	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::expr_cmp(
		TypeInfo::ID expr_type_id,
		const sema::Expr& lhs,
		const sema::Expr& rhs,
		bool is_equal,
		evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
		if(this->context.getTypeManager().isTriviallyComparable(expr_type_id)){
			auto lhs_register = std::optional<pir::Expr>();
			auto rhs_register = std::optional<pir::Expr>();

			const pir::Type pir_type = this->get_type<false, false>(expr_type_id).type;

			switch(pir_type.kind()){
				case pir::Type::Kind::UNSIGNED: case pir::Type::Kind::SIGNED:
				case pir::Type::Kind::BOOL:     case pir::Type::Kind::PTR: {
					lhs_register = this->get_expr_register(lhs);
					rhs_register = this->get_expr_register(rhs);
				} break;

				default: {
					const uint32_t expr_type_num_bits =
						uint32_t(this->context.getTypeManager().numBits(expr_type_id, false));
					const pir::Type target_pir_type = this->module.createUnsignedType(expr_type_num_bits);

					lhs_register = this->handler.createLoad(this->get_expr_pointer(lhs), target_pir_type);
					rhs_register = this->handler.createLoad(this->get_expr_pointer(rhs), target_pir_type);
				} break;
			}

			if constexpr(MODE == GetExprMode::REGISTER){
				if(is_equal){
					return this->handler.createIEq(*lhs_register, *rhs_register, this->name("EQ"));
				}else{
					return this->handler.createINeq(*lhs_register, *rhs_register, this->name("NEQ"));
				}
				
			}else if constexpr(MODE == GetExprMode::POINTER){
				const pir::Expr output_alloca =
					this->handler.createAlloca(pir_type, this->name(is_equal ? "EQ" : "NEQ"));

				const pir::Expr output_value = [&](){
					if(is_equal){
						return this->handler.createIEq(*lhs_register, *rhs_register, this->name(".EQ"));
					}else{
						return this->handler.createINeq(*lhs_register, *rhs_register, this->name(".NEQ"));
					}
				}();

				this->handler.createStore(output_alloca, output_value);
				return output_alloca;
				
			}else if constexpr(MODE == GetExprMode::STORE){
				const pir::Expr output_value = [&](){
					if(is_equal){
						return this->handler.createIEq(*lhs_register, *rhs_register, this->name(".EQ"));
					}else{
						return this->handler.createINeq(*lhs_register, *rhs_register, this->name(".NEQ"));
					}
				}();

				this->handler.createStore(store_locations[0], output_value);
				return std::nullopt;
				
			}else{
				return std::nullopt;
			}

		}else{
			return this->expr_cmp<MODE>(
				expr_type_id, this->get_expr_pointer(lhs), this->get_expr_pointer(rhs), is_equal, store_locations
			);
		}
	}



	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::expr_cmp(
		TypeInfo::ID expr_type_id,
		pir::Expr lhs,
		pir::Expr rhs,
		bool is_equal,
		evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
		const TypeInfo& expr_type = this->context.getTypeManager().getTypeInfo(expr_type_id);
		const pir::Type pir_type = this->get_type<false, false>(expr_type_id).type;

		if(this->context.getTypeManager().isTriviallyComparable(expr_type_id)){
			const uint32_t expr_type_num_bits =
				uint32_t(this->context.getTypeManager().numBits(expr_type_id, false));
			const pir::Type target_pir_type = this->module.createUnsignedType(expr_type_num_bits);

			const pir::Expr lhs_register = this->handler.createLoad(lhs, target_pir_type);
			const pir::Expr rhs_register = this->handler.createLoad(rhs, target_pir_type);


			if constexpr(MODE == GetExprMode::REGISTER){
				if(is_equal){
					return this->handler.createIEq(lhs_register, rhs_register, this->name("EQ"));
				}else{
					return this->handler.createINeq(lhs_register, rhs_register, this->name("NEQ"));
				}
				
			}else if constexpr(MODE == GetExprMode::POINTER){
				const pir::Expr output_alloca =
					this->handler.createAlloca(pir_type, this->name(is_equal ? "EQ" : "NEQ"));

				const pir::Expr output_value = [&](){
					if(is_equal){
						return this->handler.createIEq(lhs_register, rhs_register, this->name(".EQ"));
					}else{
						return this->handler.createINeq(lhs_register, rhs_register, this->name(".NEQ"));
					}
				}();

				this->handler.createStore(output_alloca, output_value);
				return output_alloca;
				
			}else if constexpr(MODE == GetExprMode::STORE){
				const pir::Expr output_value = [&](){
					if(is_equal){
						return this->handler.createIEq(lhs_register, rhs_register, this->name(".EQ"));
					}else{
						return this->handler.createINeq(lhs_register, rhs_register, this->name(".NEQ"));
					}
				}();

				this->handler.createStore(store_locations[0], output_value);
				return std::nullopt;
				
			}else{
				return std::nullopt;
			}

		}else{
			if(expr_type.qualifiers().empty() == false){
				evo::debugAssert(
					expr_type.isOptionalNotPointer(), "Unknown type with qualifiers that isn't trivially comparable"
				);
				
				const pir::Expr lhs_flag_ptr = this->handler.createCalcPtr(
					lhs,
					pir_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					is_equal ? this->name(".EQ.OPT.LHS.FLAG_PTR") : this->name(".NEQ.OPT.LHS.FLAG_PTR")
				);

				const pir::Expr lhs_flag = this->handler.createLoad(
					lhs_flag_ptr,
					this->module.createBoolType(),
					is_equal ? this->name(".EQ.OPT.LHS.FLAG") : this->name(".NEQ.OPT.LHS.FLAG")
				);

				const pir::Expr rhs_flag_ptr = this->handler.createCalcPtr(
					rhs,
					pir_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 1},
					is_equal ? this->name(".EQ.OPT.RHS.FLAG_PTR") : this->name(".NEQ.OPT.RHS.FLAG_PTR")
				);

				const pir::Expr rhs_flag = this->handler.createLoad(
					rhs_flag_ptr,
					this->module.createBoolType(),
					is_equal ? this->name(".EQ.OPT.RHS.FLAG") : this->name(".NEQ.OPT.RHS.FLAG")
				);

				const pir::BasicBlock::ID start_basic_block = this->handler.getTargetBasicBlock().getID();

				const pir::BasicBlock::ID has_flag_basic_block = this->handler.createBasicBlock(
					is_equal ? this->name(".EQ.OPT.HAS_FLAG") : this->name(".NEQ.OPT.HAS_FLAG")
				);

				this->handler.setTargetBasicBlock(has_flag_basic_block);

				const pir::Expr lhs_data_ptr = this->handler.createCalcPtr(
					lhs,
					pir_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 0},
					is_equal ? this->name(".EQ.OPT.LHS.DATA_PTR") : this->name(".NEQ.OPT.LHS.DATA_PTR")
				);

				const pir::Expr rhs_data_ptr = this->handler.createCalcPtr(
					rhs,
					pir_type,
					evo::SmallVector<pir::CalcPtr::Index>{0, 0},
					is_equal ? this->name(".EQ.OPT.RHS.DATA_PTR") : this->name(".NEQ.OPT.RHS.DATA_PTR")
				);


				const pir::Expr has_flag_result = *this->expr_cmp<GetExprMode::REGISTER>(
					this->context.type_manager.getOrCreateTypeInfo(expr_type.copyWithPoppedQualifier()),
					lhs_data_ptr,
					rhs_data_ptr,
					is_equal,
					nullptr
				);

				const pir::BasicBlock::ID has_flag_result_end_block = this->handler.getTargetBasicBlock().getID();

				const pir::BasicBlock::ID end_basic_block = this->handler.createBasicBlock(
					is_equal ? this->name(".EQ.OPT.END") : this->name(".NEQ.OPT.END")
				);

				this->handler.createJump(end_basic_block);

				this->handler.setTargetBasicBlock(start_basic_block);
				const pir::Expr has_flag_check = this->handler.createAnd(
					lhs_flag, rhs_flag, is_equal ? this->name(".EQ.OPT.HAS_FLAG") : this->name(".NEQ.OPT.HAS_FLAG")
				);
				this->handler.createBranch(has_flag_check, has_flag_basic_block, end_basic_block);

				this->handler.setTargetBasicBlock(end_basic_block);


				if constexpr(MODE == GetExprMode::REGISTER){
					return this->handler.createPhi(
						evo::SmallVector<pir::Phi::Predecessor>{
							pir::Phi::Predecessor(start_basic_block, this->handler.createBoolean(!is_equal)),
							pir::Phi::Predecessor(has_flag_result_end_block, has_flag_result),
						},
						this->name(is_equal ? "EQ.OPT" : "NEQ.OPT")
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr output_value = this->handler.createPhi(
						evo::SmallVector<pir::Phi::Predecessor>{
							pir::Phi::Predecessor(start_basic_block, this->handler.createBoolean(!is_equal)),
							pir::Phi::Predecessor(has_flag_result_end_block, has_flag_result),
						},
						this->name(is_equal ? ".EQ.OPT" : ".NEQ.OPT")
					);

					const pir::Expr output_alloca = this->handler.createAlloca(
						this->module.createBoolType(), this->name(is_equal ? "EQ.OPT" : "NEQ.OPT")
					);
					this->handler.createStore(output_alloca, output_value);

					return output_alloca;
					
				}else if constexpr(MODE == GetExprMode::STORE){
					const pir::Expr output_value = this->handler.createPhi(
						evo::SmallVector<pir::Phi::Predecessor>{
							pir::Phi::Predecessor(start_basic_block, this->handler.createBoolean(!is_equal)),
							pir::Phi::Predecessor(has_flag_result_end_block, has_flag_result),
						},
						this->name(is_equal ? ".EQ.OPT" : ".NEQ.OPT")
					);

					this->handler.createStore(store_locations[0], output_value);

					return std::nullopt;
					
				}else{
					return std::nullopt;
				}
			}


			switch(expr_type.baseTypeID().kind()){
				case BaseType::Kind::DUMMY: evo::debugFatalBreak("Invalid type");

				case BaseType::Kind::PRIMITIVE: {
					evo::debugAssert(
						this->context.getTypeManager().isFloatingPoint(expr_type.baseTypeID()),
						"This primitive is trivially comparable"
					);

					if constexpr(MODE == GetExprMode::DISCARD){
						return std::nullopt;

					}else{
						const pir::Expr target_lhs = this->handler.createLoad(
							lhs, pir_type, is_equal ? this->name(".EQ.LHS") : this->name(".NEQ.LHS")
						);

						const pir::Expr target_rhs = this->handler.createLoad(
							rhs, pir_type, is_equal ? this->name(".EQ.RHS") : this->name(".NEQ.RHS")
						);

						if constexpr(MODE == GetExprMode::REGISTER){
							if(is_equal){
								return this->handler.createFEq(target_lhs, target_rhs, this->name("EQ"));
							}else{
								return this->handler.createFNeq(target_lhs, target_rhs, this->name("NEQ"));
							}

						}else if constexpr(MODE == GetExprMode::POINTER){
							if(is_equal){
								const pir::Expr output_alloca =
									this->handler.createAlloca(this->module.createBoolType(), this->name("EQ"));

								const pir::Expr output_value =
									this->handler.createFEq(target_lhs, target_rhs, this->name(".EQ"));
								this->handler.createStore(output_alloca, output_value);
								return output_alloca;
							}else{
								const pir::Expr output_alloca =
									this->handler.createAlloca(this->module.createBoolType(), this->name("NEQ"));

								const pir::Expr output_value =
									this->handler.createFNeq(target_lhs, target_rhs, this->name(".NEQ"));
								this->handler.createStore(output_alloca, output_value);
								return output_alloca;
							}
							
						}else if constexpr(MODE == GetExprMode::STORE){
							const pir::Expr output_value = [&](){
								if(is_equal){
									return this->handler.createFEq(target_lhs, target_rhs, this->name(".EQ"));
								}else{
									return this->handler.createFNeq(target_lhs, target_rhs, this->name(".NEQ"));
								}
							}();

							this->handler.createStore(store_locations[0], output_value);
							return std::nullopt;
						}
					}
				} break;

				case BaseType::Kind::FUNCTION: {
					evo::debugFatalBreak("function is trivially comparable");
				} break;

				case BaseType::Kind::ARRAY: {
					// const BaseType::Array& array_type = this->getArray(expr_type.baseTypeID().arrayID());
					const BaseType::Array& array_type =
						this->context.getTypeManager().getArray(expr_type.baseTypeID().arrayID());

					const uint64_t num_elems = [&](){
						uint64_t output_num_elems = 1;

						for(uint64_t dimension : array_type.dimensions){
							output_num_elems *= dimension;
						}

						if(array_type.terminator.has_value()){ output_num_elems += 1; }

						return output_num_elems;
					}();

					const pir::Type usize_type = this->get_type<false, false>(TypeManager::getTypeUSize()).type;

					const pir::Expr pir_i = this->handler.createAlloca(
						usize_type, this->name(is_equal ? ".EQ.arr.i.ALLOCA" : ".NEQ.arr.i.ALLOCA")
					);
					this->handler.createStore(
						pir_i,
						this->handler.createNumber(
							usize_type, core::GenericInt(unsigned(this->context.getTypeManager().numBitsOfPtr()), 0)
						)
					);

					const pir::BasicBlock::ID cond_block = this->handler.createBasicBlock(
						this->name(is_equal ? "EQ.arr.cond" : "NEQ.arr.cond")
					);
					const pir::BasicBlock::ID then_block = this->handler.createBasicBlock(
						this->name(is_equal ? "EQ.arr.then" : "NEQ.arr.then")
					);
					const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(
						this->name(is_equal ? "EQ.arr.end" : "NEQ.arr.end")
					);

					this->handler.createJump(cond_block);


					//////////////////
					// cond

					this->handler.setTargetBasicBlock(cond_block);

					const pir::Expr array_size = this->handler.createNumber(
						usize_type, core::GenericInt(unsigned(this->context.getTypeManager().numBitsOfPtr()), num_elems)
					);

					const pir::Expr pir_i_load = this->handler.createLoad(
						pir_i, usize_type, this->name(is_equal ? "EQ.arr.i" : "NEQ.arr.i")
					);
					const pir::Expr cond = this->handler.createULT(
						pir_i_load, array_size, this->name(is_equal ? "EQ.arr.LOOP_COND" : "NEQ.arr.LOOP_COND")
					);

					this->handler.createBranch(cond, then_block, end_block);


					//////////////////
					// then

					this->handler.setTargetBasicBlock(then_block);

					const pir::Expr lhs_elem = this->handler.createCalcPtr(
						lhs,
						pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, pir_i_load},
						this->name(is_equal ? ".EQ.ARR.LHS_PTR" : ".NEQ.ARR.LHS_PTR")
					);

					const pir::Expr rhs_elem = this->handler.createCalcPtr(
						rhs,
						pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, pir_i_load},
						this->name(is_equal ? ".EQ.ARR.RHS_PTR" : ".NEQ.ARR.RHS_PTR")
					);

					const pir::Expr cmp_value = *this->expr_cmp<GetExprMode::REGISTER>(
						array_type.elementTypeID, lhs_elem, rhs_elem, is_equal, nullptr
					);


					const pir::Expr i_increment = this->handler.createAdd(
						pir_i_load,
						this->handler.createNumber(
							usize_type, core::GenericInt(unsigned(this->context.getTypeManager().numBitsOfPtr()), 1)
						),
						false,
						true
					);
					this->handler.createStore(pir_i, i_increment);

					if(is_equal){
						this->handler.createBranch(cmp_value, cond_block, end_block);
					}else{
						this->handler.createBranch(cmp_value, end_block, cond_block);
					}


					//////////////////
					// end

					this->handler.setTargetBasicBlock(end_block);

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createPhi(
							evo::SmallVector<pir::Phi::Predecessor>{
								pir::Phi::Predecessor(then_block, this->handler.createBoolean(!is_equal)),
								pir::Phi::Predecessor(cond_block, this->handler.createBoolean(is_equal)),
							},
							is_equal ? this->name("EQ.ARR") : this->name("NEQ.ARR")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr output_value = this->handler.createPhi(
							evo::SmallVector<pir::Phi::Predecessor>{
								pir::Phi::Predecessor(then_block, this->handler.createBoolean(!is_equal)),
								pir::Phi::Predecessor(cond_block, this->handler.createBoolean(is_equal)),
							},
							is_equal ? this->name(".EQ.ARR") : this->name(".NEQ.ARR")
						);

						const pir::Expr output_alloca = this->handler.createAlloca(
							this->module.createBoolType(), is_equal ? this->name("EQ.ARR") : this->name("NEQ.ARR")
						);
						this->handler.createStore(output_alloca, output_value);

						return output_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						const pir::Expr output_value = this->handler.createPhi(
							evo::SmallVector<pir::Phi::Predecessor>{
								pir::Phi::Predecessor(then_block, this->handler.createBoolean(!is_equal)),
								pir::Phi::Predecessor(cond_block, this->handler.createBoolean(is_equal)),
							},
							is_equal ? this->name(".EQ.ARR") : this->name(".NEQ.ARR")
						);

						this->handler.createStore(store_locations[0], output_value);

						return std::nullopt;
						
					}else{
						return std::nullopt;
					}

				} break;

				case BaseType::Kind::ARRAY_REF: {
					const BaseType::ArrayRef& array_ref_type =
						this->context.getTypeManager().getArrayRef(expr_type.baseTypeID().arrayRefID());


					const pir::Expr lhs_ref_ptrs_ptr = this->handler.createCalcPtr(
						lhs,
						pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 1},
						this->name(is_equal ? ".EQ.ARR_REF.LHS_REF_PTRS_PTR" : ".NEQ.ARR_REF.LHS_REF_PTRS_PTR")
					);

					const pir::Expr rhs_ref_ptrs_ptr = this->handler.createCalcPtr(
						rhs,
						pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 1},
						this->name(is_equal ? ".EQ.ARR_REF.RHS_REF_PTRS_PTR" : ".NEQ.ARR_REF.RHS_REF_PTRS_PTR")
					);

					const pir::Type ref_ptrs_int_type = this->module.createUnsignedType(
						unsigned(array_ref_type.getNumRefPtrs() * this->context.getTypeManager().numBitsOfPtr())
					);

					const pir::Expr lhs_ref_ptrs = this->handler.createLoad(
						lhs_ref_ptrs_ptr,
						ref_ptrs_int_type,
						this->name(is_equal ? ".EQ.ARR_REF.LHS_REF_PTRS" : ".NEQ.ARR_REF.LHS_REF_PTRS")
					);

					const pir::Expr rhs_ref_ptrs = this->handler.createLoad(
						rhs_ref_ptrs_ptr,
						ref_ptrs_int_type,
						this->name(is_equal ? ".EQ.ARR_REF.RHS_REF_PTRS" : ".NEQ.ARR_REF.RHS_REF_PTRS")
					);

					const pir::Expr ref_ptrs_cmp = [&](){
						if(is_equal){
							return this->handler.createIEq(
								lhs_ref_ptrs,
								rhs_ref_ptrs,
								this->name(is_equal ? ".EQ.ARR_REF.REF_PTRS_CMP" : ".NEQ.ARR_REF.REF_PTRS_CMP")
							);
						}else{
							return this->handler.createINeq(
								lhs_ref_ptrs,
								rhs_ref_ptrs,
								this->name(is_equal ? ".EQ.ARR_REF.REF_PTRS_CMP" : ".NEQ.ARR_REF.REF_PTRS_CMP")
							);
						}
					}();


					const pir::BasicBlock::ID start_basic_block = this->handler.getTargetBasicBlock().getID();

					const pir::BasicBlock::ID element_check_block = this->handler.createBasicBlock(
						this->name(is_equal ? "EQ.ARR_REF.elem_check" : "NEQ.ARR_REF.elem_check")
					);

					this->handler.setTargetBasicBlock(element_check_block);


					///////////////////////////////////
					// element check

					//////////////////
					// calc size

					const pir::Type type_usize = this->get_type<false, false>(TypeManager::getTypeUSize()).type;


					uint32_t ref_length_index = 0;
					auto size_expr = std::optional<pir::Expr>();

					for(const BaseType::ArrayRef::Dimension& dimension : array_ref_type.dimensions){
						const pir::Expr length_num = [&](){
							if(dimension.isPtr()){
								const pir::Expr length_load = this->handler.createLoad(
									this->handler.createCalcPtr(
										lhs,
										pir_type,
										evo::SmallVector<pir::CalcPtr::Index>{0, ref_length_index + 1}
									),
									type_usize
								);

								ref_length_index += 1;

								return length_load;

							}else{
								return this->handler.createNumber(
									type_usize,
									core::GenericInt(
										unsigned(this->context.getTypeManager().numBitsOfPtr()), dimension.length()
									)
								);
							}
						}();

						if(size_expr.has_value()){
							*size_expr = this->handler.createMul(*size_expr, length_num, true, false);
						}else{
							size_expr = length_num;
						}
					}


					//////////////////
					// data ptrs

					const pir::Expr lhs_data_ptr_ptr = this->handler.createCalcPtr(
						lhs,
						pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 0},
						this->name(is_equal ? ".EQ.ARR_REF.LHS_DATA_PTR_PTR" : ".NEQ.ARR_REF.LHS_DATA_PTR_PTR")
					);

					const pir::Expr rhs_data_ptr_ptr = this->handler.createCalcPtr(
						rhs,
						pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 0},
						this->name(is_equal ? ".EQ.ARR_REF.RHS_DATA_PTR_PTR" : ".NEQ.ARR_REF.RHS_DATA_PTR_PTR")
					);

					const pir::Expr lhs_data_ptr = this->handler.createLoad(
						lhs_data_ptr_ptr,
						this->module.createPtrType(),
						this->name(is_equal ? ".EQ.ARR_REF.LHS_DATA_PTR" : ".NEQ.ARR_REF.LHS_DATA_PTR")
					);

					const pir::Expr rhs_data_ptr = this->handler.createLoad(
						rhs_data_ptr_ptr,
						this->module.createPtrType(),
						this->name(is_equal ? ".EQ.ARR_REF.RHS_DATA_PTR" : ".NEQ.ARR_REF.RHS_DATA_PTR")
					);


					//////////////////
					// elem check

					const pir::Type usize_type = this->get_type<false, false>(TypeManager::getTypeUSize()).type;

					const pir::Expr pir_i = this->handler.createAlloca(
						usize_type, this->name(is_equal ? ".EQ.ARR_REF.i.ALLOCA" : ".NEQ.ARR_REF.i.ALLOCA")
					);
					this->handler.createStore(
						pir_i,
						this->handler.createNumber(
							usize_type, core::GenericInt(unsigned(this->context.getTypeManager().numBitsOfPtr()), 0)
						)
					);

					const pir::BasicBlock::ID elem_check_cond_block = this->handler.createBasicBlock(
						this->name(is_equal ? "EQ.ARR_REF.elem_check_cond" : "NEQ.ARR_REF.elem_check_cond")
					);
					const pir::BasicBlock::ID elem_check_then_block = this->handler.createBasicBlock(
						this->name(is_equal ? "EQ.ARR_REF.elem_check_then" : "NEQ.ARR_REF.elem_check_then")
					);

					this->handler.createJump(elem_check_cond_block);


					//////////////////
					// elem check cond

					this->handler.setTargetBasicBlock(elem_check_cond_block);


					const pir::Expr pir_i_load = this->handler.createLoad(
						pir_i, usize_type, this->name(is_equal ? "EQ.ARR_REF.i" : "NEQ.ARR_REF.i")
					);
					const pir::Expr cond = this->handler.createULT(
						pir_i_load, *size_expr, this->name(is_equal ? "EQ.ARR_REF.LOOP_COND" : "NEQ.ARR_REF.LOOP_COND")
					);


					//////////////////
					// elem check then

					this->handler.setTargetBasicBlock(elem_check_then_block);


					const pir::Type elem_type = this->get_type<false, false>(array_ref_type.elementTypeID).type;

					const pir::Expr lhs_elem = this->handler.createCalcPtr(
						lhs_data_ptr,
						elem_type,
						evo::SmallVector<pir::CalcPtr::Index>{pir_i_load},
						this->name(is_equal ? ".EQ.ARR.LHS_PTR" : ".NEQ.ARR.LHS_PTR")
					);

					const pir::Expr rhs_elem = this->handler.createCalcPtr(
						rhs_data_ptr,
						elem_type,
						evo::SmallVector<pir::CalcPtr::Index>{pir_i_load},
						this->name(is_equal ? ".EQ.ARR.RHS_PTR" : ".NEQ.ARR.RHS_PTR")
					);

					const pir::Expr cmp_value = *this->expr_cmp<GetExprMode::REGISTER>(
						array_ref_type.elementTypeID, lhs_elem, rhs_elem, is_equal, nullptr
					);

					const pir::Expr i_increment = this->handler.createAdd(
						pir_i_load,
						this->handler.createNumber(
							usize_type, core::GenericInt(unsigned(this->context.getTypeManager().numBitsOfPtr()), 1)
						),
						false,
						true
					);
					this->handler.createStore(pir_i, i_increment);


					///////////////////////////////////
					// end

					const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(
						this->name(is_equal ? "EQ.ARR_REF.end" : "NEQ.ARR_REF.end")
					);


					this->handler.setTargetBasicBlock(start_basic_block);

					if(is_equal){
						this->handler.createBranch(ref_ptrs_cmp, element_check_block, end_block);
					}else{
						this->handler.createBranch(ref_ptrs_cmp, end_block, element_check_block);
					}


					this->handler.setTargetBasicBlock(elem_check_cond_block);
					this->handler.createBranch(cond, elem_check_then_block, end_block);

					this->handler.setTargetBasicBlock(elem_check_then_block);
					if(is_equal){
						this->handler.createBranch(cmp_value, elem_check_cond_block, end_block);
					}else{
						this->handler.createBranch(cmp_value, end_block, elem_check_cond_block);
					}


					this->handler.setTargetBasicBlock(end_block);

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createPhi(
							evo::SmallVector<pir::Phi::Predecessor>{
								pir::Phi::Predecessor(start_basic_block, this->handler.createBoolean(!is_equal)),
								pir::Phi::Predecessor(elem_check_cond_block, this->handler.createBoolean(is_equal)),
								pir::Phi::Predecessor(elem_check_then_block, this->handler.createBoolean(!is_equal)),
							},
							is_equal ? this->name("EQ.ARR_REF") : this->name("NEQ.ARR_REF")
						);

					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr output_value = this->handler.createPhi(
							evo::SmallVector<pir::Phi::Predecessor>{
								pir::Phi::Predecessor(start_basic_block, this->handler.createBoolean(!is_equal)),
								pir::Phi::Predecessor(elem_check_cond_block, this->handler.createBoolean(is_equal)),
								pir::Phi::Predecessor(elem_check_then_block, this->handler.createBoolean(!is_equal)),
							},
							is_equal ? this->name(".EQ.ARR_REF") : this->name(".NEQ.ARR_REF")
						);

						const pir::Expr output_alloca = this->handler.createAlloca(
							this->module.createBoolType(), is_equal ? this->name("EQ.ARR") : this->name("NEQ.ARR")
						);
						this->handler.createStore(output_alloca, output_value);

						return output_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						const pir::Expr output_value = this->handler.createPhi(
							evo::SmallVector<pir::Phi::Predecessor>{
								pir::Phi::Predecessor(start_basic_block, this->handler.createBoolean(!is_equal)),
								pir::Phi::Predecessor(elem_check_cond_block, this->handler.createBoolean(is_equal)),
								pir::Phi::Predecessor(elem_check_then_block, this->handler.createBoolean(!is_equal)),
							},
							is_equal ? this->name(".EQ.ARR_REF") : this->name(".NEQ.ARR_REF")
						);

						this->handler.createStore(store_locations[0], output_value);

						return std::nullopt;
						
					}else{
						return std::nullopt;
					}
				} break;

				case BaseType::Kind::ALIAS: {
					const BaseType::Alias& alias_type =
						this->context.getTypeManager().getAlias(expr_type.baseTypeID().aliasID());

					return this->expr_cmp<MODE>(
						alias_type.aliasedType, lhs, rhs, is_equal, store_locations
					);
				} break;

				case BaseType::Kind::DISTINCT_ALIAS: {
					const BaseType::DistinctAlias& distinct_alias_type =
						this->context.getTypeManager().getDistinctAlias(expr_type.baseTypeID().distinctAliasID());

					return this->expr_cmp<MODE>(
						distinct_alias_type.underlyingType, lhs, rhs, is_equal, store_locations
					);
				} break;

				case BaseType::Kind::STRUCT: {
					const BaseType::Struct& struct_type =
						this->context.getTypeManager().getStruct(expr_type.baseTypeID().structID());

					const auto [begin_overloads_range, end_overloads_range] = struct_type.infixOverloads.equal_range(
						is_equal ? Token::lookupKind("==") : Token::lookupKind("!=")
					);

					const auto overloads_range = evo::IterRange(begin_overloads_range, end_overloads_range);

					for(const auto& [_, sema_func_id] : overloads_range){
						const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(sema_func_id);
						const BaseType::Function& func_type =
							this->context.getTypeManager().getFunction(sema_func.typeID);

						if(func_type.params[0].typeID == func_type.params[1].typeID){
							const Data::FuncInfo& func_info = this->data.get_func(sema_func_id);

							const pir::Expr target_lhs = [&](){
								if(func_info.params[0].is_copy()){
									return this->handler.createLoad(
										lhs, pir_type, is_equal ? this->name(".EQ.LHS") : this->name(".NEQ.LHS")
									);
								}else{
									return lhs;
								}
							}();

							const pir::Expr target_rhs = [&](){
								if(func_info.params[1].is_copy()){
									return this->handler.createLoad(
										rhs, pir_type, is_equal ? this->name(".EQ.RHS") : this->name(".NEQ.RHS")
									);
								}else{
									return rhs;
								}
							}();

							if constexpr(MODE == GetExprMode::REGISTER){
								return this->handler.createCall(
									func_info.pir_ids[0].as<pir::Function::ID>(),
									evo::SmallVector<pir::Expr>{target_lhs, target_rhs},
									is_equal ? this->name("EQ") : this->name("NEQ")
								);

							}else if constexpr(MODE == GetExprMode::POINTER){
								const pir::Expr call = this->handler.createCall(
									func_info.pir_ids[0].as<pir::Function::ID>(),
									evo::SmallVector<pir::Expr>{target_lhs, target_rhs},
									is_equal ? this->name(".EQ") : this->name(".NEQ")
								);

								const pir::Expr output_param = this->handler.createAlloca(
									this->module.createBoolType(), is_equal ? this->name("EQ") : this->name("NEQ")
								);
								this->handler.createStore(output_param, call);

								return output_param;
								
							}else if constexpr(MODE == GetExprMode::STORE){
								const pir::Expr call = this->handler.createCall(
									func_info.pir_ids[0].as<pir::Function::ID>(),
									evo::SmallVector<pir::Expr>{target_lhs, target_rhs},
									is_equal ? this->name(".EQ") : this->name(".NEQ")
								);

								this->handler.createStore(store_locations[0], call);
								return std::nullopt;
								
							}else{
								const pir::Expr call = this->handler.createCall(
									func_info.pir_ids[0].as<pir::Function::ID>(),
									evo::SmallVector<pir::Expr>{target_lhs, target_rhs},
									is_equal ? this->name(".EQ.discard") : this->name(".NEQ.discard")
								);

								return std::nullopt;
							}
						}
					}
				} break;

				case BaseType::Kind::UNION: {
					const BaseType::Union& union_type =
						this->context.getTypeManager().getUnion(expr_type.baseTypeID().unionID());

					evo::debugAssert(union_type.isUntagged == false, "untagged union is not comparable");


					const pir::Expr lhs_tag_ptr = this->handler.createCalcPtr(
						lhs,
						pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 1},
						this->name(is_equal ? ".EQ.UNION.LHS_TAG_PTR" : ".NEQ.UNION.LHS_TAG_PTR")
					);

					const pir::Expr rhs_tag_ptr = this->handler.createCalcPtr(
						rhs,
						pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 1},
						this->name(is_equal ? ".EQ.UNION.RHS_TAG_PTR" : ".NEQ.UNION.RHS_TAG_PTR")
					);

					const pir::Type tag_type = this->module.getStructType(pir_type).members[1];

					const pir::Expr lhs_tag = this->handler.createLoad(
						lhs_tag_ptr, tag_type, this->name(is_equal ? ".EQ.UNION.LHS_TAG" : ".NEQ.UNION.LHS_TAG")
					);

					const pir::Expr rhs_tag = this->handler.createLoad(
						rhs_tag_ptr, tag_type, this->name(is_equal ? ".EQ.UNION.RHS_TAG" : ".NEQ.UNION.RHS_TAG")
					);

					
					const pir::Expr tag_cmp = [&](){
						if(is_equal){
							return this->handler.createIEq(lhs_tag, rhs_tag, this->name(".EQ.UNION.TAG_CMP"));
						}else{
							return this->handler.createINeq(lhs_tag, rhs_tag, this->name(".NEQ.UNION.TAG_CMP"));
						}
					}();


					const pir::BasicBlock::ID start_block = this->handler.getTargetBasicBlock().getID();

					const pir::BasicBlock::ID value_cmp_block = this->handler.createBasicBlock(
						this->name(is_equal ? "EQ.UNION.value_cmp" : "NEQ.UNION.value_cmp")
					);

					this->handler.setTargetBasicBlock(value_cmp_block);

					const pir::Expr lhs_data_ptr = this->handler.createCalcPtr(
						lhs,
						pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 0},
						this->name(is_equal ? ".EQ.UNION.LHS_DATA_PTR" : ".NEQ.UNION.LHS_DATA_PTR")
					);

					const pir::Expr rhs_data_ptr = this->handler.createCalcPtr(
						rhs,
						pir_type,
						evo::SmallVector<pir::CalcPtr::Index>{0, 0},
						this->name(is_equal ? ".EQ.UNION.RHS_DATA_PTR" : ".NEQ.UNION.RHS_DATA_PTR")
					);



					auto field_cmp_cases = evo::SmallVector<pir::Switch::Case>();
					auto phi_predecesors = evo::SmallVector<pir::Phi::Predecessor>{
						pir::Phi::Predecessor(start_block, this->handler.createBoolean(!is_equal)),
						pir::Phi::Predecessor(value_cmp_block, this->handler.createBoolean(is_equal))
					};

					for(size_t i = 0; const BaseType::Union::Field& field : union_type.fields){
						EVO_DEFER([&](){ i += 1; });

						if(field.typeID.isVoid()){ continue; }
						
						const pir::BasicBlock::ID field_cmp_block = this->handler.createBasicBlock(
							is_equal
							? this->name("EQ.UNION.field_{}", i)
							: this->name("NEQ.UNION.field_{}", i)
						);

						field_cmp_cases.emplace_back(
							this->handler.createNumber(tag_type, core::GenericInt(tag_type.getWidth(), i)),
							field_cmp_block
						);

						this->handler.setTargetBasicBlock(field_cmp_block);

						const pir::Expr value_cmp = *this->expr_cmp<GetExprMode::REGISTER>(
							field.typeID.asTypeID(), lhs_data_ptr, rhs_data_ptr, is_equal, nullptr
						);

						phi_predecesors.emplace_back(field_cmp_block, value_cmp);
					}


					const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(
						this->name(is_equal ? "EQ.UNION.end" : "NEQ.UNION.end")
					);

					for(const pir::Switch::Case& field_cmp_case : field_cmp_cases){
						this->handler.setTargetBasicBlock(field_cmp_case.block);
						this->handler.createJump(end_block);
					}

					this->handler.setTargetBasicBlock(start_block);
					this->handler.createBranch(tag_cmp, value_cmp_block, end_block);

					this->handler.setTargetBasicBlock(value_cmp_block);
					this->handler.createSwitch(lhs_tag, std::move(field_cmp_cases), end_block);

					this->handler.setTargetBasicBlock(end_block);

					if constexpr(MODE == GetExprMode::REGISTER){
						return this->handler.createPhi(
							std::move(phi_predecesors), this->name(is_equal ? "EQ.UNION" : "NEQ.UNION")
						);
						
					}else if constexpr(MODE == GetExprMode::POINTER){
						const pir::Expr output_value = this->handler.createPhi(
							std::move(phi_predecesors), this->name(is_equal ? ".EQ.UNION" : ".NEQ.UNION")
						);

						const pir::Expr output_alloca = this->handler.createAlloca(
							this->module.createBoolType(), this->name(is_equal ? "EQ.UNION" : "NEQ.UNION")
						);

						this->handler.createStore(output_alloca, output_value);

						return output_alloca;
						
					}else if constexpr(MODE == GetExprMode::STORE){
						const pir::Expr output_value = this->handler.createPhi(
							std::move(phi_predecesors), this->name(is_equal ? ".EQ.UNION" : ".NEQ.UNION")
						);

						this->handler.createStore(store_locations[0], output_value);

						return std::nullopt;
						
					}else{
						return std::nullopt;
					}
				} break;

				case BaseType::Kind::ENUM: {
					evo::debugFatalBreak("enum is trivially comparable");
				} break;

				case BaseType::Kind::ARRAY_DEDUCER:           case BaseType::Kind::STRUCT_TEMPLATE:
				case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: case BaseType::Kind::TYPE_DEDUCER:
				case BaseType::Kind::INTERFACE:               case BaseType::Kind::POLY_INTERFACE_REF:
				case BaseType::Kind::INTERFACE_MAP: {
					evo::debugFatalBreak("Invalid type to compare");
				} break;
			}

			evo::debugFatalBreak("Unknown BaseType kind");
		}
	}






	auto SemaToPIR::calc_in_param_bitmap(
		const BaseType::Function& target_func_type, evo::ArrayProxy<sema::Expr> args
	) const -> uint32_t {
		uint32_t output_in_param_bitmap = 0;

		uint32_t num_in_params = 0;

		for(size_t i = 0; const sema::Expr& arg : args){
			EVO_DEFER([&](){ i += 1; });

			if(target_func_type.params[i].kind != BaseType::Function::Param::Kind::IN){ continue; }

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



	auto SemaToPIR::iterate_array(
		const BaseType::Array& array_type,
		bool include_terminator,
		std::string_view op_name,
		std::function<void(pir::Expr)> body_func
	) -> void {
		const uint64_t num_elems = [&](){
			uint64_t output_num_elems = 1;

			for(uint64_t dimension : array_type.dimensions){
				output_num_elems *= dimension;
			}

			if(include_terminator && array_type.terminator.has_value()){ output_num_elems += 1; }

			return output_num_elems;
		}();

		const pir::Type usize_type = this->get_type<false, false>(TypeManager::getTypeUSize()).type;

		const pir::Expr pir_i = this->handler.createAlloca(usize_type, this->name(".{}.i.ALLOCA", op_name));
		this->handler.createStore(
			pir_i,
			this->handler.createNumber(
				usize_type, core::GenericInt(unsigned(this->context.getTypeManager().numBitsOfPtr()), 0)
			)
		);

		const pir::BasicBlock::ID cond_block = this->handler.createBasicBlock(this->name("{}.cond", op_name));
		const pir::BasicBlock::ID then_block = this->handler.createBasicBlock(this->name("{}.then", op_name));
		const pir::BasicBlock::ID end_block = this->handler.createBasicBlock(this->name("{}.end", op_name));

		this->handler.createJump(cond_block);


		//////////////////
		// cond

		this->handler.setTargetBasicBlock(cond_block);

		const pir::Expr array_size = this->handler.createNumber(
			usize_type, core::GenericInt(unsigned(this->context.getTypeManager().numBitsOfPtr()), num_elems)
		);

		const pir::Expr pir_i_load = this->handler.createLoad(pir_i, usize_type, this->name("{}.i", op_name));
		const pir::Expr cond = 
			this->handler.createULT(pir_i_load, array_size, this->name("{}.LOOP_COND", op_name));

		this->handler.createBranch(cond, then_block, end_block);


		//////////////////
		// then

		this->handler.setTargetBasicBlock(then_block);


		body_func(pir_i_load);

		const pir::Expr i_increment = this->handler.createAdd(
			pir_i_load,
			this->handler.createNumber(
				usize_type, core::GenericInt(unsigned(this->context.getTypeManager().numBitsOfPtr()), 1)
			),
			false,
			true
		);
		this->handler.createStore(pir_i, i_increment);

		this->handler.createJump(cond_block);


		//////////////////
		// end

		this->handler.setTargetBasicBlock(end_block);
	}




	auto SemaToPIR::create_call(
		evo::Variant<std::monostate, pir::Function::ID, pir::ExternalFunction::ID> func_id,
		evo::SmallVector<pir::Expr>&& args,
		std::string&& name
	) -> pir::Expr {
		if(func_id.is<pir::Function::ID>()){
			return this->handler.createCall(func_id.as<pir::Function::ID>(), std::move(args), std::move(name));

		}else{
			evo::debugAssert(func_id.is<pir::ExternalFunction::ID>(), "This func id was deleted by in-param type");
			return this->handler.createCall(
				func_id.as<pir::ExternalFunction::ID>(), std::move(args), std::move(name)
			);
		}
	}

	auto SemaToPIR::create_call_void(
		evo::Variant<std::monostate, pir::Function::ID, pir::ExternalFunction::ID> func_id,
		evo::SmallVector<pir::Expr>&& args
	) -> void {
		if(func_id.is<pir::Function::ID>()){
			this->handler.createCallVoid(func_id.as<pir::Function::ID>(), std::move(args));
		}else{
			evo::debugAssert(func_id.is<pir::ExternalFunction::ID>(), "This func id was deleted by in-param type");
			this->handler.createCallVoid(func_id.as<pir::ExternalFunction::ID>(), std::move(args));
		}
	}

	auto SemaToPIR::create_call_no_return(
		evo::Variant<std::monostate, pir::Function::ID, pir::ExternalFunction::ID> func_id,
		evo::SmallVector<pir::Expr>&& args
	) -> void {
		if(func_id.is<pir::Function::ID>()){
			this->handler.createCallNoReturn(func_id.as<pir::Function::ID>(), std::move(args));
		}else{
			evo::debugAssert(func_id.is<pir::ExternalFunction::ID>(), "This func id was deleted by in-param type");
			this->handler.createCallNoReturn(func_id.as<pir::ExternalFunction::ID>(), std::move(args));
		}
	}




	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::intrinsic_func_call_expr(
		const sema::FuncCall& func_call, evo::ArrayProxy<pir::Expr> store_locations
	) -> std::optional<pir::Expr> {
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
			return this->handler.createNumber(
				this->module.createUnsignedType(sizeof(size_t) * 8),
				core::GenericInt::create<size_t>(size_t(&this->context))
			);
		};

		const pir::Expr value = [&](){
			switch(intrinsic_func_kind){
				case IntrinsicFunc::Kind::BUILD_CREATE_PACKAGE: {
					auto args = evo::SmallVector<pir::Expr>();
					args.emplace_back(get_context_ptr());
					get_args(args);

					return this->handler.createCall(
						this->data.getJITBuildFuncs().build_create_package, std::move(args)
					);
				} break;

				default: evo::debugFatalBreak("Unknown intrinsic expr");
			}
		}();


		if constexpr(MODE == GetExprMode::REGISTER){
			return value;

		}else if constexpr(MODE == GetExprMode::POINTER){
			const pir::Expr call_alloca = this->handler.createAlloca(this->handler.getExprType(value));
			this->handler.createStore(call_alloca, value);
			return std::nullopt;
			
		}else if constexpr(MODE == GetExprMode::STORE){
			this->handler.createStore(store_locations[0], value);
			return std::nullopt;

		}else{
			return std::nullopt;
		}
	}


	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::template_intrinsic_func_call_expr(
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

			case TemplateIntrinsicFunc::Kind::ARRAY_ELEMENT_TYPE_ID: {
				evo::debugFatalBreak("@arrayElementTypeID is constexpr evaluated");
			} break;

			case TemplateIntrinsicFunc::Kind::ARRAY_REF_ELEMENT_TYPE_ID: {
				evo::debugFatalBreak("@arrayRefElementTypeID is constexpr evaluated");
			} break;

			case TemplateIntrinsicFunc::Kind::NUM_BYTES: {
				evo::debugFatalBreak("@numBytes is constexpr evaluated");
			} break;

			case TemplateIntrinsicFunc::Kind::NUM_BITS: {
				evo::debugFatalBreak("@numBits is constexpr evaluated");
			} break;

			case TemplateIntrinsicFunc::Kind::BIT_CAST: {
				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::Type to_type =
						this->get_type<false, false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>()).type;

					if(
						to_type.isPrimitive()
						&& this->context.getTypeManager().isPrimitive(
								instantiation.templateArgs[0].as<TypeInfo::VoidableID>()
							)
					){
						const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
						return this->handler.createBitCast(from_value, to_type, this->name("BIT_CAST"));
						
					}else{
						const pir::Expr from_value = this->get_expr_pointer(func_call.args[0]);
						return this->handler.createLoad(from_value, to_type, this->name("BIT_CAST"));
					}

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->get_expr_pointer(func_call.args[0]);

				}else if constexpr(MODE == GetExprMode::STORE){
					this->get_expr_store(func_call.args[0], store_locations);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::TRUNC: {
				const pir::Type to_type =
					this->get_type<false, false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>()).type;

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->handler.createTrunc(from_value, to_type, this->name("TRUNC"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(to_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FTRUNC: {
				const pir::Type to_type =
					this->get_type<false, false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>()).type;

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->handler.createFTrunc(from_value, to_type, this->name("FTRUNC"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(to_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SEXT: {
				const pir::Type to_type =
					this->get_type<false, false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>()).type;

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->handler.createSExt(from_value, to_type, this->name("SEXT"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(to_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::ZEXT: {
				const pir::Type to_type =
					this->get_type<false, false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>()).type;

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->handler.createZExt(from_value, to_type, this->name("ZEXT"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(to_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FEXT: {
				const pir::Type to_type =
					this->get_type<false, false>(instantiation.templateArgs[1].as<TypeInfo::VoidableID>()).type;

				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);
				const pir::Expr register_value = this->handler.createFExt(from_value, to_type, this->name("FEXT"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(to_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::I_TO_F: {
				const pir::Type to_type = this->get_type<false, false>(
					instantiation.templateArgs[1].as<TypeInfo::VoidableID>().asTypeID()
				).type;
				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = [&](){
					const TypeInfo::ID from_type_id = 
						instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();

					if(this->context.type_manager.isUnsignedIntegral(from_type_id)){
						return this->handler.createUIToF(from_value, to_type, this->name("UI_TO_F"));
					}else{
						return this->handler.createIToF(from_value, to_type, this->name("I_TO_F"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(to_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::F_TO_I: {
				const TypeInfo::VoidableID to_type_id =
					instantiation.templateArgs[1].as<TypeInfo::VoidableID>().asTypeID();
				const pir::Type to_type = this->get_type<false, false>(to_type_id).type;
				const pir::Expr from_value = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = [&](){
					if(this->context.getTypeManager().isUnsignedIntegral(to_type_id)){
						return this->handler.createFToUI(from_value, to_type, this->name("F_TO_UI"));
					}else{
						return this->handler.createFToI(from_value, to_type, this->name("F_TO_I"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(to_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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

				const pir::Expr register_value = this->handler.createAdd(
					lhs, rhs, !is_unsigned & !may_wrap, is_unsigned & !may_wrap, this->name("ADD")
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false, false>(arg_type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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
						return this->handler.createUAddWrap(
							lhs, rhs, this->name("UADD_WRAP.VALUE"), this->name("UADD_WRAP.WRAPPED")
						);
					}else{
						return this->handler.createSAddWrap(
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
						this->handler.createStore(store_locations[0], this->handler.extractUAddWrapResult(result));
						this->handler.createStore(store_locations[1], this->handler.extractUAddWrapWrapped(result));
					}else{
						this->handler.createStore(store_locations[0], this->handler.extractSAddWrapResult(result));
						this->handler.createStore(store_locations[1], this->handler.extractSAddWrapWrapped(result));
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
						return this->handler.createUAddSat(lhs, rhs, this->name("UADD_SAT"));
					}else{
						return this->handler.createSAddSat(lhs, rhs, this->name("SADD_SAT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false, false>(arg_type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FADD: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->handler.createFAdd(lhs, rhs, this->name("FADD"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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

				const pir::Expr register_value = this->handler.createSub(
					lhs, rhs, !is_unsigned & !may_wrap, is_unsigned & !may_wrap, this->name("SUB")
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false, false>(arg_type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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
						return this->handler.createUSubWrap(
							lhs, rhs, this->name("USUB_WRAP.VALUE"), this->name("USUB_WRAP.WRAPPED")
						);
					}else{
						return this->handler.createSSubWrap(
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
						this->handler.createStore(store_locations[0], this->handler.extractUSubWrapResult(result));
						this->handler.createStore(store_locations[1], this->handler.extractUSubWrapWrapped(result));
					}else{
						this->handler.createStore(store_locations[0], this->handler.extractSSubWrapResult(result));
						this->handler.createStore(store_locations[1], this->handler.extractSSubWrapWrapped(result));
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
						return this->handler.createUSubSat(lhs, rhs, this->name("USUB_SAT"));
					}else{
						return this->handler.createSSubSat(lhs, rhs, this->name("SSUB_SAT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false, false>(arg_type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FSUB: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->handler.createFSub(lhs, rhs, this->name("FSUB"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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

				const pir::Expr register_value = this->handler.createMul(
					lhs, rhs, !is_unsigned & !may_wrap, is_unsigned & !may_wrap, this->name("MUL")
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false, false>(arg_type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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
						return this->handler.createUMulWrap(
							lhs, rhs, this->name("UMUL_WRAP.VALUE"), this->name("UMUL_WRAP.WRAPPED")
						);
					}else{
						return this->handler.createSMulWrap(
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
						this->handler.createStore(store_locations[0], this->handler.extractUMulWrapResult(result));
						this->handler.createStore(store_locations[1], this->handler.extractUMulWrapWrapped(result));
					}else{
						this->handler.createStore(store_locations[0], this->handler.extractSMulWrapResult(result));
						this->handler.createStore(store_locations[1], this->handler.extractSMulWrapWrapped(result));
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
						return this->handler.createUMulSat(lhs, rhs, this->name("UMUL_SAT"));
					}else{
						return this->handler.createSMulSat(lhs, rhs, this->name("SMUL_SAT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false, false>(arg_type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FMUL: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->handler.createFMul(lhs, rhs, this->name("FMUL"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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
						return this->handler.createUDiv(lhs, rhs, is_exact, this->name("UDIV"));
					}else{
						return this->handler.createSDiv(lhs, rhs, is_exact, this->name("SDIV"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Type arg_pir_type = this->get_type<false, false>(arg_type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FDIV: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->handler.createFDiv(lhs, rhs, this->name("FDIV"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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
						return this->handler.createFRem(lhs, rhs, this->name("FREM"));
					}else if(is_unsigned){
						return this->handler.createURem(lhs, rhs, this->name("UREM"));
					}else{
						return this->handler.createSRem(lhs, rhs, this->name("SREM"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::FNEG: {
				const pir::Expr rhs = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->handler.createFNeg(rhs, this->name("FNEG"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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
						return this->handler.createFEq(lhs, rhs, this->name("FEQ"));
					}else{
						return this->handler.createIEq(lhs, rhs, this->name("IEQ"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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
						return this->handler.createFNeq(lhs, rhs, this->name("FNEQ"));
					}else{
						return this->handler.createINeq(lhs, rhs, this->name("INEQ"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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
						return this->handler.createFLT(lhs, rhs, this->name("FLT"));
					}else if(is_unsigned){
						return this->handler.createULT(lhs, rhs, this->name("ULT"));
					}else{
						return this->handler.createSLT(lhs, rhs, this->name("SLT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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
						return this->handler.createFLTE(lhs, rhs, this->name("FLTE"));
					}else if(is_unsigned){
						return this->handler.createULTE(lhs, rhs, this->name("ULTE"));
					}else{
						return this->handler.createSLTE(lhs, rhs, this->name("SLTE"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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
						return this->handler.createFGT(lhs, rhs, this->name("FGT"));
					}else if(is_unsigned){
						return this->handler.createUGT(lhs, rhs, this->name("UGT"));
					}else{
						return this->handler.createSGT(lhs, rhs, this->name("SGT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
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
						return this->handler.createFGTE(lhs, rhs, this->name("FGTE"));
					}else if(is_unsigned){
						return this->handler.createUGTE(lhs, rhs, this->name("UGTE"));
					}else{
						return this->handler.createSGTE(lhs, rhs, this->name("SGTE"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;


			case TemplateIntrinsicFunc::Kind::AND: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->handler.createAnd(lhs, rhs, this->name("AND"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::OR: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->handler.createOr(lhs, rhs, this->name("OR"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::XOR: {
				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->get_expr_register(func_call.args[1]);

				const pir::Expr register_value = this->handler.createXor(lhs, rhs, this->name("XOR"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SHL: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const pir::Type arg_pir_type = this->get_type<false, false>(arg_type_id).type;
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const bool is_exact = !instantiation.templateArgs[2].as<core::GenericValue>().getBool();

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				const pir::Expr rhs = this->handler.createZExt(
					this->get_expr_register(func_call.args[1]), arg_pir_type, this->name("SHL.AMMOUNT_ZEXT")
				);

				const pir::Expr register_value = this->handler.createSHL(
					lhs, rhs, !is_unsigned & is_exact, is_unsigned & is_exact, this->name("SHL")
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SHL_SAT: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const pir::Type arg_pir_type = this->get_type<false, false>(arg_type_id).type;
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);
				

				const pir::Expr register_value = [&](){
					if(is_unsigned){
						const pir::Expr rhs = this->handler.createZExt(
							this->get_expr_register(func_call.args[1]),
							arg_pir_type,
							this->name("USHL_SAT.AMMOUNT_ZEXT")
						);
						return this->handler.createUSHLSat(lhs, rhs, this->name("USHL_SAT"));
					}else{
						const pir::Expr rhs = this->handler.createZExt(
							this->get_expr_register(func_call.args[1]),
							arg_pir_type,
							this->name("SSHL_SAT.AMMOUNT_ZEXT")
						);
						return this->handler.createSSHLSat(lhs, rhs, this->name("SSHL_SAT"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::SHR: {
				const TypeInfo::ID arg_type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
				const pir::Type arg_pir_type = this->get_type<false, false>(arg_type_id).type;
				const bool is_unsigned = this->context.type_manager.isUnsignedIntegral(arg_type_id);
				const bool is_exact = !instantiation.templateArgs[2].as<core::GenericValue>().getBool();

				const pir::Expr lhs = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = [&](){
					if(is_unsigned){
						const pir::Expr rhs = this->handler.createZExt(
							this->get_expr_register(func_call.args[1]), arg_pir_type, this->name("USHR.AMMOUNT_ZEXT")
						);
						return this->handler.createUSHR(lhs, rhs, is_exact, this->name("USHR"));
					}else{
						const pir::Expr rhs = this->handler.createZExt(
							this->get_expr_register(func_call.args[1]), arg_pir_type, this->name("SSHR.AMMOUNT_ZEXT")
						);
						return this->handler.createSSHR(lhs, rhs, is_exact, this->name("SSHR"));
					}
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::BIT_REVERSE: {
				const pir::Expr value = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->handler.createBitReverse(value, this->name("BIT_REVERSE"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::BYTE_SWAP: {
				const pir::Expr value = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->handler.createByteSwap(value, this->name("BYTE_SWAP"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::CTPOP: {
				const pir::Expr value = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->handler.createCtPop(value, this->name("CTPOP"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::CTLZ: {
				const pir::Expr value = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->handler.createCTLZ(value, this->name("CTLZ"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::CTTZ: {
				const pir::Expr value = this->get_expr_register(func_call.args[0]);

				const pir::Expr register_value = this->handler.createCTTZ(value, this->name("CTTZ"));

				if constexpr(MODE == GetExprMode::REGISTER){
					return register_value;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const TypeInfo::ID type_id = instantiation.templateArgs[0].as<TypeInfo::VoidableID>().asTypeID();
					const pir::Type arg_pir_type = this->get_type<false, false>(type_id).type;
					const pir::Expr pointer_alloca = this->handler.createAlloca(arg_pir_type);
					this->handler.createStore(pointer_alloca, register_value);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], register_value);
					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::ATOMIC_LOAD: {
				const pir::Expr value = this->get_expr_register(func_call.args[0]);

				const pir::AtomicOrdering atomic_ordering =	
					SemaToPIR::get_atomic_ordering(instantiation.templateArgs[2].as<core::GenericValue>());

				const TypeInfo::ID type_id = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().asTypeID();
				const pir::Type pir_type = this->get_type<false, false>(type_id).type;

				std::string expr_name = [&]() -> std::string {
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->name("ATOMIC_LOAD");

					}else if constexpr(MODE == GetExprMode::POINTER || MODE == GetExprMode::STORE){
						return this->name(".ATOMIC_LOAD");

					}else{
						return this->name(".DISCARD_ATOMIC_LOAD");
					}
				}();

				const pir::Expr atomic_load = [&]() -> pir::Expr {
					if(pir_type.isIntegral()){
						if(pir_type.getWidth() < 8 || std::has_single_bit(pir_type.getWidth()) == false){
							const pir::Type load_type = [&]() -> pir::Type {
								if(pir_type.kind() == pir::Type::Kind::UNSIGNED){
									return this->module.createUnsignedType(std::bit_ceil(pir_type.getWidth()));
								}else{
									return this->module.createSignedType(std::bit_ceil(pir_type.getWidth()));
								}
							}();

							const pir::Expr intermediate = this->handler.createLoad(
								value, load_type, this->name(".ATOMIC_LOAD_INTERMEDIATE"), false, atomic_ordering
							);

							return this->handler.createTrunc(intermediate, pir_type, std::move(expr_name));
						}
					}

					return this->handler.createLoad(value, pir_type, std::move(expr_name), false, atomic_ordering);
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return atomic_load;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(pir_type);
					this->handler.createStore(pointer_alloca, atomic_load);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], atomic_load);
					return std::nullopt;

				}else{
					// NOTE: yes, it's correct that the load is made
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::CMPXCHG: {
				const bool is_weak = instantiation.templateArgs[2].as<core::GenericValue>().getBool();

				const pir::AtomicOrdering success_atomic_ordering =	
					SemaToPIR::get_atomic_ordering(instantiation.templateArgs[3].as<core::GenericValue>());

				const pir::AtomicOrdering failure_atomic_ordering =	
					SemaToPIR::get_atomic_ordering(instantiation.templateArgs[4].as<core::GenericValue>());

				const pir::Expr target = this->get_expr_register(func_call.args[0]);
				pir::Expr expected = this->get_expr_register(func_call.args[1]);
				pir::Expr desired = this->get_expr_register(func_call.args[2]);

				const pir::Type value_pir_type = this->handler.getExprType(expected);

				bool requires_type_conversion = false;
				if(value_pir_type.isIntegral()){
					if(value_pir_type.getWidth() < 8 || std::has_single_bit(value_pir_type.getWidth()) == false){
						requires_type_conversion = true;

						const uint32_t target_width = std::bit_ceil(value_pir_type.getWidth());
						
						if(value_pir_type.kind() == pir::Type::Kind::UNSIGNED){
							expected =
								this->handler.createZExt(expected, this->module.createUnsignedType(target_width));
							desired = this->handler.createZExt(desired, this->module.createUnsignedType(target_width));
						}else{
							expected = this->handler.createSExt(expected, this->module.createSignedType(target_width));
							desired = this->handler.createSExt(desired, this->module.createSignedType(target_width));
						}
					}
				}

				const pir::Expr result = this->handler.createCmpXchg(
					target,
					expected,
					desired,
					this->name("CMPXCHG.LOADED"),
					this->name("CMPXCHG.SUCCEEDED"),
					is_weak,
					success_atomic_ordering,
					failure_atomic_ordering
				);


				if constexpr(MODE == GetExprMode::REGISTER){
					evo::debugFatalBreak("@addWrap returns multiple values");

				}else if constexpr(MODE == GetExprMode::POINTER){
					evo::debugFatalBreak("@addWrap returns multiple values");

				}else if constexpr(MODE == GetExprMode::STORE){
					if(requires_type_conversion){
						this->handler.createStore(
							store_locations[0],
							this->handler.createTrunc(this->handler.extractCmpXchgLoaded(result), value_pir_type)
						);
					}else{
						this->handler.createStore(store_locations[0], this->handler.extractCmpXchgLoaded(result));
					}

					this->handler.createStore(store_locations[1], this->handler.extractCmpXchgSucceeded(result));

					return std::nullopt;
				}else{
					return std::nullopt;
				}
			} break;

			case TemplateIntrinsicFunc::Kind::ATOMIC_RMW: {
				const TypeInfo::ID type_id = instantiation.templateArgs[1].as<TypeInfo::VoidableID>().asTypeID();

				const pir::Type pir_type = this->get_type<false, false>(type_id).type;

				const pir::Expr target = this->get_expr_register(func_call.args[0]);

				pir::Expr value = this->get_expr_register(func_call.args[1]);
				bool requires_type_conversion = false;
				if(this->context.getTypeManager().isIntegral(type_id)){
					const size_t bit_width = this->context.getTypeManager().numBits(type_id, false);
					if(bit_width < 8 || std::has_single_bit(bit_width) == false){
						requires_type_conversion = true;

						if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
							value = this->handler.createZExt(
								value,
								this->module.createUnsignedType(uint32_t(std::bit_ceil(bit_width))),
								this->name(".ATOMIC_RMW_EXT")
							);
						}else{
							value = this->handler.createSExt(
								value,
								this->module.createSignedType(uint32_t(std::bit_ceil(bit_width))),
								this->name(".ATOMIC_RMW_EXT")
							);
						}
					}
				}


				const pir::AtomicOrdering atomic_ordering =	
					SemaToPIR::get_atomic_ordering(instantiation.templateArgs[3].as<core::GenericValue>());

				const pir::AtomicRMW::Op op = [&]() -> pir::AtomicRMW::Op {
					switch(
						static_cast<TemplateIntrinsicFunc::AtomicRMWOp>(
							static_cast<uint32_t>(instantiation.templateArgs[2].as<core::GenericValue>().getInt(32))
						)
					){
						case TemplateIntrinsicFunc::AtomicRMWOp::XCHG: {
							return pir::AtomicRMW::Op::XCHG;
						} break;

						case TemplateIntrinsicFunc::AtomicRMWOp::ADD: {
							if(this->context.getTypeManager().isFloatingPoint(type_id)){
								return pir::AtomicRMW::Op::FADD;
							}else{
								return pir::AtomicRMW::Op::ADD;
							}
						} break;

						case TemplateIntrinsicFunc::AtomicRMWOp::SUB: {
							if(this->context.getTypeManager().isFloatingPoint(type_id)){
								return pir::AtomicRMW::Op::FSUB;
							}else{
								return pir::AtomicRMW::Op::SUB;
							}
						} break;

						case TemplateIntrinsicFunc::AtomicRMWOp::AND: {
							return pir::AtomicRMW::Op::AND;
						} break;

						case TemplateIntrinsicFunc::AtomicRMWOp::NAND: {
							return pir::AtomicRMW::Op::NAND;
						} break;

						case TemplateIntrinsicFunc::AtomicRMWOp::OR: {
							return pir::AtomicRMW::Op::OR;
						} break;

						case TemplateIntrinsicFunc::AtomicRMWOp::XOR: {
							return pir::AtomicRMW::Op::XOR;
						} break;

						case TemplateIntrinsicFunc::AtomicRMWOp::MIN: {
							if(this->context.getTypeManager().isFloatingPoint(type_id)){
								return pir::AtomicRMW::Op::FMIN;

							}else if(this->context.getTypeManager().isSignedIntegral(type_id)){
								return pir::AtomicRMW::Op::SMIN;

							}else{
								return pir::AtomicRMW::Op::UMIN;
							}
						} break;

						case TemplateIntrinsicFunc::AtomicRMWOp::MAX: {
							if(this->context.getTypeManager().isFloatingPoint(type_id)){
								return pir::AtomicRMW::Op::FMAX;

							}else if(this->context.getTypeManager().isSignedIntegral(type_id)){
								return pir::AtomicRMW::Op::SMAX;

							}else{
								return pir::AtomicRMW::Op::UMAX;
							}
						} break;
					}

					evo::debugFatalBreak("Unknown @pthr.AtomicRMWOp");
				}();

				std::string expr_name = [&]() -> std::string {
					if constexpr(MODE == GetExprMode::REGISTER){
						return this->name("ATOMIC_RMW");

					}else if constexpr(MODE == GetExprMode::POINTER || MODE == GetExprMode::STORE){
						return this->name(".ATOMIC_RMW");

					}else{
						return this->name(".DISCARD_ATOMIC_RMW");
					}
				}();

				const pir::Expr atomic_rmw_expr = [&]() -> pir::Expr {
					if(requires_type_conversion){
						const pir::Expr intermediate = this->handler.createAtomicRMW(
							op, target, value, this->name(".ATOMIC_RMW_CALC"), atomic_ordering
						);

						const uint32_t num_bits = uint32_t(this->context.getTypeManager().numBits(type_id, false));
						return this->handler.createTrunc(intermediate, pir_type, std::move(expr_name));
					}

					return this->handler.createAtomicRMW(op, target, value, std::move(expr_name), atomic_ordering);
				}();


				if constexpr(MODE == GetExprMode::REGISTER){
					return atomic_rmw_expr;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr pointer_alloca = this->handler.createAlloca(pir_type);
					this->handler.createStore(pointer_alloca, atomic_rmw_expr);
					return pointer_alloca;

				}else if constexpr(MODE == GetExprMode::STORE){
					this->handler.createStore(store_locations[0], atomic_rmw_expr);
					return std::nullopt;

				}else{
					return std::nullopt;
				}
			} break;
		}

		evo::unreachable();
	}



	auto SemaToPIR::intrinsic_func_call(const sema::FuncCall& func_call) -> void {
		const auto ssl = this->create_scoped_source_location(func_call.line, func_call.collumn);

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
			return this->handler.createNumber(
				this->module.createUnsignedType(sizeof(size_t) * 8),
				core::GenericInt::create<size_t>(size_t(&this->context))
			);
		};



		switch(intrinsic_func_kind){
			case IntrinsicFunc::Kind::ABORT: {
				this->handler.createAbort();
			} break;

			case IntrinsicFunc::Kind::BREAKPOINT: {
				this->handler.createBreakpoint();
			} break;

			case IntrinsicFunc::Kind::PANIC: {
				this->create_panic(this->get_expr_pointer(func_call.args[0]));
			} break;

			case IntrinsicFunc::Kind::BUILD_SET_NUM_THREADS: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->handler.createCallVoid(this->data.getJITBuildFuncs().build_set_num_threads, std::move(args));
			} break;

			case IntrinsicFunc::Kind::BUILD_SET_OUTPUT: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->handler.createCallVoid(this->data.getJITBuildFuncs().build_set_output, std::move(args));
			} break;

			case IntrinsicFunc::Kind::BUILD_SET_ADD_DEBUG_INFO: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->handler.createCallVoid(this->data.getJITBuildFuncs().build_set_add_debug_info, std::move(args));
			} break;

			case IntrinsicFunc::Kind::BUILD_SET_STD_LIB_PACKAGE: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->handler.createCallVoid(this->data.getJITBuildFuncs().build_set_std_lib_package, std::move(args));
			} break;

			case IntrinsicFunc::Kind::BUILD_CREATE_PACKAGE: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->handler.createCallVoid(this->data.getJITBuildFuncs().build_create_package, std::move(args));
			} break;

			case IntrinsicFunc::Kind::BUILD_ADD_SOURCE_FILE: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->handler.createCallVoid(this->data.getJITBuildFuncs().build_add_source_file, std::move(args));
			} break;

			case IntrinsicFunc::Kind::BUILD_ADD_SOURCE_DIRECTORY: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->handler.createCallVoid(this->data.getJITBuildFuncs().build_add_source_directory, std::move(args));
			} break;

			case IntrinsicFunc::Kind::BUILD_ADD_C_HEADER_FILE: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->handler.createCallVoid(this->data.getJITBuildFuncs().build_add_c_header_file, std::move(args));
			} break;

			case IntrinsicFunc::Kind::BUILD_ADD_CPP_HEADER_FILE: {
				auto args = evo::SmallVector<pir::Expr>();
				args.emplace_back(get_context_ptr());
				get_args(args);

				this->handler.createCallVoid(this->data.getJITBuildFuncs().build_add_cpp_header_file, std::move(args));
			} break;

			default: evo::debugFatalBreak("Unknown intrinsic");
		}
	}



	auto SemaToPIR::template_intrinsic_func_call(const sema::FuncCall& func_call) -> void {
		const sema::TemplateIntrinsicFuncInstantiation& instantiation = 
			this->context.getSemaBuffer().getTemplateIntrinsicFuncInstantiation(
				func_call.target.as<sema::TemplateIntrinsicFuncInstantiation::ID>()
			);

		switch(instantiation.kind){
			case TemplateIntrinsicFunc::Kind::ATOMIC_STORE: {
				const pir::Expr dst = this->get_expr_register(func_call.args[0]);
				pir::Expr value = this->get_expr_register(func_call.args[1]);

				const pir::AtomicOrdering atomic_ordering =	
					SemaToPIR::get_atomic_ordering(instantiation.templateArgs[2].as<core::GenericValue>());

				const pir::Type value_type = this->handler.getExprType(value);

				if(value_type.isIntegral()){
					if(value_type.getWidth() < 8 || std::has_single_bit(value_type.getWidth()) == false){
						if(value_type.kind() == pir::Type::Kind::UNSIGNED){
							value = this->handler.createZExt(
								value, this->module.createUnsignedType(std::bit_ceil(value_type.getWidth()))
							);
						}else{
							value = this->handler.createSExt(
								value, this->module.createSignedType(std::bit_ceil(value_type.getWidth()))
							);
						}
					}
				}

				this->handler.createStore(dst, value, false, atomic_ordering);
			} break;

			default: evo::debugFatalBreak("Unknown intrinsic");
		}
	}




	auto SemaToPIR::create_panic(pir::Expr message) -> void {
		const Data::FuncInfo& func_info = this->data.get_func(*this->context.panic);

		this->handler.createCallNoReturn(
			func_info.pir_ids[0].as<pir::Function::ID>(), evo::SmallVector<pir::Expr>{message}
		);
	}


	auto SemaToPIR::create_panic(std::string_view message)
	-> void {
		const pir::GlobalVar::String::ID string_value_id = this->module.createGlobalString(std::string(message) + '\0');

		const pir::GlobalVar::ID string_id = this->module.createGlobalVar(
			std::format("PTHR.str{}", this->data.get_string_literal_id()),
			this->module.getGlobalString(string_value_id).type,
			pir::Linkage::PRIVATE,
			string_value_id,
			true
		);

		const pir::Type array_ref_type = this->data.getArrayRefType(
			this->module,
			this->context,
			TypeManager::getTypeStringRef(),
			[&](TypeInfo::ID id){ return *this->get_type<false, true>(id).meta_type_id; }
		).pir_type;

		const pir::Expr string_ref_alloca = this->handler.createAlloca(array_ref_type, this->name(".PANIC_STRING"));

		const pir::Expr data_ptr = this->handler.createCalcPtr(
			string_ref_alloca,
			array_ref_type,
			evo::SmallVector<pir::CalcPtr::Index>{0},
			this->name(".PANIC_STRING.data_ptr")
		);
		this->handler.createStore(data_ptr, this->handler.createGlobalValue(string_id));

		const pir::Expr size_ptr = this->handler.createCalcPtr(
			string_ref_alloca,
			array_ref_type,
			evo::SmallVector<pir::CalcPtr::Index>{1},
			this->name(".PANIC_STRING.size_ptr")
		);
		const uint64_t num_bits_of_ptr = this->context.getTypeManager().numBitsOfPtr();
		this->handler.createStore(
			size_ptr,
			this->handler.createNumber(
				this->module.createUnsignedType(uint32_t(num_bits_of_ptr)),
				core::GenericInt(unsigned(num_bits_of_ptr), message.size())
			)
		);

		this->create_panic(string_ref_alloca);
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
				evo::debugFatalBreak("Can't lower `null`");
			} break;

			case sema::Expr::Kind::INT_VALUE: {
				const sema::IntValue& int_value = this->context.getSemaBuffer().getIntValue(expr.intValueID());
				return this->handler.createNumber(
					this->get_type<false, false>(*int_value.typeID).type, int_value.value
				);
			} break;

			case sema::Expr::Kind::FLOAT_VALUE: {
				const sema::FloatValue& float_value = this->context.getSemaBuffer().getFloatValue(expr.floatValueID());
				return this->handler.createNumber(
					this->get_type<false, false>(*float_value.typeID).type, float_value.value
				);
			} break;

			case sema::Expr::Kind::BOOL_VALUE: {
				const sema::BoolValue& bool_value = this->context.getSemaBuffer().getBoolValue(expr.boolValueID());
				return this->handler.createBoolean(bool_value.value);
			} break;

			case sema::Expr::Kind::STRING_VALUE: {
				const sema::StringValue& string_value =
					this->context.getSemaBuffer().getStringValue(expr.stringValueID());

				const pir::GlobalVar::String::ID string_value_id = 
					this->module.createGlobalString(string_value.value + '\0');

				const pir::GlobalVar::ID string_id = this->module.createGlobalVar(
					std::format("PTHR.str{}", this->data.get_string_literal_id()),
					this->module.getGlobalString(string_value_id).type,
					pir::Linkage::PRIVATE,
					string_value_id,
					true
				);

				this->data.create_global_string(expr.stringValueID(), string_id);

				return this->handler.createGlobalValue(string_id);
			} break;

			case sema::Expr::Kind::AGGREGATE_VALUE: {
				const sema::AggregateValue& aggregate_value = 
					this->context.getSemaBuffer().getAggregateValue(expr.aggregateValueID());

				if(aggregate_value.typeID.kind() == BaseType::Kind::ARRAY){
					const BaseType::Array& aggregate_array_type =
						this->context.getTypeManager().getArray(aggregate_value.typeID.arrayID());

					if(aggregate_array_type.elementTypeID == TypeManager::getTypeChar()){
						auto string_value = std::string();
						string_value.reserve(aggregate_value.values.size());
						for(const sema::Expr value : aggregate_value.values){
							string_value += this->context.getSemaBuffer().getCharValue(value.charValueID()).value;
						}

						return this->module.createGlobalString(std::move(string_value));
					}
				}



				auto values = evo::SmallVector<pir::GlobalVar::Value>();
				values.reserve(aggregate_value.values.size());
				for(const sema::Expr value : aggregate_value.values){
					values.emplace_back(this->get_global_var_value(value));
				}

				const pir::Type aggregate_type = this->get_type<false, false>(aggregate_value.typeID).type;

				if(aggregate_type.kind() == pir::Type::Kind::STRUCT){
					// for empty structs
					if(values.empty()){
						values.emplace_back(
							this->handler.createNumber(this->module.createUnsignedType(1), core::GenericInt(1, 0))
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
				return this->handler.createNumber(
					this->module.createSignedType(8), core::GenericInt(8, uint64_t(char_value.value))
				);
			} break;

			case sema::Expr::Kind::ADDR_OF: {
				const sema::Expr& addr_of_target = this->context.getSemaBuffer().getAddrOf(expr.addrOfID());

				switch(addr_of_target.kind()){
					case sema::Expr::Kind::STRING_VALUE: {
						return this->handler.createGlobalValue(
							this->data.get_global_string(addr_of_target.stringValueID())
						);
					} break;

					case sema::Expr::Kind::GLOBAL_VAR: {
						return this->handler.createGlobalValue(this->data.get_global_var(addr_of_target.globalVarID()));
					} break;

					default: {
						evo::debugFatalBreak("Not valid global var value (addr of)");
					} break;
				}
			} break;

			case sema::Expr::Kind::CONVERSION_TO_OPTIONAL: {
				const sema::ConversionToOptional& conversion_to_optional =
					this->context.getSemaBuffer().getConversionToOptional(expr.conversionToOptionalID());

				const TypeInfo& target_type =
					this->context.getTypeManager().getTypeInfo(conversion_to_optional.targetTypeID);

				if(target_type.isPointer()){
					return this->get_global_var_value(conversion_to_optional.expr);
				}


				return this->module.createGlobalStruct(
					this->get_type<false, false>(conversion_to_optional.targetTypeID).type,
					evo::SmallVector<pir::GlobalVar::Value>{
						this->get_global_var_value(conversion_to_optional.expr),
						this->handler.createBoolean(true)
					}
				);
			} break;

			case sema::Expr::Kind::MAKE_INTERFACE_PTR: {
				const sema::MakeInterfacePtr& make_interface_ptr =
					this->context.getSemaBuffer().getMakeInterfacePtr(expr.makeInterfacePtrID());

				const BaseType::Interface& interface_type =
					this->context.getTypeManager().getInterface(make_interface_ptr.interfaceID);

				const auto vtable_id = Data::VTableID(make_interface_ptr.interfaceID, make_interface_ptr.implTypeID);

				const pir::GlobalVar::Value vtable_value = [&]() -> auto {
					if(interface_type.methods.size() == 1){
						return this->handler.createFunctionPointer(this->data.get_single_method_vtable(vtable_id));
					}else{
						return this->handler.createGlobalValue(this->data.get_vtable(vtable_id));
					}
				}();

				const pir::Type interface_ptr_type =
					this->data.getInterfacePtrType(this->module, this->context.getSourceManager()).pir_type;

				return this->module.createGlobalStruct(
					interface_ptr_type,
					evo::SmallVector<pir::GlobalVar::Value>{
						this->get_global_var_value(make_interface_ptr.expr), vtable_value
					}
				);
			} break;

			case sema::Expr::Kind::DEFAULT_NEW: {
				const sema::DefaultNew& default_new = this->context.getSemaBuffer().getDefaultNew(expr.defaultNewID());

				const TypeInfo& target_type = this->context.getTypeManager().getTypeInfo(
					this->context.type_manager.decayType<true, true>(default_new.targetTypeID)
				);

				if(target_type.isPointer()){
					evo::debugAssert(target_type.isOptional(), "Pointer (non-optional) is not default-initializable");
					return this->handler.createNullptr();
				}

				if(target_type.isOptional()){
					return this->module.createGlobalStruct(
						this->get_type<false, false>(default_new.targetTypeID).type,
						evo::SmallVector<pir::GlobalVar::Value>{
							pir::GlobalVar::Uninit(),
							this->handler.createBoolean(false)
						}
					);
				}

				switch(target_type.baseTypeID().kind()){
					case BaseType::Kind::DUMMY: evo::debugFatalBreak("Invalid base type");

					case BaseType::Kind::PRIMITIVE: {
						// TODO(FUTURE): more specific?
						return pir::GlobalVar::Value(pir::GlobalVar::Zeroinit());
					} break;

					case BaseType::Kind::ARRAY: {
						// TODO(FUTURE): more specific?
						return pir::GlobalVar::Value(pir::GlobalVar::Zeroinit());
					} break;

					case BaseType::Kind::ARRAY_REF: {
						// TODO(FUTURE): more specific?
						return pir::GlobalVar::Value(pir::GlobalVar::Zeroinit());
					} break;

					case BaseType::Kind::STRUCT: {
						// TODO(FUTURE): more specific?
						return pir::GlobalVar::Value(pir::GlobalVar::Zeroinit());
					} break;

					case BaseType::Kind::ALIAS:
					case BaseType::Kind::DISTINCT_ALIAS:
					case BaseType::Kind::INTERFACE_MAP: {
						evo::debugFatalBreak("Type should have been decayed");
					} break;

					case BaseType::Kind::FUNCTION: 
					case BaseType::Kind::ARRAY_DEDUCER: 
					case BaseType::Kind::ARRAY_REF_DEDUCER: 
					case BaseType::Kind::STRUCT_TEMPLATE:
					case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER:
					case BaseType::Kind::ENUM:
					case BaseType::Kind::UNION:
					case BaseType::Kind::TYPE_DEDUCER:
					case BaseType::Kind::INTERFACE:
					case BaseType::Kind::POLY_INTERFACE_REF: {
						evo::debugFatalBreak("Not default-initializable");
					} break;
				}
			} break;

			case sema::Expr::Kind::INIT_ARRAY_REF: {
				const sema::InitArrayRef& init_array_ref =
					this->context.getSemaBuffer().getInitArrayRef(expr.initArrayRefID());

				const pir::GlobalVar::Value array_target = this->get_global_var_value(init_array_ref.expr);

				const pir::Type array_ref_type = this->data.getArrayRefType(
					this->module,
					this->context,
					init_array_ref.targetTypeID,
					[&](TypeInfo::ID id){ return *this->get_type<false, true>(id).meta_type_id; }
				).pir_type;

				const uint64_t num_bits_of_ptr = this->context.getTypeManager().numBitsOfPtr();

				auto values = evo::SmallVector<pir::GlobalVar::Value>();
				values.reserve(init_array_ref.dimensions.size() + 1);
				values.emplace_back(array_target);
				for(const evo::Variant<uint64_t, sema::Expr>& dimension : init_array_ref.dimensions){
					if(dimension.is<uint64_t>()){
						values.emplace_back(
							this->handler.createNumber(
								this->module.createUnsignedType(uint32_t(num_bits_of_ptr)),
								core::GenericInt(unsigned(num_bits_of_ptr), dimension.as<uint64_t>())
							)
						);
					}else{
						values.emplace_back(this->get_global_var_value(dimension.as<sema::Expr>()));
					}
				}

				return this->module.createGlobalStruct(array_ref_type, std::move(values));
			} break;

			case sema::Expr::Kind::UNION_DESIGNATED_INIT_NEW: {
				const sema::UnionDesignatedInitNew& union_designated_init_new =
					this->context.getSemaBuffer().getUnionDesignatedInitNew(expr.unionDesignatedInitNewID());

				const BaseType::Union& union_type =
					this->context.getTypeManager().getUnion(union_designated_init_new.unionTypeID);

				const pir::Type union_pir_type = this->data.get_union(union_designated_init_new.unionTypeID);

				const core::GenericValue generic_value = [&]() -> core::GenericValue {
					if(union_designated_init_new.value.kind() != sema::Expr::Kind::NULL_VALUE){
						return sema::exprToGenericValue(union_designated_init_new.value, this->context);
					}else{
						return core::GenericValue();
					}
				}();


				const size_t data_size = [&]() -> size_t {
					if(union_type.isUntagged){
						return this->module.getArrayType(union_pir_type).length;
					}else{
						return this->module.getArrayType(this->module.getStructType(union_pir_type).members[0]).length;
					}
				}();

				auto byte_array_data = evo::SmallVector<std::byte>();
				byte_array_data.resize(data_size);

				std::memcpy(byte_array_data.data(), generic_value.dataRange().data(), generic_value.dataRange().size());

				if(generic_value.dataRange().size() != data_size){
					std::memset(
						&byte_array_data[generic_value.dataRange().size()],
						0,
						data_size - generic_value.dataRange().size()
					);
				}


				const pir::GlobalVar::ByteArray::ID pir_byte_array =
					this->module.createGlobalByteArray(std::move(byte_array_data));

				if(union_type.isUntagged){
					return pir_byte_array;
				}
				
				const pir::Type tag_type = this->module.getStructType(union_pir_type).members[1];
				const pir::Expr tag_value = this->handler.createNumber(
					tag_type,
					core::GenericInt(unsigned(tag_type.getWidth()), union_designated_init_new.fieldIndex)
				);

				auto aggregate_values = evo::SmallVector<pir::GlobalVar::Value>{pir_byte_array, tag_value};

				return this->module.createGlobalStruct(union_pir_type, std::move(aggregate_values));
			} break;

			case sema::Expr::Kind::GLOBAL_VAR: {
				const sema::GlobalVar& global_var = this->context.getSemaBuffer().getGlobalVar(expr.globalVarID());
				return this->get_global_var_value(*global_var.expr.load());
			} break;

			case sema::Expr::Kind::MODULE_IDENT:               case sema::Expr::Kind::INTRINSIC_FUNC:
			case sema::Expr::Kind::TEMPLATED_INTRINSIC_FUNC_INSTANTIATION:
			case sema::Expr::Kind::COPY:                       case sema::Expr::Kind::MOVE:
			case sema::Expr::Kind::FORWARD:                    case sema::Expr::Kind::FUNC_CALL:
			case sema::Expr::Kind::OPTIONAL_NULL_CHECK:        case sema::Expr::Kind::OPTIONAL_EXTRACT:
			case sema::Expr::Kind::DEREF:                      case sema::Expr::Kind::UNWRAP:
			case sema::Expr::Kind::ACCESSOR:                   case sema::Expr::Kind::UNION_ACCESSOR:
			case sema::Expr::Kind::LOGICAL_AND:                case sema::Expr::Kind::LOGICAL_OR:
			case sema::Expr::Kind::TRY_ELSE_EXPR:              case sema::Expr::Kind::TRY_ELSE_INTERFACE_EXPR:
			case sema::Expr::Kind::BLOCK_EXPR:                 case sema::Expr::Kind::FAKE_TERM_INFO:
			case sema::Expr::Kind::INTERFACE_PTR_EXTRACT_THIS: case sema::Expr::Kind::INTERFACE_CALL:
			case sema::Expr::Kind::INDEXER:                    case sema::Expr::Kind::ARRAY_REF_INDEXER:
			case sema::Expr::Kind::ARRAY_REF_SIZE:             case sema::Expr::Kind::ARRAY_REF_DIMENSIONS:
			case sema::Expr::Kind::ARRAY_REF_DATA:             case sema::Expr::Kind::UNION_TAG_CMP:
			case sema::Expr::Kind::SAME_TYPE_CMP:              case sema::Expr::Kind::PARAM:
			case sema::Expr::Kind::VARIADIC_PARAM:             case sema::Expr::Kind::RETURN_PARAM:
			case sema::Expr::Kind::ERROR_RETURN_PARAM:         case sema::Expr::Kind::BLOCK_EXPR_OUTPUT:
			case sema::Expr::Kind::EXCEPT_PARAM:               case sema::Expr::Kind::FOR_PARAM:
			case sema::Expr::Kind::VAR:                        case sema::Expr::Kind::FUNC: {
				evo::debugFatalBreak("Not valid global var value");
			} break;
		}

		evo::unreachable();
	}



	//////////////////////////////////////////////////////////////////////
	// get type


	template<bool MAY_LOWER_DEPENDENCY, bool GET_META>
	auto SemaToPIR::get_type(TypeInfo::VoidableID voidable_type_id) -> PIRType {
		if(voidable_type_id.isVoid()){
			return PIRType(this->module.createVoidType(), std::nullopt);
		}

		return this->get_type<MAY_LOWER_DEPENDENCY, GET_META>(voidable_type_id.asTypeID());
	}


	template<bool MAY_LOWER_DEPENDENCY, bool GET_META>
	auto SemaToPIR::get_type(TypeInfo::ID type_id) -> PIRType {
		const TypeInfo& type_info = this->context.getTypeManager().getTypeInfo(type_id);

		if(type_info.isPointer()){
			if constexpr(GET_META){
				if(this->data.config.includeDebugInfo == false){
					return PIRType(this->module.createPtrType(), std::nullopt);
				}

				std::string type_name = this->context.getTypeManager().printType(type_id, this->context);

				const TypeInfo::ID pointee_type_id = this->context.type_manager.getOrCreateTypeInfo(
					type_info.copyWithPoppedQualifier()
				);

				PIRType pointee_pir_type = this->get_type<MAY_LOWER_DEPENDENCY, true>(pointee_type_id);

				const pir::meta::QualifiedType::Qualifier qualifier = [&]() -> pir::meta::QualifiedType::Qualifier {
					if(type_info.qualifiers().back().isMut){
						return pir::meta::QualifiedType::Qualifier::MUT_POINTER;
					}else{
						return pir::meta::QualifiedType::Qualifier::POINTER;
					}
				}();

				return PIRType(
					this->module.createPtrType(),
					this->data.get_or_create_meta_pointer_qualified_type(
						type_id, this->module, std::move(type_name), *pointee_pir_type.meta_type_id, qualifier
					)
				);

			}else{
				return PIRType(this->module.createPtrType(), std::nullopt);
			}
		}

		if(type_info.isOptionalNotPointer()){
			const auto lock = std::scoped_lock(this->data.optional_types_lock);
			const auto find = this->data.optional_types.find(&type_info);
			if(find != this->data.optional_types.end()){
				if constexpr(GET_META){
					return PIRType(find->second, *this->module.lookupMetaStructType(find->second));

				}else{
					return PIRType(find->second, std::nullopt);
				}
			}


			auto target_qualifiers = evo::SmallVector<TypeInfo::Qualifier>();
			target_qualifiers.reserve(type_info.qualifiers().size() - 1);
			for(size_t i = 0; i < type_info.qualifiers().size() - 1; i+=1){
				target_qualifiers.emplace_back(type_info.qualifiers()[i]);
			}

			const TypeInfo::ID target_type_id = this->context.type_manager.getOrCreateTypeInfo(
				TypeInfo(type_info.baseTypeID(), std::move(target_qualifiers))
			);

			const PIRType target_pir_type = this->get_type<MAY_LOWER_DEPENDENCY, GET_META>(target_type_id);

			const pir::Type created_struct = this->module.createStructType(
				std::format("PTHR.optional_{}", type_id.get()),
				evo::SmallVector<pir::Type>{target_pir_type.type, this->module.createBoolType()},
				true
			);

			this->data.optional_types.emplace(&type_info, created_struct);

			if constexpr(GET_META){
				std::string type_name = this->context.getTypeManager().printType(type_id, this->context);

				const pir::meta::BasicType::ID bool_meta_type = this->data.get_or_create_meta_basic_type(
					TypeManager::getTypeBool(), this->module, "Bool", this->module.createBoolType()
				);

				const pir::meta::StructType::ID created_meta_struct = this->module.createMetaStructType(
					created_struct,
					evo::copy(type_name),
					std::move(type_name),
					evo::SmallVector<pir::meta::StructType::Member>{
						pir::meta::StructType::Member(*target_pir_type.meta_type_id, "data"),
						pir::meta::StructType::Member(bool_meta_type, "flag"),
					},
					*this->current_source->getPIRMetaFileID(),
					*this->current_source->getPIRMetaFileID(),
					0
				);

				return PIRType(created_struct, created_meta_struct);

			}else{
				return PIRType(created_struct, std::nullopt);
			}
		}

		return this->get_type<MAY_LOWER_DEPENDENCY, GET_META>(type_id, type_info.baseTypeID());
	}


	template<bool MAY_LOWER_DEPENDENCY, bool GET_META>
	auto SemaToPIR::get_type(BaseType::ID base_type_id) -> PIRType {
		return this->get_type<MAY_LOWER_DEPENDENCY, GET_META>(
			this->context.type_manager.getOrCreateTypeInfo(TypeInfo(base_type_id)), base_type_id
		);
	}


	template<bool MAY_LOWER_DEPENDENCY, bool GET_META>
	auto SemaToPIR::get_type(TypeInfo::ID type_id, BaseType::ID base_type_id) -> PIRType {
		switch(base_type_id.kind()){
			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Not a valid base type");
			
			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = 
					this->context.getTypeManager().getPrimitive(base_type_id.primitiveID());

				switch(primitive.kind()){
					case Token::Kind::TYPE_INT: {
						const pir::Type pir_type = this->module.createSignedType(
							uint32_t(this->module.sizeOfGeneralRegister() * 8)
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "Int", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;


					case Token::Kind::TYPE_ISIZE: {
						const pir::Type pir_type = this->module.createSignedType(
							uint32_t(this->module.sizeOfPtr() * 8)
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "ISize", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_I_N: {
						const uint32_t bit_width = [&]() -> uint32_t {
							if(primitive.bitWidth() <= 8){
								return 8;

							}else if(primitive.bitWidth() <= 16){
								return 16;

							}else if(primitive.bitWidth() <= 32){
								return 32;

							}else{
								return uint32_t(ceil_to_multiple(primitive.bitWidth(), 64));
							}
						}();


						const pir::Type pir_type = this->module.createSignedType(bit_width);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(
										type_id, this->module, std::format("I{}", primitive.bitWidth()), pir_type
									)
								);

							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;


					case Token::Kind::TYPE_UINT: {
						const pir::Type pir_type = this->module.createUnsignedType(
							uint32_t(this->module.sizeOfGeneralRegister() * 8)
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "UInt", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_USIZE: {
						const pir::Type pir_type = this->module.createUnsignedType(
							uint32_t(this->module.sizeOfPtr() * 8)
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "USize", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_UI_N: {
						const uint32_t bit_width = [&]() -> uint32_t {
							if(primitive.bitWidth() <= 8){
								return 8;

							}else if(primitive.bitWidth() <= 16){
								return 16;

							}else if(primitive.bitWidth() <= 32){
								return 32;

							}else{
								return uint32_t(ceil_to_multiple(primitive.bitWidth(), 64));
							}
						}();


						const pir::Type pir_type = this->module.createUnsignedType(bit_width);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(
										type_id, this->module, std::format("UI{}", primitive.bitWidth()), pir_type
									)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_F16: {
						const pir::Type pir_type = this->module.createFloatType(16);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "F16", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_F32: {
						const pir::Type pir_type = this->module.createFloatType(32);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "F32", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_F64: {
						const pir::Type pir_type = this->module.createFloatType(64);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "F64", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_F80: {
						const pir::Type pir_type = this->module.createFloatType(80);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "F80", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_F128: {
						const pir::Type pir_type = this->module.createFloatType(128);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "F128", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;


					case Token::Kind::TYPE_BYTE: {
						const pir::Type pir_type = this->module.createUnsignedType(8);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "Byte", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_BOOL: {
						const pir::Type pir_type = this->module.createBoolType();

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "Bool", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_CHAR: {
						const pir::Type pir_type = this->module.createSignedType(8);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "Char", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;


					case Token::Kind::TYPE_RAWPTR: {
						const pir::Type pir_type = this->module.createPtrType();

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_pointer_qualified_type(
										type_id,
										this->module,
										"RawPtr",
										std::nullopt,
										pir::meta::QualifiedType::Qualifier::MUT_POINTER
									)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_TYPEID: {
						const pir::Type pir_type = this->module.createUnsignedType(32);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "TypeID", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_C_WCHAR: {
						const pir::Type pir_type = this->module.createSignedType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "CWChar", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_C_SHORT: {
						const pir::Type pir_type = this->module.createSignedType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "CShort", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_C_USHORT: {
						const pir::Type pir_type = this->module.createUnsignedType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "CUShort", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;


					case Token::Kind::TYPE_C_INT: {
						const pir::Type pir_type = this->module.createSignedType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "CInt", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_C_UINT: {
						const pir::Type pir_type = this->module.createUnsignedType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "CUInt", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_C_LONG: {
						const pir::Type pir_type = this->module.createSignedType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "CLong", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_C_ULONG: {
						const pir::Type pir_type = this->module.createUnsignedType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(type_id, this->module, "CULong", pir_type)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;


					case Token::Kind::TYPE_C_LONG_LONG: {
						const pir::Type pir_type = this->module.createSignedType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(
										type_id, this->module, "CLongLong", pir_type
									)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;


					case Token::Kind::TYPE_C_ULONG_LONG: {
						const pir::Type pir_type = this->module.createUnsignedType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(
										type_id, this->module, "CULongLong", pir_type
									)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					case Token::Kind::TYPE_C_LONG_DOUBLE: {
						const pir::Type pir_type = this->module.createFloatType(
							uint32_t(this->context.getTypeManager().numBits(base_type_id))
						);

						if constexpr(GET_META){
							if(this->data.config.includeDebugInfo){
								return PIRType(
									pir_type,
									this->data.get_or_create_meta_basic_type(
										type_id, this->module, "CULongLong", pir_type
									)
								);
							}else{
								return PIRType(pir_type, std::nullopt);
							}
						}else{
							return PIRType(pir_type, std::nullopt);
						}
					} break;

					default: evo::debugFatalBreak("Unknown builtin type");
				}
			} break;
			
			case BaseType::Kind::FUNCTION: {
				evo::unimplemented("BaseType::Kind::FUNCTION");
			} break;
			
			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->context.getTypeManager().getArray(base_type_id.arrayID());
				const PIRType elem_type = this->get_type<MAY_LOWER_DEPENDENCY, GET_META>(array.elementTypeID);

				const pir::Type pir_type = [&]() -> pir::Type {
					if(array.dimensions.size() == 1){
						return this->module.getOrCreateArrayType(
							elem_type.type, array.dimensions.back() + uint64_t(array.terminator.has_value())
						);
						
					}else{
						pir::Type array_type =
							this->module.getOrCreateArrayType(elem_type.type, array.dimensions.back());

						if(array.dimensions.size() > 1){
							for(ptrdiff_t i = array.dimensions.size() - 2; i >= 0; i-=1){
								array_type = this->module.getOrCreateArrayType(array_type, array.dimensions[i]);
							}
						}

						return array_type;
					}
				}();

				if constexpr(GET_META){
					const pir::meta::ArrayType::ID meta_type = this->data.get_or_create_meta_array_type(
						base_type_id.arrayID(),
						this->module,
						this->context.getTypeManager(),
						this->context,
						pir_type,
						*elem_type.meta_type_id,
						evo::copy(array.dimensions)
					);

					return PIRType(pir_type, meta_type);

				}else{
					return PIRType(pir_type, std::nullopt);
				}
			} break;

			case BaseType::Kind::ARRAY_DEDUCER: {
				evo::debugFatalBreak("Cannot get type of array deducer");
			} break;

			case BaseType::Kind::ARRAY_REF: {
				const Data::PIRType& pir_type_info = this->data.getArrayRefType(
					this->module,
					this->context,
					base_type_id.arrayRefID(),
					[&](TypeInfo::ID id){ return *this->get_type<false, true>(id).meta_type_id; }
				);

				if constexpr(GET_META){
					return PIRType(pir_type_info.pir_type, pir_type_info.meta_type_id);

				}else{
					return PIRType(pir_type_info.pir_type, std::nullopt);
				}
			} break;
			
			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->context.getTypeManager().getAlias(base_type_id.aliasID());
				return this->get_type<MAY_LOWER_DEPENDENCY, GET_META>(alias.aliasedType);
			} break;
			
			case BaseType::Kind::DISTINCT_ALIAS: {
				const BaseType::DistinctAlias& distinct_alias_type = 
					this->context.getTypeManager().getDistinctAlias(base_type_id.distinctAliasID());
				return this->get_type<MAY_LOWER_DEPENDENCY, GET_META>(distinct_alias_type.underlyingType);
			} break;
			
			case BaseType::Kind::STRUCT: {
				if constexpr(MAY_LOWER_DEPENDENCY){
					if(this->data.has_struct(base_type_id.structID()) == false){
						this->lowerStructAndDepsIfNeeded(base_type_id.structID());
					}
				}

				const pir::Type pir_type = this->data.get_struct(base_type_id.structID());

				if constexpr(GET_META){
					return PIRType(pir_type, *this->module.lookupMetaStructType(pir_type));

				}else{
					return PIRType(pir_type, std::nullopt);
				}
			} break;


			case BaseType::Kind::UNION: {
				if constexpr(MAY_LOWER_DEPENDENCY){
					if(this->data.has_union(base_type_id.unionID()) == false){
						this->lowerUnionAndDepsIfNeeded(base_type_id.unionID());
					}
				}

				const pir::Type pir_type = this->data.get_union(base_type_id.unionID());

				if constexpr(GET_META){
					return PIRType(pir_type, this->data.get_meta_union(base_type_id.unionID()));

				}else{
					return PIRType(pir_type, std::nullopt);
				}
			} break;

			case BaseType::Kind::ENUM: {
				const BaseType::Enum& enum_type = this->context.getTypeManager().getEnum(base_type_id.enumID());
				const pir::Type pir_type =
					this->get_type<MAY_LOWER_DEPENDENCY, false>(BaseType::ID(enum_type.underlyingTypeID)).type;

				if constexpr(GET_META){
					return PIRType(pir_type, this->data.get_meta_enum(base_type_id.enumID()));

				}else{
					return PIRType(pir_type, std::nullopt);
				}
			} break;
			
			case BaseType::Kind::STRUCT_TEMPLATE: {
				evo::debugFatalBreak("Cannot get type of struct template");
			} break;

			case BaseType::Kind::STRUCT_TEMPLATE_DEDUCER: {
				evo::debugFatalBreak("Cannot get type of struct template deducer");
			} break;

			case BaseType::Kind::TYPE_DEDUCER: {
				evo::debugFatalBreak("Cannot get type of type deducer");
			} break;

			case BaseType::Kind::INTERFACE: {
				evo::debugFatalBreak("Cannot get type of interface");
			} break;

			case BaseType::Kind::POLY_INTERFACE_REF: {
				const Data::PIRType pir_type =
					this->data.getInterfacePtrType(this->module, this->context.getSourceManager());

				if constexpr(GET_META){
					return PIRType(pir_type.pir_type, pir_type.meta_type_id);
					
				}else{
					return PIRType(pir_type.pir_type, std::nullopt);
				}
			} break;

			case BaseType::Kind::INTERFACE_MAP: {
				const BaseType::InterfaceMap& interface_map =
					this->context.getTypeManager().getInterfaceMap(base_type_id.interfaceMapID());

				return this->get_type<MAY_LOWER_DEPENDENCY, GET_META>(
					this->context.getTypeManager().getTypeInfo(
						interface_map.underlyingTypeID
					).baseTypeID()
				);
			} break;
		}

		evo::debugFatalBreak("Unknown base type");
	}




	auto SemaToPIR::get_location(const Diagnostic::Location& location) const -> Location {
		pir::meta::File::ID meta_file_id = pir::meta::File::ID::dummy();
		uint32_t line_number = 0;
		uint32_t collumn_number = 0;

		location.visit([&](const auto& location) -> void {
			using LocationType = std::decay_t<decltype(location)>;

			if constexpr(std::is_same<LocationType, Diagnostic::Location::None>()){
				// do nothing

			}else if constexpr(std::is_same<LocationType, Diagnostic::Location::Builtin>()){
				// do nothing

			}else if constexpr(std::is_same<LocationType, SourceLocation>()){
				meta_file_id = *this->context.getSourceManager()[location.sourceID].getPIRMetaFileID();
				line_number = location.lineStart;
				collumn_number = location.collumnStart;

			}else if constexpr(std::is_same<LocationType, ClangSourceLocation>()){
				// TODO(FUTURE): 
				evo::unimplemented("Getting debug location of clang source file");

			}else{
				static_assert(false, "Unknown location");
			}
		});

		return Location(meta_file_id, line_number, collumn_number);
	}


	auto SemaToPIR::get_current_meta_scope() const -> pir::meta::Scope {
		return *this->current_source->getPIRMetaFileID();
	}

	auto SemaToPIR::get_current_meta_local_scope() const -> pir::meta::LocalScope {
		// return *this->module.getFunction(
		// 	this->current_func_info->pir_ids[this->in_param_bitmap].as<pir::Function::ID>()
		// ).getMetaID();

		return this->local_scopes.top();
	}



	//////////////////////////////////////////////////////////////////////
	// unmangled name

	auto SemaToPIR::get_unmangled_func_name(const sema::Func& func) const -> std::string {
		if(func.isClangFunc()){
			return std::string(func.getName(this->context.getSourceManager()));

		}else if(func.attributes.isExport){
			const Source& source = this->context.getSourceManager()[func.sourceID.as<Source::ID>()];
			return std::string(source.getTokenBuffer()[func.name.as<Token::ID>()].getString());

		}else{
			const Source& source = this->context.getSourceManager()[func.sourceID.as<Source::ID>()];

			auto op_kind = std::optional<Token::Kind>();

			if(func.name.is<Token::ID>()){
				const Token& name_token = source.getTokenBuffer()[func.name.as<Token::ID>()];

				if(name_token.kind() == Token::Kind::IDENT){
					std::string output = std::format(
						"{}{}",
						this->get_parent_name<false>(func.parent, func.sourceID),
						name_token.getString()
					);

					if(func.templated_func_id.has_value()){
						const sema::TemplatedFunc& templated_func =
							this->context.getSemaBuffer().getTemplatedFunc(*func.templated_func_id);

						const evo::SmallVector<sema::TemplatedFunc::Arg> template_args =
							templated_func.getInstantiationArgs(func.instanceID);


						output += "<{";

						const TypeManager& type_manager = this->context.getTypeManager();

						for(size_t i = 0; const sema::TemplatedFunc::Arg& template_arg : template_args){
							if(template_arg.is<TypeInfo::VoidableID>()){
								output += this->context.getTypeManager().printType(
									template_arg.as<TypeInfo::VoidableID>(), context
								);

							}else if(*templated_func.templateParams[i].typeID == TypeManager::getTypeBool()){
								output += evo::boolStr(template_arg.as<core::GenericValue>().getBool());

							}else if(*templated_func.templateParams[i].typeID == TypeManager::getTypeChar()){
								output += "'";
								output += template_arg.as<core::GenericValue>().getChar();
								output += "'";

							}else if(type_manager.isUnsignedIntegral(*templated_func.templateParams[i].typeID)){
								output += template_arg.as<core::GenericValue>().getInt(
									unsigned(type_manager.numBits(*templated_func.templateParams[i].typeID))
								).toString(false);

							}else if(type_manager.isIntegral(*templated_func.templateParams[i].typeID)){
								output += template_arg.as<core::GenericValue>().getInt(
									unsigned(type_manager.numBits(*templated_func.templateParams[i].typeID))
								).toString(true);

							}else if(type_manager.isFloatingPoint(*templated_func.templateParams[i].typeID)){
								const BaseType::Primitive& primitive = type_manager.getPrimitive(
									type_manager.getTypeInfo(*templated_func.templateParams[i].typeID)
										.baseTypeID().primitiveID()
								);

								const core::GenericValue& generic_value = template_arg.as<core::GenericValue>();

								switch(primitive.kind()){
									break; case Token::Kind::TYPE_F16:
										output += generic_value.getF16().toString();

									break; case Token::Kind::TYPE_F32:
										output += generic_value.getF32().toString();

									break; case Token::Kind::TYPE_F64:
										output += generic_value.getF64().toString();

									break; case Token::Kind::TYPE_F80:
										output += generic_value.getF80().toString();

									break; case Token::Kind::TYPE_F128:
										output += generic_value.getF128().toString();

									break; default: evo::debugFatalBreak("Unknown float type");
								}
								
							}else{
								output += "<EXPR>";
							}

							if(i + 1 < template_args.size()){
								output += ", ";
								i += 1;
							}
						}

						output += "}>";
					}

					return output;

				}else{
					op_kind = name_token.kind();
				}

			}else{
				op_kind = func.name.as<sema::Func::CompilerCreatedOpOverload>().overloadKind;
			}


			switch(*op_kind){
				case Token::lookupKind("["): {
					return std::format("{}[]", this->get_parent_name<false>(func.parent, func.sourceID));
				} break;

				case Token::Kind::KEYWORD_COPY: {
					if(func.params.size() == 1){
						return std::format("{}copy.init", this->get_parent_name<false>(func.parent, func.sourceID));	
					}else{
						return std::format("{}copy.assign", this->get_parent_name<false>(func.parent, func.sourceID));
					}
				} break;

				case Token::Kind::KEYWORD_MOVE: {
					if(func.params.size() == 1){
						return std::format("{}move.init", this->get_parent_name<false>(func.parent, func.sourceID));	
					}else{
						return std::format("{}move.assign", this->get_parent_name<false>(func.parent, func.sourceID));
					}
				} break;

				case Token::Kind::KEYWORD_NEW: {
					const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.typeID);

					if(func_type.returnTypes.size() == 1){
						return std::format("{}new.init", this->get_parent_name<false>(func.parent, func.sourceID));	
					}else{
						return std::format("{}new.assign", this->get_parent_name<false>(func.parent, func.sourceID));
					}
				} break;


				case Token::Kind::KEYWORD_AS: {
					const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.typeID);

					return std::format(
						"{}as.{}",
						this->get_parent_name<false>(func.parent, func.sourceID),
						this->context.getTypeManager().printType(func_type.returnTypes[0].asTypeID(), this->context)
					);
				} break;

				default: {
					return std::format(
						"{}{}", this->get_parent_name<false>(func.parent, func.sourceID), Token::printKind(*op_kind)
					);
				} break;
			}
		}
	}




	auto SemaToPIR::get_unmangled_struct_name(BaseType::Struct::ID struct_id) const -> std::string {
		return this->context.getTypeManager().printType(BaseType::ID(struct_id), this->context);
	}


	auto SemaToPIR::get_unmangled_union_name(BaseType::Union::ID union_id) const -> std::string {
		return this->context.getTypeManager().printType(BaseType::ID(union_id), this->context);
	}






	//////////////////////////////////////////////////////////////////////
	// name mangling

	template<bool PIR_STMT_NAME_SAFE>
	auto SemaToPIR::mangle_name(BaseType::Struct::ID struct_id) const -> std::string {
		if(this->data.getConfig().useReadableNames){
			const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(struct_id);

			if(struct_type.isClangType()){
				return std::format("struct.{}", struct_type.getName(this->context.getSourceManager()));

			}else{
				if constexpr(PIR_STMT_NAME_SAFE){
					return std::format(
						"PTHR.s{}.{}{}",
						struct_id.get(),
						this->get_parent_name<true>(struct_type.parent, struct_type.sourceID),
						struct_type.getName(this->context.getSourceManager())
					);

				}else{
					return std::format(
						"PTHR.s{}-{}",
						struct_id.get(),
						this->context.getTypeManager().printType(BaseType::ID(struct_id), this->context) 
					);
				}
			}

		}else{
			return std::format("PTHR.s{}", struct_id.get());
		}
	}

	template<bool PIR_STMT_NAME_SAFE>
	auto SemaToPIR::mangle_name(BaseType::Union::ID union_id) const -> std::string {
		const BaseType::Union& union_type = this->context.getTypeManager().getUnion(union_id);

		if(union_type.isClangType()){
			return std::format("union.{}", union_type.getName(this->context.getSourceManager()));
			
		}else if(this->data.getConfig().useReadableNames){
			if constexpr(PIR_STMT_NAME_SAFE){
				return std::format(
					"PTHR.u{}.{}{}",
					union_id.get(),
					this->get_parent_name<true>(union_type.parent, union_type.sourceID),
					union_type.getName(this->context.getSourceManager())
				);
				
			}else{
				return std::format(
					"PTHR.u{}-{}",
					union_id.get(),
					this->context.getTypeManager().printType(BaseType::ID(union_id), this->context)
				);
			}
			
		}else{
			return std::format("PTHR.u{}", union_id.get());
		}
	}


	template<bool PIR_STMT_NAME_SAFE>
	auto SemaToPIR::mangle_name(BaseType::Interface::ID interface_id) const -> std::string {
		if(this->data.getConfig().useReadableNames){
			if constexpr(PIR_STMT_NAME_SAFE){
				const BaseType::Interface& interface_type = this->context.getTypeManager().getInterface(interface_id);

				return std::format(
					"PTHR.i{}.{}{}",
					interface_id.get(),
					this->get_parent_name<true>(interface_type.parent, interface_type.sourceID),
					interface_type.getName(this->context.getSourceManager())
				);
				
			}else{
				return std::format(
					"PTHR.i{}-{}",
					interface_id.get(),
					this->context.getTypeManager().printType(BaseType::ID(interface_id), this->context)
				);
			}

		}else{
			return std::format("PTHR.i{}", interface_id.get());
		}
	}



	template<bool PIR_STMT_NAME_SAFE>
	auto SemaToPIR::mangle_name(sema::GlobalVar::ID global_var_id) const -> std::string {
		const sema::GlobalVar& global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);

		if(global_var.isClangVar()){
			if constexpr(PIR_STMT_NAME_SAFE){
				return std::string(global_var.getName(this->context.getSourceManager()));
			}else{
				return global_var.clangMangledName;
			}

		}else{
			const Source& source = this->context.getSourceManager()[global_var.sourceID.as<Source::ID>()];
			if(this->data.getConfig().useReadableNames){
				if constexpr(PIR_STMT_NAME_SAFE){
					return std::format(
						"PTHR.g{}.{}{}",
						global_var_id.get(),
						this->get_parent_name<PIR_STMT_NAME_SAFE>(std::nullopt, global_var.sourceID),
						source.getTokenBuffer()[global_var.ident.as<Token::ID>()].getString()
					);
					
				}else{
					return std::format(
						"PTHR.g{}-{}{}",
						global_var_id.get(),
						this->get_parent_name<PIR_STMT_NAME_SAFE>(std::nullopt, global_var.sourceID),
						source.getTokenBuffer()[global_var.ident.as<Token::ID>()].getString()
					);
				}
				
			}else{
				return std::format("PTHR.g{}", global_var_id.get());
			}
		}
	}


	template<bool PIR_STMT_NAME_SAFE>
	auto SemaToPIR::mangle_name(sema::Func::ID func_id) const -> std::string {
		const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);

		if(func.isClangFunc()){
			if constexpr(PIR_STMT_NAME_SAFE){
				return std::string(func.getName(this->context.getSourceManager()));
			}else{
				return func.clangMangledName;
			}

		}else if(func.attributes.isExport){
			const Source& source = this->context.getSourceManager()[func.sourceID.as<Source::ID>()];
			return std::string(source.getTokenBuffer()[func.name.as<Token::ID>()].getString());

		}else{
			const Source& source = this->context.getSourceManager()[func.sourceID.as<Source::ID>()];

			if(func.name.is<Token::ID>()){
				const Token& name_token = source.getTokenBuffer()[func.name.as<Token::ID>()];
				if(name_token.kind() == Token::Kind::IDENT){
					if(this->data.getConfig().useReadableNames){
						if constexpr(PIR_STMT_NAME_SAFE){
							return std::format(
								"PTHR.f{}.{}{}",
								func_id.get(),
								this->get_parent_name<PIR_STMT_NAME_SAFE>(func.parent, func.sourceID),
								name_token.getString()
							);

						}else{
							std::string output = std::format(
								"PTHR.f{}-{}{}",
								func_id.get(),
								this->get_parent_name<PIR_STMT_NAME_SAFE>(func.parent, func.sourceID),
								name_token.getString()
							);

							if(func.templated_func_id.has_value()){
								const sema::TemplatedFunc& templated_func =
									this->context.getSemaBuffer().getTemplatedFunc(*func.templated_func_id);

								const evo::SmallVector<sema::TemplatedFunc::Arg> template_args =
									templated_func.getInstantiationArgs(func.instanceID);


								output += "<{";

								const TypeManager& type_manager = this->context.getTypeManager();

								for(size_t i = 0; const sema::TemplatedFunc::Arg& template_arg : template_args){
									if(template_arg.is<TypeInfo::VoidableID>()){
										output += this->context.getTypeManager().printType(
											template_arg.as<TypeInfo::VoidableID>(), context
										);

									}else if(*templated_func.templateParams[i].typeID == TypeManager::getTypeBool()){
										output += evo::boolStr(template_arg.as<core::GenericValue>().getBool());

									}else if(*templated_func.templateParams[i].typeID == TypeManager::getTypeChar()){
										output += "'";
										output += template_arg.as<core::GenericValue>().getChar();
										output += "'";

									}else if(type_manager.isUnsignedIntegral(*templated_func.templateParams[i].typeID)){
										output += template_arg.as<core::GenericValue>().getInt(
											unsigned(type_manager.numBits(*templated_func.templateParams[i].typeID))
										).toString(false);

									}else if(type_manager.isIntegral(*templated_func.templateParams[i].typeID)){
										output += template_arg.as<core::GenericValue>().getInt(
											unsigned(type_manager.numBits(*templated_func.templateParams[i].typeID))
										).toString(true);

									}else if(type_manager.isFloatingPoint(*templated_func.templateParams[i].typeID)){
										const BaseType::Primitive& primitive = type_manager.getPrimitive(
											type_manager.getTypeInfo(*templated_func.templateParams[i].typeID)
												.baseTypeID().primitiveID()
										);

										const core::GenericValue& generic_value = template_arg.as<core::GenericValue>();

										switch(primitive.kind()){
											break; case Token::Kind::TYPE_F16:
												output += generic_value.getF16().toString();

											break; case Token::Kind::TYPE_F32:
												output += generic_value.getF32().toString();

											break; case Token::Kind::TYPE_F64:
												output += generic_value.getF64().toString();

											break; case Token::Kind::TYPE_F80:
												output += generic_value.getF80().toString();

											break; case Token::Kind::TYPE_F128:
												output += generic_value.getF128().toString();

											break; default: evo::debugFatalBreak("Unknown float type");
										}
										
									}else{
										output += "<EXPR>";
									}

									if(i + 1 < template_args.size()){
										output += ", ";
										i += 1;
									}
								}

								output += "}>";
							}

							return output;
						}


					}else{
						return std::format("PTHR.f{}", func_id.get());
					}

				}else{
					return this->mangle_name<PIR_STMT_NAME_SAFE>(func_id, name_token.kind());
				}
			}else{
				return this->mangle_name<PIR_STMT_NAME_SAFE>(
					func_id, func.name.as<sema::Func::CompilerCreatedOpOverload>().overloadKind
				);
			}
		}
	}


	template<bool PIR_STMT_NAME_SAFE>
	auto SemaToPIR::mangle_name(sema::Func::ID func_id, Token::Kind op_kind) const -> std::string {
		if(this->data.getConfig().useReadableNames == false){
			return std::format("PTHR.f{}", func_id.get());
		}

		const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);

		if constexpr(PIR_STMT_NAME_SAFE){
			switch(op_kind){
				// prefix keywords
				case Token::Kind::KEYWORD_COPY: {
					if(func.params.size() == 1){
						return std::format(
							"PTHR.f{}.{}OP_copy_init",
							func_id.get(),
							this->get_parent_name<true>(func.parent, func.sourceID)
						);
					}else{
						return std::format(
							"PTHR.f{}.{}OP_copy_assign",
							func_id.get(),
							this->get_parent_name<true>(func.parent, func.sourceID)
						);
					}
				} break;

				case Token::Kind::KEYWORD_MOVE: {
					if(func.params.size() == 1){
						return std::format(
							"PTHR.f{}.{}OP_move_init",
							func_id.get(),
							this->get_parent_name<true>(func.parent, func.sourceID)
						);
					}else{
						return std::format(
							"PTHR.f{}.{}OP_move_assign",
							func_id.get(),
							this->get_parent_name<true>(func.parent, func.sourceID)
						);
					}
				} break;

				case Token::Kind::KEYWORD_NEW: {
					const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.typeID);

					if(func_type.returnTypes.size() == 1){
						return std::format(
							"PTHR.f{}.{}OP_new_init",
							func_id.get(),
							this->get_parent_name<true>(func.parent, func.sourceID)
						);
					}else{
						return std::format(
							"PTHR.f{}.{}OP_new_assign",
							func_id.get(),
							this->get_parent_name<true>(func.parent, func.sourceID)
						);
					}
				} break;

				case Token::Kind::KEYWORD_DELETE: {
					return std::format(
						"PTHR.f{}.{}OP_delete", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::Kind::KEYWORD_AS: {
					return std::format(
						"PTHR.f{}.{}OP_as", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;


				// assignment
				case Token::lookupKind("+="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_ADD",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("+%="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_ADD_WRAP",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("+|="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_ADD_SAT",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("-="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_SUB",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("-%="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_SUB_WRAP",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("-|="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_SUB_SAT",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("*="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_MUL",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("*%="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_MUL_WRAP",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("*|="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_MUL_SAT",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("/="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_DIV",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("%="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_MOD",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("<<="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_SHIFT_LEFT",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("<<|="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_SHIFT_LEFT_SAT",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind(">>="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_SHIFT_RIGHT",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("&="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_BITWISE_AND",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("|="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_BITWISE_OR",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("^="): {
					return std::format(
						"PTHR.f{}.{}OP_ASSIGN_BITWISE_XOR",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;


				// arithmetic
				case Token::lookupKind("+"): {
					return std::format(
						"PTHR.f{}.{}OP_PLUS", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("+%"): {
					return std::format(
						"PTHR.f{}.{}OP_ADD_WRAP",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("+|"): {
					return std::format(
						"PTHR.f{}.{}OP_ADD_SAT", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("-"): {
					return std::format(
						"PTHR.f{}.{}OP_MINUS", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("-%"): {
					return std::format(
						"PTHR.f{}.{}OP_SUB_WRAP",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("-|"): {
					return std::format(
						"PTHR.f{}.{}OP_SUB_SAT", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("*"): {
					return std::format(
						"PTHR.f{}.{}OP_ASTERISK",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("*%"): {
					return std::format(
						"PTHR.f{}.{}OP_MUL_WRAP",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("*|"): {
					return std::format(
						"PTHR.f{}.{}OP_MUL_SAT", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("/"): {
					return std::format(
						"PTHR.f{}.{}OP_FORWARD_SLASH",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("%"): {
					return std::format(
						"PTHR.f{}.{}OP_MOD", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;


				// comparative
				case Token::lookupKind("=="): {
					return std::format(
						"PTHR.f{}.{}OP_EQUAL", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("!="): {
					return std::format(
						"PTHR.f{}.{}OP_NOT_EQUAL",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("<"): {
					return std::format(
						"PTHR.f{}.{}OP_LESS_THAN",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("<="): {
					return std::format(
						"PTHR.f{}.{}OP_LESS_THAN_EQUAL",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind(">"): {
					return std::format(
						"PTHR.f{}.{}OP_GREATER_THAN",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind(">="): {
					return std::format(
						"PTHR.f{}.{}OP_GREATER_THAN_EQUAL",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;


				// logical
				case Token::lookupKind("!"): {
					return std::format(
						"PTHR.f{}.{}OP_NOT", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("&&"): {
					return std::format(
						"PTHR.f{}.{}OP_AND", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("||"): {
					return std::format(
						"PTHR.f{}.{}OP_OR", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;


				// bitwise
				case Token::lookupKind("<<"): {
					return std::format(
						"PTHR.f{}.{}OP_SHIFT_LEFT",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("<<|"): {
					return std::format(
						"PTHR.f{}.{}OP_SHIFT_LEFT_SAT",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind(">>"): {
					return std::format(
						"PTHR.f{}.{}OP_SHIFT_RIGHT",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("&"): {
					return std::format(
						"PTHR.f{}.{}OP_BITWISE_AND",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("|"): {
					return std::format(
						"PTHR.f{}.{}OP_BITWISE_OR",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("^"): {
					return std::format(
						"PTHR.f{}.{}OP_BITWISE_XOR",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;

				case Token::lookupKind("~"): {
					return std::format(
						"PTHR.f{}.{}OP_BITWISE_NOT",
						func_id.get(),
						this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;


				case Token::lookupKind("["): {
					return std::format(
						"PTHR.f{}.{}OP_INDEXER", func_id.get(), this->get_parent_name<true>(func.parent, func.sourceID)
					);
				} break;


				default: {
					evo::debugFatalBreak("Unknown overload op ({})", Token::printKind(op_kind));
				};
			}

		}else{
			switch(op_kind){
				case Token::lookupKind("["): {
					return std::format(
						"PTHR.f{}-{}[]", func_id.get(), this->get_parent_name<false>(func.parent, func.sourceID)
					);
				} break;

				case Token::Kind::KEYWORD_COPY: {
					if(func.params.size() == 1){
						return std::format(
							"PTHR.f{}-{}copy-init",
							func_id.get(),
							this->get_parent_name<false>(func.parent, func.sourceID)
						);	
					}else{
						return std::format(
							"PTHR.f{}-{}copy-assign",
							func_id.get(),
							this->get_parent_name<false>(func.parent, func.sourceID)
						);
					}
				} break;

				case Token::Kind::KEYWORD_MOVE: {
					if(func.params.size() == 1){
						return std::format(
							"PTHR.f{}-{}move-init",
							func_id.get(),
							this->get_parent_name<false>(func.parent, func.sourceID)
						);	
					}else{
						return std::format(
							"PTHR.f{}-{}move-assign",
							func_id.get(),
							this->get_parent_name<false>(func.parent, func.sourceID)
						);
					}
				} break;

				case Token::Kind::KEYWORD_NEW: {
					const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.typeID);

					if(func_type.returnTypes.size() == 1){
						return std::format(
							"PTHR.f{}-{}new-init",
							func_id.get(),
							this->get_parent_name<false>(func.parent, func.sourceID)
						);	
					}else{
						return std::format(
							"PTHR.f{}-{}new-assign",
							func_id.get(),
							this->get_parent_name<false>(func.parent, func.sourceID)
						);
					}
				} break;


				case Token::Kind::KEYWORD_AS: {
					const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.typeID);

					return std::format(
						"PTHR.f{}-{}as-{}",
						func_id.get(),
						this->get_parent_name<false>(func.parent, func.sourceID),
						this->context.getTypeManager().printType(func_type.returnTypes[0].asTypeID(), this->context)
					);
				} break;

				default: {
					return std::format(
						"PTHR.f{}-{}{}",
						func_id.get(),
						this->get_parent_name<false>(func.parent, func.sourceID),
						Token::printKind(op_kind)
					);
				} break;
			}
		}
	}


	template<bool PIR_STMT_NAME_SAFE>
	auto SemaToPIR::get_parent_name(
		std::optional<EncapsulatingSymbolID> parent,
		evo::Variant<Source::ID, ClangSource::ID, BuiltinModule::ID> source_id
	) const -> std::string {
		evo::debugAssert(
			this->data.getConfig().useReadableNames, "Should only get parent name if using readable names"
		);

		if(parent.has_value()){
			return parent->visit([&](const auto& parent_id) -> std::string {
				using ParentIDType = std::decay_t<decltype(parent_id)>;

				if constexpr(std::is_same<ParentIDType, BaseType::StructID>()){
					if constexpr(PIR_STMT_NAME_SAFE){
						const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(parent_id);
						return std::format(
							"{}{}.",
							this->get_parent_name<true>(struct_type.parent, struct_type.sourceID),
							std::string(struct_type.getName(this->context.getSourceManager()))
						);
					}else{
						std::string type_name =
							this->context.getTypeManager().printType(BaseType::ID(parent_id), this->context);
						type_name += '.';
						return type_name;
					}
					
				}else if constexpr(std::is_same<ParentIDType, BaseType::UnionID>()){
					if constexpr(PIR_STMT_NAME_SAFE){
						const BaseType::Union& union_type = this->context.getTypeManager().getUnion(parent_id);
						
						return std::format(
							"{}{}.",
							this->get_parent_name<true>(union_type.parent, union_type.sourceID),
							std::string(union_type.getName(this->context.getSourceManager()))
						);
					}else{
						std::string type_name =
							this->context.getTypeManager().printType(BaseType::ID(parent_id), this->context);
						type_name += '.';
						return type_name;
					}

				}else if constexpr(std::is_same<ParentIDType, BaseType::EnumID>()){
					if constexpr(PIR_STMT_NAME_SAFE){
						const BaseType::Enum& enum_type = this->context.getTypeManager().getEnum(parent_id);
						
						return std::format(
							"{}{}.",
							this->get_parent_name<true>(enum_type.parent, enum_type.sourceID),
							std::string(enum_type.getName(this->context.getSourceManager()))
						);
					}else{
						std::string type_name =
							this->context.getTypeManager().printType(BaseType::ID(parent_id), this->context);
						type_name += '.';
						return type_name;
					}

				}else if constexpr(std::is_same<ParentIDType, BaseType::InterfaceID>()){
					if constexpr(PIR_STMT_NAME_SAFE){
						const BaseType::Interface& interface_type =
							this->context.getTypeManager().getInterface(parent_id);
						
						return std::format(
							"{}{}.",
							this->get_parent_name<true>(interface_type.parent, interface_type.sourceID),
							std::string(interface_type.getName(this->context.getSourceManager()))
						);
					}else{
						std::string type_name =
							this->context.getTypeManager().printType(BaseType::ID(parent_id), this->context);
						type_name += '.';
						return type_name;
					}

				}else if constexpr(std::is_same<ParentIDType, sema::FuncID>()){
					const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(parent_id);
					
					return std::format(
						"{}{}.",
						this->get_parent_name<PIR_STMT_NAME_SAFE>(sema_func.parent, sema_func.sourceID),
						std::string(sema_func.getName(this->context.getSourceManager()))
					);

				}else if constexpr(std::is_same<ParentIDType, EncapsulatingSymbolID::InterfaceImplInfo>()){
					const BaseType::Interface& interface_type =
						this->context.getTypeManager().getInterface(parent_id.interfaceID);

					return std::format(
						"{}{}.impl_t{}.",
						this->get_parent_name<PIR_STMT_NAME_SAFE>(interface_type.parent, interface_type.sourceID),
						interface_type.getName(this->context.getSourceManager()),
						parent_id.targetTypeID.get()
					);

				}else{
					static_assert(false, "Unknown encapsulating symbol");
				}
			});

		}else{
			if(source_id.is<Source::ID>()){
				const Source& parent_source = context.getSourceManager()[source_id.as<Source::ID>()];
				const Source::Package& parent_package =
					context.getSourceManager().getPackage(parent_source.getPackageID());

				if constexpr(PIR_STMT_NAME_SAFE){
					return std::format("{}.", parent_package.name);
					
				}else{
					return std::format("{}::", parent_package.name);
				}

			}else if(source_id.is<BuiltinModule::ID>()){
				if constexpr(PIR_STMT_NAME_SAFE){
					switch(source_id.as<BuiltinModule::ID>()){
						break; case BuiltinModule::ID::PTHR:  return "pthr.";
						break; case BuiltinModule::ID::BUILD: return "build.";
					}
				}else{
					switch(source_id.as<BuiltinModule::ID>()){
						break; case BuiltinModule::ID::PTHR:  return "@pthr.";
						break; case BuiltinModule::ID::BUILD: return "@build.";
					}
				}

				evo::debugFatalBreak("Unknown builtin module");

			}else{
				evo::debugFatalBreak("clang parent shouldn't be possible");
			}
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


	auto SemaToPIR::add_auto_delete_target(pir::Expr expr, TypeInfo::ID type_id) -> void {
		this->get_current_scope_level().defers.emplace_back(
			AutoDeleteTarget(expr, type_id),
			DeferItem::Targets{
				.on_scope_end = true,
				.on_return    = true,
				.on_error     = true,
				.on_continue  = true,
				.on_break     = true,
			}
		);
	}



	template<SemaToPIR::DeferTarget TARGET>
	auto SemaToPIR::output_defers_for_scope_level(const ScopeLevel& scope_level) -> void {
		for(const DeferItem& defer_item : scope_level.defers | std::views::reverse){
			if constexpr(TARGET == DeferTarget::SCOPE_END){
				if(defer_item.targets.on_scope_end == false){ continue; }

			}else if constexpr(TARGET == DeferTarget::RETURN){
				if(defer_item.targets.on_return == false){ continue; }

			}else if constexpr(TARGET == DeferTarget::ERROR){
				if(defer_item.targets.on_error == false){ continue; }
				
			}else if constexpr(TARGET == DeferTarget::CONTINUE){
				if(defer_item.targets.on_continue == false){ continue; }
				
			}else if constexpr(TARGET == DeferTarget::BREAK){
				if(defer_item.targets.on_break == false){ continue; }
				
			}else{
				static_assert(false, "Unknown defer target");
			}


			defer_item.defer_item.visit([&](const auto& item) -> void {
				using ItemType = std::decay_t<decltype(item)>;

				if constexpr(std::is_same<ItemType, sema::Defer::ID>()){
					const sema::Defer& sema_defer = this->context.getSemaBuffer().getDefer(item);

					for(const sema::Stmt& stmt : sema_defer.block){
						this->lower_stmt(stmt);
					}

				}else if constexpr(std::is_same<ItemType, std::function<void()>>()){
					item();

				}else if constexpr(std::is_same<ItemType, AutoDeleteTarget>()){
					this->delete_expr(item.expr, item.typeID);

				}else if constexpr(std::is_same<ItemType, AutoDeleteManagedLifetimeTarget>()){
					for(const ScopeLevel& scope_level : this->scope_levels | std::views::reverse){
						const auto find = scope_level.value_states.find(item.expr);
						if(find == scope_level.value_states.end()){ continue; }

						if(find->second){
							item.expr.visit([&](const auto& expr) -> void {
								using ExprType = std::decay_t<decltype(expr)>;

								if constexpr(std::is_same<ExprType, pir::Expr>()){
									this->delete_expr(expr, item.typeID);

								}else if constexpr(std::is_same<ExprType, ManagedLifetimeErrorParam>()){
									if(this->context.getTypeManager().isTriviallyDeletable(item.typeID)){ return; }

									const pir::Expr pir_expr = this->handler.createCalcPtr(
										this->handler.createLoad(
											this->param_allocas.back(), this->module.createPtrType()
										),
										*this->current_func_info->error_return_type,
										evo::SmallVector<pir::CalcPtr::Index>{0, expr.index},
										this->name(".ERR_PARAM.{}", expr.index)
									);

									this->delete_expr(pir_expr, item.typeID);

								}else if constexpr(std::is_same<ExprType, OpDeleteThisAccessor>()){
									if(this->context.getTypeManager().isTriviallyDeletable(item.typeID)){ return; }
									
									const pir::Expr pir_expr = this->handler.createCalcPtr(
										this->handler.createParamExpr(0),
										*this->current_func_info->params[0].reference_type,
										evo::SmallVector<pir::CalcPtr::Index>{0, expr.abiIndex},
										this->name(".ERR_PARAM.{}", expr.abiIndex)
									);

									this->delete_expr(pir_expr, item.typeID);

								}else{
									static_assert(false, "unknown managed lifetime target");
								}
							});
						}

						break;
					}

				}else{
					static_assert(false, "Unknown defer item");
				}
			});
		}
	}



	auto SemaToPIR::get_atomic_ordering(const core::GenericValue& generic_value) -> pir::AtomicOrdering {
		const uint32_t atomic_ordering_number = static_cast<uint32_t>(generic_value.getInt(32));

		evo::debugAssert(
			atomic_ordering_number + 1 <= evo::to_underlying(pir::AtomicOrdering::SEQUENTIALLY_CONSISTENT)
			&& atomic_ordering_number + 1 != 0,
			"Invalid ordering"
		);

		return std::bit_cast<pir::AtomicOrdering>(atomic_ordering_number + 1);
	}


	auto SemaToPIR::create_scoped_source_location(uint32_t line, uint32_t collumn)
	-> std::optional<pir::InstrHandler::DeferPopSourceLocation> {
		if(this->data.config.includeDebugInfo == false){ return std::nullopt; }

		return this->handler.scopedSourceLocation(
			pir::meta::SourceLocation(this->get_current_meta_local_scope(), line, collumn)
		);
	}

	auto SemaToPIR::create_scoped_source_location(pir::meta::LocalScope scope, uint32_t line, uint32_t collumn)
	-> std::optional<pir::InstrHandler::DeferPopSourceLocation> {
		if(this->data.config.includeDebugInfo == false){ return std::nullopt; }

		return this->handler.scopedSourceLocation(pir::meta::SourceLocation(scope, line, collumn));
	}

	auto SemaToPIR::create_scoped_source_location(auto& thing_to_get_location_of)
	-> std::optional<pir::InstrHandler::DeferPopSourceLocation> {
		if(this->data.config.includeDebugInfo == false){ return std::nullopt; }

		const Location location =
			this->get_location(Diagnostic::Location::get(thing_to_get_location_of, this->context));

		return this->handler.scopedSourceLocation(
			pir::meta::SourceLocation(
				this->get_current_meta_local_scope(), location.line_number, location.collumn_number
			)
		);
	}


}