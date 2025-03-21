////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


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
			SourceManager() = default;
			~SourceManager() = default;

			SourceManager(const SourceManager&) = delete;
			auto operator=(const SourceManager&) = delete;

			auto addSource(const std::string& location, const std::string& data) -> Source::ID;
			auto addSource(const std::string& location, std::string&& data) -> Source::ID;
			auto addSource(std::string&& location, const std::string& data) -> Source::ID;
			auto addSource(std::string&& location, std::string&& data) -> Source::ID;

			auto addSource(const fs::path& location, const std::string& data) -> Source::ID;
			auto addSource(const fs::path& location, std::string&& data) -> Source::ID;
			auto addSource(fs::path&& location, const std::string& data) -> Source::ID;
			auto addSource(fs::path&& location, std::string&& data) -> Source::ID;


			EVO_NODISCARD auto getSource(Source::ID id)       ->       Source&;
			EVO_NODISCARD auto getSource(Source::ID id) const -> const Source&;

			EVO_NODISCARD auto operator[](Source::ID id)       ->       Source& { return this->getSource(id); }
			EVO_NODISCARD auto operator[](Source::ID id) const -> const Source& { return this->getSource(id); }

			EVO_NODISCARD auto numSources() const -> size_t { return this->sources.size(); }

			EVO_NODISCARD auto begin() const -> Source::ID::Iterator {
				return Source::ID::Iterator(Source::ID(0));
			}

			EVO_NODISCARD auto end() const -> Source::ID::Iterator {
				return Source::ID::Iterator(Source::ID(uint32_t(this->sources.size())));
			}
	
		private:
			core::LinearStepAlloc<Source, Source::ID, 0> sources{};
	};


}