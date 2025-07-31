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
		this->_native = nullptr;
	}


	// auto Module::getDefaultTargetTriple() -> std::string {
	// 	return llvm::sys::getDefaultTargetTriple();
	// }




	auto Module::setTargetAndDataLayout(
		core::Target target,
		Relocation relocation,
		CodeSize code_size,
		OptLevel opt_level,
		bool is_jit,
		ArchSpecificSettings arch_specific_settings
	) -> std::string {
		evo::debugAssert(this->isInitialized(), "not initialized");


		if(arch_specific_settings.is<ArchSpecificSettingsDefault>()){
			switch(target.architecture){
				case core::Target::Architecture::X86_64: {
					arch_specific_settings = ArchSpecificSettingsX86();
				} break;

				case core::Target::Architecture::UNKNOWN: break;
			}
		}


		#if defined(PCIT_CONFIG_DEBUG)
			arch_specific_settings.visit([&](const auto& settings) -> void {
				using Settings = std::decay_t<decltype(settings)>;

				if constexpr(std::is_same<Settings, ArchSpecificSettingsDefault>()){
					evo::debugAssert(
						target.architecture == core::Target::Architecture::UNKNOWN,
						"Architecture specific settings should have been set to correct architecture"
					);

				}else if constexpr(std::is_same<Settings, ArchSpecificSettingsX86>()){
					evo::debugAssert(
						target.architecture == core::Target::Architecture::X86_64,
						"Architecture and arch specific settings do not match"
					);

				}else{
					static_assert(false, "Unsupported target");
				}
			});
		#endif


		const llvm::Triple::ArchType triple_arch = [&](){
			switch(target.architecture){
				case core::Target::Architecture::UNKNOWN: return llvm::Triple::ArchType::UnknownArch;
				case core::Target::Architecture::X86_64:  return llvm::Triple::ArchType::x86_64;
			}

			evo::unreachable();
		}();

		const llvm::Triple::SubArchType triple_sub_arch = llvm::Triple::SubArchType::NoSubArch;

		const llvm::Triple::OSType triple_os = [&](){
			switch(target.platform){
				case core::Target::Platform::UNKNOWN: return llvm::Triple::OSType::UnknownOS;
				case core::Target::Platform::WINDOWS: return llvm::Triple::OSType::Win32;
				case core::Target::Platform::LINUX:   return llvm::Triple::OSType::Linux;
			}

			evo::unreachable();
		}();

		const llvm::Triple::VendorType triple_vendor = [&](){
			switch(target.platform){
				case core::Target::Platform::UNKNOWN: return llvm::Triple::VendorType::UnknownVendor;
				case core::Target::Platform::WINDOWS: return llvm::Triple::VendorType::PC;
				case core::Target::Platform::LINUX:   return llvm::Triple::VendorType::UnknownVendor;
			}

			evo::unreachable();
		}();

		const llvm::Triple::EnvironmentType triple_enviroment = [&](){
			switch(target.platform){
				case core::Target::Platform::UNKNOWN: return llvm::Triple::EnvironmentType::UnknownEnvironment;
				case core::Target::Platform::WINDOWS: return llvm::Triple::EnvironmentType::MSVC;
				case core::Target::Platform::LINUX:   return llvm::Triple::EnvironmentType::GNU;
			}

			evo::unreachable();
		}();

		// const llvm::Triple::ObjectFormatType triple_object_format = [&](){
		// 	switch(target.platform){
		// 		case core::Target::Platform::UNKNOWN: return llvm::Triple::ObjectFormatType::UnknownObjectFormat;
		// 		case core::Target::Platform::WINDOWS: return llvm::Triple::ObjectFormatType::COFF;
		// 		case core::Target::Platform::LINUX:   return llvm::Triple::ObjectFormatType::ELF;
		// 	}

		// 	evo::unreachable();
		// }();


		if(arch_specific_settings.is<ArchSpecificSettingsX86>()){
			if(
				arch_specific_settings.as<ArchSpecificSettingsX86>().dialect == 
				ArchSpecificSettingsX86::AssemblyDialect::INTEL
			){
				const auto llvm_cmd_args = std::array<const char*, 2>{"", "--x86-asm-syntax=intel"};
			    const bool llvm_parse_cmd_res = llvm::cl::ParseCommandLineOptions(
			    	int(llvm_cmd_args.size()), llvm_cmd_args.data()
			    );
			    evo::debugAssert(llvm_parse_cmd_res, "Failed to parse llvm cmd args");
			}
		}

		auto triple = llvm::Triple();
		triple.setArch(triple_arch, triple_sub_arch);
		triple.setVendor(triple_vendor);
		triple.setOS(triple_os);
		triple.setEnvironment(triple_enviroment);
		// triple.setObjectFormat(triple_object_format);


		this->_native->setTargetTriple(triple);


		auto error_msg = std::string();
		const llvm::Target* llvm_target = llvm::TargetRegistry::lookupTarget(triple, error_msg);

		if(llvm_target == nullptr){ return error_msg; }

		static constexpr std::string_view cpu = "generic";
		static constexpr std::string_view features = "";
		
		// TODO(FUTURE): https://llvm.org/doxygen/classllvm_1_1TargetOptions.html
		auto target_options = llvm::TargetOptions();

		const std::optional<llvm::Reloc::Model> reloc_model = [&](){
			switch(relocation){
				case Relocation::DEFAULT:        return std::optional<llvm::Reloc::Model>();
				case Relocation::STATIC:         return std::optional<llvm::Reloc::Model>(llvm::Reloc::Static);
				case Relocation::PIC:            return std::optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
				case Relocation::DYNAMIC_NO_PIC: return std::optional<llvm::Reloc::Model>(llvm::Reloc::DynamicNoPIC);
				case Relocation::ROPI:           return std::optional<llvm::Reloc::Model>(llvm::Reloc::ROPI);
				case Relocation::RWPI:           return std::optional<llvm::Reloc::Model>(llvm::Reloc::RWPI);
				case Relocation::ROPI_RWPI:      return std::optional<llvm::Reloc::Model>(llvm::Reloc::ROPI_RWPI);
			}
			evo::debugFatalBreak("Unknown or unsupported relocation mode");
		}();

		const std::optional<llvm::CodeModel::Model> code_model = [&](){
			switch(code_size){
				case CodeSize::DEFAULT: return std::optional<llvm::CodeModel::Model>();
				// case CodeSize::Tiny:    return std::optional<llvm::CodeModel::Model>(llvm::CodeModel::Tiny);
				case CodeSize::SMALL:   return std::optional<llvm::CodeModel::Model>(llvm::CodeModel::Small);
				case CodeSize::KERNEL:  return std::optional<llvm::CodeModel::Model>(llvm::CodeModel::Kernel);
				case CodeSize::MEDIUM:  return std::optional<llvm::CodeModel::Model>(llvm::CodeModel::Medium);
				case CodeSize::LARGE:   return std::optional<llvm::CodeModel::Model>(llvm::CodeModel::Large);
			}
			evo::debugFatalBreak("Unknown or unsupported code size mode");
		}();

		const llvm::CodeGenOptLevel code_gen_opt_level = [&](){
			switch(opt_level){
				case OptLevel::NONE:       return llvm::CodeGenOptLevel::None;
				case OptLevel::LESS:       return llvm::CodeGenOptLevel::Less;
				case OptLevel::DEFAULT:    return llvm::CodeGenOptLevel::Default;
				case OptLevel::AGGRESSIVE: return llvm::CodeGenOptLevel::Aggressive;
			}
			evo::debugFatalBreak("Unknown or unsupported opt level");
		}();


		this->target_machine = llvm_target->createTargetMachine(
			triple, cpu, features, target_options, reloc_model, code_model, code_gen_opt_level, is_jit
		);

		this->native()->setDataLayout(target_machine->createDataLayout());

		return error_msg;
	}


	auto Module::createFunction(
		evo::CStrProxy name, const FunctionType& prototype, LinkageType linkage
	) -> Function {
		evo::debugAssert(this->isInitialized(), "not initialized");

		return Function(llvm::Function::Create(
			prototype.native(), static_cast<llvm::GlobalValue::LinkageTypes>(linkage), name.c_str(), this->native()
		));
	};


	auto Module::createGlobal(
		const Constant& value, const Type& type, LinkageType linkage, bool is_constant, evo::CStrProxy name
	) -> GlobalVariable {
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


		return GlobalVariable(global);
	}

	auto Module::createGlobalUninit(const Type& type, LinkageType linkage, bool is_constant, evo::CStrProxy name)
	-> GlobalVariable {
		evo::debugAssert(this->isInitialized(), "not initialized");

		return this->createGlobal(llvm::UndefValue::get(type.native()), type, linkage, is_constant, name);
	}

	auto Module::createGlobalZeroinit(const Type& type, LinkageType linkage, bool is_constant, evo::CStrProxy name)
	-> GlobalVariable {
		evo::debugAssert(this->isInitialized(), "not initialized");

		return this->createGlobal(llvm::ConstantAggregateZero::get(type.native()), type, linkage, is_constant, name);
	}


	auto Module::createGlobalString(std::string value, LinkageType linkage, bool is_constant, evo::CStrProxy name)
	-> GlobalVariable {
		evo::debugAssert(this->isInitialized(), "not initialized");

		llvm::Constant* str_constant = llvm::ConstantDataArray::getString(this->native()->getContext(), value);
		GlobalVariable new_var = this->createGlobal(str_constant, str_constant->getType(), linkage, is_constant, name);
		new_var.native()->setUnnamedAddr(llvm::GlobalValue::UnnamedAddr::Global);
		new_var.native()->setAlignment(llvm::Align(1));
		return new_var;
	}


	auto Module::print() const -> std::string {
		evo::debugAssert(this->isInitialized(), "not initialized");

		auto data = llvm::SmallVector<char>();
		auto stream = llvm::raw_svector_ostream(data);

		this->native()->print(stream, nullptr);

		const llvm::StringRef str_ref = stream.str(); 
		return str_ref.str();
	}


	auto Module::lowerToObject() -> evo::Result<std::vector<evo::byte>> {
		auto data = llvm::SmallVector<char>();
		auto stream = llvm::raw_svector_ostream(data);

		auto pass = llvm::legacy::PassManager();
		static constexpr auto file_type = llvm::CodeGenFileType::ObjectFile;

		#if defined(PCIT_CONFIG_DEBUG)
			static constexpr bool disable_verify = false;
		#else
			static constexpr bool disable_verify = true;
		#endif

		if(this->target_machine->addPassesToEmitFile(pass, stream, nullptr, file_type, disable_verify)){
			return evo::resultError;
		}

		pass.run(*this->native());


		auto output = std::vector<evo::byte>();
		output.resize(data.size());

		std::memcpy(output.data(), data.data(), data.size());

		return output;
	}

	auto Module::lowerToAssembly() -> evo::Result<std::string> {
		auto data = llvm::SmallVector<char>();
		auto stream = llvm::raw_svector_ostream(data);

		auto pass = llvm::legacy::PassManager();
		static constexpr auto file_type = llvm::CodeGenFileType::AssemblyFile;

		#if defined(PCIT_CONFIG_DEBUG)
			static constexpr bool disable_verify = false;
		#else
			static constexpr bool disable_verify = true;
		#endif


		if(this->target_machine->addPassesToEmitFile(pass, stream, nullptr, file_type, disable_verify)){
			return evo::resultError;
		}


		pass.run(*this->native());


		auto output = std::string();
		output.resize(data.size());

		std::memcpy(output.data(), data.data(), data.size());

		return output;
	}


	auto Module::optimize(OptMode opt_mode) -> void {
		if(opt_mode == OptMode::NONE){ return; }

		// DO NOT RE-ORDER THESE (destructors must be called in this order)
		auto loop_analysis_manager = llvm::LoopAnalysisManager();
		auto function_analysis_manager = llvm::FunctionAnalysisManager();
		auto cgscc_analysis_manager = llvm::CGSCCAnalysisManager();
		auto module_analysis_manager = llvm::ModuleAnalysisManager();

		// TODO(PERF): check options of pipeline_tuning_options
		//       (https://llvm.org/doxygen/classllvm_1_1PipelineTuningOptions.html)
		auto pipeline_tuning_options = llvm::PipelineTuningOptions();

		auto pass_builder = llvm::PassBuilder(this->target_machine, pipeline_tuning_options);

		pass_builder.registerModuleAnalyses(module_analysis_manager);
		pass_builder.registerCGSCCAnalyses(cgscc_analysis_manager);
		pass_builder.registerFunctionAnalyses(function_analysis_manager);
		pass_builder.registerLoopAnalyses(loop_analysis_manager);
		pass_builder.crossRegisterProxies(
			loop_analysis_manager, function_analysis_manager, cgscc_analysis_manager, module_analysis_manager
		);

		const llvm::OptimizationLevel llvm_opt_level = [&](){
			switch(opt_mode){
				case OptMode::O0: evo::unreachable();
				case OptMode::O1: return llvm::OptimizationLevel::O1;
				case OptMode::O2: return llvm::OptimizationLevel::O2;
				case OptMode::O3: return llvm::OptimizationLevel::O3;
				case OptMode::Os: return llvm::OptimizationLevel::Os;
				case OptMode::Oz: return llvm::OptimizationLevel::Oz;
			}

			evo::unreachable();
		}();

		llvm::ModulePassManager module_pass_manager = pass_builder.buildPerModuleDefaultPipeline(llvm_opt_level);
		module_pass_manager.run(*this->native(), module_analysis_manager);
	}

		
	auto Module::get_clone() const -> std::unique_ptr<llvm::Module> {
		evo::debugAssert(this->isInitialized(), "not initialized");

		return llvm::CloneModule(*this->_native);
	}


}