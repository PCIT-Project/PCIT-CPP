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

#include "./source_data.hpp"
#include "../TypeManager.hpp"
#include "../sema/sema.hpp"



namespace pcit::panther{


	class BuiltinModule{
		public:
			using ID = BuiltinModuleID;
			using StringID = BuiltinModuleStringID;

			using Symbol = evo::Variant<BaseType::ID, sema::Func::ID, sema::GlobalVar::ID>;


		public:
			BuiltinModule() = default;
			~BuiltinModule() = default;



			[[nodiscard]] auto createSymbol(std::string_view symbol_name, Symbol symbol) -> void {
				this->symbols.emplace(symbol_name, symbol);
			}

			[[nodiscard]] auto getSymbol(std::string_view symbol_name) const -> std::optional<Symbol> {
				const auto find = this->symbols.find(symbol_name);
				if(find != this->symbols.end()){ return find->second; }
				return std::nullopt;
			}


			[[nodiscard]] auto createString(std::string&& str) -> StringID {
				return this->strings.emplace_back(std::move(str));
			}

			[[nodiscard]] auto getString(StringID id) const -> std::string_view {
				return this->strings[id];
			}

	
		private:
			core::LinearStepAlloc<std::string, StringID> strings{};
			std::unordered_map<std::string_view, Symbol> symbols{};
			
	};

	
}