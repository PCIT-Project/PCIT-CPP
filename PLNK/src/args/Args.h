////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>


namespace pcit::plnk{

	
	class Args{
		public:
			Args() = default;
			~Args() = default;

			auto addArg(std::string&& string) -> void {
				const std::string& added_string = this->string_holder.emplace_back(std::move(string));
				this->args.emplace_back(added_string.c_str());
			}

			auto addArg(const std::string& string) -> void = delete;

			auto addArg(const char* string) -> void {
				this->args.emplace_back(string);
			}

			EVO_NODISCARD auto getArgs() const -> evo::ArrayProxy<const char*> {
				return this->args;
			}
	
		private:
			evo::StepVector<std::string> string_holder{};
			evo::SmallVector<const char*> args{};
	};

}


