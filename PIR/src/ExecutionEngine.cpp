////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/ExecutionEngine.hpp"

#include <csignal>

#include "../include/getDefaultDebugger.hpp"
#include "../include/ExecutionEngineDebuggerInterface.hpp"
#include "../include/InstrHandler.hpp"


namespace pcit::pir{

	ExecutionEngine::ExecutionEngine(const Module& _module, uint32_t _max_call_depth)
		: module(_module), max_call_depth(_max_call_depth) {}

	ExecutionEngine::~ExecutionEngine(){
		if(this->jit_engine.isInitialized()){ this->jit_engine.deinit(); }
	}


	#if defined(EVO_PLATFORM_WINDOWS)
		auto ExecutionEngine::init(const InitConfig& config) -> evo::Expected<void, evo::SmallVector<std::string>> {
			return this->jit_engine.init(config);
		}

		auto ExecutionEngine::deinit() -> void {
			this->jit_engine.deinit();
		}

	#else

		static std::atomic<ExecutionEngine*> execution_engine_signal_ptr = nullptr;
		static std::atomic<bool> installed_signal_handlers = false;

		auto _internal_signal_handler(int signal) -> void {
			ExecutionEngine* engine = execution_engine_signal_ptr.load();
			if(engine == nullptr){ return; }

			const auto func_run_error = [&]() -> ExecutionEngineExecutor::FuncRunError {
				switch(signal){
					break; case SIGTERM: return ExecutionEngineExecutor::FuncRunError::UNKNOWN_EXCEPTION;
					break; case SIGSEGV: return ExecutionEngineExecutor::FuncRunError::SEG_FAULT;
					break; case SIGINT:  return ExecutionEngineExecutor::FuncRunError::UNKNOWN_EXCEPTION;
					break; case SIGILL:  return ExecutionEngineExecutor::FuncRunError::UNKNOWN_EXCEPTION;
					break; case SIGABRT: return ExecutionEngineExecutor::FuncRunError::ABORT;
					break; case SIGFPE:  return ExecutionEngineExecutor::FuncRunError::FLOATING_POINT_EXCEPTION;
					break; default:      return ExecutionEngineExecutor::FuncRunError::UNKNOWN_EXCEPTION;
				}
			}();

			const auto lock = std::scoped_lock(engine->executors_lock);
			std::longjmp(engine->executors.at(std::this_thread::get_id()).set_signal_error(func_run_error), true);
		}

		auto ExecutionEngine::init(const InitConfig& config) -> evo::Expected<void, evo::SmallVector<std::string>> {
			ExecutionEngine* expected = nullptr;
			if(execution_engine_signal_ptr.compare_exchange_strong(expected, this) == false){
				evo::debugFatalBreak("Execution engine with signal handler already exists");
			}

			
			if(installed_signal_handlers.exchange(true) == false){
				std::signal(SIGTERM, _internal_signal_handler);
				std::signal(SIGSEGV, _internal_signal_handler);
				std::signal(SIGINT, _internal_signal_handler);
				std::signal(SIGILL, _internal_signal_handler);
				std::signal(SIGABRT, _internal_signal_handler);
				std::signal(SIGFPE, _internal_signal_handler);
			}

			return this->jit_engine.init(config);
		}

		auto ExecutionEngine::deinit() -> void {
			execution_engine_signal_ptr = nullptr;
			this->jit_engine.deinit();
		}
	#endif


	auto ExecutionEngine::setDefaultDebugger() -> void {
		this->setDebugger(getDefaultDebugger());
	}


	auto ExecutionEngine::runFunction(Function::ID func_id, std::span<core::GenericValue> arguments)
	-> evo::Expected<core::GenericValue, FuncRunError> {
		Executor& current_executor = this->get_current_executor();
		return current_executor.runFunction(func_id, arguments);
	}



