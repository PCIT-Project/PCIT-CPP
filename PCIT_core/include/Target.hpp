////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>

namespace pcit::core{


	struct TargetArchitecture{
		enum class Value : uint32_t{
			X86_64,
			WASM32,
			WASM64_P32,
		};
		using enum class Value;

		constexpr TargetArchitecture(const Value& value) : _value(value) {}
		[[nodiscard]] constexpr operator Value() const { return this->_value; }
		[[nodiscard]] explicit constexpr operator uint32_t() const { return static_cast<uint32_t>(this->_value); }


		[[nodiscard]] constexpr static auto getNative() -> TargetArchitecture {
			#if defined(EVO_ARCH_X86_64)
				return TargetArchitecture::X86_64;
			#else
				#error "Compiling on an unsupported architecture"
			#endif
		}

		[[nodiscard]] constexpr auto isWasm() const -> bool {
			return this->_value == Value::WASM32 || this->_value == Value::WASM64_P32;
		}

		private:
			Value _value;
	};



	struct TargetPlatform{
		enum class Value : uint32_t{
			WINDOWS,
			LINUX,
			FREESTANDING,
		};
		
		using enum class Value;

		constexpr TargetPlatform(const Value& value) : _value(value) {}
		[[nodiscard]] constexpr operator Value() const { return this->_value; }
		[[nodiscard]] explicit constexpr operator uint32_t() const { return static_cast<uint32_t>(this->_value); }


		[[nodiscard]] constexpr static auto getNative() -> TargetPlatform {
			#if defined(EVO_PLATFORM_LINUX)
				return TargetPlatform::LINUX;

			#elif defined(EVO_PLATFORM_WINDOWS)
				return TargetPlatform::WINDOWS;

			#else
				return TargetPlatform::FREESTANDING;
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

		[[nodiscard]] constexpr auto operator==(const Target&) const -> bool = default;

		[[nodiscard]] constexpr static auto getNative() -> Target {
			return Target(Architecture::getNative(), Platform::getNative());
		}

		[[nodiscard]] constexpr auto isValid() const -> bool {
			switch(this->platform){
				case Platform::WINDOWS: {
					return this->architecture == Architecture::X86_64;
				} break;

				case Platform::LINUX: {
					return this->architecture == Architecture::X86_64;
				} break;

				case Platform::FREESTANDING: {
					return this->architecture != Architecture::X86_64;
				} break;
			}
			evo::unreachable();
		}

		[[nodiscard]] auto numBytesOfPtr() const -> size_t {
			switch(this->architecture){
				break; case core::Target::Architecture::X86_64:     return 8;
				break; case core::Target::Architecture::WASM32:     return 4;
				break; case core::Target::Architecture::WASM64_P32: return 4;
			}
			evo::unreachable();
		}

		[[nodiscard]] auto numBytesOfGeneralRegister() const -> size_t {
			switch(this->architecture){
				break; case core::Target::Architecture::X86_64:     return 8;
				break; case core::Target::Architecture::WASM32:     return 4;
				break; case core::Target::Architecture::WASM64_P32: return 8;
			}
			evo::unreachable();
		}

		[[nodiscard]] auto numBitsOfPtr() const -> size_t {
			switch(this->architecture){
				break; case core::Target::Architecture::X86_64:     return 64;
				break; case core::Target::Architecture::WASM32:     return 32;
				break; case core::Target::Architecture::WASM64_P32: return 32;
			}
			evo::unreachable();
		}

		[[nodiscard]] auto numBitsOfGeneralRegister() const -> size_t {
			switch(this->architecture){
				break; case core::Target::Architecture::X86_64:     return 64;
				break; case core::Target::Architecture::WASM32:     return 32;
				break; case core::Target::Architecture::WASM64_P32: return 64;
			}
			evo::unreachable();
		}

		[[nodiscard]] auto maxAlignmentOfPrimitive() const -> size_t {
			return this->numBytesOfPtr() * 2;
		}

		[[nodiscard]] auto maxAtomicNumBytes() const -> size_t {
			switch(this->architecture){
				break; case core::Target::Architecture::X86_64:     return 8;
				break; case core::Target::Architecture::WASM32:     return 4;
				break; case core::Target::Architecture::WASM64_P32: return 8;

				// aarch64    = 16
				// aarch64_be = 16
				// arm        = 4
				// armeb      = 4

				// riscv32    = 4
				// riscv64    = 8

				// spirv32    = 8
				// spirv64    = 8

				// wasm32     = 4
				// wasm64     = 8

				// x86        = 4
				// x86_64     = 8
			}
			evo::unreachable();
		}

		[[nodiscard]] auto maxAtomicNumBits() const -> size_t {
			switch(this->architecture){
				break; case core::Target::Architecture::X86_64:     return 64;
				break; case core::Target::Architecture::WASM32:     return 32;
				break; case core::Target::Architecture::WASM64_P32: return 64;

				// aarch64    = 128
				// aarch64_be = 128
				// arm        = 32
				// armeb      = 32

				// riscv32    = 32
				// riscv64    = 64

				// spirv32    = 64
				// spirv64    = 64

				// wasm32     = 32
				// wasm64     = 64

				// x86        = 32
				// x86_64     = 64
			}
			evo::unreachable();
		}
	};

}


template<>
struct std::formatter<pcit::core::Target::Platform> : std::formatter<std::string_view> {
    auto format(const pcit::core::Target::Platform& platform, std::format_context& ctx) const
    -> std::format_context::iterator {
        switch(platform){
        	case pcit::core::Target::Platform::LINUX:   return std::formatter<std::string_view>::format("Linux", ctx);
        	case pcit::core::Target::Platform::WINDOWS: return std::formatter<std::string_view>::format("Windows", ctx);
        	case pcit::core::Target::Platform::FREESTANDING:
        		return std::formatter<std::string_view>::format("FREESTANDING", ctx);
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

        	case pcit::core::Target::Architecture::WASM32:
        		return std::formatter<std::string_view>::format("WASM32", ctx);

        	case pcit::core::Target::Architecture::WASM64_P32:
        		return std::formatter<std::string_view>::format("WASM64_P32", ctx);

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