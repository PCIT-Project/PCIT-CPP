////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SemaToPIRData.hpp"


#include "../../include/Context.hpp"
#include "../../include/source/SourceManager.hpp"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{


	auto SemaToPIRData::getInterfacePtrType(pir::Module& module, SourceManager& source_manager) -> const PIRType& {
		const auto lock = std::scoped_lock(this->interface_ptr_type_lock);

		if(this->interface_ptr_type.has_value() == false){
			const pir::Type struct_type = module.createStructType(
				"PTHR.interface_ptr",
				evo::SmallVector<pir::Type>{module.createPtrType(), module.createPtrType()},
				false
			);

			auto meta_type_id = std::optional<pir::meta::StructType::ID>();
			if(this->getConfig().includeDebugInfo){
				// pick any panther source, doesn't really matter since array references don't exactly have a decl site
				const Source& first_source = source_manager[Source::ID(0)];

				const pir::meta::QualifiedType::ID rawptr_meta_type = this->get_or_create_meta_qualified_type(
					TypeManager::getTypeRawPtr(),
					module,
					"RawPtr",
					std::nullopt,
					pir::meta::QualifiedType::Qualifier::MUT_POINTER
				);

				meta_type_id = module.createMetaStructType(
					struct_type,
					"PTHR.interface_ptr",
					"PTHR.interface_ptr",
					*first_source.getPIRMetaFileID(),
					*first_source.getPIRMetaFileID(),
					0,
					evo::SmallVector<pir::meta::StructType::Member>{
						pir::meta::StructType::Member(rawptr_meta_type, "data"),
						pir::meta::StructType::Member(rawptr_meta_type, "methods"),
					}
				);
			}

			this->interface_ptr_type.emplace(struct_type, meta_type_id);
		}

		return *this->interface_ptr_type;
	}


	auto SemaToPIRData::getArrayRefType(
		pir::Module& module,
		Context& context,
		BaseType::ArrayRef::ID array_ref_id,
		const std::function<pir::meta::Type(TypeInfo::ID)>& get_data_ptr_meta_type
	) -> const PIRType& {
		auto value_handler = this->array_ref_type_infos.get(array_ref_id);

		if(value_handler.needsToBeSet() == false){
			return value_handler.getValue();
		}

		const BaseType::ArrayRef& array_ref_type = context.getTypeManager().getArrayRef(array_ref_id); 

		const size_t num_ref_ptrs = array_ref_type.getNumRefPtrs();

		auto member_types = evo::SmallVector<pir::Type>();
		member_types.reserve(num_ref_ptrs + 1);

		member_types.emplace_back(module.createPtrType());
		const pir::Type usize_type = module.createUnsignedType(uint32_t(module.sizeOfPtr() * 8));
		for(size_t i = 0; i < num_ref_ptrs; i+=1){
			member_types.emplace_back(usize_type);
		}

		const pir::Type struct_type = module.createStructType(
			std::format("PTHR.array_ref_{}", array_ref_id.get()), std::move(member_types), false
		);

		std::string meta_name = context.getTypeManager().printType(BaseType::ID(array_ref_id), context);

		auto meta_type_id = std::optional<pir::meta::StructType::ID>();
		if(this->getConfig().includeDebugInfo){
			auto meta_members = evo::SmallVector<pir::meta::StructType::Member>();
			meta_members.reserve(num_ref_ptrs + 1);

			const TypeInfo& element_type_info = context.getTypeManager().getTypeInfo(array_ref_type.elementTypeID);
			const TypeInfo::ID data_ptr_type_id = context.getTypeManager().getOrCreateTypeInfo(
				element_type_info.copyWithPushedQualifier(TypeInfo::Qualifier(true, array_ref_type.isMut, false, false))
			);

			meta_members.emplace_back(get_data_ptr_meta_type(data_ptr_type_id), "data");

			const pir::meta::BasicType::ID usize_meta_id =
				this->get_or_create_meta_basic_type(TypeManager::getTypeUSize(), module, "USize", usize_type);

			for(size_t i = 0; const BaseType::ArrayRef::Dimension& dimension : array_ref_type.dimensions){
				EVO_DEFER([&](){ i += 1; });
				
				if(dimension.isLength()){ continue; }
				
				meta_members.emplace_back(usize_meta_id, std::format("size{}", i));
			}

			// pick any panther source, doesn't really matter since array references don't exactly have a decl site
			const Source& first_source = context.getSourceManager()[Source::ID(0)];

			meta_type_id = module.createMetaStructType(
				struct_type,
				evo::copy(meta_name),
				std::move(meta_name),
				*first_source.getPIRMetaFileID(),
				*first_source.getPIRMetaFileID(),
				0,
				std::move(meta_members)
			);
		}

		return value_handler.emplaceValue(struct_type, meta_type_id);
	}

	auto SemaToPIRData::getArrayRefType(
		pir::Module& module,
		Context& context,
		TypeInfo::ID array_ref_id,
		const std::function<pir::meta::Type(TypeInfo::ID)>& get_data_ptr_meta_type
	) -> const PIRType& {
		return this->getArrayRefType(
			module,
			context,
			context.getTypeManager().getTypeInfo(array_ref_id).baseTypeID().arrayRefID(),
			get_data_ptr_meta_type
		);
	}






	auto SemaToPIRData::lookupGlobalVar(pir::GlobalVar::ID id) const -> std::optional<sema::GlobalVar::ID> {
		const auto lock = std::scoped_lock(this->global_vars_lock);

		const auto find = this->reverse_global_vars.find(id);
		if(find != this->reverse_global_vars.end()){ return find->second; }
		return std::nullopt;
	}

	auto SemaToPIRData::lookupGlobalString(pir::GlobalVar::ID id) const -> std::optional<sema::StringValue::ID> {
		const auto lock = std::scoped_lock(this->global_strings_lock);

		const auto find = this->reverse_global_strings.find(id);
		if(find != this->reverse_global_strings.end()){ return find->second; }
		return std::nullopt;
	}

	auto SemaToPIRData::lookupVTable(pir::GlobalVar::ID id) const -> std::optional<VTableID> {
		const auto lock = std::scoped_lock(this->vtables_lock);

		const auto find = this->reverse_vtables.find(id);
		if(find != this->reverse_vtables.end()){ return find->second; }
		return std::nullopt;
	}

	auto SemaToPIRData::lookupSingleMethodVTable(pir::Function::ID id) const -> std::optional<VTableID> {
		const auto lock = std::scoped_lock(this->single_method_vtables_lock);

		const auto find = this->reverse_single_method_vtables.find(id);
		if(find != this->reverse_single_method_vtables.end()){ return find->second; }
		return std::nullopt;
	}





	auto SemaToPIRData::createJITBuildFuncDecls(pir::Module& module) -> void {
		const auto create_func_decl = [&](
			std::string_view name,
			evo::SmallVector<pir::Parameter>&& params,
			pir::Type ret_type
		){
			return module.createExternalFunction(
				std::string(name),
				std::move(params),
				pir::CallingConvention::C,
				pir::Linkage::EXTERNAL,
				ret_type
			);
		};



		this->jit_build_funcs.build_set_num_threads = create_func_decl(
			"PTHR.BUILD.buildSetNumThreads",
			{
				pir::Parameter("context", module.createUnsignedType(sizeof(size_t) * 8)),
				pir::Parameter("num_threads", module.createUnsignedType(32))
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_set_output = create_func_decl(
			"PTHR.BUILD.buildSetOutput",
			{
				pir::Parameter("context", module.createUnsignedType(sizeof(size_t) * 8)),
				pir::Parameter("output", module.createUnsignedType(32))
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_set_add_debug_info = create_func_decl(
			"PTHR.BUILD.buildSetAddDebugInfo",
			{
				pir::Parameter("context", module.createUnsignedType(sizeof(size_t) * 8)),
				pir::Parameter("add_debug_info", module.createBoolType())
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_set_std_lib_package = create_func_decl(
			"PTHR.BUILD.buildSetStdLibPackage",
			{
				pir::Parameter("context", module.createUnsignedType(sizeof(size_t) * 8)),
				pir::Parameter("use_std_lib", module.createUnsignedType(32))
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_create_package = create_func_decl(
			"PTHR.BUILD.buildCreatePackage",
			{
				pir::Parameter("context", module.createUnsignedType(sizeof(size_t) * 8)),
				pir::Parameter("path", module.createPtrType()),
				pir::Parameter("name", module.createPtrType()),
				pir::Parameter("warns_settings", module.createPtrType())
			},
			module.createUnsignedType(32)
		);

		this->jit_build_funcs.build_add_source_file = create_func_decl(
			"PTHR.BUILD.buildAddSourceFile",
			{
				pir::Parameter("context", module.createUnsignedType(sizeof(size_t) * 8)),
				pir::Parameter("file_path", module.createPtrType()),
				pir::Parameter("package_id", module.createUnsignedType(32))
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_add_source_directory = create_func_decl(
			"PTHR.BUILD.buildAddSourceDirectory",
			{
				pir::Parameter("context", module.createUnsignedType(sizeof(size_t) * 8)),
				pir::Parameter("file_path", module.createPtrType()),
				pir::Parameter("package_id", module.createUnsignedType(32)),
				pir::Parameter("is_recursive", module.createBoolType()),
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_add_c_header_file = create_func_decl(
			"PTHR.BUILD.buildAddCHeaderFile",
			{
				pir::Parameter("context", module.createUnsignedType(sizeof(size_t) * 8)),
				pir::Parameter("file_path", module.createPtrType()),
				pir::Parameter("add_includes_to_pub_api", module.createBoolType())
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_add_cpp_header_file = create_func_decl(
			"PTHR.BUILD.buildAddCPPHeaderFile",
			{
				pir::Parameter("context", module.createUnsignedType(sizeof(size_t) * 8)),
				pir::Parameter("file_path", module.createPtrType()),
				pir::Parameter("add_includes_to_pub_api", module.createBoolType())
			},
			module.createVoidType()
		);
	}


}