//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./ComptimeExecutor.h"

#include <llvm_interface.h>

#include "../include/Context.h"
#include "./ASGToLLVMIR.h"


namespace pcit::panther{

	struct ComptimeExecutor::Data{
		llvmint::LLVMContext llvm_context{};
		llvmint::Module module{};
		llvmint::ExecutionEngine execution_engine{};
		ASGToLLVMIR* asg_to_llvmir{};
	};


	auto ComptimeExecutor::init() -> std::string {
		this->data = new Data();

		this->data->llvm_context.init();	
		this->data->module.init("testing", this->data->llvm_context);


		const std::string target_triple = this->data->module.getDefaultTargetTriple();

		const std::string data_layout_error = this->data->module.setDataLayout(
			target_triple,
			llvmint::Module::Relocation::Default,
			llvmint::Module::CodeSize::Default,
			llvmint::Module::OptLevel::None,
			false
		);

		if(!data_layout_error.empty()){
			return std::format("Failed to set data layout with message: {}", data_layout_error);
		}

		this->data->module.setTargetTriple(target_triple);

		this->data->asg_to_llvmir = new ASGToLLVMIR(
			this->context, this->data->llvm_context, this->data->module, ASGToLLVMIR::Config(false)
		);

		// this->data->asg_to_llvmir->addRuntimeLinks();
		// this->data->asg_to_llvmir->lower();

		return std::string();
	}


	auto ComptimeExecutor::deinit() -> void {
		if(this->data->execution_engine.hasCreatedEngine()){
			this->data->execution_engine.shutdownEngine();
		}

		delete this->data->asg_to_llvmir;
		this->data->asg_to_llvmir = nullptr;

		this->data->module.deinit();
		this->data->llvm_context.deinit();

		delete this->data;
		this->data = nullptr;
	}



	auto ComptimeExecutor::runFunc(
		const ASG::Func::LinkID& link_id, evo::ArrayProxy<ASG::Expr> params, ASGBuffer& asg_buffer
	) -> evo::SmallVector<ASG::Expr> {
		evo::debugAssert(this->isInitialized(), "not initialized");

		const auto lock = std::shared_lock(this->mutex);

		this->restart_engine_if_needed();

		const TypeManager& type_manager = this->context.getTypeManager();

		const ASG::Func& func = this->context.getSourceManager()[link_id.sourceID()]
			.getASGBuffer().getFunc(link_id.funcID());
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.baseTypeID.funcID());

		const TypeInfo::ID func_return_type_id = func_type.returnParams().front().typeID.typeID();
		const TypeInfo& func_return_type = type_manager.getTypeInfo(func_return_type_id);

		const std::string_view func_mangled_name = this->data->asg_to_llvmir->getFuncMangledName(link_id);


		evo::debugAssert(
			func_return_type.baseTypeID().kind() == BaseType::Kind::Primitive, "non-primitive type not supported yet"
		);
		evo::debugAssert(func_return_type.qualifiers().empty(), "qualifiers not supported yet");

		const BaseType::Primitive& func_return_base_type = type_manager.getPrimitive(
			func_return_type.baseTypeID().primitiveID()
		);

