////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./SemaToPIR.h"



namespace pcit::panther{
	

	auto SemaToPIR::lower() -> void {
		this->global_vars.reserve(this->context.getSemaBuffer().numGlobalVars());
		for(const sema::GlobalVar::ID& global_var_id : this->context.getSemaBuffer().getGlobalVars()){
			this->lower_global(global_var_id);
		}

		this->funcs.reserve(this->context.getSemaBuffer().numFuncs());
		for(const sema::Func::ID& func_id : this->context.getSemaBuffer().getFuncs()){
			this->lower_func_decl(func_id);
		}

		for(uint32_t i = 0; i < this->context.getTypeManager().structs.size(); i+=1){
			this->lower_struct(BaseType::Struct::ID(i));
		}
	}



	auto SemaToPIR::lower_struct(const BaseType::Struct::ID struct_id) -> void {
		evo::debugAssert(size_t(struct_id.get()) == this->structs.size(), "Struct isn't lining up");
		// const BaseType::Struct& struct_type = this->context.getTypeManager().getStruct(struct_id);

		this->structs.emplace_back(this->module.createStructType(
			this->mangle_name(struct_id),
			evo::SmallVector<pir::Type>(),
			false
		));
	}



	auto SemaToPIR::lower_global(const sema::GlobalVar::ID global_var_id) -> void {
		evo::debugAssert(size_t(global_var_id.get()) == this->global_vars.size(), "Global Var isn't lining up");
		const sema::GlobalVar& global_var = this->context.getSemaBuffer().getGlobalVar(global_var_id);

		if(global_var.kind == AST::VarDecl::Kind::Def){ return; }

		this->global_vars.emplace_back(this->module.createGlobalVar(
			this->mangle_name(global_var_id),
			this->get_type(*global_var.typeID),
			pcit::pir::Linkage::Private,
			this->get_global_var_value(*global_var.expr.load()),
			global_var.kind == AST::VarDecl::Kind::Const
		));
	}


