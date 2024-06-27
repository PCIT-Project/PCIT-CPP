//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/Context.h"

namespace pcit::panther{
	

	auto Context::emit_diagnostic_impl(const Diagnostic& diagnostic) noexcept -> void {
		const auto lock_guard = std::lock_guard<std::mutex>(this->callback_mutex);

		this->callback(*this, diagnostic);
	};


};