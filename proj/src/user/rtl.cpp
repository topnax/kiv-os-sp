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

	read = static_cast<size_t>(regs.rax.r);
	return result;
}

bool kiv_os_rtl::Write_File(const kiv_os::THandle file_handle, const char *buffer, const size_t buffer_size, size_t &written) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Write_File));
	regs.rdx.x = static_cast<decltype(regs.rdx.x)>(file_handle);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
	regs.rcx.r = buffer_size;

	const bool result = kiv_os::Sys_Call(regs);
	written = static_cast<size_t>(regs.rax.r);

    return result;
}

bool kiv_os_rtl::Clone_Thread(const kiv_os::TThread_Proc thread_proc, char* data, kiv_os::THandle &t_handle) {
    // get syscall context (registers) - specifying we want the Process service and Clone task
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Clone));

    // set TThreadProc (thread/process entry point)
    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(thread_proc);

    // set thread/process input data
    regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(data);

    regs.rcx.l = static_cast<uint8_t>(kiv_os::NClone::Create_Thread);

    // regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
    // regs.rcx.r = buffer_size;

    // do syscall
    const bool result = kiv_os::Sys_Call(regs);

    // load the THandle from the rax register
    t_handle = regs.rax.x;

    return result;
}

bool kiv_os_rtl::Clone_Process(const char* program, const char* data, kiv_os::THandle std_in, kiv_os::THandle std_out, kiv_os::THandle &t_handle) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Clone));

    // store the program to be run in rdx
    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(program);

    // store the program arguments in rdi
    regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(data);

    // store stdin and stdout both in rbx
    regs.rbx.e = (std_in << 16) | std_out;

    regs.rcx.l = static_cast<uint8_t>(kiv_os::NClone::Create_Process);

    const bool result = kiv_os::Sys_Call(regs);

    // rax now contains the handle of the created thread
    t_handle = regs.rax.x;

    return result;
}

bool kiv_os_rtl::Wait_For(const kiv_os::THandle handles[], uint8_t handle_count, uint8_t &handle_that_has_signalled_index) {
    // get syscall context (registers) - specifying we want the Process service and Clone task
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Wait_For));

    // save pointer to the THandle thread to registers
    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(handles);

    // store the count of handles to registers
    regs.rcx.l = handle_count;

    // do syscall
    const bool result = kiv_os::Sys_Call(regs);

    // rax now contains the index of the handle that signalled the semaphore
    handle_that_has_signalled_index = regs.rax.l;

    return result;
}

bool kiv_os_rtl::Read_Exit_Code(const kiv_os::THandle handle, kiv_os::NOS_Error &exit_code) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Read_Exit_Code));

    regs.rdx.x = static_cast<decltype(regs.rdx.x)>(handle);

    const bool result = kiv_os::Sys_Call(regs);

    exit_code = static_cast<kiv_os::NOS_Error>(regs.rcx.x);

    return result;
}

bool kiv_os_rtl::Exit(kiv_os::NOS_Error exit_code) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Exit));

    regs.rcx.x = static_cast<decltype(regs.rcx.x)>(exit_code);

    return kiv_os::Sys_Call(regs);
}


bool kiv_os_rtl::Create_Pipe(const kiv_os::THandle *handles) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Create_Pipe));

    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(handles);

    return kiv_os::Sys_Call(regs);
}


bool kiv_os_rtl::Close_Handle(const kiv_os::THandle handle) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Close_Handle));

    regs.rdx.x = static_cast<decltype(regs.rdx.x)>(handle);

    const bool result = kiv_os::Sys_Call(regs);

    return result;
}

bool kiv_os_rtl::Register_Signal_Handler(const kiv_os::NSignal_Id signal, kiv_os::TThread_Proc const thread_proc) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Register_Signal_Handler));

    regs.rcx.l = static_cast<decltype(regs.rcx.l)>(signal);
    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(thread_proc);

    const bool result = kiv_os::Sys_Call(regs);

    return result;
}

bool kiv_os_rtl::Shutdown() {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Shutdown));

    const bool result = kiv_os::Sys_Call(regs);

    return result;
}

bool kiv_os_rtl::Open_File(const char *file_name, uint8_t flags, uint8_t attributes, kiv_os::THandle &handle) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Open_File));
    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(file_name);
    regs.rcx.l = flags;
    regs.rdi.i = attributes;

    const bool result = kiv_os::Sys_Call(regs);

    handle = regs.rax.x;

    return handle != kiv_os::Invalid_Handle ;
}

bool kiv_os_rtl::Set_Working_Dir(const char *path) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Set_Working_Dir));
    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(path);

    const bool result = kiv_os::Sys_Call(regs);

    return result;
}

bool kiv_os_rtl::Get_Working_Dir(char *buffer, size_t buffer_size, size_t &read) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Get_Working_Dir));

    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(buffer);
    regs.rcx.r = buffer_size;

    const bool result = kiv_os::Sys_Call(regs);

    return result;
}

bool kiv_os_rtl::Seek(const kiv_os::THandle handle, size_t position, const kiv_os::NFile_Seek pos_type, const kiv_os::NFile_Seek op, size_t &pos_from_start) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Seek));

    regs.rdx.x = handle;
    regs.rdi.r = position;
    regs.rcx.l = static_cast<decltype(regs.rcx.l)>(pos_type);
    regs.rcx.h = static_cast<decltype(regs.rcx.h)>(op);

    const bool result = kiv_os::Sys_Call(regs);

    pos_from_start = regs.rax.r;

    return result;
}

bool kiv_os_rtl::Delete_File(const char *file_name) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Delete_File));
    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(file_name);

    const bool result = kiv_os::Sys_Call(regs);

    return result;
}

bool kiv_os_rtl::Set_File_Attributes(const char *file_name, uint8_t attributes) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Set_File_Attribute));
    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(file_name);
    regs.rdi.i = static_cast<uint16_t>(attributes);

    const bool result = kiv_os::Sys_Call(regs);

    return result;
}

bool kiv_os_rtl::Get_File_Attributes(const char *file_name, uint8_t &attributes) {
    kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Get_File_Attribute));
    regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(file_name);

    const bool result = kiv_os::Sys_Call(regs);

    attributes = regs.rdi.i;

    return result;
}
