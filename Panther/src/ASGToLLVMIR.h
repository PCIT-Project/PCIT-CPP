//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include <llvm_interface.h>

#include "../include/ASG.h"


namespace pcit::panther{

	class Source;
	class Context;

	class ASGToLLVMIR{
		public:
			ASGToLLVMIR(Context& _context, llvmint::LLVMContext& llvm_context, llvmint::Module& _module)
				: context(_context), module(_module), builder(llvm_context) {};

			~ASGToLLVMIR() = default;

			auto lower() -> void;

		private:
			auto lower_func_decl(const Source& source, ASG::Func::ID func_id) -> void;
			auto lower_func_body(const Source& source, ASG::Func::ID func_id) -> void;

			auto mangle_name(const Source& source, const ASG::Func& func) const -> std::string;
			auto submangle_parent(const Source& source, const ASG::Parent& parent) const -> std::string;
			auto get_func_ident_name(const Source& source, const ASG::Func& func) const -> std::string;
	
		private:
			Context& context;
			llvmint::Module& module;

			llvmint::IRBuilder builder;


			struct FuncInfo{
				llvmint::Function func;
			};

			std::unordered_map<ASG::Func::LinkID, FuncInfo> func_infos{};
	};

	
}