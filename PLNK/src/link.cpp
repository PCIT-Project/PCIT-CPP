////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/link.h"

#include "./args/get_windows_args.h"
#include "./args/get_unix_args.h"
#include "./args/get_darwin_args.h"
#include "./args/get_wasm_args.h"

#include <LLD.h>


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


LLD_HAS_DRIVER(coff)
LLD_HAS_DRIVER(elf)
// LLD_HAS_DRIVER(mingw)
LLD_HAS_DRIVER(macho)
LLD_HAS_DRIVER(wasm)


namespace pcit::plnk{
	

	class MessageOStream : public llvm::raw_ostream {
	    public:
	        MessageOStream(){
	            this->SetUnbuffered();
	        };
	        ~MessageOStream() = default;

	
	    public:
	        std::vector<std::string> messages{};

	    private:
	        void write_impl(const char* ptr, size_t size) noexcept override {
	            this->messages.emplace_back(ptr, size);
	            this->current_size += size;
	        };

	        uint64_t current_pos() const override { return this->current_size; }

	    private:
	        size_t current_size = 0;
	};


	static auto run_driver(llvm::ArrayRef<const char*> args, lld::Driver driver) -> LinkResult {
		auto std_os = MessageOStream();
		auto err_os = MessageOStream();

		int return_code;
		{
			auto crc = llvm::CrashRecoveryContext();
			if(!crc.RunSafely([&]() -> void { return_code = driver(args, std_os, err_os, false, false); })){
				return LinkResult(crc.RetCode, false, std::move(std_os.messages), std::move(err_os.messages));
			}
		}

		auto crc = llvm::CrashRecoveryContext();
		if(crc.RunSafely([&]() -> void { lld::CommonLinkerContext::destroy(); })){
			return LinkResult(return_code, false, std::move(std_os.messages), std::move(err_os.messages));
		}else{
			return LinkResult(return_code, true, std::move(std_os.messages), std::move(err_os.messages));
		}
	}



	auto link(evo::ArrayProxy<std::filesystem::path> object_file_paths, const Options& options) -> LinkResult {
		struct LinkData{
			lld::Driver driver;
			Args args;
		};

		LinkData link_data = [&](){
			switch(options.getTarget()){
				case Target::WINDOWS:
					return LinkData(&lld::coff::link, get_windows_args(object_file_paths, options));

				case Target::UNIX:
					return LinkData(&lld::elf::link, get_unix_args(object_file_paths, options));

				case Target::DARWIN:
					return LinkData(&lld::macho::link, get_darwin_args(object_file_paths, options));

				case Target::WEB_ASSEMBLY:
					return LinkData(&lld::wasm::link, get_wasm_args(object_file_paths, options));
			}

			evo::debugFatalBreak("Unknown Target");
		}();

		return run_driver({link_data.args.getArgs().data(), link_data.args.getArgs().size()}, link_data.driver);
	}

}