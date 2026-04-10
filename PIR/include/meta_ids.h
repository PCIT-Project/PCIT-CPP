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

#include "./forward_decl_ids.h"


namespace pcit::pir{
	struct StructType;
}

namespace pcit::pir::meta{

	struct ItemID : public core::UniqueID<uint32_t, struct ItemID> {
		using core::UniqueID<uint32_t, ItemID>::UniqueID;
	};

	struct FileID : public core::UniqueID<uint32_t, struct FileID> {
		using core::UniqueID<uint32_t, FileID>::UniqueID;
	};

	struct BasicTypeID : public core::UniqueID<uint32_t, struct BasicTypeID> {
		using core::UniqueID<uint32_t, BasicTypeID>::UniqueID;
	};

	struct QualifiedTypeID : public core::UniqueID<uint32_t, struct QualifiedTypeID> {
		using core::UniqueID<uint32_t, QualifiedTypeID>::UniqueID;
	};

	struct StructTypeID : public core::UniqueID<uint32_t, struct StructTypeID> {
		using core::UniqueID<uint32_t, StructTypeID>::UniqueID;
	};

	struct FunctionID : public core::UniqueID<uint32_t, struct FunctionID> {
		using core::UniqueID<uint32_t, FunctionID>::UniqueID;
	};



	using Type = evo::Variant<BasicTypeID, QualifiedTypeID, StructTypeID>;

	using Item = evo::Variant<FileID, BasicTypeID, QualifiedTypeID, StructTypeID, FunctionID>;

	using Scope = evo::Variant<FunctionID, FileID>;
	using LocalScope = evo::Variant<FunctionID>;


}


namespace std{
	
	template<>
	struct hash<pcit::pir::meta::FileID>{
		auto operator()(pcit::pir::meta::FileID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};

	template<>
	struct hash<pcit::pir::meta::BasicTypeID>{
		auto operator()(pcit::pir::meta::BasicTypeID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};

	template<>
	struct hash<pcit::pir::meta::QualifiedTypeID>{
		auto operator()(pcit::pir::meta::QualifiedTypeID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};

	template<>
	struct hash<pcit::pir::meta::StructTypeID>{
		auto operator()(pcit::pir::meta::StructTypeID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};

	template<>
	struct hash<pcit::pir::meta::FunctionID>{
		auto operator()(pcit::pir::meta::FunctionID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};

}