////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

#include <Evo.h>

#include <PCIT_core.h>

#include <iostream>


#if defined(EVO_PLATFORM_WINDOWS)
	#if !defined(WIN32_LEAN_AND_MEAN)
		#define WIN32_LEAN_AND_MEAN
	#endif

	#if !defined(NOCOMM)
		#define NOCOMM
	#endif

	#if !defined(NOMINMAX)
		#define NOMINMAX
	#endif

	#include <Windows.h>
#endif


#include <PIR.h>

template<typename... T, typename Callable>
auto wrapCallable(Callable const&) -> void (*)(T..., void*) {
    return +[](T... data, void* context)
    {
        (*static_cast<Callable*>(context))(data...);
    };
}



auto main(int argc, const char* argv[]) -> int {
	auto args = std::vector<std::string_view>(argv, argv + argc);

	// print UTF-8 characters on windows
	#if defined(EVO_PLATFORM_WINDOWS)
		::SetConsoleOutputCP(CP_UTF8);
	#endif

	#if defined(PCIT_CONFIG_DEBUG)
		evo::log::setDefaultThreadSaferCallback();
	#endif

	const bool print_color = pcit::core::Printer::platformSupportsColor() == pcit::core::Printer::DetectResult::Yes;
	auto printer = pcit::core::Printer::createConsole(print_color);

	printer.printlnCyan("PIRC");
	printer.printlnGray("TESTING...");

	#if defined(PCIT_BUILD_DEBUG)
		printer.printlnMagenta("v{} (debug)", pcit::core::version);
	#elif defined(PCIT_BUILD_DEV)
		printer.printlnMagenta("v{} (dev)", pcit::core::version);
	#elif defined(PCIT_BUILD_OPTIMIZE)
		printer.printlnMagenta("v{} (optimize)", pcit::core::version);
	#elif defined(PCIT_BUILD_RELEASE)
		printer.printlnMagenta("v{} (release)", pcit::core::version);
	#elif defined(PCIT_BUILD_DIST)
		printer.printlnMagenta("v{}", pcit::core::version);
	#else
		#error Unknown or unsupported build
	#endif

	#if !defined(PCIT_BUILD_DIST) && defined(EVO_PLATFORM_WINDOWS)
		if(::IsDebuggerPresent()){
			static auto at_exit_call = [&]() -> void {
				// not using printer because it should always go to stdout
				if(print_color){
					evo::printGray("Press [Enter] to close...");
				}else{
					evo::print("Press [Enter] to close...");
				}

				std::cin.get();
				evo::println();
			};
			std::atexit([]() -> void {
				at_exit_call();
			});
		}
	#endif


	//////////////////////////////////////////////////////////////////////
	// begin test

	auto module = pcit::pir::Module("PIR testing", pcit::core::getCurrentOS(), pcit::core::getCurrentArchitecture());
	auto agent = pcit::pir::Agent(module);

	const pcit::pir::GlobalVar::ID global = module.createGlobalVar(
		"global",
		module.createUnsignedType(17),
		pcit::pir::Linkage::Internal,
		agent.createNumber(module.createUnsignedType(17), pcit::core::GenericInt::create<uint64_t>(18)),
		true,
		false
	);


	const pcit::pir::FunctionDecl::ID print_hello_decl = module.createFunctionDecl(
		"print_hello",
		evo::SmallVector<pcit::pir::Parameter>{pcit::pir::Parameter("str", module.createPtrType())},
		pcit::pir::CallingConvention::C,
		pcit::pir::Linkage::External,
		module.createVoidType()
	);

	const pcit::pir::Type vec2 = module.createStructType(
		"Vec2", evo::SmallVector<pcit::pir::Type>{module.createFloatType(32), module.createFloatType(32)}, true
	);


	const pcit::pir::Function::ID entry_func_id = module.createFunction(
		"entry",
		evo::SmallVector<pcit::pir::Parameter>{
			// pcit::pir::Parameter("vec2", vec2),
			// pcit::pir::Parameter("number", module.createUnsignedType(64))
		},
		pcit::pir::CallingConvention::Fast,
		pcit::pir::Linkage::Internal,
		module.createUnsignedType(64)
	);
	agent.setTargetFunction(entry_func_id);



	const pcit::pir::BasicBlock::ID entry_block_id = agent.createBasicBlock();
	agent.setTargetBasicBlock(entry_block_id);


	const pcit::pir::Expr add = agent.createAddWrap(
		agent.createNumber(module.createUnsignedType(64), pcit::core::GenericInt::create<uint64_t>(9)),
		agent.createNumber(module.createUnsignedType(64), pcit::core::GenericInt::create<uint64_t>(3)),
		"ADD_WRAP.RESULT",
		"ADD_WRAP.WRAPPED"
	);

	// const pcit::pir::Expr add2 = agent.createAdd(add, agent.createParamExpr(1), false, "ADD");
	const pcit::pir::Expr val_alloca = agent.createAlloca(module.createUnsignedType(64), "VAL");

	const pcit::pir::Expr add3 = agent.createAddWrap(
		agent.extractAddWrapResult(add),
		agent.createNumber(module.createUnsignedType(64), pcit::core::GenericInt::create<uint64_t>(0)),
		"ADD_WRAP.RESULT",
		"ADD_WRAP.WRAPPED"
	);

	// std::ignore = agent.createAdd(add, agent.createParamExpr(1), true, "UNUSED");

	const pcit::pir::BasicBlock::ID second_block_id = agent.createBasicBlock();
	agent.createBranch(second_block_id);
	agent.setTargetBasicBlock(second_block_id);

	agent.createCallVoid(print_hello_decl, evo::SmallVector<pcit::pir::Expr>{agent.createGlobalValue(global)});
	// agent.createCallVoid(print_hello_decl, evo::SmallVector<pcit::pir::Expr>{val_alloca});

	agent.createRet(agent.extractAddWrapResult(add3));


	printer.printlnGray("--------------------------------");

	pcit::pir::printModule(module, printer);


	printer.printlnGray("--------------------------------");

	const unsigned num_threads = pcit::pir::PassManager::optimalNumThreads();
	// const unsigned num_threads = 0;
	auto pass_manager = pcit::pir::PassManager(module, num_threads);

	pass_manager.addPass(pcit::pir::passes::removeUnusedStmts());
	pass_manager.addPass(pcit::pir::passes::instCombine());
	const bool opt_result = pass_manager.run();
	if(opt_result == false){
		printer.printlnError("Error occured while running pass");
		return EXIT_FAILURE;
	}

	pcit::pir::printModule(module, printer);



	printer.printlnGray("--------------------------------");

	auto lowered = pcit::pir::lowerToLLVMIR(module);
	if(lowered.has_value()){
		printer.printlnCyan(lowered.value());
	}else{
		printer.printlnError("ERROR in LLVM: \"{}\"", lowered.error());
	}


	printer.printlnGray("--------------------------------");


	auto jit_engine = pcit::pir::JITEngine();
	jit_engine.init(module);



	
	jit_engine.registerFunction(print_hello_decl, []() -> void {
		evo::printlnYellow("Hello from PIR");
	});

	const evo::Result<pcit::core::GenericValue> result = jit_engine.runFunc(entry_func_id);
	if(result.isSuccess()){
		printer.printlnSuccess("value returned from entry: {}", result.value());
	}

	jit_engine.deinit();


	// end test
	//////////////////////////////////////////////////////////////////////



	return EXIT_SUCCESS;
}
