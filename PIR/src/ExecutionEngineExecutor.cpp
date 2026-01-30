////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./ExecutionEngineExecutor.h"

#include "../include/ExecutionEngine.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::pir{

	static EVO_NODISCARD auto generic_value_to_float(const core::GenericValue& generic_value, Type type)
	-> core::GenericFloat {
		if(type.kind() == Type::Kind::BFLOAT){
			return generic_value.getBF16();
		}else{
			switch(type.getWidth()){
				case 16:  return generic_value.getF16();
				case 32:  return generic_value.getF32();
				case 64:  return generic_value.getF64();
				case 80:  return generic_value.getF80();
				case 128: return generic_value.getF128();
			}
			evo::debugFatalBreak("Unknown or unsupported float width");
		}
	}



	auto ExecutionEngineExecutor::runFunction(Function::ID func_id, std::span<core::GenericValue> arguments)
	-> evo::Expected<core::GenericValue, FuncRunError> {
		const Function& func = this->engine.module.getFunction(func_id);
		const BasicBlock::ID basic_block_id = *func.begin();

		auto reader_agent = ReaderAgent(this->engine.module, func);

		StackFrame& stack_frame = this->stack_frames.emplace_back(
			func_id, basic_block_id, reader_agent, &reader_agent.getBasicBlock(basic_block_id)
		);

		for(size_t i = 0; core::GenericValue& arg : arguments){
			if(func.getParameters()[i].getType().kind() == Type::Kind::PTR){
				stack_frame.params.emplace_back(arg.getPtr<std::byte*>());
			}else{
				stack_frame.params.emplace_back(arg.writableDataRange().data());
			}

			i += 1;
		}

		this->setup_allocas(stack_frame);

		#if !defined(EVO_PLATFORM_WINDOWS)
			if(setjmp(this->jump_buf)){
				return evo::Unexpected(*this->signal_error.load());
			}
		#endif

		return this->run_function_impl();
	}


	#if !defined(EVO_PLATFORM_WINDOWS)
		auto ExecutionEngineExecutor::set_signal_error(FuncRunError error) -> std::jmp_buf& {
			this->signal_error.store(error);
			return this->jump_buf;
		}
	#endif



	auto ExecutionEngineExecutor::run_function_impl() -> evo::Expected<core::GenericValue, FuncRunError> {
		StackFrame* stack_frame = &this->stack_frames.back();
		
		while(true){
			const Expr expr = stack_frame->current_basic_block->operator[](stack_frame->instruction_index);

			switch(expr.kind()){
				case Expr::Kind::NONE: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::GLOBAL_VALUE: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::FUNCTION_POINTER: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::NUMBER: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::BOOLEAN: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::PARAM_EXPR: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::NULLPTR: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::CALL: {
					const Call& call_inst = stack_frame->reader_agent.getCall(expr);

					stack_frame->ret_value = expr;
					stack_frame->instruction_index += 1;

					if(call_inst.target.is<Function::ID>()){
						const Function::ID func_id = call_inst.target.as<Function::ID>();
						const Function& func = this->engine.module.getFunction(func_id);
						const BasicBlock::ID basic_block_id = *func.begin();

						auto reader_agent = ReaderAgent(this->engine.module, func);

						auto params = evo::SmallVector<std::byte*>();
						for(const Expr arg : call_inst.args){
							core::GenericValue* generic_value = this->get_expr_maybe_ptr(arg, *stack_frame);

							if(generic_value == nullptr){
								params.emplace_back(nullptr);

							}else if(stack_frame->reader_agent.getExprType(arg).kind() == Type::Kind::PTR){
								params.emplace_back(generic_value->getPtr<std::byte*>());

							}else{
								params.emplace_back(generic_value->writableDataRange().data());
							}
						}

						stack_frame = &this->stack_frames.emplace_back(
							func_id, basic_block_id, reader_agent, &reader_agent.getBasicBlock(basic_block_id)
						);

						stack_frame->params = std::move(params);

						this->setup_allocas(*stack_frame);
						
					}else if(call_inst.target.is<ExternalFunction::ID>()){
						// TODO(FUTURE): 
						evo::unimplemented("Calling external function");

					}else{
						evo::debugAssert(call_inst.target.is<PtrCall>(), "Unknown func call kind");
						// TODO(FUTURE): 
						evo::unimplemented("Ptr call");
					}

					continue;
				} break;

				case Expr::Kind::CALL_VOID: {
					const CallVoid& call_void_inst = stack_frame->reader_agent.getCallVoid(expr);

					stack_frame->instruction_index += 1;

					if(call_void_inst.target.is<Function::ID>()){
						const Function::ID func_id = call_void_inst.target.as<Function::ID>();
						const Function& func = this->engine.module.getFunction(func_id);
						const BasicBlock::ID basic_block_id = *func.begin();

						auto reader_agent = ReaderAgent(this->engine.module, func);

						auto params = evo::SmallVector<std::byte*>();
						for(const Expr arg : call_void_inst.args){
							core::GenericValue* generic_value = this->get_expr_maybe_ptr(arg, *stack_frame);

							if(generic_value == nullptr){
								params.emplace_back(nullptr);

							}else if(stack_frame->reader_agent.getExprType(arg).kind() == Type::Kind::PTR){
								params.emplace_back(generic_value->getPtr<std::byte*>());

							}else{
								params.emplace_back(generic_value->writableDataRange().data());
							}
						}

						stack_frame = &this->stack_frames.emplace_back(
							func_id, basic_block_id, reader_agent, &reader_agent.getBasicBlock(basic_block_id)
						);

						stack_frame->params = std::move(params);

						this->setup_allocas(*stack_frame);
						
					}else if(call_void_inst.target.is<ExternalFunction::ID>()){
						// TODO(FUTURE): 
						evo::unimplemented("Calling external function");

					}else{
						evo::debugAssert(call_void_inst.target.is<PtrCall>(), "Unknown func call kind");
						// TODO(FUTURE): 
						evo::unimplemented("Ptr call");
					}

					continue;
				} break;

				case Expr::Kind::ABORT: {
					return evo::Unexpected(FuncRunError::ABORT);
				} break;

				case Expr::Kind::BREAKPOINT: {
					return evo::Unexpected(FuncRunError::ABORT);
				} break;

				case Expr::Kind::RET: {
					const Ret& ret_inst = stack_frame->reader_agent.getRet(expr);

					if(ret_inst.value.has_value()){
						core::GenericValue ret_value = this->get_expr(*ret_inst.value, *stack_frame);

						this->stack_frames.pop_back();

						if(this->stack_frames.empty()){
							return ret_value;
						}

						stack_frame = &this->stack_frames.back();
						stack_frame->registers[*stack_frame->ret_value] = std::move(ret_value);

					}else{
						this->stack_frames.pop_back();

						if(this->stack_frames.empty()){
							return core::GenericValue();
						}

						stack_frame = &this->stack_frames.back();
					}

					continue;
				} break;

				case Expr::Kind::JUMP: {
					const Jump& jump_inst = stack_frame->reader_agent.getJump(expr);

					stack_frame->previous_basic_block_id = stack_frame->current_basic_block_id;
					stack_frame->current_basic_block_id = jump_inst.target;
					stack_frame->instruction_index = 0;
					stack_frame->current_basic_block =
						&stack_frame->reader_agent.getBasicBlock(stack_frame->current_basic_block_id);

					continue;
				} break;

				case Expr::Kind::BRANCH: {
					const Branch& branch_inst = stack_frame->reader_agent.getBranch(expr);

					const core::GenericValue& cond = this->get_expr(branch_inst.cond, *stack_frame);

					stack_frame->previous_basic_block_id = stack_frame->current_basic_block_id;

					if(cond.getBool()){
						stack_frame->current_basic_block_id = branch_inst.thenBlock;
					}else{
						stack_frame->current_basic_block_id = branch_inst.elseBlock;
					}

					stack_frame->instruction_index = 0;
					stack_frame->current_basic_block =
						&stack_frame->reader_agent.getBasicBlock(stack_frame->current_basic_block_id);

					continue;
				} break;

				case Expr::Kind::UNREACHABLE: {
					return evo::Unexpected(FuncRunError::UNKNOWN_EXCEPTION);
				} break;

				case Expr::Kind::PHI: {
					const Phi& phi_inst = stack_frame->reader_agent.getPhi(expr);

					bool found = false;
					for(const Phi::Predecessor& predecessor : phi_inst.predecessors){
						if(predecessor.block == stack_frame->previous_basic_block_id){
							stack_frame->registers[expr] = this->get_expr(predecessor.value, *stack_frame);

							found = true;
							break;
						}
					}

					if(found == false){ return evo::Unexpected(FuncRunError::UNKNOWN_EXCEPTION); }
				} break;

				case Expr::Kind::SWITCH: {
					const Switch& switch_inst = stack_frame->reader_agent.getSwitch(expr);

					const Type expr_pir_type = stack_frame->reader_agent.getExprType(switch_inst.cond);
					const size_t expr_size = this->engine.module.getSize(expr_pir_type, false);

					const core::GenericValue& generic_cond = this->get_expr(switch_inst.cond, *stack_frame);
					const core::GenericInt cond = generic_cond.getInt(unsigned(expr_size * 8));

					stack_frame->previous_basic_block_id = stack_frame->current_basic_block_id;

					bool found = false;
					for(const Switch::Case& switch_case : switch_inst.cases){
						const core::GenericValue& generic_case_value = this->get_expr(switch_case.value, *stack_frame);
						const core::GenericInt case_value = generic_case_value.getInt(unsigned(expr_size * 8));

						if(cond.eq(case_value)){
							stack_frame->current_basic_block_id = switch_case.block;

							found = true;
							break;
						}
					}

					if(found == false){
						stack_frame->current_basic_block_id = switch_inst.defaultBlock;
					}

					stack_frame->instruction_index = 0;
					stack_frame->current_basic_block =
						&stack_frame->reader_agent.getBasicBlock(stack_frame->current_basic_block_id);

					continue;
				} break;

				case Expr::Kind::ALLOCA: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::LOAD: {
					const Load& load_inst = stack_frame->reader_agent.getLoad(expr);

					const size_t type_size = this->engine.module.getSize(load_inst.type);

					const std::byte* source_ptr = this->get_expr_ptr(load_inst.source, *stack_frame);
					if(source_ptr == nullptr){ return evo::Unexpected(FuncRunError::NULLPTR_ACCESS); }

					const auto data_proxy = evo::ArrayProxy<std::byte>(source_ptr, type_size);

					#if defined(EVO_PLATFORM_WINDOWS)
						auto generic_value = core::GenericValue();
						__try{
							generic_value = core::GenericValue::fromData(data_proxy);
						}__except(0x1){
							return evo::Unexpected(FuncRunError::UNKNOWN_EXCEPTION);
						}

						stack_frame->registers[expr] = std::move(generic_value);

					#else
						stack_frame->registers[expr] = core::GenericValue::fromData(data_proxy);
					#endif
				} break;

				case Expr::Kind::STORE: {
					const Store& store_inst = stack_frame->reader_agent.getStore(expr);

					const Type store_type = stack_frame->reader_agent.getExprType(store_inst.value);
					const size_t store_type_size = this->engine.module.getSize(store_type);

					std::byte* destination_ptr = this->get_expr_ptr(store_inst.destination, *stack_frame);
					if(destination_ptr == nullptr){ return evo::Unexpected(FuncRunError::NULLPTR_ACCESS); }

					core::GenericValue* const value_ptr_generic =
						this->get_expr_maybe_ptr(store_inst.value, *stack_frame);

					if(value_ptr_generic != nullptr){
						const std::byte* const value_ptr = value_ptr_generic->dataRange().data();

						#if defined(EVO_PLATFORM_WINDOWS)
							__try{
								std::memmove(destination_ptr, value_ptr, store_type_size);
							}__except(0x1){
								return evo::Unexpected(FuncRunError::UNKNOWN_EXCEPTION);
							}

						#else
							std::memmove(destination_ptr, value_ptr, store_type_size);
						#endif

					}else{
						size_t* dst_pointer_full = reinterpret_cast<size_t*>(destination_ptr);

						#if defined(EVO_PLATFORM_WINDOWS)
							__try{
								*dst_pointer_full = 0;
							}__except(0x1){
								return evo::Unexpected(FuncRunError::UNKNOWN_EXCEPTION);
							}

						#else
							*dst_pointer_full = 0;
						#endif
					}
				} break;

				case Expr::Kind::CALC_PTR: {
					const CalcPtr& calc_ptr_inst = stack_frame->reader_agent.getCalcPtr(expr);
					
					Type ptr_type = calc_ptr_inst.ptrType;

					std::byte* ptr = this->get_expr_ptr(calc_ptr_inst.basePtr, *stack_frame);
					if(ptr == nullptr){ return evo::Unexpected(FuncRunError::NULLPTR_ACCESS); }

					const auto get_index = [&](const CalcPtr::Index& index) -> int64_t {
						if(index.is<Expr>()){
							return static_cast<int64_t>(this->get_expr(index.as<Expr>(), *stack_frame).getInt(64));
						}else{
							return index.as<int64_t>();
						}
					};


					{
						const size_t ptr_type_size = this->engine.module.getSize(ptr_type);
						ptr += get_index(calc_ptr_inst.indices[0]) * ptr_type_size;
					}


					for(size_t i = 1; i < calc_ptr_inst.indices.size(); i+=1){
						const int64_t index = get_index(calc_ptr_inst.indices[i]);

						if(ptr_type.kind() == Type::Kind::ARRAY){
							if(index < 0){ return evo::Unexpected(FuncRunError::OUT_OF_BOUNDS_ACCESS); }

							const ArrayType& array_type = this->engine.module.getArrayType(ptr_type);
							if(uint64_t(index) + 1 > array_type.length){
								return evo::Unexpected(FuncRunError::OUT_OF_BOUNDS_ACCESS);
							}

							ptr_type = array_type.elemType;

							const size_t ptr_type_size = this->engine.module.getSize(ptr_type);
							ptr += index * ptr_type_size;

						}else if(ptr_type.kind() == Type::Kind::STRUCT){
							if(index < 0){ return evo::Unexpected(FuncRunError::UNKNOWN_EXCEPTION); }

							const StructType& struct_type = this->engine.module.getStructType(ptr_type);
							if(uint64_t(index) + 1 > struct_type.members.size()){
								return evo::Unexpected(FuncRunError::UNKNOWN_EXCEPTION);
							}

							size_t offset = 0;
							for(size_t j = 0; j < size_t(index); j+=1){
								offset += this->engine.module.getSize(struct_type.members[j]);
							}

							ptr_type = struct_type.members[index];
							ptr += offset;

						}else{
							const size_t ptr_type_size = this->engine.module.getSize(ptr_type);
							ptr += index * ptr_type_size;
						}
					}

					stack_frame->registers[expr] = core::GenericValue::createPtr(ptr);
				} break;

				case Expr::Kind::MEMCPY: {
					const Memcpy& memcpy_inst = stack_frame->reader_agent.getMemcpy(expr);

					std::byte* dst_ptr = this->get_expr_ptr(memcpy_inst.dst, *stack_frame);
					if(dst_ptr == nullptr){ return evo::Unexpected(FuncRunError::NULLPTR_ACCESS); }

					std::byte* src_ptr = this->get_expr_ptr(memcpy_inst.src, *stack_frame);
					if(src_ptr == nullptr){ return evo::Unexpected(FuncRunError::NULLPTR_ACCESS); }

					const core::GenericValue& num_bytes_generic = this->get_expr(memcpy_inst.numBytes, *stack_frame);
					const size_t num_bytes = static_cast<uint64_t>(num_bytes_generic.getInt(sizeof(size_t) * 8));

					#if defined(EVO_PLATFORM_WINDOWS)
						__try{
							std::memcpy(dst_ptr, src_ptr, num_bytes);
						}__except(0x1){
							return evo::Unexpected(FuncRunError::UNKNOWN_EXCEPTION);
						}

					#else
						std::memcpy(dst_ptr, src_ptr, num_bytes);
					#endif
				} break;

				case Expr::Kind::MEMSET: {
					const Memset& memset_inst = stack_frame->reader_agent.getMemset(expr);

					std::byte* dst_ptr = this->get_expr_ptr(memset_inst.dst, *stack_frame);
					if(dst_ptr == nullptr){ return evo::Unexpected(FuncRunError::NULLPTR_ACCESS); }

					const core::GenericValue& value_generic = this->get_expr(memset_inst.value, *stack_frame);
					const unsigned char value = static_cast<unsigned char>(value_generic.getInt(8));

					const core::GenericValue& num_bytes_generic = this->get_expr(memset_inst.numBytes, *stack_frame);
					const size_t num_bytes = static_cast<uint64_t>(num_bytes_generic.getInt(sizeof(size_t) * 8));

					#if defined(EVO_PLATFORM_WINDOWS)
						__try{
							std::memset(dst_ptr, value, num_bytes);
						}__except(0x1){
							return evo::Unexpected(FuncRunError::UNKNOWN_EXCEPTION);
						}

					#else
						std::memset(dst_ptr, value, num_bytes);
					#endif
				} break;

				case Expr::Kind::BIT_CAST: {
					const BitCast& bit_cast_inst = stack_frame->reader_agent.getBitCast(expr);

					stack_frame->registers[expr] = this->get_expr(bit_cast_inst.fromValue, *stack_frame);
				} break;

				case Expr::Kind::TRUNC: {
					const Trunc& trunc_inst = stack_frame->reader_agent.getTrunc(expr);

					const Type from_type = stack_frame->reader_agent.getExprType(trunc_inst.fromValue);

					const core::GenericValue& generic_value = this->get_expr(trunc_inst.fromValue, *stack_frame);
					const core::GenericInt int_value = generic_value.getInt(from_type.getWidth());

					stack_frame->registers[expr] = core::GenericValue(int_value.trunc(trunc_inst.toType.getWidth()));
				} break;

				case Expr::Kind::FTRUNC: {
					const FTrunc& ftrunc_inst = stack_frame->reader_agent.getFTrunc(expr);

					const Type from_type = stack_frame->reader_agent.getExprType(ftrunc_inst.fromValue);

					const core::GenericValue& generic_value = this->get_expr(ftrunc_inst.fromValue, *stack_frame);
					const core::GenericFloat float_value = generic_value_to_float(generic_value, from_type);

					core::GenericFloat output_float = [&]() -> core::GenericFloat {
						if(ftrunc_inst.toType.kind() == Type::Kind::BFLOAT){
							return float_value.asBF16();
						}else{
							switch(ftrunc_inst.toType.getWidth()){
								case 16:  return float_value.asF16();
								case 32:  return float_value.asF32();
								case 64:  return float_value.asF64();
								case 80:  return float_value.asF80();
								case 128: return float_value.asF128();
							}
							evo::debugFatalBreak("Unknown or unsupported float width");
						}
					}();

					stack_frame->registers[expr] = core::GenericValue(std::move(output_float));
				} break;

				case Expr::Kind::SEXT: {
					const SExt& sext_inst = stack_frame->reader_agent.getSExt(expr);

					const Type from_type = stack_frame->reader_agent.getExprType(sext_inst.fromValue);

					const core::GenericValue& generic_value = this->get_expr(sext_inst.fromValue, *stack_frame);
					const core::GenericInt int_value = generic_value.getInt(from_type.getWidth());

					stack_frame->registers[expr] = core::GenericValue(int_value.sext(sext_inst.toType.getWidth()));
				} break;

				case Expr::Kind::ZEXT: {
					const ZExt& zext_inst = stack_frame->reader_agent.getZExt(expr);

					const Type from_type = stack_frame->reader_agent.getExprType(zext_inst.fromValue);

					const unsigned from_type_size = [&]() -> unsigned {
						if(from_type.kind() == Type::Kind::BOOL){
							return 1;
						}else{
							return unsigned(from_type.getWidth());
						}
					}();

					const core::GenericValue& generic_value = this->get_expr(zext_inst.fromValue, *stack_frame);
					const core::GenericInt int_value = generic_value.getInt(from_type_size);

					stack_frame->registers[expr] = core::GenericValue(int_value.zext(zext_inst.toType.getWidth()));
				} break;

				case Expr::Kind::FEXT: {
					const FExt& fext_inst = stack_frame->reader_agent.getFExt(expr);

					const Type from_type = stack_frame->reader_agent.getExprType(fext_inst.fromValue);

					const core::GenericValue& generic_value = this->get_expr(fext_inst.fromValue, *stack_frame);
					const core::GenericFloat float_value = generic_value_to_float(generic_value, from_type);

					core::GenericFloat output_float = [&]() -> core::GenericFloat {
						if(fext_inst.toType.kind() == Type::Kind::BFLOAT){
							return float_value.asBF16();
						}else{
							switch(fext_inst.toType.getWidth()){
								case 16:  return float_value.asF16();
								case 32:  return float_value.asF32();
								case 64:  return float_value.asF64();
								case 80:  return float_value.asF80();
								case 128: return float_value.asF128();
							}
							evo::debugFatalBreak("Unknown or unsupported float width");
						}
					}();

					stack_frame->registers[expr] = core::GenericValue(std::move(output_float));
				} break;

				case Expr::Kind::ITOF: {
					const IToF& itof_inst = stack_frame->reader_agent.getIToF(expr);

					const Type from_type = stack_frame->reader_agent.getExprType(itof_inst.fromValue);

					const core::GenericValue& generic_value = this->get_expr(itof_inst.fromValue, *stack_frame);
					const core::GenericInt int_value = generic_value.getInt(from_type.getWidth());

					core::GenericFloat output_float = [&]() -> core::GenericFloat {
						if(itof_inst.toType.kind() == Type::Kind::BFLOAT){
							return core::GenericFloat::createBF16FromInt(int_value, true);
						}else{
							switch(itof_inst.toType.getWidth()){
								case 16:  return core::GenericFloat::createF16FromInt(int_value, true);
								case 32:  return core::GenericFloat::createF32FromInt(int_value, true);
								case 64:  return core::GenericFloat::createF64FromInt(int_value, true);
								case 80:  return core::GenericFloat::createF80FromInt(int_value, true);
								case 128: return core::GenericFloat::createF128FromInt(int_value, true);
							}
							evo::debugFatalBreak("Unknown or unsupported float width");
						}
					}();

					stack_frame->registers[expr] = core::GenericValue(std::move(output_float));
				} break;

				case Expr::Kind::UITOF: {
					const IToF& itof_inst = stack_frame->reader_agent.getIToF(expr);

					const Type from_type = stack_frame->reader_agent.getExprType(itof_inst.fromValue);

					const core::GenericValue& generic_value = this->get_expr(itof_inst.fromValue, *stack_frame);
					const core::GenericInt int_value = generic_value.getInt(from_type.getWidth());

					core::GenericFloat output_float = [&]() -> core::GenericFloat {
						if(itof_inst.toType.kind() == Type::Kind::BFLOAT){
							return core::GenericFloat::createBF16FromInt(int_value, false);
						}else{
							switch(itof_inst.toType.getWidth()){
								case 16:  return core::GenericFloat::createF16FromInt(int_value, false);
								case 32:  return core::GenericFloat::createF32FromInt(int_value, false);
								case 64:  return core::GenericFloat::createF64FromInt(int_value, false);
								case 80:  return core::GenericFloat::createF80FromInt(int_value, false);
								case 128: return core::GenericFloat::createF128FromInt(int_value, false);
							}
							evo::debugFatalBreak("Unknown or unsupported float width");
						}
					}();

					stack_frame->registers[expr] = core::GenericValue(std::move(output_float));
				} break;

				case Expr::Kind::FTOI: {
					const FToI& ftoi_inst = stack_frame->reader_agent.getFToI(expr);

					const Type from_type = stack_frame->reader_agent.getExprType(ftoi_inst.fromValue);

					const core::GenericValue& generic_value = this->get_expr(ftoi_inst.fromValue, *stack_frame);
					const core::GenericFloat float_value = generic_value_to_float(generic_value, from_type);

					stack_frame->registers[expr] =
						core::GenericValue(float_value.toGenericInt(ftoi_inst.toType.getWidth(), true));
				} break;

				case Expr::Kind::FTOUI: {
					const FToI& ftoi_inst = stack_frame->reader_agent.getFToI(expr);

					const Type from_type = stack_frame->reader_agent.getExprType(ftoi_inst.fromValue);

					const core::GenericValue& generic_value = this->get_expr(ftoi_inst.fromValue, *stack_frame);
					const core::GenericFloat float_value = generic_value_to_float(generic_value, from_type);

					stack_frame->registers[expr] =
						core::GenericValue(float_value.toGenericInt(ftoi_inst.toType.getWidth(), false));
				} break;

				case Expr::Kind::ADD: {
					const Add& add_inst = stack_frame->reader_agent.getAdd(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(add_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(add_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(add_inst.rhs, *stack_frame).getInt(num_bits);

					if(add_inst.nsw){
						if(add_inst.nuw){
							core::GenericInt::WrapResult unsigned_result = lhs.uadd(rhs);
							if(unsigned_result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }

							core::GenericInt::WrapResult signed_result = lhs.sadd(rhs);
							if(signed_result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }

							stack_frame->registers[expr] = core::GenericValue(std::move(unsigned_result.result));	

						}else{
							core::GenericInt::WrapResult result = lhs.sadd(rhs);
							if(result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }
							stack_frame->registers[expr] = core::GenericValue(std::move(result.result));
						}
						
					}else if(add_inst.nuw){
						core::GenericInt::WrapResult result = lhs.uadd(rhs);
						if(result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }
						stack_frame->registers[expr] = core::GenericValue(std::move(result.result));
						
					}else{
						stack_frame->registers[expr] = core::GenericValue(lhs.uadd(rhs).result);
					}
				} break;

				case Expr::Kind::SADD_WRAP: {
					const SAddWrap& sadd_wrap_inst = stack_frame->reader_agent.getSAddWrap(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(sadd_wrap_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(sadd_wrap_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(sadd_wrap_inst.rhs, *stack_frame).getInt(num_bits);

					const core::GenericInt::WrapResult result = lhs.sadd(rhs);

					stack_frame->registers[ReaderAgent::extractSAddWrapResult(expr)] =
						core::GenericValue(result.result);

					stack_frame->registers[ReaderAgent::extractSAddWrapWrapped(expr)] =
						core::GenericValue(result.wrapped);
				} break;

				case Expr::Kind::SADD_WRAP_RESULT: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::SADD_WRAP_WRAPPED: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::UADD_WRAP: {
					const UAddWrap& sadd_wrap_inst = stack_frame->reader_agent.getUAddWrap(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(sadd_wrap_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(sadd_wrap_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(sadd_wrap_inst.rhs, *stack_frame).getInt(num_bits);

					const core::GenericInt::WrapResult result = lhs.uadd(rhs);

					stack_frame->registers[ReaderAgent::extractUAddWrapResult(expr)] =
						core::GenericValue(result.result);

					stack_frame->registers[ReaderAgent::extractUAddWrapWrapped(expr)] =
						core::GenericValue(result.wrapped);
				} break;

				case Expr::Kind::UADD_WRAP_RESULT: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::UADD_WRAP_WRAPPED: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::SADD_SAT: {
					const SAddSat& sadd_sat_inst = stack_frame->reader_agent.getSAddSat(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(sadd_sat_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(sadd_sat_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(sadd_sat_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.saddSat(rhs));
				} break;

				case Expr::Kind::UADD_SAT: {
					const UAddSat& uadd_sat_inst = stack_frame->reader_agent.getUAddSat(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(uadd_sat_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(uadd_sat_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(uadd_sat_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.uaddSat(rhs));
				} break;

				case Expr::Kind::FADD: {
					const FAdd& fadd_inst = stack_frame->reader_agent.getFAdd(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(fadd_inst.lhs);

					const core::GenericFloat lhs =
						generic_value_to_float(this->get_expr(fadd_inst.lhs, *stack_frame), arg_type);
					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(fadd_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(lhs.add(rhs));
				} break;

				case Expr::Kind::SUB: {
					const Sub& sub_inst = stack_frame->reader_agent.getSub(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(sub_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(sub_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(sub_inst.rhs, *stack_frame).getInt(num_bits);

					if(sub_inst.nsw){
						if(sub_inst.nuw){
							core::GenericInt::WrapResult unsigned_result = lhs.usub(rhs);
							if(unsigned_result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }

							core::GenericInt::WrapResult signed_result = lhs.ssub(rhs);
							if(signed_result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }

							stack_frame->registers[expr] = core::GenericValue(std::move(unsigned_result.result));	

						}else{
							core::GenericInt::WrapResult result = lhs.ssub(rhs);
							if(result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }
							stack_frame->registers[expr] = core::GenericValue(std::move(result.result));
						}
						
					}else if(sub_inst.nuw){
						core::GenericInt::WrapResult result = lhs.usub(rhs);
						if(result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }
						stack_frame->registers[expr] = core::GenericValue(std::move(result.result));
						
					}else{
						stack_frame->registers[expr] = core::GenericValue(lhs.usub(rhs).result);
					}
				} break;

				case Expr::Kind::SSUB_WRAP: {
					const SSubWrap& ssub_wrap_inst = stack_frame->reader_agent.getSSubWrap(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ssub_wrap_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(ssub_wrap_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(ssub_wrap_inst.rhs, *stack_frame).getInt(num_bits);

					const core::GenericInt::WrapResult result = lhs.ssub(rhs);

					stack_frame->registers[ReaderAgent::extractSSubWrapResult(expr)] =
						core::GenericValue(result.result);

					stack_frame->registers[ReaderAgent::extractSSubWrapWrapped(expr)] =
						core::GenericValue(result.wrapped);
				} break;

				case Expr::Kind::SSUB_WRAP_RESULT: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::SSUB_WRAP_WRAPPED: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::USUB_WRAP: {
					const USubWrap& ssub_wrap_inst = stack_frame->reader_agent.getUSubWrap(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ssub_wrap_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(ssub_wrap_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(ssub_wrap_inst.rhs, *stack_frame).getInt(num_bits);

					const core::GenericInt::WrapResult result = lhs.usub(rhs);

					stack_frame->registers[ReaderAgent::extractUSubWrapResult(expr)] =
						core::GenericValue(result.result);

					stack_frame->registers[ReaderAgent::extractUSubWrapWrapped(expr)] =
						core::GenericValue(result.wrapped);
				} break;

				case Expr::Kind::USUB_WRAP_RESULT: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::USUB_WRAP_WRAPPED: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::SSUB_SAT: {
					const SSubSat& ssub_sat_inst = stack_frame->reader_agent.getSSubSat(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ssub_sat_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(ssub_sat_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(ssub_sat_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.ssubSat(rhs));
				} break;

				case Expr::Kind::USUB_SAT: {
					const USubSat& usub_sat_inst = stack_frame->reader_agent.getUSubSat(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(usub_sat_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(usub_sat_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(usub_sat_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.usubSat(rhs));
				} break;

				case Expr::Kind::FSUB: {
					const FSub& fsub_inst = stack_frame->reader_agent.getFSub(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(fsub_inst.lhs);

					const core::GenericFloat lhs =
						generic_value_to_float(this->get_expr(fsub_inst.lhs, *stack_frame), arg_type);
					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(fsub_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(lhs.sub(rhs));
				} break;

				case Expr::Kind::MUL: {
					const Mul& mul_inst = stack_frame->reader_agent.getMul(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(mul_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(mul_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(mul_inst.rhs, *stack_frame).getInt(num_bits);

					if(mul_inst.nsw){
						if(mul_inst.nuw){
							core::GenericInt::WrapResult unsigned_result = lhs.umul(rhs);
							if(unsigned_result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }

							core::GenericInt::WrapResult signed_result = lhs.smul(rhs);
							if(signed_result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }

							stack_frame->registers[expr] = core::GenericValue(std::move(unsigned_result.result));	

						}else{
							core::GenericInt::WrapResult result = lhs.smul(rhs);
							if(result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }
							stack_frame->registers[expr] = core::GenericValue(std::move(result.result));
						}
						
					}else if(mul_inst.nuw){
						core::GenericInt::WrapResult result = lhs.umul(rhs);
						if(result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }
						stack_frame->registers[expr] = core::GenericValue(std::move(result.result));
						
					}else{
						stack_frame->registers[expr] = core::GenericValue(lhs.umul(rhs).result);
					}
				} break;

				case Expr::Kind::SMUL_WRAP: {
					const SMulWrap& smul_wrap_inst = stack_frame->reader_agent.getSMulWrap(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(smul_wrap_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(smul_wrap_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(smul_wrap_inst.rhs, *stack_frame).getInt(num_bits);

					const core::GenericInt::WrapResult result = lhs.smul(rhs);

					stack_frame->registers[ReaderAgent::extractSMulWrapResult(expr)] =
						core::GenericValue(result.result);

					stack_frame->registers[ReaderAgent::extractSMulWrapWrapped(expr)] =
						core::GenericValue(result.wrapped);
				} break;

				case Expr::Kind::SMUL_WRAP_RESULT: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::SMUL_WRAP_WRAPPED: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::UMUL_WRAP: {
					const UMulWrap& umul_wrap_inst = stack_frame->reader_agent.getUMulWrap(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(umul_wrap_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(umul_wrap_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(umul_wrap_inst.rhs, *stack_frame).getInt(num_bits);

					const core::GenericInt::WrapResult result = lhs.umul(rhs);

					stack_frame->registers[ReaderAgent::extractUMulWrapResult(expr)] =
						core::GenericValue(result.result);

					stack_frame->registers[ReaderAgent::extractUMulWrapWrapped(expr)] =
						core::GenericValue(result.wrapped);
				} break;

				case Expr::Kind::UMUL_WRAP_RESULT: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::UMUL_WRAP_WRAPPED: {
					evo::debugFatalBreak("Invalid stmt");
				} break;

				case Expr::Kind::SMUL_SAT: {
					const SMulSat& smul_sat_inst = stack_frame->reader_agent.getSMulSat(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(smul_sat_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(smul_sat_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(smul_sat_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.smulSat(rhs));
				} break;

				case Expr::Kind::UMUL_SAT: {
					const UMulSat& umul_sat_inst = stack_frame->reader_agent.getUMulSat(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(umul_sat_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(umul_sat_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(umul_sat_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.umulSat(rhs));
				} break;

				case Expr::Kind::FMUL: {
					const FMul& fmul_inst = stack_frame->reader_agent.getFMul(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(fmul_inst.lhs);

					const core::GenericFloat lhs =
						generic_value_to_float(this->get_expr(fmul_inst.lhs, *stack_frame), arg_type);
					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(fmul_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(lhs.mul(rhs));
				} break;

				case Expr::Kind::SDIV: {
					const SDiv& sdiv_inst = stack_frame->reader_agent.getSDiv(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(sdiv_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(sdiv_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(sdiv_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.sdiv(rhs));
				} break;

				case Expr::Kind::UDIV: {
					const UDiv& udiv_inst = stack_frame->reader_agent.getUDiv(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(udiv_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(udiv_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(udiv_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.udiv(rhs));
				} break;

				case Expr::Kind::FDIV: {
					const FDiv& fdiv_inst = stack_frame->reader_agent.getFDiv(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(fdiv_inst.lhs);

					const core::GenericFloat lhs =
						generic_value_to_float(this->get_expr(fdiv_inst.lhs, *stack_frame), arg_type);
					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(fdiv_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(lhs.div(rhs));
				} break;

				case Expr::Kind::SREM: {
					const SRem& srem_inst = stack_frame->reader_agent.getSRem(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(srem_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(srem_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(srem_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.srem(rhs));
				} break;

				case Expr::Kind::UREM: {
					const URem& urem_inst = stack_frame->reader_agent.getURem(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(urem_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(urem_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(urem_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.urem(rhs));
				} break;

				case Expr::Kind::FREM: {
					const FRem& frem_inst = stack_frame->reader_agent.getFRem(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(frem_inst.lhs);

					const core::GenericFloat lhs =
						generic_value_to_float(this->get_expr(frem_inst.lhs, *stack_frame), arg_type);
					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(frem_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(lhs.rem(rhs));
				} break;

				case Expr::Kind::FNEG: {
					const FNeg& fneg_inst = stack_frame->reader_agent.getFNeg(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(fneg_inst.rhs);

					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(fneg_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(rhs.neg());
				} break;

				case Expr::Kind::IEQ: {
					const IEq& ieq_inst = stack_frame->reader_agent.getIEq(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ieq_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(ieq_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(ieq_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.eq(rhs));
				} break;

				case Expr::Kind::FEQ: {
					const FEq& feq_inst = stack_frame->reader_agent.getFEq(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(feq_inst.lhs);

					const core::GenericFloat lhs =
						generic_value_to_float(this->get_expr(feq_inst.lhs, *stack_frame), arg_type);
					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(feq_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(lhs.eq(rhs));
				} break;

				case Expr::Kind::INEQ: {
					const INeq& ineq_inst = stack_frame->reader_agent.getINeq(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ineq_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(ineq_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(ineq_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.neq(rhs));
				} break;

				case Expr::Kind::FNEQ: {
					const FNeq& fneq_inst = stack_frame->reader_agent.getFNeq(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(fneq_inst.lhs);

					const core::GenericFloat lhs =
						generic_value_to_float(this->get_expr(fneq_inst.lhs, *stack_frame), arg_type);
					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(fneq_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(lhs.neq(rhs));
				} break;

				case Expr::Kind::SLT: {
					const SLT& slt_inst = stack_frame->reader_agent.getSLT(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(slt_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(slt_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(slt_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.slt(rhs));
				} break;

				case Expr::Kind::ULT: {
					const ULT& ult_inst = stack_frame->reader_agent.getULT(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ult_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(ult_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(ult_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.ult(rhs));
				} break;

				case Expr::Kind::FLT: {
					const FLT& flt_inst = stack_frame->reader_agent.getFLT(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(flt_inst.lhs);

					const core::GenericFloat lhs =
						generic_value_to_float(this->get_expr(flt_inst.lhs, *stack_frame), arg_type);
					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(flt_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(lhs.lt(rhs));
				} break;

				case Expr::Kind::SLTE: {
					const SLTE& slte_inst = stack_frame->reader_agent.getSLTE(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(slte_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(slte_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(slte_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.sle(rhs));
				} break;

				case Expr::Kind::ULTE: {
					const ULTE& ulte_inst = stack_frame->reader_agent.getULTE(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ulte_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(ulte_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(ulte_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.ule(rhs));
				} break;

				case Expr::Kind::FLTE: {
					const FLTE& flt_inst = stack_frame->reader_agent.getFLTE(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(flt_inst.lhs);

					const core::GenericFloat lhs =
						generic_value_to_float(this->get_expr(flt_inst.lhs, *stack_frame), arg_type);
					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(flt_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(lhs.le(rhs));
				} break;

				case Expr::Kind::SGT: {
					const SGT& sgt_inst = stack_frame->reader_agent.getSGT(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(sgt_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(sgt_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(sgt_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.sgt(rhs));
				} break;

				case Expr::Kind::UGT: {
					const UGT& ugt_inst = stack_frame->reader_agent.getUGT(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ugt_inst.lhs);
					const size_t arg_size = this->engine.module.getSize(arg_type);

					const core::GenericInt lhs = this->get_expr(ugt_inst.lhs, *stack_frame).getInt(unsigned(arg_size));
					const core::GenericInt rhs = this->get_expr(ugt_inst.rhs, *stack_frame).getInt(unsigned(arg_size));

					stack_frame->registers[expr] = core::GenericValue(lhs.ugt(rhs));
				} break;

				case Expr::Kind::FGT: {
					const FGT& fgt_inst = stack_frame->reader_agent.getFGT(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(fgt_inst.lhs);

					const core::GenericFloat lhs =
						generic_value_to_float(this->get_expr(fgt_inst.lhs, *stack_frame), arg_type);
					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(fgt_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(lhs.gt(rhs));
				} break;

				case Expr::Kind::SGTE: {
					const SGTE& sgte_inst = stack_frame->reader_agent.getSGTE(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(sgte_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(sgte_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(sgte_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.sge(rhs));
				} break;

				case Expr::Kind::UGTE: {
					const UGTE& ugte_inst = stack_frame->reader_agent.getUGTE(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ugte_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(ugte_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(ugte_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.uge(rhs));
				} break;

				case Expr::Kind::FGTE: {
					const FGTE& fgt_inst = stack_frame->reader_agent.getFGTE(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(fgt_inst.lhs);

					const core::GenericFloat lhs =
						generic_value_to_float(this->get_expr(fgt_inst.lhs, *stack_frame), arg_type);
					const core::GenericFloat rhs =
						generic_value_to_float(this->get_expr(fgt_inst.rhs, *stack_frame), arg_type);

					stack_frame->registers[expr] = core::GenericValue(lhs.ge(rhs));
				} break;

				case Expr::Kind::AND: {
					const And& and_inst = stack_frame->reader_agent.getAnd(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(and_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(and_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(and_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.bitwiseAnd(rhs));
				} break;

				case Expr::Kind::OR: {
					const Or& or_inst = stack_frame->reader_agent.getOr(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(or_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(or_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(or_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.bitwiseOr(rhs));
				} break;

				case Expr::Kind::XOR: {
					const Xor& xor_inst = stack_frame->reader_agent.getXor(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(xor_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(xor_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(xor_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.bitwiseXor(rhs));
				} break;

				case Expr::Kind::SHL: {
					const SHL& shl_inst = stack_frame->reader_agent.getSHL(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(shl_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(shl_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(shl_inst.rhs, *stack_frame).getInt(num_bits);

					if(shl_inst.nsw){
						if(shl_inst.nuw){
							core::GenericInt::WrapResult unsigned_result = lhs.ushl(rhs);
							if(unsigned_result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }

							core::GenericInt::WrapResult signed_result = lhs.sshl(rhs);
							if(signed_result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }

							stack_frame->registers[expr] = core::GenericValue(std::move(unsigned_result.result));	

						}else{
							core::GenericInt::WrapResult result = lhs.sshl(rhs);
							if(result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }
							stack_frame->registers[expr] = core::GenericValue(std::move(result.result));
						}
						
					}else if(shl_inst.nuw){
						core::GenericInt::WrapResult result = lhs.ushl(rhs);
						if(result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }
						stack_frame->registers[expr] = core::GenericValue(std::move(result.result));
						
					}else{
						stack_frame->registers[expr] = core::GenericValue(lhs.ushl(rhs).result);
					}
				} break;

				case Expr::Kind::SSHL_SAT: {
					const SSHLSat& sshl_sat_inst = stack_frame->reader_agent.getSSHLSat(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(sshl_sat_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(sshl_sat_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(sshl_sat_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.sshlSat(rhs));
				} break;

				case Expr::Kind::USHL_SAT: {
					const USHLSat& ushl_sat_inst = stack_frame->reader_agent.getUSHLSat(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ushl_sat_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(ushl_sat_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(ushl_sat_inst.rhs, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(lhs.ushlSat(rhs));
				} break;

				case Expr::Kind::SSHR: {
					const SSHR& sshr_inst = stack_frame->reader_agent.getSSHR(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(sshr_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(sshr_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(sshr_inst.rhs, *stack_frame).getInt(num_bits);

					core::GenericInt::WrapResult result = lhs.sshr(rhs);
					if(result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }
					stack_frame->registers[expr] = core::GenericValue(std::move(result.result));
				} break;

				case Expr::Kind::USHR: {
					const USHR& ushr_inst = stack_frame->reader_agent.getUSHR(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ushr_inst.lhs);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt lhs = this->get_expr(ushr_inst.lhs, *stack_frame).getInt(num_bits);
					const core::GenericInt rhs = this->get_expr(ushr_inst.rhs, *stack_frame).getInt(num_bits);

					core::GenericInt::WrapResult result = lhs.ushr(rhs);
					if(result.wrapped){ return evo::Unexpected(FuncRunError::ARITHMETIC_WRAP); }
					stack_frame->registers[expr] = core::GenericValue(std::move(result.result));
				} break;

				case Expr::Kind::BIT_REVERSE: {
					const BitReverse& bit_reverse_inst = stack_frame->reader_agent.getBitReverse(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(bit_reverse_inst.arg);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt arg = this->get_expr(bit_reverse_inst.arg, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(arg.bitReverse());
				} break;

				case Expr::Kind::BYTE_SWAP: {
					const ByteSwap& byte_swap_inst = stack_frame->reader_agent.getByteSwap(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(byte_swap_inst.arg);
					const unsigned num_bytes = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt arg = this->get_expr(byte_swap_inst.arg, *stack_frame).getInt(num_bytes);

					stack_frame->registers[expr] = core::GenericValue(arg.byteSwap());
				} break;

				case Expr::Kind::CTPOP: {
					const CtPop& ctpop_inst = stack_frame->reader_agent.getCtPop(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ctpop_inst.arg);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt arg = this->get_expr(ctpop_inst.arg, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(arg.ctPop());
				} break;

				case Expr::Kind::CTLZ: {
					const CTLZ& ctlz_inst = stack_frame->reader_agent.getCTLZ(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(ctlz_inst.arg);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt arg = this->get_expr(ctlz_inst.arg, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(arg.ctlz());
				} break;

				case Expr::Kind::CTTZ: {
					const CTTZ& cttz_inst = stack_frame->reader_agent.getCTTZ(expr);

					const Type arg_type = stack_frame->reader_agent.getExprType(cttz_inst.arg);
					const unsigned num_bits = unsigned(this->engine.module.getSize(arg_type) * 8);

					const core::GenericInt arg = this->get_expr(cttz_inst.arg, *stack_frame).getInt(num_bits);

					stack_frame->registers[expr] = core::GenericValue(arg.cttz());
				} break;

				case Expr::Kind::CMPXCHG: {
					const CmpXchg& cmpxchg = stack_frame->reader_agent.getCmpXchg(expr);

					std::byte* target_ptr = this->get_expr_ptr(cmpxchg.target, *stack_frame);

					const Type expected_type = stack_frame->reader_agent.getExprType(cmpxchg.expected);
					const unsigned num_bytes = unsigned(this->engine.module.getSize(expected_type));
					const unsigned num_bits = num_bytes * 8;

					const core::GenericValue& generic_expected = this->get_expr(cmpxchg.expected, *stack_frame);
					const core::GenericValue& generic_desired = this->get_expr(cmpxchg.desired, *stack_frame);

					stack_frame->registers[ReaderAgent::extractCmpXchgLoaded(expr)] = generic_expected;

					switch(num_bytes){
						case 1: {
							uint8_t expected = static_cast<uint8_t>(generic_expected.getInt(num_bits));
							const uint8_t desired = static_cast<uint8_t>(generic_desired.getInt(num_bits));

							auto atomic_ref = std::atomic_ref<uint8_t>(*reinterpret_cast<uint8_t*>(target_ptr));

							const bool succeeded = atomic_ref.compare_exchange_strong(expected, desired);

							stack_frame->registers[ReaderAgent::extractCmpXchgSucceeded(expr)]
								= core::GenericValue(succeeded);
						} break;

						case 2: {
							uint16_t expected = static_cast<uint16_t>(generic_expected.getInt(num_bits));
							const uint16_t desired = static_cast<uint16_t>(generic_desired.getInt(num_bits));

							auto atomic_ref = std::atomic_ref<uint16_t>(*reinterpret_cast<uint16_t*>(target_ptr));

							const bool succeeded = atomic_ref.compare_exchange_strong(expected, desired);

							stack_frame->registers[ReaderAgent::extractCmpXchgSucceeded(expr)]
								= core::GenericValue(succeeded);
						} break;

						case 4: {
							uint32_t expected = static_cast<uint32_t>(generic_expected.getInt(num_bits));
							const uint32_t desired = static_cast<uint32_t>(generic_desired.getInt(num_bits));

							auto atomic_ref = std::atomic_ref<uint32_t>(*reinterpret_cast<uint32_t*>(target_ptr));

							const bool succeeded = atomic_ref.compare_exchange_strong(expected, desired);

							stack_frame->registers[ReaderAgent::extractCmpXchgSucceeded(expr)]
								= core::GenericValue(succeeded);
						} break;

						case 8: {
							uint64_t expected = static_cast<uint64_t>(generic_expected.getInt(num_bits));
							const uint64_t desired = static_cast<uint64_t>(generic_desired.getInt(num_bits));

							auto atomic_ref = std::atomic_ref<uint64_t>(*reinterpret_cast<uint64_t*>(target_ptr));

							const bool succeeded = atomic_ref.compare_exchange_strong(expected, desired);

							stack_frame->registers[ReaderAgent::extractCmpXchgSucceeded(expr)]
								= core::GenericValue(succeeded);
						} break;

						default: {
							const auto lock = std::scoped_lock(this->engine.get_atomic_lock(target_ptr));

							if(std::memcmp(target_ptr, generic_expected.dataRange().data(), num_bytes) == 0){
								std::memcpy(target_ptr, generic_desired.dataRange().data(), num_bytes);

								stack_frame->registers[ReaderAgent::extractCmpXchgSucceeded(expr)]
									= core::GenericValue(true);

							}else{
								stack_frame->registers[ReaderAgent::extractCmpXchgSucceeded(expr)]
									= core::GenericValue(false);
							}
						} break;
					}
				} break;

				case Expr::Kind::CMPXCHG_LOADED:    evo::debugFatalBreak("Not a valid stmt");
				case Expr::Kind::CMPXCHG_SUCCEEDED: evo::debugFatalBreak("Not a valid stmt");

				case Expr::Kind::LIFETIME_START: {
					// do nothing...
				} break;

				case Expr::Kind::LIFETIME_END: {
					// do nothing...
				} break;

			}

			stack_frame->instruction_index += 1;
		}
	}


	auto ExecutionEngineExecutor::get_expr(Expr expr, StackFrame& stack_frame) -> core::GenericValue& {
		core::GenericValue* const got_expr = this->get_expr_maybe_ptr(expr, stack_frame);

		evo::debugAssert(
			got_expr != nullptr,
			"Got expr was a nullptr. Perhaps the caller should be using get_expr_maybe_ptr() instead"
		);

		return *got_expr;
	};


	auto ExecutionEngineExecutor::get_expr_maybe_ptr(Expr expr, StackFrame& stack_frame) -> core::GenericValue* {
		switch(expr.kind()){
			case Expr::Kind::GLOBAL_VALUE: {
				const GlobalVar::ID global_var_id = stack_frame.reader_agent.getGlobalValue(expr);

				ExecutionEngine::LoweredResult lowered_result = this->engine.check_global_lowered(global_var_id);

				if(lowered_result.needs_to_be_lowered){
					const evo::Expected<void, evo::SmallVector<std::string>> jit_add_result =
						this->engine.jit_engine.addModuleSubsetWithWeakDependencies(
							this->engine.module, JITEngine::ModuleSubsets{ .globalVars = global_var_id }
						);

					evo::debugAssert(jit_add_result.has_value(), "Adding to JITEngine failed");

					lowered_result.was_finished_being_lowered.store(true);

				}else{
					while(lowered_result.was_finished_being_lowered.load() == false){
						std::this_thread::yield();
					}
				}

				const GlobalVar& global_var = this->engine.module.getGlobalVar(global_var_id);

				std::byte* global_ptr = this->engine.jit_engine.getSymbol<std::byte*>(global_var.name);

				const size_t type_size = this->engine.module.getSize(global_var.type);
				return &stack_frame.registers.emplace(
					expr, core::GenericValue::fromData(evo::ArrayProxy<std::byte>(global_ptr, type_size))
				).first->second;
			} break;

			case Expr::Kind::FUNCTION_POINTER: {
				// TODO(FUTURE): 
				evo::unimplemented("Expr::Kind::FUNCTION_POINTER");
			} break;

			case Expr::Kind::NUMBER: {
				const Number& number = stack_frame.reader_agent.getNumber(expr);
				return &stack_frame.registers.emplace(expr, number.asGenericValue()).first->second;
			} break;

			case Expr::Kind::BOOLEAN: {
				return &stack_frame.registers.emplace(
					expr, core::GenericValue(stack_frame.reader_agent.getBoolean(expr))
				).first->second;
			} break;

			case Expr::Kind::PARAM_EXPR: {
				const Type param_type = stack_frame.reader_agent.getExprType(expr);

				const ParamExpr& param_expr = stack_frame.reader_agent.getParamExpr(expr);

				std::byte* param_ptr = static_cast<std::byte*>(stack_frame.params[param_expr.index]);

				if(param_ptr == nullptr){ return nullptr; }

				if(param_type.kind() == Type::Kind::PTR){
					return &stack_frame.registers.emplace(expr, core::GenericValue::createPtr(param_ptr)).first->second;

				}else{
					const size_t type_size = this->engine.module.getSize(param_type);
					return &stack_frame.registers.emplace(
						expr, core::GenericValue::fromData(evo::ArrayProxy<std::byte>(param_ptr, type_size))
					).first->second;
				}

			} break;

			case Expr::Kind::NULLPTR: {
				return &stack_frame.registers.emplace(expr, core::GenericValue::createPtr(nullptr)).first->second;
			} break;

			case Expr::Kind::ALLOCA: {
				const Alloca& alloca_expr = stack_frame.reader_agent.getAlloca(expr);

				const size_t offset = stack_frame.alloca_offsets.at(&alloca_expr);
				const std::byte* alloca_ptr = &stack_frame.alloca_buffer[offset];

				return &stack_frame.registers.emplace(expr, core::GenericValue::createPtr(alloca_ptr)).first->second;
			} break;

			default: {
				evo::debugAssert(stack_frame.registers.contains(expr), "Doesn't have this value in registers");
				return &stack_frame.registers.at(expr);
			} break;
		}
	};


	auto ExecutionEngineExecutor::get_expr_ptr(Expr expr, StackFrame& stack_frame) -> std::byte* {
		switch(expr.kind()){
			case Expr::Kind::PARAM_EXPR: {
				const ParamExpr& param_expr = stack_frame.reader_agent.getParamExpr(expr);
				return stack_frame.params[param_expr.index];
			} break;

			case Expr::Kind::GLOBAL_VALUE: {
				const GlobalVar::ID global_var_id = stack_frame.reader_agent.getGlobalValue(expr);

				ExecutionEngine::LoweredResult lowered_result = this->engine.check_global_lowered(global_var_id);

				if(lowered_result.needs_to_be_lowered){
					const evo::Expected<void, evo::SmallVector<std::string>> jit_add_result =
						this->engine.jit_engine.addModuleSubsetWithWeakDependencies(
							this->engine.module, JITEngine::ModuleSubsets{ .globalVars = global_var_id }
						);

					evo::debugAssert(jit_add_result.has_value(), "Adding to JITEngine failed");

					lowered_result.was_finished_being_lowered.store(true);

				}else{
					while(lowered_result.was_finished_being_lowered.load() == false){
						std::this_thread::yield();
					}
				}

				const GlobalVar& global_var = this->engine.module.getGlobalVar(global_var_id);

				return this->engine.jit_engine.getSymbol<std::byte*>(global_var.name);
			} break;

			case Expr::Kind::ALLOCA: {
				const Alloca& alloca_info = stack_frame.reader_agent.getAlloca(expr);

				const size_t offset = stack_frame.alloca_offsets.at(&alloca_info);

				return &stack_frame.alloca_buffer[offset];
			} break;

			default: {
				return stack_frame.registers.at(expr).getPtr<std::byte*>();
			} break;
		}
	}



	auto ExecutionEngineExecutor::setup_allocas(StackFrame& stack_frame) -> void {
		size_t total_allocas_size = 0;

		for(const Alloca& alloca_info : stack_frame.reader_agent.getTargetFunction().getAllocasRange()){
			const size_t alloca_size = this->engine.module.getSize(alloca_info.type);

			stack_frame.alloca_offsets.emplace(&alloca_info, total_allocas_size);
			total_allocas_size += alloca_size;
		}

		stack_frame.alloca_buffer = std::make_unique<std::byte[]>(total_allocas_size);
	}


}