# Change Log

<!---------------------------------->
# v0.0.4.0

### Panther
- Added parsing
	- variable declarations
	- `Void` and `Int` types
	- Literal expressions
	- Identifier expressions

### pthr
- Added `PrintAST` build target

### Misc
- Added [contributing policy](https://github.com/PCIT-Project/PCIT-CPP/blob/main/CONTRIBUTING.md)), [code of conduct](https://github.com/PCIT-Project/PCIT-CPP/blob/main/CODE_OF_CONDUCT.md), and [security policy](https://github.com/PCIT-Project/PCIT-CPP/blob/main/SECURITY.md)).

<!---------------------------------->
## v0.0.3.0
Added Tokenization and improved diagnostics

### pthr
- Added build targets to config

### Panther
- Added `Token` and `TokenBuffer`
- Added tokenization
- Fixed fatal error when multiple worker threads reported an error at the same time
- Added pointing to source code in diagnostics

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