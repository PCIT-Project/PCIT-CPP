////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>

#include <PCIT_core.hpp>

#include "./forward_decl_ids.hpp"



namespace pcit::pir::meta{


	struct FileID : public core::UniqueID<uint32_t, struct FileID> {
		using core::UniqueID<uint32_t, FileID>::UniqueID;
	};

	struct SubscopeID : public core::UniqueID<uint32_t, struct SubscopeID> {
		using core::UniqueID<uint32_t, SubscopeID>::UniqueID;
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

	struct UnionTypeID : public core::UniqueID<uint32_t, struct UnionTypeID> {
		using core::UniqueID<uint32_t, UnionTypeID>::UniqueID;
	};

	struct ArrayTypeID : public core::UniqueID<uint32_t, struct ArrayTypeID> {
		using core::UniqueID<uint32_t, ArrayTypeID>::UniqueID;
	};

	struct EnumTypeID : public core::UniqueID<uint32_t, struct EnumTypeID> {
		using core::UniqueID<uint32_t, EnumTypeID>::UniqueID;
	};

	struct FunctionID : public core::UniqueID<uint32_t, struct FunctionID> {
		using core::UniqueID<uint32_t, FunctionID>::UniqueID;
	};



	using Type = evo::Variant<BasicTypeID, QualifiedTypeID, StructTypeID, UnionTypeID, ArrayTypeID, EnumTypeID>;


	using Scope = evo::Variant<FunctionID, FileID, SubscopeID>;
	using LocalScope = evo::Variant<FunctionID, SubscopeID>;


}


namespace std{
	
	template<>
	struct hash<pcit::pir::meta::FileID>{
		auto operator()(pcit::pir::meta::FileID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};

	template<>
	struct hash<pcit::pir::meta::SubscopeID>{
		auto operator()(pcit::pir::meta::SubscopeID id) const noexcept -> size_t {
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
	struct hash<pcit::pir::meta::UnionTypeID>{
		auto operator()(pcit::pir::meta::UnionTypeID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};

	template<>
	struct hash<pcit::pir::meta::ArrayTypeID>{
		auto operator()(pcit::pir::meta::ArrayTypeID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};

	template<>
	struct hash<pcit::pir::meta::EnumTypeID>{
		auto operator()(pcit::pir::meta::EnumTypeID id) const noexcept -> size_t {
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