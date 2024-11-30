////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./get_windows_args.h"



namespace pcit::plnk{

	
	auto get_windows_args(evo::ArrayProxy<std::filesystem::path> object_file_paths, const Options& options) -> Args {
		auto args = Args();

		{
			auto output_arg = std::string("-out:");
			if(options.outputFilePath.empty()){
				output_arg += "output.exe";
			}else{
				output_arg += options.outputFilePath;
			}
			args.addArg(std::move(output_arg));
		}

		args.addArg("-WX"); // treat warnings as errors
		args.addArg("-nologo"); // suppress copyright banner (doesn't seem to do anything, but just in case)

		args.addArg("-defaultlib:libcmt");
		args.addArg("-defaultlib:oldnames");

		// TODO: how to add more than one
		evo::debugAssert(object_file_paths.size() == 1, "only 1 object file is supported at this time");
		args.addArg(object_file_paths.front().string());

		return args;
	}


}


