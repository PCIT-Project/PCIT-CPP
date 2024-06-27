//////////////////////////////////////////////////////////////////////
//                                                                  //
// Part of the PCIT-CPP, under the Apache License v2.0              //
// You may not use this file except in compliance with the License. //
// See `http://www.apache.org/licenses/LICENSE-2.0` for info        //
//                                                                  //
//////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>
#include <PCIT_core.h>

#include "./SourceManager.h"
#include "./diagnostics.h"


namespace pcit::panther{


	class Context{
		public:
			using DiagnosticCallback = std::function<void(const Context&, const Diagnostic&)>;

		public:
			Context(DiagnosticCallback diagnostic_callback) noexcept : callback(diagnostic_callback) {};
			~Context() = default;


			auto emitDiagnostic(auto&&... args) noexcept -> void {
				auto diagnostic = Diagnostic(std::forward<decltype(args)>(args)...);
				this->emit_diagnostic_impl(diagnostic);
			};


			EVO_NODISCARD auto getSourceManager()       noexcept ->       SourceManager& { return this->src_manager; };
			EVO_NODISCARD auto getSourceManager() const noexcept -> const SourceManager& { return this->src_manager; };

		private:
			auto emit_diagnostic_impl(const Diagnostic& diagnostic) noexcept -> void;
	
		private:
			SourceManager src_manager;

			DiagnosticCallback callback;
			std::mutex callback_mutex;
	};


};