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
		const auto lock = std::scoped_lock(this->vtables_lock);

		if(this->interface_ptr_type.has_value() == false){
			this->interface_ptr_type = module.createStructType(
				"PTHR.interface_ptr",
				evo::SmallVector<pir::Type>{module.createPtrType(), module.createPtrType()},
				false
			);
		}

		return *this->interface_ptr_type;
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