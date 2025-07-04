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



	auto SemaToPIRData::createJITInterfaceFuncDecls(pir::Module& module) -> void {
		const auto create_return_func_decl = [&](std::string_view name, evo::SmallVector<pir::Parameter>&& params){
			return module.createExternalFunction(
				std::string(name),
				std::move(params),
				pir::CallingConvention::C,
				pir::Linkage::EXTERNAL,
				module.createVoidType()
			);
		};


		this->jit_interface_funcs.return_generic_int = create_return_func_decl(
			"PIR.JIT.return_generic_int",
			{
				pir::Parameter("target", module.createPtrType()),
				pir::Parameter("data", module.createPtrType()),
				pir::Parameter("size", module.createIntegerType(32)),
			}
		);

		this->jit_interface_funcs.return_generic_bool = create_return_func_decl(
			"PIR.JIT.return_generic_bool",
			{pir::Parameter("target", module.createPtrType()), pir::Parameter("value", module.createBoolType())}
		);

		this->jit_interface_funcs.return_generic_f16 = create_return_func_decl(
			"PIR.JIT.return_generic_f16",
			{pir::Parameter("target", module.createPtrType()), pir::Parameter("value", module.createPtrType())}
		);
		this->jit_interface_funcs.return_generic_bf16 = create_return_func_decl(
			"PIR.JIT.return_generic_bf16",
			{pir::Parameter("target", module.createPtrType()), pir::Parameter("value", module.createPtrType())}
		);
		this->jit_interface_funcs.return_generic_f32 = create_return_func_decl(
			"PIR.JIT.return_generic_f32",
			{pir::Parameter("target", module.createPtrType()), pir::Parameter("value", module.createFloatType(32))}
		);
		this->jit_interface_funcs.return_generic_f64 = create_return_func_decl(
			"PIR.JIT.return_generic_f64",
			{pir::Parameter("target", module.createPtrType()), pir::Parameter("value", module.createFloatType(64))}
		);
		this->jit_interface_funcs.return_generic_f80 = create_return_func_decl(
			"PIR.JIT.return_generic_f80",
			{pir::Parameter("target", module.createPtrType()), pir::Parameter("value", module.createPtrType())}
		);
		this->jit_interface_funcs.return_generic_f128 = create_return_func_decl(
			"PIR.JIT.return_generic_f128",
			{pir::Parameter("target", module.createPtrType()), pir::Parameter("value", module.createPtrType())}
		);

		this->jit_interface_funcs.return_generic_char = create_return_func_decl(
			"PIR.JIT.return_generic_char",
			{pir::Parameter("target", module.createPtrType()), pir::Parameter("value", module.createIntegerType(8))}
		);




		this->jit_interface_funcs.prepare_return_generic_aggregate = create_return_func_decl(
			"PIR.JIT.prepare_return_generic_aggregate",
			{
				pir::Parameter("target", module.createPtrType()),
				pir::Parameter("num_elems", module.createIntegerType(32)),
				pir::Parameter("target_index_arr", module.createPtrType()),
				pir::Parameter("target_num_indices", module.createIntegerType(32)),
			}
		);

		this->jit_interface_funcs.return_generic_aggregate_int = create_return_func_decl(
			"PIR.JIT.return_generic_aggregate_int",
			{
				pir::Parameter("target", module.createPtrType()),
				pir::Parameter("data", module.createPtrType()),
				pir::Parameter("size", module.createIntegerType(32)),
				pir::Parameter("target_index_arr", module.createPtrType()),
				pir::Parameter("target_num_indices", module.createIntegerType(32)),
			}
		);

		this->jit_interface_funcs.return_generic_aggregate_bool = create_return_func_decl(
			"PIR.JIT.return_generic_aggregate_bool",
			{
				pir::Parameter("target", module.createPtrType()),
				pir::Parameter("value", module.createBoolType()),
				pir::Parameter("target_index_arr", module.createPtrType()),
				pir::Parameter("target_num_indices", module.createIntegerType(32)),
			}
		);

		this->jit_interface_funcs.return_generic_aggregate_f16 = create_return_func_decl(
			"PIR.JIT.return_generic_aggregate_f16",
			{
				pir::Parameter("target", module.createPtrType()),
				pir::Parameter("value", module.createPtrType()),
				pir::Parameter("target_index_arr", module.createPtrType()),
				pir::Parameter("target_num_indices", module.createIntegerType(32)),
			}
		);
		this->jit_interface_funcs.return_generic_aggregate_bf16 = create_return_func_decl(
			"PIR.JIT.return_generic_aggregate_bf16",
			{
				pir::Parameter("target", module.createPtrType()),
				pir::Parameter("value", module.createPtrType()),
				pir::Parameter("target_index_arr", module.createPtrType()),
				pir::Parameter("target_num_indices", module.createIntegerType(32)),
			}
		);
		this->jit_interface_funcs.return_generic_aggregate_f32 = create_return_func_decl(
			"PIR.JIT.return_generic_aggregate_f32",
			{
				pir::Parameter("target", module.createPtrType()),
				pir::Parameter("value", module.createFloatType(32)),
				pir::Parameter("target_index_arr", module.createPtrType()),
				pir::Parameter("target_num_indices", module.createIntegerType(32)),
			}
		);
		this->jit_interface_funcs.return_generic_aggregate_f64 = create_return_func_decl(
			"PIR.JIT.return_generic_aggregate_f64",
			{
				pir::Parameter("target", module.createPtrType()),
				pir::Parameter("value", module.createFloatType(64)),
				pir::Parameter("target_index_arr", module.createPtrType()),
				pir::Parameter("target_num_indices", module.createIntegerType(32)),
			}
		);
		this->jit_interface_funcs.return_generic_aggregate_f80 = create_return_func_decl(
			"PIR.JIT.return_generic_aggregate_f80",
			{
				pir::Parameter("target", module.createPtrType()),
				pir::Parameter("value", module.createPtrType()),
				pir::Parameter("target_index_arr", module.createPtrType()),
				pir::Parameter("target_num_indices", module.createIntegerType(32)),
			}
		);
		this->jit_interface_funcs.return_generic_aggregate_f128 = create_return_func_decl(
			"PIR.JIT.return_generic_aggregate_f128",
			{
				pir::Parameter("target", module.createPtrType()),
				pir::Parameter("value", module.createPtrType()),
				pir::Parameter("target_index_arr", module.createPtrType()),
				pir::Parameter("target_num_indices", module.createIntegerType(32)),
			}
		);





		this->jit_interface_funcs.get_generic_int = module.createExternalFunction(
			"PIR.JIT.get_generic_int",
			{
				pir::Parameter("source", module.createPtrType()),
				pir::Parameter("value", module.createPtrType()),
				pir::Parameter("num_bytes", module.createIntegerType(32)),
			},
			pir::CallingConvention::C,
			pir::Linkage::EXTERNAL,
			module.createVoidType()
		);

		this->jit_interface_funcs.get_generic_bool = module.createExternalFunction(
			"PIR.JIT.get_generic_bool",
			{pir::Parameter("source", module.createPtrType())},
			pir::CallingConvention::C,
			pir::Linkage::EXTERNAL,
			module.createBoolType()
		);

		this->jit_interface_funcs.get_generic_float = module.createExternalFunction(
			"PIR.JIT.get_generic_float",
			{pir::Parameter("source", module.createPtrType()), pir::Parameter("value", module.createPtrType())},
			pir::CallingConvention::C,
			pir::Linkage::EXTERNAL,
			module.createVoidType()
		);
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