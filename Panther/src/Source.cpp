////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Source.h"

namespace pcit::panther{
	

	auto Source::locationIsPath() const -> bool {
		return this->location.is<fs::path>();
	}

	auto Source::locationIsString() const -> bool {
		return this->location.is<std::string>();
	}



	auto Source::getLocationPath() const -> const fs::path& {
		evo::debugAssert(this->locationIsPath(), "Path location is not a path");
		return this->location.as<fs::path>();
	}

	auto Source::getLocationString() const -> const std::string& {
		evo::debugAssert(this->locationIsString(), "Path location is not a string");
		return this->location.as<std::string>();
	}

	auto Source::getLocationAsString() const -> std::string {
		if(this->location.is<std::string>()){
			return this->location.as<std::string>();
		}else{
			return this->location.as<fs::path>().string();
		}
	}


	auto Source::getLocationAsPath() const -> fs::path {
		if(this->location.is<std::string>()){
			return this->location.as<std::string>();
		}else{
			return this->location.as<fs::path>();
		}
	}


}