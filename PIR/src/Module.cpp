////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/Module.h"

#include "../include/ReaderAgent.h"

#include <unordered_set>

namespace pcit::pir{
	

	auto Module::getExprType(const Expr& expr) const -> Type {
		evo::debugAssert(
			expr.isConstant(),
			"Module can only get value of expr that is a constant. Use Function::getExprType() instead "
			"(where Function is the function the expr is from"
		);

		switch(expr.getKind()){
			case Expr::Kind::GlobalValue: return this->createTypePtr();
			case Expr::Kind::Number:      return ReaderAgent(*this).getNumber(expr).type;
		}

		evo::debugFatalBreak("Unknown or unsupported constant expr kind");
	}


	#if defined(PCIT_CONFIG_DEBUG)
		auto Module::check_param_names(evo::ArrayProxy<Parameter> params) const -> void {
			auto names_seen = std::unordered_set<std::string_view>();

			for(const Parameter& param : params){
				evo::debugAssert(param.getName().empty() == false, "Parameter must have name");
				evo::debugAssert(
					isStandardName(param.getName()), "Invalid name for parameter ({})", param.getName()
				);
				evo::debugAssert(names_seen.contains(param.getName()) == false, "Parameter name already used");

				names_seen.emplace(param.getName());
			}
		}

		auto Module::check_global_name_reusue(std::string_view global_name) const -> void {
			for(const Function& func : this->functions){
				evo::debugAssert(func.getName() != global_name, "global \"{}\" already used", global_name);
			}

			for(const FunctionDecl& func_decl : this->function_decls){
				evo::debugAssert(func_decl.name != global_name, "global \"{}\" already used", global_name);
			}

			for(const GlobalVar& global_var : this->global_vars){
				evo::debugAssert(global_var.name != global_name, "global \"{}\" already used", global_name);
			}
		}
	#endif

}