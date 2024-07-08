//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "../include/SourceManager.h"

namespace pcit::panther{


	SourceManager::~SourceManager() noexcept {
		for(Source* source : this->sources){
			this->free_source(source);
		}
	}


	auto SourceManager::reserveSources(size_t num_sources) noexcept -> void {
		this->sources.reserve(num_sources);
	};


	// TODO: create custom vector impl to not have to do a push_back
	// 		(can't do a emplace_back because I want Source::Source() to be private)
	auto SourceManager::addSource(const std::string& location, const std::string& data) noexcept -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.push_back(this->alloc_source(new_source_id, location, data));
		return new_source_id;
	};

	auto SourceManager::addSource(const std::string& location, std::string&& data) noexcept -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.push_back(this->alloc_source(new_source_id, location, std::move(data)));
		return new_source_id;
	};

	auto SourceManager::addSource(std::string&& location, const std::string& data) noexcept -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.push_back(this->alloc_source(new_source_id, std::move(location), data));
		return new_source_id;
	};

	auto SourceManager::addSource(std::string&& location, std::string&& data) noexcept -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.push_back(this->alloc_source(new_source_id, std::move(location), std::move(data)));
		return new_source_id;
	};


	auto SourceManager::addSource(const fs::path& location, const std::string& data) noexcept -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.push_back(this->alloc_source(new_source_id, location, data));
		return new_source_id;
	};

	auto SourceManager::addSource(const fs::path& location, std::string&& data) noexcept -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.push_back(this->alloc_source(new_source_id, location, std::move(data)));
		return new_source_id;
	};

	auto SourceManager::addSource(fs::path&& location, const std::string& data) noexcept -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.push_back(this->alloc_source(new_source_id, std::move(location), data));
		return new_source_id;
	};

	auto SourceManager::addSource(fs::path&& location, std::string&& data) noexcept -> Source::ID {
		const Source::ID new_source_id = Source::ID(uint32_t(this->sources.size()));
		this->sources.push_back(this->alloc_source(new_source_id, std::move(location), std::move(data)));
		return new_source_id;
	};



	
	auto SourceManager::getSource(Source::ID id) noexcept -> Source& {
		return *this->sources[id.get()];
	};

	auto SourceManager::getSource(Source::ID id) const noexcept -> const Source& {
		return *this->sources[id.get()];
	};



	// TODO: better allocation method
	auto SourceManager::alloc_source(auto&&... args) noexcept -> Source* {
		return new Source(std::forward<decltype(args)>(args)...);
	};

	auto SourceManager::free_source(Source* source) noexcept -> void {
		delete source;
	};

};