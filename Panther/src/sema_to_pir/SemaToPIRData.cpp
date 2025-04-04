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


	auto SemaToPIRData::createNeededJITInterfaceFuncDecls(pir::Module& module) -> void {
		const auto create_func_decl = [&](std::string_view name, evo::SmallVector<pir::Parameter>&& params){
			return module.createExternalFunction(
				std::string(name),
				std::move(params),
				pir::CallingConvention::C,
				pir::Linkage::EXTERNAL,
				module.createVoidType()
			);
		};


		this->jit_interface_funcs.return_generic_int = create_func_decl(
			"PTHR.JIT.return_generic_int",
			{
				pir::Parameter{"target", module.createPtrType()},
				pir::Parameter{"data", module.createPtrType()},
				pir::Parameter{"size", module.createIntegerType(64)},
			}
		);

		this->jit_interface_funcs.return_generic_bool = create_func_decl(
			"PTHR.JIT.return_generic_bool",
			{pir::Parameter{"target", module.createPtrType()}, pir::Parameter{"value", module.createBoolType()}}
		);

		this->jit_interface_funcs.return_generic_f16 = create_func_decl(
			"PTHR.JIT.return_generic_f16",
			{pir::Parameter{"target", module.createPtrType()}, pir::Parameter{"value", module.createPtrType()}}
		);
		this->jit_interface_funcs.return_generic_bf16 = create_func_decl(
			"PTHR.JIT.return_generic_bf16",
			{pir::Parameter{"target", module.createPtrType()}, pir::Parameter{"value", module.createPtrType()}}
		);
		this->jit_interface_funcs.return_generic_f32 = create_func_decl(
			"PTHR.JIT.return_generic_f32",
			{pir::Parameter{"target", module.createPtrType()}, pir::Parameter{"value", module.createFloatType(32)}}
		);
		this->jit_interface_funcs.return_generic_f64 = create_func_decl(
			"PTHR.JIT.return_generic_f64",
			{pir::Parameter{"target", module.createPtrType()}, pir::Parameter{"value", module.createFloatType(64)}}
		);
		this->jit_interface_funcs.return_generic_f80 = create_func_decl(
			"PTHR.JIT.return_generic_f80",
			{pir::Parameter{"target", module.createPtrType()}, pir::Parameter{"value", module.createPtrType()}}
		);
		this->jit_interface_funcs.return_generic_f128 = create_func_decl(
			"PTHR.JIT.return_generic_f128",
			{pir::Parameter{"target", module.createPtrType()}, pir::Parameter{"value", module.createPtrType()}}
		);

		this->jit_interface_funcs.return_generic_char = create_func_decl(
			"PTHR.JIT.return_generic_char",
			{pir::Parameter{"target", module.createPtrType()}, pir::Parameter{"value", module.createIntegerType(8)}}
		);
	}


}