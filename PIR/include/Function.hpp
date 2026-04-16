////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.hpp>

#include <PCIT_core.hpp>

#include "./forward_decl_ids.hpp"
#include "./Type.hpp"
#include "./BasicBlock.hpp"
#include "./Expr.hpp"
#include "./meta.hpp"


namespace pcit::pir{

	class Parameter{
		private:
			struct AttributeUnsigned{};
			struct AttributeSigned{};
			struct AttributePtrNoAlias{};
			struct AttributePtrNonNull{};
			struct AttributePtrDereferencable{ uint64_t size; };
			struct AttributePtrReadOnly{};
			struct AttributePtrWritable{};
			struct AttributePtrRVO{ Type type; }; // only 1 may be used

		public:
			struct Attribute : public evo::Variant<
				AttributePtrNoAlias,
				AttributePtrNonNull,
				AttributePtrDereferencable,
				AttributePtrReadOnly,
				AttributePtrWritable,
				AttributePtrRVO
			>{
				using PtrNoAlias        = AttributePtrNoAlias;
				using PtrNonNull        = AttributePtrNonNull;
				using PtrDereferencable = AttributePtrDereferencable;
				using PtrReadOnly       = AttributePtrReadOnly;
				using PtrWritable       = AttributePtrWritable;
				using PtrRVO            = AttributePtrRVO;
			};

		public:
			Parameter(std::string&& _name, Type _type) : name(std::move(_name)), type(_type), attributes() {}
			Parameter(std::string&& _name, Type _type, evo::SmallVector<Attribute>&& _attributes)
				: name(std::move(_name)), type(_type), attributes(std::move(_attributes)) {}
			~Parameter() = default;

			[[nodiscard]] auto getName() const -> std::string_view { return this->name; }
			[[nodiscard]] auto getType() const -> const Type& { return this->type; }


			// Since `.attributes` is a public member you could add attributes directly,
			// 		but using this function has the added benefit of debug checking
			auto addAttribute(Attribute&& attribute) -> void {
				#if defined(PCIT_CONFIG_DEBUG)
					// TODO(FUTURE): Checking that checks previously added attributes
					// 		(no duplicates, no conflicting, etc)
					attribute.visit([&](const auto& attr) -> void {
						using AttrType = std::decay_t<decltype(attr)>;

						evo::debugAssert(
							this->type.kind() == Type::Kind::PTR,
							"This attribute can only be added to a parameter with a pointer type"
						);
					});
				#endif

				this->attributes.emplace_back(std::move(attribute));
			}

		public:
			evo::SmallVector<Attribute> attributes;

		private:
			std::string name;
			Type type;
	};



	// Create through Module
	struct ExternalFunction{
		std::string name;
		evo::SmallVector<Parameter> parameters;
		CallingConvention callingConvention;
		Linkage linkage;
		Type returnType;
		bool isNoReturn;
		std::optional<meta::Function::ID> metaID;

		// for lookup in Module
		using ID = ExternalFunctionID;
	};




	// Create through Module
	class Function{
		public:
			// for lookup in Module
			using ID = FunctionID;

		public:
			// Don't call this directly, go through Module
			Function(Module& module, const ExternalFunction& declaration)
				: parent_module(module), func_decl(declaration) {}

			Function(Module& module, ExternalFunction&& declaration)
				: parent_module(module), func_decl(std::move(declaration)) {}

			~Function() = default;


			[[nodiscard]] auto getName() const -> std::string_view { return this->func_decl.name; }
			[[nodiscard]] auto getParameters() const -> evo::ArrayProxy<Parameter> {
				return this->func_decl.parameters;
			}
			[[nodiscard]] auto getCallingConvention() const -> const CallingConvention& {
				return this->func_decl.callingConvention;
			}
			[[nodiscard]] auto getLinkage() const -> const Linkage& { return this->func_decl.linkage; }
			[[nodiscard]] auto getReturnType() const -> const Type& { return this->func_decl.returnType; }

			[[nodiscard]] auto getIsNoReturn() const -> bool { return this->func_decl.isNoReturn; }

