////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////


#pragma once


#include <Evo.h>

#include <PCIT_core.h>

#include "./forward_decl_ids.h"
#include "./Type.h"
#include "./BasicBlock.h"
#include "./Expr.h"

namespace pcit::pir{

	class Parameter{
		private:
			struct AttributeUnsigned{};
			struct AttributeSigned{};
			struct AttributePtrNoAlias{};
			struct AttributePtrNonNull{};
			struct AttributePtrDereferencable{ uint64_t size; };
			struct AttributePtrReadOnly{};
			struct AttributePtrWriteOnly{};
			struct AttributePtrWritable{};
			struct AttributePtrRVO{ Type type; }; // only 1 may be used

		public:
			struct Attribute : public evo::Variant<
				AttributeUnsigned,
				AttributeSigned,
				AttributePtrNoAlias,
				AttributePtrNonNull,
				AttributePtrDereferencable,
				AttributePtrReadOnly,
				AttributePtrWriteOnly,
				AttributePtrWritable,
				AttributePtrRVO
			>{
				using Unsigned          = AttributeUnsigned;
				using Signed            = AttributeSigned;
				using PtrNoAlias        = AttributePtrNoAlias;
				using PtrNonNull        = AttributePtrNonNull;
				using PtrDereferencable = AttributePtrDereferencable;
				using PtrReadOnly       = AttributePtrReadOnly;
				using PtrWriteOnly      = AttributePtrWriteOnly;
				using PtrWritable       = AttributePtrWritable;
				using PtrRVO            = AttributePtrRVO;
			};

		public:
			Parameter(std::string&& _name, Type _type) : name(std::move(_name)), type(_type), attributes() {}
			Parameter(std::string&& _name, Type _type, evo::SmallVector<Attribute>&& _attributes)
				: name(std::move(_name)), type(_type), attributes(std::move(_attributes)) {}
			~Parameter() = default;

			EVO_NODISCARD auto getName() const -> std::string_view { return this->name; }
			EVO_NODISCARD auto getType() const -> const Type& { return this->type; }


			// Since `.attributes` is a public member you could add attributes directly,
			// 		but using this function has the added benefit of debug checking
			auto addAttribute(Attribute&& attribute) -> void {
				#if defined(PCIT_CONFIG_DEBUG)
					// TODO(FUTURE): Checking that checks previously added attributes
					// 		(no duplicates, no conflicting, etc)
					attribute.visit([&](const auto& attr) -> void {
						using AttrType = std::decay_t<decltype(attr)>;

						if constexpr(
							std::is_same<AttrType, Attribute::Unsigned>() || std::is_same<AttrType, Attribute::Signed>()
						){
							evo::debugAssert(
								this->type.kind() == Type::Kind::INTEGER,
								"This attribute can only be added to a parameter with an integer type"
							);
						
						}else{
							evo::debugAssert(
								this->type.kind() == Type::Kind::PTR,
								"This attribute can only be added to a parameter with a pointer type"
							);
						}
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


			EVO_NODISCARD auto getName() const -> const std::string& { return this->func_decl.name; }
			EVO_NODISCARD auto getParameters() const -> evo::ArrayProxy<Parameter> {
				return this->func_decl.parameters;
			}
			EVO_NODISCARD auto getCallingConvention() const -> const CallingConvention& {
				return this->func_decl.callingConvention;
			}
			EVO_NODISCARD auto getLinkage() const -> const Linkage& { return this->func_decl.linkage; }
			EVO_NODISCARD auto getReturnType() const -> const Type& { return this->func_decl.returnType; }

			EVO_NODISCARD auto getIsNoReturn() const -> bool { return this->func_decl.isNoReturn; }



			EVO_NODISCARD auto getParentModule() const -> const Module& { return this->parent_module; }
			EVO_NODISCARD auto getParentModule()       ->       Module& { return this->parent_module; }




			//////////////////////////////////////////////////////////////////////
			// iterators

			using Iter                 = evo::SmallVector<BasicBlock::ID>::iterator;
			using ConstIter            = evo::SmallVector<BasicBlock::ID>::const_iterator;
			using ReverseIter          = evo::SmallVector<BasicBlock::ID>::reverse_iterator;
			using ConstReverseIterator = evo::SmallVector<BasicBlock::ID>::const_reverse_iterator;

			EVO_NODISCARD auto begin()        -> Iter      { return this->basic_blocks.begin();  };
			EVO_NODISCARD auto begin()  const -> ConstIter { return this->basic_blocks.begin();  };
			EVO_NODISCARD auto cbegin() const -> ConstIter { return this->basic_blocks.cbegin(); };

			EVO_NODISCARD auto end()        -> Iter      { return this->basic_blocks.end();  };
			EVO_NODISCARD auto end()  const -> ConstIter { return this->basic_blocks.end();  };
			EVO_NODISCARD auto cend() const -> ConstIter { return this->basic_blocks.cend(); };


			EVO_NODISCARD auto rbegin()        -> ReverseIter          { return this->basic_blocks.rbegin();  };
			EVO_NODISCARD auto rbegin()  const -> ConstReverseIterator { return this->basic_blocks.rbegin();  };
			EVO_NODISCARD auto crbegin() const -> ConstReverseIterator { return this->basic_blocks.crbegin(); };

			EVO_NODISCARD auto rend()        -> ReverseIter          { return this->basic_blocks.rend();  };
			EVO_NODISCARD auto rend()  const -> ConstReverseIterator { return this->basic_blocks.rend();  };
			EVO_NODISCARD auto crend() const -> ConstReverseIterator { return this->basic_blocks.crend(); };


			using AllocasRange = evo::IterRange<core::StepAlloc<Alloca, uint32_t>::ConstIter>;
			EVO_NODISCARD auto getAllocasRange() const -> AllocasRange {
				return AllocasRange(this->allocas.begin(), this->allocas.end());
			}



		private:
			auto append_basic_block(BasicBlock::ID id) -> void;
			auto insert_basic_block_before(BasicBlock::ID id, BasicBlock::ID before) -> void;
			auto insert_basic_block_before(BasicBlock::ID id, const BasicBlock& before) -> void;
			auto insert_basic_block_after(BasicBlock::ID id, BasicBlock::ID after) -> void;
			auto insert_basic_block_after(BasicBlock::ID id, const BasicBlock& after) -> void;

			EVO_NODISCARD auto basic_block_is_already_in(BasicBlock::ID id) const -> bool;


			EVO_NODISCARD auto check_func_call_args(Function::ID id, evo::ArrayProxy<Expr> args) const -> bool;
			EVO_NODISCARD auto check_func_call_args(ExternalFunction::ID id, evo::ArrayProxy<Expr> args) const -> bool;
			EVO_NODISCARD auto check_func_call_args(Type func_type, evo::ArrayProxy<Expr> args) const -> bool;
			EVO_NODISCARD auto check_func_call_args(evo::ArrayProxy<Type> param_types, evo::ArrayProxy<Expr> args) const
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


