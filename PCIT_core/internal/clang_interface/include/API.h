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

			struct Struct{
				struct Member{
					enum class Access{
						PUBLIC,
						PROTECTED,
						PRIVATE,
					};

					std::string name;
					Type type;
					Access access;

					uint32_t declLine;
					uint32_t declCollumn;
				};

				std::string name;
				evo::SmallVector<Member> members;

				std::filesystem::path declFilePath;
				uint32_t declLine;
				uint32_t declCollumn;
			};

			struct Union{
				struct Field{
					std::string name;
					Type type;

					uint32_t declLine;
					uint32_t declCollumn;
				};

				std::string name;
				evo::SmallVector<Field> fields;

				std::filesystem::path declFilePath;
				uint32_t declLine;
				uint32_t declCollumn;
			};

			struct Function{
				struct Param{
					std::string name;

					uint32_t declLine;
					uint32_t declCollumn;
				};

				std::string name;
				std::string mangled_name;
				BaseType::Function type;
				evo::SmallVector<Param> params;
				bool isNoReturn;
				bool isInlined;
				bool isVariadic;

				std::filesystem::path declFilePath;
				uint32_t declLine;
				uint32_t declCollumn;
			};


			struct GlobalVar{
				std::string name;
				std::string mangled_name;
				Type type;
				bool isConst;

				std::filesystem::path declFilePath;
				uint32_t declLine;
				uint32_t declCollumn;
			};


			struct Macro{
				std::string name;

				std::filesystem::path declFilePath; // empty if not from a file
				uint32_t declLine; // 0 if not from a file
				uint32_t declCollumn;  // 0 if not from a file
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
					evo::Variant<Alias*, Struct*, Union*, Function*, GlobalVar*> ptr;
			};


		public:
			API() = default;
			~API() = default;


			auto addAlias(auto&&... alias_args) -> void {
				Alias& created_alias = this->aliases.emplace_back(std::forward<decltype(alias_args)>(alias_args)...);

				this->decls.emplace_back(&created_alias);
			}

			auto addStruct(auto&&... struct_args) -> void {
				Struct& created_struct =
					this->structs.emplace_back(std::forward<decltype(struct_args)>(struct_args)...);

				this->decls.emplace_back(&created_struct);
			}

			auto addUnion(auto&&... union_args) -> void {
				Union& created_union = this->unions.emplace_back(std::forward<decltype(union_args)>(union_args)...);

				this->decls.emplace_back(&created_union);
			}

			auto addFunction(auto&&... function_args) -> void {
				Function& created_function =
					this->functions.emplace_back(std::forward<decltype(function_args)>(function_args)...);

				this->decls.emplace_back(&created_function);
			}

			auto addGlobalVar(auto&&... global_var_args) -> void {
				GlobalVar& created_global_var =
					this->global_vars.emplace_back(std::forward<decltype(global_var_args)>(global_var_args)...);

				this->decls.emplace_back(&created_global_var);
			}


			auto addMacro(auto&&... macro_args) -> void {
				this->macros.emplace_back(std::forward<decltype(macro_args)>(macro_args)...);
			}


			EVO_NODISCARD auto getDecls() const -> evo::ArrayProxy<Decl> { return this->decls; }

			EVO_NODISCARD auto getMacros() const -> evo::IterRange<evo::StepVector<Macro>::const_iterator> {
				return evo::IterRange<evo::StepVector<Macro>::const_iterator>(this->macros.begin(), this->macros.end());
			}



			EVO_NODISCARD auto lookupSpecialPrimitive(std::string_view name) const 
			-> std::optional<BaseType::Primitive> {
				const auto find = this->special_primitive_lookup.find(name);
				if(find != this->special_primitive_lookup.end()){ return find->second; }
				return std::nullopt;
			}

	
		private:
			evo::SmallVector<Decl> decls{};

			evo::StepVector<Alias> aliases{};
			evo::StepVector<Struct> structs{};
			evo::StepVector<Union> unions{};
			evo::StepVector<Function> functions{};
			evo::StepVector<GlobalVar> global_vars{};

			evo::StepVector<Macro> macros{};



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