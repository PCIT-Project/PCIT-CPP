////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

namespace pcit::core{


	struct PlatformArchitecture{
		enum class Value{
			X86_64,

			UNKNOWN,
		};
		using enum class Value;

		constexpr PlatformArchitecture(const Value& value) : _value(value) {}
		EVO_NODISCARD constexpr operator Value() const { return this->_value; }


		EVO_NODISCARD constexpr static auto getCurrent() -> PlatformArchitecture {
			#if defined(EVO_ARCH_X86_64)
				return PlatformArchitecture::X86_64;
			#else
				return PlatformArchitecture::UNKNOWN;
			#endif
		}

		private:
			Value _value;
	};



	struct PlatformOS{
		enum class Value{
			WINDOWS,
			LINUX,

			UNKNOWN,
		};
		
		using enum class Value;

		constexpr PlatformOS(const Value& value) : _value(value) {}
		EVO_NODISCARD constexpr operator Value() const { return this->_value; }


		EVO_NODISCARD constexpr static auto getCurrent() -> PlatformOS {
			#if defined(EVO_PLATFORM_WINDOWS)
				return PlatformOS::WINDOWS;
			#elif defined(EVO_PLATFORM_LINUX)
				return PlatformOS::LINUX;
			#else
				return PlatformOS::UNKNOWN;
			#endif
		}

		private:
			Value _value;
	};



	struct Platform{
		using Architecture = PlatformArchitecture;
		using OS = PlatformOS;

		Architecture arch;
		OS os;

		constexpr Platform(Architecture _arch, OS _os) : arch(_arch), os(_os) {}

		EVO_NODISCARD constexpr static auto getCurrent() -> Platform {
			return Platform(Architecture::getCurrent(), OS::getCurrent());
		}
	};

}


template<>
struct std::formatter<pcit::core::Platform::OS> : std::formatter<std::string_view> {
    auto format(const pcit::core::Platform::OS& os, std::format_context& ctx) const -> std::format_context::iterator {
        switch(os){
        	case pcit::core::Platform::OS::WINDOWS: return std::formatter<std::string_view>::format("Windows", ctx);
        	case pcit::core::Platform::OS::LINUX:   return std::formatter<std::string_view>::format("Linux", ctx);
        	case pcit::core::Platform::OS::UNKNOWN: return std::formatter<std::string_view>::format("UNKNOWN", ctx);
        	default: evo::debugFatalBreak("Unknown or unsupported OS");
        }
    }
};


template<>
struct std::formatter<pcit::core::Platform::Architecture> : std::formatter<std::string_view> {
    auto format(const pcit::core::Platform::Architecture& arch, std::format_context& ctx) const
    -> std::format_context::iterator {
        switch(arch){
        	case pcit::core::Platform::Architecture::X86_64:
        		return std::formatter<std::string_view>::format("x86_64", ctx);

        	case pcit::core::Platform::Architecture::UNKNOWN:
        		return std::formatter<std::string_view>::format("UNKNOWN", ctx);

        	default: evo::debugFatalBreak("Unknown or unsupported architecture");
        }
    }
};


template<>
struct std::formatter<pcit::core::Platform> : std::formatter<std::string_view> {
    auto format(const pcit::core::Platform& platform, std::format_context& ctx) const -> std::format_context::iterator {
        return std::format_to(ctx.out(), "{}-{}", platform.arch, platform.os);
    }
};