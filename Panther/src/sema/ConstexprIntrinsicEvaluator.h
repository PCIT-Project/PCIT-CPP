////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "../../include/TypeManager.h"
#include "./TermInfo.h"


namespace pcit::panther{


	class ConstexprIntrinsicEvaluator{
		public:
			ConstexprIntrinsicEvaluator(TypeManager& _type_manager, class SemaBuffer& _sema_buffer)
				: type_manager(_type_manager), sema_buffer(_sema_buffer) {}

			~ConstexprIntrinsicEvaluator() = default;


			EVO_NODISCARD auto sizeOf(TypeInfo::ID type_id) -> TermInfo;

	
		private:
			TypeManager& type_manager;
			class SemaBuffer& sema_buffer;
	};


}
