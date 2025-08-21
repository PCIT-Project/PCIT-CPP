////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#include "../include/clang_run.h"


#include <Clang.h>

#include "../include/Type.h"

#include "./extract_macros.h"

namespace pcit::clangint{


	class DiagnosticConsumer : public clang::DiagnosticConsumer{
		public:
			DiagnosticConsumer(DiagnosticList& _diag_list) : diag_list(_diag_list) {}
			~DiagnosticConsumer() = default;


			void HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic& info) override {
				auto str = llvm::SmallVector<char>();
				info.FormatDiagnostic(str);


				const DiagnosticList::Diagnostic::Level list_level = [&](){
					using ClangLevel = clang::DiagnosticsEngine::Level;

					switch(level){
						case ClangLevel::Ignored: return DiagnosticList::Diagnostic::Level::IGNORED;
						case ClangLevel::Note:    return DiagnosticList::Diagnostic::Level::NOTE;
						case ClangLevel::Remark:  return DiagnosticList::Diagnostic::Level::REMARK;
						case ClangLevel::Warning: return DiagnosticList::Diagnostic::Level::WARNING;
						case ClangLevel::Error:   return DiagnosticList::Diagnostic::Level::ERROR;
						case ClangLevel::Fatal:   return DiagnosticList::Diagnostic::Level::FATAL;
					}

					evo::unreachable();
				}();


				if(this->source_manager != nullptr){
					const clang::PresumedLoc presumed_loc = this->source_manager->getPresumedLoc(info.getLocation());

					// this->source_manager->getFullLoc(info.getLocation()).getSpellingLineNumber(),

					if(presumed_loc.isValid()){
						this->diag_list.diagnostics.emplace_back(
							std::string(str.data(), str.size()),
							list_level,
							DiagnosticList::Diagnostic::Location(
								std::filesystem::path(std::string(presumed_loc.getFilename())),
								presumed_loc.getLine(),
								presumed_loc.getColumn()
							)
						);
						
					}else{
						this->diag_list.diagnostics.emplace_back(
							std::string(str.data(), str.size()), list_level, std::nullopt
						);
					}

				}else{
					this->diag_list.diagnostics.emplace_back(
						std::string(str.data(), str.size()), list_level, std::nullopt
					);
				}
			}


			auto set_source_manager(clang::SourceManager* _source_manager) -> void {
				evo::debugAssert(this->source_manager == nullptr, "SourceManager already set");
				this->source_manager = _source_manager;
			}
	
