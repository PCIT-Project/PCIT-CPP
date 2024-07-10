//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./printing.h"

namespace pthr{
	
	//////////////////////////////////////////////////////////////////////
	// tokens

	auto printTokens(pcit::core::Printer& printer, const panther::Source& source) noexcept -> void {
		const panther::TokenBuffer& token_buffer = source.getTokenBuffer();

		///////////////////////////////////
		// print header

		printer.printGray("------------------------------\n");

		printer.printCyan("Tokens: {}\n", source.getLocationAsString());

		if(token_buffer.size() == 0){
			printer.printGray("(NONE)\n");

		}else if(token_buffer.size() == 1){
			printer.printGray("(1 token)\n");

		}else{
			printer.printGray("({} tokens)\n", token_buffer.size());
		}


		///////////////////////////////////
		// create location strings

		auto location_strings = evo::SmallVector<std::string>();

		for(panther::Token::ID token_id : token_buffer){
			const panther::Token& token = token_buffer[token_id];
			const panther::Token::Location& location = token.getLocation();

			location_strings.emplace_back(std::format("<{}:{}>", location.lineStart, location.collumnStart));
		}

		const size_t longest_location_string_length = std::max_element(
			location_strings.begin(), location_strings.end(),
			[](const std::string& lhs, const std::string& rhs) noexcept -> bool {
				return lhs.size() < rhs.size();
			}
		)->size();

		for(std::string& str : location_strings){
			while(str.size() < longest_location_string_length){
				str += ' ';
			};

			str += ' ';
		}


		///////////////////////////////////
		// print out tokens

		for(size_t i = 0; panther::Token::ID token_id : token_buffer){
			const panther::Token& token = token_buffer[token_id];
			
			printer.printGray(location_strings[i]);
			printer.printInfo("[{}]", token.getKind());


			const std::string data_str = [&]() noexcept {
				switch(token.getKind()){
					break; case panther::Token::Kind::Ident:     return std::format(" {}", token.getString(source));
					break; case panther::Token::Kind::Intrinsic: return std::format(" @{}", token.getString(source));
					break; case panther::Token::Kind::Attribute: return std::format(" #{}", token.getString(source));

					break; case panther::Token::Kind::LiteralBool:   return std::format(" {}", token.getBool());
					break; case panther::Token::Kind::LiteralInt:    return std::format(" {}", token.getInt());
					break; case panther::Token::Kind::LiteralFloat:  return std::format(" {}", token.getFloat());
					break; case panther::Token::Kind::LiteralChar:
						return std::format(" \'{}\'", token.getString(source));
					break; case panther::Token::Kind::LiteralString:
						return std::format(" \"{}\"", token.getString(source));

					break; default: return std::string();
				};
			}();

			printer.printMagenta(data_str + '\n');

			i += 1;
		}
		
	};



	//////////////////////////////////////////////////////////////////////
	// ast


	class Indenter{
		public:
			Indenter(pcit::core::Printer& _printer) noexcept : printer(_printer) {};
			~Indenter() = default;

			auto push() noexcept -> void {
				this->indents.emplace_back(IndenterType::EndArrow);
			};


			auto pop() noexcept -> void {
				this->indents.pop_back();
			};

			auto set_arrow() noexcept -> void {
				this->indents.back() = IndenterType::Arrow;
			};

			auto set_end() noexcept -> void {
				this->indents.back() = IndenterType::EndArrow;		
			};


			auto print() noexcept -> void {
				auto print_string = std::string{};

				for(const IndenterType& indent : this->indents){
					switch(indent){
						break; case IndenterType::Line:     print_string += "|   ";
						break; case IndenterType::Arrow:    print_string += "|-> ";
						break; case IndenterType::EndArrow: print_string += "\\-> ";
						break; case IndenterType::None:     print_string += "    ";
					};
				}

				this->printer.printGray(print_string);

				if(this->indents.empty() == false){
					if(this->indents.back() == IndenterType::Arrow){
					    this->indents.back() = IndenterType::Line; 

					}else if(this->indents.back() == IndenterType::EndArrow){
						this->indents.back() = IndenterType::None;
					}
				}
			};


