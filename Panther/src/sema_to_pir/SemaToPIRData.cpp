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
				std::format("PTHR.aray_ref.d{}", num_dimensions),
				std::move(member_types),
				false
			);
		}

		return *this->array_ref_type[num_dimensions - 1];
	}





	auto SemaToPIRData::createJITBuildFuncDecls(pir::Module& module) -> void {
		const auto create_func_decl = [&](std::string_view name, evo::SmallVector<pir::Parameter>&& params){
			return module.createExternalFunction(
				std::string(name),
				std::move(params),
				pir::CallingConvention::C,
				pir::Linkage::EXTERNAL,
				module.createVoidType()
			);
		};


		this->jit_build_funcs.build_set_num_threads = create_func_decl(
			"PTHR.BUILD.build_set_num_threads",
			{
				pir::Parameter("context", module.createIntegerType(sizeof(size_t) * 8)),
				pir::Parameter("num_threads", module.createIntegerType(32))
			}
		);

		this->jit_build_funcs.build_set_output = create_func_decl(
			"PTHR.BUILD.build_set_output",
			{
				pir::Parameter("context", module.createIntegerType(sizeof(size_t) * 8)),
				pir::Parameter("output", module.createIntegerType(32))
			}
		);

		this->jit_build_funcs.build_set_use_std_lib = create_func_decl(
			"PTHR.BUILD.build_set_use_std_lib",
			{
				pir::Parameter("context", module.createIntegerType(sizeof(size_t) * 8)),
				pir::Parameter("use_std_lib", module.createBoolType())
			}
		);
	}


}