		private:
			DiagnosticList& diag_list;
			clang::SourceManager* source_manager = nullptr;
	};



	class StringOStream : public llvm::raw_pwrite_stream {
	    public:
	        StringOStream(std::string& _output) : output(_output) {
	            this->SetUnbuffered();
	        };
	        ~StringOStream() = default;

	
	    public:
	        std::string& output;

	    private:
	        void write_impl(const char* ptr, size_t size) noexcept override {
	            this->current_size += size;
	            this->output += std::string_view(ptr, size);
	        };

	        void pwrite_impl(const char* ptr, size_t size, uint64_t offset) noexcept override {
	        	this->write_impl(ptr, size);
	        	std::ignore = offset;
	        }

	        uint64_t current_pos() const override { return this->current_size; }

	    private:
	        size_t current_size = 0;
	};





	EVO_NODISCARD static auto make_type(clang::QualType qual_type, API& api) -> Type {
		auto qualifiers = evo::SmallVector<Type::Qualifier>();

		bool is_const = false;

		clang::QualType target_qual_type = qual_type;
		while(true){
			auto split = target_qual_type.split();
			
			switch(split.Ty->getTypeClass()){
				case clang::Type::Decayed: {
					target_qual_type = split.Ty->getPointeeType();

					if(qual_type.isConstQualified()){
						qualifiers.emplace(qualifiers.begin(), Type::Qualifier::CONST_POINTER);
					}else{
						qualifiers.emplace(qualifiers.begin(), Type::Qualifier::POINTER);
					}
				} break;

				case clang::Type::Builtin: {
					const BaseType::Primitive builtin_kind = [&]() -> BaseType::Primitive {
						switch(clang::cast<clang::BuiltinType>(split.Ty)->getKind()){
							case clang::BuiltinType::Kind::Void:       return BaseType::Primitive::VOID;
							case clang::BuiltinType::Kind::Bool:       return BaseType::Primitive::BOOL;
							case clang::BuiltinType::Kind::Char_U:     return BaseType::Primitive::CHAR;
							case clang::BuiltinType::Kind::WChar_U:    return BaseType::Primitive::C_WCHAR;
							case clang::BuiltinType::Kind::UChar:      return BaseType::Primitive::UI8;
							case clang::BuiltinType::Kind::UShort:     return BaseType::Primitive::C_USHORT;
							case clang::BuiltinType::Kind::UInt:       return BaseType::Primitive::C_UINT;
							case clang::BuiltinType::Kind::ULong:      return BaseType::Primitive::C_ULONG;
							case clang::BuiltinType::Kind::ULongLong:  return BaseType::Primitive::C_ULONG_LONG;
							case clang::BuiltinType::Kind::UInt128:    return BaseType::Primitive::UI128;
							case clang::BuiltinType::Kind::Char_S:     return BaseType::Primitive::CHAR;
							case clang::BuiltinType::Kind::SChar:      return BaseType::Primitive::I8;
							case clang::BuiltinType::Kind::WChar_S:    return BaseType::Primitive::C_WCHAR;
							case clang::BuiltinType::Kind::Short:      return BaseType::Primitive::C_SHORT;
							case clang::BuiltinType::Kind::Int:        return BaseType::Primitive::C_INT;
							case clang::BuiltinType::Kind::Long:       return BaseType::Primitive::C_LONG;
							case clang::BuiltinType::Kind::LongLong:   return BaseType::Primitive::C_LONG_LONG;
							case clang::BuiltinType::Kind::Int128:     return BaseType::Primitive::I128;
							case clang::BuiltinType::Kind::Half:       return BaseType::Primitive::F16;
							case clang::BuiltinType::Kind::Float:      return BaseType::Primitive::F32;
							case clang::BuiltinType::Kind::Double:     return BaseType::Primitive::F64;
							case clang::BuiltinType::Kind::LongDouble: return BaseType::Primitive::C_LONG_DOUBLE;
							case clang::BuiltinType::Kind::Float16:    return BaseType::Primitive::F16;
							case clang::BuiltinType::Kind::BFloat16:   return BaseType::Primitive::BF16;
							case clang::BuiltinType::Kind::Float128:   return BaseType::Primitive::F128;
							default:                                   return BaseType::Primitive::UNKNOWN;
						}
					}();

					return Type(builtin_kind, std::move(qualifiers), split.Quals.hasConst());
				} break;

				case clang::Type::Decltype: {
					const clang::DecltypeType& decltype_type = *clang::cast<clang::DecltypeType>(split.Ty);
					target_qual_type = decltype_type.getUnderlyingType();
				} break;

				case clang::Type::Elaborated: {
					const clang::ElaboratedType& elaborated_type = *clang::cast<clang::ElaboratedType>(split.Ty);

					if(elaborated_type.getQualifier() == nullptr){
						target_qual_type = elaborated_type.getNamedType();
						is_const = split.Quals.hasConst();
						break;
					}

					evo::unimplemented("Nested Name specifier of clang Elaborated type"); // namespaces IIRC
				} break;

				case clang::Type::FunctionProto: {
					const clang::FunctionProtoType& function_proto_type = 
						*clang::cast<clang::FunctionProtoType>(split.Ty);

					const clang::FunctionType* function_type = function_proto_type.castAs<clang::FunctionType>();

					auto types = evo::SmallVector<Type>();
					types.reserve(function_proto_type.getNumParams() + 1);
					types.emplace_back(make_type(function_type->getReturnType(), api));
					for(const clang::QualType& param_type : function_proto_type.getParamTypes()){
						types.emplace_back(make_type(param_type, api));
					}

					const bool is_no_throw = function_proto_type.isNothrow();
					const bool is_variadic = function_proto_type.isVariadic();
					const bool func_is_const = function_type->isConst();

					return Type(
						BaseType::Function(std::move(types), !is_no_throw, is_variadic, func_is_const),
						std::move(qualifiers),
						split.Quals.hasConst()
					);
				} break;

				case clang::Type::Attributed: {
					const clang::AttributedType& attributed_type = *clang::cast<clang::AttributedType>(split.Ty);
					
					target_qual_type = attributed_type.getEquivalentType();
				} break;

				case clang::Type::Paren: {
					const clang::ParenType& paren_type = *clang::cast<clang::ParenType>(split.Ty);
					
					target_qual_type = paren_type.getInnerType();
				} break;

				case clang::Type::Pointer: {
					target_qual_type = split.Ty->getPointeeType();

					if(target_qual_type.split().Ty->getTypeClass() == clang::Type::Builtin){
						const clang::BuiltinType* builtin =clang::cast<clang::BuiltinType>(target_qual_type.split().Ty);
						if(builtin->getKind() == clang::BuiltinType::Kind::Void){
							return Type(BaseType::Primitive::RAWPTR, std::move(qualifiers), split.Quals.hasConst());
						}
					}

					if(qual_type.isConstQualified()){
						qualifiers.emplace(qualifiers.begin(), Type::Qualifier::CONST_POINTER);
					}else{
						qualifiers.emplace(qualifiers.begin(), Type::Qualifier::POINTER);
					}
				} break;

				case clang::Type::LValueReference: {
					qualifiers.emplace(qualifiers.begin(), Type::Qualifier::L_VALUE_REFERENCE);
					target_qual_type = split.Ty->getPointeeType();
				} break;

				case clang::Type::RValueReference: {
					qualifiers.emplace(qualifiers.begin(), Type::Qualifier::R_VALUE_REFERENCE);
					target_qual_type = split.Ty->getPointeeType();
				} break;

				case clang::Type::Enum: {
					clang::EnumDecl* enum_decl = clang::cast<clang::EnumDecl>(split.Ty->getAsTagDecl());

					return Type(
						BaseType::NamedDecl(enum_decl->getNameAsString(), BaseType::NamedDecl::Kind::ENUM),
						std::move(qualifiers),
						split.Quals.hasConst()
					);
				} break;

				case clang::Type::Record: {
					clang::RecordDecl* record_decl = split.Ty->getAsRecordDecl();

					if(record_decl->getNameAsString().empty()){
						return Type(BaseType::Primitive::UNKNOWN, std::move(qualifiers), split.Quals.hasConst());
					}

					if(record_decl->isUnion()){
						return Type(
							BaseType::NamedDecl(record_decl->getNameAsString(), BaseType::NamedDecl::Kind::UNION),
							std::move(qualifiers),
							split.Quals.hasConst()
						);
					}else{
						return Type(
							BaseType::NamedDecl(record_decl->getNameAsString(), BaseType::NamedDecl::Kind::STRUCT),
							std::move(qualifiers),
							split.Quals.hasConst()
						);
					}
				} break;

				case clang::Type::Typedef: {
					const clang::TypedefType* typedef_type = clang::cast<clang::TypedefType>(split.Ty);
					const clang::TypedefNameDecl* typedef_name_decl = typedef_type->getDecl();

					const clang::DeclarationName declaration_name = typedef_name_decl->getDeclName();
					
					if(declaration_name.isIdentifier()){
						const std::optional<BaseType::Primitive> lookup = api.lookupSpecialPrimitive(
							declaration_name.getAsIdentifierInfo()->getName()
						);

						if(lookup.has_value()){
							return Type(*lookup, std::move(qualifiers), split.Quals.hasConst() || is_const);
						}
					}

					target_qual_type = typedef_name_decl->getUnderlyingType();
				} break;

				default: {
					return Type(BaseType::Primitive::UNKNOWN, std::move(qualifiers), split.Quals.hasConst());
				} break;
			}
		}

	}







	class ExtractAPIVisitor : public clang::RecursiveASTVisitor<ExtractAPIVisitor> {
		public:
			explicit ExtractAPIVisitor(const clang::SourceManager& src_manager, API& _api)
				: source_manager(src_manager), api(_api) {}


			auto VisitFunctionDecl(clang::FunctionDecl* func_decl) -> bool {
				if(func_decl->getDeclContext()->isRecord()){ return true; } // skip members

				auto params = evo::SmallVector<API::Function::Param>();
				params.reserve(func_decl->parameters().size());
				for(const clang::ParmVarDecl* param : func_decl->parameters()){
					const clang::PresumedLoc presumed_loc = this->source_manager.getPresumedLoc(param->getLocation());

					params.emplace_back(
						param->getNameAsString(), uint32_t(presumed_loc.getLine()), uint32_t(presumed_loc.getColumn())
					);
				}

				const clang::PresumedLoc presumed_loc = this->source_manager.getPresumedLoc(func_decl->getLocation());

				this->api.addFunction(
					func_decl->getNameAsString(),
					make_type(func_decl->getType(), this->api).baseType.as<BaseType::Function>(),
					std::move(params),
					func_decl->isNoReturn(),
					std::filesystem::path(std::string(presumed_loc.getFilename())),
					uint32_t(presumed_loc.getLine()),
					uint32_t(presumed_loc.getColumn())
				);

				return true;
			}


			auto VisitTypedefDecl(clang::TypedefDecl* typedef_decl) -> bool {
				if(typedef_decl->getDeclContext()->isRecord()){ return true; } // skip members
				if(typedef_decl->getParentFunctionOrMethod() != nullptr){ return true; } // skip scoped to functions
				
				const clang::PresumedLoc presumed_loc =
					this->source_manager.getPresumedLoc(typedef_decl->getLocation());

				this->api.addAlias(
					typedef_decl->getNameAsString(),
					make_type(typedef_decl->getUnderlyingType(), this->api),
					std::filesystem::path(std::string(presumed_loc.getFilename())),
					uint32_t(presumed_loc.getLine()),
					uint32_t(presumed_loc.getColumn())
				);

				return true;
			}

			auto VisitTypeAliasDecl(clang::TypeAliasDecl* type_alias_decl) -> bool {
				if(type_alias_decl->getDeclContext()->isRecord()){ return true; } // skip members
				if(type_alias_decl->getParentFunctionOrMethod() != nullptr){ return true; } // skip scoped to functions


				const clang::PresumedLoc presumed_loc =
					this->source_manager.getPresumedLoc(type_alias_decl->getLocation());

				this->api.addAlias(
					type_alias_decl->getNameAsString(),
					make_type(type_alias_decl->getUnderlyingType(), this->api),
					std::filesystem::path(std::string(presumed_loc.getFilename())),
					uint32_t(presumed_loc.getLine()),
					uint32_t(presumed_loc.getColumn())
				);

				return true;
			}



			auto VisitEnumDecl(clang::EnumDecl* enum_decl) -> bool {
				if(enum_decl->getDeclContext()->isRecord()){ return true; } // skip members

				// const bool is_class = enum_decl->isScopedUsingClassTag();

				// TODO(FUTURE): handle enums properly

				return true;
			}


			auto VisitRecordDecl(clang::RecordDecl* record_decl) -> bool {
				if(record_decl->getDeclContext()->isRecord()){ return true; } // skip members
				if(record_decl->getParentFunctionOrMethod() != nullptr){ return true; } // skip scoped to functions

				std::string name = record_decl->getNameAsString();
				if(name.empty()){ return true; } // skip unnaned types

				if(record_decl->isUnion()){ return this->visit_union(record_decl, std::move(name)); }

				bool is_class = false;
				if(record_decl->isStruct() == false){
					if(record_decl->isClass() == false){
						return true;
					}else{
						is_class = true;
					}
				}


				auto members = evo::SmallVector<API::Struct::Member>();
				members.reserve(std::distance(record_decl->fields().begin(), record_decl->fields().end()));
				for(const clang::FieldDecl* field : record_decl->fields()){
					auto field_name = std::string();
					{
						auto field_name_ostream = StringOStream(field_name);
						field->printName(field_name_ostream, clang::PrintingPolicy(clang::LangOptions()));
					}

					const clang::QualType field_type = field->getType();

					const API::Struct::Member::Access access = [&](){
						switch(field->getAccess()){
							case clang::AS_public:    return API::Struct::Member::Access::PUBLIC;
							case clang::AS_protected: return API::Struct::Member::Access::PROTECTED;
							case clang::AS_private:   return API::Struct::Member::Access::PRIVATE;
							case clang::AS_none:      evo::debugFatalBreak("Unknown struct member access");
						}
						evo::debugFatalBreak("Unknown clang member access kind");
					}();

					const clang::PresumedLoc presumed_loc = this->source_manager.getPresumedLoc(field->getLocation());

					members.emplace_back(
						field_name,
						make_type(field_type, this->api),
						access,
						uint32_t(presumed_loc.getLine()),
						uint32_t(presumed_loc.getColumn())
					);
				}

				const clang::PresumedLoc presumed_loc = this->source_manager.getPresumedLoc(record_decl->getLocation());

				this->api.addStruct(
					std::move(name),
					std::move(members),
					std::filesystem::path(std::string(presumed_loc.getFilename())),
					uint32_t(presumed_loc.getLine()),
					uint32_t(presumed_loc.getColumn())
				);

				return true;
			}



			auto visit_union(clang::RecordDecl* union_decl, std::string&& name) -> bool {								
				if(union_decl->getDeclContext()->isRecord()){ return true; } // skip members
				if(union_decl->getParentFunctionOrMethod() != nullptr){ return true; } // skip scoped to functions

				auto fields = evo::SmallVector<API::Union::Field>();
				fields.reserve(std::distance(union_decl->fields().begin(), union_decl->fields().end()));
				for(const clang::FieldDecl* field : union_decl->fields()){
					auto field_name = std::string();
					{
						auto field_name_ostream = StringOStream(field_name);
						field->printName(field_name_ostream, clang::PrintingPolicy(clang::LangOptions()));
					}

					const clang::QualType field_type = field->getType();


					const clang::PresumedLoc presumed_loc = this->source_manager.getPresumedLoc(field->getLocation());

					fields.emplace_back(
						field_name,
						make_type(field_type, this->api),
						uint32_t(presumed_loc.getLine()),
						uint32_t(presumed_loc.getColumn())
					);
				}

				const clang::PresumedLoc presumed_loc = this->source_manager.getPresumedLoc(union_decl->getLocation());

				this->api.addUnion(
					std::move(name),
					std::move(fields),
					std::filesystem::path(std::string(presumed_loc.getFilename())),
					uint32_t(presumed_loc.getLine()),
					uint32_t(presumed_loc.getColumn())
				);

				return true;
			}


		private:
			const clang::SourceManager& source_manager;
			API& api;
	};


	class ExtractAPIConsumer : public clang::ASTConsumer {
		public:
			explicit ExtractAPIConsumer(const clang::SourceManager& source_manager, API& api)
				: visitor(source_manager, api) {}

			virtual void HandleTranslationUnit(clang::ASTContext& context) {
				this->visitor.TraverseDecl(context.getTranslationUnitDecl());
			}

		private:
			ExtractAPIVisitor visitor;
	};

	class ExtractAPIAction : public clang::ASTFrontendAction {
		public:
			ExtractAPIAction(API& api, DiagnosticConsumer& diagnostic_consumer)
				: _api(api), _diagnostic_consumer(diagnostic_consumer) {}


			virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
				clang::CompilerInstance& clang_instance, llvm::StringRef in_file
			){
				std::ignore = in_file;
				this->_diagnostic_consumer.set_source_manager(&clang_instance.getASTContext().getSourceManager());

				return std::unique_ptr<clang::ASTConsumer>(
					new ExtractAPIConsumer(clang_instance.getASTContext().getSourceManager(), this->_api)
				);
			}


		private:
			API& _api;
			DiagnosticConsumer& _diagnostic_consumer;
	};




	EVO_NODISCARD static auto get_compiler_args(
		const std::string& file_name, evo::Variant<COpts, CPPOpts> opts, core::Target target
	) -> evo::SmallVector<const char*> {
		auto args = evo::SmallVector<const char*>{
			file_name.c_str(),
			"-Wall", "-Wextra", // TODO(FUTURE): figure out why these don't seem to do anything
			"-D__GNUC__",
		};

		opts.visit([&](const auto& opts) -> void {
			using OptsType = std::decay_t<decltype(opts)>;

			if constexpr(std::is_same<OptsType, COpts>()){
				args.emplace_back("-x");
				args.emplace_back("c");

				switch(opts.standard){
					break; case COpts::Standard::LATEST: args.emplace_back("-std=c23");
					break; case COpts::Standard::C23:    args.emplace_back("-std=c23");
					break; case COpts::Standard::C17:    args.emplace_back("-std=c17");
					break; case COpts::Standard::C11:    args.emplace_back("-std=c11");
					break; case COpts::Standard::C99:    args.emplace_back("-std=c99");
					break; case COpts::Standard::C89:    args.emplace_back("-std=c89");
				}
				
			}else if constexpr(std::is_same<OptsType, CPPOpts>()){
				args.emplace_back("-x");
				args.emplace_back("c++");

				switch(opts.standard){
					break; case CPPOpts::Standard::LATEST: args.emplace_back("-std=c++23");
					break; case CPPOpts::Standard::CPP23:  args.emplace_back("-std=c++23");
					break; case CPPOpts::Standard::CPP20:  args.emplace_back("-std=c++20");
					break; case CPPOpts::Standard::CPP17:  args.emplace_back("-std=c++17");
					break; case CPPOpts::Standard::CPP14:  args.emplace_back("-std=c++14");
					break; case CPPOpts::Standard::CPP11:  args.emplace_back("-std=c++11");
				}
				
			}else{
				static_assert(false, "Unkonwn language opts");
			}
		});

		args.emplace_back("-I");
		args.emplace_back("../extern/libc/include/any");

		switch(target.platform){
			case core::Target::Platform::WINDOWS: {
				args.emplace_back("-I");
				args.emplace_back("../extern/libc/include/any-windows-any");

				args.emplace_back("-fms-extensions");
			} break;

			case core::Target::Platform::LINUX: {
				args.emplace_back("-I");
				args.emplace_back("../extern/libc/include/any-linux-any");

				args.emplace_back("-I");
				args.emplace_back("../extern/libc/include/generic-glibc");

				switch(target.architecture){
					case core::Target::Architecture::X86_64: {
						args.emplace_back("-I");
						args.emplace_back("../extern/libc/include/x86+64-linux-gnu");
					} break;

					case core::Target::Architecture::UNKNOWN: {
						// do nothing...
					} break;
				}
			} break;

			case core::Target::Platform::UNKNOWN: {
				// do nothing...
			} break;
		}

		return args;
	}





	auto getHeaderAPI(
		const std::string& file_name,
		std::string_view file_data,
		evo::Variant<COpts, CPPOpts> opts,
		core::Target target,
		DiagnosticList& diagnostic_list,
		API& api
	) -> evo::Result<> {
		auto diagnostic_ids = llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs>(new clang::DiagnosticIDs());

		auto diagnostic_options = clang::DiagnosticOptions();
		auto diagnostic_consumer = DiagnosticConsumer(diagnostic_list);

		const evo::SmallVector<const char*> args = get_compiler_args(file_name, opts, target);
		

		// will be given ownership to clang by `setDiagnostics`
		clang::DiagnosticsEngine* diagnostics_engine =
			new clang::DiagnosticsEngine(diagnostic_ids, diagnostic_options, &diagnostic_consumer, false);

		std::shared_ptr<clang::CompilerInvocation> compiler_invocation = std::make_shared<clang::CompilerInvocation>();
		clang::CompilerInvocation::CreateFromArgs(*compiler_invocation, args, *diagnostics_engine);

		std::unique_ptr<llvm::MemoryBuffer> file_buffer = llvm::MemoryBuffer::getMemBufferCopy(file_data);
		compiler_invocation->getPreprocessorOpts().addRemappedFile(file_name, file_buffer.get());

		EVO_DEFER([&](){ file_buffer.release(); });


		auto clang_instance = clang::CompilerInstance(compiler_invocation);
		clang_instance.setDiagnostics(diagnostics_engine);
		clang_instance.createFileManager();

		clang::TargetInfo* target_info =
			clang::TargetInfo::CreateTargetInfo(*diagnostics_engine, compiler_invocation->getTargetOpts());
		clang_instance.setTarget(target_info);

		auto output = std::string();
		clang_instance.setOutputStream(std::make_unique<StringOStream>(output));


		auto compiler_action = ExtractAPIAction(api, diagnostic_consumer);
		if(clang_instance.ExecuteAction(compiler_action) == false){ return evo::resultError; }

		extract_macros(clang_instance.getPreprocessor(), clang_instance.getSourceManager(), api);

		evo::debugAssert(output.empty(), "Clang output not empty");

		return evo::Result<>();
	}




	auto getSourceLLVM(
		const std::string& file_name,
		std::string_view file_data,
		evo::Variant<COpts, CPPOpts> opts,
		core::Target target,
		llvm::LLVMContext* llvm_context,
		DiagnosticList& diagnostic_list
	) -> evo::Result<llvm::Module*> {
		auto diagnostic_ids = llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs>(new clang::DiagnosticIDs());

		auto diagnostic_options = clang::DiagnosticOptions();
		auto diagnostic_consumer = DiagnosticConsumer(diagnostic_list);

		const evo::SmallVector<const char*> args = get_compiler_args(file_name, opts, target);
		
		// will be given ownership to clang by `setDiagnostics`
		clang::DiagnosticsEngine* diagnostics_engine =
			new clang::DiagnosticsEngine(diagnostic_ids, diagnostic_options, &diagnostic_consumer, false);

		std::shared_ptr<clang::CompilerInvocation> compiler_invocation = std::make_shared<clang::CompilerInvocation>();
		clang::CompilerInvocation::CreateFromArgs(*compiler_invocation, args, *diagnostics_engine);

		std::unique_ptr<llvm::MemoryBuffer> file_buffer = llvm::MemoryBuffer::getMemBufferCopy(file_data);
		compiler_invocation->getPreprocessorOpts().addRemappedFile(file_name, file_buffer.get());

		EVO_DEFER([&](){ file_buffer.release(); });


		auto clang_instance = clang::CompilerInstance(compiler_invocation);
		clang_instance.setDiagnostics(diagnostics_engine);
		clang_instance.createFileManager();


		clang::TargetInfo* target_info =
			clang::TargetInfo::CreateTargetInfo(*diagnostics_engine, compiler_invocation->getTargetOpts());
		clang_instance.setTarget(target_info);

		auto output = std::string();
		clang_instance.setOutputStream(std::make_unique<StringOStream>(output));


		auto ela = clang::EmitLLVMAction(llvm_context); // TODO(FUTURE): make EmitLLVMOnlyAction?
		const bool execute_result = clang_instance.ExecuteAction(ela);

		if(execute_result == false){ return evo::resultError; }
		std::unique_ptr<llvm::Module> module = ela.takeModule();

		return module.release();
	}





	
}