//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////

#pragma once

#include <Evo.h>

namespace pcit::core{


	template<class T>
	class IterRange{
		public:
			IterRange(const T& begin_iter, const T& end_iter) : _begin(begin_iter), _end(end_iter) {};
			~IterRange() = default;
		
			EVO_NODISCARD auto begin() const -> const T& { return this->_begin; }
			EVO_NODISCARD auto end() const -> const T& { return this->_end; }


		private:
			T _begin;
			T _end;
	};
	
}
