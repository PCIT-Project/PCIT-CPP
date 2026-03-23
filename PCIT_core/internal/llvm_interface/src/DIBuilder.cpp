////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/DIBuilder.h"

#include <LLVM.h>

#include "../include/Module.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::llvmint{


	DIBuilder::DIBuilder(Module& _module) : module(_module), builder(new llvm::DIBuilder(*_module.native())) {}
	
	DIBuilder::~DIBuilder(){
		delete this->builder;
	}




	auto DIBuilder::addModuleLevelDebugInfo() -> void {
		this->module.native()->addModuleFlag(
			llvm::Module::ModFlagBehavior::Warning, "Debug Info Version", llvm::LLVMConstants::DEBUG_METADATA_VERSION
		);
	}


	auto DIBuilder::createCompileUnit(Language language, File file, std::string_view producer, bool is_optimized)
	-> void {
		this->builder->createCompileUnit(language.dwarfCode, file.file, producer, is_optimized, "", 0);
	}



	auto DIBuilder::createFile(std::string_view file_name, std::string_view directory) -> File {
		return File(this->builder->createFile(file_name, directory));
	}

		
}