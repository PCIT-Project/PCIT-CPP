//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0      //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

namespace pcit::core{


	template<class BaseT, class SelfReference>
	class UniqueID{
		public:
			explicit constexpr UniqueID(const BaseT& id) noexcept : internal_id(id) {};
			constexpr ~UniqueID() noexcept = default;


			EVO_NODISCARD constexpr auto get() const noexcept -> BaseT { return this->internal_id; };
	
		private:
			BaseT internal_id;
	};


	template<class BaseT, class SelfReference>
	class UniqueComparableID{
		public:
			using ThisT = UniqueComparableID<BaseT, SelfReference>;

			explicit constexpr UniqueComparableID(const BaseT& id) noexcept : internal_id(id) {};
			constexpr ~UniqueComparableID() noexcept = default;


			EVO_NODISCARD constexpr auto operator==(const ThisT& rhs) const noexcept -> bool {
				return this->get() == rhs.get();
			};

			EVO_NODISCARD constexpr auto operator!=(const ThisT& rhs) const noexcept -> bool {
				return this->get() != rhs.get();
			};


			EVO_NODISCARD constexpr auto get() const noexcept -> BaseT { return this->internal_id; };
	
		private:
			BaseT internal_id;
	};

	
	

};