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





namespace pcit::panther{

	class TypeManager;


	//////////////////////////////////////////////////////////////////////
	// forward decls

	// is aliased as TypeInfo::ID
	struct TypeInfoID : public core::UniqueID<uint32_t, struct TypeInfoID> {
		using core::UniqueID<uint32_t, TypeInfoID>::UniqueID;
	};

	struct TypeInfoIDOptInterface{
		static constexpr auto init(TypeInfoID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const TypeInfoID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

}



namespace std{

	template<>
	class optional<pcit::panther::TypeInfoID> 
		: public pcit::core::Optional<pcit::panther::TypeInfoID, pcit::panther::TypeInfoIDOptInterface>{

		public:
			using pcit::core::Optional<pcit::panther::TypeInfoID, pcit::panther::TypeInfoIDOptInterface>::Optional;
			using pcit::core::Optional<pcit::panther::TypeInfoID, pcit::panther::TypeInfoIDOptInterface>::operator=;
	};

}


namespace pcit::panther{

	// is aliased as TypeInfo::VoidableID
	class TypeInfoVoidableID{
		public:
			TypeInfoVoidableID(TypeInfoID type_id) : id(type_id) {};
			~TypeInfoVoidableID() = default;

			EVO_NODISCARD static auto Void() -> TypeInfoVoidableID { return TypeInfoVoidableID(); };

			EVO_NODISCARD auto operator==(const TypeInfoVoidableID& rhs) const -> bool {
				return this->id == rhs.id;
			};

			EVO_NODISCARD auto asTypeID() const -> const TypeInfoID& {
				evo::debugAssert(this->isVoid() == false, "type is void");
				return this->id;
			};

			EVO_NODISCARD auto asTypeID() -> TypeInfoID& {
				evo::debugAssert(this->isVoid() == false, "type is void");
				return this->id;
			};


			EVO_NODISCARD auto isVoid() const -> bool {
				return this->id.get() == std::numeric_limits<TypeInfoID::Base>::max();
			};


			EVO_NODISCARD auto hash() const -> size_t { return std::hash<uint32_t>{}(this->id.get()); }

		private:
			TypeInfoVoidableID() : id(std::numeric_limits<TypeInfoID::Base>::max()) {};
	
		private:
			TypeInfoID id;
	};


}




namespace pcit::panther::BaseType{


	struct PrimitiveID : public core::UniqueID<uint32_t, struct PrimitiveID> {
		using core::UniqueID<uint32_t, PrimitiveID>::UniqueID; 
	};

	struct FunctionID : public core::UniqueID<uint32_t, struct FunctionID> {
		using core::UniqueID<uint32_t, FunctionID>::UniqueID; 
	};

	struct ArrayID : public core::UniqueID<uint32_t, struct ArrayID> {
		using core::UniqueID<uint32_t, ArrayID>::UniqueID; 
	};

	struct AliasID : public core::UniqueID<uint32_t, struct AliasID> {
		using core::UniqueID<uint32_t, AliasID>::UniqueID; 
	};

	struct TypedefID : public core::UniqueID<uint32_t, struct TypedefID> {
		using core::UniqueID<uint32_t, TypedefID>::UniqueID; 
	};

	struct StructID : public core::UniqueID<uint32_t, struct StructID> {
		using core::UniqueID<uint32_t, StructID>::UniqueID; 
	};


}