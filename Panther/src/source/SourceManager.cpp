////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/source/SourceManager.h"



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
	auto SourceManager::lookupClangSourceID(std::string_view path) const -> std::optional<ClangSource::ID> {
		const auto file_path = std::filesystem::path(path);

		const auto clang_source_id_range = [&](){
			const auto lock = std::scoped_lock(this->priv.clang_sources_lock);
			return core::IterRange<ClangSource::ID::Iterator>(
				ClangSource::ID::Iterator(ClangSource::ID(0)),
				ClangSource::ID::Iterator(ClangSource::ID(uint32_t(this->priv.clang_sources.size())))
			);
		}();

		// look for path
		for(const ClangSource::ID clang_source_id : clang_source_id_range){
			const ClangSource& clang_source = this->operator[](clang_source_id);
			if(clang_source.getPath() == file_path){ return clang_source_id; }
		}

		return std::nullopt;
	}



	// TODO(PERF): improve lookup times with a map maybe?
	auto SourceManager::getOrCreateClangSourceID(std::filesystem::path&& path, bool is_cpp) -> GottenClangSourceID {
		const auto lock = std::scoped_lock(this->priv.clang_sources_lock);

		const auto clang_source_id_range = core::IterRange<ClangSource::ID::Iterator>(
			ClangSource::ID::Iterator(ClangSource::ID(0)),
			ClangSource::ID::Iterator(ClangSource::ID(uint32_t(this->priv.clang_sources.size())))
		);

		for(const ClangSource::ID clang_source_id : clang_source_id_range){
			const ClangSource& clang_source = this->operator[](clang_source_id);
			if(clang_source.getPath() == path){ return GottenClangSourceID(clang_source_id, false); }
		}

		
		evo::Result<std::string> file_data = evo::fs::readFile(path.string());
		evo::debugAssert(file_data.isSuccess(), "File doesn't exist");


		const ClangSource::ID new_source_id = this->priv.clang_sources.emplace_back(
			std::move(path), std::move(file_data.value()), is_cpp
		);

		this->operator[](new_source_id).id = new_source_id;

		return GottenClangSourceID(new_source_id, true);
	}


}