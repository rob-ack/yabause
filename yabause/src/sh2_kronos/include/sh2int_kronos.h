#ifndef SH2INT_KRONOS_H
#define SH2INT_KRONOS_H

#include "sh2int.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SH2CORE_KRONOS_INTERPRETER 8

extern SH2Interface_struct SH2KronosInterpreter;

extern fetchfunc krfetchlist[0x1000];
typedef void (FASTCALL* opcode_func)(SH2_struct*const);
extern opcode_func opcodeTable[0x10000];

#ifdef __cplusplus
}
#endif

#endif
