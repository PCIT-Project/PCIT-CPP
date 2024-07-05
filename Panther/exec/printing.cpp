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

		auto location_strings = std::vector<std::string>();

		for(panther::Token::ID token_id : token_buffer){
			const panther::Token& token = token_buffer[token_id];
			const panther::Token::Location& location = token.getLocation();

			location_strings.emplace_back(std::format("<{}:{}>", location.lineStart, location.collumnStart));
		}

		const size_t longest_location_string_length = std::ranges::max_element(
			location_strings,
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
					break; case panther::Token::Kind::Ident:         return std::format(" {}", token.getString());
					break; case panther::Token::Kind::Intrinsic:     return std::format(" @{}", token.getString());
					break; case panther::Token::Kind::Attribute:     return std::format(" #{}", token.getString());

					break; case panther::Token::Kind::LiteralBool:   return std::format(" {}", token.getBool());
					break; case panther::Token::Kind::LiteralInt:    return std::format(" {}", token.getInt());
					break; case panther::Token::Kind::LiteralFloat:  return std::format(" {}", token.getFloat());
					break; case panther::Token::Kind::LiteralChar:   return std::format(" \'{}\'", token.getString());
					break; case panther::Token::Kind::LiteralString: return std::format(" \"{}\"", token.getString());

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
			std::vector<IndenterType> indents{};
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
					this->print_ident(var_decl.ident);

					this->indenter.print_arrow();
					this->print_minor_header("Type");
					if(var_decl.type.hasValue()){
						this->print_type(var_decl.type.getValue());
					}else{
						this->printer.printGray(" [INFERRED]\n");
					}

					this->indenter.print_end();
					this->print_minor_header("Value");
					if(var_decl.value.hasValue()){
						this->printer.print("\n");
						this->indenter.push();
						this->print_expr(var_decl.value.getValue());
						this->indenter.pop();
					}else{
						this->printer.printGray(" [NONE]\n");
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
					this->print_ident(func_decl.ident);

					this->indenter.print_arrow();
					this->print_minor_header("Return Type");
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
					this->printer.printGray(" [EMPTY]\n");

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

			
			auto print_type(const panther::AST::Node& node) const noexcept -> void {
				if(node.getKind() == panther::AST::Kind::Type){
					const panther::AST::Type& type = this->ast_buffer.getType(node);

					const panther::Token::ID type_token_id = this->ast_buffer.getBuiltinType(type.base);
					this->printer.printMagenta(" {}", this->source.getTokenBuffer()[type_token_id].getKind());
					this->printer.printGray(" [BUILTIN]\n");

				}else{
					evo::debugFatalBreak("Invalid type kind");
				}
			};


			auto print_expr(const panther::AST::Node& node) noexcept -> void {
				this->indenter.print();

				switch(node.getKind()){
					case panther::AST::Kind::Literal: {
						const panther::Token::ID token_id = this->ast_buffer.getLiteral(node);
						const panther::Token& token = this->source.getTokenBuffer()[token_id];

						switch(token.getKind()){
							case panther::Token::Kind::LiteralInt: {
								this->printer.printMagenta(std::to_string(token.getInt()));
								this->printer.printGray(" [LiteralInt]");
							} break;

							case panther::Token::Kind::LiteralFloat: {
								this->printer.printMagenta(std::to_string(token.getFloat()));
								this->printer.printGray(" [LiteralFloat]");
							} break;

							case panther::Token::Kind::LiteralBool: {
								this->printer.printMagenta(evo::boolStr(token.getBool()));
								this->printer.printGray(" [LiteralBool]");
							} break;

							case panther::Token::Kind::LiteralString: {
								this->printer.printMagenta("\"{}\"", token.getString());
								this->printer.printGray(" [LiteralString]");
							} break;

							case panther::Token::Kind::LiteralChar: {
								this->printer.printMagenta("'{}'", token.getString());
								this->printer.printGray(" [LiteralChar]");
							} break;

							break; default: evo::debugFatalBreak("Unknown token kind");
						};

						this->printer.print("\n");
					} break;


					case panther::AST::Kind::Ident: {
						this->print_ident(node);
					} break;


					default: evo::debugFatalBreak("Unknown or unsupported expr type");
				};
			};



			auto print_ident(const panther::AST::Node& ident) const noexcept -> void {
				const panther::Token::ID ident_tok_id = this->ast_buffer.getIdent(ident);
				const panther::Token& ident_tok = this->source.getTokenBuffer()[ident_tok_id];
				this->printer.printMagenta(" {}\n", ident_tok.getString());
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