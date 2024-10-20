//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "../include/ASG.h"
#include "../include/ASGBuffer.h"


namespace pcit::panther{


	class ComptimeExecutor{
		public:
			ComptimeExecutor(class Context& _context, core::Printer& _printer) : context(_context), printer(_printer) {}
			~ComptimeExecutor() = default;

			auto init() -> std::string; // string is error message if initalization fails (empty if successful)
			auto deinit() -> void;

			EVO_NODISCARD auto isInitialized() const -> bool { return this->data != nullptr; }

			EVO_NODISCARD auto runFunc(
				const ASG::Func::LinkID& link_id, evo::ArrayProxy<ASG::Expr> params, ASGBuffer& asg_buffer
			) -> evo::SmallVector<ASG::Expr>;


			auto addFunc(const ASG::Func::LinkID& func_link_id) -> void;

		private:
			auto restart_engine_if_needed() -> void;

		private:
			class Context& context;
			core::Printer& printer;

			mutable std::shared_mutex mutex{};
			bool requires_engine_restart = false;

			struct Data;
			Data* data = nullptr;
	};


}
