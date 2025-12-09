////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SemaToPIRData.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{


	auto SemaToPIRData::getInterfacePtrType(pir::Module& module) -> pir::Type {
		const auto lock = std::scoped_lock(this->interface_ptr_type_lock);

		if(this->interface_ptr_type.has_value() == false){
			this->interface_ptr_type = module.createStructType(
				"PTHR.interface_ptr",
				evo::SmallVector<pir::Type>{module.createPtrType(), module.createPtrType()},
				false
			);
		}

		return *this->interface_ptr_type;
	}


	// TODO(PERF): 
	auto SemaToPIRData::getArrayRefType(pir::Module& module, unsigned num_dimensions) -> pir::Type {
		evo::debugAssert(num_dimensions >= 1, "Must have at least 1 dimension");

		const auto lock = std::scoped_lock(this->array_ref_type_lock);

		if(
			this->array_ref_type.size() < num_dimensions
			|| this->array_ref_type[num_dimensions - 1].has_value() == false
		){
			if(this->array_ref_type.size() < num_dimensions){
				this->array_ref_type.resize(num_dimensions);
			}

			auto member_types = evo::SmallVector<pir::Type>();
			member_types.reserve(num_dimensions + 1);

			member_types.emplace_back(module.createPtrType());
			const pir::Type usize_type = module.createIntegerType(uint32_t(module.sizeOfPtr() * 8));
			for(size_t i = 0; i < num_dimensions; i+=1){
				member_types.emplace_back(usize_type);
			}

			this->array_ref_type[num_dimensions - 1] = module.createStructType(
				std::format("PTHR.array_ref.d{}", num_dimensions),
				std::move(member_types),
				false
			);
		}

		return *this->array_ref_type[num_dimensions - 1];
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
				pir::Parameter("context", module.createIntegerType(sizeof(size_t) * 8)),
				pir::Parameter("num_threads", module.createIntegerType(32))
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_set_output = create_func_decl(
			"PTHR.BUILD.buildSetOutput",
			{
				pir::Parameter("context", module.createIntegerType(sizeof(size_t) * 8)),
				pir::Parameter("output", module.createIntegerType(32))
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_set_std_lib_package = create_func_decl(
			"PTHR.BUILD.buildSetStdLibPackage",
			{
				pir::Parameter("context", module.createIntegerType(sizeof(size_t) * 8)),
				pir::Parameter("use_std_lib", module.createIntegerType(32))
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_create_package = create_func_decl(
			"PTHR.BUILD.buildCreatePackage",
			{
				pir::Parameter("context", module.createIntegerType(sizeof(size_t) * 8)),
				pir::Parameter("path", module.createPtrType()),
				pir::Parameter("name", module.createPtrType()),
				pir::Parameter("warns_settings", module.createPtrType())
			},
			module.createIntegerType(32)
		);

		this->jit_build_funcs.build_add_source_file = create_func_decl(
			"PTHR.BUILD.buildAddSourceFile",
			{
				pir::Parameter("context", module.createIntegerType(sizeof(size_t) * 8)),
				pir::Parameter("file_path", module.createPtrType()),
				pir::Parameter("package_id", module.createIntegerType(32))
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_add_source_directory = create_func_decl(
			"PTHR.BUILD.buildAddSourceDirectory",
			{
				pir::Parameter("context", module.createIntegerType(sizeof(size_t) * 8)),
				pir::Parameter("file_path", module.createPtrType()),
				pir::Parameter("package_id", module.createIntegerType(32)),
				pir::Parameter("is_recursive", module.createBoolType()),
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_add_c_header_file = create_func_decl(
			"PTHR.BUILD.buildAddCHeaderFile",
			{
				pir::Parameter("context", module.createIntegerType(sizeof(size_t) * 8)),
				pir::Parameter("file_path", module.createPtrType()),
				pir::Parameter("add_includes_to_pub_api", module.createBoolType())
			},
			module.createVoidType()
		);

		this->jit_build_funcs.build_add_cpp_header_file = create_func_decl(
			"PTHR.BUILD.buildAddCPPHeaderFile",
			{
				pir::Parameter("context", module.createIntegerType(sizeof(size_t) * 8)),
				pir::Parameter("file_path", module.createPtrType()),
				pir::Parameter("add_includes_to_pub_api", module.createBoolType())
			},
			module.createVoidType()
		);
	}


}