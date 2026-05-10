////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/source/SourceManager.hpp"



#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{
	

	auto SourceManager::lookupSpecialNameSourceID(std::string_view path) const -> std::optional<Source::ID> {
		const auto special_name_path_find = this->priv.special_name_paths.find(path);

		if(special_name_path_find == this->priv.special_name_paths.end()){ return std::nullopt; }

		// TODO(PERF): optimize this
		// look for path
		for(const Source::ID source_id : this->getSourceIDRange()){
			const Source& source = this->operator[](source_id);
			if(source.getPath() == special_name_path_find->second){ return source_id; }
		}

		evo::debugFatalBreak(
			"Special name source existed, but then couldn't find it (\"{}\" -> \"{}\")",
			path, special_name_path_find->second.string()
		);
	}


	// TODO(PERF): improve lookup times with a map maybe?
	auto SourceManager::lookupSourceID(std::string_view path) const	-> std::optional<Source::ID> {
		const auto file_path = std::filesystem::path(path);

		// look for path
		for(const Source::ID source_id : this->getSourceIDRange()){
			const Source& source = this->operator[](source_id);
			if(source.getPath() == file_path){ return source_id; }
		}

		return std::nullopt;
	}


	// TODO(PERF): improve lookup times with a map maybe?
	auto SourceManager::lookupCFamilySourceID(std::string_view path) const -> std::optional<CFamilySource::ID> {
		const auto file_path = std::filesystem::path(path);

		const auto c_family_source_id_range = [&](){
			const auto lock = std::scoped_lock(this->priv.c_family_sources_lock);
			return evo::IterRange<CFamilySource::ID::Iterator>(
				CFamilySource::ID::Iterator(CFamilySource::ID(0)),
				CFamilySource::ID::Iterator(CFamilySource::ID(uint32_t(this->priv.c_family_sources.size())))
			);
		}();

		// look for path
		for(const CFamilySource::ID c_family_source_id : c_family_source_id_range){
			const CFamilySource& c_family_source = this->operator[](c_family_source_id);
			if(c_family_source.getPath() == file_path){ return c_family_source_id; }
		}

		return std::nullopt;
	}



	// TODO(PERF): improve lookup times with a map maybe?
	auto SourceManager::getOrCreateCFamilySourceID(std::filesystem::path&& path, bool is_cpp)
	-> GottenCFamilySourceID {
		const auto lock = std::scoped_lock(this->priv.c_family_sources_lock);

		const auto c_family_source_id_range = evo::IterRange<CFamilySource::ID::Iterator>(
			CFamilySource::ID::Iterator(CFamilySource::ID(0)),
			CFamilySource::ID::Iterator(CFamilySource::ID(uint32_t(this->priv.c_family_sources.size())))
		);

		for(const CFamilySource::ID c_family_source_id : c_family_source_id_range){
			const CFamilySource& c_family_source = this->operator[](c_family_source_id);
			if(c_family_source.getPath() == path){ return GottenCFamilySourceID(c_family_source_id, false); }
		}

		
		evo::Result<std::string> file_data = evo::fs::readFile(path.string());
		evo::debugAssert(file_data.isSuccess(), "File doesn't exist");


		const CFamilySource::ID new_source_id = this->priv.c_family_sources.emplace_back(
			std::move(path), std::move(file_data.value()), is_cpp, std::nullopt
		);

		this->operator[](new_source_id).id = new_source_id;

		return GottenCFamilySourceID(new_source_id, true);
	}


}