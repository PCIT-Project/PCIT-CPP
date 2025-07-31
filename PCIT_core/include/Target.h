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


	struct TargetArchitecture{
		enum class Value{
			X86_64,

			UNKNOWN,
		};
		using enum class Value;

		constexpr TargetArchitecture(const Value& value) : _value(value) {}
		EVO_NODISCARD constexpr operator Value() const { return this->_value; }


		EVO_NODISCARD constexpr static auto getCurrent() -> TargetArchitecture {
			#if defined(EVO_ARCH_X86_64)
				return TargetArchitecture::X86_64;
			#else
				return TargetArchitecture::UNKNOWN;
			#endif
		}

		private:
			Value _value;
	};



	struct TargetPlatform{
		enum class Value{
			WINDOWS,
			LINUX,

			UNKNOWN,
		};
		
		using enum class Value;

		constexpr TargetPlatform(const Value& value) : _value(value) {}
		EVO_NODISCARD constexpr operator Value() const { return this->_value; }


		EVO_NODISCARD constexpr static auto getCurrent() -> TargetPlatform {
			#if defined(EVO_PLATFORM_WINDOWS)
				return TargetPlatform::WINDOWS;
			#elif defined(EVO_PLATFORM_LINUX)
				return TargetPlatform::LINUX;
			#else
				return TargetPlatform::UNKNOWN;
			#endif
		}

		private:
			Value _value;
	};



	struct Target{
		using Architecture = TargetArchitecture;
		using Platform = TargetPlatform;

		Architecture architecture;
		Platform platform;

		constexpr Target(Architecture arch, Platform _platform) : architecture(arch), platform(_platform) {}

		EVO_NODISCARD constexpr static auto getCurrent() -> Target {
			return Target(Architecture::getCurrent(), Platform::getCurrent());
		}
	};

}


template<>
struct std::formatter<pcit::core::Target::Platform> : std::formatter<std::string_view> {
    auto format(const pcit::core::Target::Platform& platform, std::format_context& ctx) const
    -> std::format_context::iterator {
        switch(platform){
        	case pcit::core::Target::Platform::WINDOWS: return std::formatter<std::string_view>::format("Windows", ctx);
        	case pcit::core::Target::Platform::LINUX:   return std::formatter<std::string_view>::format("Linux", ctx);
        	case pcit::core::Target::Platform::UNKNOWN: return std::formatter<std::string_view>::format("UNKNOWN", ctx);
        	default: evo::debugFatalBreak("Unknown or unsupported Platform");
        }
    }
};


template<>
struct std::formatter<pcit::core::Target::Architecture> : std::formatter<std::string_view> {
    auto format(const pcit::core::Target::Architecture& arch, std::format_context& ctx) const
    -> std::format_context::iterator {
        switch(arch){
        	case pcit::core::Target::Architecture::X86_64:
        		return std::formatter<std::string_view>::format("x86_64", ctx);

        	case pcit::core::Target::Architecture::UNKNOWN:
        		return std::formatter<std::string_view>::format("UNKNOWN", ctx);

        	default: evo::debugFatalBreak("Unknown or unsupported architecture");
        }
    }
};


template<>
struct std::formatter<pcit::core::Target> : std::formatter<std::string_view> {
    auto format(const pcit::core::Target& platform, std::format_context& ctx) const -> std::format_context::iterator {
        return std::format_to(ctx.out(), "{}-{}", platform.architecture, platform.platform);
    }
};