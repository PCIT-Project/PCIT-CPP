# PCIT-CPP

Panther Compiler Infrastructure and Toolchain, and the home of the Panther programming language. PCIT relies heavily on the [LLVM-Project](https://github.com/llvm/llvm-project), but that is not guaranteed forever. It is written entirely in C++, but a self-hosted version (written entirely in Panther) is planned.

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

## Updates:
List of changes for each version can be found [here](CHANGELOG.md).