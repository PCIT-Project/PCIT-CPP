# Change Log

<!---------------------------------->
## v0.0.74.0

### Panther
- Added operators:
	- `&`
	- `|`
	- `^`
	- `<<`
	- `<<|`
	- `>>`
- Added support of lowering of:
	- `@and`
	- `@or`
	- `@xor`
	- `@shl`
	- `@shlSat`
	- `@shr`
- Fixed `@shlSat` having unused boolean template parameter
- Fixed fatal error when using multiple different checked math operations

### PIR
- Added bitwise instructions:
	- `@and`
	- `@or`
	- `@xor`
	- `@shl`
	- `@sshlsat`, `@ushlsat`
	- `@sshr`, `@ushr`


<!---------------------------------->
## v0.0.73.0

### Panther
- Added support of lowering of:
	- `@eq`, `@neq`
	- `@lt`, `@lte`
	- `@gt`, `@gte`

### PIR
- Added comparison instructions:
	- `@ieq`, `@feq`
	- `@ineq`, `@fneq`
	- `@slt`, `@ult`, `@flt`
	- `@slte`, `@ulte`, `@flte`
	- `@sgt`, `@ugt`, `@fgt`
	- `@sgte`, `@ugte`, `@fgte`


<!---------------------------------->
## v0.0.72.0

### Panther
- Added support for lowering of:
	- `@addSat`
	- `@sub`, `@subWrap`, `@subSat`, `@fsub`
	- `@mul`, `@mulWrap`, `@mulSat`, `@fmul`
	- `@div`, `@fdiv`, `@rem`
- Fixed fatal error when compiling an if statement that requires branches in the block

### pthr
- Added the `PrintAssembly` build target
- Added the `Assembly` build target

### PIR
- Added arithmetic instructions:
	- `@saddSat`, `@uaddSat`
	- `@sub`, `@ssubWrap`, `@usubWrap`, `@ssubSat`, `@usubSat`, `@fsub`
	- `@mul`, `@smulWrap`, `@umulWrap`, `@smulSat`, `@umulSat`, `@fmul`
	- `@sdiv`, `@udiv`, `@fdiv`
	- `@srem`, `@urem`, `@frem`


<!---------------------------------->
## v0.0.71.0

### Panther
- Added support for lowering of:
	- function calls
	- type conversion intrinsics
	- type traits intrinsics
	- multi-assign
	- conditionals
	- `@breakpoint`
	- `@_printHelloWorld`
	- `@add`, `@addWrap`, `@fadd`
- Fixed lowering of loading local variables
- Panic messages when executing the `Run` build target are now emitted as a diagnostic instead of going directly to stdout

### PIR
- Fixed fatal errors caused by lowering a function to LLVM-IR with parameters when multiple functions exist in the module
- Fixed fatal errors when lowering a float value when the `core::GenericFloat` didn't match the width of the type
- Fixed `@fadd` instructions not being created correctly
- Fixed formatting issue when printing type conversion instructions
- Fixed `@uAddWrap` incorrectly being printed as `@sAddWrap`
- Fixed `@uAddWrap` being lowered to LLVM-IR as `@llvm.sadd.with.overflow()`
- Fixed `@uAddWrap` and `@sAddWrap` wrapped values when lowered to LLVM-IR being the result
- Fixed compile-error that would occur when building an optimized build

### PCIT_core
- Fixed fatal errors around iteration of `StepAlloc`


<!---------------------------------->
## v0.0.70.0

### Panther
- When emitting `main` function, it now correctly returns `CInt`

### PIR
- Added type conversion instructions
	- `@bitCast`
	- `@trunc`, `@ftrunc`
	- `@sext`, `@zext`, `@fext`
	- `@itof`, `@uitof`, `@ftoi`, `@ftoui`
- Added removal of `@calcPtr` instructions when all indices are 0 in the instCombine pass


<!---------------------------------->
## v0.0.69.0

### PIR
- Added the `@fAdd` instruction
	- can no longer use the `@add` instruction for floats
- Condensed Signed and Unsigned into Integer
- Split `@addWrap` into `@sAddWrap` and `@uAddWrap`


<!---------------------------------->
## v0.0.68.0

### Panther
- Added printing PIR
	- Not all features are completely supported
- Added checking for moving a non-mutable value

### pthr
- Added the `PrintPIR` build target

