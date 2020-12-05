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
	kiv_hal::TRegisters regs;
	for (regs.rdx.l = 0; ; regs.rdx.l++) {
		kiv_hal::TDrive_Parameters params;		
		regs.rax.h = static_cast<uint8_t>(kiv_hal::NDisk_IO::Drive_Parameters);;
		regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&params);
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
			
		if (!regs.flags.carry) {
			auto print_str = [](const char* str) {
    			kiv_hal::TRegisters regs;
				regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_File_System::Write_File);
				regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(str);
				regs.rcx.r = strlen(str);
                return;
				Handle_IO(regs);
			};

			const char dec_2_hex[16] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };
			char hexa[3];
			hexa[0] = dec_2_hex[regs.rdx.l >> 4];
			hexa[1] = dec_2_hex[regs.rdx.l & 0xf];
			hexa[2] = 0;

			// VGA not initialised at this moment
			// print_str("Nalezen disk: 0x");
			// print_str(hexa);
			// print_str("\n");

		}

        if (regs.rdx.l == 255) break;
    }

	//spustime shell - v realnem OS bychom ovsem spousteli login
    run_shell(regs);

	Shutdown_Kernel();
}

void run_shell(const kiv_hal::TRegisters &regs) {
    kiv_hal::TRegisters shell_regs{};

    // open a VGA as the STDOUT for the shell
    std::string tty = "\\sys\\tty";
    auto std_out = Open_File(tty.c_str(), kiv_os::NOpen_File::fmOpen_Always, 0);

    // open keyboard as the STDIN for shell
    std::string kb = "\\sys\\kb";
    auto std_in = Open_File(kb.c_str(), kiv_os::NOpen_File::fmOpen_Always, 0);

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