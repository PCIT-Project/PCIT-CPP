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
			WINDOWS,
			UNIX,
			DARWIN,
			WEB_ASSEMBLY,
		};
		using enum class Value;

		constexpr Target(const Value& value) : _value(value) {}
		EVO_NODISCARD constexpr operator Value() const { return this->_value; }


		EVO_NODISCARD static constexpr auto getDefault() -> Target {
			#if defined(EVO_PLATFORM_WINDOWS)
				return Target::WINDOWS;
			#elif defined(EVO_PLATFORM_LINUX) || defined(EVO_PLATFORM_UNIX)
				return Target::UNIX;
			#elif defined(EVO_PLATFROM_APPLE)
				return Target::DARWIN;
			#else
				return Target::UNIX;
			#endif
		}

		private:
			Value _value;
	};


	
	struct Options{
		Options() : target(Target::getDefault()) { this->init(); }
		Options(Target _target) : target(_target) { this->init(); }

		std::string outputFilePath{}; // if empty, default


		EVO_NODISCARD auto getTarget() const -> Target { return this->target; }

		struct WindowsSpecific{
			// https://learn.microsoft.com/en-us/cpp/build/reference/subsystem-specify-subsystem?view=msvc-170
			enum class Subsystem{ 
				CONSOLE, // console application
				WINDOWS, // application doesn't require a console

				// less used
				BOOT_APPLICATION,        // application that runs in the Windows boot environment
				EFI_APPLICATION,         // Extensible Firmare Interface subsystem
				EFI_BOOT_SERVICE_DRIVER, // Extensible Firmare Interface subsystem
				EFI_ROM,                 // Extensible Firmare Interface subsystem
				EFI_RUNTIME_DRIVER,      // Extensible Firmare Interface subsystem
				NATIVE,                  // kernel mode drivers for Windows NT
				POSIX,                   // application that runs with the POSIX subsystem in Windows NT
			};

			Subsystem subsystem = Subsystem::CONSOLE;
		};

		struct UnixSpecific{};
		struct DarwinSpecific{};
		struct WebAssemblySpecific{};

		EVO_NODISCARD auto getWindowsSpecific() -> WindowsSpecific& {
			evo::debugAssert(this->target == Target::WINDOWS, "Not Windows target");
			return this->specific.as<WindowsSpecific>();
		}
		EVO_NODISCARD auto getWindowsSpecific() const -> const WindowsSpecific& {
			evo::debugAssert(this->target == Target::WINDOWS, "Not Windows target");
			return this->specific.as<WindowsSpecific>();
		}


		EVO_NODISCARD auto getUnixSpecific() -> UnixSpecific& {
			evo::debugAssert(this->target == Target::UNIX, "Not Unix target");
			return this->specific.as<UnixSpecific>();
		}
		EVO_NODISCARD auto getUnixSpecific() const -> const UnixSpecific& {
			evo::debugAssert(this->target == Target::UNIX, "Not Unix target");
			return this->specific.as<UnixSpecific>();
		}


		EVO_NODISCARD auto getDarwinSpecific() -> DarwinSpecific& {
			evo::debugAssert(this->target == Target::DARWIN, "Not Darwin target");
			return this->specific.as<DarwinSpecific>();
		}
		EVO_NODISCARD auto getDarwinSpecific() const -> const DarwinSpecific& {
			evo::debugAssert(this->target == Target::DARWIN, "Not Darwin target");
			return this->specific.as<DarwinSpecific>();
		}


		EVO_NODISCARD auto getWebAssemblySpecific() -> WebAssemblySpecific& {
			evo::debugAssert(this->target == Target::WEB_ASSEMBLY, "Not WebAssembly target");
			return this->specific.as<WebAssemblySpecific>();
		}
		EVO_NODISCARD auto getWebAssemblySpecific() const -> const WebAssemblySpecific& {
			evo::debugAssert(this->target == Target::WEB_ASSEMBLY, "Not WebAssembly target");
			return this->specific.as<WebAssemblySpecific>();
		}


		private:
			auto init() -> void {
				switch(this->target){
					break; case Target::WINDOWS:      this->specific.emplace<WindowsSpecific>();
					break; case Target::UNIX:         this->specific.emplace<UnixSpecific>();
					break; case Target::DARWIN:       this->specific.emplace<DarwinSpecific>();
					break; case Target::WEB_ASSEMBLY: this->specific.emplace<WebAssemblySpecific>();
				}
			}

		private:
			Target target;
			evo::Variant<WindowsSpecific, UnixSpecific, DarwinSpecific, WebAssemblySpecific> specific{};
	};


}


