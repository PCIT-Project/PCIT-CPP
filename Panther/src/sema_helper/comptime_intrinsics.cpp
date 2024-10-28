//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of PCIT-CPP, under the Apache License v2.0                  //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./comptime_intrinsics.h"

#include "../../include/Context.h"
// #include "../../include/ASGBuffer.h"
#include "../SemanticAnalyzer.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther::sema_helper{


	auto ComptimeIntrins::call(
		const ASG::TemplatedIntrinsicInstantiation& instantiation,
		evo::ArrayProxy<ASG::Expr> args,
		const AST::FuncCall& func_call
	) -> evo::Result<evo::SmallVector<ASG::Expr>> {

		switch(instantiation.kind){
			///////////////////////////////////
			// type traits 

			case TemplatedIntrinsic::Kind::IsSameType:              return this->isSameType(instantiation);
			case TemplatedIntrinsic::Kind::IsTriviallyCopyable:     return this->isTriviallyCopyable(instantiation);
			case TemplatedIntrinsic::Kind::IsTriviallyDestructable: return this->isTriviallyDestructable(instantiation);
			case TemplatedIntrinsic::Kind::IsPrimitive:             return this->isPrimitive(instantiation);
			case TemplatedIntrinsic::Kind::IsBuiltin:               return this->isBuiltin(instantiation);
			case TemplatedIntrinsic::Kind::IsIntegral:              return this->isIntegral(instantiation);
			case TemplatedIntrinsic::Kind::IsFloatingPoint:         return this->isFloatingPoint(instantiation);

			case TemplatedIntrinsic::Kind::SizeOf:                  return this->sizeOf(instantiation);
			case TemplatedIntrinsic::Kind::GetTypeID:               return this->getTypeID(instantiation);


			///////////////////////////////////
			// type conversion

			case TemplatedIntrinsic::Kind::BitCast: {
				this->analyzer.emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					func_call,
					"Compile-time `@bitCast` is not supported yet"
				);
				return evo::resultError;
			} break;

			case TemplatedIntrinsic::Kind::Trunc: {
				this->analyzer.emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					func_call,
					"Compile-time `@trunc` is not supported yet"
				);
				return evo::resultError;
			} break;

			case TemplatedIntrinsic::Kind::FTrunc: {
				this->analyzer.emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					func_call,
					"Compile-time `@ftrunc` is not supported yet"
				);
				return evo::resultError;
			} break;

			case TemplatedIntrinsic::Kind::SExt: {
				this->analyzer.emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					func_call,
					"Compile-time `@sext` is not supported yet"
				);
				return evo::resultError;
			} break;

			case TemplatedIntrinsic::Kind::ZExt: {
				this->analyzer.emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					func_call,
					"Compile-time `@zext` is not supported yet"
				);
				return evo::resultError;
			} break;

			case TemplatedIntrinsic::Kind::FExt: {
				this->analyzer.emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					func_call,
					"Compile-time `@fext` is not supported yet"
				);
				return evo::resultError;
			} break;

			case TemplatedIntrinsic::Kind::IToF: {
				this->analyzer.emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					func_call,
					"Compile-time `@itof` is not supported yet"
				);
				return evo::resultError;
			} break;


			case TemplatedIntrinsic::Kind::UIToF: {
				this->analyzer.emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					func_call,
					"Compile-time `@uitof` is not supported yet"
				);
				return evo::resultError;
			} break;

			case TemplatedIntrinsic::Kind::FToI: {
				this->analyzer.emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					func_call,
					"Compile-time `@ftoi` is not supported yet"
				);
				return evo::resultError;
			} break;

			case TemplatedIntrinsic::Kind::FToUI: {
				this->analyzer.emit_error(
					Diagnostic::Code::MiscUnimplementedFeature,
					func_call,
					"Compile-time `@ftoui` is not supported yet"
				);
				return evo::resultError;
			} break;


			///////////////////////////////////
			// arithmetic

			case TemplatedIntrinsic::Kind::Add:
				return this->add(args, instantiation.templateArgs[1].as<bool>(), func_call);
			case TemplatedIntrinsic::Kind::AddWrap: return this->addWrap(args);
			case TemplatedIntrinsic::Kind::AddSat:  return this->addSat(args);
			case TemplatedIntrinsic::Kind::FAdd:    return this->fadd(args);

			case TemplatedIntrinsic::Kind::Sub: 
				return this->sub(args, instantiation.templateArgs[1].as<bool>(), func_call);
			case TemplatedIntrinsic::Kind::SubWrap: return this->subWrap(args);
			case TemplatedIntrinsic::Kind::SubSat:  return this->subSat(args);
			case TemplatedIntrinsic::Kind::FSub:    return this->fsub(args);


			case TemplatedIntrinsic::Kind::Mul: 
				return this->mul(args, instantiation.templateArgs[1].as<bool>(), func_call);
			case TemplatedIntrinsic::Kind::MulWrap: return this->mulWrap(args);
			case TemplatedIntrinsic::Kind::MulSat:  return this->mulSat(args);
			case TemplatedIntrinsic::Kind::FMul:    return this->fmul(args);

			case TemplatedIntrinsic::Kind::Div:  return this->div(args);
			case TemplatedIntrinsic::Kind::FDiv: return this->fdiv(args);
			case TemplatedIntrinsic::Kind::Rem:  return this->rem(args);


			///////////////////////////////////
			// logical

			case TemplatedIntrinsic::Kind::Eq:  return this->eq(args);
			case TemplatedIntrinsic::Kind::NEq: return this->neq(args);
			case TemplatedIntrinsic::Kind::LT:  return this->lt(args);
			case TemplatedIntrinsic::Kind::LTE: return this->lte(args);
			case TemplatedIntrinsic::Kind::GT:  return this->gt(args);
			case TemplatedIntrinsic::Kind::GTE: return this->gte(args);


			///////////////////////////////////
			// bitwise

			case TemplatedIntrinsic::Kind::And:    return this->bitwiseAnd(args);
			case TemplatedIntrinsic::Kind::Or:     return this->bitwiseOr(args);
			case TemplatedIntrinsic::Kind::Xor:    return this->bitwiseXor(args);
			case TemplatedIntrinsic::Kind::SHL:
				return this->shl(args, instantiation.templateArgs[2].as<bool>(), func_call);
			case TemplatedIntrinsic::Kind::SHLSat: return this->shlSat(args);
			case TemplatedIntrinsic::Kind::SHR:
				return this->shr(args, instantiation.templateArgs[2].as<bool>(), func_call);



			///////////////////////////////////
			// _max_

			case TemplatedIntrinsic::Kind::_max_: {
				evo::debugFatalBreak("Intrinsic::Kind::_max_ is not an actual intrinsic");
			} break;
		}

		evo::debugFatalBreak("Unknown or unsupported templated intrinsic kind");
	}



	//////////////////////////////////////////////////////////////////////
	// type traits

	auto ComptimeIntrins::isSameType(const ASG::TemplatedIntrinsicInstantiation& instantiation)
	-> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralBool::ID literal_bool_id = this->analyzer.get_asg_buffer().createLiteralBool(
			instantiation.templateArgs[0].as<TypeInfo::VoidableID>() == 
			instantiation.templateArgs[1].as<TypeInfo::VoidableID>()
		);
		return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_bool_id)};
	}

	auto ComptimeIntrins::isTriviallyCopyable(const ASG::TemplatedIntrinsicInstantiation& instantiation)
	-> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralBool::ID literal_bool_id = this->analyzer.get_asg_buffer().createLiteralBool(
			this->analyzer.context.getTypeManager().isTriviallyCopyable(
				instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
			)
		);
		return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_bool_id)};
	}

	auto ComptimeIntrins::isTriviallyDestructable(const ASG::TemplatedIntrinsicInstantiation& instantiation)
	-> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralBool::ID literal_bool_id = this->analyzer.get_asg_buffer().createLiteralBool(
			this->analyzer.context.getTypeManager().isTriviallyDestructable(
				instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
			)
		);
		return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_bool_id)};
	}

	auto ComptimeIntrins::isPrimitive(const ASG::TemplatedIntrinsicInstantiation& instantiation)
	-> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralBool::ID literal_bool_id = this->analyzer.get_asg_buffer().createLiteralBool(
			this->analyzer.context.getTypeManager().isPrimitive(
				instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
			)
		);
		return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_bool_id)};
	}

	auto ComptimeIntrins::isBuiltin(const ASG::TemplatedIntrinsicInstantiation& instantiation)
	-> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralBool::ID literal_bool_id = this->analyzer.get_asg_buffer().createLiteralBool(
			this->analyzer.context.getTypeManager().isBuiltin(
				instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
			)
		);
		return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_bool_id)};
	}

	auto ComptimeIntrins::isIntegral(const ASG::TemplatedIntrinsicInstantiation& instantiation)
	-> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralBool::ID literal_bool_id = this->analyzer.get_asg_buffer().createLiteralBool(
			this->analyzer.context.getTypeManager().isIntegral(
				instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
			)
		);
		return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_bool_id)};
	}

	auto ComptimeIntrins::isFloatingPoint(const ASG::TemplatedIntrinsicInstantiation& instantiation)
	-> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralBool::ID literal_bool_id = this->analyzer.get_asg_buffer().createLiteralBool(
			this->analyzer.context.getTypeManager().isFloatingPoint(
				instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
			)
		);
		return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_bool_id)};
	}

	auto ComptimeIntrins::sizeOf(const ASG::TemplatedIntrinsicInstantiation& instantiation)
	-> evo::SmallVector<ASG::Expr> {
		const size_t type_size = this->analyzer.context.getTypeManager().sizeOf(
			instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID()
		);

		const ASG::LiteralInt::ID literal_int_id = this->analyzer.get_asg_buffer().createLiteralInt(
			core::GenericInt::create<uint64_t>(type_size),
			TypeManager::getTypeUSize()
		);
		return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_int_id)};
	}

	auto ComptimeIntrins::getTypeID(const ASG::TemplatedIntrinsicInstantiation& instantiation)
	-> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralInt::ID literal_int_id = this->analyzer.get_asg_buffer().createLiteralInt(
			core::GenericInt::create<uint32_t>(
				instantiation.templateArgs[0].as<TypeInfo::VoidableID>().typeID().get()
			),
			this->analyzer.context.getTypeManager().getTypeTypeID()
		);
		return evo::SmallVector<ASG::Expr>{ASG::Expr(literal_int_id)};
	}


	//////////////////////////////////////////////////////////////////////
	// arithmetic

	///////////////////////////////////
	// addition

	auto ComptimeIntrins::add(evo::ArrayProxy<ASG::Expr> args, bool may_wrap, const AST::FuncCall& func_call)
	-> evo::Result<evo::SmallVector<ASG::Expr>> {
		const ASG::LiteralInt& lhs_literal_int = 
			this->analyzer.source.getASGBuffer().getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const evo::Result<core::GenericInt> result = this->analyzer.get_comptime_executor().intrinAdd(
			type_id,
			may_wrap,
			lhs_literal_int.value,
			this->analyzer.source.getASGBuffer().getLiteralInt(args[1].literalIntID()).value
		);

		if(result.isError()){
			this->analyzer.emit_error(
				Diagnostic::Code::SemaErrorInRunningOfIntrinsicAtComptime,
				func_call,
				"Integral arithmetic wrapping occured"
			);
			return evo::resultError;
		}

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result.value(), type_id))
		};
	}

	auto ComptimeIntrins::addWrap(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralInt& lhs_literal_int = 
			this->analyzer.source.getASGBuffer().getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const core::GenericInt::WrapResult result = this->analyzer.get_comptime_executor().intrinAddWrap(
			type_id,
			lhs_literal_int.value,
			this->analyzer.source.getASGBuffer().getLiteralInt(args[1].literalIntID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result.result, type_id)),
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralBool(result.wrapped))
		};
	}

	auto ComptimeIntrins::addSat(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralInt& lhs_literal_int = 
			this->analyzer.source.getASGBuffer().getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const core::GenericInt result = this->analyzer.get_comptime_executor().intrinAddSat(
			type_id,
			lhs_literal_int.value,
			this->analyzer.source.getASGBuffer().getLiteralInt(args[1].literalIntID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result, type_id))
		};
	}

	auto ComptimeIntrins::fadd(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		const ASG::LiteralFloat& lhs_literal_float = 
			asg_buffer.getLiteralFloat(args[0].literalFloatID());
		const TypeInfo::ID type_id = *lhs_literal_float.typeID;

		const core::GenericFloat result = this->analyzer.get_comptime_executor().intrinFAdd(
			type_id,
			lhs_literal_float.value,
			asg_buffer.getLiteralFloat(args[1].literalFloatID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralFloat(result, type_id))
		};
	}


	///////////////////////////////////
	// subtraction

	auto ComptimeIntrins::sub(evo::ArrayProxy<ASG::Expr> args, bool may_wrap, const AST::FuncCall& func_call)
	-> evo::Result<evo::SmallVector<ASG::Expr>> {
		const ASG::LiteralInt& lhs_literal_int = 
			this->analyzer.source.getASGBuffer().getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const evo::Result<core::GenericInt> result = this->analyzer.get_comptime_executor().intrinSub(
			type_id,
			may_wrap,
			lhs_literal_int.value,
			this->analyzer.source.getASGBuffer().getLiteralInt(args[1].literalIntID()).value
		);

		if(result.isError()){
			this->analyzer.emit_error(
				Diagnostic::Code::SemaErrorInRunningOfIntrinsicAtComptime,
				func_call,
				"Integral arithmetic wrapping occured"
			);
			return evo::resultError;
		}

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result.value(), type_id))
		};
	}

	auto ComptimeIntrins::subWrap(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralInt& lhs_literal_int = 
			this->analyzer.source.getASGBuffer().getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const core::GenericInt::WrapResult result = this->analyzer.get_comptime_executor().intrinSubWrap(
			type_id,
			lhs_literal_int.value,
			this->analyzer.source.getASGBuffer().getLiteralInt(args[1].literalIntID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result.result, type_id)),
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralBool(result.wrapped))
		};
	}

	auto ComptimeIntrins::subSat(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralInt& lhs_literal_int = 
			this->analyzer.source.getASGBuffer().getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const core::GenericInt result = this->analyzer.get_comptime_executor().intrinSubSat(
			type_id,
			lhs_literal_int.value,
			this->analyzer.source.getASGBuffer().getLiteralInt(args[1].literalIntID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result, type_id))
		};
	}

	auto ComptimeIntrins::fsub(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		const ASG::LiteralFloat& lhs_literal_float = 
			asg_buffer.getLiteralFloat(args[0].literalFloatID());
		const TypeInfo::ID type_id = *lhs_literal_float.typeID;

		const core::GenericFloat result = this->analyzer.get_comptime_executor().intrinFSub(
			type_id,
			lhs_literal_float.value,
			asg_buffer.getLiteralFloat(args[1].literalFloatID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralFloat(result, type_id))
		};
	}


	///////////////////////////////////
	// multiplication

	auto ComptimeIntrins::mul(evo::ArrayProxy<ASG::Expr> args, bool may_wrap, const AST::FuncCall& func_call)
	-> evo::Result<evo::SmallVector<ASG::Expr>> {
		const ASG::LiteralInt& lhs_literal_int = 
			this->analyzer.source.getASGBuffer().getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const evo::Result<core::GenericInt> result = this->analyzer.get_comptime_executor().intrinMul(
			type_id,
			may_wrap,
			lhs_literal_int.value,
			this->analyzer.source.getASGBuffer().getLiteralInt(args[1].literalIntID()).value
		);

		if(result.isError()){
			this->analyzer.emit_error(
				Diagnostic::Code::SemaErrorInRunningOfIntrinsicAtComptime,
				func_call,
				"Integral arithmetic wrapping occured"
			);
			return evo::resultError;
		}

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result.value(), type_id))
		};
	}

	auto ComptimeIntrins::mulWrap(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralInt& lhs_literal_int = 
			this->analyzer.source.getASGBuffer().getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const core::GenericInt::WrapResult result = this->analyzer.get_comptime_executor().intrinMulWrap(
			type_id,
			lhs_literal_int.value,
			this->analyzer.source.getASGBuffer().getLiteralInt(args[1].literalIntID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result.result, type_id)),
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralBool(result.wrapped))
		};
	}

	auto ComptimeIntrins::mulSat(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralInt& lhs_literal_int = 
			this->analyzer.source.getASGBuffer().getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const core::GenericInt result = this->analyzer.get_comptime_executor().intrinMulSat(
			type_id,
			lhs_literal_int.value,
			this->analyzer.source.getASGBuffer().getLiteralInt(args[1].literalIntID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result, type_id))
		};
	}

	auto ComptimeIntrins::fmul(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		const ASG::LiteralFloat& lhs_literal_float = 
			asg_buffer.getLiteralFloat(args[0].literalFloatID());
		const TypeInfo::ID type_id = *lhs_literal_float.typeID;

		const core::GenericFloat result = this->analyzer.get_comptime_executor().intrinFMul(
			type_id,
			lhs_literal_float.value,
			asg_buffer.getLiteralFloat(args[1].literalFloatID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralFloat(result, type_id))
		};
	}



	///////////////////////////////////
	// division / remainder


	auto ComptimeIntrins::div(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASG::LiteralInt& lhs_literal_int = 
			this->analyzer.source.getASGBuffer().getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const core::GenericInt result = this->analyzer.get_comptime_executor().intrinDiv(
			type_id,
			lhs_literal_int.value,
			this->analyzer.source.getASGBuffer().getLiteralInt(args[1].literalIntID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result, type_id))
		};
	}

	auto ComptimeIntrins::fdiv(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		const ASG::LiteralFloat& lhs_literal_float = 
			asg_buffer.getLiteralFloat(args[0].literalFloatID());
		const TypeInfo::ID type_id = *lhs_literal_float.typeID;

		const core::GenericFloat result = this->analyzer.get_comptime_executor().intrinFDiv(
			type_id,
			lhs_literal_float.value,
			asg_buffer.getLiteralFloat(args[1].literalFloatID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralFloat(result, type_id))
		};
	}


	auto ComptimeIntrins::rem(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		if(args[0].kind() == ASG::Expr::Kind::LiteralInt){
			const ASG::LiteralInt& lhs_literal_int = 
				asg_buffer.getLiteralInt(args[0].literalIntID());
			const TypeInfo::ID type_id = *lhs_literal_int.typeID;

			const core::GenericInt result = this->analyzer.get_comptime_executor().intrinRem(
				type_id,
				lhs_literal_int.value,
				asg_buffer.getLiteralInt(args[1].literalIntID()).value
			);

			return evo::SmallVector<ASG::Expr>{
				ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result, type_id))
			};

		}else{
			evo::debugAssert(args[0].kind() == ASG::Expr::Kind::LiteralFloat, "Unknown or unsupported comptime value");

			const ASG::LiteralFloat& lhs_literal_float = 
				asg_buffer.getLiteralFloat(args[0].literalFloatID());
			const TypeInfo::ID type_id = *lhs_literal_float.typeID;

			const core::GenericFloat result = this->analyzer.get_comptime_executor().intrinRem(
				type_id,
				lhs_literal_float.value,
				asg_buffer.getLiteralFloat(args[1].literalFloatID()).value
			);

			return evo::SmallVector<ASG::Expr>{
				ASG::Expr(this->analyzer.get_asg_buffer().createLiteralFloat(result, type_id))
			};
		}
	}


	
	//////////////////////////////////////////////////////////////////////
	// logical

	auto ComptimeIntrins::eq(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		return this->logical_impl(args, [&](TypeInfo::ID type_id, auto lhs, auto rhs) -> bool {
			return this->analyzer.get_comptime_executor().intrinEQ(type_id, lhs, rhs);
		});
	}
	
	auto ComptimeIntrins::neq(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		return this->logical_impl(args, [&](TypeInfo::ID type_id, auto lhs, auto rhs) -> bool {
			return this->analyzer.get_comptime_executor().intrinNEQ(type_id, lhs, rhs);
		});
	}

	auto ComptimeIntrins::lt(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		return this->logical_impl(args, [&](TypeInfo::ID type_id, auto lhs, auto rhs) -> bool {
			return this->analyzer.get_comptime_executor().intrinLT(type_id, lhs, rhs);
		});
	}
	
	auto ComptimeIntrins::lte(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		return this->logical_impl(args, [&](TypeInfo::ID type_id, auto lhs, auto rhs) -> bool {
			return this->analyzer.get_comptime_executor().intrinLTE(type_id, lhs, rhs);
		});
	}

	auto ComptimeIntrins::gt(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		return this->logical_impl(args, [&](TypeInfo::ID type_id, auto lhs, auto rhs) -> bool {
			return this->analyzer.get_comptime_executor().intrinGT(type_id, lhs, rhs);
		});
	}

	auto ComptimeIntrins::gte(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		return this->logical_impl(args, [&](TypeInfo::ID type_id, auto lhs, auto rhs) -> bool {
			return this->analyzer.get_comptime_executor().intrinGTE(type_id, lhs, rhs);
		});
	}


	//////////////////////////////////////////////////////////////////////
	// bitwise

	auto ComptimeIntrins::bitwiseAnd(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		const ASG::LiteralInt& lhs_literal_int = 
			asg_buffer.getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const core::GenericInt result = this->analyzer.get_comptime_executor().intrinAnd(
			type_id, lhs_literal_int.value, asg_buffer.getLiteralInt(args[1].literalIntID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result, type_id))
		};
	}

	auto ComptimeIntrins::bitwiseOr(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		const ASG::LiteralInt& lhs_literal_int = 
			asg_buffer.getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const core::GenericInt result = this->analyzer.get_comptime_executor().intrinOr(
			type_id, lhs_literal_int.value, asg_buffer.getLiteralInt(args[1].literalIntID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result, type_id))
		};
	}

	auto ComptimeIntrins::bitwiseXor(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		const ASG::LiteralInt& lhs_literal_int = 
			asg_buffer.getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const core::GenericInt result = this->analyzer.get_comptime_executor().intrinXor(
			type_id, lhs_literal_int.value, asg_buffer.getLiteralInt(args[1].literalIntID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result, type_id))
		};
	}

	auto ComptimeIntrins::shl(evo::ArrayProxy<ASG::Expr> args, bool may_overflow, const AST::FuncCall& func_call)
	-> evo::Result<evo::SmallVector<ASG::Expr>> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		const ASG::LiteralInt& lhs_literal_int = 
			asg_buffer.getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const evo::Result<core::GenericInt> result = this->analyzer.get_comptime_executor().intrinSHL(
			type_id, lhs_literal_int.value, asg_buffer.getLiteralInt(args[1].literalIntID()).value, may_overflow
		);

		if(result.isError()){
			this->analyzer.emit_error(
				Diagnostic::Code::SemaErrorInRunningOfIntrinsicAtComptime,
				func_call,
				"shift-left overflow occured"
			);
			return evo::resultError;
		}

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result.value(), type_id))
		};
	}

	auto ComptimeIntrins::shlSat(evo::ArrayProxy<ASG::Expr> args) -> evo::SmallVector<ASG::Expr> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		const ASG::LiteralInt& lhs_literal_int = 
			asg_buffer.getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const core::GenericInt result = this->analyzer.get_comptime_executor().intrinSHLSat(
			type_id, lhs_literal_int.value, asg_buffer.getLiteralInt(args[1].literalIntID()).value
		);

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result, type_id))
		};
	}

	auto ComptimeIntrins::shr(evo::ArrayProxy<ASG::Expr> args, bool may_overflow, const AST::FuncCall& func_call)
	-> evo::Result<evo::SmallVector<ASG::Expr>> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		const ASG::LiteralInt& lhs_literal_int = 
			asg_buffer.getLiteralInt(args[0].literalIntID());
		const TypeInfo::ID type_id = *lhs_literal_int.typeID;

		const evo::Result<core::GenericInt> result = this->analyzer.get_comptime_executor().intrinSHR(
			type_id, lhs_literal_int.value, asg_buffer.getLiteralInt(args[1].literalIntID()).value, may_overflow
		);

		if(result.isError()){
			this->analyzer.emit_error(
				Diagnostic::Code::SemaErrorInRunningOfIntrinsicAtComptime,
				func_call,
				"shift-right overflow occured"
			);
			return evo::resultError;
		}

		return evo::SmallVector<ASG::Expr>{
			ASG::Expr(this->analyzer.get_asg_buffer().createLiteralInt(result.value(), type_id))
		};
	}



	//////////////////////////////////////////////////////////////////////
	// implementations

	template<class OP>
	auto ComptimeIntrins::logical_impl(evo::ArrayProxy<ASG::Expr> args, OP&& op) -> evo::SmallVector<ASG::Expr> {
		const ASGBuffer& asg_buffer = this->analyzer.source.getASGBuffer();

		if(args[0].kind() == ASG::Expr::Kind::LiteralInt){
			const ASG::LiteralInt& lhs_literal_int = 
				asg_buffer.getLiteralInt(args[0].literalIntID());
			const TypeInfo::ID type_id = *lhs_literal_int.typeID;

			const bool result = op(
				type_id, lhs_literal_int.value, asg_buffer.getLiteralInt(args[1].literalIntID()).value
			);

			return evo::SmallVector<ASG::Expr>{ASG::Expr(this->analyzer.get_asg_buffer().createLiteralBool(result))};

		}else{
			evo::debugAssert(args[0].kind() == ASG::Expr::Kind::LiteralFloat, "Unknown or unsupported comptime value");

			const ASG::LiteralFloat& lhs_literal_float = 
				asg_buffer.getLiteralFloat(args[0].literalFloatID());
			const TypeInfo::ID type_id = *lhs_literal_float.typeID;

			const bool result = op(
				type_id, lhs_literal_float.value, asg_buffer.getLiteralFloat(args[1].literalFloatID()).value
			);

			return evo::SmallVector<ASG::Expr>{ASG::Expr(this->analyzer.get_asg_buffer().createLiteralBool(result))};
		}
	}

}