	auto ExecutionEngine::registerExternFunc(ExternalFunction::ID extern_func_id, void* func_ptr) -> void {
		const ExternalFunction& extern_func = this->module.getExternalFunction(extern_func_id);

		const Function::ID created_func_id = this->jit_engine_module.createFunction(
			std::format("PIR.ExEng-intf.{}", extern_func.name),
			evo::SmallVector<Parameter>{
				Parameter("PARAMS", this->jit_engine_module.createPtrType()),
				Parameter("RET", this->jit_engine_module.createPtrType()),
			},
			CallingConvention::C,
			Linkage::EXTERNAL,
			this->jit_engine_module.createVoidType()
		);


		const evo::Expected<void, evo::SmallVector<std::string>> register_res = 
			this->jit_engine.registerFunc(extern_func.name, func_ptr);
		evo::debugAssert(register_res.has_value(), "registering func pointer to JIT failed");
		

		auto params = evo::SmallVector<Parameter>();
		params.reserve(extern_func.parameters.size());
		for(const Parameter& param : extern_func.parameters){
			params.emplace_back(
				std::string(param.getName()), this->convert_type_from_module_to_jit_engine_module(param.getType())
			);
		}

		const Type return_type = this->convert_type_from_module_to_jit_engine_module(extern_func.returnType);

		const ExternalFunction::ID created_extern_func_id = this->jit_engine_module.createExternalFunction(
			evo::copy(extern_func.name), std::move(params), CallingConvention::C, Linkage::EXTERNAL, return_type
		);
		const ExternalFunction& created_extern_func =
			this->jit_engine_module.getExternalFunction(created_extern_func_id);

		auto handler = InstrHandler(this->jit_engine_module, this->jit_engine_module.getFunction(created_func_id));

		const BasicBlock::ID created_func_entry_basic_block = handler.createBasicBlock();
		handler.setTargetBasicBlock(created_func_entry_basic_block);

		auto args = evo::SmallVector<Expr>();
		args.reserve(created_extern_func.parameters.size());
		for(size_t i = 0; const Parameter& param : created_extern_func.parameters){
			const Expr arg_ptr = handler.createCalcPtr(
				handler.createParamExpr(0),
				this->jit_engine_module.createPtrType(),
				evo::SmallVector<CalcPtr::Index>{int32_t(i)}
			);

			args.emplace_back(handler.createLoad(arg_ptr, param.getType()));
			
			i += 1;
		}


		if(return_type.kind() == Type::Kind::VOID){
			handler.createCallVoid(created_extern_func_id, std::move(args));
		}else{
			const Expr call_ret_val = handler.createCall(created_extern_func_id, std::move(args));
			handler.createStore(handler.createParamExpr(1), call_ret_val);
		}

		handler.createRet();

		const evo::Expected<void, evo::SmallVector<std::string>> add_res = 
			this->jit_engine.addModuleSubsetWithWeakDependencies(
				this->jit_engine_module, JITEngine::ModuleSubsets{ .funcs = created_func_id }
			);
		evo::debugAssert(add_res.has_value(), "lowering JIT interface for extern func failed");

		const ExternFuncPtr created_func_ptr = this->jit_engine.getSymbol<ExternFuncPtr>(
			this->jit_engine_module.getFunction(created_func_id).getName()
		);


		{
			const auto lock = std::scoped_lock(this->registered_extern_func_lookup_lock);
			this->registered_extern_func_lookup.emplace(extern_func_id, created_func_ptr);
		}
	}





	auto ExecutionEngine::get_current_executor() -> Executor& {
		const std::thread::id this_id = std::this_thread::get_id();

		const auto lock = std::scoped_lock(this->executors_lock);

		const auto find = this->executors.find(this_id);
		if(find != this->executors.end()){ return find->second; }

		Executor& created_executor = this->executors_alloc.emplace_back(*this);
		this->executors.emplace(this_id, created_executor);
		return created_executor;
	}



	auto ExecutionEngine::run_debugger(Executor& executor)
	-> std::optional<evo::Expected<core::GenericValue, FuncRunError::Code>> {
		const auto lock = std::scoped_lock(this->debugger_lock);

		if(static_cast<bool>(this->debugger_func) == false){ return std::nullopt; }

		auto debugger_interface = ExecutionEngineDebuggerInterface(executor);
		return this->debugger_func(debugger_interface, this->module);
	}




	auto ExecutionEngine::convert_type_from_module_to_jit_engine_module(Type module_type) -> Type {
		switch(module_type.kind()){
			case Type::Kind::VOID: case Type::Kind::UNSIGNED: case Type::Kind::SIGNED:
			case Type::Kind::BOOL: case Type::Kind::FLOAT:    case Type::Kind::PTR: {
				return module_type;
			} break;

			case Type::Kind::ARRAY: {
				const ArrayType& array_type = this->module.getArrayType(module_type);
				return this->jit_engine_module.getOrCreateArrayType(array_type.elemType, array_type.length);
			} break;

			case Type::Kind::STRUCT: {
				const auto value_handle = this->type_conv_lookup.get(module_type);

				if(value_handle.needsToBeSet() == false){
					return value_handle.getValue();
				}

				const StructType& struct_type = this->module.getStructType(module_type);

				const Type created_struct_type = this->jit_engine_module.createStructType(
					evo::copy(struct_type.name),
					evo::copy(struct_type.members),
					struct_type.alignment,
					struct_type.isPacked
				);

				value_handle.emplaceValue(created_struct_type);
				return created_struct_type;
			} break;

			case Type::Kind::FUNCTION: {
				const FunctionType& function_type = this->module.getFunctionType(module_type);
				return this->jit_engine_module.getOrCreateFunctionType(
					evo::copy(function_type.parameters), function_type.callingConvention, function_type.returnType
				);
			} break;
		}

		evo::unreachable();
	}


}