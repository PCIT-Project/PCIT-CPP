////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/platform.hpp"

#include <bit>

#if defined(EVO_PLATFORM_LINUX)
	#include <sys/mman.h>

#elif defined(EVO_PLATFORM_WINDOWS)

	using HANDLE = void*;
	using PVOID = void*;
	using ULONG = uint32_t;
	using NTSTATUS = uint32_t;
	using ULONG_PTR = size_t;
	using SIZE_T = size_t;
	using PSIZE_T = SIZE_T*;


	enum AllocationTypeFlags : ULONG {
		MEM_COMMIT     = 0x00001000,
		MEM_RESERVE    = 0x00002000,
		MEM_RESET      = 0x00080000,
		MEM_RESET_UNDO = 0x01000000,
	};

	
	enum AllocationProtectionFlags : ULONG {
		PAGE_NOACCESS     = 0x001,
		PAGE_READONLY     = 0x002,
		PAGE_READWRITE    = 0x004,
		PAGE_EXECUTE      = 0x008,
		PAGE_EXECUTE_READ = 0x010,
		PAGE_GUARD        = 0x100,
		PAGE_NOCACHE      = 0x200,
		PAGE_WRITECOMBINE = 0x400,
	};

	enum FreeTypeFlags : ULONG {
		MEM_DECOMMIT = 0x4000,
		MEM_RELEASE  = 0x8000,
	};



	#define DECLSPEC_IMPORT __declspec(dllimport)
	#define NTSYSCALLAPI DECLSPEC_IMPORT
	#define __kernel_entry


	inline auto NtCurrentProcess() -> HANDLE {
		return std::bit_cast<HANDLE>(intptr_t(-1));
	};

	extern "C" {

		__kernel_entry NTSYSCALLAPI NTSTATUS NtAllocateVirtualMemory(
			HANDLE ProcessHandle,
			PVOID* BaseAddress,
			ULONG_PTR ZeroBits,
			PSIZE_T RegionSize,
			ULONG AllocationType,
			ULONG Protect
		);

		__kernel_entry NTSYSCALLAPI NTSTATUS NtFreeVirtualMemory(
			HANDLE ProcessHandle,
			PVOID* BaseAddress,
			PSIZE_T RegionSize,
			ULONG FreeType
		);

	}


#endif

#include "../include/math.hpp"



static constexpr size_t PAGE_SIZE = 4096;


namespace pcit::core{


	auto memPageAlloc(size_t size, uint32_t alignment) -> void* {
		std::ignore = alignment; // TODO(FUTURE): handle alignment larger than page size

		#if defined(EVO_PLATFORM_LINUX)
			return mmap(
				nullptr,
				core::ceilToPowOf2Multiple(*size, PAGE_SIZE),
				PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS,
				-1,
				0
			);

		#elif defined(EVO_PLATFORM_WINDOWS)
			void* address = nullptr;

			const NTSTATUS ntstatus = NtAllocateVirtualMemory(
				NtCurrentProcess(), &address, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE
			);
			evo::debugAssert(ntstatus == 0, "NtAllocateVirtualMemory Failed");

			return address;
			
		#endif
	}


	auto memPageDealloc(void* buffer_data, size_t buffer_size) -> void {
		#if defined(EVO_PLATFORM_LINUX)
			return munmap(buffer_data, ceilToPowOf2Multiple(buffer_size, PAGE_SIZE));

		#elif defined(EVO_PLATFORM_WINDOWS)
			const NTSTATUS ntstatus = NtFreeVirtualMemory(NtCurrentProcess(), &buffer_data, &buffer_size, MEM_RELEASE);
			evo::debugAssert(ntstatus == 0, "NtFreeVirtualMemory Failed");
		#endif
	}

	
}
