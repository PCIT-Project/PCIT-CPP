////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.hpp>


#include "./class_impls/native_ptr_decls.hpp"
#include "./class_impls/types.hpp"
#include "./class_impls/enums.hpp"
#include "./Function.hpp"

#include "../../../include/Target.hpp"

namespace pcit::llvmint{

	
	class Module{
		public:
			// thanks to https://doc.rust-lang.org/rustc/codegen-options/index.html for this info
			// used for target triple
			enum class Relocation{
				DEFAULT,
				STATIC, // absolute addressing mode
				PIC,    // position independent code (-fPIC)

				// Only use on Darwin
				DYNAMIC_NO_PIC, // relocatable external references, non-relocatable code

				// Only use for certain embedded ARM targets
				ROPI, // relocatable code and read-only data
				RWPI, // relocatable read-write data
				ROPI_RWPI, // combination of the above 2
			};

			// used for target triple
			enum class CodeSize{
				DEFAULT,
				// Tiny, // seems to cause a crash within LLVM // TODO(FUTURE): check if this is still the case?
				SMALL,
				KERNEL,
				MEDIUM,
				LARGE,
			};

			// used for target triple
			enum class OptLevel{
				NONE,       // O0
				LESS,       // O1
				DEFAULT,    // O2 / Os
				AGGRESSIVE, // O3
			};

			// used for optimization pass
			enum class OptMode{
				NONE,
				O0 = NONE,
				O1,
				O2,
				O3,
				Os,
				Oz,
			};



			struct ArchSpecificSettingsDefault{};

			struct ArchSpecificSettingsX86{
				enum class AssemblyDialect{
					INTEL,
					ATT,
				};

				AssemblyDialect dialect = AssemblyDialect::INTEL;
			};

			using ArchSpecificSettings = evo::Variant<ArchSpecificSettingsDefault, ArchSpecificSettingsX86>;

		public:
			Module() = default;

			#if defined(PCIT_CONFIG_DEBUG)
				~Module(){
					evo::debugAssert(
						this->isInitialized() == false, "`Module::deinit()` must be called before destructor`"
					);
				}
			#else
				~Module() = default;
			#endif


			Module(Module&& rhs){
				llvm::Module* holder = this->_native;
				this->_native = rhs._native;
				rhs._native = holder;
			}


			auto init(std::string_view name, class LLVMContext& context) -> void;
			auto deinit() -> void;


			auto steal() -> llvm::Module* {
				llvm::Module* holder = this->_native;
				this->_native = nullptr;
				return holder;
			}


			[[nodiscard]] auto isInitialized() const -> bool { return this->_native != nullptr; }


			[[nodiscard]] auto setTargetAndDataLayout(
				core::Target target   = core::Target::getNative(),
				Relocation relocation = Relocation::DEFAULT,
				CodeSize code_size    = CodeSize::DEFAULT,
				OptLevel opt_level    = OptLevel::DEFAULT,
				bool is_jit           = false,
				ArchSpecificSettings arch_specific_settings = ArchSpecificSettingsDefault()
			) -> std::string; // returns error message (empty if no error)



			[[nodiscard]] auto createFunction(std::string_view name, const FunctionType& prototype, LinkageType linkage) 
				-> Function;


			[[nodiscard]] auto createGlobal(
				const Constant& value,
				const Type& type,
				LinkageType linkage,
				bool is_constant,
				std::string_view name = ""
			) -> GlobalVariable;

			[[nodiscard]] auto createGlobalUninit(
				const Type& type, LinkageType linkage, bool is_constant, std::string_view name = ""
			) -> GlobalVariable;

			[[nodiscard]] auto createGlobalZeroinit(
				const Type& type, LinkageType linkage, bool is_constant, std::string_view name = ""
			) -> GlobalVariable;

			[[nodiscard]] auto createGlobalString(
				std::string value, LinkageType linkage, bool is_constant, std::string_view name = ""
			) -> GlobalVariable;


			// nullopt if not found
			[[nodiscard]] auto lookupFunction(std::string_view name) -> std::optional<Function>;
			[[nodiscard]] auto lookupGlobal(std::string_view name) -> std::optional<GlobalVariable>;



			[[nodiscard]] auto print() const -> std::string;
			[[nodiscard]] auto lowerToObject() -> evo::Result<std::vector<evo::byte>>;
			[[nodiscard]] auto lowerToAssembly() -> evo::Result<std::string>;

			auto optimize(OptMode opt_mode) -> void;

			// `module_to_absorb` will be deleted
			auto merge(llvm::Module* module_to_absorb) -> void;

			auto merge(Module&& module_to_absorb) -> void {
				this->merge(module_to_absorb.steal());
			}

			

			[[nodiscard]] auto native() const -> const llvm::Module* { return this->_native; }
			[[nodiscard]] auto native()       ->       llvm::Module* { return this->_native; }


		private:
			[[nodiscard]] auto get_clone() const -> std::unique_ptr<llvm::Module>;
	

		private:
			llvm::Module* _native = nullptr;
			llvm::TargetMachine* target_machine = nullptr;
	};

	
}