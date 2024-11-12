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

	auto module = pcit::pir::Module("testing");
	auto agent = pcit::pir::Agent(module);

	const pcit::pir::GlobalVar::ID global = module.createGlobalVar(
		"global",
		module.createTypeUnsigned(17),
		pcit::pir::Linkage::Internal,
		agent.createNumber(module.createTypeUnsigned(17), pcit::core::GenericInt::create<uint64_t>(12)),
		true,
		false
	);


	const pcit::pir::FunctionDecl::ID puts_decl = module.createFunctionDecl(
		"puts",
		evo::SmallVector<pcit::pir::Parameter>{pcit::pir::Parameter("str", module.createTypePtr())},
		pcit::pir::CallingConvention::C,
		pcit::pir::Linkage::External,
		module.createTypeVoid()
	);

	const pcit::pir::Type vec2 = module.createTypeStruct(
		"Vec2", evo::SmallVector<pcit::pir::Type>{module.createTypeFloat(32), module.createTypeFloat(32)}, true
	);


	const pcit::pir::Function::ID testing_func_id = module.createFunction(
		"test()",
		evo::SmallVector<pcit::pir::Parameter>{
			pcit::pir::Parameter("vec2", vec2),
			pcit::pir::Parameter("number", module.createTypeUnsigned(64))
		},
		pcit::pir::CallingConvention::Fast,
		pcit::pir::Linkage::Internal,
		module.createTypeUnsigned(64)
	);
	agent.setTargetFunction(testing_func_id);



	const pcit::pir::BasicBlock::ID entry_block_id = agent.createBasicBlock();
	agent.setTargetBasicBlock(entry_block_id);


	const pcit::pir::Expr add = agent.createAdd(
		agent.createNumber(module.createTypeUnsigned(64), pcit::core::GenericInt::create<uint64_t>(9)),
		agent.createNumber(module.createTypeUnsigned(64), pcit::core::GenericInt::create<uint64_t>(3)),
		false,
		"ADD"
	);

	const pcit::pir::Expr add2 = agent.createAdd(add, agent.createParamExpr(1), false, "ADD");
	const pcit::pir::Expr val_alloca = agent.createAlloca(module.createTypeUnsigned(64), "VAL");

	const pcit::pir::Expr add3 = agent.createAdd(
		add2,
		agent.createNumber(module.createTypeUnsigned(64), pcit::core::GenericInt::create<uint64_t>(0)),
		false,
		"ADD"
	);

	std::ignore = agent.createAdd(add, agent.createParamExpr(1), true, "UNUSED");

	const pcit::pir::BasicBlock::ID second_block_id = agent.createBasicBlock();
	agent.createBrInst(second_block_id);
	agent.setTargetBasicBlock(second_block_id);

	agent.createCallVoidInst(puts_decl, evo::SmallVector<pcit::pir::Expr>{module.createGlobalValue(global)});

	agent.createRetInst(add3);


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

	// end test
	//////////////////////////////////////////////////////////////////////


	return EXIT_SUCCESS;
}