			auto print_arrow() noexcept -> void {
				this->set_arrow();
				this->print();
			};

			auto print_end() noexcept -> void {
				this->set_end();
				this->print();
			};

	
		private:
			pcit::core::Printer& printer;

			enum class IndenterType{
				Line,
				Arrow,
				EndArrow,
				None,
			};
			evo::SmallVector<IndenterType> indents{};
	};




	class ASTPrinter{
		public:
			ASTPrinter(pcit::core::Printer& _printer, const panther::Source& _source) noexcept 
				: printer(_printer), source(_source), ast_buffer(this->source.getASTBuffer()), indenter(_printer) {};
			~ASTPrinter() = default;


			auto print_header() noexcept -> void {
				this->printer.printGray("------------------------------\n");

				this->printer.printCyan("AST: {}\n", source.getLocationAsString());

				if(ast_buffer.numGlobalStmts() == 0){
					this->printer.printGray("(NONE)\n");

				}else if(ast_buffer.numGlobalStmts() == 1){
					this->printer.printGray("(1 global statement)\n");

				}else{
					this->printer.printGray("({} global statements)\n", ast_buffer.numGlobalStmts());
				}
			};

			auto print_globals() noexcept -> void {
				for(const panther::AST::Node& global_stmt : ast_buffer.getGlobalStmts()){
					this->print_stmt(global_stmt);
				}
			};


		private:
			auto print_stmt(const panther::AST::Node& stmt) noexcept -> void {
				switch(stmt.getKind()){
					case panther::AST::Kind::VarDecl: {
						this->print_var_decl(this->ast_buffer.getVarDecl(stmt));
					} break;

					case panther::AST::Kind::FuncDecl: {
						this->print_func_decl(this->ast_buffer.getFuncDecl(stmt));
					} break;

					case panther::AST::Kind::Infix: {
						this->print_assignment(this->ast_buffer.getInfix(stmt));
					} break;

					case panther::AST::Kind::FuncCall: {
						this->indenter.print();
						this->print_func_call(this->ast_buffer.getFuncCall(stmt));
					} break;


					default: {
						evo::debugFatalBreak(
							"Unknown or unsupported statement kind ({})", evo::to_underlying(stmt.getKind())
						);
					} break;
				};
			};

			auto print_var_decl(const panther::AST::VarDecl& var_decl) noexcept -> void {
				this->indenter.print();
				this->print_major_header("Variable Declaration");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(var_decl.ident);

					this->indenter.print_arrow();
					this->print_minor_header("Type");
					if(var_decl.type.hasValue()){
						this->printer.print(" ");
						this->print_type(var_decl.type.value());
					}else{
						this->printer.printGray(" {INFERRED}\n");
					}

					this->indenter.print_end();
					this->print_minor_header("Value");
					if(var_decl.value.hasValue()){
						this->printer.print("\n");
						this->indenter.push();
						this->print_expr(var_decl.value.value());
						this->indenter.pop();
					}else{
						this->printer.printGray(" {NONE}\n");
					}
					
					this->indenter.pop();
				}
			};


