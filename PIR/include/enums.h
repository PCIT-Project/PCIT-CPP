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



namespace pcit::pir{

	enum class CallingConvention{
		DEFAULT, // FAST

		FAST,
		COLD,
		C,
	};


	enum class Linkage{
		DEFAULT, // Internal

		INTERNAL,
		PRIVATE, // Like internal, but doesn't shows up as a local symbol in the object file
		EXTERNAL,
		WEAK,
		WEAK_EXTERNAL,
	};


	// Note: if you want a good default (that's not NONE), use SEQUENTIALLY_CONSISTENT
	struct AtomicOrdering{
		enum class Value{
			NONE, // not atomic
			MONOTONIC, // allows reordering of reads or writes
			ACQUIRE, // (only load) no reordering of reads or writes to before
			RELEASE, // (only store) no reordering of reads or writes to after
			ACQUIRE_RELEASE, // (only read-modify-write) an acquire and release 
			                 //   only works if both threads use the same atomic variable
			SEQUENTIALLY_CONSISTENT, // load = aquire, store = release, rmw = AcquireRelease 
				                     //   however it works with multiple atomic variables
			                         //   (slower than raw acquire, release, AcquireRelease)
		};
		using enum class Value;

		constexpr AtomicOrdering(const Value& value) : _value(value) {}
		EVO_NODISCARD constexpr operator Value() const { return this->_value; }

		EVO_NODISCARD constexpr auto isValidForLoad() const -> bool {
			switch(*this){
				case AtomicOrdering::NONE:                    return true;
				case AtomicOrdering::MONOTONIC:               return true;
				case AtomicOrdering::ACQUIRE:                 return true;
				case AtomicOrdering::RELEASE:                 return false;
				case AtomicOrdering::ACQUIRE_RELEASE:         return false;
				case AtomicOrdering::SEQUENTIALLY_CONSISTENT: return true;
			}

			evo::unreachable();
		}

		EVO_NODISCARD constexpr auto isValidForStore() const -> bool {
			switch(*this){
				case AtomicOrdering::NONE:                    return true;
				case AtomicOrdering::MONOTONIC:               return true;
				case AtomicOrdering::ACQUIRE:                 return false;
				case AtomicOrdering::RELEASE:                 return true;
				case AtomicOrdering::ACQUIRE_RELEASE:         return false;
				case AtomicOrdering::SEQUENTIALLY_CONSISTENT: return true;
			}

			evo::unreachable();
		}

		private:
			Value _value;
	};



	enum class OptMode{
		NONE,
		O0 = NONE,
		O1,
		O2,
		O3,
		Os,
		Oz,
	};

}

