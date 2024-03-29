#pragma once

#include "kernel.h"
#include "io.h"
#include "process.h"
#include "files.h"
#include <Windows.h>
#include <string>

HMODULE User_Programs;


void run_shell(const kiv_hal::TRegisters &regs);

void Initialize_Kernel() {
	User_Programs = LoadLibraryW(L"user.dll");
	Init_Filesystems();
}

void Shutdown_Kernel() {
	FreeLibrary(User_Programs);
}

void __stdcall Sys_Call(kiv_hal::TRegisters &regs) {

	switch (static_cast<kiv_os::NOS_Service_Major>(regs.rax.h)) {
	
		case kiv_os::NOS_Service_Major::File_System:
			Handle_IO(regs);
			break;

	    case kiv_os::NOS_Service_Major::Process:
	        Handle_Process(regs, User_Programs);
	        break;

	}

}

void __stdcall Bootstrap_Loader(kiv_hal::TRegisters &context) {
	Initialize_Kernel();
	kiv_hal::Set_Interrupt_Handler(kiv_os::System_Int_Number, Sys_Call);

	//v ramci ukazky jeste vypiseme dostupne disky
	kiv_hal::TRegisters regs{};


	//spustime shell - v realnem OS bychom ovsem spousteli login
    run_shell(regs);

	Shutdown_Kernel();
}

void run_shell(const kiv_hal::TRegisters &regs) {
    kiv_hal::TRegisters shell_regs{};

    // open a VGA as the STDOUT for the shell
    std::string tty = "\\sys\\tty";
    kiv_os::NOS_Error error = kiv_os::NOS_Error::Success;
    auto std_out = Open_File(tty.c_str(), kiv_os::NOpen_File::fmOpen_Always, 0, error);
    if (error != kiv_os::NOS_Error::Success) {
        return;
    }

    // open keyboard as the STDIN for shell
    std::string kb = "\\sys\\kb";
    auto std_in = Open_File(kb.c_str(), kiv_os::NOpen_File::fmOpen_Always, 0, error);
    if (error != kiv_os::NOS_Error::Success) {
        return;
    }

    // "shell" program should be run
    shell_regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>("shell");

    // with no arguments
    shell_regs.rdi.r = reinterpret_cast<decltype(regs.rdx.r)>("");

    // save std_in and std_out to rbx
    shell_regs.rbx.e = (std_in << 16) | std_out;

    // create a new process
    shell_regs.rcx.l = static_cast<uint8_t>(kiv_os::NClone::Create_Process);

    // perform clone
    clone(shell_regs, User_Programs);

    // prepare read exit code regs
    kiv_hal::TRegisters read_exit_code_regs{};
    read_exit_code_regs.rdx.x = static_cast<decltype(regs.rdx.x)>((kiv_os::THandle)shell_regs.rax.x);

    // perform Wait_For
    read_exit_code(read_exit_code_regs);

    kiv_hal::TRegisters close_regs{};
    // close shell's std_out
    close_regs.rdx.x = std_out;
    Close_File(close_regs);

    // close shell's std_in
    close_regs.rdx.x = std_in;
    Close_File(close_regs);
}

void Set_Error(const bool failed, kiv_hal::TRegisters &regs) {
	if (failed) {
		regs.flags.carry = true;
		regs.rax.r = GetLastError();
	}
	else
		regs.flags.carry = false;
}