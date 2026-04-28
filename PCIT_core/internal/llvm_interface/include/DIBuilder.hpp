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


#include "./class_impls/native_ptr_decls.hpp"
#include "./class_impls/stmts.hpp"


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
				[[nodiscard]] auto asType() -> Type;
			};

			struct DerivedType {
				llvm::DIDerivedType* type;
				[[nodiscard]] auto asType() -> Type;
			};

			struct CompositeType {
				llvm::DICompositeType* type;
				[[nodiscard]] auto asType() -> Type;
			};

			struct SubroutineType {
				llvm::DISubroutineType* type;
				[[nodiscard]] auto asType() -> Type;
			};


			//////////////////
			// scope

			struct Subprogram {
				llvm::DISubprogram* subprogram;
				[[nodiscard]] auto asScope() -> Scope;
				[[nodiscard]] auto asLocalScope() -> LocalScope;
			};


			struct File {
				llvm::DIFile* file;
				[[nodiscard]] auto asScope() -> Scope;
			};


			//////////////////
			// misc

			struct Location { llvm::DILocation* location; };
			struct Enumerator { llvm::DIEnumerator* enumerator; };


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

			[[nodiscard]] auto createFile(std::string_view file_name, std::string_view directory) -> File;

			[[nodiscard]] auto createVoidType() -> Type;

			[[nodiscard]] auto createBasicType(
				std::string_view name, uint64_t size_in_bits, BasicTypeKind basic_type_kind
			) -> BasicType;


			[[nodiscard]] auto createPointerType(Type pointee_type, uint64_t size_in_bits, std::string_view name)
				-> DerivedType;

			[[nodiscard]] auto createReferenceType(Type pointee_type, uint64_t size_in_bits) -> DerivedType;

			[[nodiscard]] auto createConstType(Type target_type) -> DerivedType;


			[[nodiscard]] auto createSubroutineType(Type ret_type, evo::ArrayProxy<Type> args) -> SubroutineType;

			[[nodiscard]] auto createFunction(
				Scope scope,
				std::string_view name,
				std::string_view linkage_name,
				File file,
				uint32_t line_number,
				SubroutineType subroutine_type
			) -> Subprogram;


			[[nodiscard]] auto createClassType(
				Scope scope,
				std::string_view name,
				File file,
				uint32_t line_number,
				uint64_t size_in_bits,
				uint32_t align_in_bits,
				evo::ArrayProxy<DerivedType> members
			) -> CompositeType;

			[[nodiscard]] auto createUnionType(
				Scope scope,
				std::string_view name,
				File file,
				uint32_t line_number,
				uint64_t size_in_bits,
				uint32_t align_in_bits,
				evo::ArrayProxy<DerivedType> members
			) -> CompositeType;

			[[nodiscard]] auto createMemberType(
				Scope scope,
				std::string_view name,
				File file,
				uint32_t line_number,
				uint64_t size_in_bits,
				uint32_t align_in_bits,
				uint64_t offset_in_bits,
				Type type
			) -> DerivedType;


			[[nodiscard]] auto createArrayType(
				Type element_type, uint64_t number_of_elements, uint64_t size_in_bits, uint32_t align_in_bits
			) -> CompositeType;


			[[nodiscard]] auto createEnumType(
				Scope scope,
				std::string_view name,
				File file,
				uint32_t line_number,
				uint64_t size_in_bits,
				uint32_t align_in_bits,
				evo::ArrayProxy<Enumerator> enumerators,
				Type underlying_type
			) -> CompositeType;

			[[nodiscard]] auto createEnumerator(std::string_view name, const core::GenericInt& value, bool is_unsigned)
				-> Enumerator;



			auto addLocalVariable(
				LocalScope scope,
				std::string_view name,
				uint32_t line_number,
				uint32_t collumn_number,
				Type type,
				BasicBlock basic_block,
				const class Value& value
			) -> void;

			auto addParam(
				LocalScope scope,
				std::string_view name,
				uint32_t arg_number,
				uint32_t line_number,
				uint32_t collumn_number,
				Type type,
				BasicBlock basic_block,
				const class Value& value
			) -> void;


			[[nodiscard]] auto createSourceLocation(LocalScope local_scope, uint32_t line, uint32_t collumn) 
				-> Location;

	
		private:
			class Module& module;
			llvm::DIBuilder* builder = nullptr;
	};

	
}