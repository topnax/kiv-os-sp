#pragma once

#include "..\api\api.h"

void Handle_Process(kiv_hal::TRegisters &regs);

void clone(kiv_hal::TRegisters &registers);

void wait_for(kiv_hal::TRegisters &registers);
