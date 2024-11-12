////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Evo.h>

#include <iterator>


namespace pcit::core{


	template<class T>
	class IterRange{
		public:
			IterRange(const T& begin_iter, const T& end_iter) : _begin(begin_iter), _end(end_iter) {};
			~IterRange() = default;
		
			EVO_NODISCARD auto begin() const -> const T& { return this->_begin; }
			EVO_NODISCARD auto end() const -> const T& { return this->_end; }

			EVO_NODISCARD auto size() const -> size_t { return std::distance(this->_begin, this->_end); }


		private:
			T _begin;
			T _end;
	};
	
}
