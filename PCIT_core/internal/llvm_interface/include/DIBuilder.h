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


#include "./class_impls/native_ptr_decls.h"


namespace pcit::llvmint{

	
	class DIBuilder{
		public:
			struct Type { llvm::DIType* type; };
			struct Scope { llvm::DIScope* scope; };
			struct LocalScope { llvm::DILocalScope* scope; };


			//////////////////
			// type

			struct BasicType {
				llvm::DIBasicType* type;
				EVO_NODISCARD auto asType() -> Type;
			};

			struct DerivedType {
				llvm::DIDerivedType* type;
				EVO_NODISCARD auto asType() -> Type;
			};

			struct CompositeType {
				llvm::DICompositeType* type;
				EVO_NODISCARD auto asType() -> Type;
			};

			struct SubroutineType {
				llvm::DISubroutineType* type;
				EVO_NODISCARD auto asType() -> Type;
			};


			//////////////////
			// scope

			struct Subprogram {
				llvm::DISubprogram* subprogram;
				EVO_NODISCARD auto asScope() -> Scope;
				EVO_NODISCARD auto asLocalScope() -> LocalScope;
			};


			struct File {
				llvm::DIFile* file;
				EVO_NODISCARD auto asScope() -> Scope;
			};


			//////////////////
			// misc

			struct Location { llvm::DILocation* location; };


		public:
			struct Language{
				enum class Kind : unsigned {
					PANTHER = 0x000c, // using the C99 code for now, beacuse that's what Zig and Odin do
					C       = 0x000c, // C99
					CPP     = 0x0021, // C++14
				};

				using enum class Kind;
				
				Language(Kind kind) : dwarfCode(evo::to_underlying(kind)) {}
				Language(unsigned dwarf_code) : dwarfCode(dwarf_code) {}

				unsigned dwarfCode;
			};


			struct BasicTypeKind{
				enum class Kind : unsigned {
					BOOL          = 0x02,
					FLOAT         = 0x04,
					SIGNED_INT    = 0x05,
					UNSIGNED_INT  = 0x07,
					SIGNED_CHAR   = 0x06,
					UNSIGNED_CHAR = 0x08,
				};

				using enum class Kind;
				
				BasicTypeKind(Kind kind) : dwarfCode(evo::to_underlying(kind)) {}
				BasicTypeKind(unsigned dwarf_code) : dwarfCode(dwarf_code) {}

				unsigned dwarfCode;
			};


		public:
			DIBuilder(class Module& _module);
			~DIBuilder();

			auto addModuleLevelDebugInfo(core::Target target) -> void;


			auto createCompileUnit(Language language, File file, std::string_view producer, bool is_optimized) -> void;

			EVO_NODISCARD auto createFile(std::string_view file_name, std::string_view directory) -> File;

			EVO_NODISCARD auto createVoidType() -> Type;

			EVO_NODISCARD auto createBasicType(
				std::string_view name, uint64_t size_in_bits, BasicTypeKind basic_type_kind
			) -> BasicType;


			EVO_NODISCARD auto createPointerType(Type pointee_type, uint64_t size_in_bits, std::string_view name)
				-> DerivedType;

			EVO_NODISCARD auto createConstType(Type target_type) -> DerivedType;


			EVO_NODISCARD auto createSubroutineType(Type ret_type, evo::ArrayProxy<Type> args) -> SubroutineType;

			EVO_NODISCARD auto createFunction(
				Scope scope,
				std::string_view name,
				std::string_view linkage_name,
				File file,
				uint32_t line_number,
				SubroutineType subroutine_type
			) -> Subprogram;

			EVO_NODISCARD auto createClassType(
				Scope scope,
				std::string_view name,
				File file,
				uint32_t line_number,
				uint64_t size_in_bits,
				uint32_t align_in_bits,
				evo::ArrayProxy<DerivedType> members
			) -> CompositeType;

			EVO_NODISCARD auto createMemberType(
				Scope scope,
				std::string_view name,
				File file,
				uint32_t line_number,
				uint64_t size_in_bits,
				uint32_t align_in_bits,
				uint64_t offset_in_bits,
				Type type
			) -> DerivedType;


			EVO_NODISCARD auto createArrayType(
				Type element_type, uint64_t number_of_elements, uint64_t size_in_bits, uint32_t align_in_bits
			) -> CompositeType;
			


			EVO_NODISCARD auto createSourceLocation(LocalScope local_scope, uint32_t line, uint32_t collumn) 
				-> Location;

	
		private:
			class Module& module;
			llvm::DIBuilder* builder = nullptr;
	};

	
}