			auto print_func_decl(const panther::AST::FuncDecl& func_decl) noexcept -> void {
				this->indenter.print();
				this->print_major_header("Function Declaration");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(func_decl.ident);

					this->indenter.print_arrow();
					this->print_minor_header("Parameters");
					if(func_decl.params.empty()){
						this->printer.printGray(" {NONE}\n");
					}else{
						this->printer.print("\n");
						this->indenter.push();
						for(size_t i = 0; const panther::AST::FuncDecl::Param& param : func_decl.params){
							if(i + 1 < func_decl.params.size()){
								this->indenter.print_arrow();
							}else{
								this->indenter.print_end();
							}

							this->print_major_header(std::format("Parameter {}", i));

							{
								this->indenter.push();

								if(param.ident.getKind() == panther::AST::Kind::Ident){
									this->indenter.print_arrow();
									this->print_minor_header("Identifier");
									this->printer.print(" ");
									this->print_ident(param.ident);

									this->indenter.print_arrow();
									this->print_minor_header("Type");
									this->printer.print(" ");
									this->print_type(param.type.value());

								}else{
									this->indenter.print_arrow();
									this->print_minor_header("Identifier");
									this->printer.printMagenta(" {this}\n");
								}

								this->indenter.print_end();
								this->print_minor_header("Kind");
								using ParamKind = panther::AST::FuncDecl::Param::Kind;
								switch(param.kind){
									break; case ParamKind::Read: this->printer.printMagenta(" {read}\n");
									break; case ParamKind::Mut: this->printer.printMagenta(" {mut}\n");
									break; case ParamKind::In: this->printer.printMagenta(" {in}\n");
								};

								this->indenter.pop();
							}


							i += 1;
						}
						this->indenter.pop();
					}

					this->indenter.print_arrow();
					this->print_minor_header("Return Type");
					this->printer.print(" ");
					this->print_type(func_decl.returnType);

					this->indenter.set_end();
					this->print_block(this->ast_buffer.getBlock(func_decl.block));

					this->indenter.pop();
				}
			};


			auto print_block(const panther::AST::Block& block) noexcept -> void {
				this->indenter.print();
				this->print_minor_header("Statement Block");

				if(block.stmts.empty()){
					this->printer.printGray(" {EMPTY}\n");

				}else{
					this->printer.print("\n");

					this->indenter.push();

					for(size_t i = 0; const panther::AST::Node& stmt : block.stmts){
						if(i + 1 < block.stmts.size()){
							this->indenter.set_arrow();
						}else{
							this->indenter.set_end();
						}

						this->print_stmt(stmt);

						i += 1;
					}

					this->indenter.pop();
				}
			};



			auto print_base_type(const panther::AST::Node& base_type) noexcept -> void {
				switch(base_type.getKind()){
					case panther::AST::Kind::BuiltinType: {
						const panther::Token::ID type_token_id = this->ast_buffer.getBuiltinType(base_type);
						const panther::Token& type_token = this->source.getTokenBuffer()[type_token_id];

						if(type_token.getKind() == panther::Token::Kind::TypeI_N){
							this->printer.printMagenta("I{}", type_token.getBitWidth());
						}else if(type_token.getKind() == panther::Token::Kind::TypeUI_N){
							this->printer.printMagenta("UI{}", type_token.getBitWidth());
						}else{
							this->printer.printMagenta("{}", type_token.getKind());
						}

					} break;

					case panther::AST::Kind::Ident: {
						const panther::Token::ID type_token_id = this->ast_buffer.getIdent(base_type);
						this->printer.printMagenta(
							"{}", this->source.getTokenBuffer()[type_token_id].getString(this->source)
						);
					} break;

					case panther::AST::Kind::Intrinsic: {
						const panther::Token::ID type_token_id = this->ast_buffer.getIntrinsic(base_type);
						this->printer.printMagenta(
							"@{}", this->source.getTokenBuffer()[type_token_id].getString(this->source)
						);
					} break;

					case panther::AST::Kind::Infix: {
						const panther::AST::Infix& infix = this->ast_buffer.getInfix(base_type);
						print_base_type(infix.lhs);
						this->printer.printMagenta(".");
						print_base_type(infix.rhs);
					} break;

					default: evo::debugFatalBreak("Unknown or unsupported base type");
				};
			};

			
			auto print_type(const panther::AST::Node& node) noexcept -> void {
				if(node.getKind() == panther::AST::Kind::Type){
					const panther::AST::Type& type = this->ast_buffer.getType(node);

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
							if(qualifier.isOptional){ qualifier_str += '?'; }
						}

						this->printer.printMagenta(qualifier_str);
					}

					if(type.base.getKind() == panther::AST::Kind::BuiltinType){
						this->printer.printGray(" {BUILTIN}\n");
					}else{
						this->printer.print("\n");
					}

				}else{
					evo::debugFatalBreak("Invalid type kind");
				}
			};


			auto print_expr(const panther::AST::Node& node) noexcept -> void {
				this->indenter.print();

				switch(node.getKind()){
					case panther::AST::Kind::Prefix: {
						this->print_prefix(this->ast_buffer.getPrefix(node));
					} break;

					case panther::AST::Kind::Infix: {
						this->print_infix(this->ast_buffer.getInfix(node));
					} break;

					case panther::AST::Kind::Postfix: {
						this->print_postfix(this->ast_buffer.getPostfix(node));
					} break;

					case panther::AST::Kind::FuncCall: {
						this->print_func_call(this->ast_buffer.getFuncCall(node));
					} break;

					case panther::AST::Kind::Type: {
						this->print_type(node);
					} break;


					case panther::AST::Kind::Literal: {
						const panther::Token::ID token_id = this->ast_buffer.getLiteral(node);
						const panther::Token& token = this->source.getTokenBuffer()[token_id];

						switch(token.getKind()){
							case panther::Token::Kind::LiteralInt: {
								this->printer.printMagenta(std::to_string(token.getInt()));
								this->printer.printGray(" {LiteralInt}");
							} break;

							case panther::Token::Kind::LiteralFloat: {
								this->printer.printMagenta(std::to_string(token.getFloat()));
								this->printer.printGray(" {LiteralFloat}");
							} break;

							case panther::Token::Kind::LiteralBool: {
								this->printer.printMagenta(evo::boolStr(token.getBool()));
								this->printer.printGray(" {LiteralBool}");
							} break;

							case panther::Token::Kind::LiteralString: {
								this->printer.printMagenta("\"{}\"", token.getString(this->source));
								this->printer.printGray(" {LiteralString}");
							} break;

							case panther::Token::Kind::LiteralChar: {
								this->printer.printMagenta("'{}'", token.getString(this->source));
								this->printer.printGray(" {LiteralChar}");
							} break;

							case panther::Token::Kind::KeywordNull: {
								this->printer.printMagenta("[null]");
							} break;

							break; default: evo::debugFatalBreak("Unknown token kind");
						};

						this->printer.print("\n");
					} break;


					case panther::AST::Kind::Ident: {
						this->print_ident(node);
					} break;

					case panther::AST::Kind::Intrinsic: {
						this->print_intrinsic(node);
					} break;

					case panther::AST::Kind::Uninit: {
						this->printer.printMagenta("[uninit]\n");
					} break;

					case panther::AST::Kind::This: {
						this->printer.printMagenta("[this]\n");
					} break;


					default: evo::debugFatalBreak("Unknown or unsupported expr type");
				};
			};


			auto print_prefix(const panther::AST::Prefix& prefix) noexcept -> void {
				this->print_major_header("Prefix");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Operator");
					this->printer.printMagenta(" {}\n", this->source.getTokenBuffer()[prefix.opTokenID].getKind());

					this->indenter.print_end();
					this->print_minor_header("RHS");
					{
						this->printer.print("\n");
						this->indenter.push();
						this->print_expr(prefix.rhs);
						this->indenter.pop();
					}

					this->indenter.pop();
				}
			};


			auto print_assignment(const panther::AST::Infix& infix) noexcept -> void {
				this->indenter.print();
				this->print_major_header("Assignment");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Operator");
					this->printer.printMagenta(" {}\n", this->source.getTokenBuffer()[infix.opTokenID].getKind());

					this->indenter.print_arrow();
					this->print_minor_header("LHS");
					{
						this->printer.print("\n");
						this->indenter.push();
						this->print_expr(infix.lhs);
						this->indenter.pop();
					}

					this->indenter.print_end();
					this->print_minor_header("RHS");
					{
						this->printer.print("\n");
						this->indenter.push();
						this->print_expr(infix.rhs);
						this->indenter.pop();
					}

					this->indenter.pop();
				}
			};


			auto print_infix(const panther::AST::Infix& infix) noexcept -> void {
				this->print_major_header("Infix");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Operator");
					this->printer.printMagenta(" {}\n", this->source.getTokenBuffer()[infix.opTokenID].getKind());

					this->indenter.print_arrow();
					this->print_minor_header("LHS");
					{
						this->printer.print("\n");
						this->indenter.push();
						this->print_expr(infix.lhs);
						this->indenter.pop();
					}

					this->indenter.print_end();
					this->print_minor_header("RHS");
					{
						this->printer.print("\n");
						this->indenter.push();
						this->print_expr(infix.rhs);
						this->indenter.pop();
					}

					this->indenter.pop();
				}
			};

			auto print_postfix(const panther::AST::Postfix& postfix) noexcept -> void {
				this->print_major_header("Postfix");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Operator");
					this->printer.printMagenta(" {}\n", this->source.getTokenBuffer()[postfix.opTokenID].getKind());

					this->indenter.print_end();
					this->print_minor_header("LHS");
					{
						this->printer.print("\n");
						this->indenter.push();
						this->print_expr(postfix.lhs);
						this->indenter.pop();
					}

					this->indenter.pop();
				}
			};


			auto print_func_call(const panther::AST::FuncCall& func_call) noexcept -> void {
				this->print_major_header("Function Call");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Target");
					{
						this->printer.print("\n");
						this->indenter.push();
						this->print_expr(func_call.target);
						this->indenter.pop();
					}

					this->indenter.print_end();
					this->print_minor_header("Arguments");
					if(func_call.args.empty()){
						this->printer.printGray(" {EMPTY}\n");
					}else{
						this->printer.print("\n");
						this->indenter.push();

						for(size_t i = 0; const panther::AST::FuncCall::Arg& arg : func_call.args){
							if(i + 1 < func_call.args.size()){
								this->indenter.print_arrow();
							}else{
								this->indenter.print_end();
							}

							this->print_major_header(std::format("Argument {}", i));
							{
								this->indenter.push();

								if(arg.explicitIdent.hasValue()){
									this->indenter.print_arrow();
									this->print_minor_header("Explicit Identifier");
									this->printer.print(" ");
									this->print_ident(arg.explicitIdent.value());

									this->indenter.print_end();
									this->print_minor_header("Expression");
									this->printer.print("\n");
									this->indenter.push();
									this->print_expr(arg.value);
									this->indenter.pop();
								}else{
									// this->print_minor_header("Expression");
									// this->printer.print("\n");
									this->print_expr(arg.value);
								}

								this->indenter.pop();
							}

						
							i += 1;
						}


						this->indenter.pop();
					}



					this->indenter.pop();
				}
			};



			auto print_ident(const panther::AST::Node& ident) const noexcept -> void {
				const panther::Token::ID ident_tok_id = this->ast_buffer.getIdent(ident);
				const panther::Token& ident_tok = this->source.getTokenBuffer()[ident_tok_id];
				this->printer.printMagenta("{}\n", ident_tok.getString(this->source));
			};

			auto print_intrinsic(const panther::AST::Node& ident) const noexcept -> void {
				const panther::Token::ID ident_tok_id = this->ast_buffer.getIntrinsic(ident);
				const panther::Token& ident_tok = this->source.getTokenBuffer()[ident_tok_id];
				this->printer.printMagenta("@{}\n", ident_tok.getString(this->source));
			};


			auto print_major_header(std::string_view title) noexcept -> void {
				this->printer.printCyan("{}", title);
				this->printer.printGray(":\n");
			};

			auto print_minor_header(std::string_view title) noexcept -> void {
				this->printer.printBlue("{}", title);
				this->printer.printGray(":");
			};


	
		private:
			pcit::core::Printer& printer;
			const panther::Source& source;
			const panther::ASTBuffer& ast_buffer;

			Indenter indenter;
	};





	auto printAST(pcit::core::Printer& printer, const panther::Source& source) noexcept -> void {
		auto ast_printer = ASTPrinter(printer, source);

		ast_printer.print_header();
		ast_printer.print_globals();
	};


};