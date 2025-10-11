# Change Log

<!---------------------------------->
<a name="v0.0.169.0"></a>
## v0.0.169.0

### Panther
- Added enums
- Added checking if expression is in function scope (also fixes crashing when doing so):
	- operator `copy`
	- operator `move`
	- operator `forward`
	- `this`
- Fixed printing of AST for Union member statements
- Fixed error diagnostic for when a fluid integer is negative is the target type is unsigned
- Fixed crashing when using global `const` variable outside of function scope


<!---------------------------------->
<a name="v0.0.168.1"></a>
## v0.0.168.1

### Panther
- Minor code refactors


<!---------------------------------->
<a name="v0.0.168.0"></a>
## v0.0.168.0

### Panther
- Added tagged unions
	- Added comparason of the tag
- Fixed ouptut parameter of constructors of types with no members not counting as initialized

### PIR
- Added `@switch`


<!---------------------------------->
<a name="v0.0.167.0"></a>
## v0.0.167.0

### Panther
- Added support for interface methods with `this` parameters
- Fixed interfaces not being considered type scope in semantic analysis

### pthr
- Improved messaging of invalid paths for files/projects 
- Fixed project ids being incorrect for compilation source files when adding the Panther standard library


<!---------------------------------->
<a name="v0.0.166.1"></a>
## v0.0.166.1

### Panther
- Fixed template instantiation on interfaces only working properly if the method has the same name


<!---------------------------------->
<a name="v0.0.166.0"></a>
## v0.0.166.0

### Panther
- Added checking that all return parameters are initialized at a `return` statement
	- same with error returns
- Added warning for `delete` statement of moved-from expression
- Added warning for `delete` statement of a trivially-deletable type
- Improved diagnostic message for why an argument of a `delete` statement is invalid
- Fixed diagnostic locations for builtin symbols
- Fixed initialization checks is some cases for:
	- return parameters
	- error exception parameters
	- block expression return parameters
- Fixed constexpr lowering of functions with error return values


<!---------------------------------->
<a name="v0.0.165.0"></a>
## v0.0.165.0

### Panther
- Added implicit RVO


<!---------------------------------->
<a name="v0.0.164.0"></a>
## v0.0.164.0

### Panther
- Added optional `.extract()`
- Fixed lowering of operators `copy` and `move` sometimes being incorrect for trivial types


<!---------------------------------->
<a name="v0.0.163.0"></a>
## v0.0.163.0

### Panther
- Added operator overloading `copy` and `move`
- Added checking that all members of the output of operator `new` are initialized
- Added missing deletion of the LHS of assignment
- Fixed not deleting in default `new` assignment operator


<!---------------------------------->
<a name="v0.0.162.0"></a>
## v0.0.162.0

### Panther
- Added automatic creation of operator `delete`
- Added checking that block expressions declare their return parameters
	- Fixes crashing if they aren't declared
- Fixed automatic calling of deleters not happening at `break` and `continue` statements
- Fixed defers and automatic calling of deleters not happening at the right time in block scopes
- Fixed value state tracking hoisting declarations into parent scopes
- Fixed crashing when lowering a discarding assignment of a fluid value


<!---------------------------------->
<a name="v0.0.161.1"></a>
## v0.0.161.1

### Panther
- Fixed race condition for compiler-constructed constexpr default operator `new`


<!---------------------------------->
<a name="v0.0.161.0"></a>
## v0.0.161.0

### Panther
- Added `delete` statement
- Fixed missing automatic deletion of members in operator `delete`
- Fixed trivially-default-initable detection not taking default member values into account
- Fixed lowering of `defer` and `deleters` in functions that return `Void` that have an implicit `return`
- Fixed automatic deleters sometimes being run in the wrong order


<!---------------------------------->
<a name="v0.0.160.0"></a>
## v0.0.160.0

### Panther
- Added overloading operator `delete`
- Added compiler created default operator `new` using default values


<!---------------------------------->
<a name="v0.0.159.0"></a>
## v0.0.159.0

### Panther
- Added compiler default generated operator `new`
- Added operator `new` for primitives
- Fixed missing checks that arguments to operator `new` for optional and array reference are ephemeral
- Fixed comptime calling of operator `new`


<!---------------------------------->
<a name="v0.0.158.0"></a>
## v0.0.158.0

### Panther
- Added operator `new` of optionals
- Added operator `new` of array references
- Added checking of operator `as` to array reference if it not being read-only is valid
- Fixed crashing and/or incorrect behavior when instantiating template functions that are defined in a different file
- Fixed assert firing in debug mode when dynamically importing files
- Fixed crashing when getting size of an optional
- Fixed crashing when calling methods on builtin type

### pthr
- Fixed printing of AST of functions that are operator overloads


<!---------------------------------->
<a name="v0.0.157.0"></a>
## v0.0.157.0

### Panther
- Added overloading operator `new`
- Added checking for function parameters that are uninitialized pointers
- Fixed member vars incorrectly erroring as shadowed
- Fixed return parameters and error return parameters not being uninitialized
- Fixed lowering of methods with parameters


<!---------------------------------->
<a name="v0.0.156.0"></a>
## v0.0.156.0

### Panther
- Added logical infix operators (`&&`, `||`)
- Added support for lowering builtin types only if they are used

### PIR
- Added `@phi` instruction


<!---------------------------------->
<a name="v0.0.155.1"></a>
## v0.0.155.1

### Panther
- Fixed bitwise logical operators (`&`, `|`, `^`) for `Bool`


<!---------------------------------->
<a name="v0.0.155.0"></a>
## v0.0.155.0

### Panther
- Added support for macros with values
	- integer values
	- float values
	- bool values
	- identifier values
	- parenthesis enclosed values
	- brace enclosed values
- Fixed condition in when conditional not being properly classified as constexpr (which could in some cases cause crashes)
- Fixed constexpr infix math operators for `Bool`
- Fixed constexpr operator `as` for `Bool`
- Fixed constexpr infix math operators not maintaining correct types


<!---------------------------------->
<a name="v0.0.154.1"></a>
## v0.0.154.1

### Panther
- Fixed race condition when using type `This`


<!---------------------------------->
<a name="v0.0.154.0"></a>
## v0.0.154.0

### Panther
- Added build system intrinsic functions
	- `@buildCreateProject`
	- `@buildAddSourceFile`
	- `@buildAddCHeaderFile`
	- `@buildAddCPPHeaderFile`
- Added `@pthr`
	- Added `@pthr.ProjectWarningSettings`
- Added parsing of accessors on intrinsics
- Fixed intrinsics incorrectly being allowed in comptime
	- `@abort`
	- `@breakpoint`
	- `@builtSetNumThreads`
	- `@buildSetOutput`
	- `@buildSetUseStdLib`


<!---------------------------------->
<a name="v0.0.153.0"></a>
## v0.0.153.0

### Panther
- Added builtin-type methods
	- Array `.size()`
	- Array `.dimensions()`
	- Array reference `.size()`
	- Array reference `.dimensions()`
- Fixed crashing when doing some invalid `as` conversions


<!---------------------------------->
<a name="v0.0.152.1"></a>
## v0.0.152.1

### Panther
- Fixed crashing when printing Char expressions


<!---------------------------------->
<a name="v0.0.152.0"></a>
## v0.0.152.0

### Panther
- Added uninitialized pointers
- Added array references
- Fixed printing of array types having an extra comma
- Fixed crashing when there are too few indices in an indexer
- Fixed PIR lowering of multi-dimensional arrays having the dimensions in the wrong order


