#pragma once

#include "..\api\api.h"

#include "Windows.h"


void thread_post_execute(bool is_process);

void Handle_Process(kiv_hal::TRegisters &regs, HMODULE user_programs);

void clone(kiv_hal::TRegisters &registers, HMODULE user_programs);

void wait_for(kiv_hal::TRegisters &registers);

void exit(kiv_hal::TRegisters &registers);

void exit(kiv_os::THandle handle, kiv_os::NOS_Error exit_code);

void read_exit_code(kiv_hal::TRegisters &registers);
