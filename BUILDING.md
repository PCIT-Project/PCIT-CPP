# Building

These are the building instructions for the PCIT project. This has only been tested on Windows with Visual Studio 2022.

## Requirements:
- [Premake5](https://premake.github.io/)
- cmake
- A version of MSVC, GCC, or Clang that is C++20 compliant
- Either Visual Studio or make


## Downloading PCIT
`git clone https://github.com/PCIT-Project/PCIT-CPP.git --recursive`


## Building the LLVM Project
You can do this anywhere on your computer.

```
mkdir LLVM
cd LLVM
mkdir build
git clone https://github.com/llvm/llvm-project.git --depth=1
```


### Windows:
In this directory create a file called `setup_llvm.bat`, and fill it with this:

```bat
rem This file is licensed under the public domain.
rem Thanks to https://github.com/ziglang/zig-bootstrap/blob/master/build.bat

cd ./build

cmake "../llvm-project/llvm" ^
  -G "Visual Studio 17 2022" ^
  -DCMAKE_INSTALL_PREFIX="../llvm-project/llvm/build-output" ^
  -DCMAKE_PREFIX_PATH="../llvm-project/llvm/build-output" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DLLVM_ENABLE_PROJECTS="lld;clang" ^
  -DLLVM_ENABLE_LIBXML2=OFF ^
  -DLLVM_ENABLE_ZSTD=OFF ^
  -DLLVM_INCLUDE_UTILS=OFF ^
  -DLLVM_INCLUDE_TESTS=OFF ^
  -DLLVM_INCLUDE_EXAMPLES=OFF ^
  -DLLVM_INCLUDE_BENCHMARKS=OFF ^
  -DLLVM_INCLUDE_DOCS=OFF ^
  -DLLVM_ENABLE_BINDINGS=OFF ^
  -DLLVM_ENABLE_OCAMLDOC=OFF ^
  -DLLVM_ENABLE_Z3_SOLVER=OFF ^
  -DLLVM_TOOL_LLVM_LTO2_BUILD=OFF ^
  -DLLVM_TOOL_LLVM_LTO_BUILD=OFF ^
  -DLLVM_TOOL_LTO_BUILD=OFF ^
  -DLLVM_TOOL_REMARKS_SHLIB_BUILD=OFF ^
  -DLLVM_BUILD_TOOLS=OFF ^
  -DCLANG_BUILD_TOOLS=OFF ^
  -DCLANG_INCLUDE_DOCS=OFF ^
  -DLLVM_INCLUDE_DOCS=OFF ^
  -DCLANG_TOOL_CLANG_IMPORT_TEST_BUILD=OFF ^
  -DCLANG_TOOL_CLANG_LINKER_WRAPPER_BUILD=OFF ^
  -DCLANG_TOOL_C_INDEX_TEST_BUILD=OFF ^
  -DCLANG_TOOL_ARCMT_TEST_BUILD=OFF ^
  -DCLANG_TOOL_C_ARCMT_TEST_BUILD=OFF ^
  -DCLANG_TOOL_LIBCLANG_BUILD=OFF ^
  -DLLVM_USE_CRT_RELEASE=MT ^
  -DLLVM_BUILD_LLVM_C_DYLIB=NO

if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
cmake --build . %JOBS_ARG% --target install
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
```

Run `setup_llvm.bat`. This may take awhile (took my PC 2+ hours).


### Other Platforms:

> [!NOTE]
> This is untested

In this directory create a file called `setup_llvm`, and fill it with the following:

```sh
#!/bin/sh
# This file is licensed under the public domain.
# thanks to: https://github.com/ziglang/zig-bootstrap/blob/master/build

cd "./build"

cmake "../llvm-project/llvm" \
  -DCMAKE_INSTALL_PREFIX="../llvm-project/llvm/build-output" \
  -DCMAKE_PREFIX_PATH="../llvm-project/llvm/build-output" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS="lld;clang" \
  -DLLVM_ENABLE_LIBXML2=OFF \
  -DLLVM_ENABLE_ZSTD=OFF \
  -DLLVM_INCLUDE_UTILS=OFF \
  -DLLVM_INCLUDE_TESTS=OFF \
  -DLLVM_INCLUDE_EXAMPLES=OFF \
  -DLLVM_INCLUDE_BENCHMARKS=OFF \
  -DLLVM_INCLUDE_DOCS=OFF \
  -DLLVM_ENABLE_BINDINGS=OFF \
  -DLLVM_ENABLE_OCAMLDOC=OFF \
  -DLLVM_ENABLE_Z3_SOLVER=OFF \
  -DLLVM_TOOL_LLVM_LTO2_BUILD=OFF \
  -DLLVM_TOOL_LLVM_LTO_BUILD=OFF \
  -DLLVM_TOOL_LTO_BUILD=OFF \
  -DLLVM_TOOL_REMARKS_SHLIB_BUILD=OFF \
  -DCLANG_BUILD_TOOLS=OFF \
  -DCLANG_INCLUDE_DOCS=OFF \
  -DCLANG_TOOL_CLANG_IMPORT_TEST_BUILD=OFF \
  -DCLANG_TOOL_CLANG_LINKER_WRAPPER_BUILD=OFF \
  -DCLANG_TOOL_C_INDEX_TEST_BUILD=OFF \
  -DCLANG_TOOL_ARCMT_TEST_BUILD=OFF \
  -DCLANG_TOOL_C_ARCMT_TEST_BUILD=OFF \
  -DCLANG_TOOL_LIBCLANG_BUILD=OFF

cmake --build . --target install
```

Run `setup_llvm`. This may take awhile.


## Adding LLVM to the PCIT Project:
Once you've successfully built the LLVM Project, you will need to go into `./llvm-project/llvm/build-output`. Copy `include` and `lib` into the directory `PCIT-CPP/libs/LLVM` (the libs directory already exists, but you need to create the LLVM directory inside).


## Building
### Visual Studio
Run `premake5 vs2022`. Then, open up the generated solution in Visual Studio, set the build configuration to ReleaseDist, and compile.

### Clang
> [!NOTE]
> This is untested

```
premake5 gmake2 cc=clang
make configuration=releasedist_linux
```

### GCC
> [!NOTE]
> This is untested

```
premake5 gmake2 cc=gcc
make configuration=releasedist_linux
```

## Generated output
The generated output is in `./build/[Windows|Linux]/ReleaseDist/bin`