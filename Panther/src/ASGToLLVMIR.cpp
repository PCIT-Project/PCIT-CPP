//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./ASGToLLVMIR.h"

#include "../include/Context.h"
#include "../include/Source.h"
#include "../include/SourceManager.h"



namespace pcit::panther{

	auto ASGToLLVMIR::lower() -> void {
		for(const Source::ID& source_id : this->context.getSourceManager()){
			const Source& source = this->context.getSourceManager()[source_id];

			for(const ASG::Func::ID func_id : source.getASGBuffer().getFuncs()){
				this->lower_func_decl(source, func_id);
			}
		}

		for(const Source::ID& source_id : this->context.getSourceManager()){
			const Source& source = this->context.getSourceManager()[source_id];

			for(const ASG::Func::ID func_id : source.getASGBuffer().getFuncs()){
				this->lower_func_body(source, func_id);
			}
		}
	}

	auto ASGToLLVMIR::lower_func_decl(const Source& source, ASG::Func::ID func_id) -> void {
		const ASG::Func& func = source.getASGBuffer().getFunc(func_id);

		const llvmint::FunctionType func_proto = this->builder.getFuncProto(this->builder.getTypeVoid(), {}, false);
		const auto linkage = llvmint::LinkageType::Internal;

		llvmint::Function llvm_func = this->module.createFunction(this->mangle_name(source, func), func_proto, linkage);
		llvm_func.setNoThrow();
		llvm_func.setCallingConv(llvmint::CallingConv::Fast);

		this->func_infos.emplace(ASG::Func::LinkID(source.getID(), func_id), FuncInfo(llvm_func));

		this->builder.createBasicBlock(llvm_func, "begin");
	}


	auto ASGToLLVMIR::lower_func_body(const Source& source, ASG::Func::ID func_id) -> void {
		const FuncInfo& func_info = this->func_infos.find(ASG::Func::LinkID(source.getID(), func_id))->second;

		this->builder.setInsertionPointAtBack(func_info.func);
		this->builder.createRet();
	}


	auto ASGToLLVMIR::mangle_name(const Source& source, const ASG::Func& func) const -> std::string {
		return std::format(
			"PTHR.{}{}.{}",
			source.getID().get(),
			this->submangle_parent(source, func.parent),
			this->get_func_ident_name(source, func)
		);
	}


	auto ASGToLLVMIR::submangle_parent(const Source& source, const ASG::Parent& parent) const -> std::string {
		return parent.visit([&](auto parent_id) -> std::string {
			using ParentID = std::decay_t<decltype(parent_id)>;

			if constexpr(std::is_same_v<ParentID, std::monostate>){
				return "";
			}else if constexpr(std::is_same_v<ParentID, ASG::Func::ID>){
				const ASG::Func& parent_func = source.getASGBuffer().getFunc(parent_id);
				return std::format(
					"{}.{}",
					this->submangle_parent(source, parent_func.parent),
					this->get_func_ident_name(source, parent_func)
				);
			}
		});
	}


	auto ASGToLLVMIR::get_func_ident_name(const Source& source, const ASG::Func& func) const -> std::string {
		return std::string(source.getTokenBuffer()[source.getASTBuffer().getIdent(func.name)].getString());
	}
	
}