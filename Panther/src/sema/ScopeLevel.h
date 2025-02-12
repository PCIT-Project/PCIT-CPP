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
#include "../../include/sema/Stmt.h"

namespace pcit::panther::sema{

	
	class ScopeLevel{
		public:
			struct ID : public core::UniqueID<uint32_t, struct ID> {
				using core::UniqueID<uint32_t, ID>::UniqueID;
			};

			struct ModuleInfo{
				SourceID sourceID;
				Token::ID tokenID;
				bool isPub;
			};


			using FuncOverloadList = evo::SmallVector<evo::Variant<sema::FuncID, sema::TemplatedFuncID>>;

			using IdentID = evo::Variant<
				FuncOverloadList,
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

			// returns false if is redef (functions don't take into account redef of equivalent overloads)
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::FuncID id) -> bool;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::TemplatedFuncID id) -> bool;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::VarID id) -> bool;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::ParamID id) -> bool;
			EVO_NODISCARD auto addIdent(std::string_view ident, sema::ReturnParamID id) -> bool;
			EVO_NODISCARD auto addIdent(std::string_view ident, SourceID id, Token::ID location, bool is_pub) -> bool;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::Alias::ID id) -> bool;
			EVO_NODISCARD auto addIdent(std::string_view ident, BaseType::Typedef::ID id) -> bool;

			EVO_NODISCARD auto lookupIdent(std::string_view ident) const -> const IdentID*;
	
		private:
			std::unordered_map<std::string_view, IdentID> ids{};
			mutable core::SpinLock ids_lock{};

			sema::StmtBlock* _stmt_block;
			unsigned num_sub_scopes_not_terminated = 0;
			bool has_sub_scopes = false;
			mutable core::SpinLock sub_scopes_and_stmt_block_lock{};
	};


}
