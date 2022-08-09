#pragma once

#include "sh2core.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int BackupHandled(SH2_struct* sh, u32 addr);

u8 FASTCALL SH2MappedMemoryReadByte(SH2_struct* context, u32 addr);
u16 FASTCALL SH2MappedMemoryReadWord(SH2_struct* context, u32 addr);
u32 FASTCALL SH2MappedMemoryReadLong(SH2_struct* context, u32 addr);
void FASTCALL SH2MappedMemoryWriteByte(SH2_struct* context, u32 addr, u8 val);
void FASTCALL SH2MappedMemoryWriteWord(SH2_struct* context, u32 addr, u16 val);
void FASTCALL SH2MappedMemoryWriteLong(SH2_struct* context, u32 addr, u32 val);

#ifdef __cplusplus
}
#endif