<!---------------------------------->
<a name="v0.0.151.0"></a>
## v0.0.151.0

### Panther
- Added importing C/C++ function definitions
- Added importing C/C++ global variables
- Added support for mangled functions


<!---------------------------------->
<a name="v0.0.150.0"></a>
## v0.0.150.0

### Panther
- Added checking for usage of uninitialized and moved-from values
- Added union designated initializer operator new


<!---------------------------------->
<a name="v0.0.149.1"></a>
## v0.0.149.1

### Panther
- Fixed array initializer new with aliases
- Fixed array indexers when the type is an alias

### PIR
- Fixed first indentation level being 3 spaces instead of 4


<!---------------------------------->
<a name="v0.0.149.0"></a>
## v0.0.149.0

### Panther
- Added support for constexpr functions returning:
	- arrays
	- alias
	- union
- Added support for constexpr functions with parameters of type:
	- struct
	- array
	- union
- Improved performance of running a constexpr function at compile-time
- Fixed race condition in semantic analysis when using a struct
- Fixed code generation of copying a reference parameter always copying the size of a pointer
- Fixed decision if a read parameter should be a copy not waiting for the definition of the parameter type


<!---------------------------------->
<a name="v0.0.148.0"></a>
## v0.0.148.0

### Panther
- Added importing of C/C++ macros definitions
- Added `@isMacroDefined()`
- Added type `CWChar`
- Updated LibC
	- Fixes errors when including `immintrin.h`
- Improved binary size by not linking against unused LLVM libs
	- Fixed build warning
- Removed `core::IterRange`
	- (now part of Evo, so using that version)
- Fixed optimized builds failing due to being too large
- Fixed name mangling of C/C++ union types
- Fixed various crashes when importing C/C++ code


<!---------------------------------->
<a name="v0.0.147.1"></a>
## v0.0.147.1

### Panther
- Added missing error diagnostics for failing to instantiate functions
- Fixed crashing when failing to deduce an parameter type for function template instantiation
- Fixed templated methods
- Fixed `@getNumBytes()` and `@getNumBits()` giving the wrong value for empty types


<!---------------------------------->
<a name="v0.0.147.0"></a>
## v0.0.147.0

### Panther
- Added importing C function declarations
- Fixed `@getNumBytes` giving the wrong value for some widths of arbitrary bit-width integers
- Fixed C compilation 
	- of types with attributes
	- of types within parentheses
	- of function types
	- include paths for Linux
	- unnamed types
	- types like `const char*` being imported as `Char*?` instead of `Char*|?`

### Misc
- Added missing files needed for libc


<!---------------------------------->
<a name="v0.0.146.0"></a>
## v0.0.146.0

### Panther
- Added support for importing C unions
- Added support for importing C structs


<!---------------------------------->
<a name="v0.0.145.0"></a>
## v0.0.145.0

### Panther
- Added diagnostic reasons for failing to match template function overload
- Added checking for too many function template arguments
- Fixed template methods expecting the `this` parameter as an argument


<!---------------------------------->
<a name="v0.0.144.0"></a>
## v0.0.144.0

### Panther
- Added untagged unions
- Added checking that the type of argument of operator `copy` is copyable
- Added checking that the type of argument of operator `move` is movable
- Improved memory usage of Semantic Analysis
- Fixed printing of AST for struct (no arrow to attribute block minor-header)
- Fixed template type parameter declaration being `type` when it should be `Type`


<!---------------------------------->
<a name="v0.0.143.1"></a>
## v0.0.143.1

### Panther
- Fixed semantic analysis of automatic dereferencing accessor being considered ephemeral when it should have been concrete
- Fixed implicit conversion of fluid values to optionals


<!---------------------------------->
<a name="v0.0.143.0"></a>
## v0.0.143.0

### Panther
- Added type `This`
- Changed TypeID converter from `Type(...{EXPR}...)` to `type(...{EXPR}...)`

### Misc
- Removed build mode "Debug"
- Renamed build mode "Dev" to "Debug"


<!---------------------------------->
<a name="v0.0.142.1"></a>
## v0.0.142.1

### Panther
- Fixed crashing when compiling C/C++ code
- Fixed crashing in default diagnostic callback for Clang errors that point to after the code line

### Misc
- Updated LibC
	- Also fixes warnings from Clang such as when compiling `stdio.h`


<!---------------------------------->
<a name="v0.0.142.0"></a>
## v0.0.142.0

### Panther
- Added optional types
- Added null checking of optional types
- Added optional unwrapping
- Added specific error diagnostic when moving an in-parameter
- Fixed constexpr functions with in-parameters
- Fixed calls to functions with in-parameters sometimes not calling the correct version of the target function
- Fixed PIR lowering of indexers in certain cases (such as assignment)

### PIR
- Fixed `Agent` incorrectly asserting that `@ieq` cannot compare pointers
- Fixed `Agent` incorrectly asserting that `@ineq` cannot compare `Bool`s


<!---------------------------------->
<a name="v0.0.141.0"></a>
## v0.0.141.0

### Panther
- Added array initializer operator `new`
- Added function call statements with template parameter pack
- Fixed parsing of array types as template parameters
- Fixed type deducers in array types
- Fixed calls to a function with a type deducer parameter incorrectly failing to match overload in some cases


<!---------------------------------->
<a name="v0.0.140.0"></a>
## v0.0.140.0

### Panther
- Added compiling of C and C++ headers
	- `typedef` and `using` alias
	- types `void`, primitives, pointers, lvalue/rvalue references
	- libc
- Added `@importC` and `@importCPP`
- Renamed "Platform" to "Target", (and "OS" to "Platform")
- Renamed "Typedef" to "Distinct Alias"

