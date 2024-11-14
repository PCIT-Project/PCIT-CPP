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


namespace pcit::pir{


	struct BasicBlockID : public core::UniqueID<uint32_t, struct BasicBlockID> {
		using core::UniqueID<uint32_t, BasicBlockID>::UniqueID;
	};


	struct FunctionID : public core::UniqueID<uint32_t, struct FunctionID> {
		using core::UniqueID<uint32_t, FunctionID>::UniqueID;
	};

	struct FunctionDeclID : public core::UniqueID<uint32_t, struct FunctionDeclID> {
		using core::UniqueID<uint32_t, FunctionDeclID>::UniqueID;
	};


}


namespace std{

	template<>
	struct hash<pcit::pir::BasicBlockID>{
		auto operator()(const pcit::pir::BasicBlockID& expr) const noexcept -> size_t {
			return hash<uint32_t>{}(expr.get());
		}
	};


	template<>
	struct hash<pcit::pir::FunctionID>{
		auto operator()(const pcit::pir::FunctionID& expr) const noexcept -> size_t {
			return hash<uint32_t>{}(expr.get());
		}
	};


	template<>
	struct hash<pcit::pir::FunctionDeclID>{
		auto operator()(const pcit::pir::FunctionDeclID& expr) const noexcept -> size_t {
			return hash<uint32_t>{}(expr.get());
		}
	};
	
}


