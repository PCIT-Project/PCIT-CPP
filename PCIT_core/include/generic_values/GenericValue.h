////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
// Part of PCIT-CPP, under the Apache License v2.0 with LLVM and PCIT exceptions. //
// You may not use this file except in compliance with the License.               //
// See `https://github.com/PCIT-Project/PCIT-CPP/blob/main/LICENSE`for info.      //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <Evo.h>


#include "./GenericInt.h"
#include "./GenericFloat.h"


namespace pcit::core{

	//////////////////////////////////////////////////////////////////////
	// 
	// GenericValue
	// 	 There is purposely no way to check which value type is held as if GenericValue is being used you should know
	// 	 which type there is held
	// 
	//////////////////////////////////////////////////////////////////////

	struct GenericValue{
		explicit GenericValue() : data{.small = 0}, num_bytes(0) {}

		explicit GenericValue(bool val) : data{.small = uint64_t(val)}, num_bytes(1) {}
		explicit GenericValue(char val) : data{.small = uint64_t(val)}, num_bytes(1) {}

		explicit GenericValue(std::string_view val) : data{.small = 0}, num_bytes(val.size()) {
			if(val.size() <= 8){
				std::memcpy(&this->data.small, val.data(), this->num_bytes);

			}else{
				const size_t alloc_size = ceil_to_multiple(this->num_bytes, 8);

				this->data.buffer = (uint64_t*)std::malloc(alloc_size);
				std::memcpy(this->data.buffer, val.data(), this->num_bytes);
			}
		}


		explicit GenericValue(const core::GenericInt& val) : num_bytes(ceil_to_multiple(val.getBitWidth(), 64) / 8) {
			if(this->is_small()){
				this->data.small = static_cast<uint64_t>(val);

			}else{
				this->data.buffer = (uint64_t*)std::malloc(this->num_bytes * 8);
				std::memcpy(this->data.buffer, val.data(), this->num_bytes);
			}
		}

		explicit GenericValue(const core::GenericFloat& val) {
			std::construct_at(this, val.bitCastToGenericInt());
		}


		EVO_NODISCARD static auto createPtr(auto* ptr) -> GenericValue {
			return GenericValue(GenericInt::create<size_t>(reinterpret_cast<size_t>(ptr)));
		}

		EVO_NODISCARD static auto createPtr(std::nullptr_t) -> GenericValue {
			return GenericValue(GenericInt::create<size_t>(0));
		}



		EVO_NODISCARD static auto createUninit(size_t num_bytes) -> GenericValue {
			return GenericValue(num_bytes);
		}

		EVO_NODISCARD static auto fromData(evo::ArrayProxy<std::byte> data) -> GenericValue {
			return GenericValue(std::string_view((const char*)data.data(), data.size()));
		}


		~GenericValue(){
			if(this->num_bytes > 8){
				std::free(this->data.buffer);
			}
		}


		GenericValue(const GenericValue& rhs) : num_bytes(rhs.num_bytes) {
			if(rhs.is_small()){
				this->data.small = rhs.data.small;
			}else{
				this->data.buffer = (uint64_t*)std::malloc(this->num_bytes * 8);
				std::memcpy(this->data.buffer, rhs.data.buffer, this->num_bytes);
			}
		}
		auto operator=(const GenericValue& rhs) -> GenericValue& {
			std::construct_at(this, rhs);
			return *this;
		}


		GenericValue(GenericValue&& rhs) : num_bytes(std::exchange(rhs.num_bytes, 0)), data(rhs.data) {}
		auto operator=(GenericValue&& rhs) -> GenericValue& {
			std::construct_at(this, std::move(rhs));
			return *this;
		}




		EVO_NODISCARD auto getBool() const -> bool {
			evo::debugAssert(this->num_bytes == 1, "Not a bool");
			return bool(this->data.small & 1);
		}

		EVO_NODISCARD auto getChar() const -> char {
			evo::debugAssert(this->num_bytes == 1, "Not a char");
			return char(this->data.small);
		}

		EVO_NODISCARD auto getStr() const -> std::string_view {
			const evo::ArrayProxy<std::byte> data_range = this->dataRange();
			return std::string_view((const char*)data_range.data(), data_range.size());
		}


