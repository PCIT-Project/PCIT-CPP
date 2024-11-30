////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>


namespace pcit::plnk{


	struct Target{
		enum class Value{
			Windows,
			Unix,
			Darwin,
			WebAssembly,
		};
		using enum class Value;

		constexpr Target(const Value& value) : _value(value) {}
		EVO_NODISCARD constexpr operator Value() const { return this->_value; }


		EVO_NODISCARD static constexpr auto getDefault() -> Target {
			#if defined(EVO_PLATFORM_WINDOWS)
				return Target::Windows;
			#elif defined(EVO_PLATFORM_LINUX) || defined(EVO_PLATFORM_UNIX)
				return Target::Unix;
			#elif defined(EVO_PLATFROM_APPLE)
				return Target::Darwin;
			#else
				return Target::Unix;
			#endif
		}

		private:
			Value _value;
	};


	
	struct Options{
		Options() : target(Target::getDefault()) {}
		Options(Target _target) : target(_target) {}

		std::string outputFilePath{};


		EVO_NODISCARD auto getTarget() const -> Target { return this->target; }

		struct WindowsSpecific{};
		struct UnixSpecific{};
		struct DarwinSpecific{};
		struct WebAssemblySpecific{};

		EVO_NODISCARD auto getWindowsSpecific() -> WindowsSpecific& {
			evo::debugAssert(this->target == Target::Windows, "Not Windows target");
			return this->specific.as<WindowsSpecific>();
		}

		EVO_NODISCARD auto getUnixSpecific() -> UnixSpecific& {
			evo::debugAssert(this->target == Target::Unix, "Not Unix target");
			return this->specific.as<UnixSpecific>();
		}

		EVO_NODISCARD auto getDarwinSpecific() -> DarwinSpecific& {
			evo::debugAssert(this->target == Target::Darwin, "Not Darwin target");
			return this->specific.as<DarwinSpecific>();
		}

		EVO_NODISCARD auto getWebAssemblySpecific() -> WebAssemblySpecific& {
			evo::debugAssert(this->target == Target::WebAssembly, "Not WebAssembly target");
			return this->specific.as<WebAssemblySpecific>();
		}

		private:
			Target target;
			evo::Variant<WindowsSpecific, UnixSpecific, DarwinSpecific, WebAssemblySpecificN> specific{};
	};


}