	auto SemaToPIR::lower_func_decl(const sema::Func::ID func_id) -> void {
		evo::debugAssert(size_t(func_id.get()) == this->funcs.size(), "Function isn't lining up");

		const sema::Func& func = this->context.getSemaBuffer().getFunc(func_id);
		this->current_source = &this->context.getSourceManager()[func.sourceID];
		EVO_DEFER([&](){ this->current_source = nullptr; });

		if(func.hasInParam){ evo::unimplemented("functions with `in` parameter"); }

		const BaseType::Function& func_type = this->context.getTypeManager().getFunction(func.typeID);


		auto params = evo::SmallVector<pir::Parameter>();
		params.reserve(func_type.params.size() + func_type.returnParams.size() + func_type.errorParams.size());

		for(const BaseType::Function::Param& param : func_type.params){
			std::string param_name = [&](){
				if(this->config.useReadableNames == false){
					return std::format(".{}", params.size());

				}else if(param.ident.is<Token::ID>()){
					return std::string(this->current_source->getTokenBuffer()[param.ident.as<Token::ID>()].getString());

				}else{
					return std::string(strings::toStringView(param.ident.as<strings::StringCode>()));
				}
			}();

			if(param.shouldCopy){
				params.emplace_back(std::move(param_name), this->get_type(param.typeID));
			}else{
				params.emplace_back(std::move(param_name), this->module.createPtrType());
			}
		}

		if(func_type.hasNamedReturns()){
			for(const BaseType::Function::ReturnParam& return_param : func_type.returnParams){
				if(this->config.useReadableNames){
					params.emplace_back(
						std::format("RET.{}", this->current_source->getTokenBuffer()[*return_param.ident].getString()),
						this->module.createPtrType()
					);
				}else{
					params.emplace_back(std::format(".{}", params.size()), this->module.createPtrType());
				}
			}

		}else if(func_type.hasErrorReturns() && func_type.returnsVoid() == false){
			if(this->config.useReadableNames){
				params.emplace_back("RET", this->module.createPtrType());
			}else{
				params.emplace_back(std::format(".{}", params.size()), this->module.createPtrType());
			}
		}

		if(func_type.hasErrorReturnParams()){
			for(const BaseType::Function::ReturnParam& error_param : func_type.errorParams){
				if(this->config.useReadableNames){
					params.emplace_back(
						std::format("ERR.{}", this->current_source->getTokenBuffer()[*error_param.ident].getString()),
						this->module.createPtrType()
					);
				}else{
					params.emplace_back(std::format(".{}", params.size()), this->module.createPtrType());
				}
			}
		}

		const pir::Type return_type = [&](){
			if(func_type.hasErrorReturns()){
				return this->module.createBoolType();

			}else if(func_type.hasNamedReturns()){
				return this->module.createVoidType();

			}else{
				return this->get_type(func_type.returnParams.front().typeID);
			}
		}();

		const pcit::pir::Function::ID new_func_id = module.createFunction(
			this->mangle_name(func_id),
			std::move(params),
			this->config.isJIT ? pir::CallingConvention::C : pir::CallingConvention::Fast,
			this->config.isJIT ? pir::Linkage::External : pir::Linkage::Private,
			return_type
		);

		this->agent.setTargetFunction(new_func_id);

		this->funcs.emplace_back(new_func_id);



		if(func_type.params.empty()){
			this->agent.createBasicBlock(this->name("begin"));

		}else{
			const pir::BasicBlock::ID setup_block = this->agent.createBasicBlock(this->name("setup"));
			const pir::BasicBlock::ID begin_block = this->agent.createBasicBlock(this->name("begin"));

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
				const pir::Type param_alloca_type = param.shouldCopy ? param_type : this->module.createPtrType();
				const pir::Expr param_alloca = this->agent.createAlloca(
					param_alloca_type, this->name("{}.alloca", param_name)
				);

				// this->param_infos.emplace(
				// 	ASG::Param::LinkID(asg_func_link_id, func.params[i]), ParamInfo(param_alloca, param_type, i)
				// );

				this->agent.createStore(param_alloca, this->agent.createParamExpr(i), false, pir::AtomicOrdering::None);

				i += 1;
			}

			this->agent.createBranch(begin_block);
		}
	}




	//////////////////////////////////////////////////////////////////////
	// get expr


	auto SemaToPIR::get_global_var_value(const sema::Expr expr) -> pir::GlobalVar::Value {
		switch(expr.kind()){
			case sema::Expr::Kind::None: evo::debugFatalBreak("Invalid Expr");

			case sema::Expr::Kind::Uninit: {
				return pir::GlobalVar::Uninit();
			} break;

			case sema::Expr::Kind::Zeroinit: {
				return pir::GlobalVar::Zeroinit();
			} break;

			case sema::Expr::Kind::IntValue: {
				const sema::IntValue& int_value = this->context.getSemaBuffer().getIntValue(expr.intValueID());
				return this->agent.createNumber(this->get_type(*int_value.typeID), int_value.value);
			} break;

			case sema::Expr::Kind::FloatValue: {
				const sema::FloatValue& float_value = this->context.getSemaBuffer().getFloatValue(expr.floatValueID());
				return this->agent.createNumber(this->get_type(*float_value.typeID), float_value.value);
			} break;

			case sema::Expr::Kind::BoolValue: {
				const sema::BoolValue& bool_value = this->context.getSemaBuffer().getBoolValue(expr.boolValueID());
				return this->agent.createBoolean(bool_value.value);
			} break;

			case sema::Expr::Kind::StringValue: {
				const sema::StringValue& string_value =
					this->context.getSemaBuffer().getStringValue(expr.stringValueID());

				return this->module.createGlobalString(evo::copy(string_value.value));
			} break;

			case sema::Expr::Kind::CharValue: {
				const sema::CharValue& char_value = this->context.getSemaBuffer().getCharValue(expr.charValueID());
				return this->agent.createNumber(
					this->module.createIntegerType(8), core::GenericInt(8, uint64_t(char_value.value))
				);
			} break;

			case sema::Expr::Kind::Intrinsic: case sema::Expr::Kind::TemplatedIntrinsicInstantiation:
			case sema::Expr::Kind::Copy:      case sema::Expr::Kind::Move:     case sema::Expr::Kind::DestructiveMove:
			case sema::Expr::Kind::Forward:   case sema::Expr::Kind::FuncCall: case sema::Expr::Kind::AddrOf:
			case sema::Expr::Kind::Deref:     case sema::Expr::Kind::Param:    case sema::Expr::Kind::ReturnParam:
			case sema::Expr::Kind::GlobalVar: case sema::Expr::Kind::Func: {
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
			case BaseType::Kind::Dummy: evo::debugFatalBreak("Not a valid base type");
			
			case BaseType::Kind::Primitive: {
				const BaseType::Primitive& primitive = 
					this->context.getTypeManager().getPrimitive(base_type_id.primitiveID());

				switch(primitive.kind()){
					case Token::Kind::TypeInt:        case Token::Kind::TypeISize:  case Token::Kind::TypeUInt:
					case Token::Kind::TypeUSize:      case Token::Kind::TypeTypeID: case Token::Kind::TypeCShort:
					case Token::Kind::TypeCUShort:    case Token::Kind::TypeCInt:   case Token::Kind::TypeCUInt:
					case Token::Kind::TypeCLong:      case Token::Kind::TypeCULong: case Token::Kind::TypeCLongLong:
					case Token::Kind::TypeCULongLong:
						return this->module.createIntegerType(
							uint32_t(this->context.getTypeManager().sizeOf(base_type_id) * 8)
						);

					case Token::Kind::TypeI_N:
					case Token::Kind::TypeUI_N:
						return this->module.createIntegerType(primitive.bitWidth());

					case Token::Kind::TypeF16:    return this->module.createFloatType(16);
					case Token::Kind::TypeBF16:   return this->module.createBFloatType();
					case Token::Kind::TypeF32:    return this->module.createFloatType(32);
					case Token::Kind::TypeF64:    return this->module.createFloatType(64);
					case Token::Kind::TypeF80:    return this->module.createFloatType(80);
					case Token::Kind::TypeF128:   return this->module.createFloatType(128);

					case Token::Kind::TypeByte:   return this->module.createIntegerType(8);
					case Token::Kind::TypeBool:   return this->module.createBoolType();
					case Token::Kind::TypeChar:   return this->module.createIntegerType(8);

					case Token::Kind::TypeRawPtr: return this->module.createPtrType();

					case Token::Kind::TypeCLongDouble: 
						return this->context.getTypeManager().sizeOf(base_type_id) 
							? this->module.createFloatType(64)
							: this->module.createFloatType(128);
				}
			} break;
			
			case BaseType::Kind::Function: {
				evo::unimplemented("BaseType::Kind::Function");
			} break;
			
			case BaseType::Kind::Array: {
				const BaseType::Array& array = this->context.getTypeManager().getArray(base_type_id.arrayID());
				const pir::Type elem_type = this->get_type(array.elementTypeID);

				uint64_t length = array.lengths.front();
				for(size_t i = 1; i < array.lengths.size(); i+=1){
					length *= array.lengths[i];
				}

				if(array.terminator.has_value()){ length += 1; }

				return this->module.createArrayType(elem_type, length);
			} break;
			
			case BaseType::Kind::Alias: {
				const BaseType::Alias& alias = this->context.getTypeManager().getAlias(base_type_id.aliasID());
				return this->get_type(*alias.aliasedType.load());
			} break;
			
			case BaseType::Kind::Typedef: {
				const BaseType::Typedef& typedef_type = 
					this->context.getTypeManager().getTypedef(base_type_id.typedefID());
				return this->get_type(*typedef_type.underlyingType.load());
			} break;
			
			case BaseType::Kind::Struct: {
				evo::unimplemented("BaseType::Kind::Struct");
			} break;
			
			case BaseType::Kind::StructTemplate: {
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

		if(this->config.useReadableNames){
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

		if(this->config.useReadableNames){
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

		if(func.name.kind() == AST::Kind::Ident){
			if(this->config.useReadableNames){
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
		if(this->config.useReadableNames) [[unlikely]] {
			return std::string(str);
		}else{
			return std::string();
		}
	}


	template<class... Args>
	auto SemaToPIR::name(std::format_string<Args...> fmt, Args&&... args) const -> std::string {
		if(this->config.useReadableNames) [[unlikely]] {
			return std::format(fmt, std::forward<Args...>(args)...);
		}else{
			return std::string();
		}
	}


}