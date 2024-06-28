# PCIT-CPP

Panther Compiler Infrastructure and Toolchain, and the home of the Panther programming language. PCIT relies on the [LLVM-Project](https://github.com/llvm/llvm-project), but that is not guaranteed forever. It is written entirely in C++, but a self-hosted version (written entirely in Panther) is planned.

> [!NOTE]
> This is in the very early stages and as such is not usable yet.


## Tools
All tools can be used as stand-alone programs or as libraries.

### Panther / pthr
Statically typed, compiled, high-performance, general-purpose programming language.

### IR
[Unnamed for now]. Compiler IR / optimizing back-end.

### Linker
[Unnamed for now]. Linker that aims to make use between platforms as seamless as possible.

### Other compilers
Will also be able to compile C and hopefully C++ code. The main intent is for this to allow seamless interoperability with Panther, but these frontends will be able to be used stand-alone.


## Downloading:
`git clone https://github.com/PCIT-Project/PCIT-CPP.git --recursive`


## Versioning:
The versioning scheme is as follows: 
`[major].[release].[minor].[patch]`
A "release" is the number of the release within the major version. This is zero-indexed if major is >= 1, otherwise it is one-indexed. Any versions within releases is not expected to necessarily be bug-free and stable.

### Expected Timeline:
- 0.1.0.0: (the first release) 
	- `Panther`/`pthr` working enough to give to testers. Some major features will be missing. Some documentation will exist
	- Bare-bones version of the Linker library will also be working
- 0.2.0.0:
	- Bare-bones version of the IR library
	- Some missing features of Panther added
- *in-between*:
	- TBD
- 1.0.0.0:
	- The project is "released" and is production ready


## Updates:
List of changes for each version can be found [here](CHANGELOG.md).