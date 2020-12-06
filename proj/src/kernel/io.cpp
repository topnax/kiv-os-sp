#include "io.h"
#include "pipes.h"
#include "files.h"
#include "dir.h"


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

        case kiv_os::NOS_File_System::Set_Working_Dir: {
            bool result = Set_Working_Dir(regs);
            if (!result) regs.flags.carry = 1;
            break;
        }

        case kiv_os::NOS_File_System::Get_Working_Dir: {
            bool result = Get_Working_Dir(regs);
            if (!result) regs.flags.carry = 1;
            break;
        }

        case kiv_os::NOS_File_System::Seek: {
            Seek(regs);
            break;
        }

        case kiv_os::NOS_File_System::Delete_File: {
            Delete_File(regs);
            break;
        }

        case kiv_os::NOS_File_System::Set_File_Attribute: {
            Set_File_Attributes(regs);
            break;
        }

        case kiv_os::NOS_File_System::Get_File_Attribute: {
            Get_File_Attributes(regs);
            break;
        }

	}
}
