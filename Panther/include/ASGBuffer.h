//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <deque>

#include <Evo.h>
#include <PCIT_core.h>

#include "./ASG.h"


namespace pcit::panther{


	class ASGBuffer{
		public:
			ASGBuffer() = default;
			~ASGBuffer() = default;

			auto createFunc(auto&&... args) -> ASG::Func::ID {
				const auto created_id = ASG::Func::ID(uint32_t(this->funcs.size()));
				this->funcs.emplace_back(std::forward<decltype(args)>(args)...);
				return created_id;
			}

			EVO_NODISCARD auto getFunc(ASG::Func::ID id) const -> const ASG::Func& { return this->funcs[id.get()]; }
			EVO_NODISCARD auto getFunc(ASG::Func::ID id)       ->       ASG::Func& { return this->funcs[id.get()]; }

			EVO_NODISCARD auto getFuncs() const -> core::IterRange<ASG::Func::ID::Iterator> {
				return core::IterRange<ASG::Func::ID::Iterator>(
					ASG::Func::ID::Iterator(ASG::Func::ID(0)),
					ASG::Func::ID::Iterator(ASG::Func::ID(uint32_t(this->funcs.size())))
				);
			};

			EVO_NODISCARD auto numFuncs() const -> size_t { return this->funcs.size(); }
	
		private:
			evo::SmallVector<ASG::Func> funcs{};
	};


}
