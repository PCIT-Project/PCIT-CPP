////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once

#include <Evo.h>

#include "../source/source_data.h"
#include "../TypeManager.h"
#include "./sema_stmt.h"

namespace pcit::panther::sema{

	
	class ScopeLevel{
		public:
			struct ID : public core::UniqueID<uint32_t, struct ID> {
				using core::UniqueID<uint32_t, ID>::UniqueID;
			};

			struct ModuleInfo{
				SourceID sourceID;
				Token::ID tokenID;
			};

			struct IDNotReady{
				Token::ID location;
			};

			using IdentID = evo::Variant<
				IDNotReady,
				evo::SmallVector<sema::FuncID>,
				sema::TemplatedFuncID,
				sema::VarID,
				sema::ParamID,
				sema::ReturnParamID,
				ModuleInfo,
				BaseType::Alias::ID,
				BaseType::Typedef::ID
			>;

		public:
			ScopeLevel(sema::StmtBlock* stmt_block = nullptr) : _stmt_block(stmt_block) {}
			~ScopeLevel() = default;

			EVO_NODISCARD auto hasStmtBlock() const -> bool;
			EVO_NODISCARD auto stmtBlock() const -> const sema::StmtBlock&;
			EVO_NODISCARD auto stmtBlock()       ->       sema::StmtBlock&;

			auto addSubScope() -> void;
			auto setSubScopeTerminated() -> void;
			auto setTerminated() -> void;
			EVO_NODISCARD auto isTerminated() const -> bool;
			EVO_NODISCARD auto isNotTerminated() const -> bool;

			// returns if created new ident (false if already existed)
			auto addIdent(std::string_view ident, Token::ID location) -> bool;

			auto setFunc(std::string_view ident, sema::FuncID id) -> void;
			auto setTemplatedFunc(std::string_view ident, sema::TemplatedFuncID id) -> void;
			auto setVar(std::string_view ident, sema::VarID id) -> void;
			auto setParam(std::string_view ident, sema::ParamID id) -> void;
			auto setReturnParam(std::string_view ident, sema::ReturnParamID id) -> void;
			auto setModule(std::string_view ident, SourceID id, Token::ID location) -> void;
			auto setAlias(std::string_view ident, BaseType::Alias::ID id) -> void;
			auto setTypedef(std::string_view ident, BaseType::Typedef::ID id) -> void;

			EVO_NODISCARD auto lookupIdent(std::string_view ident) const -> const IdentID*;

		private:
			EVO_NODISCARD auto lookup_ident_without_locking(std::string_view ident) const -> const IdentID*;
	
		private:
			std::unordered_map<std::string_view, IdentID> ids{};
			mutable core::SpinLock ids_lock{};

			sema::StmtBlock* _stmt_block;
			unsigned num_sub_scopes_not_terminated = 0;
			bool has_sub_scopes = false;
			mutable core::SpinLock sub_scopes_and_stmt_block_lock{};
	};


}
