//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
//                                                                  //
// Source Manager													//
// 		manages the sources, and guarantees that references to		//
// 		sources are stable											//
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
			SourceManager() noexcept = default;
			~SourceManager() noexcept;

			SourceManager(const SourceManager&) = delete;
			auto operator=(const SourceManager&) = delete;

			// make sure enough space for `num_source` source is allocated
			auto reserveSources(size_t num_source) noexcept -> void;

			auto addSource(const std::string& location, const std::string& data) noexcept -> Source::ID;
			auto addSource(const std::string& location, std::string&& data) noexcept -> Source::ID;
			auto addSource(std::string&& location, const std::string& data) noexcept -> Source::ID;
			auto addSource(std::string&& location, std::string&& data) noexcept -> Source::ID;

			auto addSource(const fs::path& location, const std::string& data) noexcept -> Source::ID;
			auto addSource(const fs::path& location, std::string&& data) noexcept -> Source::ID;
			auto addSource(fs::path&& location, const std::string& data) noexcept -> Source::ID;
			auto addSource(fs::path&& location, std::string&& data) noexcept -> Source::ID;


			EVO_NODISCARD auto getSource(Source::ID id)       noexcept ->       Source&;
			EVO_NODISCARD auto getSource(Source::ID id) const noexcept -> const Source&;

			EVO_NODISCARD auto operator[](Source::ID id)       noexcept ->       Source& {return this->getSource(id);};
			EVO_NODISCARD auto operator[](Source::ID id) const noexcept -> const Source& {return this->getSource(id);};

			EVO_NODISCARD auto numSources() const noexcept -> size_t { return this->sources.size(); };

			EVO_NODISCARD auto begin() const noexcept -> Source::ID::Iterator {
				return Source::ID::Iterator(Source::ID(0));
			};

			EVO_NODISCARD auto end() const noexcept -> Source::ID::Iterator {
				return Source::ID::Iterator(Source::ID(uint32_t(this->sources.size())));
			};

		private:
			auto alloc_source(auto&&... args) noexcept -> Source*;
			auto free_source(Source* source) noexcept -> void;

	
		private:
			std::vector<Source*> sources{};

			friend class Context;
	};


};