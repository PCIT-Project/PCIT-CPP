//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include <Evo.h>
#include <PCIT_core.h>

#include "./Source.h"

namespace pcit::panther{


	class SourceManager{
		public:
			SourceManager() = default;
			~SourceManager() = default;


			auto lock() noexcept -> void { this->locked = true; };
			EVO_NODISCARD auto isLocked() const noexcept -> bool { return this->locked; };

			// make sure enough space for `num_source` source is allocated
			auto reserveSources(size_t num_source) noexcept -> void;

			EVO_NODISCARD auto addSource(const std::string& location, const std::string& data) noexcept -> Source::ID;
			EVO_NODISCARD auto addSource(const std::string& location, std::string&& data) noexcept -> Source::ID;
			EVO_NODISCARD auto addSource(std::string&& location, const std::string& data) noexcept -> Source::ID;
			EVO_NODISCARD auto addSource(std::string&& location, std::string&& data) noexcept -> Source::ID;

			EVO_NODISCARD auto addSource(const fs::path& location, const std::string& data) noexcept -> Source::ID;
			EVO_NODISCARD auto addSource(const fs::path& location, std::string&& data) noexcept -> Source::ID;
			EVO_NODISCARD auto addSource(fs::path&& location, const std::string& data) noexcept -> Source::ID;
			EVO_NODISCARD auto addSource(fs::path&& location, std::string&& data) noexcept -> Source::ID;


			EVO_NODISCARD auto getSource(Source::ID id)       noexcept ->       Source&;
			EVO_NODISCARD auto getSource(Source::ID id) const noexcept -> const Source&;

	
		private:
			std::vector<Source> sources{};

			bool locked = false;
	};


};