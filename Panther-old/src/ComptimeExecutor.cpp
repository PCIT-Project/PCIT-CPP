////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./ComptimeExecutor.h"

#include "../include/Context.h"
#include "./ASGToPIR.h"


namespace pcit::panther{

	ComptimeExecutor::ComptimeExecutor(Context& _context, core::Printer& _printer) 
		: context(_context),
		printer(_printer), 
		module("PTHR-ComptimeExecutor", this->context.getConfig().os, this->context.getConfig().arch)
		{}



	auto ComptimeExecutor::init() -> void {
		const auto config = ASGToPIR::Config{
			.useReadableRegisters = false,
			.checkedMath          = true,
			.isJIT                = true,
			.addSourceLocations   = true,
		};

		this->asg_to_pir = new ASGToPIR(this->context, this->module, config);
	}


	auto ComptimeExecutor::deinit() -> void {
		delete this->asg_to_pir;
		this->asg_to_pir = nullptr;

		if(this->jit_engine.isInitialized()){
			this->jit_engine.deinit();
		}
	}



	auto ComptimeExecutor::runFunc(
		const ASG::Func::LinkID& link_id, evo::ArrayProxy<ASG::Expr>, ASGBuffer& asg_buffer
	) -> evo::Result<evo::SmallVector<ASG::Expr>> {
		evo::debugAssert(this->isInitialized(), "not initialized");

		const auto lock = std::shared_lock(this->mutex);

		this->restart_engine_if_needed();

		const TypeManager& type_manager = this->context.getTypeManager();

		const ASG::Func& func = this->context.getSourceManager()[link_id.sourceID()]
			.getASGBuffer().getFunc(link_id.funcID());
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.baseTypeID.funcID());

		const TypeInfo::ID func_return_type_id = func_type.returnParams.front().typeID.typeID();
		const TypeInfo& func_return_type = type_manager.getTypeInfo(func_return_type_id);

		const pir::Function::ID& func_pir_id = this->asg_to_pir->getPIRFunctionID(link_id);


		evo::debugAssert(
			func_return_type.baseTypeID().kind() == BaseType::Kind::Primitive, "non-primitive type not supported yet"
		);
		evo::debugAssert(func_return_type.qualifiers().empty(), "qualifiers not supported yet");

		const BaseType::Primitive& func_return_base_type = type_manager.getPrimitive(
			func_return_type.baseTypeID().primitiveID()
		);

