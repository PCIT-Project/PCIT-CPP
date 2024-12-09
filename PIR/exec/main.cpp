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


struct Config{
	bool optimize = true;
	bool print_assembly = true;
} config;


auto print_title(pcit::core::Printer& printer) -> void {
	// https://www.asciiart.eu/text-to-ascii-art
	// modified from the `Slant` font with the `Fitted` horizontal layout

	printer.printlnCyan(R"(            _            
    ____   (_)_____ _____
   / __ \ / // ___// ___/
  / /_/ // // /   / /__  (PCIT Intermediate Representation Compiler)
 / ____//_//_/    \___/  
/_/)");

	printer.printlnGray("--------------------------------------------------------------------");
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

	print_title(printer);

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
		module.createIntegerType(17),
		pcit::pir::Linkage::Internal,
		agent.createNumber(module.createIntegerType(17), pcit::core::GenericInt::create<uint64_t>(18)),
		true
	);

	const pcit::pir::GlobalVar::String::ID global_str_value = module.createGlobalString(
		"[NOT PRINTED] Hello World, I'm PIR!"
	);
	const pcit::pir::GlobalVar::ID global_str = module.createGlobalVar(
		"string", module.getGlobalString(global_str_value).type, pcit::pir::Linkage::Private, global_str_value, true
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

	const pcit::pir::GlobalVar::ID vec2_value = module.createGlobalVar(
		"vec2", vec2, pcit::pir::Linkage::Private, module.createGlobalStruct(vec2, {
			agent.createNumber(module.createFloatType(32), pcit::core::GenericFloat::createF32(12)),
			agent.createNumber(module.createFloatType(32), pcit::core::GenericFloat::createF32(19)),
		}), true
	);


	const pcit::pir::Function::ID entry_func_id = module.createFunction(
		"entry",
		evo::SmallVector<pcit::pir::Parameter>{
			// pcit::pir::Parameter("vec2", vec2),
			// pcit::pir::Parameter("number", module.createIntegerType(64))
		},
		pcit::pir::CallingConvention::Fast,
		pcit::pir::Linkage::External,
		module.createIntegerType(64)
	);
	agent.setTargetFunction(entry_func_id);


	const pcit::pir::BasicBlock::ID entry_block_id = agent.createBasicBlock();
	agent.setTargetBasicBlock(entry_block_id);


	const pcit::pir::Expr add = agent.createUAddWrap(
		agent.createNumber(module.createIntegerType(64), pcit::core::GenericInt::create<uint64_t>(9)),
		agent.createNumber(module.createIntegerType(64), pcit::core::GenericInt::create<uint64_t>(3)),
		"ADD_WRAP.RESULT",
		"ADD_WRAP.WRAPPED"
	);

	// const pcit::pir::Expr add2 = agent.createAdd(add, agent.createParamExpr(1), false, "ADD");
	const pcit::pir::Expr val_alloca = agent.createAlloca(module.createIntegerType(64), "VAL");

	const pcit::pir::Expr add3 = agent.createUAddWrap(
		agent.extractUAddWrapResult(add),
		agent.createNumber(module.createIntegerType(64), pcit::core::GenericInt::create<uint64_t>(0)),
		"ADD_WRAP.RESULT",
		"ADD_WRAP.WRAPPED"
	);

	std::ignore = agent.createUAddWrap(
		agent.extractUAddWrapResult(add),
		agent.createNumber(module.createIntegerType(64), pcit::core::GenericInt::create<uint64_t>(2))
	);

	agent.createStore(
		val_alloca,
		agent.createNumber(module.createIntegerType(64), pcit::core::GenericInt::create<uint64_t>(2)),
		false,
		pcit::pir::AtomicOrdering::Release
	);
	std::ignore = agent.createLoad(val_alloca, module.createIntegerType(64), true, pcit::pir::AtomicOrdering::Acquire);

	// std::ignore = agent.createAdd(add, agent.createParamExpr(1), true, "UNUSED");

	const pcit::pir::BasicBlock::ID second_block_id = agent.createBasicBlock();
	agent.createCondBranch(agent.createBoolean(true), second_block_id, entry_block_id);
	agent.setTargetBasicBlock(second_block_id);


	pcit::pir::Expr str_ptr = agent.createCalcPtr(
		agent.createGlobalValue(global_str), module.getGlobalString(global_str_value).type, {0, 14}
	);

	agent.createCallVoid(print_hello_decl, evo::SmallVector<pcit::pir::Expr>{str_ptr});
	// agent.createCallVoid(print_hello_decl, evo::SmallVector<pcit::pir::Expr>{val_alloca});

	agent.createRet(agent.extractUAddWrapResult(add3));


	printer.printlnGray("--------------------------------");

	pcit::pir::printModule(module, printer);

	if(config.optimize){
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
	}


	const pcit::pir::OptMode opt_mode = config.optimize ? pcit::pir::OptMode::O3 : pcit::pir::OptMode::None;


	printer.printlnGray("--------------------------------");

	printer.printlnCyan(pcit::pir::lowerToLLVMIR(module, opt_mode));


	if(config.print_assembly){
		printer.printlnGray("--------------------------------");

		printer.printlnCyan(pcit::pir::lowerToAssembly(module, opt_mode).value());
	}


	printer.printlnGray("--------------------------------");


	auto jit_engine = pcit::pir::JITEngine();
	jit_engine.init(module);


	jit_engine.registerFunction(print_hello_decl, [](const char* msg) -> void {
		evo::printlnYellow("Message: \"{}\"", msg);
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