### PIR
- Fixed fatal errors when lowering to LLVM-IR when multiple functions are defined

### pirc
- Added logo to output


<!---------------------------------->
## v0.0.67.0

### PIR
- Added `@breakpoint` instruction
- Added `@unreachable` instruction
- Added `@condBranch` instruction


<!---------------------------------->
## v0.0.66.0

### PIR
- Added `@calcPtr` instruction
- Fixed fatal error when compiling an array type into LLVM-IR


<!---------------------------------->
## v0.0.65.0

### plnk
- Added the plnk project. General project layout has been created
- Added basic linking for Windows


<!---------------------------------->
## v0.0.64.0

### PIR
- Added lowering to assembly
- Added lowering to object file
- Added LLVM default optimization passes


<!---------------------------------->
## v0.0.63.0

### PIR
- Added struct and array values for global vars


<!---------------------------------->
## v0.0.62.1

### Misc
- Updated the README to point to the [PCIT website](pcit-project.github.io).


<!---------------------------------->
## v0.0.62.0

### PIR
- Added string values for global vars
- Added alignment to global variables in LLVM-IR output
- Fixed some size and alignment calculations


<!---------------------------------->
## v0.0.61.1

### PIR
- Fixed memory corruption issue


<!---------------------------------->
## v0.0.61.0

### PIR
- Added `@load` and `@store` instructions
- Adjusted positioning of statement attributes
- Fixed statement automatic naming for `@addWrap` when result and wrapped names are the same not being unique


<!---------------------------------->
## v0.0.60.1

### PIR
- Fixed various bugs and fatal errors around stmt passes and reverse stmt passes in `PassManager`
- Fixed fatal error around erasing `Expr`s


<!---------------------------------->
## v0.0.60.0

### PIR
- Added the ability for `Agent` to insert at any index within the basic block
- Added optimization to convert `@addWrap` to `@add` if wrapped value isn't used
- Added various debug asserts
- Fixed fatal error when adding statements during a statement pass
- Fixed various compiler warnings


<!---------------------------------->
## v0.0.59.0

### PIR
- Added `@addWrap` instruction


<!---------------------------------->
## v0.0.58.0

### PIR
- Added `JITEngine`
- Added setting of OS and Architecture
- Fixed fatal error when a function doesn't have any allocas


<!---------------------------------->
## v0.0.57.0

### PIR
- Added lowering to LLVMIR
- Changed `@br` to `@branch`


<!---------------------------------->
## v0.0.56.0

### PIR
- Added `alloca` statements
- Added `passes::removeUnusedStmts`
- Added reverse stmt passes to `PassManager`
- Added local name collision (registers, basic blocks, params) prevention and automatic naming of statements and blocks
- Added checking for name collisions of globals
- Added checking of valid names
- Added printing of non-standard function and global names
- Fixed `Agent::createRetInst()` not appending to the target basic block


<!---------------------------------->
## v0.0.55.0
- Updated License to Apache License v2.0 with LLVM and PCIT exceptions
- Fixed LLVM license missing


<!---------------------------------->
## v0.0.54.1

### PIR
- Made most functions of `Agent` const
- Modified API of `PassManager` passes to `const Agent&` instead of `Agent&`
- Removed `;`s from printing
- Fixed printing of `@add` instruction
- Fixed potential issues when multithreading `PassManager`


<!---------------------------------->
## v0.0.54.0

### PIR
- Added `Agent`
- Added `ReaderAgent`
- Modified API of `PassManager` passes to use `Agent`s
- Moved expr creation and reading from `Module` and `Function` to `Agent` and `ReaderAgent`
- Replacing instructions now properly deletes the replaced instruction


<!---------------------------------->
## v0.0.53.0

### PIR
- Added `PassManager`
- Added `passes::instCombine`
	- `passes::instSimplify`
	- `passes::constantFolding`

### PCIT_core
- Added `SpinLock`
- Added `ThreadPool`


<!---------------------------------->
## v0.0.52.0

### PIR
- Added the PIR project. General project layout has been created

### PCIT_core
- Added `StepAlloc`
- Added the ability to iterate over `LinearStepAlloc`
- Added `Printer` support printing to `std::string` as an option
- Added format arg versions of `Printer::print()` and `Printer::println()`


<!---------------------------------->
## v0.0.51.0

