////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./MacroParser.h"

#include "../include/API.h"

namespace pcit::clangint{


	auto MacroParser::parse() -> MacroExpr {
		const evo::Result<MacroExpr> expr = this->parse_expr();

		if(this->at_end() == false){ return MacroExprBuffer::createNone(); }

		if(expr.isSuccess()){ return expr.value(); }
		return MacroExprBuffer::createNone();
	}



	auto MacroParser::parse_expr() -> evo::Result<MacroExpr> {
		if(this->at_end()){ return evo::resultError; }

		switch(this->peek().kind()){
			case MacroToken::Kind::IDENT: {
				return this->api.macroExprBuffer.createIdent(std::string(this->next().getString()));
			} break;

			case MacroToken::Kind::LITERAL_INT: {
				const MacroToken::IntValue& int_value = this->next().getInt();

				const std::optional<Type> type = [&]() -> std::optional<Type> {
					switch(int_value.type){
						case MacroToken::IntValue::Type::FLUID: {
							return std::nullopt;
						} break;

						case MacroToken::IntValue::Type::C_UINT: {
							return Type(BaseType::Primitive::C_UINT, evo::SmallVector<Type::Qualifier>(), false);
						} break;

						case MacroToken::IntValue::Type::C_LONG: {
							return Type(BaseType::Primitive::C_LONG, evo::SmallVector<Type::Qualifier>(), false);
						} break;

						case MacroToken::IntValue::Type::C_ULONG: {
							return Type(BaseType::Primitive::C_LONG, evo::SmallVector<Type::Qualifier>(), false);
						} break;

						case MacroToken::IntValue::Type::C_LONG_LONG: {
							return Type(BaseType::Primitive::C_LONG_LONG, evo::SmallVector<Type::Qualifier>(), false);
						} break;

						case MacroToken::IntValue::Type::C_ULONG_LONG: {
							return Type(BaseType::Primitive::C_ULONG_LONG, evo::SmallVector<Type::Qualifier>(), false);
						} break;

						case MacroToken::IntValue::Type::ISIZE: {
							return Type(BaseType::Primitive::ISIZE, evo::SmallVector<Type::Qualifier>(), false);
						} break;

						case MacroToken::IntValue::Type::USIZE: {
							return Type(BaseType::Primitive::USIZE, evo::SmallVector<Type::Qualifier>(), false);
						} break;
					}

					evo::debugFatalBreak("Unknown type");
				}();

				return this->api.macroExprBuffer.createInteger(int_value.value, type);
			} break;

			case MacroToken::Kind::LITERAL_FLOAT: {
				const MacroToken::FloatValue& float_value = this->next().getFloat();

				const std::optional<Type> type = [&]() -> std::optional<Type> {
					switch(float_value.type){
						case MacroToken::FloatValue::Type::FLUID: {
							return std::nullopt;
						} break;

						case MacroToken::FloatValue::Type::F16: {
							return Type(BaseType::Primitive::F16, evo::SmallVector<Type::Qualifier>(), false);
						} break;

						case MacroToken::FloatValue::Type::BF16: {
							return Type(BaseType::Primitive::BF16, evo::SmallVector<Type::Qualifier>(), false);
						} break;

						case MacroToken::FloatValue::Type::F32: {
							return Type(BaseType::Primitive::F32, evo::SmallVector<Type::Qualifier>(), false);
						} break;

						case MacroToken::FloatValue::Type::F64: {
							return Type(BaseType::Primitive::F64, evo::SmallVector<Type::Qualifier>(), false);
						} break;

						// case MacroToken::FloatValue::Type::F128: {
						// 	return Type(BaseType::Primitive::F128, evo::SmallVector<Type::Qualifier>(), false);
						// } break;

						case MacroToken::FloatValue::Type::C_LONG_DOUBLE: {
							return Type(BaseType::Primitive::C_LONG_DOUBLE, evo::SmallVector<Type::Qualifier>(), false);
						} break;
					}

					evo::debugFatalBreak("Unknown type");
				}();

				return this->api.macroExprBuffer.createFloat(float_value.value, type);
			} break;

			case MacroToken::Kind::LITERAL_BOOL: {
				return MacroExprBuffer::createBool(this->next().getBool());
			} break;

			case MacroToken::lookupKind("("): {
				this->skip();

				const evo::Result<MacroExpr> inner_value = this->parse_expr();
				if(inner_value.isError()){ return evo::resultError; }

				if(this->skip_token(MacroToken::lookupKind(")")).isError()){ return evo::resultError; }

				return inner_value.value();
			} break;

			case MacroToken::lookupKind("{"): {
				this->skip();

				const evo::Result<MacroExpr> inner_value = this->parse_expr();
				if(inner_value.isError()){ return evo::resultError; }

				if(this->skip_token(MacroToken::lookupKind("}")).isError()){ return evo::resultError; }

				return inner_value.value();
			} break;

			default: return evo::resultError;
		}
	}

	
}