		switch(func_return_base_type.kind()){
			case Token::Kind::TypeBool: {
				const evo::Result<core::GenericValue> value = this->jit_engine.runFunc(func_pir_id);
				if(value.isError()){ return evo::resultError; }
				return evo::SmallVector<ASG::Expr>{ASG::Expr(asg_buffer.createLiteralBool(value.value().as<bool>()))};
			} break;

			case Token::Kind::TypeChar: {
				const evo::Result<core::GenericValue> value = this->jit_engine.runFunc(func_pir_id);
				if(value.isError()){ return evo::resultError; }
				return evo::SmallVector<ASG::Expr>{ASG::Expr(asg_buffer.createLiteralChar(value.value().as<char>()))};
			} break;

			case Token::Kind::TypeF16: {
				evo::fatalBreak("Type F16 not supported for comptime return yet (bugged in LLVM)");
			} break;

			case Token::Kind::TypeBF16: {
				evo::fatalBreak("Type BF16 not supported for comptime return yet (bugged in LLVM)");
			} break;

			case Token::Kind::TypeF32: {
				const evo::Result<core::GenericValue> value = this->jit_engine.runFunc(func_pir_id);
				if(value.isError()){ return evo::resultError; }
				return evo::SmallVector<ASG::Expr>{
					ASG::Expr(
						asg_buffer.createLiteralFloat(value.value().as<core::GenericFloat>(), func_return_type_id)
					)
				};
			} break;

			case Token::Kind::TypeF64: {
				const evo::Result<core::GenericValue> value = this->jit_engine.runFunc(func_pir_id);
				if(value.isError()){ return evo::resultError; }
				return evo::SmallVector<ASG::Expr>{
					ASG::Expr(
						asg_buffer.createLiteralFloat(value.value().as<core::GenericFloat>(), func_return_type_id)
					)
				};
			} break;

			case Token::Kind::TypeF80: case Token::Kind::TypeF128: {
				evo::fatalBreak("Type F80 and F128 not supported for comptime return yet");
			} break;

			case Token::Kind::TypeCLongDouble: {
				evo::fatalBreak("Type CLongDouble is not supported for comptime return yet");
			} break;

			case Token::Kind::TypeInt:       case Token::Kind::TypeISize:        case Token::Kind::TypeI_N:
			case Token::Kind::TypeUInt:      case Token::Kind::TypeUSize:        case Token::Kind::TypeUI_N:
			case Token::Kind::TypeByte:      case Token::Kind::TypeRawPtr:       case Token::Kind::TypeTypeID:
			case Token::Kind::TypeCShort:    case Token::Kind::TypeCUShort:      case Token::Kind::TypeCInt:
			case Token::Kind::TypeCUInt:     case Token::Kind::TypeCLong:        case Token::Kind::TypeCULong:
			case Token::Kind::TypeCLongLong: case Token::Kind::TypeCULongLong: {
				const evo::Result<ASG::LiteralInt::ID> literal_int_id = [&](){
					const size_t size_of_func_return_base_type = type_manager.sizeOf(func_return_type.baseTypeID());

					if(size_of_func_return_base_type == 1){
						const evo::Result<core::GenericValue> value = this->jit_engine.runFunc(func_pir_id);
						if(value.isError()){ return evo::Result<ASG::LiteralInt::ID>::error(); }
						return evo::Result<ASG::LiteralInt::ID>(asg_buffer.createLiteralInt(
							value.value().as<core::GenericInt>(), func_return_type_id
						));

					}else if(size_of_func_return_base_type == 2){
						const evo::Result<core::GenericValue> value = this->jit_engine.runFunc(func_pir_id);
						if(value.isError()){ return evo::Result<ASG::LiteralInt::ID>::error(); }
						return evo::Result<ASG::LiteralInt::ID>(asg_buffer.createLiteralInt(
							value.value().as<core::GenericInt>(), func_return_type_id
						));

					}else if(size_of_func_return_base_type == 4){
						const evo::Result<core::GenericValue> value = this->jit_engine.runFunc(func_pir_id);
						if(value.isError()){ return evo::Result<ASG::LiteralInt::ID>::error(); }
						return evo::Result<ASG::LiteralInt::ID>(asg_buffer.createLiteralInt(
							value.value().as<core::GenericInt>(), func_return_type_id
						));

					}else if(size_of_func_return_base_type == 8){
						const evo::Result<core::GenericValue> value = this->jit_engine.runFunc(func_pir_id);
						if(value.isError()){ return evo::Result<ASG::LiteralInt::ID>::error(); }
						return evo::Result<ASG::LiteralInt::ID>(asg_buffer.createLiteralInt(
							value.value().as<core::GenericInt>(), func_return_type_id
						));

					}else{
						evo::debugFatalBreak("This type is not supported");
					}
				}();
				if(literal_int_id.isError()){ return evo::resultError; }

				return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_int_id.value())};
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

