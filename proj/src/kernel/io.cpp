#include <map>
#include "io.h"
#include "kernel.h"
#include "handles.h"
#include "pipe.h"
#include "pipes.h"
#include "files.h"
#include <set>


void Open_File(kiv_hal::TRegisters &registers);

void Handle_IO(kiv_hal::TRegisters &regs) {

	//V ostre verzi pochopitelne do switche dejte volani funkci a ne primo vykonny kod


	switch (static_cast<kiv_os::NOS_File_System>(regs.rax.l)) {

		case kiv_os::NOS_File_System::Read_File: {
            Read_File(regs);
            break;
		}

		case kiv_os::NOS_File_System::Write_File: {
            Write_File(regs);
            break;
		}

        case kiv_os::NOS_File_System::Create_Pipe: {
            // create a pipe
            auto generated_handle_pair = Create_Pipe();

            // assign pipe in/out to the handle array
            auto *handle_array = reinterpret_cast<kiv_os::THandle *>(regs.rdx.r);
            handle_array[0] = generated_handle_pair.first;
            handle_array[1] = generated_handle_pair.second;

            break;
        }

        case kiv_os::NOS_File_System::Close_Handle: {
            Close_File(regs);
            break;
        }

        case kiv_os::NOS_File_System::Open_File: {
            Open_File(regs);
            break;
        }

	/* Nasledujici dve vetve jsou ukazka, ze starsiho zadani, ktere ukazuji, jak mate mapovat Windows HANDLE na kiv_os handle a zpet, vcetne jejich alokace a uvolneni

		case kiv_os::scCreate_File: {
			HANDLE result = CreateFileA((char*)regs.rdx.r, GENERIC_READ | GENERIC_WRITE, (DWORD)regs.rcx.r, 0, OPEN_EXISTING, 0, 0);
			//zde je treba podle Rxc doresit shared_read, shared_write, OPEN_EXISING, etc. podle potreby
			regs.flags.carry = result == INVALID_HANDLE_VALUE;
			if (!regs.flags.carry) regs.rax.x = Convert_Native_Handle(result);
			else regs.rax.r = GetLastError();
		}
									break;	//scCreateFile

		case kiv_os::scClose_Handle: {
				HANDLE hnd = Resolve_kiv_os_Handle(regs.rdx.x);
				regs.flags.carry = !CloseHandle(hnd);
				if (!regs.flags.carry) Remove_Handle(regs.rdx.x);
					else regs.rax.r = GetLastError();
			}

			break;	//CloseFile

	*/
	}
}

void Open_File(kiv_hal::TRegisters &registers) {
    char *file_name = reinterpret_cast<char * >(registers.rdx.r);
    uint8_t flags = registers.rcx.l;
    uint8_t attributes = registers.rdi.i;
    registers.rax.x = Open_File(file_name, flags, attributes);
}
