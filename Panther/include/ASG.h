//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./AST.h"
#include "./TypeManager.h"


namespace pcit::panther::ASG{

	class FuncID : public core::UniqueID<uint32_t, class FuncID> {
		public:
			using core::UniqueID<uint32_t, FuncID>::UniqueID;
	};

	using Parent = evo::Variant<std::monostate, FuncID>;

	struct Func{
		using ID = FuncID;

		struct LinkID{
			LinkID(SourceID source_id, ID func_id) : _source_id(source_id), _func_id(func_id) {}

			EVO_NODISCARD auto sourceID() const -> SourceID { return this->_source_id; }
			EVO_NODISCARD auto funcID() const -> ID { return this->_func_id; }

			EVO_NODISCARD auto operator==(const LinkID& rhs) const -> bool {
				return this->_source_id == rhs._source_id && this->_func_id == rhs._func_id;
			}

			EVO_NODISCARD auto operator!=(const LinkID& rhs) const -> bool {
				return this->_source_id != rhs._source_id || this->_func_id != rhs._func_id;
			}
			
			private:
				SourceID _source_id;
				ID _func_id;
		};



		AST::Node name;
		BaseType::ID baseTypeID;
		Parent parent;
	};

}


template<>
struct std::hash<pcit::panther::ASG::Func::LinkID>{
	auto operator()(const pcit::panther::ASG::Func::LinkID& link_id) const noexcept -> size_t {
		auto hasher = std::hash<uint32_t>{};
		return evo::hashCombine(hasher(link_id.sourceID().get()), hasher(link_id.funcID().get()));
	};
};

