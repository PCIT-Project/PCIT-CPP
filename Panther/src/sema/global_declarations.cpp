//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#include "./global_declarations.h"

#include "../../include/Context.h"
#include "../../include/AST.h"
#include "./get_source_location.h"


namespace pcit::panther::sema{

	
	auto analyze_global_declarations(Context& context, SourceID source_id) -> bool {
		const Source& source = context.getSourceManager()[source_id];

		for(const AST::Node& global_stmt : source.getASTBuffer().getGlobalStmts()){

			#if defined(EVO_COMPILER_MSVC)
				#pragma warning(default : 4062)
			#endif

			switch(global_stmt.getKind()){
				case AST::Kind::None: {
					context.emitFatal(
						Diagnostic::Code::SemaEncounteredKindNone,
						std::nullopt,
						Diagnostic::createFatalMessage("Encountered AST node kind of None")
					);
					return false;
				} break;

				case AST::Kind::VarDecl: {
					// evo::log::info("global variable");
				} break;

				case AST::Kind::FuncDecl: {
					// evo::log::info("Func");
				} break;

				case AST::Kind::AliasDecl: {
					// evo::log::info("alias");
				} break;


				case AST::Kind::Return:        case AST::Kind::Block: case AST::Kind::FuncCall:
				case AST::Kind::TemplatedExpr: case AST::Kind::Infix: case AST::Kind::Postfix:
				case AST::Kind::MultiAssign:   case AST::Kind::Ident: case AST::Kind::Intrinsic:
				case AST::Kind::Literal:       case AST::Kind::This: {
					context.emitError(
						Diagnostic::Code::SemaInvalidGlobalStmt,
						get_source_location(source.getASTBuffer().getReturn(global_stmt), source),
						"Invalid global statement"
					);
					return false;
				};


				case AST::Kind::TemplatePack:   case AST::Kind::Prefix:    case AST::Kind::Type:
				case AST::Kind::AttributeBlock: case AST::Kind::Attribute: case AST::Kind::BuiltinType:
				case AST::Kind::Uninit:         case AST::Kind::Discard: {
					context.emitFatal(
						Diagnostic::Code::SemaInvalidGlobalStmt,
						std::nullopt,
						Diagnostic::createFatalMessage("Invalid global statement: TemplatePack")
					);
					return false;
				};

			}
		}

		return true;
	}


}