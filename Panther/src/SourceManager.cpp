//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/SourceManager.h"

namespace pcit::panther{


	auto SourceManager::addSource(const std::string& location, const std::string& data) -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.emplace_back(Source(new_source_id, location, data));
		return new_source_id;
	}

	auto SourceManager::addSource(const std::string& location, std::string&& data) -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.emplace_back(Source(new_source_id, location, std::move(data)));
		return new_source_id;
	}

	auto SourceManager::addSource(std::string&& location, const std::string& data) -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.emplace_back(Source(new_source_id, std::move(location), data));
		return new_source_id;
	}

	auto SourceManager::addSource(std::string&& location, std::string&& data) -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.emplace_back(Source(new_source_id, std::move(location), std::move(data)));
		return new_source_id;
	}


	auto SourceManager::addSource(const fs::path& location, const std::string& data) -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.emplace_back(Source(new_source_id, location, data));
		return new_source_id;
	}

	auto SourceManager::addSource(const fs::path& location, std::string&& data) -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.emplace_back(Source(new_source_id, location, std::move(data)));
		return new_source_id;
	}

	auto SourceManager::addSource(fs::path&& location, const std::string& data) -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.emplace_back(Source(new_source_id, std::move(location), data));
		return new_source_id;
	}

	auto SourceManager::addSource(fs::path&& location, std::string&& data) -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.emplace_back(Source(new_source_id, std::move(location), std::move(data)));
		return new_source_id;
	}




	auto SourceManager::getSource(Source::ID id) -> Source& {
		return this->sources[id];
	}

	auto SourceManager::getSource(Source::ID id) const -> const Source& {
		return this->sources[id];
	}



}