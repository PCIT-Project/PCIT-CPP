//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include "./class_impls/native_ptr_decls.h"
#include "./class_impls/types.h"
#include "./class_impls/enums.h"
#include "./Function.h"

namespace pcit::llvmint{

	
	class Module{
		public:
			// thanks to https://doc.rust-lang.org/rustc/codegen-options/index.html for this info
			enum class Relocation{
				Default,
				Static, // absolute addressing mode
				PIC,    // position independent code (-fPIC)

				// Only use on Darwin
				DynamicNoPIC, // relocatable external references, non-relocatable code

				// Only use for certain embedded ARM targets
				ROPI, // relocatable code and read-only data
				RWPI, // relocatable read-write data
				ROPI_RWPI, // combination of the above 2
			};

			enum class CodeSize{
				Default,
				Tiny,
				Small,
				Kernel,
				Medium,
				Large,
			};

			enum class OptLevel{
				None,       // O0
				Less,       // O1
				Default,    // O2 / Os
				Aggressive, // O3
			};

		public:
			Module(std::string_view name, class LLVMContext& context);
			~Module();

			EVO_NODISCARD auto getDefaultTargetTriple() -> std::string;
			auto setTargetTriple(const std::string& target_triple) -> void;
			EVO_NODISCARD auto setDataLayout(
				std::string_view target_triple,
				Relocation relocation = Relocation::Default,
				CodeSize code_size    = CodeSize::Default,
				OptLevel opt_level    = OptLevel::Default,
				bool is_jit           = false
			) -> std::string; // returns error message (empty if no error)

			EVO_NODISCARD auto print() const -> std::string;


			EVO_NODISCARD auto createFunction(evo::CStrProxy name, const FunctionType& prototype, LinkageType linkage) 
				-> Function;
			

			EVO_NODISCARD auto getNative() const -> const llvm::Module* { return this->native; }
			EVO_NODISCARD auto getNative()       ->       llvm::Module* { return this->native; }
	
		private:
			llvm::Module* native;
			llvm::TargetMachine* target_machine = nullptr;
	};

	
}