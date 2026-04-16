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


#include "./meta_ids.hpp"
#include "./Type.hpp"
#include "./forward_decl_ids.hpp"



namespace pcit::pir::meta{


	enum class Language{
		PANTHER,
		C,
		CPP, 
	};

	
	struct File{
		using ID = FileID;

		std::string metaName;
		std::string path;
		Language language;
		std::string producerName;
	};


	struct BasicType{
		using ID = BasicTypeID;

		std::string metaName;
		std::string typeName;
		pir::Type underlyingType;
	};


	struct QualifiedType{
		using ID = QualifiedTypeID;

		enum class Qualifier{
			POINTER,
			MUT_POINTER,
		};

		std::string metaName;
		std::string typeName;
		std::optional<meta::Type> qualeeType; // nullptr if RawPtr or void*
		Qualifier qualifier;
	};


	struct StructType{
		using ID = StructTypeID;

		struct Member{
			meta::Type type;
			std::string name;
		};

		std::string metaName;
		pir::Type structType;
		std::string typeName;
		meta::FileID fileID;
		meta::Scope scopeWhereDefined;
		uint32_t lineNumber;
		evo::SmallVector<Member> members;
	};


	struct ArrayType{
		using ID = ArrayTypeID;

		std::string metaName;
		pir::Type arrayType;
		meta::Type elementType;
		evo::SmallVector<uint64_t> dimensions;
	};


	struct Function{
		using ID = FunctionID;

		std::string metaName;
		std::string unmangledName;
		meta::File::ID fileID;
		meta::Scope scopeWhereDefined;
		uint32_t lineNumber;
		std::optional<meta::Type> returnMetaType; // nullopt if `Void`
		evo::SmallVector<meta::Type> paramMetaTypes;
	};


	struct SourceLocation{
		LocalScope scope;
		uint32_t line;
		uint32_t collumn;
	};

}