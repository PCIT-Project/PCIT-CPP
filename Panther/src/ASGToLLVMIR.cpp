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


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{

	auto ASGToLLVMIR::lower() -> void {
		for(const Source::ID& source_id : this->context.getSourceManager()){
			this->current_source = &this->context.getSourceManager()[source_id];

			for(const ASG::Func::ID func_id : this->current_source->getASGBuffer().getFuncs()){
				this->lower_func_decl(func_id);
			}
		}

		for(const Source::ID& source_id : this->context.getSourceManager()){
			this->current_source = &this->context.getSourceManager()[source_id];

			for(const ASG::Func::ID func_id : this->current_source->getASGBuffer().getFuncs()){
				this->lower_func_body(func_id);
			}
		}

		this->current_source = nullptr;
	}

	auto ASGToLLVMIR::lower_func_decl(ASG::Func::ID func_id) -> void {
		const ASG::Func& func = this->current_source->getASGBuffer().getFunc(func_id);

		const llvmint::FunctionType func_proto = this->builder.getFuncProto(this->builder.getTypeVoid(), {}, false);
		const auto linkage = llvmint::LinkageType::Internal;

		llvmint::Function llvm_func = this->module.createFunction(this->mangle_name(func), func_proto, linkage);
		llvm_func.setNoThrow();
		llvm_func.setCallingConv(llvmint::CallingConv::Fast);

		this->func_infos.emplace(ASG::Func::LinkID(this->current_source->getID(), func_id), FuncInfo(llvm_func));

		this->builder.createBasicBlock(llvm_func, "begin");
	}


	auto ASGToLLVMIR::lower_func_body(ASG::Func::ID func_id) -> void {
		const ASG::Func& asg_func = this->current_source->getASGBuffer().getFunc(func_id);
		auto link_id = ASG::Func::LinkID(this->current_source->getID(), func_id);
		const FuncInfo& func_info = this->func_infos.find(link_id)->second;

		this->builder.setInsertionPointAtBack(func_info.func);

		for(const ASG::Stmt& stmt : asg_func.stmts){
			this->lower_stmt(stmt);
		}

		this->builder.createRet();
	}



	auto ASGToLLVMIR::lower_stmt(const ASG::Stmt& stmt) -> void {
		const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();

		switch(stmt.kind()){
			break; case ASG::Stmt::Kind::Var: this->lower_var(asg_buffer.getVar(stmt.varID()));
			break; case ASG::Stmt::Kind::FuncCall: this->lower_func_call(asg_buffer.getFuncCall(stmt.funcCallID()));
		}
	}


	auto ASGToLLVMIR::lower_var(const ASG::Var& var) -> void {
		const llvmint::Alloca var_alloca = this->builder.createAlloca(
			this->get_type(var.typeID),
			this->stmt_name("{}.alloca", this->current_source->getTokenBuffer()[var.ident].getString())
		);

		this->builder.createStore(var_alloca, this->get_value(var.expr), false);
	}


	auto ASGToLLVMIR::lower_func_call(const ASG::FuncCall& func_call) -> void {
		const FuncInfo& func_info = this->func_infos.find(func_call.target)->second;

		this->builder.createCall(func_info.func, {});
	}



	auto ASGToLLVMIR::get_type(const TypeInfo::ID& type_info_id) const -> llvmint::Type {
		return this->get_type(this->context.getTypeManager().getTypeInfo(type_info_id));
	}

	auto ASGToLLVMIR::get_type(const TypeInfo& type_info) const -> llvmint::Type {
		const evo::ArrayProxy<TypeInfo::Qualifier> type_qualifiers = type_info.qualifiers();
		if(type_qualifiers.empty() == false){
			if(type_qualifiers.back().has(TypeInfo::QualifierFlag::Ptr)){
				return static_cast<llvmint::Type>(this->builder.getTypePtr());
			}else{
				evo::fatalBreak("Optional is unsupported");	
			}
		}

		switch(type_info.baseTypeID().kind()){
			case BaseType::Kind::Builtin: {
				const BaseType::Builtin::ID builtin_id = type_info.baseTypeID().id<BaseType::Builtin::ID>();
				const BaseType::Builtin& builtin = this->context.getTypeManager().getBuiltin(builtin_id);

				// TODO: select correct type based on target platform / architecture
				switch(builtin.kind()){
					case Token::Kind::TypeInt: return static_cast<llvmint::Type>(this->builder.getTypeI64());
					case Token::Kind::TypeISize: return static_cast<llvmint::Type>(this->builder.getTypeI64()); 
					case Token::Kind::TypeI_N: {
						return static_cast<llvmint::Type>(this->builder.getTypeI_N(evo::uint(builtin.bitWidth())));
					} break;
					case Token::Kind::TypeUInt: return static_cast<llvmint::Type>(this->builder.getTypeI64()); 
					case Token::Kind::TypeUSize: return static_cast<llvmint::Type>(this->builder.getTypeI64()); 
					case Token::Kind::TypeUI_N: {
						return static_cast<llvmint::Type>(this->builder.getTypeI_N(evo::uint(builtin.bitWidth())));
					} break;
					case Token::Kind::TypeF16: return static_cast<llvmint::Type>(this->builder.getTypeF16()); 
					case Token::Kind::TypeBF16: return static_cast<llvmint::Type>(this->builder.getTypeBF16()); 
					case Token::Kind::TypeF32: return static_cast<llvmint::Type>(this->builder.getTypeF32()); 
					case Token::Kind::TypeF64: return static_cast<llvmint::Type>(this->builder.getTypeF64()); 
					case Token::Kind::TypeF128: return static_cast<llvmint::Type>(this->builder.getTypeF128());
					case Token::Kind::TypeByte: return static_cast<llvmint::Type>(this->builder.getTypeI8());
					case Token::Kind::TypeBool: return static_cast<llvmint::Type>(this->builder.getTypeBool()); 
					case Token::Kind::TypeChar: return static_cast<llvmint::Type>(this->builder.getTypeI8());
					case Token::Kind::TypeRawPtr: return static_cast<llvmint::Type>(this->builder.getTypePtr());

					case Token::Kind::TypeCShort: return static_cast<llvmint::Type>(this->builder.getTypeI16()); 
					case Token::Kind::TypeCUShort: return static_cast<llvmint::Type>(this->builder.getTypeI16()); 
					case Token::Kind::TypeCInt: return static_cast<llvmint::Type>(this->builder.getTypeI32()); 
					case Token::Kind::TypeCUInt: return static_cast<llvmint::Type>(this->builder.getTypeI32()); 
					case Token::Kind::TypeCLong: return static_cast<llvmint::Type>(this->builder.getTypeI32()); 
					case Token::Kind::TypeCULong: return static_cast<llvmint::Type>(this->builder.getTypeI32()); 
					case Token::Kind::TypeCLongLong: return static_cast<llvmint::Type>(this->builder.getTypeI64()); 
					case Token::Kind::TypeCULongLong: return static_cast<llvmint::Type>(this->builder.getTypeI64()); 
					case Token::Kind::TypeCLongDouble: return static_cast<llvmint::Type>(this->builder.getTypeF64());

					default: evo::debugFatalBreak(
						"Unknown or unsupported builtin-type: {}", evo::to_underlying(builtin.kind())
					);
				}
			} break;

			case BaseType::Kind::Function: {
				evo::fatalBreak("Function types are unsupported");
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported builtin kind");
	}


	auto ASGToLLVMIR::get_value(const ASG::Expr& expr, bool get_pointer_to_value) const -> llvmint::Value {
		evo::Assert(get_pointer_to_value == false, "getting pointer to value is currently unsupported");

		switch(expr.kind()){
			case ASG::Expr::Kind::LiteralInt: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralInt& literal_int = asg_buffer.getLiteralInt(expr.literalIntID());

				const llvmint::Type literal_type = this->get_type(*literal_int.typeID);
				const auto integer_type = llvmint::IntegerType((llvm::IntegerType*)literal_type.native());
				return static_cast<llvmint::Value>(this->builder.getValueIntegral(integer_type, literal_int.value));
			} break;

			case ASG::Expr::Kind::LiteralFloat: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralFloat& literal_float = asg_buffer.getLiteralFloat(expr.literalFloatID());

				const llvmint::Type literal_type = this->get_type(*literal_float.typeID);
				return static_cast<llvmint::Value>(this->builder.getValueFloat(literal_type, literal_float.value));
			} break;

			case ASG::Expr::Kind::LiteralBool: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const bool bool_value = asg_buffer.getLiteralBool(expr.literalBoolID()).value;

				return static_cast<llvmint::Value>(this->builder.getValueBool(bool_value));
			} break;

			case ASG::Expr::Kind::LiteralChar: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const char char_value = asg_buffer.getLiteralChar(expr.literalCharID()).value;

				return static_cast<llvmint::Value>(this->builder.getValueI8(uint8_t(char_value)));
			} break;

			case ASG::Expr::Kind::Var: {
				evo::fatalBreak("ASG::Expr::Kind::Var is unsupported");
			} break;

			case ASG::Expr::Kind::Func: {
				evo::fatalBreak("ASG::Expr::Kind::Func is unsupported");
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported expr kind");
	}


	auto ASGToLLVMIR::mangle_name(const ASG::Func& func) const -> std::string {
		return std::format(
			"PTHR.{}{}.{}",
			this->current_source->getID().get(),
			this->submangle_parent(func.parent),
			this->get_func_ident_name(func)
		);
	}


	auto ASGToLLVMIR::submangle_parent(const ASG::Parent& parent) const -> std::string {
		return parent.visit([&](auto parent_id) -> std::string {
			using ParentID = std::decay_t<decltype(parent_id)>;

			if constexpr(std::is_same_v<ParentID, std::monostate>){
				return "";
			}else if constexpr(std::is_same_v<ParentID, ASG::Func::ID>){
				const ASG::Func& parent_func = this->current_source->getASGBuffer().getFunc(parent_id);
				return std::format(
					"{}.{}",
					this->submangle_parent(parent_func.parent),
					this->get_func_ident_name(parent_func)
				);
			}
		});
	}


	auto ASGToLLVMIR::get_func_ident_name(const ASG::Func& func) const -> std::string {
		const Token::ID func_ident_token_id = this->current_source->getASTBuffer().getIdent(func.name);
		auto name = std::string(this->current_source->getTokenBuffer()[func_ident_token_id].getString());

		if(func.instanceID.has_value()){
			name += std::format("-i{}", func.instanceID.get());
		}

		return name;
	}



	template<class... Args>
	auto ASGToLLVMIR::stmt_name(std::format_string<Args...> fmt, Args&&... args) const -> std::string {
		if(this->config.optimize){
			return "";
		}else{
			return std::format(fmt, std::forward<Args...>(args)...);
		}
	}
	
}