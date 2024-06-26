# Panthera-Project-CPP

Open-source compiler toolchain. At the start will rely heavily on the LLVM-project, but the plan is to be a completely separate toolchain. It is written entirely in C++, but a self-hosted version is planned.

> [!NOTE]
> This is in the very early stages and as such is not usable yet.

## Tools

All tools can be used as standalone programs or as libraries.

### Panther
Statically typed, compiled, high-performance, general-purpose programming language.

### Tiger
Low level compiler IR (intermediate representation) and optimizer.

### Lion
Linker.

### Other compilers
Will also be able to compile C and hopefully C++ code. The main intent is for this to allow seamless interoperability with Panther, but these frontends will be able to be used standalone.


## Downloading:
`git clone https://github.com/Panthera-Project/Panthera-Project.git --recursive`

## Updates:
List of changes for each version can be found [here](CHANGELOG.md).