			[[nodiscard]] auto getMetaID() const -> std::optional<meta::Function::ID> { return this->func_decl.metaID; }

			[[nodiscard]] auto getParentModule() const -> const Module& { return this->parent_module; }
			[[nodiscard]] auto getParentModule()       ->       Module& { return this->parent_module; }




			//////////////////////////////////////////////////////////////////////
			// iterators

			using Iter                 = evo::SmallVector<BasicBlock::ID>::iterator;
			using ConstIter            = evo::SmallVector<BasicBlock::ID>::const_iterator;
			using ReverseIter          = evo::SmallVector<BasicBlock::ID>::reverse_iterator;
			using ConstReverseIterator = evo::SmallVector<BasicBlock::ID>::const_reverse_iterator;

			[[nodiscard]] auto begin()        -> Iter      { return this->basic_blocks.begin();  };
			[[nodiscard]] auto begin()  const -> ConstIter { return this->basic_blocks.begin();  };
			[[nodiscard]] auto cbegin() const -> ConstIter { return this->basic_blocks.cbegin(); };

			[[nodiscard]] auto end()        -> Iter      { return this->basic_blocks.end();  };
			[[nodiscard]] auto end()  const -> ConstIter { return this->basic_blocks.end();  };
			[[nodiscard]] auto cend() const -> ConstIter { return this->basic_blocks.cend(); };


			[[nodiscard]] auto rbegin()        -> ReverseIter          { return this->basic_blocks.rbegin();  };
			[[nodiscard]] auto rbegin()  const -> ConstReverseIterator { return this->basic_blocks.rbegin();  };
			[[nodiscard]] auto crbegin() const -> ConstReverseIterator { return this->basic_blocks.crbegin(); };

			[[nodiscard]] auto rend()        -> ReverseIter          { return this->basic_blocks.rend();  };
			[[nodiscard]] auto rend()  const -> ConstReverseIterator { return this->basic_blocks.rend();  };
			[[nodiscard]] auto crend() const -> ConstReverseIterator { return this->basic_blocks.crend(); };


			using AllocasRange = evo::IterRange<core::StepAlloc<Alloca, uint32_t>::ConstIter>;
			[[nodiscard]] auto getAllocasRange() const -> AllocasRange {
				return AllocasRange(this->allocas.begin(), this->allocas.end());
			}


			// Not intended to be called directly, better to be called by Module::deleteBodyOfFunction
			// 	as it will delete all exprs and all basic blocks
			auto clear() -> void {
				this->basic_blocks.clear();
				this->allocas.clear();
			}


		private:
			auto append_basic_block(BasicBlock::ID id) -> void;
			auto insert_basic_block_before(BasicBlock::ID id, BasicBlock::ID before) -> void;
			auto insert_basic_block_before(BasicBlock::ID id, const BasicBlock& before) -> void;
			auto insert_basic_block_after(BasicBlock::ID id, BasicBlock::ID after) -> void;
			auto insert_basic_block_after(BasicBlock::ID id, const BasicBlock& after) -> void;

			[[nodiscard]] auto basic_block_is_already_in(BasicBlock::ID id) const -> bool;


			[[nodiscard]] auto check_func_call_args(Function::ID id, evo::ArrayProxy<Expr> args) const -> bool;
			[[nodiscard]] auto check_func_call_args(ExternalFunction::ID id, evo::ArrayProxy<Expr> args) const -> bool;
			[[nodiscard]] auto check_func_call_args(Type func_type, evo::ArrayProxy<Expr> args) const -> bool;
			[[nodiscard]] auto check_func_call_args(evo::ArrayProxy<Type> param_types, evo::ArrayProxy<Expr> args) const
				-> bool;

	
		private:
			ExternalFunction func_decl;
			Module& parent_module;

			evo::SmallVector<BasicBlock::ID> basic_blocks{};
			core::StepAlloc<Alloca, uint32_t> allocas{};

			friend class ReaderAgent;
			friend class Agent;
	};


}