### Panther
- Added `while` loops
- Fixed fatal error when printing AST of conditional (`if` and `when`)
- Fixed `as` expressions sometimes being considered as not ephemeral


<!---------------------------------->
## v0.0.50.1

### Panther
- Fixed comptime arithmetic operators not always being precomputed correctly
- Fixed comptime logical operators not always being precomputed correctly


<!---------------------------------->
## v0.0.50.0

### Panther
- Added bitwise intrinsic functions:
	- `@and`
	- `@or`
	- `@xor`
	- `@shl`
	- `@shlSat`
	- `@shr`


<!---------------------------------->
## v0.0.49.0

### Panther
- Added logical operators:
	- `==`
	- `!=`
	- `<`
	- `<=`
	- `>`
	- `>=`
- Added logical intrinsic functions:
	- `@eq`
	- `@neq`
	- `@lt`
	- `@lte`
	- `@gt`
	- `@gte`
- Added type trait intrinsic function `@isBuiltin`
	

<!---------------------------------->
## v0.0.48.0

### Panther
- Added support for arithmetic operators
- Added support for fluid `def` variables
	- `def` variables that have the value of a fluid literal and use type-inference act as fluid


<!---------------------------------->
## v0.0.47.0

### Panther
- Added support for all arithmetic intrinsics to be run at compile-time
- Added support for checked arithmetic in compile-time executed functions
- Fixed fatal error when using `as` operator with type `BF16`


<!---------------------------------->
## v0.0.46.0

### Panther
- Added arithmetic intrinsic functions:
	- `@add`
	- `@addWrap`
	- `@addSat`
	- `@fadd`
	- `@sub`
	- `@subWrap`
	- `@subSat`
	- `@fsub`
	- `@mul`
	- `@mulWrap`
	- `@mulSat`
	- `@fmul`
	- `@div`
	- `@fdiv`
	- `@rem`
- Added new settings to `Context::Config`
	- `addSourceLocations`
	- `checkedArithmetic`
- Improved performance of compile-time execution
- Renamed some intrinsics:
	- `@TruncFloatPoint` -> `@ftrunc`
	- `@ExtFloatPoint` -> `@fext`
	- `@IntegralToFloatPoint` -> `@itof`
	- `@UIntegralToFloatPoint` -> `@uitof`
	- `@FloatPointToIntegral` -> `@ftoi`
	- `@FloatPointToUIntegral` -> `@ftoui`
- Fixed `@isFloatingPoint` incorrectly being an unknown intrinsic


<!---------------------------------->
## v0.0.45.0

### Panther
- Added `as` operator
- Improved error recovery of conditionals
- Fixed underlying type of `RawPtr` being `UI64` (should be `RawPtr`)
- Fixed `@bitCast` between integrals and pointers


<!---------------------------------->
## v0.0.44.0

### Panther
- Added type conversion intrinsics:
	- `@trunc`
	- `@truncFloatPoint`
	- `@sext`
	- `@zext`
	- `@extFloatPoint`
	- `@integralToFloatPoint`
	- `@uintegralToFloatPoint`
	- `@floatPointToIntegral`
	- `@floatPointToUIntegral`


<!---------------------------------->
## v0.0.43.0

### Panther
- Added `alias` declarations
- Added primitive-type `TypeID`
- Added intrinsic function `@getTypeID`
- Added Type ID conversion expression
- Improved rules around read-only-ness of chained type qualifiers
- Fixed fatal error when creating a global variable of type `CLongDouble` that was set to `zeroinit`
- Fixed fatal error caused by error when parsing a type
- Fixed incorrect error diagnostic being emitted that the function scope was terminated after a sub-function declaration
- Fixed fatal error when trying to run a compile-time function declared inside a runtime function

## Misc
- Updated README to move Panther example to new [PCIT website](pcit-project.github.io)


<!---------------------------------->
## v0.0.42.0

### Panther
- Added intrinsics:
	- `@isSameType`
	- `@isTriviallyCopyable`
	- `@isTriviallyDestructable`
	- `@isPrimitive`
	- `@isIntegral`
	- `@isFloatingPoint`
	- `@bitCast`
- Added checking for `@sizeOf` of type `Void`
- Fixed else blocks of `when` conditionals always considering stmts inside to be in global scope
- Fixed incorrect fatal error when using some value kinds as the assignment value in a variable declaration


<!---------------------------------->
## v0.0.41.2

