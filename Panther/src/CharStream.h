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



namespace pcit::panther{


	class CharStream{
		public:
			CharStream(std::string_view src_data) noexcept : data(src_data) {};
			~CharStream() = default;

			EVO_NODISCARD auto peek(size_t ammount_forward = 0) const noexcept -> char;
			EVO_NODISCARD auto peek_raw_ptr() const noexcept -> const char*;
			EVO_NODISCARD auto next() noexcept -> char;
			auto skip(size_t ammount) noexcept -> void;

			EVO_NODISCARD auto at_end() const noexcept -> bool { return this->cursor == this->data.size(); };
			EVO_NODISCARD auto ammount_left() const noexcept -> size_t { return this->data.size() - this->cursor; };

			EVO_NODISCARD auto get_line() const noexcept -> uint32_t { return this->line; };
			EVO_NODISCARD auto get_collumn() const noexcept -> uint16_t { return this->collumn; };

		private:
			EVO_NODISCARD auto skip_single() noexcept -> void;
	
		private:
			std::string_view data;
			size_t cursor = 0;

			uint32_t line = 1;
			uint16_t collumn = 1;
	};


};
