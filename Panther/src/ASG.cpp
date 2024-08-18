//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


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