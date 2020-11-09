#include <map>
#include "io.h"
#include "kernel.h"
#include "handles.h"
#include "pipe.h"
#include "pipes.h"
#include "files.h"
#include <set>


size_t Read_Line_From_Console(char *buffer, const size_t buffer_size) {
	kiv_hal::TRegisters registers;

	size_t pos = 0;
	while (pos < buffer_size) {
		//read char
		registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NKeyboard::Read_Char);
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, registers);

		if (!registers.flags.non_zero) break;	//nic jsme neprecetli,
												//pokud je rax.l EOT, pak byl zrejme vstup korektne ukoncen
												//jinak zrejme doslo k chybe zarizeni

		char ch = registers.rax.l;

		//osetrime zname kody
		switch (static_cast<kiv_hal::NControl_Codes>(ch)) {
			case kiv_hal::NControl_Codes::BS: {
					//mazeme znak z bufferu
					if (pos > 0) pos--;

					registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_Control_Char);
					registers.rdx.l = ch;
					kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
				}
				break;

			case kiv_hal::NControl_Codes::LF:  break;	//jenom pohltime, ale necteme
			case kiv_hal::NControl_Codes::NUL:			//chyba cteni?
			case kiv_hal::NControl_Codes::CR:  return pos;	//docetli jsme az po Enter


			default: buffer[pos] = ch;
					 pos++;
					 registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_String);
					 registers.rdx.r = reinterpret_cast<decltype(registers.rdx.r)>(&ch);
					 registers.rcx.r = 1;
					 kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
					 break;
		}
	}

	return pos;

}

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