////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <filesystem>

#include <Evo.h>

#include "../../../include/StepVector.h"
#include "./Type.h"


namespace pcit::clangint{


	class API{
		public:
			struct Alias{
				std::string name;
				Type type;

				std::filesystem::path declFilePath;
				uint32_t declLine;
				uint32_t declCollumn;
			};


			struct Decl{
				Decl(auto* decl_ptr) : ptr(decl_ptr) {}

				template<class T>
				EVO_NODISCARD auto is() const -> bool { return this->ptr.is<T*>(); }

				template<class T>
				EVO_NODISCARD auto as() const -> const T& { return *this->ptr.as<T*>(); }

				template<class T>
				EVO_NODISCARD auto as() -> T& { return *this->ptr.as<T*>(); }


				auto visit(auto callable) const { return this->ptr.visit(callable); }
				auto visit(auto callable)       { return this->ptr.visit(callable); }

				
				private:
					evo::Variant<Alias*> ptr;
			};


		public:
			API() = default;
			~API() = default;


			auto addAlias(auto&&... alias_args) -> void {
				this->decls.emplace_back(
					&this->aliases.emplace_back(std::forward<decltype(alias_args)>(alias_args)...)
				);
			}


			EVO_NODISCARD auto getDecls() const -> evo::ArrayProxy<Decl> { return this->decls; }



			EVO_NODISCARD auto lookupSpecialPrimitive(std::string_view name) const 
			-> std::optional<BaseType::Primitive> {
				const auto find = this->special_primitive_lookup.find(name);
				if(find != this->special_primitive_lookup.end()){ return find->second; }
				return std::nullopt;
			}

	
		private:
			evo::SmallVector<Decl> decls{};

			core::StepVector<Alias> aliases{};

			const std::unordered_map<std::string_view, BaseType::Primitive> special_primitive_lookup{
				{"ptrdiff_t", BaseType::Primitive::ISIZE},
				{"ssize_t",   BaseType::Primitive::ISIZE},
				{"size_t",    BaseType::Primitive::USIZE},
				{"int8_t",    BaseType::Primitive::I8},
				{"int16_t",   BaseType::Primitive::I16},
				{"int32_t",   BaseType::Primitive::I32},
				{"int64_t",   BaseType::Primitive::I64},
				{"uint8_t",   BaseType::Primitive::UI8},
				{"uint16_t",  BaseType::Primitive::UI16},
				{"uint32_t",  BaseType::Primitive::UI32},
				{"uint64_t",  BaseType::Primitive::UI64},
			};
			using SpecialPrimitiveLookupType = 
				std::unordered_map<std::string_view, BaseType::Primitive>::const_iterator;
			const SpecialPrimitiveLookupType special_primitive_lookup_end = special_primitive_lookup.cend();
	
	};



}