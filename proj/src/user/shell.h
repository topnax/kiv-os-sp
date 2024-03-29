#pragma once

#include "..\api\api.h"
#include "rtl.h"
#include <set>
#include <string>

extern "C" size_t __stdcall shell(const kiv_hal::TRegisters &regs);


//nasledujici funkce si dejte do vlastnich souboru
//cd nemuze byt externi program, ale vestavny prikaz shellu!
extern "C" size_t __stdcall type(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall md(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall rd(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall dir(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall echo(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall find(const kiv_hal::TRegisters & regs);
extern "C" size_t __stdcall sort(const kiv_hal::TRegisters & regs);
extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall freq(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall tasklist(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall shutdown(const kiv_hal::TRegisters & regs);
void fill_supported_commands_set(std::set<std::string>&);
