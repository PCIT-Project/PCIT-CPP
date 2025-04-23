////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////




#if defined(EVO_COMPILER_MSVC)
	#pragma warning(default : 4062)
#endif

namespace pcit::panther{


	class Attribute{
		public:
			Attribute(SemanticAnalyzer& _sema, std::string_view _name) : sema(_sema), name(_name) {}
			~Attribute() = default;

			EVO_NODISCARD auto is_set() const -> bool {
				return this->set_location.has_value() || this->implicitly_set_location.has_value();
			}

			EVO_NODISCARD auto set(Token::ID location) -> evo::Result<> {
				if(this->set_location.has_value()){
					this->sema.emit_error(
						Diagnostic::Code::SEMA_ATTRIBUTE_ALREADY_SET,
						location,
						std::format("Attribute #{} was already set", this->name),
						Diagnostic::Info(
							"First set here:", Diagnostic::Location::get(this->set_location.value(), this->sema.source)
						)
					);
					return evo::resultError;
				}

				if(this->implicitly_set_location.has_value()){
					// TODO(FEATURE): make this warning turn-off-able in settings
					this->sema.emit_warning(
						Diagnostic::Code::SEMA_ATTRIBUTE_IMPLICT_SET,
						location,
						std::format("Attribute #{} was already implicitly set", this->name),
						Diagnostic::Info(
							"Implicitly set here:",
							Diagnostic::Location::get(this->implicitly_set_location.value(), this->sema.source)
						)
					);
					return evo::Result<>();
				}

				this->set_location = location;
				return evo::Result<>();
			}

			EVO_NODISCARD auto implicitly_set(Token::ID location) -> void {
				if(this->set_location.has_value()){
					// TODO(FEATURE): make this warning turn-off-able in settings
					this->sema.emit_warning(
						Diagnostic::Code::SEMA_ATTRIBUTE_IMPLICT_SET,
						this->set_location.value(),
						std::format("Attribute #{} was implicitly set", this->name),
						Diagnostic::Info(
							"Implicitly set here:", Diagnostic::Location::get(location, this->sema.source)
						)
					);
					return;
				}

				evo::debugAssert(
					this->implicitly_set_location.has_value() == false,
					"Attribute #{} already implicitly set. Should this be handled? Design changed?",
					this->name
				);

				this->implicitly_set_location = location;
			}
	
		private:
			SemanticAnalyzer& sema;
			std::string_view name;
			std::optional<Token::ID> set_location{};
			std::optional<Token::ID> implicitly_set_location{};
	};




	class ConditionalAttribute{
		public:
			ConditionalAttribute(SemanticAnalyzer& _sema, std::string_view _name) : sema(_sema), name(_name) {}
			~ConditionalAttribute() = default;

			EVO_NODISCARD auto is_set() const -> bool {
				return this->is_set_true;
			}

			EVO_NODISCARD auto set(Token::ID location, bool cond) -> evo::Result<> {
				if(this->set_location.has_value()){
					this->sema.emit_error(
						Diagnostic::Code::SEMA_ATTRIBUTE_ALREADY_SET,
						location,
						std::format("Attribute #{} was already set", this->name),
						Diagnostic::Info(
							"First set here:", Diagnostic::Location::get(this->set_location.value(), this->sema.source)
						)
					);
					return evo::resultError;
				}

				if(this->implicitly_set_location.has_value()){
					// TODO(FEATURE): make this warning turn-off-able in settings
					this->sema.emit_warning(
						Diagnostic::Code::SEMA_ATTRIBUTE_IMPLICT_SET,
						location,
						std::format("Attribute #{} was already implicitly set", this->name),
						Diagnostic::Info(
							"Implicitly set here:",
							Diagnostic::Location::get(this->implicitly_set_location.value(), this->sema.source)
						)
					);
					return evo::Result<>();
				}

				this->is_set_true = cond;
				this->set_location = location;
				return evo::Result<>();
			}

			EVO_NODISCARD auto implicitly_set(Token::ID location, bool cond) -> void {
				if(this->set_location.has_value()){
					if(this->is_set_true){
						// TODO(FEATURE): make this warning turn-off-able in settings
						this->sema.emit_warning(
							Diagnostic::Code::SEMA_ATTRIBUTE_IMPLICT_SET,
							this->set_location.value(),
							std::format("Attribute #{} was implicitly set", this->name),
							Diagnostic::Info(
								"Implicitly set here:", Diagnostic::Location::get(location, this->sema.source)
							)
						);
						return;
					}else{
						this->sema.emit_error(
							Diagnostic::Code::SEMA_ATTRIBUTE_ALREADY_SET,
							this->set_location.value(),
							std::format("Attribute #{} was implicitly set", this->name),
							Diagnostic::Info(
								"Implicitly set here:", Diagnostic::Location::get(location, this->sema.source)
							)
						);
						return;
					}

				}

				evo::debugAssert(
					this->implicitly_set_location.has_value() == false,
					"Attribute #{} already implicitly set. Should this be handled? Design changed?",
					this->name
				);

				this->is_set_true = cond;
				this->implicitly_set_location = location;
			}
	
		private:
			SemanticAnalyzer& sema;
			std::string_view name;

			bool is_set_true = false;
			std::optional<Token::ID> set_location{};
			std::optional<Token::ID> implicitly_set_location{};
	};


}