////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "./printing.h"



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
				this->indents.emplace_back(IndenterType::EndArrow);
			}


			auto pop() -> void {
				this->indents.pop_back();
			}

			auto set_arrow() -> void {
				this->indents.back() = IndenterType::Arrow;
			}

			auto set_end() -> void {
				this->indents.back() = IndenterType::EndArrow;		
			}


			auto print() -> void {
				auto print_string = std::string{};

				for(const IndenterType& indent : this->indents){
					switch(indent){
						break; case IndenterType::Line:     print_string += "|   ";
						break; case IndenterType::Arrow:    print_string += "|-> ";
						break; case IndenterType::EndArrow: print_string += "\\-> ";
						break; case IndenterType::None:     print_string += "    ";
					}
				}

				this->printer.printGray(print_string);

				if(this->indents.empty() == false){
					if(this->indents.back() == IndenterType::Arrow){
					    this->indents.back() = IndenterType::Line; 

					}else if(this->indents.back() == IndenterType::EndArrow){
						this->indents.back() = IndenterType::None;
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
				Line,
				Arrow,
				EndArrow,
				None,
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
					break; case panther::Token::Kind::Ident:     return std::format(" {}", token.getString());
					break; case panther::Token::Kind::Intrinsic: return std::format(" @{}", token.getString());
					break; case panther::Token::Kind::Attribute: return std::format(" #{}", token.getString());

					break; case panther::Token::Kind::LiteralBool:   return std::format(" {}", token.getBool());
					break; case panther::Token::Kind::LiteralInt:    return std::format(" {}", token.getInt());
					break; case panther::Token::Kind::LiteralFloat:  return std::format(" {}", token.getFloat());
					break; case panther::Token::Kind::LiteralChar:   return std::format(" \'{}\'", token.getString());
					break; case panther::Token::Kind::LiteralString: return std::format(" \"{}\"", token.getString());

					break; case panther::Token::Kind::TypeI_N: return std::format(" {}", token.getBitWidth());
					break; case panther::Token::Kind::TypeUI_N: return std::format(" {}", token.getBitWidth());

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
					case panther::AST::Kind::VarDecl: {
						this->print_var_decl(this->ast_buffer.getVarDecl(stmt));
					} break;

					case panther::AST::Kind::FuncDecl: {
						this->print_func_decl(this->ast_buffer.getFuncDecl(stmt));
					} break;

					case panther::AST::Kind::AliasDecl: {
						this->print_alias_decl(this->ast_buffer.getAliasDecl(stmt));
					} break;

					case panther::AST::Kind::TypedefDecl: {
						this->print_type_decl(this->ast_buffer.getTypedefDecl(stmt));
					} break;

					case panther::AST::Kind::StructDecl: {
						this->print_struct_decl(this->ast_buffer.getStructDecl(stmt));
					} break;

					case panther::AST::Kind::Return: {
						this->print_return(this->ast_buffer.getReturn(stmt));
					} break;

					case panther::AST::Kind::Conditional: {
						this->print_conditional(this->ast_buffer.getConditional(stmt));
					} break;

					case panther::AST::Kind::WhenConditional: {
						this->print_conditional(this->ast_buffer.getWhenConditional(stmt));
					} break;

					case panther::AST::Kind::While: {
						this->print_while(this->ast_buffer.getWhile(stmt));
					} break;

					case panther::AST::Kind::Infix: {
						this->print_assignment(this->ast_buffer.getInfix(stmt));
					} break;

					case panther::AST::Kind::MultiAssign: {
						this->print_multi_assign(this->ast_buffer.getMultiAssign(stmt));
					} break;

					case panther::AST::Kind::FuncCall: {
						this->indenter.print();
						this->print_func_call(this->ast_buffer.getFuncCall(stmt));
					} break;

					case panther::AST::Kind::Block: {
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

			auto print_var_decl(const panther::AST::VarDecl& var_decl) -> void {
				this->indenter.print();
				this->print_major_header("Variable Declaration");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Kind");
					switch(var_decl.kind){
						break; case panther::AST::VarDecl::Kind::Var:   this->printer.printMagenta(" var\n");
						break; case panther::AST::VarDecl::Kind::Const: this->printer.printMagenta(" const\n");
						break; case panther::AST::VarDecl::Kind::Def:   this->printer.printMagenta(" def\n");
					}

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(var_decl.ident);

					this->indenter.print_arrow();
					this->print_minor_header("Type");
					if(var_decl.type.has_value()){
						this->printer.print(" ");
						this->print_type(this->ast_buffer.getType(*var_decl.type));
					}else{
						this->printer.printGray(" {INFERRED}\n");
					}

					this->indenter.set_arrow();
					this->print_attribute_block(this->ast_buffer.getAttributeBlock(var_decl.attributeBlock));

					this->indenter.print_end();
					this->print_minor_header("Value");
					if(var_decl.value.has_value()){
						this->printer.println();
						this->indenter.push();
						this->print_expr(*var_decl.value);
						this->indenter.pop();
					}else{
						this->printer.printGray(" {NONE}\n");
					}
					
					this->indenter.pop();
				}
			}


			auto print_func_decl(const panther::AST::FuncDecl& func_decl) -> void {
				this->indenter.print();
				this->print_major_header("Function Declaration");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(func_decl.name);

					this->print_template_pack(func_decl.templatePack);

					this->indenter.print_arrow();
					this->print_minor_header("Parameters");
					if(func_decl.params.empty()){
						this->printer.printGray(" {NONE}\n");
					}else{
						this->printer.println();
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

								if(param.name.kind() == panther::AST::Kind::Ident){
									this->indenter.print_arrow();
									this->print_minor_header("Identifier");
									this->printer.print(" ");
									this->print_ident(param.name);

									this->indenter.print_arrow();
									this->print_minor_header("Type");
									this->printer.print(" ");
									this->print_type(this->ast_buffer.getType(*param.type));

								}else{
									this->indenter.print_arrow();
									this->print_minor_header("Identifier");
									this->printer.printMagenta(" {this}\n");
								}

								this->indenter.print_arrow();
								this->print_minor_header("Kind");
								using ParamKind = panther::AST::FuncDecl::Param::Kind;
								switch(param.kind){
									break; case ParamKind::Read: this->printer.printMagenta(" {read}\n");
									break; case ParamKind::Mut: this->printer.printMagenta(" {mut}\n");
									break; case ParamKind::In: this->printer.printMagenta(" {in}\n");
								}

								this->indenter.set_end();
								this->print_attribute_block(this->ast_buffer.getAttributeBlock(param.attributeBlock));

								this->indenter.pop();
							}

							i += 1;
						}
						this->indenter.pop();
					}

					this->indenter.set_arrow();
					this->print_attribute_block(this->ast_buffer.getAttributeBlock(func_decl.attributeBlock));

					this->indenter.print_arrow();
					this->print_minor_header("Returns");
					this->printer.println();
					{
						this->indenter.push();
						for(size_t i = 0; const panther::AST::FuncDecl::Return& return_param : func_decl.returns){
							if(i + 1 < func_decl.returns.size()){
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

								this->indenter.pop();
							}

							i += 1;
						}
						this->indenter.pop();
					}

					this->indenter.print_end();
					this->print_minor_header("Statement Block");
					this->print_block(this->ast_buffer.getBlock(func_decl.block));

					this->indenter.pop();
				}
			}


			auto print_alias_decl(const panther::AST::AliasDecl& alias_decl) -> void {
				this->indenter.print();
				this->print_major_header("Alias Declaration");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(alias_decl.ident);

					this->indenter.set_arrow();
					this->print_attribute_block(this->ast_buffer.getAttributeBlock(alias_decl.attributeBlock));

					this->indenter.print_end();
					this->print_minor_header("Type");
					this->printer.print(" ");
					this->print_type(this->ast_buffer.getType(alias_decl.type));

					this->indenter.pop();
				}
			}

			auto print_type_decl(const panther::AST::TypedefDecl& type_decl) -> void {
				this->indenter.print();
				this->print_major_header("Type Declaration");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(type_decl.ident);

					this->indenter.set_arrow();
					this->print_attribute_block(this->ast_buffer.getAttributeBlock(type_decl.attributeBlock));

					this->indenter.print_end();
					this->print_minor_header("Type");
					this->printer.print(" ");
					this->print_type(this->ast_buffer.getType(type_decl.type));

					this->indenter.pop();
				}
			}


			auto print_struct_decl(const panther::AST::StructDecl& struct_decl) -> void {
				this->indenter.print();
				this->print_major_header("Struct Declaration");

				{
					this->indenter.push();

					this->indenter.print_arrow();
					this->print_minor_header("Identifier");
					this->printer.print(" ");
					this->print_ident(struct_decl.ident);

					this->indenter.print_arrow();
					this->print_template_pack(struct_decl.templatePack);

					this->print_attribute_block(this->ast_buffer.getAttributeBlock(struct_decl.attributeBlock));

					this->indenter.print_end();
					this->print_minor_header("Statement Block");
					this->print_block(this->ast_buffer.getBlock(struct_decl.block));

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
							this->printer.printlnGray(" [...]");

						}else{
							static_assert(sizeof(ValueT) < 0, "Unknown or unsupported return value kind");
						}
					});


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
					this->print_minor_header("Else");
					if(else_block.has_value()){
						this->print_block(this->source.getASTBuffer().getBlock(*else_block));
					}else{
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
					this->print_minor_header("Label Explicit Type");
					this->printer.print(" ");
					if(block.labelExplicitType.has_value()){
						this->print_type(this->source.getASTBuffer().getType(*block.labelExplicitType));
					}else{
						this->printer.printlnGray("{NONE}");
					}

				}else{
					this->printer.printGray(" {NONE}\n");
				}


				this->indenter.print_end();
				this->print_minor_header("Statements");

				if(block.stmts.empty()){
					this->printer.printGray(" {EMPTY}\n");
					
				}else{
					this->printer.println();
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


				this->indenter.pop();
			}



			auto print_base_type(const panther::AST::Node& base_type) -> void {
				switch(base_type.kind()){
					case panther::AST::Kind::PrimitiveType: {
						const panther::Token::ID type_token_id = this->ast_buffer.getPrimitiveType(base_type);
						const panther::Token& type_token = this->source.getTokenBuffer()[type_token_id];

						if(type_token.kind() == panther::Token::Kind::TypeI_N){
							this->printer.printMagenta("I{}", type_token.getBitWidth());
						}else if(type_token.kind() == panther::Token::Kind::TypeUI_N){
							this->printer.printMagenta("UI{}", type_token.getBitWidth());
						}else{
							this->printer.printMagenta("{}", type_token.kind());
						}

					} break;

					case panther::AST::Kind::Ident: {
						const panther::Token::ID type_token_id = this->ast_buffer.getIdent(base_type);
						this->printer.printMagenta("{}", this->source.getTokenBuffer()[type_token_id].getString());
					} break;

					case panther::AST::Kind::Intrinsic: {
						const panther::Token::ID type_token_id = this->ast_buffer.getIntrinsic(base_type);
						this->printer.printMagenta("@{}", this->source.getTokenBuffer()[type_token_id].getString());
					} break;

					case panther::AST::Kind::Infix: {
						const panther::AST::Infix& infix = this->ast_buffer.getInfix(base_type);
						print_base_type(infix.lhs);
						this->printer.printMagenta(".");
						print_base_type(infix.rhs);
					} break;


					// TODO: print this properly
					case panther::AST::Kind::TemplatedExpr: {
						const panther::AST::TemplatedExpr& templated_expr = 
							this->ast_buffer.getTemplatedExpr(base_type);
						print_base_type(templated_expr.base);
						this->printer.printMagenta("<{");
						this->printer.printGray("...{} args...", templated_expr.args.size());
						this->printer.printMagenta("}>");
					} break;

					// TODO: print this properly
					case panther::AST::Kind::TypeIDConverter: {
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
						if(qualifier.isOptional){ qualifier_str += '?'; }
					}

					this->printer.printMagenta(qualifier_str);
				}

				if(type.base.kind() == panther::AST::Kind::PrimitiveType){
					this->printer.printGray(" {PRIMITIVE}\n");
				}else{
					this->printer.println();
				}
			}


			auto print_expr(const panther::AST::Node& node) -> void {
				this->indenter.print();

				switch(node.kind()){
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

					case panther::AST::Kind::TemplatedExpr: {
						this->print_templated_expr(this->ast_buffer.getTemplatedExpr(node));
					} break;

					case panther::AST::Kind::Block: {
						this->print_major_header("Statement Block");
						this->print_block(this->ast_buffer.getBlock(node), false);
					} break;

					case panther::AST::Kind::Type: {
						this->print_type(this->ast_buffer.getType(node));
					} break;

					case panther::AST::Kind::New: {
						this->print_new(this->ast_buffer.getNew(node));
					} break;

					case panther::AST::Kind::Literal: {
						const panther::Token::ID token_id = this->ast_buffer.getLiteral(node);
						const panther::Token& token = this->source.getTokenBuffer()[token_id];

						switch(token.kind()){
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
								this->printer.printMagenta("\"{}\"", token.getString());
								this->printer.printGray(" {LiteralString}");
							} break;

							case panther::Token::Kind::LiteralChar: {
								const std::string_view str = token.getString();
								this->printer.printMagenta("'{}'", token.getString());
								this->printer.printGray(" {LiteralChar}");
							} break;

							case panther::Token::Kind::KeywordNull: {
								this->printer.printMagenta("[null]");
							} break;

							break; default: evo::debugFatalBreak("Unknown token kind");
						}

						this->printer.println();
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

					case panther::AST::Kind::Zeroinit: {
						this->printer.printMagenta("[zeroinit]\n");
					} break;

					case panther::AST::Kind::This: {
						this->printer.printMagenta("[this]\n");
					} break;

					case panther::AST::Kind::Discard: {
						this->printer.printMagenta("[_]\n");
					} break;


					default: evo::debugFatalBreak("Unknown or unsupported expr type");
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
								case panther::AST::Kind::Ident: {
									this->printer.print(" ");
									this->print_ident(assign);
								} break;

								case panther::AST::Kind::Discard: {
									this->printer.printMagenta(" [_]\n");
								} break;

								default: {
									evo::debugFatalBreak("Unknown or unsupported multi-assignment kind");
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

							this->indenter.print_end();
							this->print_minor_header("Type");
							this->printer.print(" ");
							this->print_type(this->ast_buffer.getType(param.type));

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

				this->indenter.print_end();
				this->print_minor_header("Arguments");
				this->print_func_call_args(new_expr.args);

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
								this->print_minor_header("Argument");
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
				evo::debugAssert(ident_tok.kind() == panther::Token::Kind::Ident);
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



	//////////////////////////////////////////////////////////////////////
	// DG

	#if defined(PCIT_CONFIG_DEBUG)

		auto print_DG(core::Printer& printer, const panther::Context& context, const fs::path& relative_dir) -> void {
			const panther::SourceManager& source_manager = context.getSourceManager();
			const panther::DGBuffer& dg_buffer = context.getDGBuffer();

			auto indenter = Indenter(printer);


			for(const panther::DG::Node& dg_node : dg_buffer){
				const panther::Source& source = source_manager[dg_node.sourceID];
				const panther::TokenBuffer& token_buffer = source.getTokenBuffer();
				const panther::ASTBuffer& ast_buffer = source.getASTBuffer();

				print_file_header(printer, source, relative_dir, "DG Node in");

				indenter.push();
				indenter.print_arrow();

				printer.printCyan("Kind");
				printer.printGray(": ");

				switch(dg_node.astNode.kind()){
					case panther::AST::Kind::None: {
						evo::debugFatalBreak("Invalid AST node");
					} break;
					
					case panther::AST::Kind::VarDecl: {
						const panther::AST::VarDecl& var_decl = ast_buffer.getVarDecl(dg_node.astNode);
						printer.printlnGray("VarDecl");
						
						indenter.print_arrow();
						printer.printCyan("Name");
						printer.printGray(": ");
						printer.printlnGray(token_buffer[var_decl.ident].getString());
					} break;

					case panther::AST::Kind::FuncDecl: {
						const panther::AST::FuncDecl& func_decl = ast_buffer.getFuncDecl(dg_node.astNode);
						printer.printlnGray("FuncDecl");
						if(func_decl.name.kind() == panther::AST::Kind::Ident){
							
							indenter.print_arrow();
							printer.printCyan("Name");
							printer.printGray(": ");
							printer.printlnGray(token_buffer[ast_buffer.getIdent(func_decl.name)].getString());
						}else{
							evo::debugFatalBreak("UNIMPLEMENTED");
						}
					} break;

					case panther::AST::Kind::AliasDecl: {
						const panther::AST::AliasDecl& alias_decl = ast_buffer.getAliasDecl(dg_node.astNode);
						printer.printlnGray("AliasDecl");
						
						indenter.print_arrow();
						printer.printCyan("Name");
						printer.printGray(": ");
						printer.printlnGray(token_buffer[alias_decl.ident].getString());
					} break;

					case panther::AST::Kind::TypedefDecl: {
						const panther::AST::TypedefDecl& typedef_decl = ast_buffer.getTypedefDecl(dg_node.astNode);
						printer.printlnGray("TypedefDecl");
						
						indenter.print_arrow();
						printer.printCyan("Name");
						printer.printGray(": ");
						printer.printlnGray(token_buffer[typedef_decl.ident].getString());
					} break;

					case panther::AST::Kind::StructDecl: {
						const panther::AST::StructDecl& struct_decl = ast_buffer.getStructDecl(dg_node.astNode);
						printer.printlnGray("StructDecl");
						
						indenter.print_arrow();
						printer.printCyan("Name");
						printer.printGray(": ");
						printer.printlnGray(token_buffer[struct_decl.ident].getString());
					} break;

					case panther::AST::Kind::WhenConditional: {
						// const panther::AST::WhenConditional& when_conditional = 
						// 	ast_buffer.getWhenConditional(dg_node.astNode);
						printer.printlnGray("WhenConditional");
					} break;

					default: evo::debugFatalBreak("Unknown DG node kind");
				}

				indenter.print_arrow();
				printer.printCyan("Usage Kind");
				printer.printGray(": ");
				switch(dg_node.usageKind){
					break; case panther::DG::Node::UsageKind::Comptime:  printer.printlnGray("Comptime");
					break; case panther::DG::Node::UsageKind::Constexpr: printer.printlnGray("SafeInComptime");
					break; case panther::DG::Node::UsageKind::Runtime:   printer.printlnGray("Runtime");
					break; case panther::DG::Node::UsageKind::Unknown:   printer.printlnGray("Unknown");
				}


				auto print_ids = [&](std::string_view title, const auto& container, bool print_title_cyan) -> void {
					indenter.print();

					if(print_title_cyan){
						printer.printCyan(title);
					}else{
						printer.printBlue(title);
					}
					printer.printlnGray(": {}", std::to_string(container.size()));

					indenter.push();

					for(size_t i = 0; const panther::DG::Node::ID& required_by_id : container){
						EVO_DEFER([&](){ i += 1; });

						if(i + 1 < container.size()){
							indenter.print_arrow();
						}else{
							indenter.print_end();
						}

						const panther::DG::Node& required_by_node = dg_buffer[required_by_id];
						
						switch(required_by_node.astNode.kind()){
							case panther::AST::Kind::None: {
								evo::debugFatalBreak("Invalid AST node");
							} break;
							
							case panther::AST::Kind::VarDecl: {
								const panther::AST::VarDecl& var_decl = ast_buffer.getVarDecl(required_by_node.astNode);
								printer.printlnGray("VarDecl: {}", token_buffer[var_decl.ident].getString());
							} break;

							case panther::AST::Kind::FuncDecl: {
								const panther::AST::FuncDecl& func_decl = 
									ast_buffer.getFuncDecl(required_by_node.astNode);
								if(func_decl.name.kind() == panther::AST::Kind::Ident){
									printer.printlnGray(
										"FuncDecl: {}", token_buffer[ast_buffer.getIdent(func_decl.name)].getString()
									);
								}else{
									evo::debugFatalBreak("UNIMPLEMENTED");
								}
							} break;

							case panther::AST::Kind::AliasDecl: {
								const panther::AST::AliasDecl& alias_decl =
									ast_buffer.getAliasDecl(required_by_node.astNode);
								printer.printCyan("AliasDecl: {}", token_buffer[alias_decl.ident].getString());
							} break;

							case panther::AST::Kind::TypedefDecl: {
								const panther::AST::TypedefDecl& typedef_decl =
									ast_buffer.getTypedefDecl(required_by_node.astNode);
								printer.printCyan("TypedefDecl: {}", token_buffer[typedef_decl.ident].getString());
							} break;

							case panther::AST::Kind::StructDecl: {
								const panther::AST::StructDecl& struct_decl =
									ast_buffer.getStructDecl(required_by_node.astNode);
								printer.printCyan("StructDecl: {}", token_buffer[struct_decl.ident].getString());
							} break;

							case panther::AST::Kind::WhenConditional: {
								// const panther::AST::WhenConditional& when_conditional = 
								// 	ast_buffer.getWhenConditional(required_by_node.astNode);
								printer.printlnGray("WhenConditional");
							} break;

							default: evo::debugFatalBreak("Unknown DG node kind");
						}
					}

					indenter.pop();
				};


				indenter.print_arrow();
				printer.printCyan("Deps");
				printer.printlnGray(": ");
				indenter.push();

				indenter.set_arrow();
				print_ids("declDeps.decls", dg_node.declDeps.decls, false);
				indenter.set_arrow();
				print_ids("declDeps.defs", dg_node.declDeps.defs, false);
				indenter.set_arrow();
				print_ids("defDeps.decls", dg_node.defDeps.decls, false);
				indenter.set_end();
				print_ids("defDeps.defs", dg_node.defDeps.defs, false);

				indenter.pop();

				indenter.set_end();
				print_ids("requiredBy", dg_node.requiredBy, true);

				indenter.pop();
			}
		}
	#endif


	
}