### Misc
- Added LibC 
	- [Huge thanks to Zig!](https://github.com/ziglang/zig/tree/master/lib/libc)


<!---------------------------------->
<a name="v0.0.139.0"></a>
## v0.0.139.0

### Panther
- Added indexers (`[]`)


<!---------------------------------->
<a name="v0.0.138.0"></a>
## v0.0.138.0

### Panther
- Added array types
- Added string template args
- Fixed lowering to PIR of string values
- Fixed string literals being given a type of array (corrected to a read-only pointer to an array)
- Fixed type deducers incorrectly failing to deduce types when they have fewer qualifiers than the got type
- Fixed diagnostic location for unknown string escape sequence being incorrect
- Fixed type id converter expressions

### PIR
- Added linkages
	- `weak`
	- `weak_external`
- Removed `LegacyJITEngine`
- Fixed lowering to LLVMIR of globals that have a value of another global

### PCIT_core
- Fixed `SingleThreadedWorkQueue` from attempting to remove a task after there was an error in a task and the queue was force-cleared


<!---------------------------------->
<a name="v0.0.137.0"></a>
## v0.0.137.0

### Panther
- Added `while` loops
- Added `break` statements
- Added `continue` statements
- Removed old compiler implementation
- Fixed missing printing of AST for error statements
- Fixed template function args being used as function parameter types incorrectly erroring as not existent

### Misc
- Fixed build warnings for unreachable code in LLVM


<!---------------------------------->
<a name="v0.0.136.0"></a>
## v0.0.136.0
- Updated build system to satisfy [llvm-project-build v0.2.0](https://github.com/PCIT-Project/llvm-project-build/tree/main/v0.2.0)


<!---------------------------------->
<a name="v0.0.135.0"></a>
## v0.0.135.0

### Panther
- Added build targets:
	- OBJECT
	- CONSOLE_EXECUTABLE
	- WINDOWED_EXECUTABLE

### PLNK
- Added Subsystem option to Windows target specific settings
- Fixed windows args potentially referencing freed memory
- Fixed options not setting target specific settings
- Fixed windows no correctly using output file path

### Misc
- Updated to [Panther-std v0.0.3.0](https://github.com/PCIT-Project/Panther-std/blob/main/CHANGELOG.md#v0.0.3.0)


<!---------------------------------->
<a name="v0.0.134.0"></a>
## v0.0.134.0

### Panther
- Added function templates
	- function template args
	- function parameters with a type of an interface
	- function parameters with a type of a type deducer
- Fixed shadowing redefinition check sometimes emitting an error diagnostic pointing to the wrong source location
- Fixed race condition in semantic analysis when calling when analyzing an accessor operator (`.`) on an interface pointer


<!---------------------------------->
<a name="v0.0.133.1"></a>
## v0.0.133.1

### Panther
- Fixed infix operators with arguments that have a type that is an alias
- Fixed lowering to PIR of parameters have a type that is an alias of an unsigned integral having the PIR attribute of `#signed`


<!---------------------------------->
<a name="v0.0.133.0"></a>
## v0.0.133.0

### Panther
- Added support for lowering in parameters 
- Fixed fatal when lowering a discarding function call that has named returns
- Fixed crashing of constexpr functions with integer parameters


<!---------------------------------->
<a name="v0.0.132.0"></a>
## v0.0.132.0

### Panther
- Added interfaces
- Improved names of PIR registers
- Fixed crashing when printing AST of struct initializer new
- Fixed functions that have the same type and have return parameters usually incorrectly registering as different types

### PIR
- Added printing of function pointer calls
- Added printing of function types
- Fixed function pointer exprs not being considered a constant or a value
- Fixed some asserts in `Module` sometimes incorrectly saying two types were not equivalent
- Fixed lowering to LLVMIR of global arrays of certain types
- Fixed lowering to LLVMIR not including names of call expressions

### Misc
- Updated README


<!---------------------------------->
<a name="v0.0.131.2"></a>
## v0.0.131.2

### Panther
- Added checking for fluid argument to operator `~`
- Improved diagnostic error message for fluid conversion requiring truncation
- Fixed crashing when lowering to PIR of where the LHS xor RHS of an operator `<<|` is fluid
- Fixed operator `as` not properly erroring when a fluid conversion would require truncation (which caused further issues)


<!---------------------------------->
<a name="v0.0.131.1"></a>
## v0.0.131.1

### Panther
- Fixed integral wrapping operators:
	- `+%`
	- `-%`
	- `*%`

### PIR
- Fixed lowering to LLVM-IR of `@umulSat` and `@smulSat`


<!---------------------------------->
<a name="v0.0.131.0"></a>
## v0.0.131.0

### Panther
- Added prefix operators:
	- `-`
	- `!`
	- `~`
- Fixed fatal error when doing semantic analysis of compile-time functions

### PIR
- Fixed assert incorrectly stating that `@ieq` can only supports integers (it can also supports `Bool`s)


<!---------------------------------->
<a name="v0.0.130.2"></a>
## v0.0.130.2

### Panther
- Fixed a race condition in semantic analysis

### PCIT_core
- Fixed bug in `ThreadQueue::isWorking`


<!---------------------------------->
<a name="v0.0.130.1"></a>
## v0.0.130.1

### Panther
- Fixed 2 race conditions in semantic analysis
- Made project C++23 compliant
	- Project still only requires C++20 compliant compiler
- Removed `lib` directory for being extraneous


<!---------------------------------->
<a name="v0.0.130.0"></a>
## v0.0.130.0

### Panther
- Added intrinsic `@getTypeID`
- Added Type ID converter
- Added better diagnostics for incorrect operator `as`
- Fixed diagnostic location pointing for Type ID converter
- Fixed checking for correct return value of the entry function not looking through aliases


<!---------------------------------->
<a name="v0.0.129.0"></a>
## v0.0.129.0

### Panther
- Added local `when` conditionals

 
<!---------------------------------->
<a name="v0.0.128.0"></a>
## v0.0.128.0

### PCIT_core
- Added conditionals
- Removed the required ';' at end of `defer` statement
- Fixed potential compilation error


<!---------------------------------->
<a name="v0.0.127.0"></a>
## v0.0.127.0

### PCIT_core
- Added support to `Optional` for non-trivial types
- Improved the interface for `Optional`
- Added custom wait thresholds to `SpinLock`


<!---------------------------------->
<a name="v0.0.126.1"></a>
## v0.0.126.1

### Panther
- Fixed printing of AST for `else if`


<!---------------------------------->
<a name="v0.0.126.0"></a>
## v0.0.126.0

### Panther
- Added operator `as` for fluid values
- Added constexpr operator `as`
- Changed name mangling for global variables to use the letter 'g' (for global) instead of the letter 'v'


<!---------------------------------->
<a name="v0.0.125.0"></a>
## v0.0.125.0

### Panther
- Added aliases declared in function scope
- Added functions declared in function scope
- Added structs declared in function scope
- Fixed debug check for multi-threading failing incorrectly


<!---------------------------------->
<a name="v0.0.124.0"></a>
## v0.0.124.0

### Panther
- Added infix operators
	- `==`, `!=`, `<`, `<=`, `>`, `>=`
	- `&`, `|`, `^`
	- `<<`, `<<|`, `>>`
	- `+`, `+%`, `+|`
	- `-`, `-%`, `-|`
	- `*`, `*%`, `*|`
	- `/`, `%`
- Fixed compiler crashing for constexpr `@div` when `IS_EXACT` is `true`
- Fixed constexpr operator `as`


<!---------------------------------->
<a name="v0.0.123.0"></a>
## v0.0.123.0

### Panther
- Improved printing of templated types in diagnostics to include template args
- Fixed `Char` template args causing weird behavior


<!---------------------------------->
<a name="v0.0.122.0"></a>
## v0.0.122.0

### Panther
- Added overloading of operator `as`
- Fixed assert (that checks all symbol procs finished incorrectly) firing in debug mode


<!---------------------------------->
<a name="v0.0.121.2"></a>
## v0.0.121.2

### Panther
- Fixed member variables of templated structs sometimes being considered to not exist


<!---------------------------------->
<a name="v0.0.121.1"></a>
## v0.0.121.1

### Panther
- Added checking for use of a non-instantiated templated type


<!---------------------------------->
<a name="v0.0.121.0"></a>
## v0.0.121.0

### Panther
- Added operator `as`
- Fixed `TypeManager::getUnderlyingType()` for `Int`, `ISize`, `UInt`, and `USize`


<!---------------------------------->
<a name="v0.0.120.0"></a>
## v0.0.120.0

### Panther
- Added warning option when making a method call to a function that is not a method
- Fixed method call to functions that are not methods


<!---------------------------------->
<a name="v0.0.119.0"></a>
## v0.0.119.0

### Panther
- Added struct methods

### Misc
- Updated to [Panther-std v0.0.2.0](https://github.com/PCIT-Project/Panther-std/blob/main/CHANGELOG.md#v0.0.2.0)


<!---------------------------------->
<a name="v0.0.118.0"></a>
## v0.0.118.0

### Panther
- Added auto-dereferencing accessor operator
- Combined `@fToI` and `@fToUI`
- Combined `@iToF` and `@uiToF`


<!---------------------------------->
<a name="v0.0.117.4"></a>
## v0.0.117.4

### Panther
- Fixed requiring the definition of the base if the type is a pointer
- Fixed diagnostic locations of `new` expressions pointing to the type instead of the keyword `new`


<!---------------------------------->
<a name="v0.0.117.3"></a>
## v0.0.117.3

### Panther
- Fixed constexpr struct initializer


<!---------------------------------->
<a name="v0.0.117.2"></a>
## v0.0.117.2

### Panther
- Fixed circular dependency detection error diagnostic not always giving correct information (declaration vs definition)


<!---------------------------------->
<a name="v0.0.117.1"></a>
## v0.0.117.1

### Panther
- Added more information to circular dependency detection error diagnostic
- Fixed circular dependency checking methods with the declarations containing the parent struct
- Fixed fatal error caused by trying to a struct with all members being zero-sized
- Fixed many issues with empty structs


<!---------------------------------->
<a name="v0.0.117.0"></a>
## v0.0.117.0

### Panther
- Added struct initializer new expressions
- Added constexpr accessor

### PIR
- Changed `@branch` to `@jump`
- Changed `@condBranch` to `@branch`


<!---------------------------------->
<a name="v0.0.116.1"></a>
## v0.0.116.1

### Panther
- Fixed various bugs by updating to [Evo v1.30.1](https://github.com/12Thanjo/Evo/blob/main/CHANGELOG.md#v1.30.1)


<!---------------------------------->
<a name="v0.0.116.0"></a>
## v0.0.116.0

### Panther
- Added struct member variables
- Added struct attributes `#ordered` and `#packed`
- Added variable attribute `#global`
- Fixed declaring local variables with initializer values
- Fixed parsing of function parameter `this` with a default kind
- Fixed race condition in semantic analysis


<!---------------------------------->
<a name="v0.0.115.0"></a>
## v0.0.115.0

### Panther
- Added prefix operators `&` and `&|`
- Added postfix operator `.*`
- Fixed type deducers when they have qualifiers


<!---------------------------------->
<a name="v0.0.114.0"></a>
## v0.0.114.0

### Panther
- Added arithmetic intrinsics:
	- `@add`, `@addWrap`, `@addSat`, `@fadd`
	- `@sub`, `@subWrap`, `@subSat`, `@fsub`
	- `@mul`, `@mulWrap`, `@mulSat`, `@fmul`
	- `@div`, `@fdiv`
	- `@rem`, `@fneg`
- Added comparison intrinsics:
	- `@eq`, `@neq`
	- `@lt`, `@lte`
	- `@gt`, `@gte`
- Added bitwise intrinsics:
	- `@and`, `@or`, `@xor`
	- `@shl`, `@shlSat`, `@shr`
	- `@bitReverse`
	- `@bSwap`
	- `@ctPop`
	- `@ctLZ`, `@ctTZ`
- Added type traits intrinsics:
	- `@bitWidth`
- Added constexpr execution type conversion intrinsics:
	- `@trunc`, `@ftrunc`
	- `@sext`, `@zext`, `@fext`
	- `@iToF`, `@uiToF`, `@fToI`, `@fToUI`

### PIR
- Added bitwise instructions:
	- `@bitReverse`
	- `@bSwap`
	- `@ctPop`
	- `@ctLZ`
	- `@ctTZ`


<!---------------------------------->
<a name="v0.0.113.0"></a>
## v0.0.113.0

### Panther
- Added `defer`/`errorDefer` statements
- Added `unreachable` statements
- Added block statements
- Added missing checks for scope being terminated
- Transitioned Panther standard library to using new standalone project ([Panther-std](https://github.com/PCIT-Project/Panther-std))
- Fixed fatal on certain invalid statements in symbol proc building


<!---------------------------------->
<a name="v0.0.112.0"></a>
## v0.0.112.0

### Panther
- Added local variables
- Added checking for constexpr block expressions


<!---------------------------------->
<a name="v0.0.111.0"></a>
## v0.0.111.0

### Panther
- Added checking for use of runtime identifiers in constexpr functions
- Added support for const variables in constexpr functions
- Changed attribute `#runtime` to `#rt`

### PIR
- Added external global variables


<!---------------------------------->
<a name="v0.0.110.0"></a>
## v0.0.110.0

### Panther
- Added except parameters to `try/else` expressions
- Added checking that except expression in `try/except` expression is ephemeral
- Fixed PIR of function error parameter not having attributes

### PIR
- Fixed `Agent::createMemcpy()` emitting incorrect number of bytes to copy


<!---------------------------------->
<a name="v0.0.109.0"></a>
## v0.0.109.0

### Panther
- Added block expressions
- Allowed `this` parameters to have default parameter kind (`read`)
- Changed syntax of block expressions to allow for returning multiple values
- Fixed race condition in semantic analysis

### PCIT_core
- Fixed race condition in `ThreadQueue::isWorking()`


<!---------------------------------->
<a name="v0.0.108.1"></a>
## v0.0.108.1

### Panther
- Added checking for attributes in the wrong place in parser
- Fixed fatal error when selecting function overload failed for an intrinsic function
- Fixed fatal error when encountering an error in semantic analysis of a call to a template intrinsic function


<!---------------------------------->
<a name="v0.0.108.0"></a>
## v0.0.108.0

### Panther
- Added `try/else` expressions
- Removed destructive move


<!---------------------------------->
<a name="v0.0.107.1"></a>
## v0.0.107.1

### Panther
- Fixed constexpr function call parameter being given the wrong value for any integer type width other than 256 (not including `Char` or `Bool`)
- Fixed fatal error when lowering constexpr/runtime function call to a function with named return value when in not an assignment


<!---------------------------------->
<a name="v0.0.107.0"></a>
## v0.0.107.0

### Panther
- Added support for constexpr function calls with named return values

### PIR
- Added parameter attributes:
	- `#unsigned`
	- `#signed`
	- `#ptrNoAlias`
	- `#ptrNonNull`
	- `#ptrDereferencable(n)`
	- `#ptrReadOnly`
	- `#ptrWriteOnly`
	- `#ptrWritable`
	- `#ptrRVO(type)`


<!---------------------------------->
<a name="v0.0.106.0"></a>
## v0.0.106.0

### Panther
- Added output target of printing LLVMIR


<!---------------------------------->
<a name="v0.0.105.0"></a>
## v0.0.105.0

### Panther
- Added assignment
- Added multi-assignment
- Added discarding assignment
- Optimized ABI to not include function parameters of types that are size 0
- Fixed lowering of calls to functions that have named return values
- Fixed multi-assigns silently failing if an assignment target was something else besides an identifier or a discard (`_`)


<!---------------------------------->
<a name="v0.0.104.0"></a>
## v0.0.104.0

### Panther
- Added template expression parameters type allowed to be a previous template type parameter
- Fixed when conditionals not being allowed at global scope


<!---------------------------------->
<a name="v0.0.103.0"></a>
## v0.0.103.0

### Panther
- Added parameter expressions
- Added support for calling functions as constexpr values with parameters
- Added checking that argument of `move` is mutable

### PIR
- Fixed asserts in `Agent::createCalcPtr` not allowing for non-aggregate types


<!---------------------------------->
<a name="v0.0.102.0"></a>
## v0.0.102.0

### Panther
- Added type conversion intrinsics
	- `@bitCast`
	- `@trunc`, `@ftrunc`
	- `@sext`, `@zext`, `@fext`
	- `@iToF`, `@uiToF`, `@fToI`, `@fToUI`
- Fixed `@sizeOf` not including padding for integers with a width larger than 64


<!---------------------------------->
<a name="v0.0.101.1"></a>
## v0.0.101.1

### Panther
- Changed syntax to create type alias to `type alias {IDENT} = {TYPE};` (from `alias {IDENT} = {TYPE};`)
- Fixed circular dependency checking not checking direct self dependency


<!---------------------------------->
<a name="v0.0.101.0"></a>
## v0.0.101.0

### Panther
- Added intrinsic function `@sizeOf`


<!---------------------------------->
<a name="v0.0.100.0"></a>
## v0.0.100.0

### Panther
- Added operator `copy`
- Added operator `move`


<!---------------------------------->
<a name="v0.0.99.0"></a>
## v0.0.99.0

### Panther
- Added type deducers


<!---------------------------------->
<a name="v0.0.98.1"></a>
## v0.0.98.1

### Panther
- Fixed default diagnostic callback location pointing sometimes getting spacing incorrect and sometimes crashing (OOB error)
- Fixed assert failing incorrectly in semantic analysis
- Fixed template parameters not being found within scope
- Fixed semantic analysis saying a symbol didn't exist if there was an error in that symbol

### PIR
- Fixed race condition in `JITEngine`


<!---------------------------------->
<a name="v0.0.98.0"></a>
## v0.0.98.0

### Panther
- Added build system
- Added function call statements
- Added generation and running of entry function via JIT
- Added Assembly output
- Removed leftover breakpoint when type mismatch was detected in semantic analysis

### Panther STD
- Added build sub-library

### PIR
- Added `@abort` instruction
- Fixed converting global variables of type `Bool` and `Ptr` with a value of `zeroinit` to LLVM


<!---------------------------------->
<a name="v0.0.97.0"></a>
## v0.0.97.0

### Panther
- Move constexpr-execution from `pir::Interpreter` to `pir::JITEngine`
- Changed the function ABI to have error return params be a packed struct instead of separate parameters
- Fixed race condition in `TypeManager`

### PIR
- Changed `FunctionDecl` to `ExternalFunction`
- Removed `Interpreter` in favor of the new `JITEngine`
	- Note: it will probably come back in some form in the long-term future

### Misc
- Changed from using `bool` as error return value to `evo::Result<>`


<!---------------------------------->
<a name="v0.0.96.0"></a>
## v0.0.96.0

### PIR
- Created new `JITEngine` (old one is currently present in `LegacyJITEngine`)
- Fixed `ModulePrinter` not printing out the base location of a `@calPtr` instruction
- Fixed `ModulePrinter` printing global value expression and function pointers with a `@` prefix instead of `&`
- Fixed some minor memory leaks 

### Misc
- Removed BUILDING.md and pointed the link to the builing instructions that's on the README to [https://www.pcitproject.org/site/build.html](https://www.pcitproject.org/site/build.html)
- Added a link on the README to the PIR documentation
- Fixed the link to the pcitproject.org home page on the README


<!---------------------------------->
<a name="v0.0.95.0"></a>
## v0.0.95.0

### Panther
- Fixed missing token string for destructive move

### PIR
- Fixed fatal errors when erasing expressions from basic blocks

### Misc
- Upgraded LLVM from 18.1.1 to 20.1.1
- Upgraded Premake build system from 5.0-beta2 to 5.0-beta5
- Improved / updated build instructions and script
- Changed enums to match Panther style


<!---------------------------------->
<a name="v0.0.94.0"></a>
## v0.0.94.0

### Panther
- Added constexpr execution of functions
- Added checks for proper value stages
- Fixed 2 race conditions in semantic analysis

### PIR
- Added `Interpreter`

### PCIT_core
- Added `StepVector`


<!---------------------------------->
<a name="v0.0.93.0"></a>
## v0.0.93.0

### Panther
- Added `error` statements
- Added better diagnostics when there is a function overload collision 
- Added checking for float literal ending in a `.`
- Added checking for implicitly-typed `var` and `const` variables having a value of a fulid literal
- Added checking for newlines and tabs in character literals
- Swapped definitions of language terms `comptime` and `constexpr`
- Improved PIR code generation for function parameters (removing unnecessary `@alloca`s)
- Slight tweaks to CONTRIBUTING.md
- Fixed race condition in semantic analysis where sometimes not all overloads of a function were considered
- Fixed code generation for functions with an unnamed return value and error return
- Fixed fatal error when generating code for function that has an unnamed error return param
- Fixed literal integer and literal float scientific notation for when the exponent is `1`
- Fixed assert going off incorrectly in lowering to PIR when using `def` variables


<!---------------------------------->
<a name="v0.0.92.0"></a>
## v0.0.92.0

### Panther
- Added `return` statements


<!---------------------------------->
<a name="v0.0.91.0"></a>
## v0.0.91.0

### Panther
- Added lowering to PIR
- Fixed semantic analysis incorrectly saying a fluid float needed to be truncated when the target type is `F128`
- Fixed `TypeManager::sizeOf()` for float types

### PIR
- Fixed type checking debug asserts in `Module::createGlobalArray`, `Module::createGlobalStruct`, and `Module::createGlobalVar`
- Fixed stmt name de-duplicator / static automatic name-er not taking `@alloca`s into account


<!---------------------------------->
<a name="v0.0.90.0"></a>
## v0.0.90.0

### Panther
- Added functions


<!---------------------------------->
<a name="v0.0.89.1"></a>
## v0.0.89.1

### Panther
- Fixed race condition in semantic analysis
- Fixed issues with multiple template instantiations of a single type


<!---------------------------------->
<a name="v0.0.89.0"></a>
## v0.0.89.0

### Panther
- Added struct templates
- Added checking for shadowing
- Fixed semantic analysis sometimes incorrectly requiring that a struct member have the `#pub` attribute to be used
- Fixed struct members within a when conditional being put into the wrong scope level
- Fixed fatal error when statement is two or more when-conditional deep and the when-conditional path for that symbol was not taken
- Fixed circular dependency diagnostics having the locations switched
- Fixed identifier lookup not looking in all required scope levels
- Slight improvement to printing the AST of templated types

### PIR
- Changed assembly output from AT&T to Intel

### PCIT_core
- Fixed race condition in `ThreadQueue::isWorking()`


<!---------------------------------->
<a name="v0.0.88.0"></a>
## v0.0.88.0

### Panther
- Added checking for empty template parameter blocks


<!---------------------------------->
<a name="v0.0.87.0"></a>
## v0.0.87.0

### Panther
- Added parsing of function parameter default values
- Added parsing of template parameter default values
- Added parsing of function error return parameter blocks


<!---------------------------------->
<a name="v0.0.86.0"></a>
## v0.0.86.0

### Panther
- Added struct namespaces
- Fixed fluid literals not implicitly converting when the expected type was an integral
- Fixed semantic analysis sometimes deadlocking
- Fixed 3 race conditions in semantic analysis
- Fixed type checking when the type of the value is an alias

### PCIT_core
- Fixed race condition in `ThreadQueue::isWorking()`


<!---------------------------------->
<a name="v0.0.85.0"></a>
## v0.0.85.0

### Panther
- Added semantic analysis of initializer values


<!---------------------------------->
<a name="v0.0.84.1"></a>
## v0.0.84.1

### Panther
- Changed alias to not allow being `Void`
- Fixed declaration and definition of alias not being properly separated
- Fixed not checking for / waiting for definition of symbol if declaration was already completed


<!---------------------------------->
<a name="v0.0.84.0"></a>
## v0.0.84.0

### Panther
- Added aliases
- Added checking that module variables are `def`
- Fixed `def variables` not being type checked
- Fixed implicit conversion of fluid integral checking for integral types > 256


<!---------------------------------->
<a name="v0.0.83.2"></a>
## v0.0.83.2

### Panther
- Made `def variables` not allow for declaration and definition to be separate
- Fixed race condition


<!---------------------------------->
<a name="v0.0.83.1"></a>
## v0.0.83.1

### Panther
- Fixed variable declarations not being added to sema buffer until definition


<!---------------------------------->
<a name="v0.0.83.0"></a>
## v0.0.83.0

### Panther
- Added semantic analysis of when conditionals


<!---------------------------------->
<a name="v0.0.82.0"></a>
## v0.0.82.0
Removed dependency analysis and added the symbol proc stage in its place. Doing this required re-doing semantic analysis.

### Panther
- Added semantic analysis of global variables
- Added support for `@import`


<!---------------------------------->
<a name="v0.0.81.0"></a>
## v0.0.81.0

### Panther
- Added circular dependency detection
- Fixed dependency analysis of struct members
- Fixed printing of dependencies that are a struct, alias, or typedef


<!---------------------------------->
<a name="v0.0.80.0"></a>
## v0.0.80.0

### Panther
- Added `move!` and `forward`
- Added semantic analysis of global variables

### misc
- Updated README with the new domain for the website ([www.pcitproject.org](https://www.pcitproject.org/home/site.html))


<!---------------------------------->
<a name="v0.0.79.0"></a>
## v0.0.79.0

### Panther
- Added dependency analysis
- Added framework of semantic analysis

### PCIT_core
- Fixed various issues with `ThreadQueue`
- Fixed race condition in `SyncLinearStepAlloc`

### misc
- Updated README to remove the word `compiled` from the Panther tagline
- Fixed pthr_old not compiling


<!---------------------------------->
<a name="v0.0.78.1"></a>
## v0.0.78.1
- Added custom anchors to CHANGELOG


<!---------------------------------->
<a name="v0.0.78.0"></a>
## v0.0.78.0
Renamed "Panther" to "Panther-old" and started a new Panther project. This is because adding the dependency analysis stage and supporting the build system (the differing in the way `@import` needs to work) requires a rework of the semantic analysis as well as the work management system in `Context`.

### Panther
- Added tokenization
- Added parsing
- Added standard library

### PCIT_core
- Added `ThreadQueue`
- Fixed issue with `ThreadPool` never registering as finished working when it is given more data than it has workers

### misc
- Updated README to have correct path to main of `pthr`


<!---------------------------------->
<a name="v0.0.77.0"></a>
## v0.0.77.0

### Panther
- Added parsing of struct declarations
- Changed sytax of labeled block
	-  from `{->IDENT /*...*/ }` to `{->(IDENT) /*...*/ }`
	-  from `{->IDENT<{TYPE}> /*...*/ }` to `{->(IDENT: TYPE) /*...*/ }`

### pthr
- Fixed printing of AST statement block when empty


<!---------------------------------->
<a name="v0.0.76.0"></a>
## v0.0.76.0

### Panther
- Added typedef declarations
- Added operator `new`
- Fixed fatal error when printing AST `zeroinit`
- Fixed implicit conversion of fluid values checking for `USize`, `ISize`, `Int`, `UInt`


<!---------------------------------->
<a name="v0.0.75.1"></a>
## v0.0.75.1

### Panther
- Updated comptime executor to use PIR
- Fixed generating linkage and calling convention functions when lowering to PIR for JITing


<!---------------------------------->
<a name="v0.0.75.0"></a>
## v0.0.75.0

### Panther
- Added `!` operator
- Added `-` (negate) operator
- Added `~` operator
- Added checking that implicit conversion of fluid values is valid (checking that no truncation required)
- Fixed `@trunc` converting to `Bool` (and by extension `as` operator from integers)
- Fixed `@ftoui` converting to `Bool` (and by extension `as` operator from floats)
- Fixed `CInt` and `CLong` being switched

### pthr
- Diagnostic messages now show filepaths as relative

### PIR
- Added function pointers
- Fixed printing of `@and`, `@or`, and `@xor`
- Fixed printing of global var expressions
- Replaced the `#mayWrap` attribute with `#nsw` (no signed wrap) and `#nuw` (no unsigned wrap)


<!---------------------------------->
<a name="v0.0.74.0"></a>
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
<a name="v0.0.73.0"></a>
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
<a name="v0.0.72.0"></a>
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
<a name="v0.0.71.0"></a>
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
<a name="v0.0.70.0"></a>
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
<a name="v0.0.69.0"></a>
## v0.0.69.0

### PIR
- Added the `@fAdd` instruction
	- can no longer use the `@add` instruction for floats
- Condensed Signed and Unsigned into Integer
- Split `@addWrap` into `@sAddWrap` and `@uAddWrap`


<!---------------------------------->
<a name="v0.0.68.0"></a>
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
<a name="v0.0.67.0"></a>
## v0.0.67.0

### PIR
- Added `@breakpoint` instruction
- Added `@unreachable` instruction
- Added `@condBranch` instruction


<!---------------------------------->
<a name="v0.0.66.0"></a>
## v0.0.66.0

### PIR
- Added `@calcPtr` instruction
- Fixed fatal error when compiling an array type into LLVM-IR


<!---------------------------------->
<a name="v0.0.65.0"></a>
## v0.0.65.0

### plnk
- Added the plnk project. General project layout has been created
- Added basic linking for Windows


<!---------------------------------->
<a name="v0.0.64.0"></a>
## v0.0.64.0

### PIR
- Added lowering to assembly
- Added lowering to object file
- Added LLVM default optimization passes


<!---------------------------------->
<a name="v0.0.63.0"></a>
## v0.0.63.0

### PIR
- Added struct and array values for global vars


<!---------------------------------->
<a name="v0.0.62.1"></a>
## v0.0.62.1

### Misc
- Updated the README to point to the [PCIT website](pcit-project.github.io).


<!---------------------------------->
<a name="v0.0.62.0"></a>
## v0.0.62.0

### PIR
- Added string values for global vars
- Added alignment to global variables in LLVM-IR output
- Fixed some size and alignment calculations


<!---------------------------------->
<a name="v0.0.61.1"></a>
## v0.0.61.1

### PIR
- Fixed memory corruption issue


<!---------------------------------->
<a name="v0.0.61.0"></a>
## v0.0.61.0

### PIR
- Added `@load` and `@store` instructions
- Adjusted positioning of statement attributes
- Fixed statement automatic naming for `@addWrap` when result and wrapped names are the same not being unique


<!---------------------------------->
<a name="v0.0.60.1"></a>
## v0.0.60.1

### PIR
- Fixed various bugs and fatal errors around stmt passes and reverse stmt passes in `PassManager`
- Fixed fatal error around erasing `Expr`s


<!---------------------------------->
<a name="v0.0.60.0"></a>
## v0.0.60.0

### PIR
- Added the ability for `Agent` to insert at any index within the basic block
- Added optimization to convert `@addWrap` to `@add` if wrapped value isn't used
- Added various debug asserts
- Fixed fatal error when adding statements during a statement pass
- Fixed various compiler warnings


<!---------------------------------->
<a name="v0.0.59.0"></a>
## v0.0.59.0

### PIR
- Added `@addWrap` instruction


<!---------------------------------->
<a name="v0.0.58.0"></a>
## v0.0.58.0

### PIR
- Added `JITEngine`
- Added setting of OS and Architecture
- Fixed fatal error when a function doesn't have any allocas


<!---------------------------------->
<a name="v0.0.57.0"></a>
## v0.0.57.0

### PIR
- Added lowering to LLVMIR
- Changed `@br` to `@branch`


<!---------------------------------->
<a name="v0.0.56.0"></a>
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
<a name="v0.0.55.0"></a>
## v0.0.55.0
- Updated License to Apache License v2.0 with LLVM and PCIT exceptions
- Fixed LLVM license missing


<!---------------------------------->
<a name="v0.0.54.1"></a>
## v0.0.54.1

### PIR
- Made most functions of `Agent` const
- Modified API of `PassManager` passes to `const Agent&` instead of `Agent&`
- Removed `;`s from printing
- Fixed printing of `@add` instruction
- Fixed potential issues when multi-threading `PassManager`


<!---------------------------------->
<a name="v0.0.54.0"></a>
## v0.0.54.0

### PIR
- Added `Agent`
- Added `ReaderAgent`
- Modified API of `PassManager` passes to use `Agent`s
- Moved expr creation and reading from `Module` and `Function` to `Agent` and `ReaderAgent`
- Replacing instructions now properly deletes the replaced instruction


<!---------------------------------->
<a name="v0.0.53.0"></a>
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
<a name="v0.0.52.0"></a>
## v0.0.52.0

### PIR
- Added the PIR project. General project layout has been created

### PCIT_core
- Added `StepAlloc`
- Added the ability to iterate over `LinearStepAlloc`
- Added `Printer` support printing to `std::string` as an option
- Added format arg versions of `Printer::print()` and `Printer::println()`


<!---------------------------------->
<a name="v0.0.51.0"></a>
## v0.0.51.0

### Panther
- Added `while` loops
- Fixed fatal error when printing AST of conditional (`if` and `when`)
- Fixed `as` expressions sometimes being considered as not ephemeral


<!---------------------------------->
<a name="v0.0.50.1"></a>
## v0.0.50.1

### Panther
- Fixed comptime arithmetic operators not always being precomputed correctly
- Fixed comptime logical operators not always being precomputed correctly


<!---------------------------------->
<a name="v0.0.50.0"></a>
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
<a name="v0.0.49.0"></a>
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
<a name="v0.0.48.0"></a>
## v0.0.48.0

### Panther
- Added support for arithmetic operators
- Added support for fluid `def` variables
	- `def` variables that have the value of a fluid literal and use type-inference act as fluid


<!---------------------------------->
<a name="v0.0.47.0"></a>
## v0.0.47.0

### Panther
- Added support for all arithmetic intrinsics to be run at compile-time
- Added support for checked arithmetic in compile-time executed functions
- Fixed fatal error when using `as` operator with type `BF16`


<!---------------------------------->
<a name="v0.0.46.0"></a>
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
<a name="v0.0.45.0"></a>
## v0.0.45.0

### Panther
- Added `as` operator
- Improved error recovery of conditionals
- Fixed underlying type of `RawPtr` being `UI64` (should be `RawPtr`)
- Fixed `@bitCast` between integrals and pointers


<!---------------------------------->
<a name="v0.0.44.0"></a>
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
<a name="v0.0.43.0"></a>
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

### Misc
- Updated README to move Panther example to new [PCIT website](pcit-project.github.io)


<!---------------------------------->
<a name="v0.0.42.0"></a>
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
<a name="v0.0.41.2"></a>
## v0.0.41.2

### Panther
- Added checking for circular dependencies in comptime
- Added support for comptime functions that return `Bool`, `Char`, `F32`, and `F64`
- Fixed printing of type of literal integers and literal floats


<!---------------------------------->
<a name="v0.0.41.1"></a>
## v0.0.41.1

### Panther
- Fixed fatal error when calling a templated function as a comptime value
- Fixed fatal error when calling functions in comptime not declared before usage


<!---------------------------------->
<a name="v0.0.41.0"></a>
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
<a name="v0.0.40.1"></a>
## v0.0.40.1

### Panther
- Fixed `def` variables defined in local scope being emitted


<!---------------------------------->
<a name="v0.0.40.0"></a>
## v0.0.40.0

### Panther
- Fixed fatal error with usage of templated functions
- Improved performance around memory allocations / reallocations
- Fixed `TypeManager` not deallocating memory for function types

### PCIT_core
- Added `LinearStepAlloc`


<!---------------------------------->
<a name="v0.0.39.0"></a>
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

### Misc
- Slight tweaks to README


<!---------------------------------->
<a name="v0.0.38.0"></a>
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
<a name="v0.0.37.2"></a>
## v0.0.37.2

### Panther
- Added checking for initializer values being used as function call expression arguments
- Fixed diagnostic being emitted that the intrinsic `@import` was unknown if the call was a statement


<!---------------------------------->
<a name="v0.0.37.1"></a>
## v0.0.37.1

### Panther
- Added intrinsic function `@_printHelloWorld`

### pthr
- Added nicer logo when compiling in verbose mode


<!---------------------------------->
<a name="v0.0.37.0"></a>
## v0.0.37.0

### Panther
- Added support for intrinsic functions
- Added intrinsic function `@breakpoint`
- Added intrinsic function `@sizeOf`


<!---------------------------------->
<a name="v0.0.36.1"></a>
## v0.0.36.1

### Panther
- Fixed fatal error when calling a function pointer that as a statement


<!---------------------------------->
<a name="v0.0.36.0"></a>
## v0.0.36.0

### Panther
- Added `if` conditionals
- Added `unreachable` statements
- Added checking that a function that doesn't return `Void` is terminated
- Fixed checking of termination in scoped statement blocks
- Fixed breakpoint being thrown when attempting to dereference a value that's not a pointer


<!---------------------------------->
<a name="v0.0.35.0"></a>
## v0.0.35.0

### Panther
- Added `when` conditionals (compile time conditionals - doesn't open a new scope)
- Added parsing of `if` conditionals

### Misc
- Improved README


<!---------------------------------->
<a name="v0.0.34.0"></a>
## v0.0.34.0

### Panther
- Added local scope statement blocks
- Added explicit typing of block expressions


<!---------------------------------->
<a name="v0.0.33.0"></a>
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
<a name="v0.0.32.0"></a>
## v0.0.32.0

### Panther
- Added function pointers
	- Note: there is no way to write a function pointer type yet
- Fixed fatal error when copying or moving a function
- Made various improvements to some diagnostic messages


<!---------------------------------->
<a name="v0.0.31.1"></a>
## v0.0.31.1

### Panther
- Fixed function overloads not respecting differences in parameter kinds
- Fixed fatal error when taking the address of an expression with multiple values


<!---------------------------------->
<a name="v0.0.31.0"></a>
## v0.0.31.0

### Panther
- Added function overloading
- Fixed diagnostic source locations from failing to match function overload
- Fixed fatal error when returning a value in a function that has return type `Void`


<!---------------------------------->
<a name="v0.0.30.1"></a>
## v0.0.30.1

### Panther
- Fixed fatal error messages caused by setting a single value with multiple values (multiple return param function)


<!---------------------------------->
<a name="v0.0.30.0"></a>
## v0.0.30.0

### Panther
- Added multiple function return parameters
- Added checking that entry function does not have named return parameter
- Fixed discarding of a return value of a function with a named return parameter


<!---------------------------------->
<a name="v0.0.29.0"></a>
## v0.0.29.0

### Panther
- Added support for initializer value expression `uninint`
- Added initializer value expression `zeroinit`
- Temporarily removed specific diagnostic codes for semantic analysis


<!---------------------------------->
<a name="v0.0.28.0"></a>
## v0.0.28.0

### Panther
- Added support for named return values
- Added support for `move` expressions
- Improved the API of `ScopeManager::ScopeLevel` to make it nicer and increase performance
- Improved the wordings of some error diagnostics
- Fixed fluid literals not correctly being classified as ephemeral when passed into an `in` parameter


<!---------------------------------->
<a name="v0.0.27.0"></a>
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
<a name="v0.0.26.0"></a>
## v0.0.26.0

### Panther
- Added support for prefix `&` operator (address-of)
- Added support for postfix `.*` operator (dereference)
- Added checking for invalid type qualifiers (read-only pointer above a mutable pointer)
- Fixed `def` variables not always working as ephemeral values
- Fixed tokenization issues with caused by interactions with multiple `&` or `|`


<!---------------------------------->
<a name="v0.0.25.0"></a>
## v0.0.25.0

### Panther
- Added function call expressions
- Added checking for discarding of function return values
- Added support for the discard statement
- Fixed LLVM IR not having readable register names


<!---------------------------------->
<a name="v0.0.24.0"></a>
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
<a name="v0.0.23.0"></a>
## v0.0.23.0

### Panther
- Added support for return statements
- Added tokenizing and parsing of return statements for multiple returns


<!---------------------------------->
<a name="v0.0.22.0"></a>
## v0.0.22.0

### Panther
- Added support for variables to be used as consteval values
- Added support for assignment expressions
- Added support for `copy` expressions
- Fixed fatal error when encountering an invalid global statement
- Fatal diagnostics now always emit whether hit max errors or not. Additionally, no more errors should be emitted after it
- Fixed diagnostic "Cannot get a consteval value from a variable that isn't def" not giving the correct source location


<!---------------------------------->
<a name="v0.0.21.0"></a>
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
<a name="v0.0.20.0"></a>
## v0.0.20.0

### Panther
- Added templated function with value parameters


<!---------------------------------->
<a name="v0.0.19.0"></a>
## v0.0.19.0

### Panther
- Added templated function with `Type` parameters
- Fixed issue where no statements were analyzed after a function call (in semantic analysis)
- Fixed local variables declared before a locally-declared function were accepted as part of the scope
- Fixed identifiers `I`, `U`, and `UI` causing fatal errors tokenizer

### pthr
- Added build target `LLVMIR`


<!---------------------------------->
<a name="v0.0.18.0"></a>
## v0.0.18.0

### Panther
- Added function call statements
- Added variable expressions

### PCIT_core
- Fixed `Optional` sometimes not selecting a constructor as expected


<!---------------------------------->
<a name="v0.0.17.0"></a>
## v0.0.17.0

### Panther
- Added variable declarations
- Added the concept of fluid literals
	- literal ints can become any integral type
	- literal floats can become any float type
- Removed primitive-type `F80`


<!---------------------------------->
<a name="v0.0.16.0"></a>
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
<a name="v0.0.15.0"></a>
## v0.0.15.0

### Panther
- Added support for declaration of functions inside functions
- Added more support for the `mayRecover` config option

### PCIT_core
- Added `UniqueID` and `UniqueComparableID`


<!---------------------------------->
<a name="v0.0.14.2"></a>
## v0.0.14.2

### Panther
- Added detection of valid statements within functions
- Added `mayRecover` option to `Context::Context`
	- if `true`, can continue semantic analysis on that file if encountered a recoverable error (won't exceed `maxErrors`)


<!---------------------------------->
<a name="v0.0.14.1"></a>
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
<a name="v0.0.14.0"></a>
## v0.0.14.0

### Panther
- Added checking of supported statements in semantic analysis
- Added checking of function re-definition
- Added the BF16 type ("brain" floating point - 7-bit significand)
- Fixed fatal errors caused by diagnostics with infos
- Fixed end of file checking for tokenization of operators and identifiers
- Fixed unstable behavior with function parameters in the AST
- Changed indentation level of diagnostic infos in the default diagnostic callback


<!---------------------------------->
<a name="v0.0.13.0"></a>
## v0.0.13.0

### Panther
- Added basic semantic analysis (checking of valid global statements)

### PCIT_core
- Added fatal diagnostics with source locations (`DiagnosticImpl::createFatalMessage`)


<!---------------------------------->
<a name="v0.0.12.1"></a>
## v0.0.12.1

### Panther
- Fixed string and character literal tokens sometimes pointing to garbage/invalid strings


<!---------------------------------->
<a name="v0.0.12.0"></a>
## v0.0.12.0

### Panther
- Added `def` variable declaration
- Added `alias` declarations


<!---------------------------------->
<a name="v0.0.11.0"></a>
## v0.0.11.0

### Panther
- Separated Tokens and Token locations (more data-oriented)
- Fixed string and character literal tokens pointing to garbage/invalid strings
- Added checking if a source location is too big (max lines / column number is 2^32-1)


<!---------------------------------->
<a name="v0.0.10.3"></a>
## v0.0.10.3

### Panther
- Added checking for valid characters


<!---------------------------------->
<a name="v0.0.10.2"></a>
## v0.0.10.2

### Panther
- Removed type expressions (except after an `as` operator)
- Improved diagnostic for cases like `foo as SomeOptionalAlias.?`
- Improved diagnostic for cases like `foo as Int?.?`
- Improved the performance of the Parser
- Fixed typo in Parser diagnostic "Expected value after [=] in variable declaration, ..."


<!---------------------------------->
<a name="v0.0.10.1"></a>
## v0.0.10.1
- Fixed various compile errors caused by update to MSVC
- Minor tweaks to coding style


<!---------------------------------->
<a name="v0.0.10.0"></a>
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
<a name="v0.0.9.1"></a>
## v0.0.9.1

### Panther
- Removed unnamed (`___`)
	- It may come back at some point, but it's being considered if it would lead to bad practices


<!---------------------------------->
<a name="v0.0.9.0"></a>
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
<a name="v0.0.8.0"></a>
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
<a name="v0.0.7.0"></a>
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
<a name="v0.0.6.0"></a>
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
<a name="v0.0.5.0"></a>
## v0.0.5.0

### Panther
- Added basic parsing of function declarations
- Made `AST::Node` and `AST::NodeOptional` trivially copyable

### pthr
- Added `max_num_errors` to config

### Misc
- Fixed links in [contributing policy](https://github.com/PCIT-Project/PCIT-CPP/blob/main/CONTRIBUTING.md) and [security policy](https://github.com/PCIT-Project/PCIT-CPP/blob/main/SECURITY.md) to point to the correct headings


<!---------------------------------->
<a name="v0.0.4.0"></a>
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
<a name="v0.0.3.0"></a>
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
<a name="v0.0.2.0"></a>
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
<a name="v0.0.1.0"></a>
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
<a name="v0.0.0.0"></a>
## v0.0.0.0
- Initial commit
- Setup build system for Evo, Panther, and pthr