		EVO_NODISCARD auto getInt(unsigned bit_width) const -> core::GenericInt {
			#if defined(PCIT_CONFIG_DEBUG)
				     if(bit_width <= 8){ evo::debugAssert(this->num_bytes >= 1, "Invalid bit width for what is held"); }
				else if(bit_width <= 16){ evo::debugAssert(this->num_bytes >= 2, "Invalid bit width for what is held");}
				else if(bit_width <= 32){ evo::debugAssert(this->num_bytes >= 4, "Invalid bit width for what is held");}
				else{
					evo::debugAssert(
						ceil_to_multiple(bit_width, 64) / 8 >= this->num_bytes, "Invalid bit width for what is held"
					);
				}
			#endif

			if(this->is_small()){
				return core::GenericInt(bit_width, this->data.small);
			}else{
				return core::GenericInt(bit_width, evo::ArrayProxy<uint64_t>(this->data.buffer, this->num_bytes * 8));
			}
		}


		EVO_NODISCARD auto getF16() const -> core::GenericFloat {
			return core::GenericFloat::createF16(this->getInt(16));
		}

		EVO_NODISCARD auto getBF16() const -> core::GenericFloat {
			return core::GenericFloat::createBF16(this->getInt(16));
		}

		EVO_NODISCARD auto getF32() const -> core::GenericFloat {
			return core::GenericFloat::createF32(this->getInt(32));
		}

		EVO_NODISCARD auto getF64() const -> core::GenericFloat {
			return core::GenericFloat::createF64(this->getInt(64));
		}

		EVO_NODISCARD auto getF80() const -> core::GenericFloat {
			return core::GenericFloat::createF80(this->getInt(80));
		}

		EVO_NODISCARD auto getF128() const -> core::GenericFloat {
			return core::GenericFloat::createF128(this->getInt(128));
		}


		template<class T>
		EVO_NODISCARD auto getPtr() const -> T {
			evo::debugAssert(std::is_pointer<T>(), "Not a pointer");
			evo::debugAssert(this->num_bytes == sizeof(T), "Not a pointer");
			return reinterpret_cast<T>(this->data.small);
		}



		EVO_NODISCARD auto dataRange() const -> evo::ArrayProxy<std::byte> {
			if(this->is_small()){
				return evo::ArrayProxy((std::byte*)&this->data.small, this->num_bytes);
			}else{
				return evo::ArrayProxy((std::byte*)this->data.buffer, this->num_bytes);
			}
		}


		EVO_NODISCARD auto writableDataRange() -> std::span<std::byte> {
			if(this->is_small()){
				return std::span((std::byte*)&this->data.small, this->num_bytes);
			}else{
				return std::span((std::byte*)this->data.buffer, this->num_bytes);
			}
		}



		EVO_NODISCARD auto hash() const -> size_t {
			if(this->is_small()){
				return std::hash<uint64_t>{}(this->data.small);

			}else{
				size_t hash_value = std::hash<size_t>{}(this->num_bytes);
				
				for(size_t i = 0; i < this->num_words(); i+=1){
					hash_value = evo::hashCombine(hash_value, std::hash<uint64_t>{}(this->data.buffer[i]));
				}

				return hash_value;
			}
		}


		EVO_NODISCARD auto operator==(const GenericValue& rhs) const -> bool {
			if(this->num_bytes != rhs.num_bytes){ return false; }

			if(this->is_small()){
				return this->data.small == rhs.data.small;
			}else{
				for(size_t i = 0; i < this->num_words(); i+=1){
					if(this->data.buffer[i] != rhs.data.buffer[i]){ return false; }
				}
				return true;
			}
		}


		private:
			EVO_NODISCARD static constexpr auto ceil_to_multiple(size_t num, size_t multiple) -> size_t {
				return (num + (multiple - 1)) & ~(multiple - 1);
			}

			EVO_NODISCARD auto num_words() const -> size_t {
				return ceil_to_multiple(this->num_bytes, sizeof(uint64_t)) / sizeof(uint64_t);
			}

			EVO_NODISCARD auto is_small() const -> bool {
				return this->num_bytes <= 8;
			}


			explicit GenericValue(size_t uninit_num_bytes) : num_bytes(uninit_num_bytes) {
				if(this->is_small() == false){
					this->data.buffer = (uint64_t*)std::malloc(this->num_bytes * 8);
				}
			}


		private:
			
			union Data{
				uint64_t small;
				uint64_t* buffer;
			} data;

			size_t num_bytes; // <= 8 if using small buffer
	};


}


namespace std{


	template<>
	struct hash<pcit::core::GenericValue>{
		auto operator()(const pcit::core::GenericValue& generic_value) const noexcept -> size_t {
			return generic_value.hash();
		};
	};

	
}
