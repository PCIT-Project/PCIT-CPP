////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./ComptimeIntrinsicEvaluator.h"

#include "../../include/sema/SemaBuffer.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther{

	auto ComptimeIntrinsicEvaluator::getTypeID(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeTypeID(),
			sema::Expr(this->sema_buffer.createIntValue(
				core::GenericInt::create<uint32_t>(type_id.get()),
				this->type_manager.getTypeInfo(TypeManager::getTypeTypeID()).baseTypeID()
			))
		);
	}

	auto ComptimeIntrinsicEvaluator::arrayElementTypeID(TypeInfo::ID type_id) -> TermInfo {
		const TypeInfo& type_info = this->type_manager.getTypeInfo(type_id);
		evo::debugAssert(type_info.qualifiers().empty(), "Should have no qualifiers");
		const BaseType::Array& array_type = this->type_manager.getArray(type_info.baseTypeID().arrayID());

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeTypeID(),
			sema::Expr(this->sema_buffer.createIntValue(
				core::GenericInt::create<uint32_t>(array_type.elementTypeID.get()),
				this->type_manager.getTypeInfo(TypeManager::getTypeTypeID()).baseTypeID()
			))
		);
	}

	auto ComptimeIntrinsicEvaluator::arrayRefElementTypeID(TypeInfo::ID type_id) -> TermInfo {
		const TypeInfo& type_info = this->type_manager.getTypeInfo(type_id);
		evo::debugAssert(type_info.qualifiers().empty(), "Should have no qualifiers");
		const BaseType::ArrayRef& array_ref_type = this->type_manager.getArrayRef(type_info.baseTypeID().arrayRefID());

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeTypeID(),
			sema::Expr(this->sema_buffer.createIntValue(
				core::GenericInt::create<uint32_t>(array_ref_type.elementTypeID.get()),
				this->type_manager.getTypeInfo(TypeManager::getTypeTypeID()).baseTypeID()
			))
		);
	}


	
	auto ComptimeIntrinsicEvaluator::numBytes(TypeInfo::ID type_id, bool include_padding) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeUSize(),
			sema::Expr(this->sema_buffer.createIntValue(
				core::GenericInt(
					unsigned(this->type_manager.numBitsOfPtr()), this->type_manager.numBytes(type_id, include_padding)
				),
				this->type_manager.getTypeInfo(TypeManager::getTypeUSize()).baseTypeID()
			))
		);
	}

	auto ComptimeIntrinsicEvaluator::numBits(TypeInfo::ID type_id, bool include_padding) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeUSize(),
			sema::Expr(this->sema_buffer.createIntValue(
				core::GenericInt(
					unsigned(this->type_manager.numBitsOfPtr()), this->type_manager.numBits(type_id, include_padding)
				),
				this->type_manager.getTypeInfo(TypeManager::getTypeUSize()).baseTypeID()
			))
		);
	}

	auto ComptimeIntrinsicEvaluator::isDefaultInitializable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isDefaultInitializable(type_id)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isTriviallyDefaultInitializable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isTriviallyDefaultInitializable(type_id)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isComptimeDefaultInitializable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isComptimeDefaultInitializable(type_id)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isNoErrorDefaultInitializable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isNoErrorDefaultInitializable(type_id)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isSafeDefaultInitializable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isSafeDefaultInitializable(type_id)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isTriviallyDeletable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isTriviallyDeletable(type_id)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isComptimeDeletable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isComptimeDeletable(type_id, this->sema_buffer)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isCopyable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isCopyable(type_id)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isTriviallyCopyable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isTriviallyCopyable(type_id)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isComptimeCopyable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isComptimeCopyable(type_id, this->sema_buffer)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isNoErrorCopyable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isNoErrorCopyable(type_id, this->sema_buffer)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isSafeCopyable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isSafeCopyable(type_id, this->sema_buffer)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isMovable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isMovable(type_id)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isTriviallyMovable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isTriviallyMovable(type_id)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isComptimeMovable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isComptimeMovable(type_id, this->sema_buffer)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isNoErrorMovable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isNoErrorMovable(type_id, this->sema_buffer)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isSafeMovable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isSafeMovable(type_id, this->sema_buffer)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isComparable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isComparable(type_id, this->sema_buffer)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isTriviallyComparable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isTriviallyComparable(type_id)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isComptimeComparable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isComptimeComparable(type_id, this->sema_buffer)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isNoErrorComparable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isNoErrorComparable(type_id, this->sema_buffer)
				)
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::isSafeComparable(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(
				this->sema_buffer.createBoolValue(
					this->type_manager.isSafeComparable(type_id, this->sema_buffer)
				)
			)
		);
	}




	///////////////////////////////////
	// type conversion

	auto ComptimeIntrinsicEvaluator::trunc(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			to_type_id,
			sema::Expr(this->sema_buffer.createIntValue(arg, this->type_manager.getTypeInfo(to_type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::ftrunc(const TypeInfo::ID to_type_id, const core::GenericFloat& arg) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			to_type_id,
			sema::Expr(this->sema_buffer.createFloatValue(arg, this->type_manager.getTypeInfo(to_type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::sext(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			to_type_id,
			sema::Expr(this->sema_buffer.createIntValue(arg, this->type_manager.getTypeInfo(to_type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::zext(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			to_type_id,
			sema::Expr(this->sema_buffer.createIntValue(arg, this->type_manager.getTypeInfo(to_type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::fext(const TypeInfo::ID to_type_id, const core::GenericFloat& arg) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			to_type_id,
			sema::Expr(this->sema_buffer.createFloatValue(arg, this->type_manager.getTypeInfo(to_type_id).baseTypeID()))
		);
	}


	auto ComptimeIntrinsicEvaluator::iToF(const TypeInfo::ID to_type_id, const core::GenericInt& arg) -> TermInfo {
		const TypeInfo& to_type = this->type_manager.getTypeInfo(to_type_id);
		const BaseType::Primitive& to_type_primitive =
			this->type_manager.getPrimitive(to_type.baseTypeID().primitiveID());


		const bool is_signed = this->type_manager.isSignedIntegral(to_type_id);

		core::GenericFloat result = [&](){
			switch(to_type_primitive.kind()){
				case Token::Kind::TYPE_F16:  return core::GenericFloat::createF16FromInt(arg, is_signed);
				case Token::Kind::TYPE_BF16: return core::GenericFloat::createBF16FromInt(arg, is_signed);
				case Token::Kind::TYPE_F32:  return core::GenericFloat::createF32FromInt(arg, is_signed);
				case Token::Kind::TYPE_F64:  return core::GenericFloat::createF64FromInt(arg, is_signed);
				case Token::Kind::TYPE_F80:  return core::GenericFloat::createF80FromInt(arg, is_signed);
				case Token::Kind::TYPE_F128: return core::GenericFloat::createF128FromInt(arg, is_signed);
				default: evo::debugFatalBreak("Invalid float type");
			}
		}();

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			to_type_id,
			sema::Expr(this->sema_buffer.createFloatValue(std::move(result), to_type.baseTypeID()))
		);
	}


	auto ComptimeIntrinsicEvaluator::fToI(const TypeInfo::ID to_type_id, const core::GenericFloat& arg) -> TermInfo {
		const size_t bit_width = [&](){
			const TypeInfo& type_info = this->type_manager.getTypeInfo(to_type_id);
			return this->type_manager.getPrimitive(type_info.baseTypeID().primitiveID()).bitWidth();
		}();

		const bool is_signed = this->type_manager.isSignedIntegral(to_type_id);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			to_type_id,
			sema::Expr(this->sema_buffer.createIntValue(
				arg.toGenericInt(unsigned(bit_width), is_signed),
				this->type_manager.getTypeInfo(to_type_id).baseTypeID())
			)
		);
	}



	///////////////////////////////////
	// addition

	auto ComptimeIntrinsicEvaluator::add(
		const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> evo::Result<TermInfo> {
		const core::GenericInt::WrapResult result = this->add_wrap_impl(type_id, lhs, rhs);
		if(result.wrapped && !may_wrap){ return evo::resultError; }

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(
				this->sema_buffer.createIntValue(result.result, this->type_manager.getTypeInfo(type_id).baseTypeID())
			)
		);
	}


	auto ComptimeIntrinsicEvaluator::addWrap(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const core::GenericInt::WrapResult result = this->add_wrap_impl(type_id, lhs, rhs);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(
				this->sema_buffer.createIntValue(result.result, this->type_manager.getTypeInfo(type_id).baseTypeID())
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::addSat(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const core::GenericInt result = this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.uaddSat(rhs);
				}else{
					return lhs.saddSat(rhs);
				}
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::fadd(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> TermInfo {
		const core::GenericFloat result = this->intrin_base_impl<core::GenericFloat>(type_id, lhs, rhs, 
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> core::GenericFloat {
				return lhs.add(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createFloatValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}


	///////////////////////////////////
	// subtraction

	auto ComptimeIntrinsicEvaluator::sub(
		const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> evo::Result<TermInfo> {
		const core::GenericInt::WrapResult result = this->sub_wrap_impl(type_id, lhs, rhs);
		if(result.wrapped && !may_wrap){ return evo::resultError; }

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(
				this->sema_buffer.createIntValue(result.result, this->type_manager.getTypeInfo(type_id).baseTypeID())
			)
		);
	}


	auto ComptimeIntrinsicEvaluator::subWrap(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const core::GenericInt::WrapResult result = this->sub_wrap_impl(type_id, lhs, rhs);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(
				this->sema_buffer.createIntValue(result.result, this->type_manager.getTypeInfo(type_id).baseTypeID())
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::subSat(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const core::GenericInt result = this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.usubSat(rhs);
				}else{
					return lhs.ssubSat(rhs);
				}
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::fsub(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> TermInfo {
		const core::GenericFloat result = this->intrin_base_impl<core::GenericFloat>(type_id, lhs, rhs, 
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> core::GenericFloat {
				return lhs.sub(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createFloatValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}


	///////////////////////////////////
	// multiplication

	auto ComptimeIntrinsicEvaluator::mul(
		const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> evo::Result<TermInfo> {
		const core::GenericInt::WrapResult result = this->mul_wrap_impl(type_id, lhs, rhs);
		if(result.wrapped && !may_wrap){ return evo::resultError; }

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(
				this->sema_buffer.createIntValue(result.result, this->type_manager.getTypeInfo(type_id).baseTypeID())
			)
		);
	}


	auto ComptimeIntrinsicEvaluator::mulWrap(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const core::GenericInt::WrapResult result = this->mul_wrap_impl(type_id, lhs, rhs);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(
				this->sema_buffer.createIntValue(result.result, this->type_manager.getTypeInfo(type_id).baseTypeID())
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::mulSat(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const core::GenericInt result = this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.umulSat(rhs);
				}else{
					return lhs.smulSat(rhs);
				}
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::fmul(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> TermInfo {
		const core::GenericFloat result = this->intrin_base_impl<core::GenericFloat>(type_id, lhs, rhs, 
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> core::GenericFloat {
				return lhs.mul(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createFloatValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}


	///////////////////////////////////
	// division / remainder

	auto ComptimeIntrinsicEvaluator::div(
		const TypeInfo::ID type_id, bool is_exact, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> evo::Result<TermInfo> {
		const bool is_unsigned = this->type_manager.isUnsignedIntegral(type_id);

		const unsigned type_width = [&](){
			const TypeInfo::ID underlying_id = this->type_manager.getUnderlyingType(type_id);
			const TypeInfo& underlying_type = this->type_manager.getTypeInfo(underlying_id);
			const BaseType::Primitive& primitive = this->type_manager.getPrimitive(
				underlying_type.baseTypeID().primitiveID()
			);
			return primitive.bitWidth();
		}();
		

		const core::GenericInt& lhs_converted = lhs.extOrTrunc(type_width, is_unsigned);
		const core::GenericInt& rhs_converted = rhs.extOrTrunc(type_width, is_unsigned);


		auto result = std::optional<core::GenericInt>();
		if(is_exact){
			if(this->type_manager.isUnsignedIntegral(type_id)){
				result = lhs_converted.udiv(rhs_converted);

				if(result->umul(rhs_converted).result.neq(lhs_converted)){
					return evo::resultError;
				}
			}else{
				result = lhs_converted.sdiv(rhs_converted);

				if(result->smul(rhs_converted).result.neq(lhs_converted)){
					return evo::resultError;
				}
			}
		}else{
			if(this->type_manager.isUnsignedIntegral(type_id)){
				result = lhs_converted.udiv(rhs_converted);
			}else{
				result = lhs_converted.sdiv(rhs_converted);
			}
		}

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(*result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::fdiv(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> TermInfo {
		const core::GenericFloat result = this->intrin_base_impl<core::GenericFloat>(type_id, lhs, rhs, 
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> core::GenericFloat {
				return lhs.div(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createFloatValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::rem(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const core::GenericInt result = this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.urem(rhs);
				}else{
					return lhs.srem(rhs);
				}
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::rem(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> TermInfo {
		const core::GenericFloat result = this->intrin_base_impl<core::GenericFloat>(type_id, lhs, rhs, 
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> core::GenericFloat {
				return lhs.rem(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createFloatValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}


	auto ComptimeIntrinsicEvaluator::fneg(const TypeInfo::ID type_id, const core::GenericFloat& arg) -> TermInfo {
		const TypeInfo& type = this->type_manager.getTypeInfo(type_id);
		const BaseType::Primitive& primitive = this->type_manager.getPrimitive(type.baseTypeID().primitiveID());

		auto arg_converted = std::optional<core::GenericFloat>();
		switch(primitive.kind()){
			case Token::Kind::TYPE_F16: {
				arg_converted = arg.asF16();
			} break;

			case Token::Kind::TYPE_BF16: {
				arg_converted = arg.asBF16();
			} break;

			case Token::Kind::TYPE_F32: {
				arg_converted = arg.asF32();
			} break;

			case Token::Kind::TYPE_F64: {
				arg_converted = arg.asF64();
			} break;

			case Token::Kind::TYPE_F80: {
				arg_converted = arg.asF80();
			} break;

			case Token::Kind::TYPE_F128: {
				arg_converted = arg.asF128();
			} break;

			default: evo::debugFatalBreak("Unknown or unsupported float type");
		}

		const core::GenericFloat result = arg_converted->neg();

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createFloatValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}


	// }

	///////////////////////////////////
	// logical

	auto ComptimeIntrinsicEvaluator::eq(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				return lhs.eq(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}

	auto ComptimeIntrinsicEvaluator::eq(bool lhs, bool rhs) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(lhs == rhs))
		);
	}

	auto ComptimeIntrinsicEvaluator::eq(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.eq(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}


	auto ComptimeIntrinsicEvaluator::neq(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				return lhs.neq(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}

	auto ComptimeIntrinsicEvaluator::neq(bool lhs, bool rhs) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(lhs != rhs))
		);
	}

	auto ComptimeIntrinsicEvaluator::neq(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.neq(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}


	auto ComptimeIntrinsicEvaluator::lt(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.ult(rhs);
				}else{
					return lhs.slt(rhs);
				}
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}

	auto ComptimeIntrinsicEvaluator::lt(bool lhs, bool rhs) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(lhs < rhs))
		);
	}

	auto ComptimeIntrinsicEvaluator::lt(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.lt(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}


	auto ComptimeIntrinsicEvaluator::lte(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.ule(rhs);
				}else{
					return lhs.sle(rhs);
				}
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}

	auto ComptimeIntrinsicEvaluator::lte(bool lhs, bool rhs) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(lhs <= rhs))
		);
	}

	auto ComptimeIntrinsicEvaluator::lte(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.le(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}


	auto ComptimeIntrinsicEvaluator::gt(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.ugt(rhs);
				}else{
					return lhs.sgt(rhs);
				}
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}

	auto ComptimeIntrinsicEvaluator::gt(bool lhs, bool rhs) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(lhs > rhs))
		);
	}

	auto ComptimeIntrinsicEvaluator::gt(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.gt(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}


	auto ComptimeIntrinsicEvaluator::gte(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) -> bool {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.uge(rhs);
				}else{
					return lhs.sge(rhs);
				}
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}

	auto ComptimeIntrinsicEvaluator::gte(bool lhs, bool rhs) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(lhs >= rhs))
		);
	}

	auto ComptimeIntrinsicEvaluator::gte(
		const TypeInfo::ID type_id, const core::GenericFloat& lhs, const core::GenericFloat& rhs
	) -> TermInfo {
		const bool result = this->intrin_base_impl<bool>(type_id, lhs, rhs,
			[&](const core::GenericFloat& lhs, const core::GenericFloat& rhs) -> bool {
				return lhs.ge(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(result))
		);
	}



	///////////////////////////////////
	// bitwise


	auto ComptimeIntrinsicEvaluator::bitwiseAnd(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const core::GenericInt result = this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> core::GenericInt {
				return lhs.bitwiseAnd(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::bitwiseAnd(bool lhs, bool rhs) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(lhs & rhs))
		);
	}



	auto ComptimeIntrinsicEvaluator::bitwiseOr(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const core::GenericInt result = this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> core::GenericInt {
				return lhs.bitwiseOr(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::bitwiseOr(bool lhs, bool rhs) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(lhs | rhs))
		);
	}



	auto ComptimeIntrinsicEvaluator::bitwiseXor(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const core::GenericInt result = this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> core::GenericInt {
				return lhs.bitwiseXor(rhs);
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::bitwiseXor(bool lhs, bool rhs) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			TypeManager::getTypeBool(),
			sema::Expr(this->sema_buffer.createBoolValue(lhs ^ rhs))
		);
	}



	auto ComptimeIntrinsicEvaluator::shl(
		const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> evo::Result<TermInfo> {
		const evo::Result<core::GenericInt> result = this->intrin_base_impl<evo::Result<core::GenericInt>>(
			type_id,
			lhs,
			rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> evo::Result<core::GenericInt> {
				const core::GenericInt::WrapResult result = [&](){
					if(this->type_manager.isUnsignedIntegral(type_id)){
						return lhs.ushl(rhs);
					}else{
						return lhs.sshl(rhs);
					}
				}();
				if(result.wrapped && !may_wrap){ return evo::Result<core::GenericInt>::error(); }
				return evo::Result<core::GenericInt>(result.result);
			}
		);

		if(result.isError()){ return evo::resultError; }

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(
				this->sema_buffer.createIntValue(result.value(), this->type_manager.getTypeInfo(type_id).baseTypeID())
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::shlSat(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> TermInfo {
		const core::GenericInt result = this->intrin_base_impl<core::GenericInt>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> core::GenericInt {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.ushlSat(rhs);
				}else{
					return lhs.sshlSat(rhs);
				}
			}
		);

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::shr(
		const TypeInfo::ID type_id, bool may_wrap, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> evo::Result<TermInfo> {
		const evo::Result<core::GenericInt> result = this->intrin_base_impl<evo::Result<core::GenericInt>>(
			type_id,
			lhs,
			rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs)
			-> evo::Result<core::GenericInt> {
				const core::GenericInt::WrapResult result = [&](){
					if(this->type_manager.isUnsignedIntegral(type_id)){
						return lhs.ushr(rhs);
					}else{
						return lhs.sshr(rhs);
					}
				}();
				if(result.wrapped && !may_wrap){ return evo::Result<core::GenericInt>::error(); }
				return evo::Result<core::GenericInt>(result.result);
			}
		);

		if(result.isError()){ return evo::resultError; }

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(
				this->sema_buffer.createIntValue(result.value(), this->type_manager.getTypeInfo(type_id).baseTypeID())
			)
		);
	}

	auto ComptimeIntrinsicEvaluator::bitReverse(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo {
		const bool is_unsigned = this->type_manager.isUnsignedIntegral(type_id);

		const unsigned type_width = [&](){
			const TypeInfo::ID underlying_id = this->type_manager.getUnderlyingType(type_id);
			const TypeInfo& underlying_type = this->type_manager.getTypeInfo(underlying_id);
			const BaseType::Primitive& primitive = this->type_manager.getPrimitive(
				underlying_type.baseTypeID().primitiveID()
			);
			return primitive.bitWidth();
		}();

		const core::GenericInt result = arg.extOrTrunc(type_width, is_unsigned).bitReverse();

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::byteSwap(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo {
		const bool is_unsigned = this->type_manager.isUnsignedIntegral(type_id);

		const unsigned type_width = [&](){
			const TypeInfo::ID underlying_id = this->type_manager.getUnderlyingType(type_id);
			const TypeInfo& underlying_type = this->type_manager.getTypeInfo(underlying_id);
			const BaseType::Primitive& primitive = this->type_manager.getPrimitive(
				underlying_type.baseTypeID().primitiveID()
			);
			return primitive.bitWidth();
		}();

		const core::GenericInt result = arg.extOrTrunc(type_width, is_unsigned).byteSwap();

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::ctPop(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo {
		const bool is_unsigned = this->type_manager.isUnsignedIntegral(type_id);

		const unsigned type_width = [&](){
			const TypeInfo::ID underlying_id = this->type_manager.getUnderlyingType(type_id);
			const TypeInfo& underlying_type = this->type_manager.getTypeInfo(underlying_id);
			const BaseType::Primitive& primitive = this->type_manager.getPrimitive(
				underlying_type.baseTypeID().primitiveID()
			);
			return primitive.bitWidth();
		}();

		const core::GenericInt result = arg.extOrTrunc(type_width, is_unsigned).ctPop();

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::ctlz(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo {
		const bool is_unsigned = this->type_manager.isUnsignedIntegral(type_id);

		const unsigned type_width = [&](){
			const TypeInfo::ID underlying_id = this->type_manager.getUnderlyingType(type_id);
			const TypeInfo& underlying_type = this->type_manager.getTypeInfo(underlying_id);
			const BaseType::Primitive& primitive = this->type_manager.getPrimitive(
				underlying_type.baseTypeID().primitiveID()
			);
			return primitive.bitWidth();
		}();

		const core::GenericInt result = arg.extOrTrunc(type_width, is_unsigned).ctlz();

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}

	auto ComptimeIntrinsicEvaluator::cttz(const TypeInfo::ID type_id, const core::GenericInt& arg) -> TermInfo {
		const bool is_unsigned = this->type_manager.isUnsignedIntegral(type_id);

		const unsigned type_width = [&](){
			const TypeInfo::ID underlying_id = this->type_manager.getUnderlyingType(type_id);
			const TypeInfo& underlying_type = this->type_manager.getTypeInfo(underlying_id);
			const BaseType::Primitive& primitive = this->type_manager.getPrimitive(
				underlying_type.baseTypeID().primitiveID()
			);
			return primitive.bitWidth();
		}();

		const core::GenericInt result = arg.extOrTrunc(type_width, is_unsigned).cttz();

		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL,
			TermInfo::ValueStage::COMPTIME,
			TermInfo::ValueState::NOT_APPLICABLE,
			type_id,
			sema::Expr(this->sema_buffer.createIntValue(result, this->type_manager.getTypeInfo(type_id).baseTypeID()))
		);
	}




	//////////////////////////////////////////////////////////////////////
	// intrin impl



	auto ComptimeIntrinsicEvaluator::add_wrap_impl(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt::WrapResult {
		return this->intrin_base_impl<core::GenericInt::WrapResult>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt::WrapResult {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.uadd(rhs);
				}else{
					return lhs.sadd(rhs);
				}
			}
		);
	}

	auto ComptimeIntrinsicEvaluator::sub_wrap_impl(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt::WrapResult {
		return this->intrin_base_impl<core::GenericInt::WrapResult>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt::WrapResult {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.usub(rhs);
				}else{
					return lhs.ssub(rhs);
				}
			}
		);
	}

	auto ComptimeIntrinsicEvaluator::mul_wrap_impl(
		const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs
	) -> core::GenericInt::WrapResult {
		return this->intrin_base_impl<core::GenericInt::WrapResult>(type_id, lhs, rhs,
			[&](const TypeInfo::ID type_id, const core::GenericInt& lhs, const core::GenericInt& rhs) 
				-> core::GenericInt::WrapResult {
				if(this->type_manager.isUnsignedIntegral(type_id)){
					return lhs.umul(rhs);
				}else{
					return lhs.smul(rhs);
				}
			}
		);
	}







	template<class RETURN>
	auto ComptimeIntrinsicEvaluator::intrin_base_impl(
		const TypeInfo::ID type_id,
		const core::GenericInt& lhs,
		const core::GenericInt& rhs,
		IntrinOp<RETURN> intrin_op
	) -> RETURN {
		const bool is_unsigned = this->type_manager.isUnsignedIntegral(type_id);

		const unsigned type_width = [&](){
			const TypeInfo::ID underlying_id = this->type_manager.getUnderlyingType(type_id);
			const TypeInfo& underlying_type = this->type_manager.getTypeInfo(underlying_id);
			const BaseType::Primitive& primitive = this->type_manager.getPrimitive(
				underlying_type.baseTypeID().primitiveID()
			);
			return primitive.bitWidth();
		}();
		

		const core::GenericInt& lhs_converted = lhs.extOrTrunc(type_width, is_unsigned);
		const core::GenericInt& rhs_converted = rhs.extOrTrunc(type_width, is_unsigned);

		return intrin_op(type_id, lhs_converted, rhs_converted);
	}


	template<class RETURN>
	auto ComptimeIntrinsicEvaluator::intrin_base_impl(
		const TypeInfo::ID type_id,
		const core::GenericFloat& lhs,
		const core::GenericFloat& rhs,
		std::function<RETURN(const core::GenericFloat&, const core::GenericFloat&)> intrin_op
	) -> RETURN {
		const TypeInfo& type = this->type_manager.getTypeInfo(type_id);
		const BaseType::Primitive& primitive = this->type_manager.getPrimitive(type.baseTypeID().primitiveID());

		auto lhs_converted = std::optional<core::GenericFloat>();
		auto rhs_converted = std::optional<core::GenericFloat>();
		switch(primitive.kind()){
			case Token::Kind::TYPE_F16: {
				lhs_converted = lhs.asF16();
				rhs_converted = rhs.asF16();
			} break;

			case Token::Kind::TYPE_BF16: {
				lhs_converted = lhs.asBF16();
				rhs_converted = rhs.asBF16();
			} break;

			case Token::Kind::TYPE_F32: {
				lhs_converted = lhs.asF32();
				rhs_converted = rhs.asF32();
			} break;

			case Token::Kind::TYPE_F64: {
				lhs_converted = lhs.asF64();
				rhs_converted = rhs.asF64();
			} break;

			case Token::Kind::TYPE_F80: {
				lhs_converted = lhs.asF80();
				rhs_converted = rhs.asF80();
			} break;

			case Token::Kind::TYPE_F128: {
				lhs_converted = lhs.asF128();
				rhs_converted = rhs.asF128();
			} break;

			default: evo::debugFatalBreak("Unknown or unsupported float type");
		}

		return intrin_op(*lhs_converted, *rhs_converted);
	}


}