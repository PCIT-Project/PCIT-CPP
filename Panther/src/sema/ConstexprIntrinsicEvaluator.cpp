////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./ConstexprIntrinsicEvaluator.h"

#include "../../include/sema/SemaBuffer.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther{

	
	auto ConstexprIntrinsicEvaluator::sizeOf(TypeInfo::ID type_id) -> TermInfo {
		return TermInfo(
			TermInfo::ValueCategory::EPHEMERAL, 
			TermInfo::ValueStage::CONSTEXPR,
			TypeManager::getTypeUSize(),
			sema::Expr(this->sema_buffer.createIntValue(
				core::GenericInt(unsigned(this->type_manager.sizeOfPtr()), this->type_manager.sizeOf(type_id)),
				this->type_manager.getTypeInfo(TypeManager::getTypeUSize()).baseTypeID()
			))
		);
	}


}