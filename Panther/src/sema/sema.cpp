////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/sema/sema.h"

#include "../../include/Context.h"


namespace pcit::panther::sema{


	auto Func::getName(const SourceManager& source_manager) const -> std::string_view {
		if(this->isClangFunc()){
			const ClangSource& clang_source = source_manager[this->sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(this->name.as<ClangSource::DeclInfoID>()).name;
		}else{
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[this->name.as<Token::ID>()].getString();
		}
	}

	auto Func::getParamName(const Param& param, const SourceManager& source_manager) const
	-> std::string_view {
		if(this->isClangFunc()){
			const ClangSource& clang_source = source_manager[this->sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(param.ident.as<ClangSource::DeclInfoID>()).name;
		}else{
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[param.ident.as<Token::ID>()].getString();
		}
	}


	auto Func::isEquivalentOverload(const Func& rhs, const Context& context) const -> bool {
		const BaseType::Function& this_type = context.getTypeManager().getFunction(this->typeID);
		const BaseType::Function& rhs_type = context.getTypeManager().getFunction(rhs.typeID);

		if(this->minNumArgs == rhs.minNumArgs){
			for(size_t i = 0; i < this->minNumArgs; i+=1){
				if(this_type.params[i].typeID != rhs_type.params[i].typeID){ return false; }
				if(this_type.params[i].kind != rhs_type.params[i].kind){ return false; }
			}
			
		}else{
			if(this_type.params.size() != rhs_type.params.size()){ return false; }

			for(size_t i = 0; i < this_type.params.size(); i+=1){
				if(this_type.params[i].typeID != rhs_type.params[i].typeID){ return false; }
				if(this_type.params[i].kind != rhs_type.params[i].kind){ return false; }
			}
		}

		return true;
	}


	auto Func::isMethod(const Context& context) const -> bool {
		if(this->params.empty()){ return false; }

		if(this->isClangFunc()){
			return false;
		}else{
			const Source& source = context.getSourceManager()[this->sourceID.as<Source::ID>()];
			const Token& first_param_token = source.getTokenBuffer()[this->params[0].ident.as<Token::ID>()];
			return first_param_token.kind() == Token::Kind::KEYWORD_THIS;
		}
	}



	auto GlobalVar::getName(const SourceManager& source_manager) const -> std::string_view {
		if(this->isClangVar()){
			const ClangSource& clang_source = source_manager[this->sourceID.as<ClangSource::ID>()];
			return clang_source.getDeclInfo(this->ident.as<ClangSource::DeclInfoID>()).name;
		}else{
			const Source& source = source_manager[this->sourceID.as<Source::ID>()];
			return source.getTokenBuffer()[this->ident.as<Token::ID>()].getString();
		}
	}




	auto TemplatedFunc::lookupInstantiation(evo::SmallVector<Arg>&& args) -> InstantiationInfo {
		const auto lock = std::scoped_lock(this->instantiation_lock);

		auto find = this->instantiation_map.find(args);
		if(find == this->instantiation_map.end()){
			const uint32_t instantiation_id = uint32_t(this->instantiations.size());
			Instantiation& new_instantiation = this->instantiations[this->instantiations.emplace_back()];
			this->instantiation_map.emplace(std::move(args), new_instantiation);
			return InstantiationInfo(new_instantiation, instantiation_id);

		}else{
			return InstantiationInfo(find->second, std::nullopt);
		}
	}


	auto TemplatedFunc::isMethod(const Context& context) const -> bool {
		const Source& source = context.getSourceManager()[this->symbolProc.getSourceID()];
		const AST::FuncDecl& ast_func = source.getASTBuffer().getFuncDecl(this->symbolProc.getASTNode());

		if(ast_func.params.empty()){ return false; }
		return ast_func.params[0].name.kind() == AST::Kind::THIS;
	}


}