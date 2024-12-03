////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./ASGToPIR.h"

#include "../include/Context.h"
#include "../include/Source.h"
#include "../include/SourceManager.h"
#include "../include/get_source_location.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{

	
	auto ASGToPIR::lower() -> void {
		// lower variables
		for(const Source::ID& source_id : this->context.getSourceManager()){
			this->current_source = &this->context.getSourceManager()[source_id];

			for(const GlobalScope::Var& global_var : this->current_source->getGlobalScope().getVars()){
				this->lower_global_var(global_var.asg_var);
			}
		}

		// lower func decls
		for(const Source::ID& source_id : this->context.getSourceManager()){
			this->current_source = &this->context.getSourceManager()[source_id];

			for(const ASG::Func::ID func_id : this->current_source->getASGBuffer().getFuncs()){
				this->lower_func_decl(func_id);
			}
		}

		// lower func bodies
		for(const Source::ID& source_id : this->context.getSourceManager()){
			this->current_source = &this->context.getSourceManager()[source_id];

			for(const ASG::Func::ID func_id : this->current_source->getASGBuffer().getFuncs()){
				this->lower_func_body(func_id);
			}
		}

		this->current_source = nullptr;
	}


	auto ASGToPIR::addRuntime() -> void {
		const FuncInfo& entry_func_info = this->get_func_info(*this->context.getEntry());

		// TODO: make return int
		// const pir::Type int_type = this->module.createSignedType(
		// 	this->context.getConfig().os == core::OS::Windows ? 32 : 64
		// );

		const pir::Function::ID created_func_id = module.createFunction(
			"main", {}, pir::CallingConvention::Fast, pir::Linkage::Internal, this->module.createUnsignedType(8)
		);

		this->agent.setTargetFunction(created_func_id);

		const pir::BasicBlock::ID begin_block = this->agent.createBasicBlock("begin");
		this->agent.setTargetBasicBlock(begin_block);

		const pir::Expr call_to_entry = this->agent.createCall(entry_func_info.func, {}, "");
		this->agent.createRet(call_to_entry);
	}



	auto ASGToPIR::lower_global_var(const ASG::Var::ID& var_id) -> void {
		const ASG::Var& asg_var = this->current_source->getASGBuffer().getVar(var_id);
		if(asg_var.kind == AST::VarDecl::Kind::Def){ return; } // make sure not to emit def variables

		const pir::Type var_type = this->get_type(*asg_var.typeID);

		const pir::GlobalVar::ID global_var = this->module.createGlobalVar(
			this->mangle_name(asg_var),
			var_type,
			pir::Linkage::Private,
			this->get_global_var_value(var_type, asg_var.expr),
			asg_var.isConst
		);

		this->var_infos.emplace(ASG::Var::LinkID(this->current_source->getID(), var_id), VarInfo(global_var));
	}



	auto ASGToPIR::lower_func_decl(const ASG::Func::ID& func_id) -> void {
		const ASG::Func& func = this->current_source->getASGBuffer().getFunc(func_id);
		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.baseTypeID.funcID());

		auto params = evo::SmallVector<pir::Parameter>();
		params.reserve(func_type.params.size());
		for(const BaseType::Function::Param& param : func_type.params){
			std::string param_name = [&](){
				if(param.ident.is<Token::ID>()){
					return std::string(this->current_source->getTokenBuffer()[param.ident.as<Token::ID>()].getString());
				}else{
					return std::string(strings::toStringView(param.ident.as<strings::StringCode>()));
				}
			}();

			if(param.optimizeWithCopy){
				params.emplace_back(std::move(param_name), this->get_type(param.typeID));
			}else{
				params.emplace_back(std::move(param_name), this->module.createPtrType());
			}
		}

		if(func_type.hasNamedReturns()){
			for(const BaseType::Function::ReturnParam& return_param : func_type.returnParams){
				params.emplace_back(
					std::format("RET.{}", this->current_source->getTokenBuffer()[*return_param.ident].getString()),
					this->module.createPtrType()
				);
			}
		}

		const pir::Type return_type = [&](){
			if(func_type.hasNamedReturns()){
				return this->module.createVoidType();
			}else{
				return this->get_type(func_type.returnParams.front().typeID);
			}
		}();

		std::string mangled_name = this->mangle_name(func);

		const pir::Function::ID created_func_id = module.createFunction(
			std::string(mangled_name),
			std::move(params),
			pir::CallingConvention::Fast,
			pir::Linkage::Private,
			return_type
		);

		const auto asg_func_link_id = ASG::Func::LinkID(this->current_source->getID(), func_id);
		this->func_infos.emplace(asg_func_link_id, FuncInfo(created_func_id, std::move(mangled_name)));

		this->agent.setTargetFunction(created_func_id);


		if(func_type.hasNamedReturns()){
			const unsigned first_return_param_index = unsigned(func_type.params.size());
			unsigned i = first_return_param_index;
			for(const BaseType::Function::ReturnParam& ret_param : func_type.returnParams){
				const unsigned return_i = i - first_return_param_index;

				this->return_param_infos.emplace(
					ASG::ReturnParam::LinkID(asg_func_link_id, func.returnParams[return_i]),
					ReturnParamInfo(
						pir::Agent::createParamExpr(i), this->get_type(ret_param.typeID.typeID())/*, return_i*/
					)
				);

				i += 1;
			}			
		}

		if(func_type.params.empty()){
			this->agent.createBasicBlock("begin");

		}else{
			const pir::BasicBlock::ID setup_block = this->agent.createBasicBlock("setup");
			const pir::BasicBlock::ID begin_block = this->agent.createBasicBlock("begin");

			this->agent.setTargetBasicBlock(setup_block);

			for(uint32_t i = 0; const BaseType::Function::Param& param : func_type.params){
				const std::string param_name = [&](){
					if(param.ident.is<Token::ID>()){
						return std::string(
							this->current_source->getTokenBuffer()[param.ident.as<Token::ID>()].getString()
						);
					}else{
						return std::string(strings::toStringView(param.ident.as<strings::StringCode>()));
					}
				}();

				const pir::Type param_type = this->get_type(param.typeID);
				const pir::Type param_alloca_type = param.optimizeWithCopy ? param_type : this->module.createPtrType();
				const pir::Expr param_alloca = this->agent.createAlloca(
					param_alloca_type, this->stmt_name("{}.alloca", param_name)
				);

				this->param_infos.emplace(
					ASG::Param::LinkID(asg_func_link_id, func.params[i]), ParamInfo(param_alloca, param_type, i)
				);

				this->agent.createStore(param_alloca, this->agent.createParamExpr(i), false, pir::AtomicOrdering::None);

				i += 1;
			}

			this->agent.createBranch(begin_block);
		}
	}


	auto ASGToPIR::lower_func_body(const ASG::Func::ID& func_id) -> void {
		const ASG::Func& asg_func = this->current_source->getASGBuffer().getFunc(func_id);
		this->current_func = &asg_func;


		auto link_id = ASG::Func::LinkID(this->current_source->getID(), func_id);
		this->current_func_link_id = link_id;

		this->agent.setTargetFunction(this->get_current_func_info().func);
		this->agent.setTargetBasicBlockAtEnd();

		for(const ASG::Stmt& stmt : asg_func.stmts){
			this->lower_stmt(stmt);
		}

		if(asg_func.isTerminated == false){
			const BaseType::Function& func_type = 
				this->context.getTypeManager().getFunction(asg_func.baseTypeID.funcID());


			if(func_type.returnsVoid()){
				this->agent.createRet();
			}else{
				this->agent.createUnreachable();
			}
		}

		this->current_func_link_id = std::nullopt;
		this->current_func = nullptr;
	}


	auto ASGToPIR::lower_stmt(const ASG::Stmt& stmt) -> void {
		const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();

		switch(stmt.kind()){
			break; case ASG::Stmt::Kind::Var:         this->lower_var(stmt.varID());
			break; case ASG::Stmt::Kind::FuncCall:    this->lower_func_call(asg_buffer.getFuncCall(stmt.funcCallID()));
			break; case ASG::Stmt::Kind::Assign:      this->lower_assign(asg_buffer.getAssign(stmt.assignID()));
			break; case ASG::Stmt::Kind::MultiAssign:
				this->lower_multi_assign(asg_buffer.getMultiAssign(stmt.multiAssignID()));
			break; case ASG::Stmt::Kind::Return:      this->lower_return(asg_buffer.getReturn(stmt.returnID()));
			break; case ASG::Stmt::Kind::Unreachable: this->agent.createUnreachable();
			break; case ASG::Stmt::Kind::Conditional:
				this->lower_conditional(asg_buffer.getConditional(stmt.conditionalID()));
			break; case ASG::Stmt::Kind::While:       this->lower_while(asg_buffer.getWhile(stmt.whileID()));
		}
	}


	auto ASGToPIR::lower_var(const ASG::Var::ID& var_id) -> void {
		const ASG::Var& asg_var = this->current_source->getASGBuffer().getVar(var_id);
		if(asg_var.kind == AST::VarDecl::Kind::Def){ return; } // make sure not to emit def variables

		const pir::Expr var_alloca = this->agent.createAlloca(
			this->get_type(*asg_var.typeID),
			this->stmt_name("{}.alloca", this->current_source->getTokenBuffer()[asg_var.ident].getString())
		);

		switch(asg_var.expr.kind()){
			case ASG::Expr::Kind::Uninit: {
				// do nothing
			} break;

			case ASG::Expr::Kind::Zeroinit: {
				// TODO: 
				evo::log::fatal("UNIMPLEMENTED (var value of [zeroinit])");
				evo::breakpoint();
				// this->builder.createMemSetInline(
				// 	var_alloca.asValue(),
				// 	this->builder.getValueI8(0).asValue(),
				// 	this->get_value_size(this->context.getTypeManager().sizeOf(*asg_var.typeID)).asValue(),
				// 	false
				// );
			} break;

			default: {
				this->agent.createStore(
					var_alloca, this->get_value<false>(asg_var.expr), false, pir::AtomicOrdering::None
				);
			} break;
		}

		this->var_infos.emplace(ASG::Var::LinkID(this->current_source->getID(), var_id), VarInfo(var_alloca));
	}

	auto ASGToPIR::lower_func_call(const ASG::FuncCall& func_call) -> void {
		// TODO: 
		evo::log::fatal("UNIMPLEMENTED (ASG::FuncCall)");
		evo::breakpoint();
	}

	auto ASGToPIR::lower_assign(const ASG::Assign& assign) -> void {
		const pir::Expr lhs = this->get_value<true>(assign.lhs);
		const pir::Expr rhs = this->get_value<false>(assign.rhs);

		this->agent.createStore(lhs, rhs, false, pir::AtomicOrdering::None);
	}

	auto ASGToPIR::lower_multi_assign(const ASG::MultiAssign& multi_assign) -> void {
		// TODO: 
		evo::log::fatal("UNIMPLEMENTED (ASG::MultiAssign)");
		evo::breakpoint();
	}

	auto ASGToPIR::lower_return(const ASG::Return& return_stmt) -> void {
		if(return_stmt.value.has_value() == false){
			this->agent.createRet();
			return;
		}

		this->agent.createRet(this->get_value<false>(*return_stmt.value));
	}

	auto ASGToPIR::lower_conditional(const ASG::Conditional& conditional_stmt) -> void {
		// TODO: 
		evo::log::fatal("UNIMPLEMENTED (ASG::Conditional)");
		evo::breakpoint();
	}

	auto ASGToPIR::lower_while(const ASG::While& while_loop) -> void {
		// TODO: 
		evo::log::fatal("UNIMPLEMENTED (ASG::While)");
		evo::breakpoint();
	}



	auto ASGToPIR::get_type(const TypeInfo::VoidableID& type_info_voidable_id) const -> pir::Type {
		if(type_info_voidable_id.isVoid()) [[unlikely]] {
			return this->module.createVoidType();
		}else{
			return this->get_type(this->context.getTypeManager().getTypeInfo(type_info_voidable_id.typeID()));
		}
	}

	auto ASGToPIR::get_type(const TypeInfo::ID& type_info_id) const -> pir::Type {
		return this->get_type(this->context.getTypeManager().getTypeInfo(type_info_id));
	}

	auto ASGToPIR::get_type(const TypeInfo& type_info) const -> pir::Type {
		const evo::ArrayProxy<AST::Type::Qualifier> type_qualifiers = type_info.qualifiers();
		if(type_qualifiers.empty() == false){
			if(type_qualifiers.back().isPtr){
				return this->module.createPtrType();
			}else{
				evo::fatalBreak("Optional is unsupported");	
			}
		}

		switch(type_info.baseTypeID().kind()){
			case BaseType::Kind::Primitive: {
				const BaseType::Primitive::ID primitive_id = type_info.baseTypeID().primitiveID();
				const BaseType::Primitive& primitive = this->context.getTypeManager().getPrimitive(primitive_id);
				return this->get_type(primitive);
			} break;

			case BaseType::Kind::Function: {
				return this->module.createPtrType();
			} break;

			case BaseType::Kind::Alias: {
				const BaseType::Alias::ID alias_id = type_info.baseTypeID().aliasID();
				const BaseType::Alias& alias = this->context.getTypeManager().getAlias(alias_id);
				return this->get_type(alias.aliasedType);
			} break;

			case BaseType::Kind::Dummy: evo::debugFatalBreak("Cannot get a dummy type");
		}

		evo::debugFatalBreak("Unknown or unsupported primitive kind");
	}


	auto ASGToPIR::get_type(const BaseType::Primitive& primitive) const -> pir::Type {
		switch(primitive.kind()){
			case Token::Kind::TypeInt: {
				return this->module.createSignedType(
					uint32_t(this->context.getTypeManager().sizeOfGeneralRegister() * 8)
				);
			} break;

			case Token::Kind::TypeUInt: {
				return this->module.createUnsignedType(
					uint32_t(this->context.getTypeManager().sizeOfGeneralRegister() * 8)
				);
			} break;

			case Token::Kind::TypeISize: {
				return this->module.createSignedType(uint32_t(this->context.getTypeManager().sizeOfPtr() * 8));
			} break;

			case Token::Kind::TypeUSize:{
				return this->module.createUnsignedType(uint32_t(this->context.getTypeManager().sizeOfPtr() * 8));
			} break;

			case Token::Kind::TypeI_N: {
				return this->module.createSignedType(unsigned(primitive.bitWidth()));
			} break;

			case Token::Kind::TypeUI_N: {
				return this->module.createUnsignedType(unsigned(primitive.bitWidth()));
			} break;

			case Token::Kind::TypeF16: return this->module.createFloatType(16);
			case Token::Kind::TypeBF16: return this->module.createBFloatType();
			case Token::Kind::TypeF32: return this->module.createFloatType(32);
			case Token::Kind::TypeF64: return this->module.createFloatType(64);
			case Token::Kind::TypeF80: return this->module.createFloatType(80);
			case Token::Kind::TypeF128: return this->module.createFloatType(128);
			case Token::Kind::TypeByte: return this->module.createUnsignedType(8);
			case Token::Kind::TypeBool: return this->module.createBoolType();
			case Token::Kind::TypeChar: return this->module.createSignedType(8);
			case Token::Kind::TypeRawPtr: return this->module.createPtrType();
			case Token::Kind::TypeTypeID: return this->module.createUnsignedType(32);

			case Token::Kind::TypeCShort:  return this->module.createSignedType(16);
			case Token::Kind::TypeCUShort: return this->module.createUnsignedType(16);

			case Token::Kind::TypeCInt: {
				if(this->context.getTypeManager().getOS() == core::OS::Windows){
					return this->module.createSignedType(32);
				}else{
					return this->module.createSignedType(64);
				}
			} break;

			case Token::Kind::TypeCUInt: {
				if(this->context.getTypeManager().getOS() == core::OS::Windows){
					return this->module.createUnsignedType(32);
				}else{
					return this->module.createUnsignedType(64);
				}
			} break;

			case Token::Kind::TypeCLong:  return this->module.createSignedType(32);
			case Token::Kind::TypeCULong: return this->module.createUnsignedType(32);
			case Token::Kind::TypeCLongLong: return this->module.createSignedType(64);
			case Token::Kind::TypeCULongLong: return this->module.createUnsignedType(64);

			case Token::Kind::TypeCLongDouble: {
				if(this->context.getTypeManager().getOS() == core::OS::Windows){
					return this->module.createFloatType(64);
				}else{
					return this->module.createFloatType(80);
				}
			} break;

			default: evo::debugFatalBreak(
				"Unknown or unsupported primitive-type: {}", evo::to_underlying(primitive.kind())
			);
		}
	}



	template<bool GET_POINTER_TO_VALUE>
	auto ASGToPIR::get_value(const ASG::Expr& expr) const -> pir::Expr {
		const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();

		switch(expr.kind()){
			case ASG::Expr::Kind::Uninit: {
				evo::debugFatalBreak("Cannot get value of [uninit]");
			} break;

			case ASG::Expr::Kind::Zeroinit: {
				evo::debugFatalBreak("Cannot get value of [zeroinit]");
			} break;

			case ASG::Expr::Kind::LiteralInt: {
				const ASG::LiteralInt& literal_int = asg_buffer.getLiteralInt(expr.literalIntID());

				const pir::Type int_type = this->get_type(*literal_int.typeID);
				const pir::Expr pir_num = this->agent.createNumber(int_type, literal_int.value);

				if constexpr(!GET_POINTER_TO_VALUE){
					return pir_num;
				}else{
					const pir::Expr alloca = this->agent.createAlloca(int_type);
					this->agent.createStore(alloca, pir_num, false, pir::AtomicOrdering::None);
					return alloca;
				}
			} break;

			case ASG::Expr::Kind::LiteralFloat: {
				const ASG::LiteralFloat& literal_float = asg_buffer.getLiteralFloat(expr.literalFloatID());

				const pir::Type float_type = this->get_type(*literal_float.typeID);
				const pir::Expr pir_num = this->agent.createNumber(float_type, literal_float.value);

				if constexpr(!GET_POINTER_TO_VALUE){
					return pir_num;
				}else{
					const pir::Expr alloca = this->agent.createAlloca(float_type);
					this->agent.createStore(alloca, pir_num, false, pir::AtomicOrdering::None);
					return alloca;
				}
			} break;

			case ASG::Expr::Kind::LiteralBool: {
				const ASG::LiteralBool& literal_bool = asg_buffer.getLiteralBool(expr.literalBoolID());

				const pir::Expr pir_bool = this->agent.createBoolean(literal_bool.value);

				if constexpr(!GET_POINTER_TO_VALUE){
					return pir_bool;
				}else{
					const pir::Expr alloca = this->agent.createAlloca(this->module.createBoolType());
					this->agent.createStore(alloca, pir_bool, false, pir::AtomicOrdering::None);
					return alloca;
				}
			} break;

			case ASG::Expr::Kind::LiteralChar: {
				const ASG::LiteralChar& literal_char = asg_buffer.getLiteralChar(expr.literalCharID());

				const pir::Type char_type = this->module.createSignedType(8);
				const pir::Expr pir_char = this->agent.createNumber(
					char_type, core::GenericInt::create<char>(literal_char.value)
				);

				if constexpr(!GET_POINTER_TO_VALUE){
					return pir_char;
				}else{
					const pir::Expr alloca = this->agent.createAlloca(char_type);
					this->agent.createStore(alloca, pir_char, false, pir::AtomicOrdering::None);
					return alloca;
				}
			} break;

			case ASG::Expr::Kind::Intrinsic: {
				evo::debugFatalBreak("Cannot get value of intrinsic function");
			} break;

			case ASG::Expr::Kind::TemplatedIntrinsicInstantiation: {
				evo::debugFatalBreak("Cannot get value of template instantiated intrinsic function");
			} break;

			case ASG::Expr::Kind::Copy: {
				const ASG::Expr& copy_expr = asg_buffer.getCopy(expr.copyID());
				return this->get_value<GET_POINTER_TO_VALUE>(copy_expr);
			} break;

			case ASG::Expr::Kind::Move: {
				const ASG::Expr& move_expr = asg_buffer.getMove(expr.moveID());
				return this->get_value<GET_POINTER_TO_VALUE>(move_expr);
			} break;

			case ASG::Expr::Kind::FuncCall: {
				// TODO: 
				evo::log::fatal("UNIMPLEMENTED (ASG::Expr::Kind::FuncCall)");
				evo::breakpoint();
			} break;

			case ASG::Expr::Kind::AddrOf: {
				const ASG::Expr& addr_of_expr = asg_buffer.getAddrOf(expr.addrOfID());
				const pir::Expr address = this->get_value<true>(addr_of_expr);

				if constexpr(!GET_POINTER_TO_VALUE){
					return address;

				}else{
					const pir::Expr alloca = this->agent.createAlloca(this->module.createPtrType());
					this->agent.createStore(alloca, address, false, pir::AtomicOrdering::None);
					return alloca;
				}

			} break;

			case ASG::Expr::Kind::Deref: {
				const ASG::Deref& deref_expr = asg_buffer.getDeref(expr.derefID());
				const pir::Expr value = this->get_value<false>(deref_expr.expr);


				if constexpr(GET_POINTER_TO_VALUE){
					return value;

				}else{
					return this->agent.createLoad(
						value,
						this->get_type(deref_expr.typeID),
						false,
						pir::AtomicOrdering::None,
						this->stmt_name("DEREF")
					);
				}
			} break;

			case ASG::Expr::Kind::Var: {
				const VarInfo& var_info = this->get_var_info(expr.varLinkID());

				if constexpr(GET_POINTER_TO_VALUE){
					return var_info.value.visit([&](auto& value) -> pir::Expr {
						using ValueT = std::decay_t<decltype(value)>;

						if constexpr(std::is_same<ValueT, pir::Expr>()){
							return value;

						}else if constexpr(std::is_same<ValueT, pir::GlobalVar::ID>()){
							return this->agent.createGlobalValue(value);

						}else{
							static_assert(false, "Unknown or unsupported var kind");
						}
					});

				}else{
					return var_info.value.visit([&](auto& value) -> pir::Expr {
						using ValueT = std::decay_t<decltype(value)>;

						if constexpr(std::is_same<ValueT, pir::Expr>()){
							return this->agent.createLoad(
								value,
								this->agent.getExprType(value),
								false,
								pir::AtomicOrdering::None,
								this->stmt_name("VAR.load")
							);

						}else if constexpr(std::is_same<ValueT, pir::GlobalVar::ID>()){
							const pir::Expr global_value = this->agent.createGlobalValue(value);
							return this->agent.createLoad(
								global_value,
								this->agent.getExprType(global_value),
								false,
								pir::AtomicOrdering::None,
								this->stmt_name("GLOBAL.load")
							);

						}else{
							static_assert(false, "Unknown or unsupported var kind");
						}
					});
				}
			} break;

			case ASG::Expr::Kind::Func: {
				// TODO: 
				evo::log::fatal("UNIMPLEMENTED (ASG::Expr::Kind::Func)");
				evo::breakpoint();
			} break;

			case ASG::Expr::Kind::Param: {
				const ParamInfo& param_info = this->get_param_info(expr.paramLinkID());

				const BaseType::Function& func_type = this->context.getTypeManager().getFunction(
					this->current_func->baseTypeID.funcID()
				);

				const bool optimize_with_copy = func_type.params[param_info.index].optimizeWithCopy;

				if(optimize_with_copy){
					if constexpr(GET_POINTER_TO_VALUE){
						return param_info.alloca;
					}else{
						return this->agent.createLoad(
							param_info.alloca,
							param_info.type,
							false,
							pir::AtomicOrdering::None,
							this->stmt_name("PARAM.load")
						);
					}

				}else{
					const pir::Expr load = this->agent.createLoad(
						param_info.alloca,
						param_info.type,
						false,
						pir::AtomicOrdering::None,
						this->stmt_name("PARAM.ptr_lookup")
					);

					if constexpr(GET_POINTER_TO_VALUE){
						return load;
					}else{
						return this->agent.createLoad(
							load, param_info.type, false, pir::AtomicOrdering::None, this->stmt_name("PARAM.load")
						);
					}
				}
			} break;

			case ASG::Expr::Kind::ReturnParam: {
				const ReturnParamInfo& ret_param_info = this->get_return_param_info(expr.returnParamLinkID());

				if constexpr(GET_POINTER_TO_VALUE){
					return ret_param_info.param;
				}else{
					return this->agent.createLoad(
						ret_param_info.param,
						ret_param_info.type,
						false,
						pir::AtomicOrdering::None,
						this->stmt_name("RET_PARAM.load")
					);
				}
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported expr kind");
	}


	auto ASGToPIR::get_global_var_value(const pir::Type& type, const ASG::Expr& expr) const -> pir::GlobalVar::Value {
		switch(expr.kind()){
			case ASG::Expr::Kind::Uninit: return pir::GlobalVar::Uninit();
			case ASG::Expr::Kind::Zeroinit: return pir::GlobalVar::Zeroinit();

			case ASG::Expr::Kind::LiteralInt: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralInt& literal_int = asg_buffer.getLiteralInt(expr.literalIntID());

				return this->agent.createNumber(type, literal_int.value);
			} break;

			case ASG::Expr::Kind::LiteralFloat: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralFloat& literal_float = asg_buffer.getLiteralFloat(expr.literalFloatID());

				return this->agent.createNumber(type, literal_float.value);
			} break;

			case ASG::Expr::Kind::LiteralBool: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralBool& literal_bool = asg_buffer.getLiteralBool(expr.literalBoolID());

				return this->agent.createBoolean(literal_bool.value);
			} break;

			case ASG::Expr::Kind::LiteralChar: {
				const ASGBuffer& asg_buffer = this->current_source->getASGBuffer();
				const ASG::LiteralChar& literal_char = asg_buffer.getLiteralChar(expr.literalCharID());

				return this->agent.createNumber(
					this->module.createSignedType(8), core::GenericInt::create<char>(literal_char.value)
				);
			} break;

			case ASG::Expr::Kind::Intrinsic:     case ASG::Expr::Kind::TemplatedIntrinsicInstantiation:
			case ASG::Expr::Kind::Copy:          case ASG::Expr::Kind::Move:
			case ASG::Expr::Kind::FuncCall:      case ASG::Expr::Kind::AddrOf:
			case ASG::Expr::Kind::Deref:         case ASG::Expr::Kind::Var:
			case ASG::Expr::Kind::Func:          case ASG::Expr::Kind::Param:
			case ASG::Expr::Kind::ReturnParam: {
				evo::debugFatalBreak("Invalid global var value");
			}
		}

		evo::debugFatalBreak("Unknown or unsupported expr kind");
	}





	auto ASGToPIR::mangle_name(const ASG::Func& func) const -> std::string {
		return std::format(
			"PTHR.{}{}.{}",
			this->current_source->getID().get(),
			this->submangle_parent(func.parent),
			this->get_func_ident_name(func)
		);
	}


	auto ASGToPIR::mangle_name(const ASG::Var& var) const -> std::string {
		const std::string_view var_ident = this->current_source->getTokenBuffer()[var.ident].getString();

		return std::format("PTHR.{}.{}", this->current_source->getID().get(), var_ident);
	}


	auto ASGToPIR::submangle_parent(const ASG::Parent& parent) const -> std::string {
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


	auto ASGToPIR::get_func_ident_name(const ASG::Func& func) const -> std::string {
		const Token::ID func_ident_token_id = this->current_source->getASTBuffer().getIdent(func.name);
		auto name = std::string(this->current_source->getTokenBuffer()[func_ident_token_id].getString());

		if(func.instanceID.has_value()){
			name += std::format("-i{}", func.instanceID.get());
		}

		return name;
	}


	auto ASGToPIR::stmt_name(std::string_view str) const -> std::string {
		if(this->config.useReadableRegisters) [[unlikely]] {
			return std::string(str);
		}else{
			return std::string();
		}
	}


	template<class... Args>
	auto ASGToPIR::stmt_name(std::format_string<Args...> fmt, Args&&... args) const -> std::string {
		if(this->config.useReadableRegisters) [[unlikely]] {
			return std::format(fmt, std::forward<Args...>(args)...);
		}else{
			return std::string();
		}
	}



	auto ASGToPIR::get_func_info(ASG::Func::LinkID link_id) const -> const FuncInfo& {
		const auto& info_find = this->func_infos.find(link_id);
		evo::debugAssert(info_find != this->func_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}

	auto ASGToPIR::get_current_func_info() const -> const FuncInfo& {
		evo::debugAssert(this->current_func_link_id.has_value(), "current_func_link_id is not set");
		return this->get_func_info(*this->current_func_link_id);
	}


	auto ASGToPIR::get_var_info(ASG::Var::LinkID link_id) const -> const VarInfo& {
		const auto& info_find = this->var_infos.find(link_id);
		evo::debugAssert(info_find != this->var_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}

	auto ASGToPIR::get_param_info(ASG::Param::LinkID link_id) const -> const ParamInfo& {
		const auto& info_find = this->param_infos.find(link_id);
		evo::debugAssert(info_find != this->param_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}

	auto ASGToPIR::get_return_param_info(ASG::ReturnParam::LinkID link_id) const -> const ReturnParamInfo& {
		const auto& info_find = this->return_param_infos.find(link_id);
		evo::debugAssert(info_find != this->return_param_infos.end(), "doesn't have info for that link id");
		return info_find->second;
	}


	
}