		switch(func_return_base_type.kind()){
			case Token::Kind::TypeBool: {
				const bool value = this->data->module.run<bool>(func_mangled_name);
				return evo::SmallVector<ASG::Expr>{ASG::Expr(asg_buffer.createLiteralBool(value))};
			} break;

			case Token::Kind::TypeChar: {
				const char value = this->data->module.run<char>(func_mangled_name);
				return evo::SmallVector<ASG::Expr>{ASG::Expr(asg_buffer.createLiteralChar(value))};
			} break;

			case Token::Kind::TypeF16: {
				evo::fatalBreak("Type F16 not supported for comptime return yet (bugged in LLVM)");
			} break;

			case Token::Kind::TypeBF16: {
				evo::fatalBreak("Type BF16 not supported for comptime return yet (bugged in LLVM)");
			} break;

			case Token::Kind::TypeF32: {
				const float32_t value = this->data->module.run<float32_t>(func_mangled_name);
				return evo::SmallVector<ASG::Expr>{
					ASG::Expr(
						asg_buffer.createLiteralFloat(core::GenericFloat(value), func_return_type_id)
					)
				};
			} break;

			case Token::Kind::TypeF64: {
				const float64_t value = this->data->module.run<float64_t>(func_mangled_name);
				return evo::SmallVector<ASG::Expr>{
					ASG::Expr(
						asg_buffer.createLiteralFloat(core::GenericFloat(value), func_return_type_id)
					)
				};
			} break;

			case Token::Kind::TypeF80: case Token::Kind::TypeF128: {
				evo::fatalBreak("Type F80 and F128 not supported for comptime return yet");
			} break;

			case Token::Kind::TypeCLongDouble: {
				evo::fatalBreak("Type CLongDouble is not supported for comptime return yet");
			} break;

			case Token::Kind::TypeInt:     case Token::Kind::TypeISize:  case Token::Kind::TypeI_N:
			case Token::Kind::TypeUInt:    case Token::Kind::TypeUSize:  case Token::Kind::TypeUI_N:
			case Token::Kind::TypeByte:    case Token::Kind::TypeRawPtr: case Token::Kind::TypeCShort:
			case Token::Kind::TypeCUShort: case Token::Kind::TypeCInt:   case Token::Kind::TypeCUInt:
			case Token::Kind::TypeCLong:   case Token::Kind::TypeCULong: case Token::Kind::TypeCLongLong:
			case Token::Kind::TypeCULongLong: {
				const ASG::LiteralInt::ID literal_int_id = [&](){
					const size_t size_of_func_return_base_type = type_manager.sizeOf(func_return_type.baseTypeID());

					if(size_of_func_return_base_type == 1){
						const uint8_t value = this->data->module.run<uint8_t>(func_mangled_name);
						return asg_buffer.createLiteralInt(
							core::GenericInt::create<uint8_t>(value), func_return_type_id
						);

					}else if(size_of_func_return_base_type == 2){
						const uint16_t value = this->data->module.run<uint16_t>(func_mangled_name);
						return asg_buffer.createLiteralInt(
							core::GenericInt::create<uint16_t>(value), func_return_type_id
						);

					}else if(size_of_func_return_base_type == 4){
						const uint32_t value = this->data->module.run<uint32_t>(func_mangled_name);
						return asg_buffer.createLiteralInt(
							core::GenericInt::create<uint32_t>(value), func_return_type_id
						);

					}else if(size_of_func_return_base_type == 8){
						const uint64_t value = this->data->module.run<uint64_t>(func_mangled_name);
						return asg_buffer.createLiteralInt(
							core::GenericInt::create<uint64_t>(value), func_return_type_id
						);

					}else{
						evo::debugFatalBreak("This type is not supported");
					}
				}();

				return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_int_id)};
			} break;


			default: {
				evo::debugFatalBreak("Unknown or unsupported type");
			} break;
		}

	};




	auto ComptimeExecutor::addFunc(const ASG::Func::LinkID& func_link_id) -> void {
		evo::debugAssert(this->isInitialized(), "not initialized");

		const auto lock = std::unique_lock(this->mutex);
		this->requires_engine_restart = true;

		this->data->asg_to_llvmir->lowerFunc(func_link_id);
	}


	auto ComptimeExecutor::restart_engine_if_needed() -> void {
		evo::debugAssert(this->isInitialized(), "not initialized");

		if(this->requires_engine_restart == false){ return; }
		this->requires_engine_restart = false;

		if(this->data->execution_engine.hasCreatedEngine()){
			this->data->execution_engine.shutdownEngine();
		}

		this->data->execution_engine.createEngine(this->data->module);
		this->data->execution_engine.setupLinkedFuncs();
	}


}