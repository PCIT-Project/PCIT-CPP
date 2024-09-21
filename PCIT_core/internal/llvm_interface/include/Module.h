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
#include "./ExecutionEngine.h"

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
				// Tiny, // seems to cause a crash within LLVM
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



			EVO_NODISCARD auto print() const -> std::string;


			template<typename ReturnType>
			EVO_NODISCARD auto run(std::string_view func_name) -> ReturnType {
				auto execution_engine = ExecutionEngine();
				execution_engine.createEngine(*this);
				this->setup_linked_funcs(execution_engine);


				const ReturnType output = execution_engine.runFunction<ReturnType>(func_name);

				execution_engine.shutdownEngine();

				return output;
			};

			template<>
			auto run<void>(std::string_view func_name) -> void {
				auto execution_engine = ExecutionEngine();
				execution_engine.createEngine(*this);
				this->setup_linked_funcs(execution_engine);

				execution_engine.runFunction<void>(func_name);

				execution_engine.shutdownEngine();
			};
			

			EVO_NODISCARD auto native() const -> const llvm::Module* { return this->_native; }
			EVO_NODISCARD auto native()       ->       llvm::Module* { return this->_native; }


		private:
			EVO_NODISCARD auto get_clone() const -> std::unique_ptr<llvm::Module>;

			auto setup_linked_funcs(ExecutionEngine& execution_engine) -> void;
	

		private:
			llvm::Module* _native;
			llvm::TargetMachine* target_machine = nullptr;

			friend class ExecutionEngine;
	};

	
}