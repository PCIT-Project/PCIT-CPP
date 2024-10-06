//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

#pragma once

#include <Evo.h>


#include "./GenericInt.h"
#include "./GenericFloat.h"


namespace pcit::core{


	using GenericValue = evo::Variant<GenericInt, GenericFloat, bool, char>;


	// class GenericValue{
	// 	public:
	// 		explicit GenericValue(GenericInt&& val)   : value(std::move(val)) {}
	// 		explicit GenericValue(GenericFloat&& val) : value(std::move(val)) {}
	// 		explicit GenericValue(bool val)           : value(std::move(val)) {}
	// 		explicit GenericValue(char val)           : value(std::move(val)) {}
	// 		~GenericValue() = default;


	// 		EVO_NODISCARD auto operator==(const GenericValue& rhs) const -> bool { return this->value == rhs.value; }

	
	// 	private:
	// 		evo::Variant<GenericInt, GenericFloat, bool, char> value;
	// };

	
}
