////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/DIBuilder.hpp"

#include <LLVM.hpp>

#include "../include/Module.hpp"
#include "../include/class_impls/values.hpp"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::llvmint{


	DIBuilder::DIBuilder(Module& _module) : module(_module), builder(new llvm::DIBuilder(*_module.native())) {}
	
	DIBuilder::~DIBuilder(){
		delete this->builder;
	}



	auto DIBuilder::BasicType::asType() -> Type {
		return Type(static_cast<llvm::DIType*>(this->type));
	}

	auto DIBuilder::DerivedType::asType() -> Type {
		return Type(static_cast<llvm::DIType*>(this->type));
	}

	auto DIBuilder::CompositeType::asType() -> Type {
		return Type(static_cast<llvm::DIType*>(this->type));
	}

	auto DIBuilder::SubroutineType::asType() -> Type {
		return Type(static_cast<llvm::DIType*>(this->type));
	}


	auto DIBuilder::Subprogram::asScope() -> Scope {
		return Scope(static_cast<llvm::DIScope*>(this->subprogram));
	}
	auto DIBuilder::Subprogram::asLocalScope() -> LocalScope {
		return LocalScope(static_cast<llvm::DILocalScope*>(this->subprogram));
	}

	auto DIBuilder::File::asScope() -> Scope {
		return Scope(static_cast<llvm::DIScope*>(this->file));
	}



	auto DIBuilder::addModuleLevelDebugInfo(core::Target target) -> void {
		this->module.native()->addModuleFlag(
			llvm::Module::ModFlagBehavior::Warning, "Debug Info Version", llvm::LLVMConstants::DEBUG_METADATA_VERSION
		);

		if(target.platform == core::Target::Platform::WINDOWS){
			this->module.native()->addModuleFlag(llvm::Module::ModFlagBehavior::Warning, "CodeView", 1);
		}
	}


	auto DIBuilder::createCompileUnit(Language language, File file, std::string_view producer, bool is_optimized)
	-> void {
		this->builder->createCompileUnit(language.dwarfCode, file.file, producer, is_optimized, "", 0);
	}



	auto DIBuilder::createFile(std::string_view file_name, std::string_view directory) -> File {
		return File(this->builder->createFile(file_name, directory));
	}


	auto DIBuilder::createVoidType() -> Type {
		return Type(nullptr);
	}


	auto DIBuilder::createBasicType(std::string_view name, uint64_t size_in_bits, BasicTypeKind basic_type_kind)
	-> BasicType {
		return BasicType(this->builder->createBasicType(name, size_in_bits, basic_type_kind.dwarfCode));
	}


	auto DIBuilder::createPointerType(Type pointee_type, uint64_t size_in_bits, std::string_view name) -> DerivedType {
		return DerivedType(this->builder->createPointerType(pointee_type.type, size_in_bits, 0, std::nullopt, name));
	}

	auto DIBuilder::createConstType(Type target_type) -> DerivedType {
		return DerivedType(this->builder->createQualifiedType(llvm::dwarf::Tag::DW_TAG_const_type, target_type.type));
	}



	auto DIBuilder::createSubroutineType(Type ret_type, evo::ArrayProxy<Type> args) -> SubroutineType {
		auto metadatas = evo::SmallVector<llvm::Metadata*, 16>();
		metadatas.reserve(args.size() + 1);

		metadatas.emplace_back(ret_type.type);

		for(Type arg : args){
			metadatas.emplace_back(arg.type);
		}

		llvm::DITypeRefArray type_ref_array = this->builder->getOrCreateTypeArray(
			llvm::ArrayRef<llvm::Metadata*>(metadatas.data(), metadatas.size())
		);

		return SubroutineType(this->builder->createSubroutineType(type_ref_array));
	}


	auto DIBuilder::createFunction(
		Scope scope,
		std::string_view name,
		std::string_view linkage_name,
		File file,
		uint32_t line_number,
		SubroutineType subroutine_type
	) -> Subprogram {
		return Subprogram(
			this->builder->createFunction(
				scope.scope,
				name,
				linkage_name,
				file.file,
				line_number,
				subroutine_type.type,
				line_number,
				llvm::DINode::FlagZero,
				llvm::DISubprogram::SPFlagDefinition
			)
		);
	}


	auto DIBuilder::createClassType(
		Scope scope,
		std::string_view name,
		File file,
		uint32_t line_number,
		uint64_t size_in_bits,
		uint32_t align_in_bits,
		evo::ArrayProxy<DerivedType> members
	) -> CompositeType {
		auto metadatas = evo::SmallVector<llvm::Metadata*, 64>();
		for(const DerivedType& member : members){
			metadatas.emplace_back(member.type);
		}

		llvm::DINodeArray elements_array = this->builder->getOrCreateArray(
			llvm::ArrayRef<llvm::Metadata*>(metadatas.data(), metadatas.size())
		);

		return CompositeType(
			this->builder->createClassType(
				scope.scope,
				name,
				file.file,
				line_number,
				size_in_bits,
				align_in_bits,
				0,
				llvm::DINode::FlagZero,
				nullptr,
				elements_array
			)
		);
	}


	auto DIBuilder::createMemberType(
		Scope scope,
		std::string_view name,
		File file,
		uint32_t line_number,
		uint64_t size_in_bits,
		uint32_t align_in_bits,
		uint64_t offset_in_bits,
		Type type
	) -> DerivedType {
		return DerivedType(
			this->builder->createMemberType(
				scope.scope,
				name,
				file.file,
				line_number,
				size_in_bits,
				align_in_bits,
				offset_in_bits,
				llvm::DINode::FlagZero,
				type.type
			)
		);
	}


	auto DIBuilder::createArrayType(
		Type element_type, uint64_t number_of_elements, uint64_t size_in_bits, uint32_t align_in_bits
	) -> CompositeType {
		auto subranges = std::array<llvm::Metadata*, 1>{this->builder->getOrCreateSubrange(0, number_of_elements)};

		llvm::DINodeArray subscripts = this->builder->getOrCreateArray(
			llvm::ArrayRef<llvm::Metadata*>(subranges.data(), subranges.size())
		);

		return CompositeType(
			this->builder->createArrayType(size_in_bits, align_in_bits, element_type.type, subscripts)
		);
	}



	auto DIBuilder::createEnumType(
		Scope scope,
		std::string_view name,
		File file,
		uint32_t line_number,
		uint64_t size_in_bits,
		uint32_t align_in_bits,
		evo::ArrayProxy<Enumerator> enumerators,
		Type underlying_type
	) -> CompositeType {

		llvm::DINodeArray elements = this->builder->getOrCreateArray(
			llvm::ArrayRef<llvm::Metadata*>((llvm::Metadata**)enumerators.data(), enumerators.size())
		);
		
		return CompositeType(
			this->builder->createEnumerationType(
				scope.scope,
				name, 
				file.file,
				line_number,
				size_in_bits,
				align_in_bits,
				elements,
				underlying_type.type
			)
		);
	}


	auto DIBuilder::createEnumerator(std::string_view name, const core::GenericInt& value, bool is_unsigned)
	-> Enumerator {
		return Enumerator(
			this->builder->createEnumerator(
				name,
				llvm::APSInt(
					llvm::APInt(value.getBitWidth(), llvm::ArrayRef(value.data(), value.numWords())), is_unsigned
				)
			)
		);
	}



	auto DIBuilder::addLocalVariable(
		LocalScope scope,
		std::string_view name,
		uint32_t line_number,
		uint32_t collumn_number,
		Type type,
		BasicBlock basic_block,
		const Value& value
	) -> void {
		llvm::DILocalVariable* local_var =
			this->builder->createAutoVariable(scope.scope, name, scope.scope->getFile(), line_number, type.type);

		this->builder->insertDeclare(
			value.native(),
			local_var,
			this->builder->createExpression(llvm::ArrayRef<uint64_t>()),
			this->createSourceLocation(scope, line_number, collumn_number).location,
			basic_block.native()
		);
	}



	auto DIBuilder::createSourceLocation(LocalScope local_scope, uint32_t line, uint32_t collumn) -> Location {
		return Location(llvm::DILocation::get(this->module.native()->getContext(), line, collumn, local_scope.scope));
	}


		
}