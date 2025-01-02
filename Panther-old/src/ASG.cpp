////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/ASG.h"

#include "../include/Source.h"

namespace pcit::panther::ASG{
	

	auto TemplatedFunc::lookupInstance(evo::SmallVector<Arg>&& args) -> LookupInfo {
		const auto lock = std::lock_guard(this->instance_lock);
		
		for(uint32_t i = 0; std::unique_ptr<Instatiation>& instatiation : this->instantiations){
			if(instatiation->args == args){
				return LookupInfo(false, ASG::Func::InstanceID(i), instatiation->id);
			}

			i += 1;
		}

		const auto instance_id = ASG::Func::InstanceID(uint32_t(this->instantiations.size()));
		const std::unique_ptr<Instatiation>& new_instantiation = this->instantiations.emplace_back(
			std::make_unique<Instatiation>()
		);
		new_instantiation->args = std::move(args);
		return LookupInfo(true, instance_id, new_instantiation->id);
	}


}