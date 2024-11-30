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
		Default, // C

		C,
		Fast,
		Cold,
	};


	enum class Linkage{
		Default, // Internal

		Private,
		Internal, // Like private, but shows up as a local symbol in the object file
		External,
	};


	// Note: if you want a good default (that's not None), use SequentiallyConsistent
	struct AtomicOrdering{
		enum class Value{
			None, // not atomic
			Monotonic, // allows reordering of reads or writes
			Acquire, // (only load) no reordering of reads or writes to before
			Release, // (only store) no reordering of reads or writes to after
			AcquireRelease, // (only read-modify-write) an acquire and release 
			                //   only works if both threads use the same atomic variable
			SequentiallyConsistent, // load = aquire, store = release, rmw = AcquireRelease 
				                    //   however it works with multiple atomic variables
			                        //   (slower than raw acquire, release, AcquireRelease)
		};
		using enum class Value;

		constexpr AtomicOrdering(const Value& value) : _value(value) {}
		EVO_NODISCARD constexpr operator Value() const { return this->_value; }

		EVO_NODISCARD constexpr auto isValidForLoad() const -> bool {
			switch(*this){
				case AtomicOrdering::None:                   return true;
				case AtomicOrdering::Monotonic:              return true;
				case AtomicOrdering::Acquire:                return true;
				case AtomicOrdering::Release:                return false;
				case AtomicOrdering::AcquireRelease:         return false;
				case AtomicOrdering::SequentiallyConsistent: return true;
			}

			evo::unreachable();
		}

		EVO_NODISCARD constexpr auto isValidForStore() const -> bool {
			switch(*this){
				case AtomicOrdering::None:                   return true;
				case AtomicOrdering::Monotonic:              return true;
				case AtomicOrdering::Acquire:                return false;
				case AtomicOrdering::Release:                return true;
				case AtomicOrdering::AcquireRelease:         return false;
				case AtomicOrdering::SequentiallyConsistent: return true;
			}

			evo::unreachable();
		}

		private:
			Value _value;
	};



	enum class OptMode{
		None,
		O0 = None,
		O1,
		O2,
		O3,
		Os,
		Oz,
	};

}

