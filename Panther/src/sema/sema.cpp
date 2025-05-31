////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/sema/sema.h"

#include "../../include/Context.h"


namespace pcit::panther::sema{


	auto Func::isEquivalentOverload(const Func& rhs, const Context& context) const -> bool {
		const BaseType::Function& this_type = context.getTypeManager().getFunction(this->typeID);
		const BaseType::Function& rhs_type = context.getTypeManager().getFunction(rhs.typeID);

		if(this->minNumArgs == rhs.minNumArgs){
			for(size_t i = 0; i < this->minNumArgs; i+=1){
				if(this_type.params[i].typeID != rhs_type.params[i].typeID){ return false; }
				if(this_type.params[i].kind != rhs_type.params[i].kind){ return false; }
			}
			
		}else{
			if(this_type.params.size() != rhs_type.params.size()){ return false; }

			for(size_t i = 0; i < this_type.params.size(); i+=1){
				if(this_type.params[i].typeID != rhs_type.params[i].typeID){ return false; }
				if(this_type.params[i].kind != rhs_type.params[i].kind){ return false; }
			}
		}

		return true;
	}


	auto Func::isMethod(const Context& context) const -> bool {
		if(this->params.empty()){ return false; }

		const Source& source = context.getSourceManager()[this->sourceID];
		const Token& first_token = source.getTokenBuffer()[this->params[0].ident];
		return first_token.kind() == Token::Kind::KEYWORD_THIS;
	}




	auto TemplatedFunc::lookupInstantiation(evo::SmallVector<Arg>&& args) -> InstantiationInfo {
		const auto lock = std::scoped_lock(this->instantiation_lock);

		auto find = this->instantiation_map.find(args);
		if(find == this->instantiation_map.end()){
			const uint32_t instantiation_id = uint32_t(this->instantiations.size());
			Instantiation& new_instantiation = this->instantiations[this->instantiations.emplace_back()];
			this->instantiation_map.emplace(std::move(args), new_instantiation);
			return InstantiationInfo(new_instantiation, instantiation_id);

		}else{
			return InstantiationInfo(find->second, std::nullopt);
		}
	}


}