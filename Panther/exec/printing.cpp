////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./printing.h"


#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif


namespace pthr{

	//////////////////////////////////////////////////////////////////////
	// helpers

	static auto print_file_header(
		core::Printer& printer, const panther::Source& source, const fs::path& relative_dir, std::string_view title
	) -> void {
		printer.printlnGray("------------------------------");
		printer.printlnCyan("{}: \"{}\"", title, fs::relative(source.getPath(), relative_dir).string());
	}



	auto print_logo(core::Printer& printer) -> void {
		// https://www.asciiart.eu/text-to-ascii-art
		// modified from the `Slant` font with the `Fitted` horizontal layout

		printer.printlnCyan(R"(            __   __         
    ____   / /_ / /_   _____
   / __ \ / __// __ \ / ___/
  / /_/ // /_ / / / // /     (Panther Compiler)
 / ____/ \__//_/ /_//_/     
/_/)");

		printer.printlnGray("-----------------------------------------------");

	}



	class Indenter{
		public:
			Indenter(core::Printer& _printer) : printer(_printer) {}
			~Indenter() = default;


			auto push() -> void {
				this->indents.emplace_back(IndenterType::END_ARROW);
			}

			auto pop() -> void {
				this->indents.pop_back();
			}

			auto set_arrow() -> void {
				this->indents.back() = IndenterType::ARROW;
			}

			auto set_end() -> void {
				this->indents.back() = IndenterType::END_ARROW;		
			}


			auto print() -> void {
				auto print_string = std::string{};

				for(const IndenterType& indent : this->indents){
					switch(indent){
						break; case IndenterType::LINE:      print_string += "|   ";
						break; case IndenterType::ARROW:     print_string += "|-> ";
						break; case IndenterType::END_ARROW: print_string += "\\-> ";
						break; case IndenterType::NONE:      print_string += "    ";
					}
				}

				this->printer.printGray(print_string);

				if(this->indents.empty() == false){
					if(this->indents.back() == IndenterType::ARROW){
					    this->indents.back() = IndenterType::LINE; 

					}else if(this->indents.back() == IndenterType::END_ARROW){
						this->indents.back() = IndenterType::NONE;
					}
				}
			}


			auto print_arrow() -> void {
				this->set_arrow();
				this->print();
			}

			auto print_end() -> void {
				this->set_end();
				this->print();
			}

	
		private:
			core::Printer& printer;

			enum class IndenterType : uint8_t {
				LINE,
				ARROW,
				END_ARROW,
				NONE,
			};
			evo::SmallVector<IndenterType> indents{};
	};



	//////////////////////////////////////////////////////////////////////
	// tokens

	auto print_tokens(core::Printer& printer, const panther::Source& source, const fs::path& relative_dir) -> void {
		const panther::TokenBuffer& token_buffer = source.getTokenBuffer();

		///////////////////////////////////
		// print header

		print_file_header(printer, source, relative_dir, "Tokens");

		if(token_buffer.size() == 0){
			printer.printlnGray("(NONE)");

		}else if(token_buffer.size() == 1){
			printer.printlnGray("(1 token)");

		}else{
			printer.printlnGray("({} tokens)", token_buffer.size());
		}


		///////////////////////////////////
		// create location strings

		auto location_strings = evo::SmallVector<std::string>();

		for(panther::Token::ID token_id : token_buffer){
			const panther::Token::Location& location = token_buffer.getLocation(token_id);

			location_strings.emplace_back(std::format("<{}:{}>", location.lineStart, location.collumnStart));
		}

		const size_t longest_location_string_length = std::max_element(
			location_strings.begin(), location_strings.end(),
			[](const std::string& lhs, const std::string& rhs) -> bool {
				return lhs.size() < rhs.size();
			}
		)->size();

		for(std::string& str : location_strings){
			while(str.size() < longest_location_string_length){
				str += ' ';
			}

			str += ' ';
		}


		///////////////////////////////////
		// print out tokens

		for(size_t i = 0; panther::Token::ID token_id : token_buffer){
			const panther::Token& token = token_buffer[token_id];
			
			printer.printGray(location_strings[i]);
			printer.printInfo("[{}]", token.kind());


			const std::string data_str = [&]() {
				switch(token.kind()){
					break; case panther::Token::Kind::IDENT:          return std::format(" {}", token.getString());
					break; case panther::Token::Kind::INTRINSIC:      return std::format(" @{}", token.getString());
					break; case panther::Token::Kind::ATTRIBUTE:      return std::format(" #{}", token.getString());
					break; case panther::Token::Kind::DEDUCER:        return std::format(" ${}", token.getString());

					break; case panther::Token::Kind::LITERAL_BOOL:   return std::format(" {}", token.getBool());
					break; case panther::Token::Kind::LITERAL_INT:    return std::format(" {}", token.getInt());
					break; case panther::Token::Kind::LITERAL_FLOAT:  return std::format(" {}", token.getFloat());
					break; case panther::Token::Kind::LITERAL_CHAR:   return std::format(" \'{}\'", token.getString());
					break; case panther::Token::Kind::LITERAL_STRING: return std::format(" \"{}\"", token.getString());

					break; case panther::Token::Kind::TYPE_I_N:       return std::format(" {}", token.getBitWidth());
					break; case panther::Token::Kind::TYPE_UI_N:      return std::format(" {}", token.getBitWidth());

					break; default: return std::string();
				}
			}();

			printer.printlnMagenta(data_str);

			i += 1;
		}
	}



	//////////////////////////////////////////////////////////////////////
	// ast


	class ASTPrinter{
		public:
			ASTPrinter(core::Printer& _printer, const panther::Source& _source, const fs::path& _relative_dir) 
				: printer(_printer),
				source(_source),
				relative_dir(_relative_dir),
				ast_buffer(this->source.getASTBuffer()),
				indenter(_printer)
				{}
			~ASTPrinter() = default;


			auto print_header() -> void {
				print_file_header(this->printer, this->source, this->relative_dir, "AST");

				if(ast_buffer.numGlobalStmts() == 0){
					this->printer.printGray("(NONE)\n");

				}else if(ast_buffer.numGlobalStmts() == 1){
					this->printer.printGray("(1 global statement)\n");

				}else{
					this->printer.printGray("({} global statements)\n", ast_buffer.numGlobalStmts());
				}
			}

			auto print_globals() -> void {
				for(const panther::AST::Node& global_stmt : ast_buffer.getGlobalStmts()){
					this->print_stmt(global_stmt);
				}
			}


