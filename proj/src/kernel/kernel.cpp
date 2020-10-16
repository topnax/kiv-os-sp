#pragma once

#include "kernel.h"
#include "io.h"
#include "process.h"
#include <Windows.h>

HMODULE User_Programs;


void run_shell(const kiv_hal::TRegisters &regs);

void Initialize_Kernel() {
	User_Programs = LoadLibraryW(L"user.dll");
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
				Handle_IO(regs);
			};

			const char dec_2_hex[16] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };
			char hexa[3];
			hexa[0] = dec_2_hex[regs.rdx.l >> 4];
			hexa[1] = dec_2_hex[regs.rdx.l & 0xf];
			hexa[2] = 0;

			print_str("Nalezen disk: 0x");
			print_str(hexa);
			print_str("\n");

		}

        if (regs.rdx.l == 255) break;
    }

	//spustime shell - v realnem OS bychom ovsem spousteli login
    run_shell(regs);

	Shutdown_Kernel();
}

void run_shell(const kiv_hal::TRegisters &regs) {
    kiv_hal::TRegisters shell_regs;

    const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
    const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

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

    // created THandle is stored in rax
    kiv_os::THandle handles[] = {(kiv_os::THandle) shell_regs.rax.x};

    kiv_hal::TRegisters wait_for_regs;

    // wait for this array of handles
    wait_for_regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(handles);

    // store the count of handles to registers
    wait_for_regs.rcx.l = (uint8_t) 1;

    // perform Wait_For
    wait_for(wait_for_regs);
}

void Set_Error(const bool failed, kiv_hal::TRegisters &regs) {
	if (failed) {
		regs.flags.carry = true;
		regs.rax.r = GetLastError();
	}
	else
		regs.flags.carry = false;
}