### Panther
- Added checking for circular dependencies in comptime
- Added support for comptime functions that return `Bool`, `Char`, `F32`, and `F64`
- Fixed printing of type of literal integers and literal floats


<!---------------------------------->
## v0.0.41.1

### Panther
- Fixed fatal error when calling a templated function as a comptime value
- Fixed fatal error when calling functions in comptime not declared before usage


<!---------------------------------->
## v0.0.41.0

### Panther
- Added compile-time execution
	- Just proof of concept
	- Not all required checking or features are implemented
- Fixed termination checking for conditionals without `else` blocks

### PCIT_core
- Added `GenericInt`
- Added `GenericFloat`
- Added `GenericValue`


<!---------------------------------->
## v0.0.40.1

### Panther
- Fixed `def` variables defined in local scope being emitted


<!---------------------------------->
## v0.0.40.0

### Panther
- Fixed fatal error with usage of templated functions
- Improved performance around memory allocations / reallocations
- Fixed `TypeManager` not deallocating memory for function types

### PCIT_core
- Added `LinearStepAlloc`


<!---------------------------------->
## v0.0.39.0

### Panther
- Added the `#runtime` function attribute
- Added read-only address-of operator (`&|`)
- Added checking for type inference of an initializer value
	- Also fixes a fatal error caused by attempting to do type inference of an initializer value
- Removed conditional argument from attribute `#mustLabel`
- Improved / simplified error diagnostic messages about missing attributes
- Fixed fatal error caused by declaring function overloads with different number of parameters
- Fixed calling an overloaded functions causing a diagnostic to be emitted that it is not callable
- Fixed fatal error caused by in some cases having a type mismatch in a failed overload resolution
- Fixed fatal error when printing `...` token

## Misc
- Slight tweaks to README


<!---------------------------------->
## v0.0.38.0

### Panther
- Added conditional argument to attributes:
	- `#pub`
	- `#mustLabel`
- Added checking for if attributes were already set
- Added support for attributes to have 2 arguments
- Added checking of valid attribute arguments
- Fixed a function being marked as `#entry` being templated not being properly handled


<!---------------------------------->
## v0.0.37.2

### Panther
- Added checking for initializer values being used as function call expression arguments
- Fixed diagnostic being emitted that the intrinsic `@import` was unknown if the call was a statement


<!---------------------------------->
## v0.0.37.1

### Panther
- Added intrinsic function `@_printHelloWorld`

### pthr
- Added nicer logo when compiling in verbose mode


<!---------------------------------->
## v0.0.37.0

### Panther
- Added support for intrinsic functions
- Added intrinsic function `@breakpoint`
- Added intrinsic function `@sizeOf`


<!---------------------------------->
## v0.0.36.1

### Panther
- Fixed fatal error when calling a function pointer that as a statement


<!---------------------------------->
## v0.0.36.0

### Panther
- Added `if` conditionals
- Added `unreachable` statements
- Added checking that a function that doesn't return `Void` is terminated
- Fixed checking of termination in scoped statement blocks
- Fixed breakpoint being thrown when attempting to dereference a value that's not a pointer


<!---------------------------------->
## v0.0.35.0

