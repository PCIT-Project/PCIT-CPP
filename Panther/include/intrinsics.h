//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./TypeManager.h"

namespace pcit::panther{


	struct Intrinsic{
		// Use ID to lookup Intrinsics in Context
		enum class Kind : uint32_t {
			Breakpoint,
			_printHelloWorld,

			_max_,
		};

		BaseType::ID baseType;

		Intrinsic() : baseType(BaseType::ID::dummy()) {};
		Intrinsic(BaseType::ID base_type) : baseType(base_type) {};
	};


	struct TemplatedIntrinsic{
		// Use ID to lookup TemplatedIntrinsics in Context
		enum class Kind : uint32_t {
			SizeOf,

			_max_
		};


		struct Param{
			strings::StringCode ident;
			evo::Variant<TypeInfo::ID, uint32_t> type; // uint32_t is the index of the templateParam
			AST::FuncDecl::Param::Kind kind;
		};

		using ReturnParam = evo::Variant<TypeInfo::VoidableID, uint32_t>; // uint32_t is the index of the templateParam

		evo::SmallVector<std::optional<TypeInfo::ID>> templateParams; // nullopt means it's a `Type` param
		evo::SmallVector<Param> params;
		evo::SmallVector<ReturnParam> returns;


		// nullopt if is an expr argument
		EVO_NODISCARD auto getTypeInstantiation(
			evo::SmallVector<std::optional<TypeInfo::VoidableID>> template_args
		) const -> BaseType::Function {
			auto instantiated_params = evo::SmallVector<BaseType::Function::Param>();
			instantiated_params.reserve(this->params.size());

			auto instantiated_returns = evo::SmallVector<BaseType::Function::ReturnParam>();
			instantiated_returns.reserve(this->returns.size());

			for(const Param& param : this->params){
				const TypeInfo::ID param_type = param.type.visit([&](const auto& param_type) -> TypeInfo::ID {
					if constexpr(std::is_same_v<std::decay_t<decltype(param_type)>, TypeInfo::ID>){
						return param_type;
					}else{
						return template_args[param_type]->typeID();
					}
				});

				instantiated_params.emplace_back(param.ident, param_type, param.kind, false, false);
			}

			for(const ReturnParam& return_param : this->returns){
				const TypeInfo::VoidableID return_type = return_param.visit([&](const auto& return_data){
					if constexpr(std::is_same_v<std::decay_t<decltype(return_data)>, TypeInfo::VoidableID>){
						return return_data;
					}else{
						return *template_args[return_data];
					}
				});

				instantiated_returns.emplace_back(std::nullopt, return_type);
			}

			return BaseType::Function(std::move(instantiated_params), std::move(instantiated_returns), false);
		}
	};
	

}