		this->asg_to_pir->lowerFunc(func_link_id);
	}


	//////////////////////////////////////////////////////////////////////
	// intrinsics


	///////////////////////////////////
	// addition

	auto ComptimeExecutor::intrinAdd(
		const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> evo::Result<core::GenericInt> {
		const core::GenericInt::WrapResult result = this->intrinAddWrap(type_id, lhs, rhs);
		if(result.wrapped && !may_wrap){ return evo::resultError; }
		return result.result;
	}


	auto ComptimeExecutor::intrinAddWrap(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt::WrapResult {
		return this->intrin_base_impl<core::GenericInt::WrapResult>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt::WrapResult {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.uadd(rhs);
				}else{
					return lhs.sadd(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinAddSat(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt {
		return this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.uaddSat(rhs);
				}else{
					return lhs.saddSat(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinFAdd(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> core::GenericFloat {
		return this->intrin_base_impl<core::GenericFloat>(type_id, lhs, rhs, 
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> core::GenericFloat {
				return lhs.add(rhs);
			}
		);
	}


	///////////////////////////////////
	// subtraction

	auto ComptimeExecutor::intrinSub(
		const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> evo::Result<core::GenericInt> {
		const core::GenericInt::WrapResult result = this->intrinSubWrap(type_id, lhs, rhs);
		if(result.wrapped && !may_wrap){ return evo::resultError; }
		return result.result;
	}


	auto ComptimeExecutor::intrinSubWrap(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt::WrapResult {
		return this->intrin_base_impl<core::GenericInt::WrapResult>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt::WrapResult {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.usub(rhs);
				}else{
					return lhs.ssub(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinSubSat(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt {
		return this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.usubSat(rhs);
				}else{
					return lhs.ssubSat(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinFSub(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> core::GenericFloat {
		return this->intrin_base_impl<core::GenericFloat>(type_id, lhs, rhs, 
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> core::GenericFloat {
				return lhs.sub(rhs);
			}
		);
	}


	///////////////////////////////////
	// multiplication

	auto ComptimeExecutor::intrinMul(
		const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> evo::Result<core::GenericInt> {
		const core::GenericInt::WrapResult result = this->intrinMulWrap(type_id, lhs, rhs);
		if(result.wrapped && !may_wrap){ return evo::resultError; }
		return result.result;
	}


	auto ComptimeExecutor::intrinMulWrap(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt::WrapResult {
		return this->intrin_base_impl<core::GenericInt::WrapResult>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt::WrapResult {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.umul(rhs);
				}else{
					return lhs.smul(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinMulSat(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt {
		return this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.umulSat(rhs);
				}else{
					return lhs.smulSat(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinFMul(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> core::GenericFloat {
		return this->intrin_base_impl<core::GenericFloat>(type_id, lhs, rhs, 
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> core::GenericFloat {
				return lhs.mul(rhs);
			}
		);
	}


	///////////////////////////////////
	// division / remainder

	auto ComptimeExecutor::intrinDiv(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt {
		return this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.udiv(rhs);
				}else{
					return lhs.sdiv(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinFDiv(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> core::GenericFloat {
		return this->intrin_base_impl<core::GenericFloat>(type_id, lhs, rhs, 
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> core::GenericFloat {
				return lhs.div(rhs);
			}
		);
	}

	auto ComptimeExecutor::intrinRem(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt {
		return this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.urem(rhs);
				}else{
					return lhs.srem(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinRem(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> core::GenericFloat {
		return this->intrin_base_impl<core::GenericFloat>(type_id, lhs, rhs, 
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> core::GenericFloat {
				return lhs.rem(rhs);
			}
		);
	}


	///////////////////////////////////
	// logical

	auto ComptimeExecutor::intrinEQ(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				return lhs.eq(rhs);
			}
		);
	}

	auto ComptimeExecutor::intrinEQ(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.eq(rhs);
			}
		);	
	}


	auto ComptimeExecutor::intrinNEQ(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				return lhs.neq(rhs);
			}
		);
	}

	auto ComptimeExecutor::intrinNEQ(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.neq(rhs);
			}
		);	
	}


	auto ComptimeExecutor::intrinLT(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.ult(rhs);
				}else{
					return lhs.slt(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinLT(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.lt(rhs);
			}
		);	
	}


	auto ComptimeExecutor::intrinLTE(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.ule(rhs);
				}else{
					return lhs.sle(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinLTE(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.le(rhs);
			}
		);	
	}


	auto ComptimeExecutor::intrinGT(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.ugt(rhs);
				}else{
					return lhs.sgt(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinGT(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.gt(rhs);
			}
		);	
	}


	auto ComptimeExecutor::intrinGTE(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.uge(rhs);
				}else{
					return lhs.sge(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinGTE(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> bool {
		return this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.ge(rhs);
			}
		);	
	}



	///////////////////////////////////
	// bitwise


	auto ComptimeExecutor::intrinAnd(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt {
		return this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> core::GenericInt {
				return lhs.bitwiseAnd(rhs);
			}
		);
	}

	auto ComptimeExecutor::intrinOr(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt {
		return this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> core::GenericInt {
				return lhs.bitwiseOr(rhs);
			}
		);
	}

	auto ComptimeExecutor::intrinXor(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt {
		return this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> core::GenericInt {
				return lhs.bitwiseXor(rhs);
			}
		);
	}

	auto ComptimeExecutor::intrinSHL(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs, bool may_overflow
	) -> evo::Result<core::GenericInt> {
		return this->intrin_base_impl<evo::Result<core::GenericInt>>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> evo::Result<core::GenericInt> {
				const core::GenericInt::WrapResult result = [&](){
					if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
						return lhs.ushl(rhs);
					}else{
						return lhs.sshl(rhs);
					}
				}();
				if(result.wrapped && !may_overflow){ return evo::Result<core::GenericInt>::error(); }
				return evo::Result<core::GenericInt>(result.result);
			}
		);
	}

	auto ComptimeExecutor::intrinSHLSat(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt {
		return this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> core::GenericInt {
				if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
					return lhs.ushlSat(rhs);
				}else{
					return lhs.sshlSat(rhs);
				}
			}
		);
	}

	auto ComptimeExecutor::intrinSHR(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs, bool may_overflow
	) -> evo::Result<core::GenericInt> {
		return this->intrin_base_impl<evo::Result<core::GenericInt>>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> evo::Result<core::GenericInt> {
				const core::GenericInt::WrapResult result = [&](){
					if(this->context.getTypeManager().isUnsignedIntegral(type_id)){
						return lhs.ushr(rhs);
					}else{
						return lhs.sshr(rhs);
					}
				}();
				if(result.wrapped && !may_overflow){ return evo::Result<core::GenericInt>::error(); }
				return evo::Result<core::GenericInt>(result.result);
			}
		);
	}





	//////////////////////////////////////////////////////////////////////
	// internal engine

	auto ComptimeExecutor::restart_engine_if_needed() -> void {
		evo::debugAssert(this->isInitialized(), "not initialized");

		if(this->requires_engine_restart == false){ return; }
		this->requires_engine_restart = false;

		if(this->jit_engine.isInitialized()){
			this->jit_engine.deinit();
		}

		this->jit_engine.init(this->module);
	}


	//////////////////////////////////////////////////////////////////////
	// intrin impl

	template<class RETURN>
	auto ComptimeExecutor::intrin_base_impl(
		const TypeInfo::ID type_id,
		const core::GenericInt& lhs,
		const core::GenericInt& rhs,
		IntrinOp<RETURN> intrin_op
	) -> RETURN {
		TypeManager& type_manager = this->context.getTypeManager();
		const bool is_unsigned = type_manager.isUnsignedIntegral(type_id);

		const unsigned type_width = [&](){
			const TypeInfo::ID underlying_id = type_manager.getUnderlyingType(type_id).value();
			const TypeInfo& underlying_type = type_manager.getTypeInfo(underlying_id);
			const BaseType::Primitive& primitive = type_manager.getPrimitive(
				underlying_type.baseTypeID().primitiveID()
			);
			return primitive.bitWidth();
		}();
		

		const core::GenericInt& lhs_converted = lhs.extOrTrunc(type_width, is_unsigned);
		const core::GenericInt& rhs_converted = rhs.extOrTrunc(type_width, is_unsigned);

		return intrin_op(type_id, lhs_converted, rhs_converted);
	}

	template<class RETURN>
	auto ComptimeExecutor::intrin_base_impl(
		const TypeInfo::ID type_id,
		const core::GenericFloat& lhs,
		const core::GenericFloat& rhs,
		std::function<RETURN(const core::GenericFloat&, const core::GenericFloat&)> intrin_op
	) -> RETURN {
		const TypeManager& type_manager = this->context.getTypeManager();

		const TypeInfo& type = type_manager.getTypeInfo(type_id);
		const BaseType::Primitive& primitive = type_manager.getPrimitive(type.baseTypeID().primitiveID());

		auto lhs_converted = std::optional<core::GenericFloat>();
		auto rhs_converted = std::optional<core::GenericFloat>();
		switch(primitive.kind()){
			case Token::Kind::TypeF16: {
				lhs_converted = lhs.asF16();
				rhs_converted = rhs.asF16();
			} break;

			case Token::Kind::TypeBF16: {
				lhs_converted = lhs.asBF16();
				rhs_converted = rhs.asBF16();
			} break;

			case Token::Kind::TypeF32: {
				lhs_converted = lhs.asF32();
				rhs_converted = rhs.asF32();
			} break;

			case Token::Kind::TypeF64: {
				lhs_converted = lhs.asF64();
				rhs_converted = rhs.asF64();
			} break;

			case Token::Kind::TypeF80: {
				lhs_converted = lhs.asF80();
				rhs_converted = rhs.asF80();
			} break;

			case Token::Kind::TypeF128: {
				lhs_converted = lhs.asF128();
				rhs_converted = rhs.asF128();
			} break;

			default: evo::debugFatalBreak("Unknown or unsupported float type");
		}

		return intrin_op(*lhs_converted, *rhs_converted);
	}




}