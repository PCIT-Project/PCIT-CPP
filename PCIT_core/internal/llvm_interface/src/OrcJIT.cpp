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
#include "../include/init.h"

#include <LLVM.h>

#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::llvmint{

	struct OrcJIT::Data{
		std::unique_ptr<llvm::orc::LLJIT> lljit;
	};

	static auto extract_llvm_error_messages(llvm::Error&& error) -> evo::SmallVector<std::string> {
		auto error_msgs = evo::SmallVector<std::string>();

		llvm::handleAllErrors(std::move(error), [&](const llvm::ErrorInfoBase& error_base) -> void {
			error_msgs.emplace_back(error_base.message());
		});

		return error_msgs;
	}


	static auto is_error(llvm::Error& error) -> bool {
		return bool(error);
	}

	template<class T>
	static auto is_error(llvm::Expected<T>& expected) -> bool {
		return bool(expected) == false;
	}


	auto OrcJIT::init(const InitConfig& config) -> evo::Expected<void, evo::SmallVector<std::string>> {
		evo::debugAssert(this->isInitialized() == false, "OrcJIT already initialized");

		if(isInitialized() == false){ llvmint::init(); }

		auto lljit_builder = llvm::orc::LLJITBuilder();

		if(config.allowDefaultSymbolLinking == false){
			lljit_builder.setLinkProcessSymbolsByDefault(false);

			// this is needed because setLinkProcessSymbolsByDefault on it's own causes creation to fail
			lljit_builder.setProcessSymbolsJITDylibSetup(
				[](llvm::orc::LLJIT& lljit){ return &lljit.getExecutionSession().createBareJITDylib("<procsymbol>"); }
			);
		}

		llvm::Expected<std::unique_ptr<llvm::orc::LLJIT>> created_lljit = lljit_builder.create();


		if(is_error(created_lljit)){
			return evo::Unexpected(extract_llvm_error_messages(created_lljit.takeError()));
		}

		this->data = new Data(std::move(*created_lljit));
		return {};
	}


	auto OrcJIT::deinit() -> void {
		evo::debugAssert(this->isInitialized(), "OrcJIT not initialized");

		delete this->data;
		this->data = nullptr;
	}


	auto OrcJIT::addModule(LLVMContext&& context, Module&& module)
	-> evo::Expected<void, evo::SmallVector<std::string>> {
		evo::debugAssert(this->isInitialized(), "OrcJIT not initialized");

		llvm::Error add_module_result = this->data->lljit->addIRModule(
			llvm::orc::ThreadSafeModule(
				std::unique_ptr<llvm::Module>(module.steal()),
				std::unique_ptr<llvm::LLVMContext>(context.steal())
			)
		);


		if(is_error(add_module_result)){
			return evo::Unexpected(extract_llvm_error_messages(std::move(add_module_result)));
		}

		return {};
	}


	auto OrcJIT::addModule(llvm::LLVMContext* context, llvm::Module* module)
	-> evo::Expected<void, evo::SmallVector<std::string>> {
		evo::debugAssert(this->isInitialized(), "OrcJIT not initialized");

		llvm::Error add_module_result = this->data->lljit->addIRModule(
			llvm::orc::ThreadSafeModule(
				std::unique_ptr<llvm::Module>(module),
				std::unique_ptr<llvm::LLVMContext>(context)
			)
		);


		if(is_error(add_module_result)){
			return evo::Unexpected(extract_llvm_error_messages(std::move(add_module_result)));
		}

		return {};
	}



	auto OrcJIT::lookupSymbol(std::string_view name) -> void* {
		evo::debugAssert(this->isInitialized(), "OrcJIT not initialized");

		const auto lock = std::scoped_lock(this->mutex);

		return (void*)(this->data->lljit->lookup(name)->getValue());
	}




	using LLVMSymbolMapPair = llvm::detail::DenseMapPair<llvm::orc::SymbolStringPtr, llvm::orc::ExecutorSymbolDef>;

	auto OrcJIT::registerFunc(std::string_view name, void* func_call_address)
	-> evo::Expected<void, evo::SmallVector<std::string>> {
		evo::debugAssert(this->isInitialized(), "OrcJIT not initialized");

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

		if(is_error(define_res)){ return evo::Unexpected(extract_llvm_error_messages(std::move(define_res))); }
		return {};
	}


	auto OrcJIT::registerFuncs(evo::ArrayProxy<FuncRegisterInfo> func_register_infos)
	-> evo::Expected<void, evo::SmallVector<std::string>> {
		evo::debugAssert(this->isInitialized(), "OrcJIT not initialized");

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

		if(is_error(define_res)){ return evo::Unexpected(extract_llvm_error_messages(std::move(define_res))); }
		return {};
	}



}