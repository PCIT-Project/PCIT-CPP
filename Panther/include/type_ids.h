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


#include "./sema/sema_ids.h"


namespace pcit::panther{

	class TypeManager;



	//////////////////////////////////////////////////////////////////////
	// forward decls

	// is aliased as TypeInfo::ID
	struct TypeInfoID : public core::UniqueID<uint32_t, struct TypeInfoID> {
		using core::UniqueID<uint32_t, TypeInfoID>::UniqueID;


		EVO_NODISCARD auto hash() const -> size_t { return std::hash<uint32_t>{}(this->get()); }


		// This is not the most elegant way of doing this, but I don't want to overcomplicate things for one place
		EVO_NODISCARD static auto createTemplateDeclInstantiation() -> TypeInfoID {
			return TypeInfoID(std::numeric_limits<uint32_t>::max() - 1);
		}
		EVO_NODISCARD auto isTemplateDeclInstantiation() const -> bool {
			return this->get() == (std::numeric_limits<uint32_t>::max() - 1);
		}
	};

}



namespace pcit::core{

	template<>
	struct OptionalInterface<panther::TypeInfoID>{
		static constexpr auto init(panther::TypeInfoID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::TypeInfoID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};

}



namespace std{

	template<>
	class optional<pcit::panther::TypeInfoID> : public pcit::core::Optional<pcit::panther::TypeInfoID>{

		public:
			using pcit::core::Optional<pcit::panther::TypeInfoID>::Optional;
			using pcit::core::Optional<pcit::panther::TypeInfoID>::operator=;
	};


	template<>
	struct hash<pcit::panther::TypeInfoID>{
		auto operator()(const pcit::panther::TypeInfoID& voidable_id) const noexcept -> size_t {
			return voidable_id.hash();
		};
	};

}


namespace pcit::panther{

	// is aliased as TypeInfo::VoidableID
	class TypeInfoVoidableID{
		public:
			TypeInfoVoidableID(TypeInfoID type_id) : id(type_id) {}; // purposely not explicit
			~TypeInfoVoidableID() = default;

			EVO_NODISCARD static auto Void() -> TypeInfoVoidableID { return TypeInfoVoidableID(); };

			EVO_NODISCARD auto operator==(TypeInfoVoidableID rhs) const -> bool {
				return this->id == rhs.id;
			};

