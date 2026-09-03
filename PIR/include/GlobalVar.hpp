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


#include "./enums.hpp"
#include "./Type.hpp"
#include "./Expr.hpp"
#include "./meta.hpp"


namespace pcit::pir{


	struct GlobalVar{
		struct NoValue{};
		struct Zeroinit{};
		struct Uninit{};

		struct String{
			// For lookup in Module
			struct ID : public core::UniqueID<uint32_t, struct ID> {
				using core::UniqueID<uint32_t, ID>::UniqueID;
			};

			std::string value;
			Type type;
		};

		struct ByteArray{
			// For lookup in Module
			struct ID : public core::UniqueID<uint32_t, struct ID> {
				using core::UniqueID<uint32_t, ID>::UniqueID;
			};

			Type type;
			evo::SmallVector<std::byte> bytes;
		};
		
		// Forward declaration
		struct ArrayID : public core::UniqueID<uint32_t, struct ArrayID> {
			using core::UniqueID<uint32_t, ArrayID>::UniqueID;
		};

		// Forward declaration
		struct StructID : public core::UniqueID<uint32_t, struct StructID> {
			using core::UniqueID<uint32_t, StructID>::UniqueID;
		};

		struct UnionID : public core::UniqueID<uint32_t, struct UnionID> {
			using core::UniqueID<uint32_t, UnionID>::UniqueID;
		};

		struct CalcPtrID : public core::UniqueID<uint32_t, struct CalcPtrID> {
			using core::UniqueID<uint32_t, CalcPtrID>::UniqueID;
		};

		using Value = evo::Variant<
			NoValue, Expr, Zeroinit, Uninit, String::ID, ByteArray::ID, ArrayID, StructID, UnionID, CalcPtrID
		>;

		struct Array{
			// For lookup in Module
			using ID = ArrayID;

			Type type;
			evo::SmallVector<Value> values;
		};

		struct Struct{
			// For lookup in Module
			using ID = StructID;

			Type type;
			evo::SmallVector<Value> values;
		};

		struct Union{
			// For lookup in Module
			using ID = UnionID;

			Type type;
			Value value;
		};

		struct CalcPtr{ // TODO(FUTURE): make type an offsets
			// For lookup in Module
			using ID = CalcPtrID;

			Value value;
			uint32_t byteOffset;
			// Type type;
			// evo::SmallVector<uint32_t> offsets;
		};





		const std::string name;
		const Type type;
		const Linkage linkage;
		Value value;
		const bool isConstant;
		std::optional<meta::GlobalVariable::ID> metaID;
		

		// For lookup in Module
		struct ID : public core::UniqueID<uint32_t, struct ID> {
			using core::UniqueID<uint32_t, ID>::UniqueID;
		};
	};


}



namespace std{

	template<>
	struct hash<pcit::pir::GlobalVar::ID>{
		auto operator()(pcit::pir::GlobalVar::ID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};

	template<>
	struct hash<pcit::pir::GlobalVar::ByteArray::ID>{
		auto operator()(pcit::pir::GlobalVar::ByteArray::ID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};

	template<>
	struct hash<pcit::pir::GlobalVar::Array::ID>{
		auto operator()(pcit::pir::GlobalVar::Array::ID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};
	
	template<>
	struct hash<pcit::pir::GlobalVar::Struct::ID>{
		auto operator()(pcit::pir::GlobalVar::Struct::ID id) const noexcept -> size_t {
			return std::hash<uint32_t>{}(id.get());
		};
	};

}