### Panther
- Added `when` conditionals (compile time conditionals - doesn't open a new scope)
- Added parsing of `if` conditionals

### Misc
- Improved README


<!---------------------------------->
## v0.0.34.0

### Panther
- Added local scope statement blocks
- Added explicit typing of block expressions


<!---------------------------------->
## v0.0.33.0

### Panther
- Added global variables
- Added checking of valid variable attributes
- Fixed identifier re-declaration diagnostic not always adding the "First defined here" info
- changed wrapping arithmetic operator syntax from `@` to `%`
	- `+@`  -> `+%`
	- `+@=` -> `+%=`
	- `-@`  -> `-%`
	- `-@=` -> `-%=`
	- `*@`  -> `*%`
	- `*@=` -> `*%=`


<!---------------------------------->
## v0.0.32.0

### Panther
- Added function pointers
	- Note: there is no way to write a function pointer type yet
- Fixed fatal error when copying or moving a function
- Made various improvements to some diagnostic messages


<!---------------------------------->
## v0.0.31.1

### Panther
- Fixed function overloads not respecting differences in parameter kinds
- Fixed fatal error when taking the address of an expression with multiple values


<!---------------------------------->
## v0.0.31.0

### Panther
- Added function overloading
- Fixed diagnostic source locations from failing to match function overload
- Fixed fatal error when returning a value in a function that has return type `Void`


<!---------------------------------->
## v0.0.30.1

### Panther
- Fixed fatal error messages caused by setting a single value with multiple values (multiple return param function)


<!---------------------------------->
## v0.0.30.0

### Panther
- Added multiple function return parameters
- Added checking that entry function does not have named return parameter
- Fixed discarding of a return value of a function with a named return parameter


<!---------------------------------->
## v0.0.29.0

### Panther
- Added support for initializer value expression `uninint`
- Added initializer value expression `zeroinit`
- Temporarily removed specific diagnostic codes for semantic analysis


<!---------------------------------->
## v0.0.28.0

### Panther
- Added support for named return values
- Added support for `move` expressions
- Improved the API of `ScopeManager::ScopeLevel` to make it nicer and increase performance
- Improved the wordings of some error diagnostics
- Fixed fluid literals not correctly being classified as ephemeral when passed into an `in` parameter


<!---------------------------------->
## v0.0.27.0

### Panther
- Added support for function parameters
- Added parameter attribute `#mustLabel`
- Re-added primitive-type `F80`
- *[Experimental]* Added clickable source file locations in diagnostics (only works if terminal supports it) 
- Fixed functions with return types that are from the template instantiation
- Fixed not properly checking for identifier reuse within a template parameter block
- Fixed checking for identifier reuse not including expression template arguments


<!---------------------------------->
## v0.0.26.0

### Panther
- Added support for prefix `&` operator (address-of)
- Added support for postfix `.*` operator (dereference)
- Added checking for invalid type qualifiers (read-only pointer above a mutable pointer)
- Fixed `def` variables not always working as ephemeral values
- Fixed tokenization issues with caused by interactions with multiple `&` or `|`


<!---------------------------------->
## v0.0.25.0

### Panther
- Added function call expressions
- Added checking for discarding of function return values
- Added support for the discard statement
- Fixed LLVM IR not having readable register names


<!---------------------------------->
## v0.0.24.0
Added the ability to run compiled Panther code

### Panther
- Added support for return values
- Added support for functions with single-value, unnamed returns
- Added function attribute `#entry` 
- Added Panther runtime generation
- Fixed LLVM IR functions not having the correct return type

### pthr
- Added build target `Run`
- Added Panther runtime generation to the `LLVMIR` build target


<!---------------------------------->
## v0.0.23.0

### Panther
- Added support for return statements
- Added tokenizing and parsing of return statements for multiple returns


<!---------------------------------->
## v0.0.22.0

### Panther
- Added support for variables to be used as consteval values
- Added support for assignment expressions
- Added support for `copy` expressions
- Fixed fatal error when encountering an invalid global statement
- Fatal diagnostics now always emit whether hit max errors or not. Additionally, no more errors should be emitted after it
- Fixed diagnostic "Cannot get a consteval value from a variable that isn't def" not giving the correct source location


<!---------------------------------->
## v0.0.21.0

### Panther
- Added `const` variables
- Added intrinsic function `@import`
	- hacked in at the moment, so no checking of parameters
- Added the function attribute `pub`
- Allowed functions to have attributes
- Fixed fluid literals not being converted into ephemeral values properly
- Improved source code location in diagnostics for accessor operators
- Improved readability of type mismatch diagnostic


<!---------------------------------->
## v0.0.20.0

### Panther
- Added templated function with value parameters


<!---------------------------------->
## v0.0.19.0

### Panther
- Added templated function with `Type` parameters
- Fixed issue where no statements were analyzed after a function call (in semantic analysis)
- Fixed local variables declared before a locally-declared function were accepted as part of the scope
- Fixed identifiers `I`, `U`, and `UI` causing fatal errors tokenizer

### pthr
- Added build target `LLVMIR`


<!---------------------------------->
## v0.0.18.0

### Panther
- Added function call statements
- Added variable expressions

### PCIT_core
- Fixed `Optional` sometimes not selecting a constructor as expected


<!---------------------------------->
## v0.0.17.0

### Panther
- Added variable declarations
- Added the concept of fluid literals
	- literal ints can become any integral type
	- literal floats can become any float type
- Removed primitive-type `F80`


<!---------------------------------->
## v0.0.16.0
Added LLVM to the build

### Panther
- Added compiling to LLVMIR
- Temporarily disallowed functions with that return anything other than `Void`

### pthr
- Added `PrintLLVMIR` build target

### PCIT_core
- Added `IterRange`



<!---------------------------------->
## v0.0.15.0

### Panther
- Added support for declaration of functions inside functions
- Added more support for the `mayRecover` config option

### PCIT_core
- Added `UniqueID` and `UniqueComparableID`


<!---------------------------------->
## v0.0.14.2

### Panther
- Added detection of valid statements within functions
- Added `mayRecover` option to `Context::Context`
	- if `true`, can continue semantic analysis on that file if encountered a recoverable error (won't exceed `maxErrors`)


<!---------------------------------->
## v0.0.14.1

### Panther
- Added function return type checking
- Added functions with multiple returns
- Changed max tokens per source file to 2^32-2 (from 2^32-1)
- Fixed fatal error doing semantic analysis when multi-threaded

### PCIT_core
- Added the `Optional` interface
	- simple way to overload `std::optional` for user-types


<!---------------------------------->
## v0.0.14.0

### Panther
- Added checking of supported statements in semantic analysis
- Added checking of function re-definition
- Added the BF16 type ("brain" floating point - 7-bit significand)
- Fixed fatal errors caused by diagnostics with infos
- Fixed end of file checking for tokenization of operators and identifiers
- Fixed unstable behaviour with function parameters in the AST
- Changed indentation level of diagnostic infos in the default diagnostic callback


<!---------------------------------->
## v0.0.13.0

### Panther
- Added basic semantic analysis (checking of valid global statements)

### PCIT_core
- Added fatal diagnostics with source locations (`DiagnosticImpl::createFatalMessage`)


<!---------------------------------->
## v0.0.12.1

### Panther
- Fixed string and character literal tokens sometimes pointing to garbage/invalid strings


<!---------------------------------->
## v0.0.12.0

### Panther
- Added `def` variable declaration
- Added `alias` declarations


<!---------------------------------->
## v0.0.11.0

### Panther
- Separated Tokens and Token locations (more data-oriented)
- Fixed string and character literal tokens pointing to garbage/invalid strings
- Added checking if a source location is too big (max lines / column number is 2^32-1)


<!---------------------------------->
## v0.0.10.3

### Panther
- Added checking for valid characters


<!---------------------------------->
## v0.0.10.2

### Panther
- Removed type expressions (except after an `as` operator)
- Improved diagnostic for cases like `foo as SomeOptionalAlias.?`
- Improved diagnostic for cases like `foo as Int?.?`
- Improved the performance of the Parser
- Fixed typo in Parser diagnostic "Expected value after [=] in variable declaration, ..."


<!---------------------------------->
## v0.0.10.1
- Fixed various compile errors caused by update to MSVC
- Minor tweaks to coding style


<!---------------------------------->
## v0.0.10.0

### Panther
- Added templated functions
- Added templated expressions
- Added requirement that expression blocks must have labels
- Added requirement that statement scope blocks must not have labels
- Fixed parser incorrectly detecting statement scope blocks as statement expressions

### pthr
- Fixed some AST printing formatting issues


<!---------------------------------->
## v0.0.9.1

### Panther
- Removed unnamed (`___`)
	- It may come back at some point, but it's being considered if it would lead to bad practices


<!---------------------------------->
## v0.0.9.0

### Panther
- Added scoped statement blocks
- Added statement block labels
- Added block statements (essentially immediately invoked lambda expressions)
- Added attributes (for variables, functions, and function parameters)
- Added function multiple / named returns
- Added discard and unnamed assignment
- Added multiple assignment


<!---------------------------------->
## v0.0.8.0

### Panther
- Improved performance and greatly reduced memory utilization of tokenization
- Added function calls
- Added types: 
	`This`
	- `Int`
	- `ISize`
	- arbitrary bit-width integers (example: `I12`)
	- `UInt`
	- `USize`
	- arbitrary bit-width unsigned integers (example: `UI12`)
	- `F16`
	- `F32`
	- `F64`
	- `F80`
	- `F128`
	- `Byte`
	- `Bool`
	- `Char`
	- `RawPtr`
	- `CShort`
	- `CUShort`
	- `CInt`
	- `CUInt`
	- `CLong`
	- `CULong`
	- `CLongLong`
	- `CULongLong`
	- `CLongDouble`
- Added keyword `this`
- Fixed tokenizer continuing even if an error occurred in the file
- Fixed fatal error when using non-base-10 floating-point literals


<!---------------------------------->
## v0.0.7.0

### Panther
- Added type qualifiers:
	- `*`
	- `*|` (read-only pointer)
	- `?` (optional)
	- `*?` (optional pointer)
	- `*|?` (optional read-only pointer)
- Added accessor operators:
	- `.`
	- `.*` (dereference)
	- `.?` (unwrap)
- Added assignment statements
- Added assignment operators:
	- `+=`
	- `+@=`
	- `+|=`
	- `-=`
	- `-@=`
	- `-|=`
	- `*=`
	- `*@=`
	- `*|=`
	- `/=`
	- `%=`
	- `<<=`
	- `<<|=`
	- `>>=`
	- `&=`
	- `|=`
	- `^=`
- Added parsing of intrinsics
- Changed prefix operator `addr` to prefix operator `&`


<!---------------------------------->
## v0.0.6.0

### Panther
- Added keyword `uninit`
- Added built-in-types:
	- `Type`
	- `Bool`
- Added prefix operators:
	- `copy`
	- `addr`
	- `move`
	- `-`
	- `!`
	- `~`
- Added arithmetic infix operators:
	- `+`
	- `+@` (wrapping addition)
	- `+|` (saturating addition)
	- `-`
	- `-@` (wrapping subtraction)
	- `-|` (saturating subtraction)
	- `*`
	- `*@` (wrapping multiplication)
	- `*|` (saturating multiplication)
	- `/`
	- `%`
- Added comparative infix operators:
	- `==`
	- `!=`
	- `<`
	- `<=`
	- `>`
	- `>=`
- Added logical infix operators:
	- `&&`
	- `||`
- Added bitwise infix operators:
	- `<<`
	- `<<|` (saturating shift left)
	- `>>`
	- `&`
	- `|`
	- `^`
- Added the `as` infix operator
- Fixed tokenizing of boolean literals
- Added guarantee that memory addresses of `Source`s remain stable
- Added checking that file doesn't have too many tokens
- Improved printing of version in verbose mode to include the build config if not in `ReleaseDist`


<!---------------------------------->
## v0.0.5.0

### Panther
- Added basic parsing of function declarations
- Made `AST::Node` and `AST::NodeOptional` trivially copyable

### pthr
- Added `max_num_errors` to config

### Misc
- Fixed links in [contributing policy](https://github.com/PCIT-Project/PCIT-CPP/blob/main/CONTRIBUTING.md) and [security policy](https://github.com/PCIT-Project/PCIT-CPP/blob/main/SECURITY.md) to point to the correct headings


<!---------------------------------->
## v0.0.4.0

### Panther
- Added parsing
	- variable declarations
	- `Void` and `Int` types
	- Literal expressions
	- Identifier expressions

### pthr
- Added `PrintAST` build target

### Misc
- Added [contributing policy](https://github.com/PCIT-Project/PCIT-CPP/blob/main/CONTRIBUTING.md), [code of conduct](https://github.com/PCIT-Project/PCIT-CPP/blob/main/CODE_OF_CONDUCT.md), and [security policy](https://github.com/PCIT-Project/PCIT-CPP/blob/main/SECURITY.md).


<!---------------------------------->
## v0.0.3.0
Added Tokenization and improved diagnostics

### Panther
- Added `Token` and `TokenBuffer`
- Added tokenization
- Fixed fatal error when multiple worker threads reported an error at the same time
- Added pointing to source code in diagnostics

### pthr
- Added build targets to config


<!---------------------------------->
## v0.0.2.0
Setup the threading system for Panther `Context`. Allows for both single-threading and multi-threading

### Panther
- Added a number of functions to `Context` related to the threading and tasks
- Added `Context::loadFiles()`

### pthr
- Setup basic runtime config settings

### Misc
- Added the testing directory


<!---------------------------------->
## v0.0.1.0
Setting up basic frameworks

### Panther
- Added `Context`
- Added `Source`
- Added `SourceManager`
- Added diagnostics

### PCIT_core
- Added `DiagnosticImpl`
- Added `Printer`
- Added `UniqueID` and `UniqueComparableID`


<!---------------------------------->
## v0.0.0.0
- Initial commit
- Setup build system for Evo, Panther, and pthr