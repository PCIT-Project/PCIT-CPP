////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/source/SourceManager.h"

#include <filesystem>
namespace fs = std::filesystem;


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{
	

	auto SourceManager::lookupSpecialNameSourceID(std::string_view path) const -> std::optional<Source::ID> {
		const auto special_name_path_find = this->priv.special_name_paths.find(path);

		if(special_name_path_find == this->priv.special_name_paths.end()){ return std::nullopt; }

		// TODO(PERF): optimize this
		// look for path
		for(const Source::ID source_id : *this){
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
		const auto file_path = fs::path(path);

		// look for path
		for(const Source::ID source_id : *this){
			const Source& source = this->operator[](source_id);
			if(source.getPath() == file_path){ return source_id; }
		}

		return std::nullopt;
	}


}