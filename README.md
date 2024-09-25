# PCIT-CPP

Panther Compiler Infrastructure and Toolchain, and the home of the Panther programming language. PCIT (pronounced "P-Kit") relies on the [LLVM-Project](https://github.com/llvm/llvm-project), but that is not guaranteed forever. It is written entirely in C++, but a self-hosted version (written entirely in Panther) is planned.

> [!NOTE]
> This is in the very early stages and as such is not usable yet. If you really want to try it now (without any documentation), you can modify `pthr/main.cpp`.


## Tools
All tools can be used as stand-alone programs or as libraries.

### Panther / pthr
Statically typed, compiled, high-performance, general-purpose programming language. A few goals of the language:
- Help good programmers write good, fast code
	- Zero-cost abstractions
	- Give the compiler knowledge of common patterns to allow it to help you write fast code, easier
	- Allow as much compile-time computation as possible
	- Powerful generics without needing to be an expert
- Enjoyable to use
	- Fast compile times
	- Build system for Panther *in* Panther
	- Nice / helpful error messages
- Seamless interop with C and C++ without compromising on language design

### IR
*[Unnamed for now]*. Compiler IR / optimizing back-end.

### Linker
*[Unnamed for now]*. Linker that aims to make use between platforms (including cross-compilation) as seamless as possible.

### Other compilers
Will also be able to compile C and hopefully C++ code. The main intent is for this to allow seamless interoperability with Panther, but these frontends will be able to be used stand-alone.

## Building:
The build instructions can be found [here](BUILDING.md).


## Versioning:
The versioning scheme is as follows: `[major].[release].[minor].[patch]`. A "release" is the number of the release within the major version. This is zero-indexed if major is >= 1, otherwise it is one-indexed. Any versions within releases is not expected to necessarily be bug-free and stable.
For brevity, the minor and patch may be left off (making it just `[major].[release]`)


### Expected Timeline:
- 0.1: (the first release) 
	- `Panther`/`pthr` working enough to give to testers. Some major features will be missing. Some documentation will exist
	- Bare-bones version of the Linker library will also be working
- 0.2:
	- Bare-bones version of the IR library
	- Some missing features of Panther added
- *in-between*:
	- TBD
- 1.0:
	- The project is "released" and is production ready


## Updates:
List of changes for each version can be found [here](CHANGELOG.md). Note: very small changes may not be listed.


## Panther Syntax
Here's a quick taste of the syntax of the Panther programming language. All of the following currently compiles (as of `v0.0.39.0`). If you want a peek at all currently supported features, maybe look at [the change log](CHANGELOG.md). Please keep in mind that any and all syntax may change in the future.
```Panther
// importing a file
def some_file = @import("directory/file.pthr");

// function declaration (parameter `num` is implicity `read`)
func set_num = (num: UI8, num_to_change: UI8 mut) -> Void {
	num_to_change = copy num;
}

// templated function
func just_return_num = <{T: Type}> (num: T) -> T {
	func sub_func = (sub_num: T) -> T {
		return (move sub_num);
	}

	return sub_func(num);
}

// entry function (notice the name doesn't matter, but it has the attribute `#entry`)
func asdf = () #entry -> UI8 {
	def COMPILE_TIME_VALUE: Bool = true;
	when(COMPILE_TIME_VALUE){ // compile time conditional (doesn't enter a new scope)
		var foo = just_return_num<{UI8}>(some_file.get_UI8_12()); // variable declaration with type inference
	}else{
		var foo: UI8 = 12; // variable declaration with explicit type
	}

	var bar: UI8 = uninit;
	set_num(foo, bar);

	return (move bar);
}
```