		private:
			auto print_stmt(const panther::AST::Node& stmt) -> void {
				switch(stmt.kind()){
					case panther::AST::Kind::VAR_DEF: {
						this->print_var_def(this->ast_buffer.getVarDef(stmt));
					} break;

					case panther::AST::Kind::FUNC_DEF: {
						this->print_func_def(this->ast_buffer.getFuncDef(stmt));
					} break;

					case panther::AST::Kind::DELETED_SPECIAL_METHOD: {
						this->print_deleted_special_method(this->ast_buffer.getDeletedSpecialMethod(stmt));
					} break;

					case panther::AST::Kind::ALIAS_DEF: {
						this->print_alias_def(this->ast_buffer.getAliasDef(stmt));
					} break;

					case panther::AST::Kind::DISTINCT_ALIAS_DEF: {
						this->print_distinct_alias(this->ast_buffer.getDistinctAliasDef(stmt));
					} break;

					case panther::AST::Kind::STRUCT_DEF: {
						this->print_struct_def(this->ast_buffer.getStructDef(stmt));
					} break;

					case panther::AST::Kind::UNION_DEF: {
						this->print_union_def(this->ast_buffer.getUnionDef(stmt));
					} break;

					case panther::AST::Kind::ENUM_DEF: {
						this->print_enum_def(this->ast_buffer.getEnumDef(stmt));
					} break;

					case panther::AST::Kind::INTERFACE_DEF: {
						this->print_interface_def(this->ast_buffer.getInterfaceDef(stmt));
					} break;

					case panther::AST::Kind::INTERFACE_IMPL: {
						this->print_interface_impl(this->ast_buffer.getInterfaceImpl(stmt));
					} break;

					case panther::AST::Kind::RETURN: {
						this->print_return(this->ast_buffer.getReturn(stmt));
					} break;

					case panther::AST::Kind::ERROR: {
						this->print_error(this->ast_buffer.getError(stmt));
					} break;

					case panther::AST::Kind::BREAK: {
						this->print_break(this->ast_buffer.getBreak(stmt));
					} break;

					case panther::AST::Kind::CONTINUE: {
						this->print_continue(this->ast_buffer.getContinue(stmt));
					} break;

					case panther::AST::Kind::DELETE: {
						this->print_delete(this->ast_buffer.getDelete(stmt));
					} break;

					case panther::AST::Kind::CONDITIONAL: {
						this->print_conditional(this->ast_buffer.getConditional(stmt));
					} break;

					case panther::AST::Kind::WHEN_CONDITIONAL: {
						this->print_conditional(this->ast_buffer.getWhenConditional(stmt));
					} break;

					case panther::AST::Kind::WHILE: {
						this->print_while(this->ast_buffer.getWhile(stmt));
					} break;

					case panther::AST::Kind::DEFER: {
						this->print_defer(this->ast_buffer.getDefer(stmt));
					} break;

					case panther::AST::Kind::INFIX: {
						this->print_assignment(this->ast_buffer.getInfix(stmt));
					} break;

					case panther::AST::Kind::MULTI_ASSIGN: {
						this->print_multi_assign(this->ast_buffer.getMultiAssign(stmt));
					} break;

					case panther::AST::Kind::FUNC_CALL: {
						this->indenter.print();
						this->print_func_call(this->ast_buffer.getFuncCall(stmt));
					} break;

					case panther::AST::Kind::TRY_ELSE: {
						this->indenter.print();
						this->print_try_else(this->ast_buffer.getTryElse(stmt));
					} break;

					case panther::AST::Kind::BLOCK: {
						this->indenter.print();
						this->print_major_header("Statement Block");
						this->print_block(this->ast_buffer.getBlock(stmt), false);
					} break;

					default: {
						evo::debugFatalBreak(
							"Unknown or unsupported statement kind ({})", evo::to_underlying(stmt.kind())
						);
					} break;
				}
			}

			auto print_var_def(const panther::AST::VarDef& var_def) -> void {
				this->indenter.print();
				this->print_major_header("Variable Definition");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Kind");
					switch(var_def.kind){
						break; case panther::AST::VarDef::Kind::VAR:   this->printer.printMagenta(" var\n");
						break; case panther::AST::VarDef::Kind::CONST: this->printer.printMagenta(" const\n");
						break; case panther::AST::VarDef::Kind::DEF:   this->printer.printMagenta(" def\n");
					}

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(var_def.ident);

					this->indenter.print_arrow();
					this->print_minor_header("Type");
					if(var_def.type.has_value()){
						this->printer.print(" ");
						this->print_type(this->ast_buffer.getType(*var_def.type));
						this->printer.println();
					}else{
						this->printer.printGray(" {INFERRED}\n");
					}

					this->indenter.set_arrow();
					this->print_attribute_block(this->ast_buffer.getAttributeBlock(var_def.attributeBlock));

					this->indenter.print_end();
					this->print_minor_header("Value");
					if(var_def.value.has_value()){
						this->printer.println();
						this->indenter.push();
						this->print_expr(*var_def.value);
						this->indenter.pop();
					}else{
						this->printer.printGray(" {NONE}\n");
					}
					
					this->indenter.pop();
				}
			}


