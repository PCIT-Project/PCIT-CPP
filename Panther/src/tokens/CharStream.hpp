////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>
#include <PCIT_core.hpp>



namespace pcit::panther{


	class CharStream{
		public:
			CharStream(std::string_view src_data) : data(src_data) {}
			~CharStream() = default;

			[[nodiscard]] auto peek(size_t ammount_forward = 0) const -> char;
			[[nodiscard]] auto peek_back(size_t ammount_backward = 1) const -> char;
			[[nodiscard]] auto peek_raw_ptr() const -> const char*;
			[[nodiscard]] auto next() -> char;
			auto skip(size_t ammount) -> void;

			[[nodiscard]] auto at_end() const -> bool { return this->cursor == this->data.size(); }
			[[nodiscard]] auto ammount_left() const -> size_t { return this->data.size() - this->cursor; }

			[[nodiscard]] auto get_line() const -> evo::Result<uint32_t> {
				if(this->line > std::numeric_limits<uint32_t>::max()) [[unlikely]] { return evo::resultError; }
				return uint32_t(this->line);
			}
			[[nodiscard]] auto get_collumn() const -> evo::Result<uint32_t> {
				if(this->collumn > std::numeric_limits<uint32_t>::max()) [[unlikely]] { return evo::resultError; }
				return uint32_t(this->collumn);
			}

		private:
			[[nodiscard]] auto skip_single() -> void;
	
		private:
			std::string_view data;
			size_t cursor = 0;

			uint64_t line = 1;
			uint64_t collumn = 1;
	};


}
