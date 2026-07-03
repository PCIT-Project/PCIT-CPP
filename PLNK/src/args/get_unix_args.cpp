////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./get_unix_args.hpp"



namespace pcit::plnk{

	
	auto get_unix_args(evo::ArrayProxy<std::filesystem::path> link_file_paths, const Options& options) -> Args {
		auto args = Args();

		args.addArg("ld.lld");

		args.addArg("-o");
		args.addArg(evo::copy(options.outputFilePath));

		for(const std::filesystem::path& link_file_path : link_file_paths){
			args.addArg(link_file_path.string());
		}

		return args;
	}


}