			auto print_func_def(const panther::AST::FuncDef& func_def) -> void {
				this->indenter.print();
				this->print_major_header("Function Definition");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					const panther::Token::Kind name_kind = this->source.getTokenBuffer()[func_def.name].kind();
					if(name_kind == panther::Token::Kind::IDENT){
						this->print_ident(func_def.name);
					}else{
						this->printer.printMagenta("{}\n", name_kind);
					}

					this->print_template_pack(func_def.templatePack);

					this->indenter.print_arrow();
					this->print_minor_header("Parameters");
					if(func_def.params.empty()){
						this->printer.printGray(" {NONE}\n");
					}else{
						this->printer.println();
						this->indenter.push();
						for(size_t i = 0; const panther::AST::FuncDef::Param& param : func_def.params){
							if(i + 1 < func_def.params.size()){
								this->indenter.print_arrow();
							}else{
								this->indenter.print_end();
							}

							this->print_major_header(std::format("Parameter {}", i));

							{
								this->indenter.push();

								if(param.name.kind() == panther::AST::Kind::IDENT){
									this->indenter.print_arrow();
									this->print_minor_header("Identifier");
									this->printer.print(" ");
									this->print_ident(param.name);

									this->indenter.print_arrow();
									this->print_minor_header("Type");
									this->printer.print(" ");
									this->print_type(this->ast_buffer.getType(*param.type));
									this->printer.println();

								}else{
									this->indenter.print_arrow();
									this->print_minor_header("Identifier");
									this->printer.printMagenta(" {this}\n");
								}

								this->indenter.print_arrow();
								this->print_minor_header("Kind");
								using ParamKind = panther::AST::FuncDef::Param::Kind;
								switch(param.kind){
									break; case ParamKind::READ: this->printer.printMagenta(" {read}\n");
									break; case ParamKind::MUT: this->printer.printMagenta(" {mut}\n");
									break; case ParamKind::IN: this->printer.printMagenta(" {in}\n");
								}

								this->indenter.set_arrow();
								this->print_attribute_block(this->ast_buffer.getAttributeBlock(param.attributeBlock));

								this->indenter.print_end();
								this->print_minor_header("Default Value");
								if(param.defaultValue.has_value()){
									this->printer.println();
									this->indenter.push();
									this->print_expr(*param.defaultValue);
									this->indenter.pop();
								}else{
									this->printer.printGray(" {NONE}\n");
								}

								this->indenter.pop();
							}

							i += 1;
						}
						this->indenter.pop();
					}

					this->indenter.set_arrow();
					this->print_attribute_block(this->ast_buffer.getAttributeBlock(func_def.attributeBlock));

					this->indenter.print_arrow();
					this->print_minor_header("Returns");
					this->printer.println();
					{
						this->indenter.push();
						for(size_t i = 0; const panther::AST::FuncDef::Return& return_param : func_def.returns){
							if(i + 1 < func_def.returns.size()){
								this->indenter.print_arrow();
							}else{
								this->indenter.print_end();
							}

							this->print_major_header(std::format("Return Parameter {}", i));

							{
								this->indenter.push();

								this->indenter.print_arrow();
								this->print_minor_header("Identifier");
								this->printer.print(" ");
								if(return_param.ident.has_value()){
									this->print_ident(*return_param.ident);
								}else{
									this->printer.printGray("{NONE}\n");
								}

								this->indenter.print_end();
								this->print_minor_header("Type");
								this->printer.print(" ");
								this->print_type(this->ast_buffer.getType(return_param.type));
								this->printer.println();

								this->indenter.pop();
							}

							i += 1;
						}
						this->indenter.pop();
					}

					this->indenter.print_arrow();
					this->print_minor_header("Error Returns");
					if(func_def.errorReturns.empty()){
						this->printer.printGray(" {NONE}\n");
					}else{
						this->printer.println();
						this->indenter.push();
						for(size_t i = 0; const panther::AST::FuncDef::Return& error_return : func_def.errorReturns){
							if(i + 1 < func_def.errorReturns.size()){
								this->indenter.print_arrow();
							}else{
								this->indenter.print_end();
							}

							this->print_major_header(std::format("Error Return Parameter {}", i));

							{
								this->indenter.push();

								this->indenter.print_arrow();
								this->print_minor_header("Identifier");
								this->printer.print(" ");
								if(error_return.ident.has_value()){
									this->print_ident(*error_return.ident);
								}else{
									this->printer.printGray("{NONE}\n");
								}

								this->indenter.print_end();
								this->print_minor_header("Type");
								this->printer.print(" ");
								this->print_type(this->ast_buffer.getType(error_return.type));
								this->printer.println();

								this->indenter.pop();
							}

							i += 1;
						}
						this->indenter.pop();
					}

					this->indenter.print_end();
					this->print_minor_header("Statement Block");

					if(func_def.block.has_value()){
						this->print_block(this->ast_buffer.getBlock(*func_def.block));
					}else{
						this->printer.printGray("{NONE}\n");
					}

					this->indenter.pop();
				}
			}


			auto print_deleted_special_method(const panther::AST::DeletedSpecialMethod& deleted_special_method)
			-> void {
				this->indenter.print();
				this->print_major_header("Deleted Special Member");

				{
					this->indenter.push();

					this->indenter.print_end();
					this->printer.printMagenta(
						"{}\n", this->source.getTokenBuffer()[deleted_special_method.memberToken].kind()
					);

					this->indenter.pop();
				}
			}



			auto print_alias_def(const panther::AST::AliasDef& alias_def) -> void {
				this->indenter.print();
				this->print_major_header("Alias Definition");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(alias_def.ident);

					this->indenter.set_arrow();
					this->print_attribute_block(this->ast_buffer.getAttributeBlock(alias_def.attributeBlock));

					this->indenter.print_end();
					this->print_minor_header("Type");
					this->printer.print(" ");
					this->print_type(this->ast_buffer.getType(alias_def.type));
					this->printer.println();

					this->indenter.pop();
				}
			}

			auto print_distinct_alias(const panther::AST::DistinctAliasDef& distinct_alias) -> void {
				this->indenter.print();
				this->print_major_header("Type Definition");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(distinct_alias.ident);

					this->indenter.set_arrow();
					this->print_attribute_block(this->ast_buffer.getAttributeBlock(distinct_alias.attributeBlock));

					this->indenter.print_end();
					this->print_minor_header("Type");
					this->printer.print(" ");
					this->print_type(this->ast_buffer.getType(distinct_alias.type));
					this->printer.println();

					this->indenter.pop();
				}
			}


			auto print_struct_def(const panther::AST::StructDef& struct_def) -> void {
				this->indenter.print();
				this->print_major_header("Struct Definition");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(struct_def.ident);

					this->indenter.print_arrow();
					this->print_template_pack(struct_def.templatePack);

					this->indenter.set_arrow();
					this->print_attribute_block(this->ast_buffer.getAttributeBlock(struct_def.attributeBlock));

					this->indenter.print_end();
					this->print_minor_header("Statement Block");
					this->print_block(this->ast_buffer.getBlock(struct_def.block));

					this->indenter.pop();
				}
			}


			auto print_union_def(const panther::AST::UnionDef& union_def) -> void {
				this->indenter.print();
				this->print_major_header("Union Definition");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(union_def.ident);

					this->indenter.set_arrow();
					this->print_attribute_block(this->ast_buffer.getAttributeBlock(union_def.attributeBlock));

					this->indenter.print_arrow();
					this->print_minor_header("Fields");
					this->printer.println();
					this->indenter.push();
					for(size_t i = 0; const panther::AST::UnionDef::Field& field : union_def.fields){
						if(i + 1 < union_def.fields.size()){
							this->indenter.print_arrow();
						}else{
							this->indenter.print_end();
						}

						this->print_major_header(std::format("Field {}", i));
						{
							this->indenter.push();

							this->indenter.print_arrow();
							this->print_minor_header("Identifier");
							this->printer.print(" ");
							this->print_ident(field.ident);

							this->indenter.print_end();
							this->print_minor_header("Type");
							this->printer.print(" ");
							this->print_type(this->ast_buffer.getType(field.type));
							this->printer.println();

							this->indenter.pop();
						}
					
						i += 1;
					}
					this->indenter.pop();

					this->indenter.print_end();
					this->print_minor_header("Statements");
					if(union_def.statements.empty()){
						this->printer.printlnGray(" {NONE}");
					}else{
						this->printer.println();

						this->indenter.push();

						for(size_t i = 0; const panther::AST::Node& stmt : union_def.statements){
							if(i + 1 < union_def.statements.size()){
								this->indenter.set_arrow();
							}else{
								this->indenter.set_end();
							}

							this->print_stmt(stmt);
						
							i += 1;
						}

						this->indenter.pop();
					}

					this->indenter.pop();
				}
			}


			auto print_enum_def(const panther::AST::EnumDef& enum_def) -> void {
				this->indenter.print();
				this->print_major_header("Enum Definition");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(enum_def.ident);

					this->indenter.print_arrow();
					this->print_minor_header("Underlying Type");
					this->printer.print(" ");
					if(enum_def.underlyingType.has_value()){
						this->print_type(this->ast_buffer.getType(*enum_def.underlyingType));
						this->printer.println();
					}else{
						this->printer.printlnGray("{DEFAULT}");
					}

					this->indenter.set_arrow();
					this->print_attribute_block(this->ast_buffer.getAttributeBlock(enum_def.attributeBlock));

					this->indenter.print_arrow();
					this->print_minor_header("Enumerators");
					this->printer.println();
					this->indenter.push();
					for(size_t i = 0; const panther::AST::EnumDef::Enumerator& enumerator : enum_def.enumerators){
						if(i + 1 < enum_def.enumerators.size()){
							this->indenter.print_arrow();
						}else{
							this->indenter.print_end();
						}

						this->print_major_header(std::format("Enumerator {}", i));
						{
							this->indenter.push();

							this->indenter.print_arrow();
							this->print_minor_header("Identifier");
							this->printer.print(" ");
							this->print_ident(enumerator.ident);

							this->indenter.print_end();
							this->print_minor_header("Value");
							if(enumerator.value.has_value()){
								this->printer.println();
								this->indenter.push();
								this->print_expr(*enumerator.value);
								this->indenter.pop();
							}else{
								this->printer.printlnGray(" {NONE}");
							}


							this->indenter.pop();
						}
					
						i += 1;
					}
					this->indenter.pop();

					this->indenter.print_end();
					this->print_minor_header("Statements");
					if(enum_def.statements.empty()){
						this->printer.printlnGray(" {NONE}");
					}else{
						this->printer.println();

						this->indenter.push();

						for(size_t i = 0; const panther::AST::Node& stmt : enum_def.statements){
							if(i + 1 < enum_def.statements.size()){
								this->indenter.set_arrow();
							}else{
								this->indenter.set_end();
							}

							this->print_stmt(stmt);
						
							i += 1;
						}

						this->indenter.pop();
					}

					this->indenter.pop();
				}
			}



			auto print_interface_def(const panther::AST::InterfaceDef& interface_def) -> void {
				this->indenter.print();
				this->print_major_header("Interface Definition");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(interface_def.ident);


					this->indenter.print_end();
					this->print_minor_header("Methods");
					{
						this->indenter.push();

						this->printer.println();

						for(size_t i = 0; const panther::AST::Node& func : interface_def.methods){
							if(i + 1 < interface_def.methods.size()){
								this->indenter.set_arrow();
							}else{
								this->indenter.set_end();
							}

							this->print_func_def(this->ast_buffer.getFuncDef(func));

							i += 1;
						}

						this->indenter.pop();
					}
					this->printer.println();

					this->indenter.pop();
				}
			}



			auto print_interface_impl(const panther::AST::InterfaceImpl& interface_impl) -> void {
				this->indenter.print();
				this->print_major_header("Interface Impl");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Target");
					this->printer.print(" ");
					this->print_type(this->ast_buffer.getType(interface_impl.target));
					this->printer.println();

					this->indenter.print_end();
					this->print_minor_header("Methods");
					this->printer.println();
					{
						this->indenter.push();

						for(size_t i = 0; const panther::AST::InterfaceImpl::Method& method : interface_impl.methods){
							if(i + 1 < interface_impl.methods.size()){
								this->indenter.print_arrow();
							}else{
								this->indenter.print_end();
							}


							this->print_major_header(std::format("Method {}", i));

							{
								this->indenter.push();

								this->indenter.print_arrow();
								this->print_minor_header("Name");
								this->printer.print(" ");
								this->print_ident(method.method);

								this->indenter.print_end();
								this->print_minor_header("Target");
								this->printer.print(" ");
								this->print_ident(method.value);

								this->indenter.pop();
							}


							
							i += 1;
						}

						this->indenter.pop();
					}


					this->indenter.pop();
				}
			}


			auto print_return(const panther::AST::Return& ret) -> void {
				this->indenter.print();
				this->print_major_header("Return");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Label");
					if(ret.label.has_value()){
						this->printer.print(" ");
						this->print_ident(*ret.label);
					}else{
						this->printer.printlnGray(" {NONE}");
					}

					this->indenter.print_end();
					this->print_minor_header("Value");

					ret.value.visit([&](auto value) -> void {
						using ValueT = std::decay_t<decltype(value)>;

						if constexpr(std::is_same<ValueT, std::monostate>()){
							this->printer.printlnGray(" {NONE}");

						}else if constexpr(std::is_same<ValueT, panther::AST::Node>()){
							this->indenter.push();
							this->printer.println();
							this->print_expr(value);
							this->indenter.pop();

						}else if constexpr(std::is_same<ValueT, panther::Token::ID>()){
							this->printer.printlnGray(" {...}");

						}else{
							static_assert(sizeof(ValueT) < 0, "Unknown or unsupported return value kind");
						}
					});


					this->indenter.pop();
				}
			}


			auto print_error(const panther::AST::Error& err) -> void {
				this->indenter.print();
				this->print_major_header("Error");

				{
					this->indenter.push();

					this->indenter.print_end();
					this->print_minor_header("Value");

					err.value.visit([&](auto value) -> void {
						using ValueT = std::decay_t<decltype(value)>;

						if constexpr(std::is_same<ValueT, std::monostate>()){
							this->printer.printlnGray(" {NONE}");

						}else if constexpr(std::is_same<ValueT, panther::AST::Node>()){
							this->indenter.push();
							this->printer.println();
							this->print_expr(value);
							this->indenter.pop();

						}else if constexpr(std::is_same<ValueT, panther::Token::ID>()){
							this->printer.printlnGray(" {...}");

						}else{
							static_assert(sizeof(ValueT) < 0, "Unknown or unsupported error value kind");
						}
					});


					this->indenter.pop();
				}
			}


			auto print_break(const panther::AST::Break& break_stmt) -> void {
				this->indenter.print();
				this->print_major_header("Break");

				{
					this->indenter.push();

					this->indenter.print_end();
					this->print_minor_header("Label");
					if(break_stmt.label.has_value()){
						this->printer.print(" ");
						this->print_ident(*break_stmt.label);
					}else{
						this->printer.printlnGray(" {NONE}");
					}

					this->indenter.pop();
				}
			}


			auto print_continue(const panther::AST::Continue& continue_stmt) -> void {
				this->indenter.print();
				this->print_major_header("Continue");

				{
					this->indenter.push();

					this->indenter.print_end();
					this->print_minor_header("Label");
					if(continue_stmt.label.has_value()){
						this->printer.print(" ");
						this->print_ident(*continue_stmt.label);
					}else{
						this->printer.printlnGray(" {NONE}");
					}

					this->indenter.pop();
				}
			}


			auto print_delete(const panther::AST::Delete& delete_stmt) -> void {
				this->indenter.print();
				this->print_major_header("Delete");

				{
					this->indenter.push();

					this->indenter.print_end();
					this->print_minor_header("Value");
					this->printer.println();
					this->indenter.push();
					this->print_expr(delete_stmt.value);
					this->indenter.pop();

					this->indenter.pop();
				}
			}


			auto print_conditional(const panther::AST::Conditional& conditional) -> void {
				this->print_conditional_impl<false>(conditional.cond, conditional.thenBlock, conditional.elseBlock);
			}

			auto print_conditional(const panther::AST::WhenConditional& conditional) -> void {
				this->print_conditional_impl<true>(conditional.cond, conditional.thenBlock, conditional.elseBlock);
			}

			template<bool IS_WHEN>
			auto print_conditional_impl(
				const panther::AST::Node& cond,
				const panther::AST::Node& then_block,
				const std::optional<panther::AST::Node>& else_block
			) -> void {
				this->indenter.print();
				if constexpr(IS_WHEN){
					this->print_major_header("When Conditional");
				}else{
					this->print_major_header("Conditional");
				}

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Condition");
					this->indenter.push();
					this->printer.println();
					this->print_expr(cond);
					this->indenter.pop();

					this->indenter.print_arrow();
					this->print_minor_header("Then");
					this->print_block(this->source.getASTBuffer().getBlock(then_block));

					this->indenter.print_end();
					if(else_block.has_value()){
						if(else_block->kind() == panther::AST::Kind::BLOCK){
							this->print_minor_header("Else");
							this->print_block(this->source.getASTBuffer().getBlock(*else_block));
						}else{
							this->print_minor_header("Else If");
							this->printer.println();
							this->indenter.push();
							if constexpr(IS_WHEN){
								this->print_conditional(this->source.getASTBuffer().getWhenConditional(*else_block));
							}else{
								this->print_conditional(this->source.getASTBuffer().getConditional(*else_block));
							}
							this->indenter.pop();
						}
					}else{
						this->print_minor_header("Else");
						this->printer.printlnGray(" {NONE}");
					}

					this->indenter.pop();
				}
			}

			auto print_while(const panther::AST::While& while_loop) -> void {
				this->indenter.print();
				this->print_major_header("While Loop");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Condition");
					this->indenter.push();
					this->printer.println();
					this->print_expr(while_loop.cond);
					this->indenter.pop();

					this->indenter.print_end();
					this->print_minor_header("Block");
					this->print_block(this->source.getASTBuffer().getBlock(while_loop.block));

					this->indenter.pop();
				}
			}


			auto print_defer(const panther::AST::Defer& defer_stmt) -> void {
				this->indenter.print();
				this->print_major_header("Defer");

				{
					this->indenter.push();

					this->indenter.print_end();
					this->print_minor_header("Block");
					this->print_block(this->source.getASTBuffer().getBlock(defer_stmt.block));

					this->indenter.pop();
				}
			}


			auto print_block(const panther::AST::Block& block, bool is_not_on_newline = true) -> void {
				if(is_not_on_newline){
					this->printer.println();
				}

				this->indenter.push();

				this->indenter.print_arrow();
				this->print_minor_header("Label");
				if(block.label.has_value()){
					this->printer.print(" ");
					this->print_ident(*block.label);

					this->indenter.print_arrow();
					this->print_minor_header("Outputs");
					if(block.outputs.empty()){
						this->printer.printlnGray(" {NONE}");
					}else{
						this->printer.println();
						this->indenter.push();
						for(size_t i = 0; const panther::AST::Block::Output& output : block.outputs){
							if(i + 1 < block.outputs.size()){
								this->indenter.print_arrow();
							}else{
								this->indenter.print_end();
							}

							this->print_major_header(std::format("Output Parameter {}", i));

							this->indenter.push();

							this->indenter.print_arrow();
							this->print_minor_header("Identifier");
							if(output.ident.has_value()){
								this->printer.print(" ");
								this->print_ident(*output.ident);
							}else{
								this->printer.printGray(" {NONE}\n");
							}

							this->indenter.print_end();
							this->print_minor_header("Type");
							this->printer.print(" ");
							this->print_type(this->source.getASTBuffer().getType(output.typeID));
							this->printer.println();

							this->indenter.pop();

							i += 1;
						}
						this->indenter.pop();
					}

				}else{
					this->printer.printGray(" {NONE}\n");
				}


				this->indenter.print_end();
				this->print_minor_header("Statements");

				if(block.statements.empty()){
					this->printer.printGray(" {EMPTY}\n");
					
				}else{
					this->printer.println();
					this->indenter.push();

					for(size_t i = 0; const panther::AST::Node& stmt : block.statements){
						if(i + 1 < block.statements.size()){
							this->indenter.set_arrow();
						}else{
							this->indenter.set_end();
						}

						this->print_stmt(stmt);

						i += 1;
					}

					this->indenter.pop();
				}


				this->indenter.pop();
			}



			auto print_base_type(const panther::AST::Node& base_type) -> void {
				switch(base_type.kind()){
					case panther::AST::Kind::PRIMITIVE_TYPE: {
						const panther::Token::ID type_token_id = this->ast_buffer.getPrimitiveType(base_type);
						const panther::Token& type_token = this->source.getTokenBuffer()[type_token_id];

						if(type_token.kind() == panther::Token::Kind::TYPE_I_N){
							this->printer.printMagenta("I{}", type_token.getBitWidth());
						}else if(type_token.kind() == panther::Token::Kind::TYPE_UI_N){
							this->printer.printMagenta("UI{}", type_token.getBitWidth());
						}else{
							this->printer.printMagenta("{}", type_token.kind());
						}
					} break;

					case panther::AST::Kind::IDENT: {
						const panther::Token::ID type_token_id = this->ast_buffer.getIdent(base_type);
						this->printer.printMagenta("{}", this->source.getTokenBuffer()[type_token_id].getString());
					} break;

					case panther::AST::Kind::INTRINSIC: {
						const panther::Token::ID type_token_id = this->ast_buffer.getIntrinsic(base_type);
						this->printer.printMagenta("@{}", this->source.getTokenBuffer()[type_token_id].getString());
					} break;

					case panther::AST::Kind::DEDUCER: {
						const panther::Token::ID type_token_id = this->ast_buffer.getDeducer(base_type);
						const panther::Token& type_token = this->source.getTokenBuffer()[type_token_id];

						if(type_token.kind() == panther::Token::Kind::DEDUCER){
							this->printer.printMagenta("${}", type_token.getString());
							
						}else{
							evo::debugAssert(
								type_token.kind() == panther::Token::Kind::ANONYMOUS_DEDUCER,
								"Unknown type deducer kind"
							);
							this->printer.printMagenta("$$");
						}

					} break;

					case panther::AST::Kind::INFIX: {
						const panther::AST::Infix& infix = this->ast_buffer.getInfix(base_type);
						this->print_base_type(infix.lhs);
						this->printer.printMagenta(".");
						this->print_base_type(infix.rhs);
					} break;


					// TODO(FUTURE): print this properly
					case panther::AST::Kind::TEMPLATED_EXPR: {
						const panther::AST::TemplatedExpr& templated_expr = 
							this->ast_buffer.getTemplatedExpr(base_type);
						this->print_base_type(templated_expr.base);
						this->printer.printMagenta("<{");

						for(size_t i = 0; const panther::AST::Node& arg : templated_expr.args){
							if(arg.kind() == panther::AST::Kind::TYPE){
								this->print_type(this->ast_buffer.getType(arg));
							}else{
								this->printer.printGray("{ARG}");
							}

							if(i + 1 < templated_expr.args.size()){
								this->printer.printMagenta(", ");
							}
						
							i += 1;
						}
						this->printer.printMagenta("}>");
					} break;

					// TODO(FUTURE): print this properly
					case panther::AST::Kind::ARRAY_TYPE: {
						const panther::AST::ArrayType& array_type = this->ast_buffer.getArrayType(base_type);

						this->printer.printMagenta("[");
						this->print_type(this->ast_buffer.getType(array_type.elemType));
						this->printer.printMagenta(":");

						for(size_t i = 0; const std::optional<panther::AST::Node>& dimension : array_type.dimensions){
							if(dimension.has_value()){
								this->printer.printGray("...expr...");
							}else{
								if(*array_type.refIsReadOnly){
									this->printer.printMagenta("*|");
								}else{
									this->printer.printMagenta("*");
								}
							}

							if(i + 1 < array_type.dimensions.size()){ this->printer.printMagenta(","); }
						
							i += 1;
						}

						if(array_type.terminator.has_value()){
							this->printer.printMagenta(";");
							this->printer.printGray("...expr...");
						}

						this->printer.printMagenta("]");
					} break;

					// TODO(FUTURE): print this properly
					case panther::AST::Kind::TYPEID_CONVERTER: {
						this->printer.printMagenta("Type(");
						this->printer.printGray("...expr...");
						this->printer.printMagenta(")");
					} break;

					default: evo::debugFatalBreak("Unknown or unsupported base type");
				}
			}

			
			auto print_type(const panther::AST::Type& type) -> void {
				this->print_base_type(type.base);

				if(type.qualifiers.empty() == false){
					auto qualifier_str = std::string();
					bool not_first_qualifier = false;
					for(const panther::AST::Type::Qualifier& qualifier : type.qualifiers){
						if(not_first_qualifier){
							qualifier_str += ' ';
						}else{
							not_first_qualifier = true;
						}

						if(qualifier.isPtr){ qualifier_str += '*'; }
						if(qualifier.isReadOnly){ qualifier_str += '|'; }
						if(qualifier.isUninit){ qualifier_str += '!'; }
						if(qualifier.isOptional){ qualifier_str += '?'; }
					}

					this->printer.printMagenta(qualifier_str);
				}

				if(type.base.kind() == panther::AST::Kind::PRIMITIVE_TYPE){
					this->printer.printGray(" {PRIMITIVE}");
				}
			}


			auto print_expr(const panther::AST::Node& node) -> void {
				this->indenter.print();

				switch(node.kind()){
					case panther::AST::Kind::PREFIX: {
						this->print_prefix(this->ast_buffer.getPrefix(node));
					} break;

					case panther::AST::Kind::INFIX: {
						this->print_infix(this->ast_buffer.getInfix(node));
					} break;

					case panther::AST::Kind::POSTFIX: {
						this->print_postfix(this->ast_buffer.getPostfix(node));
					} break;

					case panther::AST::Kind::FUNC_CALL: {
						this->print_func_call(this->ast_buffer.getFuncCall(node));
					} break;

					case panther::AST::Kind::INDEXER: {
						this->print_indexer(this->ast_buffer.getIndexer(node));
					} break;

					case panther::AST::Kind::TEMPLATED_EXPR: {
						this->print_templated_expr(this->ast_buffer.getTemplatedExpr(node));
					} break;

					case panther::AST::Kind::BLOCK: {
						this->print_major_header("Statement Block");
						this->print_block(this->ast_buffer.getBlock(node), false);
					} break;

					case panther::AST::Kind::TYPE: {
						this->print_type(this->ast_buffer.getType(node));
						this->printer.println();
					} break;

					case panther::AST::Kind::NEW: {
						this->print_new(this->ast_buffer.getNew(node));
					} break;

					case panther::AST::Kind::TRY_ELSE: {
						this->print_try_else(this->ast_buffer.getTryElse(node));
					} break;

					case panther::AST::Kind::LITERAL: {
						const panther::Token::ID token_id = this->ast_buffer.getLiteral(node);
						const panther::Token& token = this->source.getTokenBuffer()[token_id];

						switch(token.kind()){
							case panther::Token::Kind::LITERAL_INT: {
								this->printer.printMagenta(std::to_string(token.getInt()));
								this->printer.printGray(" {LITERAL_INT}");
							} break;

							case panther::Token::Kind::LITERAL_FLOAT: {
								this->printer.printMagenta(std::to_string(token.getFloat()));
								this->printer.printGray(" {LITERAL_FLOAT}");
							} break;

							case panther::Token::Kind::LITERAL_BOOL: {
								this->printer.printMagenta(evo::boolStr(token.getBool()));
								this->printer.printGray(" {LITERAL_BOOL}");
							} break;

							case panther::Token::Kind::LITERAL_STRING: {
								this->printer.printMagenta("\"{}\"", token.getString());
								this->printer.printGray(" {LITERAL_STRING}");
							} break;

							case panther::Token::Kind::LITERAL_CHAR: {
								const std::string_view str = token.getString();
								this->printer.printMagenta("'{}'", token.getString());
								this->printer.printGray(" {LITERAL_CHAR}");
							} break;

							case panther::Token::Kind::KEYWORD_NULL: {
								this->printer.printMagenta("[null]");
							} break;

							break; default: evo::debugFatalBreak("Unknown token kind");
						}

						this->printer.println();
					} break;


					case panther::AST::Kind::IDENT: {
						this->print_ident(node);
					} break;

					case panther::AST::Kind::INTRINSIC: {
						this->print_intrinsic(node);
					} break;

					case panther::AST::Kind::UNINIT: {
						this->printer.printMagenta("[uninit]\n");
					} break;

					case panther::AST::Kind::ZEROINIT: {
						this->printer.printMagenta("[zeroinit]\n");
					} break;

					case panther::AST::Kind::THIS: {
						this->printer.printMagenta("[this]\n");
					} break;

					case panther::AST::Kind::DISCARD: {
						this->printer.printMagenta("[_]\n");
					} break;

					case panther::AST::Kind::ARRAY_INIT_NEW: {
						this->print_array_init_new(this->ast_buffer.getArrayInitNew(node));
					} break;

					case panther::AST::Kind::DESIGNATED_INIT_NEW: {
						this->print_designated_init_new(this->ast_buffer.getDesignatedInitNew(node));
					} break;


					case panther::AST::Kind::NONE:             case panther::AST::Kind::VAR_DEF:
					case panther::AST::Kind::FUNC_DEF:         case panther::AST::Kind::DELETED_SPECIAL_METHOD:
					case panther::AST::Kind::ALIAS_DEF:        case panther::AST::Kind::DISTINCT_ALIAS_DEF:
					case panther::AST::Kind::STRUCT_DEF:       case panther::AST::Kind::UNION_DEF:
					case panther::AST::Kind::ENUM_DEF:         case panther::AST::Kind::INTERFACE_DEF:
					case panther::AST::Kind::INTERFACE_IMPL:   case panther::AST::Kind::RETURN:
					case panther::AST::Kind::ERROR:            case panther::AST::Kind::UNREACHABLE:
					case panther::AST::Kind::BREAK:            case panther::AST::Kind::CONTINUE:
					case panther::AST::Kind::DELETE:           case panther::AST::Kind::CONDITIONAL:
					case panther::AST::Kind::WHEN_CONDITIONAL: case panther::AST::Kind::WHILE:
					case panther::AST::Kind::DEFER:            case panther::AST::Kind::TEMPLATE_PACK:
					case panther::AST::Kind::MULTI_ASSIGN:     case panther::AST::Kind::ARRAY_TYPE:
					case panther::AST::Kind::TYPEID_CONVERTER: case panther::AST::Kind::ATTRIBUTE_BLOCK:
					case panther::AST::Kind::ATTRIBUTE:        case panther::AST::Kind::DEDUCER:
					case panther::AST::Kind::PRIMITIVE_TYPE: {
						evo::debugFatalBreak("Unsupported expr type");
					} break;
				}
			}


			auto print_prefix(const panther::AST::Prefix& prefix) -> void {
				this->print_major_header("Prefix");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Operator");
					this->printer.printMagenta(" {}\n", this->source.getTokenBuffer()[prefix.opTokenID].kind());

					this->indenter.print_end();
					this->print_minor_header("RHS");
					{
						this->printer.println();
						this->indenter.push();
						this->print_expr(prefix.rhs);
						this->indenter.pop();
					}

					this->indenter.pop();
				}
			}


			auto print_assignment(const panther::AST::Infix& infix) -> void {
				this->indenter.print();
				this->print_major_header("Assign");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Operator");
					this->printer.printMagenta(" {}\n", this->source.getTokenBuffer()[infix.opTokenID].kind());

					this->indenter.print_arrow();
					this->print_minor_header("LHS");
					{
						this->printer.println();
						this->indenter.push();
						this->print_expr(infix.lhs);
						this->indenter.pop();
					}

					this->indenter.print_end();
					this->print_minor_header("RHS");
					{
						this->printer.println();
						this->indenter.push();
						this->print_expr(infix.rhs);
						this->indenter.pop();
					}

					this->indenter.pop();
				}
			}


			auto print_multi_assign(const panther::AST::MultiAssign& multi_assign) -> void {
				this->indenter.print();
				this->print_major_header("Multiple Assignment");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Assignments");
					this->printer.println();
					{
						this->indenter.push();
						for(size_t i = 0; const panther::AST::Node& assign : multi_assign.assigns){
							if(i + 1 < multi_assign.assigns.size()){
								this->indenter.print_arrow();
							}else{
								this->indenter.print_end();
							}

							this->print_minor_header(std::format("Assignment {}", i));

							switch(assign.kind()){
								case panther::AST::Kind::IDENT: {
									this->printer.print(" ");
									this->print_ident(assign);
								} break;

								case panther::AST::Kind::DISCARD: {
									this->printer.printMagenta(" [_]\n");
								} break;

								default: {
									this->printer.println();
									this->indenter.push();
									this->print_expr(assign);
									this->indenter.pop();
								} break;
							}

							i += 1;
						}
						this->indenter.pop();
					}


					this->indenter.print_end();
					this->print_minor_header("Value");
					this->printer.println();
					this->indenter.push();
					this->print_expr(multi_assign.value);
					this->indenter.pop();

					this->indenter.pop();
				}
			}


			auto print_infix(const panther::AST::Infix& infix) -> void {
				this->print_major_header("Infix");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Operator");
					this->printer.printMagenta(" {}\n", this->source.getTokenBuffer()[infix.opTokenID].kind());

					this->indenter.print_arrow();
					this->print_minor_header("LHS");
					{
						this->printer.println();
						this->indenter.push();
						this->print_expr(infix.lhs);
						this->indenter.pop();
					}

					this->indenter.print_end();
					this->print_minor_header("RHS");
					{
						this->printer.println();
						this->indenter.push();
						this->print_expr(infix.rhs);
						this->indenter.pop();
					}

					this->indenter.pop();
				}
			}

			auto print_postfix(const panther::AST::Postfix& postfix) -> void {
				this->print_major_header("Postfix");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Operator");
					this->printer.printMagenta(" {}\n", this->source.getTokenBuffer()[postfix.opTokenID].kind());

					this->indenter.print_end();
					this->print_minor_header("LHS");
					{
						this->printer.println();
						this->indenter.push();
						this->print_expr(postfix.lhs);
						this->indenter.pop();
					}

					this->indenter.pop();
				}
			}


			auto print_func_call(const panther::AST::FuncCall& func_call) -> void {
				this->print_major_header("Function Call");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Target");
					{
						this->printer.println();
						this->indenter.push();
						this->print_expr(func_call.target);
						this->indenter.pop();
					}

					this->indenter.print_end();
					this->print_minor_header("Arguments");
					this->print_func_call_args(func_call.args);

					this->indenter.pop();
				}
			}

			auto print_indexer(const panther::AST::Indexer& indexer) -> void {
				this->print_major_header("Indexer");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Target");
					{
						this->printer.println();
						this->indenter.push();
						this->print_expr(indexer.target);
						this->indenter.pop();
					}

					if(indexer.indices.size() == 1){
						this->indenter.print_end();
						this->print_minor_header("Index");
						{
							this->printer.println();
							this->indenter.push();
							this->print_expr(indexer.indices[0]);
							this->indenter.pop();
						}

					}else{
						this->indenter.print_end();
						this->print_minor_header("Indices");
						{
							this->printer.println();
							this->indenter.push();
							
							for(size_t i = 0; const panther::AST::Node index : indexer.indices){
								if(i + 1 < indexer.indices.size()){
									this->indenter.print_arrow();
								}else{
									this->indenter.print_end();
								}

								this->print_major_header(std::format("Index: {}", i));

								{
									this->indenter.push();
									this->print_expr(index);
									this->indenter.pop();
								}
							
								i += 1;
							}

							this->indenter.pop();
						}
					}

					this->indenter.pop();
				}
			}


			auto print_func_call_args(const evo::SmallVector<panther::AST::FuncCall::Arg>& args) -> void {
				if(args.empty()){
					this->printer.printGray(" {EMPTY}\n");
				}else{
					this->printer.println();
					this->indenter.push();

					for(size_t i = 0; const panther::AST::FuncCall::Arg& arg : args){
						if(i + 1 < args.size()){
							this->indenter.print_arrow();
						}else{
							this->indenter.print_end();
						}

						this->print_major_header(std::format("Argument {}", i));
						{
							this->indenter.push();

							if(arg.label.has_value()){
								this->indenter.print_arrow();
								this->print_minor_header("Label");
								this->printer.print(" ");
								this->print_ident(*arg.label);

								this->indenter.print_end();
								this->print_minor_header("Expression");
								this->printer.println();
								this->indenter.push();
								this->print_expr(arg.value);
								this->indenter.pop();
							}else{
								// this->print_minor_header("Expression");
								// this->printer.println();
								this->print_expr(arg.value);
							}

							this->indenter.pop();
						}

					
						i += 1;
					}


					this->indenter.pop();
				}
			}


			auto print_template_pack(const std::optional<panther::AST::Node>& template_pack_node) -> void {
				this->indenter.print_arrow();
				this->print_minor_header("Template Pack");
				if(template_pack_node.has_value()){
					const panther::AST::TemplatePack& template_pack = 
						this->ast_buffer.getTemplatePack(*template_pack_node);

					this->printer.println();
					this->indenter.push();
					for(size_t i = 0; const panther::AST::TemplatePack::Param& param : template_pack.params){
						if(i + 1 < template_pack.params.size()){
							this->indenter.print_arrow();
						}else{
							this->indenter.print_end();
						}

						this->print_major_header(std::format("Parameter {}", i));

						{
							this->indenter.push();

							this->indenter.print_arrow();
							this->print_minor_header("Identifier");
							this->printer.print(" ");
							this->print_ident(param.ident);

							this->indenter.print_arrow();
							this->print_minor_header("Type");
							this->printer.print(" ");
							this->print_type(this->ast_buffer.getType(param.type));
							this->printer.println();

							this->indenter.print_end();
							this->print_minor_header("Default Value");
							if(param.defaultValue.has_value()){
								this->printer.println();
								this->indenter.push();
								this->print_expr(*param.defaultValue);
								this->indenter.pop();
							}else{
								this->printer.printGray(" {NONE}\n");
							}

							this->indenter.pop();
						}

						i += 1;
					}
					this->indenter.pop();
					
				}else{
					this->printer.printGray(" {NONE}\n");
				}
			}


			auto print_templated_expr(const panther::AST::TemplatedExpr& templated_expr) -> void {
				this->print_major_header("Templated Expressions");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Base");
					{
						this->printer.println();
						this->indenter.push();
						this->print_expr(templated_expr.base);
						this->indenter.pop();
					}

					this->indenter.print_end();
					this->print_minor_header("Arguments");
					if(templated_expr.args.empty()){
						this->printer.printGray(" {EMPTY}\n");
					}else{
						this->printer.println();
						this->indenter.push();

						for(size_t i = 0; const panther::AST::Node& arg : templated_expr.args){
							if(i + 1 < templated_expr.args.size()){
								this->indenter.print_arrow();
							}else{
								this->indenter.print_end();
							}

							this->print_major_header(std::format("Argument {}", i));
							this->indenter.push();
							this->print_expr(arg);
							this->indenter.pop();
						
							i += 1;
						}


						this->indenter.pop();
					}


					this->indenter.pop();
				}
			}


			auto print_new(const panther::AST::New& new_expr) -> void {
				this->print_major_header("New");

				this->indenter.push();

				this->indenter.print_arrow();
				this->print_minor_header("Type");
				this->printer.print(" ");
				this->print_type(this->ast_buffer.getType(new_expr.type));
				this->printer.println();

				this->indenter.print_end();
				this->print_minor_header("Arguments");
				this->print_func_call_args(new_expr.args);

				this->indenter.pop();
			}



			auto print_array_init_new(const panther::AST::ArrayInitNew& array_init_new) -> void {
				this->print_major_header("Array Initializer New");

				this->indenter.push();

				this->indenter.print_arrow();
				this->print_minor_header("Type");
				this->printer.print(" ");
				this->print_type(this->ast_buffer.getType(array_init_new.type));
				this->printer.println();

				this->indenter.print_end();
				this->print_minor_header("Values");

				{
					this->printer.println();
					this->indenter.push();

					for(size_t i = 0; const panther::AST::Node& value : array_init_new.values){
						if(i + 1 < array_init_new.values.size()){
							this->indenter.print_arrow();
						}else{
							this->indenter.print_end();
						}

						this->print_major_header(std::format("Value {}", i));
						this->indenter.push();
						this->print_expr(value);
						this->indenter.pop();

						i += 1;
					}

					this->indenter.pop();
				}

				this->indenter.pop();
			}


			auto print_designated_init_new(const panther::AST::DesignatedInitNew& designated_init_new) -> void {
				this->print_major_header("Designated Initializer New");

				this->indenter.push();

				this->indenter.print_arrow();
				this->print_minor_header("Type");
				this->printer.print(" ");
				this->print_type(this->ast_buffer.getType(designated_init_new.type));
				this->printer.println();

				this->indenter.print_end();
				this->print_minor_header("Arguments");
				
				{
					this->printer.println();
					this->indenter.push();

					for(
						size_t i = 0; 
						const panther::AST::DesignatedInitNew::MemberInit& member_init : designated_init_new.memberInits
					){
						if(i + 1 < designated_init_new.memberInits.size()){
							this->indenter.print_arrow();
						}else{
							this->indenter.print_end();
						}

						this->print_major_header(std::format("Initializer {}", i));

						{
							this->indenter.push();

							this->indenter.print_arrow();
							this->print_minor_header("Identifier");
							this->printer.print(" ");
							this->print_ident(member_init.ident);

							this->indenter.print_end();
							this->print_minor_header("Value");
							this->printer.println();
							this->indenter.push();
							this->print_expr(member_init.expr);
							this->indenter.pop();

							this->indenter.pop();
						}

						i += 1;
					}

					this->indenter.pop();
				}

				this->indenter.pop();
			}


			auto print_try_else(const panther::AST::TryElse& try_else_expr) -> void {
				this->print_major_header("Try/Else");

				this->indenter.push();

				this->indenter.print_arrow();
				this->print_minor_header("Attempt");
				this->printer.println();
				this->indenter.push();
				this->print_expr(try_else_expr.attemptExpr);
				this->indenter.pop();
				
				this->indenter.print_end();
				this->print_minor_header("Except");
				this->printer.println();
				this->indenter.push();
				this->print_expr(try_else_expr.exceptExpr);
				this->indenter.pop();

				this->indenter.pop();
			}


			auto print_attribute_block(const panther::AST::AttributeBlock& attr_block) -> void {
				this->indenter.print();
				this->print_minor_header("Attribute Block");

				if(attr_block.attributes.empty()){
					this->printer.printGray(" {NONE}\n");
				}else{
					this->printer.println();

					this->indenter.push();
					for(size_t i = 0; const panther::AST::AttributeBlock::Attribute& attribute : attr_block.attributes){
						if(i + 1 < attr_block.attributes.size()){
							this->indenter.print_arrow();
						}else{
							this->indenter.print_end();
						}

						this->print_major_header(std::format("Attribute {}", i));
						{
							this->indenter.push();

							this->indenter.print_arrow();
							this->print_minor_header("Attribute");
							const panther::Token& attr_token = this->source.getTokenBuffer()[attribute.attribute];
							this->printer.printMagenta(" #{}\n", attr_token.getString());

							this->indenter.print_end();
							if(attribute.args.empty()){
								this->print_minor_header("Argument(s)");
								this->printer.printGray(" {NONE}\n");

							}else if(attribute.args.size() == 1){
								this->print_minor_header("Argument");
								this->printer.println();
								this->indenter.push();
								this->print_expr(attribute.args[0]);
								this->indenter.pop();

							}else{
								this->print_minor_header("Arguments");
								this->printer.println();
								this->indenter.push();
								for(size_t j = 0; const panther::AST::Node& arg : attribute.args){
									if(j - 1 < attribute.args.size()){
										this->indenter.print_end();
									}else{
										this->indenter.print_arrow();
									}

									this->print_major_header(std::format("Argument {}", j));
									this->indenter.push();
									this->print_expr(arg);
									this->indenter.pop();
								
									j += 1;
								}
								this->indenter.pop();
							}

							this->indenter.pop();
						}
					
						i += 1;
					}
					this->indenter.pop();
				}
			}


			auto print_ident(const panther::AST::Node& ident) const -> void {
				const panther::Token::ID ident_token_id = this->ast_buffer.getIdent(ident);
				const panther::Token& ident_tok = this->source.getTokenBuffer()[ident_token_id];
				this->printer.printMagenta("{}\n", ident_tok.getString());
			}

			auto print_ident(panther::Token::ID ident_token_id) const -> void {
				const panther::Token& ident_tok = this->source.getTokenBuffer()[ident_token_id];
				evo::debugAssert(ident_tok.kind() == panther::Token::Kind::IDENT);
				this->printer.printMagenta("{}\n", ident_tok.getString());
			}

			auto print_intrinsic(const panther::AST::Node& intrinsic) const -> void {
				const panther::Token::ID intrinsic_token_id = this->ast_buffer.getIntrinsic(intrinsic);
				const panther::Token& intrinsic_tok = this->source.getTokenBuffer()[intrinsic_token_id];
				this->printer.printMagenta("@{}\n", intrinsic_tok.getString());
			}


			auto print_major_header(std::string_view title) -> void {
				this->printer.printCyan(title);
				this->printer.printGray(":\n");
			}

			auto print_minor_header(std::string_view title) -> void {
				this->printer.printBlue(title);
				this->printer.printGray(":");
			}


	
		private:
			core::Printer& printer;
			const panther::Source& source;
			const fs::path& relative_dir;
			const panther::ASTBuffer& ast_buffer;

			Indenter indenter;
	};



	auto print_AST(core::Printer& printer, const panther::Source& source, const fs::path& relative_dir) -> void {
		auto ast_printer = ASTPrinter(printer, source, relative_dir);

		ast_printer.print_header();
		ast_printer.print_globals();
	}

	
}
