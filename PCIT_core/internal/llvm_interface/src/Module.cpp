////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Module.h"

#include <LLVM.h>

#include "../include/LLVMContext.h"

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::llvmint{

	auto Module::init(std::string_view name, LLVMContext& context) -> void {
		this->_native = new llvm::Module(llvm::StringRef(name), *context.native());
	}
	
	auto Module::deinit() -> void {
		delete this->_native;
	}


	auto Module::getDefaultTargetTriple() -> std::string {
		return llvm::sys::getDefaultTargetTriple();
	}


	auto Module::setTargetTriple(const std::string& target_triple) -> void {
		evo::debugAssert(this->isInitialized(), "not initialized");
		this->_native->setTargetTriple(target_triple);
	}

	auto Module::setDataLayout(
		std::string_view target_triple,
		Relocation relocation,
		CodeSize code_size,
		OptLevel opt_level,
		bool is_jit
	) -> std::string {
		evo::debugAssert(this->isInitialized(), "not initialized");

		auto error_msg = std::string();
		const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, error_msg);

		if(target == nullptr){ return error_msg; }

		static constexpr std::string_view cpu = "generic";
		static constexpr std::string_view features = "";
		
		// TODO: https://llvm.org/doxygen/classllvm_1_1TargetOptions.html
		auto target_options = llvm::TargetOptions();

		const std::optional<llvm::Reloc::Model> reloc_model = [&](){
			switch(relocation){
				case Relocation::Default:      return std::optional<llvm::Reloc::Model>();
				case Relocation::Static:       return std::optional<llvm::Reloc::Model>(llvm::Reloc::Static);
				case Relocation::PIC:          return std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
				case Relocation::DynamicNoPIC: return std::optional<llvm::Reloc::Model>(llvm::Reloc::DynamicNoPIC);
				case Relocation::ROPI:         return std::optional<llvm::Reloc::Model>(llvm::Reloc::ROPI);
				case Relocation::RWPI:         return std::optional<llvm::Reloc::Model>(llvm::Reloc::RWPI);
				case Relocation::ROPI_RWPI:    return std::optional<llvm::Reloc::Model>(llvm::Reloc::ROPI_RWPI);
			}
			evo::debugFatalBreak("Unknown or unsupported relocation mode");
		}();

		const std::optional<llvm::CodeModel::Model> code_model = [&](){
			switch(code_size){
				case CodeSize::Default: return std::optional<llvm::CodeModel::Model>();
				// case CodeSize::Tiny:    return std::optional<llvm::CodeModel::Model>(llvm::CodeModel::Tiny);
				case CodeSize::Small:   return std::optional<llvm::CodeModel::Model>(llvm::CodeModel::Small);
				case CodeSize::Kernel:  return std::optional<llvm::CodeModel::Model>(llvm::CodeModel::Kernel);
				case CodeSize::Medium:  return std::optional<llvm::CodeModel::Model>(llvm::CodeModel::Medium);
				case CodeSize::Large:   return std::optional<llvm::CodeModel::Model>(llvm::CodeModel::Large);
			}
			evo::debugFatalBreak("Unknown or unsupported code size mode");
		}();

		const llvm::CodeGenOptLevel code_gen_opt_level = [&](){
			switch(opt_level){
				case OptLevel::None:       return llvm::CodeGenOptLevel::None;
				case OptLevel::Less:       return llvm::CodeGenOptLevel::Less;
				case OptLevel::Default:    return llvm::CodeGenOptLevel::Default;
				case OptLevel::Aggressive: return llvm::CodeGenOptLevel::Aggressive;
			}
			evo::debugFatalBreak("Unknown or unsupported opt level");
		}();

		this->target_machine = target->createTargetMachine(
			target_triple, cpu, features, target_options, reloc_model, code_model, code_gen_opt_level, is_jit
		);

		this->native()->setDataLayout(target_machine->createDataLayout());

		return error_msg;
	}


	auto Module::createFunction(
		evo::CStrProxy name, const FunctionType& prototype, llvmint::LinkageType linkage
	) -> Function {
		evo::debugAssert(this->isInitialized(), "not initialized");

		return Function(llvm::Function::Create(
			prototype.native(), static_cast<llvm::GlobalValue::LinkageTypes>(linkage), name.c_str(), this->native()
		));
	};


	auto Module::createGlobal(
		const llvmint::Constant& value,
		const llvmint::Type& type,
		llvmint::LinkageType linkage,
		bool is_constant,
		evo::CStrProxy name
	) -> llvmint::GlobalVariable {
		evo::debugAssert(this->isInitialized(), "not initialized");

		// this gets freed automatically in the destructor of the module
		llvm::GlobalVariable* global = new llvm::GlobalVariable(
			*this->native(),
			type.native(),
			is_constant,
			static_cast<llvm::GlobalValue::LinkageTypes>(linkage),
			value.native(),
			name.c_str()
		);


		return llvmint::GlobalVariable(global);
	}

	auto Module::createGlobalUninit(
		const llvmint::Type& type, llvmint::LinkageType linkage, bool is_constant, evo::CStrProxy name
	) -> llvmint::GlobalVariable {
		evo::debugAssert(this->isInitialized(), "not initialized");

		return this->createGlobal(llvm::UndefValue::get(type.native()), type, linkage, is_constant, name);
	}

	auto Module::createGlobalZeroinit(
		const llvmint::Type& type, llvmint::LinkageType linkage, bool is_constant, evo::CStrProxy name
	) -> llvmint::GlobalVariable {
		evo::debugAssert(this->isInitialized(), "not initialized");

		return this->createGlobal(llvm::ConstantAggregateZero::get(type.native()), type, linkage, is_constant, name);
	}


	auto Module::print() const -> std::string {
		evo::debugAssert(this->isInitialized(), "not initialized");

		auto data = llvm::SmallVector<char>();
		auto stream = llvm::raw_svector_ostream(data);

		this->native()->print(stream, nullptr);

		const llvm::StringRef str_ref = stream.str(); 
		return str_ref.str();
	}

		
	auto Module::get_clone() const -> std::unique_ptr<llvm::Module> {
		evo::debugAssert(this->isInitialized(), "not initialized");

		return llvm::CloneModule(*this->_native);
	}


}