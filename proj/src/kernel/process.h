#pragma once

#include "..\api\api.h"

#include "Windows.h"


void thread_post_execute();

void Handle_Process(kiv_hal::TRegisters &regs, HMODULE user_programs);

void clone(kiv_hal::TRegisters &registers, HMODULE user_programs);

void wait_for(kiv_hal::TRegisters &registers);
