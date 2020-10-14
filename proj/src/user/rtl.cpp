#include "rtl.h"

std::atomic<kiv_os::NOS_Error> kiv_os_rtl::Last_Error;

kiv_hal::TRegisters Prepare_SysCall_Context(kiv_os::NOS_Service_Major major, uint8_t minor) {
	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<uint8_t>(major);
	regs.rax.l = minor;
	return regs;
}


bool kiv_os_rtl::Read_File(const kiv_os::THandle file_handle, char* const buffer, const size_t buffer_size, size_t &read) {
	kiv_hal::TRegisters regs =  Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Read_File));
	regs.rdx.x = static_cast<decltype(regs.rdx.x)>(file_handle);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
	regs.rcx.r = buffer_size;

	const bool result = kiv_os::Sys_Call(regs);
	read = regs.rax.r;
	return result;
}

bool kiv_os_rtl::Write_File(const kiv_os::THandle file_handle, const char *buffer, const size_t buffer_size, size_t &written) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Write_File));
	regs.rdx.x = static_cast<decltype(regs.rdx.x)>(file_handle);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
	regs.rcx.r = buffer_size;

	const bool result = kiv_os::Sys_Call(regs);
	written = regs.rax.r;
	return result;
}

bool kiv_os_rtl::Clone(const kiv_os::TThread_Proc thread_proc, char* data, kiv_os::THandle &t_handle) {
    // TODO Clone should take into consideration Create_Process and Create_Thread

    // get syscall context (registers) - specifying we want the Process service and Clone task
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Clone));

    // set TThreadProc (thread/process entry point)
    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(thread_proc);

    // set thread/process input data
    regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(data);

    // regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
    // regs.rcx.r = buffer_size;

    // do syscall
    const bool result = kiv_os::Sys_Call(regs);

    // load the THandle from the rax register
    t_handle = regs.rax.x;

    return result;
}

bool kiv_os_rtl::Wait_For(const kiv_os::THandle handles[], uint8_t handle_count, uint8_t &handleThatSignalledIndex) {
    // TODO Clone should take into consideration Create_Process and Create_Thread
    // Wait_For, //IN : rdx pointer na pole THandle, na ktere se ma cekat, rcx je pocet handlu
            //funkce se vraci jakmile je signalizovan prvni handle
            //OUT : rax je index handle, ktery byl signalizovan


    // get syscall context (registers) - specifying we want the Process service and Clone task
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Wait_For));

    // save pointer to the THandle thread to registers
    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(handles);

    // store the count of handles to registers
    regs.rcx.l = handle_count;

    // do syscall
    const bool result = kiv_os::Sys_Call(regs);

    handleThatSignalledIndex = regs.rax.l;

    return result;
}
