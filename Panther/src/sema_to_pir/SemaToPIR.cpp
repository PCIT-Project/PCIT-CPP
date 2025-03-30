////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SemaToPIR.h"

#include "../../include/Context.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pcit::panther{
	

	auto SemaToPIR::lower() -> void {
		for(uint32_t i = 0; i < this->context.getTypeManager().structs.size(); i+=1){
			this->lowerStruct(BaseType::Struct::ID(i));
		}

		for(const sema::GlobalVar::ID& global_var_id : this->context.getSemaBuffer().getGlobalVars()){
			this->lowerGlobal(global_var_id);
		}

		for(const sema::Func::ID& func_id : this->context.getSemaBuffer().getFuncs()){
			this->lowerFuncDecl(func_id);
		}
		for(const sema::Func::ID& func_id : this->context.getSemaBuffer().getFuncs()){
			this->lowerFuncDef(func_id);
		}
	}



	auto SemaToPIR::lowerStruct(const BaseType::Struct::ID struct_id) -> void {
		// const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(struct_id);

		const pir::Type new_type = this->module.createStructType(
			this->mangle_name(struct_id),
			evo::SmallVector<pir::Type>(),
			false
		);

		this->data.create_struct(struct_id, new_type);
	}



	auto SemaToPIR::lowerGlobal(const sema::GlobalVar::ID global_var_id) -> void {
		const sema::GlobalVar& global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);

		if(global_var.kind == AST::VarDecl::Kind::Def){ return; }

		const pir::GlobalVar::ID new_global_var = this->module.createGlobalVar(
			this->mangle_name(global_var_id),
			this->get_type(*global_var.typeID),
			pir::Linkage::PRIVATE,
			this->get_global_var_value(*global_var.expr.load()),
			global_var.kind == AST::VarDecl::Kind::Const
		);

		this->data.create_global_var(global_var_id, new_global_var);
	}


	auto SemaToPIR::lowerFuncDecl(const sema::Func::ID func_id) -> pir::Function::ID {
		const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);
		this->current_source = &this->context.getSourceManager()[func.sourceID];
		EVO_DEFER([&](){ this->current_source = nullptr; });

		if(func.hasInParam){ evo::unimplemented("functions with `in` parameter"); }

		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.typeID);


		auto params = evo::SmallVector<pir::Parameter>();
		params.reserve(func_type.params.size() + func_type.returnParams.size() + func_type.errorParams.size());

		auto arg_is_copy = evo::SmallVector<bool>();
		arg_is_copy.reserve(func_type.params.size());

		bool no_params_need_alloca = true;
		for(size_t i = 0; const BaseType::Function::Param& param : func_type.params){
			EVO_DEFER([&](){ i += 1; });

			std::string param_name = [&](){
				if(this->data.getConfig().useReadableNames == false){
					return std::format(".{}", params.size());
				}else{
					return std::string(this->current_source->getTokenBuffer()[func.params[i].ident].getString());
				}
			}();

			if(param.shouldCopy){
				params.emplace_back(std::move(param_name), this->get_type(param.typeID));

				if(param.kind == AST::FuncDecl::Param::Kind::In){
					no_params_need_alloca = false;
				}
			}else{
				params.emplace_back(std::move(param_name), this->module.createPtrType());
			}

			arg_is_copy.emplace_back(param.shouldCopy);
		}

		auto return_params = evo::SmallVector<pir::Expr>();
		if(func_type.hasNamedReturns()){
			for(const BaseType::Function::ReturnParam& return_param : func_type.returnParams){
				return_params.emplace_back(this->agent.createParamExpr(uint32_t(params.size())));

				if(this->data.getConfig().useReadableNames){
					params.emplace_back(
						std::format("RET.{}", this->current_source->getTokenBuffer()[*return_param.ident].getString()),
						this->module.createPtrType()
					);
				}else{
					params.emplace_back(std::format(".{}", params.size()), this->module.createPtrType());
				}
			}

		}else if(func_type.hasErrorReturn() && func_type.returnsVoid() == false){
			return_params.emplace_back(this->agent.createParamExpr(uint32_t(params.size())));

			if(this->data.getConfig().useReadableNames){
				params.emplace_back("RET", this->module.createPtrType());
			}else{
				params.emplace_back(std::format(".{}", params.size()), this->module.createPtrType());
			}
		}

		auto error_return_params = evo::SmallVector<pir::Expr>();
		if(func_type.hasErrorReturnParams()){
			if(func_type.hasNamedErrorReturns()){
				for(const BaseType::Function::ReturnParam& error_param : func_type.errorParams){
					error_return_params.emplace_back(this->agent.createParamExpr(uint32_t(params.size())));

					if(this->data.getConfig().useReadableNames){
						params.emplace_back(
							std::format(
								"ERR.{}", this->current_source->getTokenBuffer()[*error_param.ident].getString()
							),
							this->module.createPtrType()
						);

					}else{
						params.emplace_back(std::format(".{}", params.size()), this->module.createPtrType());
					}
				}
				
			}else{
				error_return_params.emplace_back(this->agent.createParamExpr(uint32_t(params.size())));

				if(this->data.getConfig().useReadableNames){
					params.emplace_back("ERR", this->module.createPtrType());
				}else{
					params.emplace_back(std::format(".{}", params.size()), this->module.createPtrType());
				}
			}
		}

		const pir::Type return_type = [&](){
			if(func_type.hasErrorReturn()){
				return this->module.createBoolType();

			}else if(func_type.hasNamedReturns()){
				return this->module.createVoidType();

			}else{
				return this->get_type(func_type.returnParams.front().typeID);
			}
		}();

		const pir::Function::ID new_func_id = module.createFunction(
			this->mangle_name(func_id),
			std::move(params),
			this->data.getConfig().isJIT ? pir::CallingConvention::C : pir::CallingConvention::FAST,
			this->data.getConfig().isJIT ? pir::Linkage::EXTERNAL : pir::Linkage::PRIVATE,
			return_type
		);

		this->agent.setTargetFunction(new_func_id);

		this->data.create_func(
			func_id,
			new_func_id, // first arg of FuncInfo construction
			return_type,
			std::move(arg_is_copy),
			std::move(return_params),
			std::move(error_return_params)
		);


		if(func_type.params.empty() | no_params_need_alloca){
			this->agent.createBasicBlock(this->name("begin"));

		}else{
			const pir::BasicBlock::ID setup_block = this->agent.createBasicBlock(this->name("setup"));
			const pir::BasicBlock::ID begin_block = this->agent.createBasicBlock(this->name("begin"));

			this->agent.setTargetBasicBlock(setup_block);

			for(uint32_t i = 0; const BaseType::Function::Param& param : func_type.params){
				if(param.shouldCopy == false){ continue; }

				const pir::Type param_type = this->get_type(param.typeID);
				const pir::Expr param_alloca = this->agent.createAlloca(
					param_type,
					this->name("{}.alloca", this->current_source->getTokenBuffer()[func.params[i].ident].getString())
				);

				// this->param_infos.emplace(
				// 	ASG::Param::LinkID(asg_func_link_id, func.params[i]), ParamInfo(param_alloca, param_type, i)
				// );

				this->agent.createStore(param_alloca, this->agent.createParamExpr(i), false, pir::AtomicOrdering::NONE);

				i += 1;
			}

			this->agent.createBranch(begin_block);
		}


		return new_func_id;
	}



	auto SemaToPIR::lowerFuncDef(const sema::Func::ID func_id) -> void {
		const sema::Func& sema_func = this->context.getSemaBuffer().getFunc(func_id);
		this->current_source = &this->context.getSourceManager()[sema_func.sourceID];
		EVO_DEFER([&](){ this->current_source = nullptr; });

		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(sema_func.typeID);

		pir::Function& func = this->module.getFunction(this->data.get_func(func_id).pir_id);

		this->agent.setTargetFunction(func);
		this->agent.setTargetBasicBlockAtEnd();

		for(const sema::Stmt& stmt : sema_func.stmtBlock){
			// TODO: move into separate function?
			switch(stmt.kind()){
				case sema::Stmt::Kind::GLOBAL_VAR: {
					evo::unimplemented("To PIR of sema::Stmt::Kind::GLOBAL_VAR");
				} break;

				case sema::Stmt::Kind::FUNC_CALL: {
					evo::unimplemented("To PIR of sema::Stmt::Kind::FUNC_CALL");
				} break;

				case sema::Stmt::Kind::ASSIGN: {
					evo::unimplemented("To PIR of sema::Stmt::Kind::ASSIGN");
				} break;

				case sema::Stmt::Kind::MULTI_ASSIGN: {
					evo::unimplemented("To PIR of sema::Stmt::Kind::MULTI_ASSIGN");
				} break;

				case sema::Stmt::Kind::RETURN: {
					const sema::Return& return_stmt = this->context.getSemaBuffer().getReturn(stmt.returnID());

					if(func_type.hasErrorReturn()){
						if(return_stmt.value.has_value()){
							this->agent.createStore(
								this->data.get_func(func_id).return_params.front(),
								this->get_expr_register(*return_stmt.value),
								false,
								pir::AtomicOrdering::NONE
							);
						}

						this->agent.createRet(this->agent.createBoolean(true));

					}else{
						if(return_stmt.value.has_value()){
							this->agent.createRet(this->get_expr_register(*return_stmt.value));

						}else{
							this->agent.createRet();
						}
					}
				} break;

				case sema::Stmt::Kind::ERROR: {
					const sema::Error& error_stmt = this->context.getSemaBuffer().getError(stmt.errorID());

					if(error_stmt.value.has_value()){
						this->agent.createStore(
							this->data.get_func(func_id).error_return_params.front(),
							this->get_expr_register(*error_stmt.value),
							false,
							pir::AtomicOrdering::NONE
						);
						this->agent.createRet(this->agent.createBoolean(false));

					}else{
						this->agent.createRet(this->agent.createBoolean(false));
					}
					
				} break;

				case sema::Stmt::Kind::UNREACHABLE: {
					evo::unimplemented("To PIR of sema::Stmt::Kind::UNREACHABLE");
				} break;

				case sema::Stmt::Kind::CONDITIONAL: {
					evo::unimplemented("To PIR of sema::Stmt::Kind::CONDITIONAL");
				} break;

				case sema::Stmt::Kind::WHILE: {
					evo::unimplemented("To PIR of sema::Stmt::Kind::WHILE");
				} break;
			}
		}


		if(sema_func.isTerminated == false){
			if(func_type.returnsVoid()){
				if(func_type.hasErrorReturn()){
					this->agent.createRet(this->agent.createBoolean(true));
				}else{
					this->agent.createRet();
				}
				
			}else{
				this->agent.createUnreachable();
			}
		}
	}




	//////////////////////////////////////////////////////////////////////
	// get expr


	auto SemaToPIR::get_expr_register(const sema::Expr expr) -> pir::Expr {
		return *this->get_expr_impl<GetExprMode::REGISTER>(expr, nullptr);
	}

	auto SemaToPIR::get_expr_pointer(const sema::Expr expr) -> pir::Expr {
		return *this->get_expr_impl<GetExprMode::POINTER>(expr, nullptr);
	}

	auto SemaToPIR::get_expr_store(const sema::Expr expr, evo::ArrayProxy<pir::Expr> store_locations) -> void {
		this->get_expr_impl<GetExprMode::STORE>(expr, store_locations);
	}



	template<SemaToPIR::GetExprMode MODE>
	auto SemaToPIR::get_expr_impl(const sema::Expr expr, evo::ArrayProxy<pir::Expr> store_locations)
	-> std::optional<pir::Expr> {
		if constexpr(MODE == GetExprMode::STORE){
			evo::debugAssert(!store_locations.empty(), "Must have store location(s) if `MODE == GetExprMode::STORE`");

		}else{
			evo::debugAssert(store_locations.empty(), "Cannot have store location(s) if `MODE != GetExprMode::STORE`");
		}

		switch(expr.kind()){
			case sema::Expr::Kind::NONE: {
				evo::debugFatalBreak("Not a valid sema::Expr");
			} break;

			case sema::Expr::Kind::MODULE_IDENT: {
				evo::unimplemented("lower sema::Expr::Kind::ModuleIdent");
			} break;

			case sema::Expr::Kind::UNINIT: {
				evo::unimplemented("lower sema::Expr::Kind::Uninit");
			} break;

			case sema::Expr::Kind::ZEROINIT: {
				evo::unimplemented("lower sema::Expr::Kind::Zeroinit");
			} break;

			case sema::Expr::Kind::INT_VALUE: {
				const sema::IntValue& int_value = this->context.getSemaBuffer().getIntValue(expr.intValueID());
				const pir::Type value_type = this->get_type(*int_value.typeID);
				const pir::Expr number = this->agent.createNumber(value_type, int_value.value);

				if constexpr(MODE == GetExprMode::REGISTER){
					return number;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(value_type, this->name("NUMBER.ALLOCA"));
					this->agent.createStore(alloca, number, false, pir::AtomicOrdering::NONE);
					return alloca;

				}else{
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], number, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::FLOAT_VALUE: {
				const sema::FloatValue& float_value = this->context.getSemaBuffer().getFloatValue(expr.floatValueID());
				const pir::Type value_type = this->get_type(*float_value.typeID);
				const pir::Expr number = this->agent.createNumber(value_type, float_value.value);

				if constexpr(MODE == GetExprMode::REGISTER){
					return number;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(value_type, this->name("NUMBER.ALLOCA"));
					this->agent.createStore(alloca, number, false, pir::AtomicOrdering::NONE);
					return alloca;

				}else{
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], number, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::BOOL_VALUE: {
				const sema::BoolValue& bool_value = this->context.getSemaBuffer().getBoolValue(expr.boolValueID());
				const pir::Expr boolean = this->agent.createBoolean(bool_value.value);

				if constexpr(MODE == GetExprMode::REGISTER){
					return boolean;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(
						this->module.createBoolType(), this->name("BOOLEAN.ALLOCA")
					);
					this->agent.createStore(alloca, boolean, false, pir::AtomicOrdering::NONE);
					return alloca;

				}else{
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], boolean, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::STRING_VALUE: {
				evo::unimplemented("lower sema::Expr::Kind::StringValue");
			} break;

			case sema::Expr::Kind::CHAR_VALUE: {
				const sema::CharValue& char_value = this->context.getSemaBuffer().getCharValue(expr.charValueID());
				const pir::Type value_type = this->module.createIntegerType(8);
				const pir::Expr number = this->agent.createNumber(
					value_type, core::GenericInt(8, uint64_t(char_value.value))
				);

				if constexpr(MODE == GetExprMode::REGISTER){
					return number;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(value_type, this->name("NUMBER.ALLOCA"));
					this->agent.createStore(alloca, number, false, pir::AtomicOrdering::NONE);
					return alloca;

				}else{
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations[0], number, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::INTRINSIC: {
				evo::unimplemented("lower sema::Expr::Kind::Intrinsic");
			} break;

			case sema::Expr::Kind::TEMPLATED_INTRINSIC_INSTANTIATION: {
				evo::unimplemented("lower sema::Expr::Kind::TemplatedIntrinsicInstantiation");
			} break;

			case sema::Expr::Kind::COPY: {
				evo::unimplemented("lower sema::Expr::Kind::Copy");
			} break;

			case sema::Expr::Kind::MOVE: {
				evo::unimplemented("lower sema::Expr::Kind::Move");
			} break;

			case sema::Expr::Kind::DESTRUCTIVE_MOVE: {
				evo::unimplemented("lower sema::Expr::Kind::DestructiveMove");
			} break;

			case sema::Expr::Kind::FORWARD: {
				evo::unimplemented("lower sema::Expr::Kind::Forward");
			} break;

			case sema::Expr::Kind::FUNC_CALL: {
				const sema::FuncCall& func_call = this->context.getSemaBuffer().getFuncCall(expr.funcCallID());
				const Data::FuncInfo& target_func_info = this->data.get_func(func_call.target.as<sema::Func::ID>());

				auto args = evo::SmallVector<pir::Expr>();
				for(size_t i = 0; const sema::Expr& arg : func_call.args){
					if(target_func_info.arg_is_copy[i]){
						args.emplace_back(this->get_expr_register(arg));
					}else{
						args.emplace_back(this->get_expr_pointer(arg));
					}

					i += 1;
				}

				const pir::Expr output = [&](){
					return this->agent.createCall(
						target_func_info.pir_id,
						std::move(args),
						this->name("CALL.{}", this->mangle_name(func_call.target.as<sema::Func::ID>()))
					);
				}();

				if constexpr(MODE == GetExprMode::REGISTER){
					return output;

				}else if constexpr(MODE == GetExprMode::POINTER){
					const pir::Expr alloca = this->agent.createAlloca(target_func_info.return_type);
					this->agent.createStore(alloca, output, false, pir::AtomicOrdering::NONE);
					return alloca;
					
				}else{
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(store_locations.front(), output, false, pir::AtomicOrdering::NONE);
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::ADDR_OF: {
				evo::unimplemented("lower sema::Expr::Kind::AddrOf");
			} break;

			case sema::Expr::Kind::DEREF: {
				evo::unimplemented("lower sema::Expr::Kind::Deref");
			} break;

			case sema::Expr::Kind::PARAM: {
				evo::unimplemented("lower sema::Expr::Kind::Param");
			} break;

			case sema::Expr::Kind::RETURN_PARAM: {
				evo::unimplemented("lower sema::Expr::Kind::ReturnParam");
			} break;

			case sema::Expr::Kind::GLOBAL_VAR: {
				const pir::GlobalVar::ID pir_var_id = this->data.get_global_var(expr.globalVarID());
				
				if constexpr(MODE == GetExprMode::REGISTER){
					const pir::GlobalVar& pir_var = this->module.getGlobalVar(pir_var_id);
					return this->agent.createLoad(
						this->agent.createGlobalValue(pir_var_id),
						pir_var.type,
						false,
						pir::AtomicOrdering::NONE,
						this->name("{}.LOAD", this->mangle_name(expr.globalVarID()))
					);

				}else if constexpr(MODE == GetExprMode::POINTER){
					return this->agent.createGlobalValue(pir_var_id);
					
				}else{
					evo::debugAssert(store_locations.size() == 1, "Only has 1 value to store");
					this->agent.createStore(
						store_locations[0], this->agent.createGlobalValue(pir_var_id), false, pir::AtomicOrdering::NONE
					);
					return std::nullopt;
				}
			} break;

			case sema::Expr::Kind::FUNC: {
				evo::unimplemented("lower sema::Expr::Kind::Func");
			} break;
		}

		evo::unreachable();
	}



	auto SemaToPIR::get_global_var_value(const sema::Expr expr) -> pir::GlobalVar::Value {
		switch(expr.kind()){
			case sema::Expr::Kind::NONE: evo::debugFatalBreak("Invalid Expr");

			case sema::Expr::Kind::UNINIT: {
				return pir::GlobalVar::Uninit();
			} break;

			case sema::Expr::Kind::ZEROINIT: {
				return pir::GlobalVar::Zeroinit();
			} break;

			case sema::Expr::Kind::INT_VALUE: {
				const sema::IntValue& int_value = this->context.getSemaBuffer().getIntValue(expr.intValueID());
				return this->agent.createNumber(this->get_type(*int_value.typeID), int_value.value);
			} break;

			case sema::Expr::Kind::FLOAT_VALUE: {
				const sema::FloatValue& float_value = this->context.getSemaBuffer().getFloatValue(expr.floatValueID());
				return this->agent.createNumber(this->get_type(*float_value.typeID), float_value.value);
			} break;

			case sema::Expr::Kind::BOOL_VALUE: {
				const sema::BoolValue& bool_value = this->context.getSemaBuffer().getBoolValue(expr.boolValueID());
				return this->agent.createBoolean(bool_value.value);
			} break;

			case sema::Expr::Kind::STRING_VALUE: {
				const sema::StringValue& string_value =
					this->context.getSemaBuffer().getStringValue(expr.stringValueID());

				return this->module.createGlobalString(evo::copy(string_value.value));
			} break;

			case sema::Expr::Kind::CHAR_VALUE: {
				const sema::CharValue& char_value = this->context.getSemaBuffer().getCharValue(expr.charValueID());
				return this->agent.createNumber(
					this->module.createIntegerType(8), core::GenericInt(8, uint64_t(char_value.value))
				);
			} break;

			case sema::Expr::Kind::MODULE_IDENT:                      case sema::Expr::Kind::INTRINSIC:
			case sema::Expr::Kind::TEMPLATED_INTRINSIC_INSTANTIATION: case sema::Expr::Kind::COPY:
			case sema::Expr::Kind::MOVE:                              case sema::Expr::Kind::DESTRUCTIVE_MOVE:
			case sema::Expr::Kind::FORWARD:                           case sema::Expr::Kind::FUNC_CALL:
			case sema::Expr::Kind::ADDR_OF:                           case sema::Expr::Kind::DEREF:
			case sema::Expr::Kind::PARAM:                             case sema::Expr::Kind::RETURN_PARAM:
			case sema::Expr::Kind::GLOBAL_VAR:                        case sema::Expr::Kind::FUNC: {
				evo::debugFatalBreak("Not valid global var value");
			} break;
		}

		evo::unreachable();
	}



	//////////////////////////////////////////////////////////////////////
	// get type


	auto SemaToPIR::get_type(const TypeInfo::VoidableID voidable_type_id) -> pir::Type {
		if(voidable_type_id.isVoid()){ return this->module.createVoidType(); }
		return this->get_type(voidable_type_id.asTypeID());
	}


	auto SemaToPIR::get_type(const TypeInfo::ID type_id) -> pir::Type {
		return this->get_type(this->context.getTypeManager().getTypeInfo(type_id));
	}

	auto SemaToPIR::get_type(const TypeInfo& type_info) -> pir::Type {
		if(type_info.isPointer()){ return this->module.createPtrType(); }

		if(type_info.isOptionalNotPointer()){
			evo::unimplemented("Optional type");
		}

		return this->get_type(type_info.baseTypeID());
	}


	auto SemaToPIR::get_type(const BaseType::ID base_type_id) -> pir::Type {
		switch(base_type_id.kind()){
			case BaseType::Kind::DUMMY: evo::debugFatalBreak("Not a valid base type");
			
			case BaseType::Kind::PRIMITIVE: {
				const BaseType::Primitive& primitive = 
					this->context.getTypeManager().getPrimitive(base_type_id.primitiveID());

				switch(primitive.kind()){
					case Token::Kind::TYPE_INT:      case Token::Kind::TYPE_ISIZE:   case Token::Kind::TYPE_UINT:
					case Token::Kind::TYPE_USIZE:    case Token::Kind::TYPE_TYPEID:  case Token::Kind::TYPE_C_SHORT:
					case Token::Kind::TYPE_C_USHORT: case Token::Kind::TYPE_C_INT:   case Token::Kind::TYPE_C_UINT:
					case Token::Kind::TYPE_C_LONG:   case Token::Kind::TYPE_C_ULONG: case Token::Kind::TYPE_C_LONG_LONG:
					case Token::Kind::TYPE_C_ULONG_LONG:
						return this->module.createIntegerType(
							uint32_t(this->context.getTypeManager().sizeOf(base_type_id) * 8)
						);

					case Token::Kind::TYPE_I_N:
					case Token::Kind::TYPE_UI_N:
						return this->module.createIntegerType(primitive.bitWidth());

					case Token::Kind::TYPE_F16:    return this->module.createFloatType(16);
					case Token::Kind::TYPE_BF16:   return this->module.createBFloatType();
					case Token::Kind::TYPE_F32:    return this->module.createFloatType(32);
					case Token::Kind::TYPE_F64:    return this->module.createFloatType(64);
					case Token::Kind::TYPE_F80:    return this->module.createFloatType(80);
					case Token::Kind::TYPE_F128:   return this->module.createFloatType(128);

					case Token::Kind::TYPE_BYTE:   return this->module.createIntegerType(8);
					case Token::Kind::TYPE_BOOL:   return this->module.createBoolType();
					case Token::Kind::TYPE_CHAR:   return this->module.createIntegerType(8);

					case Token::Kind::TYPE_RAWPTR: return this->module.createPtrType();

					case Token::Kind::TYPE_C_LONG_DOUBLE: 
						return this->context.getTypeManager().sizeOf(base_type_id) 
							? this->module.createFloatType(64)
							: this->module.createFloatType(128);

					default: evo::debugFatalBreak("Unknown builtin type");
				}
			} break;
			
			case BaseType::Kind::FUNCTION: {
				evo::unimplemented("BaseType::Kind::FUNCTION");
			} break;
			
			case BaseType::Kind::ARRAY: {
				const BaseType::Array& array = this->context.getTypeManager().getArray(base_type_id.arrayID());
				const pir::Type elem_type = this->get_type(array.elementTypeID);

				uint64_t length = array.lengths.front();
				for(size_t i = 1; i < array.lengths.size(); i+=1){
					length *= array.lengths[i];
				}

				if(array.terminator.has_value()){ length += 1; }

				return this->module.createArrayType(elem_type, length);
			} break;
			
			case BaseType::Kind::ALIAS: {
				const BaseType::Alias& alias = this->context.getTypeManager().getAlias(base_type_id.aliasID());
				return this->get_type(*alias.aliasedType.load());
			} break;
			
			case BaseType::Kind::TYPEDEF: {
				const BaseType::Typedef& typedef_type = 
					this->context.getTypeManager().getTypedef(base_type_id.typedefID());
				return this->get_type(*typedef_type.underlyingType.load());
			} break;
			
			case BaseType::Kind::STRUCT: {
				evo::unimplemented("BaseType::Kind::STRUCT");
			} break;
			
			case BaseType::Kind::STRUCT_TEMPLATE: {
				evo::debugFatalBreak("Cannot get type of struct template");
			} break;
		}

		return this->module.createIntegerType(12);
	}



	//////////////////////////////////////////////////////////////////////
	// name mangling

	auto SemaToPIR::mangle_name(const BaseType::Struct::ID struct_id) const -> std::string {
		const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(struct_id);
		const Source& source = this->context.getSourceManager()[struct_type.sourceID];

		if(this->data.getConfig().useReadableNames){
			return std::format(
				"PTHR.s{}.{}", struct_id.get(), source.getTokenBuffer()[struct_type.identTokenID].getString()
			);
			
		}else{
			return std::format("PTHR.s{}", struct_id.get());
		}
	}

	auto SemaToPIR::mangle_name(const sema::GlobalVar::ID global_var_id) const -> std::string {
		const sema::GlobalVar& global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);
		const Source& source = this->context.getSourceManager()[global_var.sourceID];

		if(this->data.getConfig().useReadableNames){
			return std::format(
				"PTHR.v{}.{}", global_var_id.get(), source.getTokenBuffer()[global_var.ident].getString()
			);
			
		}else{
			return std::format("PTHR.v{}", global_var_id.get());
		}
	}


	auto SemaToPIR::mangle_name(const sema::Func::ID func_id) const -> std::string {
		const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);
		const Source& source = this->context.getSourceManager()[func.sourceID];

		if(func.name.kind() == AST::Kind::IDENT){
			if(this->data.getConfig().useReadableNames){
				return std::format(
					"PTHR.f{}.{}",
					func_id.get(),
					source.getTokenBuffer()[source.getASTBuffer().getIdent(func.name)].getString()
				);
			}else{
				return std::format("PTHR.f{}", func_id.get());
			}

		}else{
			evo::unimplemented("Name mangling of operator overload func name");
		}
	}





	auto SemaToPIR::name(std::string_view str) const -> std::string {
		if(this->data.getConfig().useReadableNames) [[unlikely]] {
			return std::string(str);
		}else{
			return std::string();
		}
	}


	template<class... Args>
	auto SemaToPIR::name(std::format_string<Args...> fmt, Args&&... args) const -> std::string {
		if(this->data.getConfig().useReadableNames) [[unlikely]] {
			return std::format(fmt, std::forward<Args...>(args)...);
		}else{
			return std::string();
		}
	}


}