			EVO_NODISCARD auto operator==(TypeInfoID rhs) const -> bool {
				return this->id == rhs;
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


namespace std{
	
	template<>
	struct hash<pcit::panther::TypeInfoVoidableID>{
		auto operator()(const pcit::panther::TypeInfoVoidableID& voidable_id) const noexcept -> size_t {
			return voidable_id.hash();
		};
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

	struct ArrayDeducerID : public core::UniqueID<uint32_t, struct ArrayDeducerID> {
		using core::UniqueID<uint32_t, ArrayDeducerID>::UniqueID; 
	};

	struct ArrayRefID : public core::UniqueID<uint32_t, struct ArrayRefID> {
		using core::UniqueID<uint32_t, ArrayRefID>::UniqueID; 
	};

	struct ArrayRefDeducerID : public core::UniqueID<uint32_t, struct ArrayRefDeducerID> {
		using core::UniqueID<uint32_t, ArrayRefDeducerID>::UniqueID; 
	};

	struct AliasID : public core::UniqueID<uint32_t, struct AliasID> {
		using core::UniqueID<uint32_t, AliasID>::UniqueID; 
	};

	struct DistinctAliasID : public core::UniqueID<uint32_t, struct DistinctAliasID> {
		using core::UniqueID<uint32_t, DistinctAliasID>::UniqueID; 
	};

	struct StructID : public core::UniqueID<uint32_t, struct StructID> {
		using core::UniqueID<uint32_t, StructID>::UniqueID; 
	};

	struct StructTemplateID : public core::UniqueID<uint32_t, struct StructTemplateID> {
		using core::UniqueID<uint32_t, StructTemplateID>::UniqueID; 
	};

	struct StructTemplateDeducerID : public core::UniqueID<uint32_t, struct StructTemplateDeducerID> {
		using core::UniqueID<uint32_t, StructTemplateDeducerID>::UniqueID; 
	};

	struct UnionID : public core::UniqueID<uint32_t, struct UnionID> {
		using core::UniqueID<uint32_t, UnionID>::UniqueID; 
	};

	struct EnumID : public core::UniqueID<uint32_t, struct EnumID> {
		using core::UniqueID<uint32_t, EnumID>::UniqueID; 
	};

	struct TypeDeducerID : public core::UniqueID<uint32_t, struct TypeDeducerID> {
		using core::UniqueID<uint32_t, TypeDeducerID>::UniqueID; 
	};

	struct InterfaceID : public core::UniqueID<uint32_t, struct InterfaceID> {
		using core::UniqueID<uint32_t, InterfaceID>::UniqueID; 
	};

	struct PolyInterfaceRefID : public core::UniqueID<uint32_t, struct PolyInterfaceRefID> {
		using core::UniqueID<uint32_t, PolyInterfaceRefID>::UniqueID; 
	};

	struct InterfaceMapID : public core::UniqueID<uint32_t, struct InterfaceMapID> {
		using core::UniqueID<uint32_t, InterfaceMapID>::UniqueID; 
	};

}


namespace pcit::core{
	

	template<>
	struct OptionalInterface<panther::BaseType::PrimitiveID>{
		static constexpr auto init(panther::BaseType::PrimitiveID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::PrimitiveID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::FunctionID>{
		static constexpr auto init(panther::BaseType::FunctionID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::FunctionID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::ArrayID>{
		static constexpr auto init(panther::BaseType::ArrayID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::ArrayID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::ArrayDeducerID>{
		static constexpr auto init(panther::BaseType::ArrayDeducerID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::ArrayDeducerID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::ArrayRefID>{
		static constexpr auto init(panther::BaseType::ArrayRefID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::ArrayRefID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::ArrayRefDeducerID>{
		static constexpr auto init(panther::BaseType::ArrayRefDeducerID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::ArrayRefDeducerID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::AliasID>{
		static constexpr auto init(panther::BaseType::AliasID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::AliasID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::DistinctAliasID>{
		static constexpr auto init(panther::BaseType::DistinctAliasID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::DistinctAliasID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::StructID>{
		static constexpr auto init(panther::BaseType::StructID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::StructID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::StructTemplateID>{
		static constexpr auto init(panther::BaseType::StructTemplateID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::StructTemplateID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::StructTemplateDeducerID>{
		static constexpr auto init(panther::BaseType::StructTemplateDeducerID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::StructTemplateDeducerID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::UnionID>{
		static constexpr auto init(panther::BaseType::UnionID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::UnionID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::EnumID>{
		static constexpr auto init(panther::BaseType::EnumID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::EnumID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::TypeDeducerID>{
		static constexpr auto init(panther::BaseType::TypeDeducerID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::TypeDeducerID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::InterfaceID>{
		static constexpr auto init(panther::BaseType::InterfaceID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::InterfaceID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::PolyInterfaceRefID>{
		static constexpr auto init(panther::BaseType::PolyInterfaceRefID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::PolyInterfaceRefID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};



	template<>
	struct OptionalInterface<panther::BaseType::InterfaceMapID>{
		static constexpr auto init(panther::BaseType::InterfaceMapID* id) -> void {
			std::construct_at(id, std::numeric_limits<uint32_t>::max());
		}

		static constexpr auto has_value(const panther::BaseType::InterfaceMapID& id) -> bool {
			return id.get() != std::numeric_limits<uint32_t>::max();
		}
	};





}





namespace std{


	template<>
	struct hash<pcit::panther::BaseType::PrimitiveID>{
		auto operator()(const pcit::panther::BaseType::PrimitiveID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::PrimitiveID>
		: public pcit::core::Optional<pcit::panther::BaseType::PrimitiveID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::PrimitiveID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::PrimitiveID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::FunctionID>{
		auto operator()(const pcit::panther::BaseType::FunctionID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::FunctionID>
		: public pcit::core::Optional<pcit::panther::BaseType::FunctionID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::FunctionID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::FunctionID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::ArrayID>{
		auto operator()(const pcit::panther::BaseType::ArrayID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::ArrayID>
		: public pcit::core::Optional<pcit::panther::BaseType::ArrayID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::ArrayID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::ArrayID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::ArrayDeducerID>{
		auto operator()(const pcit::panther::BaseType::ArrayDeducerID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::ArrayDeducerID>
		: public pcit::core::Optional<pcit::panther::BaseType::ArrayDeducerID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::ArrayDeducerID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::ArrayDeducerID>::operator=;
	};




	template<>
	struct hash<pcit::panther::BaseType::ArrayRefID>{
		auto operator()(const pcit::panther::BaseType::ArrayRefID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::ArrayRefID>
		: public pcit::core::Optional<pcit::panther::BaseType::ArrayRefID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::ArrayRefID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::ArrayRefID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::ArrayRefDeducerID>{
		auto operator()(const pcit::panther::BaseType::ArrayRefDeducerID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::ArrayRefDeducerID>
		: public pcit::core::Optional<pcit::panther::BaseType::ArrayRefDeducerID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::ArrayRefDeducerID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::ArrayRefDeducerID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::AliasID>{
		auto operator()(const pcit::panther::BaseType::AliasID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::AliasID>
		: public pcit::core::Optional<pcit::panther::BaseType::AliasID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::AliasID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::AliasID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::DistinctAliasID>{
		auto operator()(const pcit::panther::BaseType::DistinctAliasID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::DistinctAliasID>
		: public pcit::core::Optional<pcit::panther::BaseType::DistinctAliasID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::DistinctAliasID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::DistinctAliasID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::StructID>{
		auto operator()(const pcit::panther::BaseType::StructID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::StructID>
		: public pcit::core::Optional<pcit::panther::BaseType::StructID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::StructID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::StructID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::StructTemplateID>{
		auto operator()(const pcit::panther::BaseType::StructTemplateID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::StructTemplateID>
		: public pcit::core::Optional<pcit::panther::BaseType::StructTemplateID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::StructTemplateID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::StructTemplateID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::StructTemplateDeducerID>{
		auto operator()(const pcit::panther::BaseType::StructTemplateDeducerID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::StructTemplateDeducerID>
		: public pcit::core::Optional<pcit::panther::BaseType::StructTemplateDeducerID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::StructTemplateDeducerID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::StructTemplateDeducerID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::UnionID>{
		auto operator()(const pcit::panther::BaseType::UnionID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::UnionID>
		: public pcit::core::Optional<pcit::panther::BaseType::UnionID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::UnionID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::UnionID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::EnumID>{
		auto operator()(const pcit::panther::BaseType::EnumID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::EnumID>
		: public pcit::core::Optional<pcit::panther::BaseType::EnumID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::EnumID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::EnumID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::TypeDeducerID>{
		auto operator()(const pcit::panther::BaseType::TypeDeducerID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::TypeDeducerID>
		: public pcit::core::Optional<pcit::panther::BaseType::TypeDeducerID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::TypeDeducerID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::TypeDeducerID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::InterfaceID>{
		auto operator()(const pcit::panther::BaseType::InterfaceID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::InterfaceID>
		: public pcit::core::Optional<pcit::panther::BaseType::InterfaceID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::InterfaceID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::InterfaceID>::operator=;
	};
	


	template<>
	struct hash<pcit::panther::BaseType::PolyInterfaceRefID>{
		auto operator()(const pcit::panther::BaseType::PolyInterfaceRefID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::PolyInterfaceRefID>
		: public pcit::core::Optional<pcit::panther::BaseType::PolyInterfaceRefID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::PolyInterfaceRefID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::PolyInterfaceRefID>::operator=;
	};



	template<>
	struct hash<pcit::panther::BaseType::InterfaceMapID>{
		auto operator()(const pcit::panther::BaseType::InterfaceMapID& id) const noexcept -> size_t {
			return hash<uint32_t>{}(id.get());
		};
	};
	template<>
	class optional<pcit::panther::BaseType::InterfaceMapID>
		: public pcit::core::Optional<pcit::panther::BaseType::InterfaceMapID>{

		public:
			using pcit::core::Optional<pcit::panther::BaseType::InterfaceMapID>::Optional;
			using pcit::core::Optional<pcit::panther::BaseType::InterfaceMapID>::operator=;
	};


	
}




namespace pcit::panther{

	struct EncapsulatingSymbolID{
		struct InterfaceImplInfo{
			TypeInfoID targetTypeID;
			BaseType::InterfaceID interfaceID;

			EVO_NODISCARD auto operator==(const InterfaceImplInfo&) const -> bool = default;
		};

		template<class T>
		EVO_NODISCARD auto is() const -> bool { return this->id.is<T>(); }

		template<class T>
		EVO_NODISCARD auto as() const -> const T& { return this->id.as<T>(); }

		template<class T>
		EVO_NODISCARD auto as() -> T& { return this->id.as<T>(); }


		auto visit(auto callable) const -> auto { return this->id.visit(callable); }
		auto visit(auto callable) -> auto { return this->id.visit(callable); }


		EVO_NODISCARD auto operator==(const EncapsulatingSymbolID&) const -> bool = default;


		evo::Variant<
			BaseType::StructID,
			BaseType::UnionID,
			BaseType::EnumID,
			BaseType::InterfaceID,
			sema::FuncID,
			InterfaceImplInfo
		> id;
	};
	
}