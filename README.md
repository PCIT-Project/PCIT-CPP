<p align="center">
	<a href="https://www.pcitproject.org">
		<img src="https://www.pcitproject.org/assets/LogoBig.png" alt="PCIT Project Logo">
	</a>
</p>


# PCIT-CPP

PCIT (pronounced "P-Kit") is the Panther Compiler Infrastructure and Toolchain, and the home of the Panther programming language. Check out the PCIT Project website ([www.pcitproject.org](https://www.pcitproject.org)) for more information.

> [!NOTE]
> This is in the very early stages and as such is not usable yet. If you really want to try it now (with little documentation), you can run the `pthr` executable (once compiled) and modify the files found in `testing`.

## Tools
- [Panther](https://www.pcitproject.org/site/Panther.html): Statically-typed, high-performance, systems programming language
- [PIR](https://www.pcitproject.org/site/documentation/pir/documentation.html): Pronounced "P I R". Compiler IR and SSA-based optimizing back-end.
- PLNK: Pronounced "plink". Linker that aims to make use between platforms (including cross-compilation and linking against libc) as seamless as possible.


## Building:
The build instructions can be found on the [Building PCIT Project Software](https://www.pcitproject.org/site/build.html) page.


## Updates:
List of changes for each version can be found [here](CHANGELOG.md). Note: very small changes may not be listed.


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
