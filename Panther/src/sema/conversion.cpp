////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../../include/sema/conversion.hpp"

#include "../../include/Context.hpp"


namespace pcit::panther::sema{


	auto exprToGenericValue(Expr expr, const Context& context) -> core::GenericValue {
		switch(expr.kind()){
			case sema::Expr::Kind::INT_VALUE: {
				return core::GenericValue(context.getSemaBuffer().getIntValue(expr.intValueID()).value);
			} break;

			case sema::Expr::Kind::FLOAT_VALUE: {
				return core::GenericValue(context.getSemaBuffer().getFloatValue(expr.floatValueID()).value);
			} break;

			case sema::Expr::Kind::BOOL_VALUE: {
				return core::GenericValue(context.getSemaBuffer().getBoolValue(expr.boolValueID()).value);
			} break;

			case sema::Expr::Kind::STRING_VALUE: {
				return core::GenericValue(
					std::string_view(context.getSemaBuffer().getStringValue(expr.stringValueID()).value)
				);
			} break;

			case sema::Expr::Kind::AGGREGATE_VALUE: {
				const sema::AggregateValue& aggregate_value =
					context.getSemaBuffer().getAggregateValue(expr.aggregateValueID());

				const size_t output_size = context.getTypeManager().numBytes(aggregate_value.typeID);
				core::GenericValue output = core::GenericValue::createUninit(output_size);

				if(aggregate_value.typeID.kind() == BaseType::Kind::STRUCT){
					const BaseType::Struct& struct_type = 
						context.getTypeManager().getStruct(aggregate_value.typeID.structID());

					size_t offset = 0;

					for(size_t i = 0; const BaseType::Struct::MemberVar* member : struct_type.memberVarsABI){
						const core::GenericValue member_val = exprToGenericValue(aggregate_value.values[i], context);

						const size_t member_size = context.getTypeManager().numBytes(member->typeID);

						std::memcpy(&output.writableDataRange()[offset], member_val.dataRange().data(), member_size);

						offset += member_size;

						i += 1;
					}

				}else if(aggregate_value.typeID.kind() == BaseType::Kind::ARRAY){
					const BaseType::Array& array_type = 
						context.getTypeManager().getArray(aggregate_value.typeID.arrayID());

					const size_t elem_size = context.getTypeManager().numBytes(array_type.elementTypeID);

					const size_t num_elems = output_size / elem_size;
					for(size_t i = 0; i < num_elems; i+=1){
						const core::GenericValue elem_val = exprToGenericValue(aggregate_value.values[i], context);

						std::memcpy(&output.writableDataRange()[i * elem_size], elem_val.dataRange().data(), elem_size);
					}

				}else{
					evo::debugAssert(aggregate_value.typeID.kind() == BaseType::Kind::UNION, "Unknown aggregate type");

					const size_t num_elems = context.getTypeManager().numBytes(aggregate_value.typeID);
					for(size_t i = 0; i < num_elems; i+=1){
						const core::GenericValue elem_val = exprToGenericValue(aggregate_value.values[i], context);

						output.writableDataRange()[i] = *elem_val.dataRange().data();
					}
				}

				return output;
			} break;

			case sema::Expr::Kind::CHAR_VALUE: {
				return core::GenericValue(context.getSemaBuffer().getCharValue(expr.charValueID()).value);
			} break;

			case sema::Expr::Kind::GLOBAL_VAR: {
				const sema::GlobalVar& global_var = context.getSemaBuffer().getGlobalVar(expr.globalVarID());
				return exprToGenericValue(*global_var.expr.load(), context);
			} break;

			case sema::Expr::Kind::DEFAULT_NEW: {
				const sema::DefaultNew& default_new_expr =
					context.getSemaBuffer().getDefaultNew(expr.defaultNewID());

				const size_t num_bytes = context.getTypeManager().numBytes(default_new_expr.targetTypeID);

				return core::GenericValue::createZeroinit(num_bytes);
			} break;

			default: evo::debugFatalBreak("Invalid comptime value");
		}
	}


	auto extractStringFromExpr(Expr expr, const Context& context) -> std::string_view {
		switch(expr.kind()){
			case sema::Expr::Kind::STRING_VALUE: {
				return context.getSemaBuffer().getStringValue(expr.stringValueID()).value;
			} break;

			case sema::Expr::Kind::INIT_ARRAY_REF: {
				const sema::InitArrayRef& init_array_ref = 
					context.getSemaBuffer().getInitArrayRef(expr.initArrayRefID());

				return extractStringFromExpr(init_array_ref.expr, context);
			} break;

			case sema::Expr::Kind::GLOBAL_VAR: {
				const sema::GlobalVar& global_var = context.getSemaBuffer().getGlobalVar(expr.globalVarID());

				return extractStringFromExpr(*global_var.expr.load(std::memory_order::relaxed), context);
			} break;

			default: {
				evo::debugFatalBreak("Not a string value");
			} break;
		}
	}


}