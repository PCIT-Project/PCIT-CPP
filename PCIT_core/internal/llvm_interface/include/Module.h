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
#include "./class_impls/types.h"
#include "./class_impls/enums.h"
#include "./Function.h"
#include "./ExecutionEngine.h"

#include "../../../include/platform.h"

namespace pcit::llvmint{

	
	class Module{
		public:
			// thanks to https://doc.rust-lang.org/rustc/codegen-options/index.html for this info
			// used for target triple
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

			// used for target triple
			enum class CodeSize{
				Default,
				// Tiny, // seems to cause a crash within LLVM
				Small,
				Kernel,
				Medium,
				Large,
			};

			// used for target triple
			enum class OptLevel{
				None,       // O0
				Less,       // O1
				Default,    // O2 / Os
				Aggressive, // O3
			};

			// used for optimization pass
			enum class OptMode{
				None,
				O0 = None,
				O1,
				O2,
				O3,
				Os,
				Oz,
			};



			struct ArchSpecificSettingsDefault{};

			struct ArchSpecificSettingsX86{
				enum class AssemblyDialect{
					Intel,
					ATT,
				};

				AssemblyDialect dialect = AssemblyDialect::Intel;
			};

			using ArchSpecificSettings = evo::Variant<ArchSpecificSettingsDefault, ArchSpecificSettingsX86>;

		public:
			Module() = default;
			~Module() = default;

			auto init(std::string_view name, class LLVMContext& context) -> void;
			auto deinit() -> void;

			EVO_NODISCARD auto isInitialized() const -> bool { return this->_native != nullptr; }





			EVO_NODISCARD auto setTargetAndDataLayout(
				core::OS os             = core::getCurrentOS(),
				core::Architecture arch = core::getCurrentArchitecture(),
				Relocation relocation   = Relocation::Default,
				CodeSize code_size      = CodeSize::Default,
				OptLevel opt_level      = OptLevel::Default,
				bool is_jit             = false,
				ArchSpecificSettings arch_specific_settings = ArchSpecificSettingsDefault()
			) -> std::string; // returns error message (empty if no error)



			EVO_NODISCARD auto createFunction(evo::CStrProxy name, const FunctionType& prototype, LinkageType linkage) 
				-> Function;


			EVO_NODISCARD auto createGlobal(
				const Constant& value,
				const Type& type,
				LinkageType linkage,
				bool is_constant,
				evo::CStrProxy name = '\0'
			) -> GlobalVariable;

			EVO_NODISCARD auto createGlobalUninit(
				const Type& type, LinkageType linkage, bool is_constant, evo::CStrProxy name = '\0'
			) -> GlobalVariable;

			EVO_NODISCARD auto createGlobalZeroinit(
				const Type& type, LinkageType linkage, bool is_constant, evo::CStrProxy name = '\0'
			) -> GlobalVariable;

			EVO_NODISCARD auto createGlobalString(
				std::string value, LinkageType linkage, bool is_constant, evo::CStrProxy name = '\0'
			) -> GlobalVariable;



			EVO_NODISCARD auto print() const -> std::string;
			EVO_NODISCARD auto lowerToObject() -> evo::Result<std::vector<evo::byte>>;
			EVO_NODISCARD auto lowerToAssembly() -> evo::Result<std::string>;

			auto optimize(OptMode opt_mode) -> void;


			template<typename ReturnType>
			EVO_NODISCARD auto run(std::string_view func_name, core::Printer& printer) -> evo::Result<ReturnType> {
				auto execution_engine = ExecutionEngine();
				execution_engine.createEngine(*this);
				execution_engine.setupLinkedFuncs(printer);

				const evo::Result<ReturnType> result = execution_engine.runFunctionDirectly<ReturnType>(func_name);
				
				execution_engine.shutdownEngine();

				return result;
			};
			

			EVO_NODISCARD auto native() const -> const llvm::Module* { return this->_native; }
			EVO_NODISCARD auto native()       ->       llvm::Module* { return this->_native; }


		private:
			EVO_NODISCARD auto get_clone() const -> std::unique_ptr<llvm::Module>;

			auto setup_linked_funcs(ExecutionEngine& execution_engine) -> void;
	

		private:
			llvm::Module* _native = nullptr;
			llvm::TargetMachine* target_machine = nullptr;

			friend class ExecutionEngine;
	};

	
}