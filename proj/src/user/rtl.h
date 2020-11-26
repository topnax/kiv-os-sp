#pragma once

#include "..\api\api.h"
#include <atomic>

namespace kiv_os_rtl {

	extern std::atomic<kiv_os::NOS_Error> Last_Error;

	bool Read_File(const kiv_os::THandle file_handle, char* const buffer, const size_t buffer_size, size_t &read);
		//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti buffer_size a vrati pocet zapsanych dat ve written
		//vraci true, kdyz vse OK
		//vraci true, kdyz vse OK

	bool Write_File(const kiv_os::THandle file_handle, const char *buffer, const size_t buffer_size, size_t &written);
	//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti buffer_size a vrati pocet zapsanych dat ve written
	//vraci true, kdyz vse OK
	//vraci true, kdyz vse OK


	bool Clone_Thread(kiv_os::TThread_Proc const thread_proc, char* data, kiv_os::THandle &t_handle);

    bool Clone_Process(const char* program, const char* data, kiv_os::THandle std_in, kiv_os::THandle std_out, kiv_os::THandle &t_handle);

	bool Wait_For(const kiv_os::THandle handles[], uint8_t handle_count, uint8_t &handle_that_has_signalled_index);

	bool Read_Exit_Code(const kiv_os::THandle handle, kiv_os::NOS_Error &exit_code);

	bool Exit(kiv_os::NOS_Error exit_code);

    bool Create_Pipe(const kiv_os::THandle *handles);

    bool Open_File(const char *file_name, uint8_t flags, uint8_t attributes, kiv_os::THandle &handle);

    bool Close_Handle(const kiv_os::THandle handle);

    bool Register_Signal_Handler(const kiv_os::NSignal_Id signal, kiv_os::TThread_Proc const thread_proc);

    bool Shutdown();
}