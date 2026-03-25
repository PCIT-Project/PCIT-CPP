////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include "./class_impls/native_ptr_decls.h"


namespace pcit::llvmint{

	
	class DIBuilder{
		public:
			struct File { llvm::DIFile* file; };


			struct Language{
				enum class Name : unsigned {
					PANTHER = 0x000c, // using the C99 code for now
					C       = 0x000c, // C99
					CPP     = 0x0021, // C++ 14
				};

				using enum class Name;
				
				Language(Name name) : dwarfCode(evo::to_underlying(name)) {}
				Language(unsigned dwarf_code) : dwarfCode(dwarf_code) {}

				unsigned dwarfCode;
			};


		public:
			DIBuilder(class Module& _module);
			~DIBuilder();

			auto addModuleLevelDebugInfo() -> void;


			auto createCompileUnit(Language language, File file, std::string_view producer, bool is_optimized) -> void;

			EVO_NODISCARD auto createFile(std::string_view file_name, std::string_view directory) -> File;

	
		private:
			class Module& module;
			llvm::DIBuilder* builder = nullptr;
	};

	
}