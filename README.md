# PCIT-CPP

Panther Compiler Infrastructure and Toolchain, and the home of the Panther programming language. PCIT (pronounced "P-Kit") relies on the [LLVM-Project](https://github.com/llvm/llvm-project), but that is not guaranteed forever. It is written entirely in C++, but a self-hosted version (written entirely in Panther) is planned.

> [!NOTE]
> This is in the very early stages and as such is not usable yet. If you really want to try it now (without any documentation), you can modify `pthr/main.cpp`.

## Tools
Check out the [PCIT website](https://pcit-project.github.io) for more information about all of the tools in the PCIT-Project.
- [Panther](https://pcit-project.github.io/site/Panther.html): Statically typed, compiled, high-performance, general-purpose programming language
- PIR: Compiler IR / optimizing back-end
- PLNK: Linker that aims to make use between platforms (including cross-compilation) as seamless as possible.


## Building:
The build instructions can be found [here](BUILDING.md).


## Versioning:
The versioning scheme is as follows: `[major].[release].[minor].[patch]`. A "release" is the number of the release within the major version. This is zero-indexed if major is >= 1, otherwise it is one-indexed. Any versions within releases is not expected to necessarily be bug-free and stable.
For brevity, the minor and patch may be left off (making it just `[major].[release]`)


### Expected Timeline:
- 0.1: (the first release) 
	- `Panther`/`pthr` working enough to give to testers. Some major features will be missing. Some documentation will exist
	- Bare-bones version of the Linker library will also be working
	- Bare-bones version of PIR library
- 0.2:
	- Some missing features of Panther added
- *in-between*:
	- TBD
- 1.0:
	- The project is "released" and is production ready


## Updates:
List of changes for each version can be found [here](CHANGELOG.md). Note: very small changes may not be listed.