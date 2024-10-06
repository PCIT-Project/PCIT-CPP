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



namespace pcit::panther{


	class CharStream{
		public:
			CharStream(std::string_view src_data) : data(src_data) {}
			~CharStream() = default;

			EVO_NODISCARD auto peek(size_t ammount_forward = 0) const -> char;
			EVO_NODISCARD auto peek_raw_ptr() const -> const char*;
			EVO_NODISCARD auto next() -> char;
			auto skip(size_t ammount) -> void;

			EVO_NODISCARD auto at_end() const -> bool { return this->cursor == this->data.size(); }
			EVO_NODISCARD auto ammount_left() const -> size_t { return this->data.size() - this->cursor; }

			EVO_NODISCARD auto get_line() const -> evo::Result<uint32_t> {
				if(this->line > std::numeric_limits<uint32_t>::max()) [[likely]] { return evo::resultError; }
				return uint32_t(this->line);
			}
			EVO_NODISCARD auto get_collumn() const -> evo::Result<uint32_t> {
				if(this->collumn > std::numeric_limits<uint32_t>::max()) [[likely]] { return evo::resultError; }
				return uint32_t(this->collumn);
			}

		private:
			EVO_NODISCARD auto skip_single() -> void;
	
		private:
			std::string_view data;
			size_t cursor = 0;

			uint64_t line = 1;
			uint64_t collumn = 1;
	};


}
