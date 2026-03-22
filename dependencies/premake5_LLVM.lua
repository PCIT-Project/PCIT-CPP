

function llvm_link(lib_name)
	filter "configurations:Debug"
		links{
			(config.location .. "/dependencies/LLVM_build/lib-debug/" .. lib_name)
		}
	filter {}

	filter "configurations:Optimize or Release or ReleaseDist"
		links{
			(config.location .. "/dependencies/LLVM_build/lib-release/" .. lib_name)
		}
	filter {}
end



------------------------------------------------------------------------------
-- LLVM


function llvm_link_AArch64()
	llvm_link("LLVMAArch64AsmParser")
	llvm_link("LLVMAArch64CodeGen")
	llvm_link("LLVMAArch64Desc")
	llvm_link("LLVMAArch64Disassembler")
	llvm_link("LLVMAArch64Info")
	llvm_link("LLVMAArch64Utils")
end

function llvm_link_RISCV()
	llvm_link("LLVMRISCVAsmParser")
	llvm_link("LLVMRISCVCodeGen")
	llvm_link("LLVMRISCVDesc")
	llvm_link("LLVMRISCVDisassembler")
	llvm_link("LLVMRISCVInfo")
	llvm_link("LLVMRISCVTargetMCA")
end

function llvm_link_WebAssembly()
	llvm_link("LLVMWebAssemblyAsmParser")
	llvm_link("LLVMWebAssemblyCodeGen")
	llvm_link("LLVMWebAssemblyDesc")
	llvm_link("LLVMWebAssemblyDisassembler")
	llvm_link("LLVMWebAssemblyInfo")
	llvm_link("LLVMWebAssemblyUtils")
end

function llvm_link_X86()
	llvm_link("LLVMX86AsmParser")
	llvm_link("LLVMX86CodeGen")
	llvm_link("LLVMX86Desc")
	llvm_link("LLVMX86Disassembler")
	llvm_link("LLVMX86Info")
	llvm_link("LLVMX86TargetMCA")
end



function llvm_link_all_platforms()
	llvm_link_AArch64()
	llvm_link_RISCV()
	llvm_link_WebAssembly()
	llvm_link_X86()
end
