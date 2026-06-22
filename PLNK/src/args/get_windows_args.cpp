////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./get_windows_args.hpp"



namespace pcit::plnk{

	
	auto get_windows_args(evo::ArrayProxy<std::filesystem::path> link_file_paths, const Options& options) -> Args {
		auto args = Args();


		// args.addArg("-wx"); // treat warnings as errors

		// args.addArg("-defaultlib:libcmt"); // libc for multithreaded programms
		// args.addArg("-defaultlib:oldnames");

		args.addArg("/nodefaultlib");


		switch(options.getWindowsSpecific().subsystem){
			break; case Options::WindowsSpecific::Subsystem::CONSOLE:
				args.addArg("/subsystem:console");

			break; case Options::WindowsSpecific::Subsystem::WINDOWS:
				args.addArg("/subsystem:windows");

			break; case Options::WindowsSpecific::Subsystem::BOOT_APPLICATION:
				args.addArg("/subsystem:boot_application");

			break; case Options::WindowsSpecific::Subsystem::EFI_APPLICATION:
				args.addArg("/subsystem:efi_application");

			break; case Options::WindowsSpecific::Subsystem::EFI_BOOT_SERVICE_DRIVER:
				args.addArg("/subsystem:efi_boot_service_driver");

			break; case Options::WindowsSpecific::Subsystem::EFI_ROM:
				args.addArg("/subsystem:efi_rom");

			break; case Options::WindowsSpecific::Subsystem::EFI_RUNTIME_DRIVER:
				args.addArg("/subsystem:efi_runtime_driver");

			break; case Options::WindowsSpecific::Subsystem::NATIVE:
				args.addArg("/subsystem:native");

			break; case Options::WindowsSpecific::Subsystem::POSIX:
				args.addArg("/subsystem:posix");
		}

		args.addArg("/nologo"); // suppress copyright banner (doesn't seem to do anything, but just in case)

		if(options.includeDebugInfo){
			args.addArg("/debug");
		}


		for(const std::filesystem::path& link_file_path : link_file_paths){
			args.addArg(link_file_path.string());
		}

		// args.addArg("libcmt.lib");

		if(options.outputFilePath.empty()){
			args.addArg("/out:output.exe");
		}else{
			args.addArg(std::format("/out:{}", options.outputFilePath));
		}


		return args;
	}


}


