//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/Source.h"

namespace pcit::panther{
	

	auto Source::locationIsPath() const noexcept -> bool {
		return this->location.is<fs::path>();
	};

	auto Source::locationIsString() const noexcept -> bool {
		return this->location.is<std::string>();
	};



	auto Source::getLocationPath() const noexcept -> const fs::path& {
		evo::debugAssert(this->locationIsPath(), "Path location is not a path");
		return this->location.as<fs::path>();
	};

	auto Source::getLocationString() const noexcept -> const std::string& {
		evo::debugAssert(this->locationIsString(), "Path location is not a string");
		return this->location.as<std::string>();
	};

	auto Source::getLocationAsString() const noexcept -> std::string {
		if(this->location.is<std::string>()){
			return this->location.as<std::string>();
		}else{
			return this->location.as<fs::path>().string();
		}
	};


};