////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/OrcJIT.h"

#include "../include/LLVMContext.h"
#include "../include/Module.h"

#include <LLVM.h>

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::llvmint{

	struct OrcJIT::Data{
		std::unique_ptr<llvm::orc::LLJIT> lljit;
	};


	auto OrcJIT::init(const InitConfig& config) -> evo::Result<> {
		auto lljit_builder = llvm::orc::LLJITBuilder();

		if(config.allowDefaultSymbolLinking == false){
			lljit_builder.setLinkProcessSymbolsByDefault(false);
			// this is needed because setLinkProcessSymbolsByDefault on it's own causes creation to fail
			lljit_builder.setProcessSymbolsJITDylibSetup(
				[](llvm::orc::LLJIT& lljit){ return &lljit.getExecutionSession().createBareJITDylib("<procsymbol>"); }
			);
		}

		llvm::Expected<std::unique_ptr<llvm::orc::LLJIT>> created_lljit = lljit_builder.create();

		if(bool(created_lljit) == false){ return evo::resultError; }

		this->data = new Data(std::move(*created_lljit));
		return evo::Result<>();
	}


	auto OrcJIT::deinit() -> void {
		delete this->data;
		this->data = nullptr;
	}


	auto OrcJIT::addModule(LLVMContext&& context, Module&& module) -> evo::Result<> {
		llvm::Error add_module_result = this->data->lljit->addIRModule(
			llvm::orc::ThreadSafeModule(
				std::unique_ptr<llvm::Module>(module.steal()),
				std::unique_ptr<llvm::LLVMContext>(context.steal())
			)
		);
		return evo::Result<>::fromBool(bool(add_module_result));
	}


	auto OrcJIT::lookupFunc(std::string_view name) -> void* {
		return (void*)(this->data->lljit->lookup(name)->getValue());
	}




	using LLVMSymbolMapPair = llvm::detail::DenseMapPair<llvm::orc::SymbolStringPtr, llvm::orc::ExecutorSymbolDef>;

	auto OrcJIT::registerFunc(std::string_view name, void* func_call_address) -> evo::Result<> {
		llvm::Error define_res = this->data->lljit->getMainJITDylib().define(
			llvm::orc::absoluteSymbols(
				llvm::orc::SymbolMap{
					LLVMSymbolMapPair(
						this->data->lljit->mangleAndIntern(name),
						llvm::orc::ExecutorSymbolDef(
							llvm::orc::ExecutorAddr::fromPtr(func_call_address),
							llvm::JITSymbolFlags(llvm::JITSymbolFlags::Absolute | llvm::JITSymbolFlags::Callable)
						)
					)
				}
			)
		);

		return evo::Result<>::fromBool(bool(define_res));
	}


	auto OrcJIT::registerFuncs(evo::ArrayProxy<FuncRegisterInfo> func_register_infos) -> evo::Result<> {
		auto symbol_list = evo::SmallVector<LLVMSymbolMapPair>();
		symbol_list.reserve(func_register_infos.size());

		for(const FuncRegisterInfo& func_register_info : func_register_infos){
			symbol_list.emplace_back(
				this->data->lljit->mangleAndIntern(func_register_info.name),
				llvm::orc::ExecutorSymbolDef(
					llvm::orc::ExecutorAddr::fromPtr(func_register_info.funcCallAddress),
					llvm::JITSymbolFlags(llvm::JITSymbolFlags::Absolute | llvm::JITSymbolFlags::Callable)
				)
			);
		}

		llvm::Error define_res = this->data->lljit->getMainJITDylib().define(
			llvm::orc::absoluteSymbols(llvm::orc::SymbolMap(symbol_list.begin(), symbol_list.end()))
		);

		return evo::Result<>::fromBool(bool(define_res));
	}



}