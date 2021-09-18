/*  Copyright 2005 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

/*! \file memory.c
    \brief Memory access functions.
*/

#ifdef PSP  // see FIXME in T1MemoryInit()
# include <stdint.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>

#include "memory.h"
#include "cs0.h"
#include "cs1.h"
#include "cs2.h"
#include "debug.h"
#include "error.h"
#include "sh2core.h"
#include "scsp.h"
#include "scu.h"
#include "smpc.h"
#include "vdp1.h"
#include "vdp2.h"
#include "yabause.h"
#include "yui.h"
#include "movie.h"
#include "bios.h"
#include "peripheral.h"

#include "osdcore.h"
#include "vidogl.h"
#include "videoInterface.h"

//////////////////////////////////////////////////////////////////////////////

u8** MemoryBuffer[0x1000];

writebytefunc WriteByteList[0x1000];
writewordfunc WriteWordList[0x1000];
writelongfunc WriteLongList[0x1000];

readbytefunc ReadByteList[0x1000];
readwordfunc ReadWordList[0x1000];
readlongfunc ReadLongList[0x1000];

readbytefunc CacheReadByteList[0x1000];
readwordfunc CacheReadWordList[0x1000];
readlongfunc CacheReadLongList[0x1000];
writebytefunc CacheWriteByteList[0x1000];
writewordfunc CacheWriteWordList[0x1000];
writelongfunc CacheWriteLongList[0x1000];

#define EXTENDED_BACKUP_SIZE 0x00800000
#define EXTENDED_BACKUP_ADDR 0x08000000

u32 backup_file_addr = EXTENDED_BACKUP_ADDR;
u32 backup_file_size = EXTENDED_BACKUP_SIZE;
static const char *bupfilename = NULL;

u8 *HighWram;
u8 *LowWram;
u8 *BiosRom;
u8 *BupRam;

u8* VoidMem = NULL;

/* This flag is set to 1 on every write to backup RAM.  Ports can freely
 * check or clear this flag to determine when backup RAM has been written,
 * e.g. for implementing autosave of backup RAM. */
u8 BupRamWritten;

#if defined(_WINDOWS)

#include <windows.h>
HANDLE hFMWrite = INVALID_HANDLE_VALUE;
HANDLE hFile = INVALID_HANDLE_VALUE;
void * YabMemMap(char * filename, u32 size ) {

  struct stat sb;
  off_t len;
  char *p;
  int fd;

  hFile = CreateFileA(
    filename,
    GENERIC_READ|GENERIC_WRITE,
    0,
    0,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    0);
  if (INVALID_HANDLE_VALUE == hFile) {
    DWORD errorMessageID = GetLastError();
    LPSTR messageBuffer = NULL;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    LOG(messageBuffer);
    return NULL;
  }

  hFMWrite = CreateFileMapping(
    hFile,
    NULL,
    PAGE_READWRITE,
    0,
    size,
    L"BACKUP");
  if (hFMWrite == INVALID_HANDLE_VALUE)
    return NULL;

  return MapViewOfFile(hFMWrite, FILE_MAP_ALL_ACCESS, 0, 0, size);
}

void YabFreeMap(void * p) {
  UnmapViewOfFile(p);
  CloseHandle(hFMWrite);
  CloseHandle(hFile);
  hFMWrite = NULL;
}

#elif defined(__GNUC__)

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

static u32 mmapsize = 0;

void * YabMemMap(char * filename, u32 size ) {

  struct stat sb;
  off_t len;
  char *p;
  int fd;

  fd = open(filename, O_RDONLY);
  if (fd ==-1) {
    perror("open");
    return NULL;
   }

  if (fstat(fd, &sb) ==-1) {
    perror("fstat");
    return NULL;
  }

  if (!S_ISREG(sb.st_mode)) {
    fprintf(stderr, "%s is not a file\n", filename);
    return NULL;
  }

  p = mmap(0, sb.st_size, PROT_READ| PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED) {
    perror("mmap");
    return NULL;
  }

  if (close(fd) == -1) {
    perror("close");
    return NULL;
  }

  mmapsize = sb.st_size;
  return p;
}

void YabFreeMap(void * p) {
  munmap(p, mmapsize);
}


#else
void * YabMemMap(char * filename) {
  return NULL;
}
#endif


//////////////////////////////////////////////////////////////////////////////

u8 * T1MemoryInit(u32 size)
{
#ifdef PSP  // FIXME: could be ported to all arches, but requires stdint.h
            //        for uintptr_t
   u8 * base;
   u8 * mem;

   if ((base = calloc((size * sizeof(u8)) + sizeof(u8 *) + 64, 1)) == NULL)
      return NULL;

   mem = base + sizeof(u8 *);
   mem = mem + (64 - ((uintptr_t) mem & 63));
   *(u8 **)(mem - sizeof(u8 *)) = base; // Save base pointer below memory block

   return mem;
#else
   return (u8*) calloc(size, sizeof(u8));
#endif
}

//////////////////////////////////////////////////////////////////////////////

void T1MemoryDeInit(u8 * mem)
{
#ifdef PSP
   if (mem)
      free(*(u8 **)(mem - sizeof(u8 *)));
#else
   free(mem);
#endif
}

//////////////////////////////////////////////////////////////////////////////

T3Memory * T3MemoryInit(u32 size)
{
   T3Memory * mem;

   if ((mem = (T3Memory *) calloc(1, sizeof(T3Memory))) == NULL)
      return NULL;

   if ((mem->base_mem = (u8 *) calloc(size, sizeof(u8))) == NULL)
   {
      free(mem);
      return NULL;
   }

   mem->mem = mem->base_mem + size;

   return mem;
}

//////////////////////////////////////////////////////////////////////////////

void T3MemoryDeInit(T3Memory * mem)
{
   free(mem->base_mem);
   free(mem);
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL UnhandledMemoryReadByte(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
   LOG("Unhandled byte read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL UnhandledMemoryReadWord(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
   LOG("Unhandled word read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL UnhandledMemoryReadLong(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
   LOG("Unhandled long read %08X\n", (unsigned int)addr);
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteByte(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u8 val)
{
   LOG("Unhandled byte write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteWord(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u16 val)
{
   LOG("Unhandled word write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteLong(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u32 val)
{
   LOG("Unhandled long write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL HighWramMemoryReadByte(SH2_struct *context, u8* mem, u32 addr)
{
  if (context != NULL){
    context->cycles +=4; //Done
  }
   return T2ReadByte(mem, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL HighWramMemoryReadWord(SH2_struct *context, u8* mem, u32 addr)
{
  if (context != NULL){
    context->cycles +=4; //Done
  }
   return T2ReadWord(mem, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL HighWramMemoryReadLong(SH2_struct *context, u8* mem, u32 addr)
{
  if (context != NULL){
    context->cycles +=4; //Done
  }
   return T2ReadLong(mem, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL HighWramMemoryWriteByte(SH2_struct *context, u8* mem, u32 addr, u8 val)
{
  if (context != NULL){
    context->cycles +=1; //Done
  }
   T2WriteByte(mem, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL HighWramMemoryWriteWord(SH2_struct *context, u8* mem, u32 addr, u16 val)
{
  if (context != NULL){
    context->cycles += 1; //Done
  }
   T2WriteWord(mem, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL HighWramMemoryWriteLong(SH2_struct *context, u8* mem, u32 addr, u32 val)
{
  if (context != NULL){
    context->cycles += 1; //Done
  }
   T2WriteLong(mem, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL LowWramMemoryReadByte(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
  if (context != NULL){
    context->cycles += 4; //Done
  }
   return T2ReadByte(memory, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL LowWramMemoryReadWord(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
  if (context != NULL){
    context->cycles += 4; //Done
  }
   return T2ReadWord(memory, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL LowWramMemoryReadLong(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
  if (context != NULL){
    context->cycles += 12; //Done
  }
   return T2ReadLong(memory, addr & 0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL LowWramMemoryWriteByte(SH2_struct *context, UNUSED u8* memory, u32 addr, u8 val)
{
  if (context != NULL){
    context->cycles += 2; //Done
  }
   T2WriteByte(memory, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL LowWramMemoryWriteWord(SH2_struct *context, UNUSED u8* memory, u32 addr, u16 val)
{
  if (context != NULL){
    context->cycles += 2; //Done
  }
   T2WriteWord(memory, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL LowWramMemoryWriteLong(SH2_struct *context, UNUSED u8* memory, u32 addr, u32 val)
{
  if (context != NULL){
    context->cycles += 9; //Done
  }
   T2WriteLong(memory, addr & 0xFFFFF, val);
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL BiosRomMemoryReadByte(SH2_struct *context, UNUSED u8* memory,  u32 addr)
{
  // if (context != NULL){
  //   context->cycles += 16;
  // }
   return T2ReadByte(memory, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL BiosRomMemoryReadWord(SH2_struct *context, u8* memory, u32 addr)
{
  // if (context != NULL){
  //   context->cycles += 16;
  // }
   return T2ReadWord(memory, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL BiosRomMemoryReadLong(SH2_struct *context, u8* memory, u32 addr)
{
  // if (context != NULL){
  //   context->cycles += 16;
  // }
   return T2ReadLong(memory, addr & 0x7FFFF);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BiosRomMemoryWriteByte(SH2_struct *context, UNUSED u8* memory,UNUSED u32 addr, UNUSED u8 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BiosRomMemoryWriteWord(SH2_struct *context, UNUSED u8* memory,UNUSED u32 addr, UNUSED u16 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BiosRomMemoryWriteLong(SH2_struct *context, UNUSED u8* memory,UNUSED u32 addr, UNUSED u32 val)
{
   // read-only
}

//////////////////////////////////////////////////////////////////////////////

static u8 FASTCALL BupRamMemoryReadByte(SH2_struct *context, UNUSED u8* memory, u32 addr)
{
#if 0
  // maped memory is better
  if (BupRam == NULL) {
    if ((addr&0x0FFFFFFF) >= tweak_backup_file_addr) {
      addr = (addr & 0x0FFFFFFF) - tweak_backup_file_addr;
      if (addr >= tweak_backup_file_size) {
        return 0;
      }
    }
    else {
      addr = (addr & 0xFFFF);
    }
    fseek(pbackup, addr, 0);
    return fgetc(pbackup);
  }
#endif
  // if (context != NULL){
  //   context->cycles += 16;
  // }
  addr = addr & ((backup_file_size<<1) - 1);
  if (addr & 0x1) {
    return T1ReadByte(memory, addr>>1);
  }
  return 0xFF;
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL BupRamMemoryReadWord(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
   // LOG("bup\t: BackupRam read word - %08X\n", addr);
   // if (context != NULL){
   //   context->cycles += 16;
   // }
   return (BupRamMemoryReadByte(context, memory, addr | 0x1) << 8);
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL BupRamMemoryReadLong(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
   // LOG("bup\t: BackupRam read long - %08X\n", addr);
   // if (context != NULL){
   //   context->cycles += 16;
   // }
   return ((BupRamMemoryReadByte(context, memory, addr | 0x1) << 8) || (BupRamMemoryReadByte(context, memory, addr | 0x3) << 16));
}

//////////////////////////////////////////////////////////////////////////////
static void FASTCALL BupRamMemoryWriteByte(SH2_struct *context, UNUSED u8* memory, u32 addr, u8 val)
{
#if 0
  // maped memory is better
  if (BupRam == NULL) {
    if ((addr & 0x0FFFFFFF) >= tweak_backup_file_addr) {
      addr = ((addr & 0x0FFFFFFF) - tweak_backup_file_addr)|0x01;
    }else {
      addr = (addr & 0xFFFF) | 0x1;
    }
    fseek(pbackup, addr, 0);
    fputc(val, pbackup);
    fflush(pbackup);
  }
  else {
    T1WriteByte(BupRam, (addr & 0xFFFF) | 0x1, val);
    BupRamWritten = 1;
  }
#endif
  addr = addr & ((backup_file_size<<1) - 1);
  if (addr & 0x1) {
    T1WriteByte(memory, addr>>1, val);
  }
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BupRamMemoryWriteWord(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u16 val)
{
   // LOG("bup\t: BackupRam write word - %08X %x\n", addr, val);
   BupRamMemoryWriteByte(context, memory, addr | 0x1, (val>>8) & 0xFF);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL BupRamMemoryWriteLong(SH2_struct *context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u32 val)
{
   // LOG("bup\t: BackupRam write long - %08X %x\n", addr, val);
   BupRamMemoryWriteByte(context, memory, addr | 0x1, (val>>8) & 0xFF);
   BupRamMemoryWriteByte(context, memory, addr | 0x3, (val>>24) & 0xFF);
}

//////////////////////////////////////////////////////////////////////////////

static void FillMemoryArea(unsigned short start, unsigned short end,
                           readbytefunc r8func, readwordfunc r16func,
                           readlongfunc r32func, writebytefunc w8func,
                           writewordfunc w16func, writelongfunc w32func, u8** memory)
{
   int i;

   for (i=start; i < (end+1); i++)
   {
      MemoryBuffer[i] = memory;
      ReadByteList[i] = r8func;
      ReadWordList[i] = r16func;
      ReadLongList[i] = r32func;
      WriteByteList[i] = w8func;
      WriteWordList[i] = w16func;
      WriteLongList[i] = w32func;

      CacheReadByteList[i] = r8func;
      CacheReadWordList[i] = r16func;
      CacheReadLongList[i] = r32func;
      CacheWriteByteList[i] = w8func;
      CacheWriteWordList[i] = w16func;
      CacheWriteLongList[i] = w32func;
   }
}

//////////////////////////////////////////////////////////////////////////////

void MappedMemoryInit()
{
   // Initialize everyting to unhandled to begin with
   FillMemoryArea(0x000, 0xFFF, &UnhandledMemoryReadByte,
                                &UnhandledMemoryReadWord,
                                &UnhandledMemoryReadLong,
                                &UnhandledMemoryWriteByte,
                                &UnhandledMemoryWriteWord,
                                &UnhandledMemoryWriteLong,
                                &VoidMem);

   // Fill the rest
   FillMemoryArea(0x000, 0x00F, &BiosRomMemoryReadByte,
                                &BiosRomMemoryReadWord,
                                &BiosRomMemoryReadLong,
                                &BiosRomMemoryWriteByte,
                                &BiosRomMemoryWriteWord,
                                &BiosRomMemoryWriteLong,
                                &BiosRom);
   FillMemoryArea(0x010, 0x017, &SmpcReadByte,
                                &SmpcReadWord,
                                &SmpcReadLong,
                                &SmpcWriteByte,
                                &SmpcWriteWord,
                                &SmpcWriteLong,
                                &VoidMem);
   FillMemoryArea(0x020, 0x02F, &LowWramMemoryReadByte,
                                &LowWramMemoryReadWord,
                                &LowWramMemoryReadLong,
                                &LowWramMemoryWriteByte,
                                &LowWramMemoryWriteWord,
                                &LowWramMemoryWriteLong,
                                &LowWram);
   FillMemoryArea(0x040, 0x041, &IOPortReadByte,
                                &IOPortReadWord,
                                &UnhandledMemoryReadLong,
                                &IOPortWriteByte,
                                &UnhandledMemoryWriteWord,
                                &UnhandledMemoryWriteLong,
                                &VoidMem);
   FillMemoryArea(0x100, 0x17F, &UnhandledMemoryReadByte,
                                &UnhandledMemoryReadWord,
                                &UnhandledMemoryReadLong,
                                &UnhandledMemoryWriteByte,
                                &SSH2InputCaptureWriteWord,
                                &UnhandledMemoryWriteLong,
                                &VoidMem);
   FillMemoryArea(0x180, 0x1FF, &UnhandledMemoryReadByte,
                                &UnhandledMemoryReadWord,
                                &UnhandledMemoryReadLong,
                                &UnhandledMemoryWriteByte,
                                &MSH2InputCaptureWriteWord,
                                &UnhandledMemoryWriteLong,
                                &VoidMem);
   FillMemoryArea(0x200, 0x3FF, CartridgeArea->Cs0ReadByte,
                                CartridgeArea->Cs0ReadWord,
                                CartridgeArea->Cs0ReadLong,
                                CartridgeArea->Cs0WriteByte,
                                CartridgeArea->Cs0WriteWord,
                                CartridgeArea->Cs0WriteLong,
                                &CartridgeArea->rom); //A voir ici
   FillMemoryArea(0x400, 0x4FF, &Cs1ReadByte,
                                &Cs1ReadWord,
                                &Cs1ReadLong,
                                &Cs1WriteByte,
                                &Cs1WriteWord,
                                &Cs1WriteLong,
                                &VoidMem); //A voir ici
   FillMemoryArea(0x580, 0x58F, &Cs2ReadByte,
                                &Cs2ReadWord,
                                &Cs2ReadLong,
                                &Cs2WriteByte,
                                &Cs2WriteWord,
                                &Cs2WriteLong,
                                &VoidMem); //A voir ici
   FillMemoryArea(0x5A0, 0x5AF, &SoundRamReadByte,
                                &SoundRamReadWord,
                                &SoundRamReadLong,
                                &SoundRamWriteByte,
                                &SoundRamWriteWord,
                                &SoundRamWriteLong,
                                &SoundRam);
   FillMemoryArea(0x5B0, 0x5BF, &scsp_r_b,
                                &scsp_r_w,
                                &scsp_r_d,
                                &scsp_w_b,
                                &scsp_w_w,
                                &scsp_w_d,
                                &VoidMem);
   FillMemoryArea(0x5C0, 0x5C7, &Vdp1RamReadByte,
                                &Vdp1RamReadWord,
                                &Vdp1RamReadLong,
                                &Vdp1RamWriteByte,
                                &Vdp1RamWriteWord,
                                &Vdp1RamWriteLong,
                                &Vdp1Ram);
   FillMemoryArea(0x5C8, 0x5CF, &Vdp1FrameBufferReadByte,
                                &Vdp1FrameBufferReadWord,
                                &Vdp1FrameBufferReadLong,
                                &Vdp1FrameBufferWriteByte,
                                &Vdp1FrameBufferWriteWord,
                                &Vdp1FrameBufferWriteLong,
                                &VoidMem);
   FillMemoryArea(0x5D0, 0x5D7, &Vdp1ReadByte,
                                &Vdp1ReadWord,
                                &Vdp1ReadLong,
                                &Vdp1WriteByte,
                                &Vdp1WriteWord,
                                &Vdp1WriteLong,
                                &VoidMem);
   FillMemoryArea(0x5E0, 0x5EF, &Vdp2RamReadByte,
                                &Vdp2RamReadWord,
                                &Vdp2RamReadLong,
                                &Vdp2RamWriteByte,
                                &Vdp2RamWriteWord,
                                &Vdp2RamWriteLong,
                                &Vdp2Ram);
   FillMemoryArea(0x5F0, 0x5F7, &Vdp2ColorRamReadByte,
                                &Vdp2ColorRamReadWord,
                                &Vdp2ColorRamReadLong,
                                &Vdp2ColorRamWriteByte,
                                &Vdp2ColorRamWriteWord,
                                &Vdp2ColorRamWriteLong,
                                &Vdp2ColorRam);
   FillMemoryArea(0x5F8, 0x5FB, &Vdp2ReadByte,
                                &Vdp2ReadWord,
                                &Vdp2ReadLong,
                                &Vdp2WriteByte,
                                &Vdp2WriteWord,
                                &Vdp2WriteLong,
                                &VoidMem);
   FillMemoryArea(0x5FE, 0x5FE, &ScuReadByte,
                                &ScuReadWord,
                                &ScuReadLong,
                                &ScuWriteByte,
                                &ScuWriteWord,
                                &ScuWriteLong,
                                &VoidMem);
   FillMemoryArea(0x600,  0x7FF, &HighWramMemoryReadByte,
                                &HighWramMemoryReadWord,
                                &HighWramMemoryReadLong,
                                &HighWramMemoryWriteByte,
                                &HighWramMemoryWriteWord,
                                &HighWramMemoryWriteLong,
                                &HighWram);

     FillMemoryArea( ((backup_file_addr >> 16) & 0xFFF) , (((backup_file_addr + (backup_file_size<<1)) >> 16) & 0xFFF)+1, &BupRamMemoryReadByte,
     &BupRamMemoryReadWord,
     &BupRamMemoryReadLong,
     &BupRamMemoryWriteByte,
     &BupRamMemoryWriteWord,
     &BupRamMemoryWriteLong,
     &BupRam);
}

u8 FASTCALL DMAMappedMemoryReadByte(u32 addr) {
  u8 ret;
  ret = MappedMemoryReadByte(NULL, addr);
return ret;
}

//////////////////////////////////////////////////////////////////////////////
u8 FASTCALL MappedMemoryReadByte(SH2_struct *context, u32 addr)
{
   switch (addr >> 29)
   {
      case 0x0:
      case 0x1:
      {
        return ReadByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadByte(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled Byte R %x\n", addr);
         return UnhandledMemoryReadByte(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }

   return 0;
}
u8 FASTCALL SH2MappedMemoryReadByte(SH2_struct *context, u32 addr) {
CACHE_LOG("rb %x %x\n", addr, addr >> 29);
   switch (addr >> 29)
   {
      case 0x1:
      {
        return ReadByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x0:
      case 0x4:
      {
         return CacheReadByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x2:
         return 0xFF;
      //case 0x4:
      case 0x6:
         // Data Array
       LOG("DAta Byte R %x\n", addr);
         return DataArrayReadByte(context, addr);
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadByte(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled Byte R %x\n", addr);
         return UnhandledMemoryReadByte(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }

   return 0;
}


u16 FASTCALL DMAMappedMemoryReadWord(u32 addr) {
  return MappedMemoryReadWord(NULL, addr);
}

//////////////////////////////////////////////////////////////////////////////
u16 FASTCALL MappedMemoryReadWord(SH2_struct *context, u32 addr)
{
   int id = addr >> 29;
   if (context == NULL) id =1;
   switch (id)
   {
      case 0x0:
      case 0x1:
      {
        return ReadWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x2:
         return UnhandledMemoryReadLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      case 0x7:
      {
         if ((addr >= 0xFFFFFE00) && (context != NULL))
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadWord(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled Word R %x\n", addr);
         return UnhandledMemoryReadWord(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }

   return 0;
}


u16 FASTCALL SH2MappedMemoryReadWord(SH2_struct *context, u32 addr)
{
    int id = addr >> 29;
    if (context == NULL) id =1;
    switch (id)
   {
      case 0x1:
      {
        return ReadWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x0: //0x0 cache
           return CacheReadWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      case 0x2:
      case 0x5:
         return 0xFFFF;
      //case 0x4:
      case 0x6:
         // Data Array
       CACHE_LOG("DAta Word R %x\n", addr);
         return DataArrayReadWord(context, addr);
      case 0x7:
      {
         if ((addr >= 0xFFFFFE00) && (context != NULL))
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadWord(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled Word R %x\n", addr);
         return UnhandledMemoryReadWord(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }

   return 0;
}

u32 FASTCALL DMAMappedMemoryReadLong(u32 addr)
{
  return MappedMemoryReadLong(NULL, addr);
}
//////////////////////////////////////////////////////////////////////////////
u32 FASTCALL MappedMemoryReadLong(SH2_struct *context, u32 addr)
{
  int id = addr >> 29;
  if (context == NULL) id =1;
  switch (id)
   {
      case 0x0:
      case 0x1: //0x0 no cache
      {
        return ReadLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadLong(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled Long R %x %d 0x%x\n", addr, (addr >> 29), (addr >> 16) & 0xFFF);
         return UnhandledMemoryReadLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }
   return 0;
}

u32 FASTCALL SH2MappedMemoryReadLong(SH2_struct *context, u32 addr)
{
  int id = addr >> 29;
  if (context == NULL) id =1;
  switch (id)
   {
      case 0x1: //0x0 no cache
      {
        return ReadLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x0:
      {
         return CacheReadLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
      case 0x2:
      case 0x5:
         return 0xFFFFFFFF;
      case 0x3:
      {
         // Address Array
         return AddressArrayReadLong(context, addr);
      }
      //case 0x4:
      case 0x6:
         // Data Array
       CACHE_LOG("Data Long R %x\n", addr);
         return DataArrayReadLong(context, addr);
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadLong(context, addr);
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         break;
      }
      default:
      {
LOG("Hunandled SH2 Long R %x %d\n", addr,(addr >> 29));
         return UnhandledMemoryReadLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
      }
   }
   return 0;
}

void FASTCALL DMAMappedMemoryWriteByte(u32 addr, u8 val)
{
   MappedMemoryWriteByte(NULL, addr, val);
}

//////////////////////////////////////////////////////////////////////////////
void FASTCALL MappedMemoryWriteByte(SH2_struct *context, u32 addr, u8 val)
{
   int id = addr >> 29;
   if (context == NULL) id =1;
   SH2WriteNotify(context, addr, 1);
   switch (id)
   {
      case 0x0:
      case 0x1:
      {
        WriteByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteByte(context, addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Byte W %x\n", addr);
         UnhandledMemoryWriteByte(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

void FASTCALL SH2MappedMemoryWriteByte(SH2_struct *context, u32 addr, u8 val)
{
   int id = addr >> 29;
   if (context == NULL) id =1;
   SH2WriteNotify(context, addr, 1);
   switch (id)
   {
      case 0x1:
      {
        WriteByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x0:
      {
CACHE_LOG("wb %x %x\n", addr, addr >> 29);
         CacheWriteByteList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }

      case 0x2:
      {
         // Purge Area
         CacheInvalidate(context, addr);
         return;
      }

      //case 0x4:
      case 0x6:
         // Data Array
         DataArrayWriteByte(context, addr, val);
         return;
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteByte(context, addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Byte W %x\n", addr);
         UnhandledMemoryWriteByte(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

void FASTCALL DMAMappedMemoryWriteWord(u32 addr, u16 val)
{
   MappedMemoryWriteWord(NULL, addr, val);
}

//////////////////////////////////////////////////////////////////////////////
void FASTCALL MappedMemoryWriteWord(SH2_struct *context, u32 addr, u16 val)
{
   int id = addr >> 29;
   if (context == NULL) id =1;
   SH2WriteNotify(context, addr, 2);
   switch (id)
   {
      case 0x0:
      case 0x1:
      {
        WriteWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x7:
      {
         if ((addr >= 0xFFFFFE00) && (context != NULL))
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteWord(context, addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM setup
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Word W %x\n", addr);
         UnhandledMemoryWriteWord(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

void FASTCALL SH2MappedMemoryWriteWord(SH2_struct *context, u32 addr, u16 val)
{
   int id = addr >> 29;
   if (context == NULL) id =1;
   SH2WriteNotify(context, addr, 2);
   switch (id)
   {
      case 0x1:
      {
        WriteWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x0:
      {
CACHE_LOG("ww %x %x\n", addr, addr >> 29);
         // Cache/Non-Cached
         CacheWriteWordList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }

      case 0x2:
      {
         // Purge Area
         CacheInvalidate(context, addr);
         return;
      }

      //case 0x4:
      case 0x6:
         // Data Array

         DataArrayWriteWord(context, addr, val);
         return;
      case 0x7:
      {
         if ((addr >= 0xFFFFFE00) && (context != NULL))
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteWord(context, addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM setup
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Word W %x\n", addr);
         UnhandledMemoryWriteWord(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

void FASTCALL DMAMappedMemoryWriteLong(u32 addr, u32 val)
{
   MappedMemoryWriteLong(NULL, addr, val);
}

//////////////////////////////////////////////////////////////////////////////
void FASTCALL MappedMemoryWriteLong(SH2_struct *context, u32 addr, u32 val)
{
   int id = addr >> 29;
   if (context == NULL) id =1;
   SH2WriteNotify(context, addr, 4);
   switch (id)
   {
      case 0x0:
      case 0x1:
      {
        WriteLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteLong(context, addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Long W %x\n", addr);
         UnhandledMemoryWriteLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

void FASTCALL SH2MappedMemoryWriteLong(SH2_struct *context, u32 addr, u32 val)
{
   int id = addr >> 29;
   if (context == NULL) id =1;
   SH2WriteNotify(context, addr, 4);
   switch (id)
   {
      case 0x1:
      {
        WriteLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
        return;
      }
      case 0x0:
      {
CACHE_LOG("wl %x %x\n", addr, addr >> 29);
         // Cache/Non-Cached
         CacheWriteLongList[(addr >> 16) & 0xFFF](context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
      case 0x2:
      {
         // Purge Area
         CacheInvalidate(context, addr);
         return;
      }
      case 0x3:
      {
         // Address Array
         AddressArrayWriteLong(context, addr, val);
         return;
      }
      //case 0x4:
      case 0x6:

         // Data Array
         DataArrayWriteLong(context, addr, val);
         return;
      case 0x7:
      {
         if (addr >= 0xFFFFFE00)
         {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteLong(context, addr, val);
            return;
         }
         else if (addr >= 0xFFFF8000 && addr < 0xFFFFC000)
         {
            // SDRAM
         }
         else
         {
            // Garbage data
         }
         return;
      }
      default:
      {
LOG("Hunandled Long W %x\n", addr);
         UnhandledMemoryWriteLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr, val);
         return;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

int MappedMemoryLoad(SH2_struct *sh, const char *filename, u32 addr)
{
   FILE *fp;
   long filesize;
   u8 *buffer;
   u32 i;
   size_t num_read = 0;

   if (!filename)
      return -1;

   if ((fp = fopen(filename, "rb")) == NULL)
      return -1;

   // Calculate file size
   fseek(fp, 0, SEEK_END);
   filesize = ftell(fp);

   if (filesize <= 0)
   {
      YabSetError(YAB_ERR_FILEREAD, filename);
      fclose(fp);
      return -1;//error
   }

   fseek(fp, 0, SEEK_SET);

   if ((buffer = (u8 *)malloc(filesize)) == NULL)
   {
      fclose(fp);
      return -2;
   }

   num_read = fread((void *)buffer, 1, filesize, fp);
   fclose(fp);

   for (i = 0; i < filesize; i++)
      MappedMemoryWriteByte(sh, addr+i, buffer[i]);

   free(buffer);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int MappedMemorySave(SH2_struct *sh, const char *filename, u32 addr, u32 size)
{
   FILE *fp;
   u8 *buffer;
   u32 i;

   if (!filename)
      return -1;

   if ((fp = fopen(filename, "wb")) == NULL)
      return -1;

   if ((buffer = (u8 *)malloc(size)) == NULL)
   {
      fclose(fp);
      return -2;
   }

   for (i = 0; i < size; i++)
      buffer[i] = MappedMemoryReadByte(sh, addr+i);

   fwrite((void *)buffer, 1, size, fp);
   fclose(fp);
   free(buffer);

   return 0;
}

int BackupHandled(SH2_struct * sh, u32 addr) {
   if ((addr & 0xFFFFF) == 0x7d600) {
     if (sh == NULL) return 1;
     BiosBUPInit(sh);
     return 1;
   }
   if (((addr & 0xFFFFF) >= 0x0384 && (addr & 0xFFFFF) <= 0x03A8) || ((addr & 0xFFFFF) == 0x358) ) {
     if (sh == NULL) return 1;
     return BiosHandleFunc(sh); //replace by NOP
   }
   return 0;
}

int isBackupHandled(u32 addr) {
   return( ((addr & 0xFFFFF) == 0x7d600) || (((addr & 0xFFFFF) >= 0x0384 && (addr & 0xFFFFF) <= 0x03A8) || ((addr & 0xFFFFF) == 0x358) ));
}

int BackupInit(const char* path, int extended) {
  int currentSaveSize = TSize(path);
  int forceFormat = 0;
  if (currentSaveSize != -1) {
    int isNormalSize = (currentSaveSize == 0x8000);
    int isExtendedSize = (currentSaveSize == EXTENDED_BACKUP_SIZE);
    if (extended && isNormalSize) {
      extended = 0; //Force to use small save format
      YuiMsg("Internal backup file format is detected as standard save format - Force to use standard save format\n");
    }
    if (!extended && isExtendedSize) {
      extended = 1; //Force to use large save format
      YuiMsg("Internal backup file format is detected as extended save format - Force to use extended save format\n");
    }
    if (!((!extended && isNormalSize)||(extended && isExtendedSize))) {
      YuiMsg("Internal backup file format is bad - Force format to %s save format\n", (extended?"extended":"standard") );
      forceFormat =1; //Size is not the good one - Format
    }
  }
  if (!extended) {
    backup_file_addr = 0x00180000;
    backup_file_size = 0x8000;
  } else {
    backup_file_addr = EXTENDED_BACKUP_ADDR;
    backup_file_size = EXTENDED_BACKUP_SIZE;
  }

  if ((BupRam = T1MemoryInit(backup_file_size)) == NULL)
       return -1;

  if ((LoadBackupRam(path) != 0) || (forceFormat))
       FormatBackupRam(BupRam, backup_file_size);
  BupRamWritten = 0;
  bupfilename = path;
  return 0;
}

void BackupFlush() {
  if (BupRam)
  {
      if (T123Save(BupRam, backup_file_size, 1, bupfilename) != 0)
        YabSetError(YAB_ERR_FILEWRITE, (void *)bupfilename);
  }
}

void BackupExtended() {
}

void BackupDeinit() {
   if (BupRam)
   {
       if (T123Save(BupRam, backup_file_size, 1, bupfilename) != 0)
        YabSetError(YAB_ERR_FILEWRITE, (void *)bupfilename);
       T1MemoryDeInit(BupRam);
   }
   BupRam = NULL;
}


//////////////////////////////////////////////////////////////////////////////

int LoadBios(const char *filename)
{
  int ret = 0;
  if (yabsys.isSTV == 0) ret = T123Load(BiosRom, 0x80000, 2, filename); //Saturn
  if (yabsys.isSTV) YuiMsg("ST-V Emulation mode\n");
  else YuiMsg("Saturn Emulation mode\n");
  return ret;
}

//////////////////////////////////////////////////////////////////////////////

int LoadBackupRam(const char *filename)
{
   return T123Load(BupRam, backup_file_size, 1, filename);
}

static u8 header[16] = {
  'B', 'a', 'c', 'k',
  'U', 'p', 'R', 'a',
  'm', ' ', 'F', 'o',
  'r', 'm', 'a', 't'
};

int CheckBackupFile(FILE *fp) {
  int i, i2;
  u32 i3;

  // Fill in header
  for (i2 = 0; i2 < 4; i2++) {
    for (i = 0; i < 16; i++) {
      u8 val = fgetc(fp);
      if ( val != header[i]) {
        return -1;
      }
    }
  }
  return 0;
}

int ExtendBackupFile(FILE *fp, u32 size ) {
  u32 acsize;
  fseek(fp, 0, SEEK_END);
  acsize = ftell(fp);
  if (acsize < size) {
    // Clear the rest
    for ( u32 i = (acsize&0xFFFFFFFE) ; i < size; i++)
    {
      fputc(0x00, fp);
    }
    fflush(fp);
  }
  fseek(fp, 0, SEEK_SET);

  return 0;
}


//////////////////////////////////////////////////////////////////////////////
void FormatBackupRamFile(FILE *fp, u32 size) {

  int i, i2;
  u32 i3;

  // Fill in header
  for (i2 = 0; i2 < 4; i2++)
    for (i = 0; i < 16; i++)
      fputc(header[i],fp);

  // Clear the rest
  for (i3 = 0x80; i3 < size; i3 ++)
  {
    fputc(0x00,fp);
  }
  fflush(fp);
}

void FormatBackupRam(void *mem, u32 size)
{
   int i, i2;
   u32 i3;

   // Fill in header
   for(i2 = 0; i2 < 4; i2++)
      for(i = 0; i < 16; i++)
         T1WriteByte((u8*)mem, (i2 * 16) + i, header[i]);

   // Clear the rest
   for(i3 = 0x80; i3 < size; i3++)
   {
      T1WriteByte((u8*)mem, i3, 0x00);
   }
}

//////////////////////////////////////////////////////////////////////////////

static int MemStateCurrentOffset = 0;

void MemStateWrite(void * ptr, size_t size, size_t nmemb, void ** stream)
{
   if (stream != NULL)
      memcpy((char *)(*stream) + MemStateCurrentOffset, ptr, size*nmemb);
   MemStateCurrentOffset += size*nmemb;
}

void MemStateWriteOffset(void * ptr, size_t size, size_t nmemb, void ** stream, int offset)
{
   if (stream != NULL)
      memcpy((char *)(*stream) + offset, ptr, size*nmemb);
}

int MemStateWriteHeader(void ** stream, const char *name, int version)
{
   MemStateWrite((void *)name, 1, 4, stream); // lengths are fixed for this, no need to guess
   MemStateWrite((void *)&version, sizeof(version), 1, stream);
   MemStateWrite((void *)&version, sizeof(version), 1, stream); // place holder for size
   return MemStateCurrentOffset;
}

int MemStateFinishHeader(void ** stream, int offset)
{
   int size = 0;
   size = MemStateCurrentOffset - offset;
   MemStateWriteOffset((void *)&size, sizeof(size), 1, stream, offset - 4);
   return (size + 12);
}

void MemStateRead(void * ptr, size_t size, size_t nmemb, const void * stream)
{
   memcpy(ptr, (const char *)stream + MemStateCurrentOffset, size*nmemb);
   MemStateCurrentOffset += size*nmemb;
}

void MemStateReadOffset(void * ptr, size_t size, size_t nmemb, const void * stream, int offset)
{
   memcpy(ptr, (const char *)stream + offset, size*nmemb);
}

int MemStateCheckRetrieveHeader(const void * stream, const char *name, int *version, int *size) {
   char id[4];

   MemStateRead((void *)id, 1, 4, stream);

   if (strncmp(name, id, 4) != 0)
      return -2;

   MemStateRead((void *)version, 1, 4, stream);
   MemStateRead((void *)size, 1, 4, stream);

   return 0;
}

void MemStateSetOffset(int offset) {
   MemStateCurrentOffset = offset;
}

int MemStateGetOffset() {
   return MemStateCurrentOffset;
}

//////////////////////////////////////////////////////////////////////////////

int YabSaveStateBuffer(void ** buffer, size_t * size)
{
   int status;

   // Reset buffer & size if needed
   if (buffer != NULL) *buffer = NULL;
   *size = 0;

   // Mute scsp & lock its thread (workaround for audio issues ? it doesn't seem reliable though)
   ScspMuteAudio(SCSP_MUTE_SYSTEM);
   ScspLockThread();

   // Get size
   status = YabSaveStateStream(NULL);
   if (status != 0) {
      ScspUnLockThread();
      return status;
   }
   *size = MemStateGetOffset();

   // Allocate buffer
   *buffer = (void *)malloc(*size);
   if (buffer == NULL) {
      ScspUnLockThread();
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return -1;
   }

   // Fill buffer
   status = YabSaveStateStream(buffer);
   if (status != 0) {
      ScspUnLockThread();
      ScspUnMuteAudio(SCSP_MUTE_SYSTEM);
      return status;
   }

   // Unlock scsp thread
   ScspUnLockThread();
   ScspUnMuteAudio(SCSP_MUTE_SYSTEM);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YabSaveState(const char *filename)
{
   FILE *fp;
   int status;

   // use a second set of savestates for movies
   filename = MakeMovieStateName(filename);
   if (!filename)
      return -1;

   // stop if we can't open file
   if ((fp = fopen(filename, "wb")) == NULL)
      return -1;

   // retrieve savestate buffer and its size
   void *buffer;
   size_t size;
   status = YabSaveStateBuffer(&buffer, &size);

   // if retrieval failed, cleanup and stop
   if (status != 0) {
      fclose(fp);
      if(buffer)
         free(buffer);
      return status;
   }

   // if writing fails (out of space ?), cleanup and stop
   if (fwrite(buffer, 1, size, fp) != size) {
      fclose(fp);
      if(buffer)
         free(buffer);
      return -1;
   }

   // cleanup
   fclose(fp);
   if(buffer)
      free(buffer);

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

// FIXME: Here's a (possibly incomplete) list of data that should be added
// to the next version of the save state file:
//    yabsys.DecilineStop (new format)
//    yabsys.SH2CycleFrac (new field)
//    yabsys.DecilineUSed (new field)
//    yabsys.UsecFrac (new field)
//    [scsp2.c] It would be nice to redo the format entirely because so
//              many fields have changed format/size from the old scsp.c
//    [scsp2.c] scsp_clock, scsp_clock_frac, ScspState.sample_timer (timing)
//    [scsp2.c] cdda_buf, cdda_next_in, cdda_next_out (CDDA buffer)
//    [sh2core.c] frc.div changed to frc.shift
//    [sh2core.c] wdt probably needs to be written as well

int YabSaveStateStream(void ** stream)
{
   u32 i;
   int offset;
   u8 *buf;
   int totalsize;
   int outputwidth;
   int outputheight;
   int movieposition;
   int temp;
   u32 temp32;
   u8 endian;

   // Set buffer position to 0
   MemStateSetOffset(0);

   // Write signature
   MemStateWrite((void*)"YSS", 1, 3, stream);

   // Write endianness byte
#ifdef WORDS_BIGENDIAN
   endian = 0x00;
#else
   endian = 0x01;
#endif
   MemStateWrite((void *)&endian, 1, 1, stream);

   // Write version(fix me)
   i = 3;
   MemStateWrite((void *)&i, sizeof(i), 1, stream);

   // Skip the next 4 bytes for now
   i = 0;
   MemStateWrite((void *)&i, sizeof(i), 1, stream);

   //write frame number
   MemStateWrite((void *)&framecounter, 4, 1, stream);

   //this will be updated with the movie position later
   MemStateWrite((void *)&framecounter, 4, 1, stream);

   // Go through each area and write each state
   i += CartSaveState(stream);
   i += Cs2SaveState(stream);
   i += SH2SaveState(MSH2, stream);
   i += SH2SaveState(SSH2, stream);
   i += SoundSaveState(stream);
   i += ScuSaveState(stream);
   i += SmpcSaveState(stream);
   i += Vdp1SaveState(stream);
   i += Vdp2SaveState(stream);

   offset = MemStateWriteHeader(stream, "OTHR", 1);

   // Other data
   //MemStateWrite((void *)BupRam, 0x8000, 1, stream); // do we really want to save this?
   MemStateWrite((void *)HighWram, 0x100000, 1, stream);
   MemStateWrite((void *)LowWram, 0x100000, 1, stream);

   MemStateWrite((void *)&yabsys.DecilineCount, sizeof(int), 1, stream);
   MemStateWrite((void *)&yabsys.LineCount, sizeof(int), 1, stream);
   MemStateWrite((void *)&yabsys.VBlankLineCount, sizeof(int), 1, stream);
   MemStateWrite((void *)&yabsys.MaxLineCount, sizeof(int), 1, stream);
   temp = yabsys.DecilineStop >> YABSYS_TIMING_BITS;
   MemStateWrite((void *)&temp, sizeof(int), 1, stream);
   temp = (yabsys.CurSH2FreqType == CLKTYPE_26MHZ) ? 268 : 286;
   MemStateWrite((void *)&temp, sizeof(int), 1, stream);
   temp32 = (yabsys.UsecFrac * temp / 10) >> YABSYS_TIMING_BITS;
   MemStateWrite((void *)&temp32, sizeof(u32), 1, stream);
   MemStateWrite((void *)&yabsys.CurSH2FreqType, sizeof(int), 1, stream);
   MemStateWrite((void *)&yabsys.IsPal, sizeof(int), 1, stream);

   VIDCore->GetGlSize(&outputwidth, &outputheight);

   totalsize=outputwidth * outputheight * sizeof(u32);

   if ((buf = (u8 *)malloc(totalsize)) == NULL)
   {
      return -2;
   }

   //YuiSwapBuffers();
   #ifdef USE_OPENGL
   glPixelZoom(1,1);
   glReadBuffer(GL_BACK);
   glReadPixels(0, 0, outputwidth, outputheight, GL_RGBA, GL_UNSIGNED_BYTE, buf);
   #else
   //memcpy(buf, dispbuffer, totalsize);
   #endif
   //YuiSwapBuffers();

   MemStateWrite((void *)&outputwidth, sizeof(outputwidth), 1, stream);
   MemStateWrite((void *)&outputheight, sizeof(outputheight), 1, stream);

   MemStateWrite((void *)buf, totalsize, 1, stream);

   movieposition=MemStateGetOffset();

   //write the movie to the end of the savestate
   SaveMovieInState(stream);

   i += MemStateFinishHeader(stream, offset);

   // Go back and update size
   MemStateWriteOffset((void *)&i, sizeof(i), 1, stream, 8);
   MemStateWriteOffset((void *)&movieposition, sizeof(movieposition), 1, stream, 16);

   free(buf);

   OSDPushMessage(OSDMSG_STATUS, 150, "STATE SAVED");
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YabLoadStateBuffer(const void * buffer, size_t size)
{
   int status;

   ScspMuteAudio(SCSP_MUTE_SYSTEM);
   ScspLockThread();
   status = YabLoadStateStream(buffer, size);
   ScspUnLockThread();
   ScspUnMuteAudio(SCSP_MUTE_SYSTEM);

   return status;
}

//////////////////////////////////////////////////////////////////////////////

int YabLoadState(const char *filename)
{
   FILE *fp;
   int status;

   filename = MakeMovieStateName(filename);
   if (!filename)
      return -1;

   if ((fp = fopen(filename, "rb")) == NULL)
      return -1;

   // retrieve filesize
   fseek(fp, 0, SEEK_END);
   size_t size = ftell(fp);
   fseek(fp, 0L, SEEK_SET);

   // prepare buffer
   void *buffer = (void *)malloc(size);
   if (buffer == NULL) {
      fclose(fp);
      return -1;
   }
   fread(buffer, 1, size, fp);

   fclose(fp);

   status = YabLoadStateBuffer(buffer, size);

   return status;
}

//////////////////////////////////////////////////////////////////////////////

int YabLoadStateStream(const void * stream, size_t size_stream)
{
   char id[3];
   u8 endian;
   int headerversion, version, size, chunksize, headersize;
   u8* buf;
   int totalsize;
   int outputwidth;
   int outputheight;
   int curroutputwidth;
   int curroutputheight;
   int movieposition;
   int temp;
   u32 temp32;
   int test_endian;

   headersize = 0xC;

   // Set buffer position to 0
   MemStateSetOffset(0);

   // Read signature
   MemStateRead(&id, 1, 3, stream);

   if (strncmp(id, "YSS", 3) != 0)
   {
      YuiMsg("Save file is not a YSS\n");
      return -2;
   }

   // Read header
   MemStateRead((void *)&endian, 1, 1, stream);
   MemStateRead((void *)&headerversion, 4, 1, stream);
   MemStateRead((void *)&size, 4, 1, stream);

   switch(headerversion)
   {
      case 1:
         /* This is the "original" version of the format */
         break;
      case 2:
      case 3:
         /* version 2 adds video recording */
         MemStateRead((void *)&framecounter, 4, 1, stream);
         movieposition=MemStateGetOffset(); // yabause was doing this, why ? it's replaced right after
         MemStateRead((void *)&movieposition, 4, 1, stream);
         headersize = 0x14;
         break;
      default:
         /* we're trying to open a save state using a future version
          * of the YSS format, that won't work, sorry :) */
         YuiMsg("Save file is not supported. Might be a future version (%d).\n", headerversion);
         return -3;
         break;
   }

#ifdef WORDS_BIGENDIAN
   test_endian = endian == 1;
#else
   test_endian = endian == 0;
#endif
   if (test_endian)
   {
      // should setup reading so it's byte-swapped
      YabSetError(YAB_ERR_OTHER, (void *)"Load State byteswapping not supported");
      return -3;
   }

   // Make sure size variable matches actual size minus header
   if (size != (size_stream - headersize))
      return -2;

   // Verify version here

   if (MemStateCheckRetrieveHeader(stream, "CART", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      YuiMsg("Can not load %s (Ver %d)\n", "CART", version);
      return -3;
   }
   CartLoadState(stream, version, chunksize);

   if (MemStateCheckRetrieveHeader(stream, "CS2 ", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      YuiMsg("Can not load %s (Ver %d)\n", "CS2", version);
      return -3;
   }
   Cs2LoadState(stream, version, chunksize);

   if (MemStateCheckRetrieveHeader(stream, "MSH2", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      YuiMsg("Can not load %s (Ver %d)\n", "MSH2", version);
      return -3;
   }
   SH2LoadState(MSH2, stream, version, chunksize);

   if (MemStateCheckRetrieveHeader(stream, "SSH2", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      YuiMsg("Can not load %s (Ver %d)\n", "SSH2", version);
      return -3;
   }
   SH2LoadState(SSH2, stream, version, chunksize);

   if (MemStateCheckRetrieveHeader(stream, "SCSP", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      YuiMsg("Can not load %s (Ver %d)\n", "SCSP", version);
      return -3;
   }
   SoundLoadState(stream, version, chunksize);

   if (MemStateCheckRetrieveHeader(stream, "SCU ", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      YuiMsg("Can not load %s (Ver %d)\n", "SCU", version);
      return -3;
   }
   ScuLoadState(stream, version, chunksize);

   if (MemStateCheckRetrieveHeader(stream, "SMPC", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      YuiMsg("Can not load %s (Ver %d)\n", "SMPC", version);
      return -3;
   }
   SmpcLoadState(stream, version, chunksize);

   if (MemStateCheckRetrieveHeader(stream, "VDP1", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      YuiMsg("Can not load %s (Ver %d)\n", "VDP1", version);
      return -3;
   }
   Vdp1LoadState(stream, version, chunksize);

   if (MemStateCheckRetrieveHeader(stream, "VDP2", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      YuiMsg("Can not load %s (Ver %d)\n", "VDP2", version);
      return -3;
   }
   Vdp2LoadState(stream, version, chunksize);

   if (MemStateCheckRetrieveHeader(stream, "OTHR", &version, &chunksize) != 0)
   {
      // Revert back to old state here
      YuiMsg("Can not load %s (Ver %d)\n", "OTHR", version);
      return -3;
   }
   // Other data
   //MemStateRead((void *)BupRam, 0x8000, 1, stream);
   MemStateRead((void *)HighWram, 0x100000, 1, stream);
   MemStateRead((void *)LowWram, 0x100000, 1, stream);

   MemStateRead((void *)&yabsys.DecilineCount, sizeof(int), 1, stream);
   MemStateRead((void *)&yabsys.LineCount, sizeof(int), 1, stream);
   MemStateRead((void *)&yabsys.VBlankLineCount, sizeof(int), 1, stream);
   MemStateRead((void *)&yabsys.MaxLineCount, sizeof(int), 1, stream);
   MemStateRead((void *)&temp, sizeof(int), 1, stream);
   MemStateRead((void *)&temp, sizeof(int), 1, stream);
   MemStateRead((void *)&temp32, sizeof(u32), 1, stream);
   MemStateRead((void *)&yabsys.CurSH2FreqType, sizeof(int), 1, stream);
   MemStateRead((void *)&yabsys.IsPal, sizeof(int), 1, stream);
   YabauseChangeTiming(yabsys.CurSH2FreqType);
   yabsys.UsecFrac = (temp32 << YABSYS_TIMING_BITS) * temp / 10;

   if (headerversion > 1) {

      MemStateRead((void *)&outputwidth, sizeof(outputwidth), 1, stream);
      MemStateRead((void *)&outputheight, sizeof(outputheight), 1, stream);

      totalsize=outputwidth * outputheight * sizeof(u32);

      if ((buf = (u8 *)malloc(totalsize)) == NULL)
      {
         return -2;
      }

      MemStateRead((void *)buf, totalsize, 1, stream);

      YuiTimedSwapBuffers();

#ifdef USE_OPENGL
      if(VIDCore->id == VIDCORE_SOFT)
         glRasterPos2i(0, outputheight);
      if(VIDCore->id == VIDCORE_OGL)
         glRasterPos2i(0, outputheight/2);
#endif

      VIDCore->GetGlSize(&curroutputwidth, &curroutputheight);
#ifdef USE_OPENGL
      glPixelZoom((float)curroutputwidth / (float)outputwidth, ((float)curroutputheight / (float)outputheight));
      glDrawPixels(outputwidth, outputheight, GL_RGBA, GL_UNSIGNED_BYTE, buf);
#endif
      YuiTimedSwapBuffers();
      free(buf);

      MemStateSetOffset(movieposition);
      MovieReadState(stream);
   }

   OSDPushMessage(OSDMSG_STATUS, 150, "STATE LOADED");
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YabSaveStateSlot(const char *dirpath, u8 slot)
{
  int rtn;
   char filename[512];

   if (cdip == NULL)
      return -1;

#ifdef WIN32
   sprintf(filename, "%s\\%s_%03d.yss", dirpath, cdip->itemnum, slot);
#else
   sprintf(filename, "%s/%s_%03d.yss", dirpath, cdip->itemnum, slot);
#endif

   rtn = YabSaveState(filename);

   return rtn;
}

//////////////////////////////////////////////////////////////////////////////

int YabLoadStateSlot(const char *dirpath, u8 slot)
{
   int rtn;
   char filename[512];

   if (cdip == NULL)
      return -1;

#ifdef WIN32
   sprintf(filename, "%s\\%s_%03d.yss", dirpath, cdip->itemnum, slot);
#else
   sprintf(filename, "%s/%s_%03d.yss", dirpath, cdip->itemnum, slot);
#endif

   rtn = YabLoadState(filename);

   return rtn;
}

//////////////////////////////////////////////////////////////////////////////

static int MappedMemoryAddMatch(SH2_struct *sh, u32 addr, u32 val, int searchtype, result_struct *result, u32 *numresults)
{
   result[numresults[0]].addr = addr;
   result[numresults[0]].val = val;
   numresults[0]++;
   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static INLINE int SearchIncrementAndCheckBounds(result_struct *prevresults,
                                                u32 *maxresults,
                                                u32 numresults, u32 *i,
                                                u32 inc, u32 *newaddr,
                                                u32 endaddr)
{
   if (prevresults)
   {
      if (i[0] >= maxresults[0])
      {
         maxresults[0] = numresults;
         return 1;
      }
      newaddr[0] = prevresults[i[0]].addr;
      i[0]++;
   }
   else
   {
      newaddr[0] = inc;

      if (newaddr[0] > endaddr || numresults >= maxresults[0])
      {
         maxresults[0] = numresults;
         return 1;
      }
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////

static int SearchString(SH2_struct *sh, u32 startaddr, u32 endaddr, int searchtype,
                        const char *searchstr, result_struct *results,
                        u32 *maxresults)
{
   u8 *buf=NULL;
   u32 *buf32=NULL;
   u32 buflen=0;
   u32 counter;
   u32 addr;
   u32 numresults=0;

   buflen=(u32)strlen(searchstr);

   if ((buf32=(u32 *)malloc(buflen*sizeof(u32))) == NULL)
      return 0;

   buf = (u8 *)buf32;

   // Copy string to buffer
   switch (searchtype & 0x70)
   {
      case SEARCHSTRING:
         strcpy((char *)buf, searchstr);
         break;
      case SEARCHREL8BIT:
      case SEARCHREL16BIT:
      {
         char *text;
         char *searchtext=strdup(searchstr);

         // Calculate buffer length and read values into table
         buflen = 0;
         for (text=strtok((char *)searchtext, " ,"); text != NULL; text=strtok(NULL, " ,"))
         {
            buf32[buflen] = strtoul(text, NULL, 0);
            buflen++;
         }
         free(searchtext);

         break;
      }
   }

   addr = startaddr;
   counter = 0;

   for (;;)
   {
      // Fetch byte/word/etc.
      switch (searchtype & 0x70)
      {
         case SEARCHSTRING:
         {
            u8 val = MappedMemoryReadByte(sh, addr);
            addr++;

            if (val == buf[counter])
            {
               counter++;
               if (counter == buflen)
                  MappedMemoryAddMatch(sh, addr-buflen, val, searchtype, results, &numresults);
            }
            else
               counter = 0;
            break;
         }
         case SEARCHREL8BIT:
         {
            int diff;
            u32 j;
            u8 val2;
            u8 val = MappedMemoryReadByte(sh, addr);

            for (j = 1; j < buflen; j++)
            {
               // grab the next value
               val2 = MappedMemoryReadByte(sh, addr+j);

               // figure out the diff
               diff = (int)val2 - (int)val;

               // see if there's a match
               if (((int)buf32[j] - (int)buf32[j-1]) != diff)
                  break;

               if (j == (buflen - 1))
                  MappedMemoryAddMatch(sh, addr, val, searchtype, results, &numresults);

               val = val2;
            }

            addr++;

            break;
         }
         case SEARCHREL16BIT:
         {
            int diff;
            u32 j;
            u16 val2;
            u16 val = MappedMemoryReadWord(sh, addr);

            for (j = 1; j < buflen; j++)
            {
               // grab the next value
               val2 = MappedMemoryReadWord(sh, addr+(j*2));

               // figure out the diff
               diff = (int)val2 - (int)val;

               // see if there's a match
               if (((int)buf32[j] - (int)buf32[j-1]) != diff)
                  break;

               if (j == (buflen - 1))
                  MappedMemoryAddMatch(sh, addr, val, searchtype, results, &numresults);

               val = val2;
            }

            addr+=2;
            break;
         }
      }

      if (addr > endaddr || numresults >= maxresults[0])
         break;
   }

   free(buf);
   maxresults[0] = numresults;
   return 1;
}

//////////////////////////////////////////////////////////////////////////////

result_struct *MappedMemorySearch(SH2_struct *sh, u32 startaddr, u32 endaddr, int searchtype,
                                  const char *searchstr,
                                  result_struct *prevresults, u32 *maxresults)
{
   u32 i=0;
   result_struct *results;
   u32 numresults=0;
   unsigned long searchval = 0;
   int issigned=0;
   u32 addr;

   if ((results = (result_struct *)malloc(sizeof(result_struct) * maxresults[0])) == NULL)
      return NULL;

   switch (searchtype & 0x70)
   {
      case SEARCHSTRING:
      case SEARCHREL8BIT:
      case SEARCHREL16BIT:
      {
         // String/8-bit relative/16-bit relative search(not supported, yet)
         if (SearchString(sh, startaddr, endaddr,  searchtype, searchstr,
                          results, maxresults) == 0)
         {
            maxresults[0] = 0;
            free(results);
            return NULL;
         }

         return results;
      }
      case SEARCHHEX:
         sscanf(searchstr, "%08lx", &searchval);
         break;
      case SEARCHUNSIGNED:
         searchval = (unsigned long)strtoul(searchstr, NULL, 10);
         issigned = 0;
         break;
      case SEARCHSIGNED:
         searchval = (unsigned long)strtol(searchstr, NULL, 10);
         issigned = 1;
         break;
   }

   if (prevresults)
   {
      addr = prevresults[i].addr;
      i++;
   }
   else
      addr = startaddr;

   // Regular value search
   for (;;)
   {
       u32 val=0;
       u32 newaddr;

       // Fetch byte/word/etc.
       switch (searchtype & 0x3)
       {
          case SEARCHBYTE:
             val = MappedMemoryReadByte(sh, addr);
             // sign extend if neccessary
             if (issigned)
                val = (s8)val;

             if (SearchIncrementAndCheckBounds(prevresults, maxresults, numresults, &i, addr+1, &newaddr, endaddr))
                return results;
             break;
          case SEARCHWORD:
             val = MappedMemoryReadWord(sh, addr);
             // sign extend if neccessary
             if (issigned)
                val = (s16)val;

             if (SearchIncrementAndCheckBounds(prevresults, maxresults, numresults, &i, addr+2, &newaddr, endaddr))
                return results;
             break;
          case SEARCHLONG:
             val = MappedMemoryReadLong(sh, addr);

             if (SearchIncrementAndCheckBounds(prevresults, maxresults, numresults, &i, addr+4, &newaddr, endaddr))
                return results;
             break;
          default:
             maxresults[0] = 0;
             if (results)
                free(results);
             return NULL;
       }

       // Do a comparison
       switch (searchtype & 0xC)
       {
          case SEARCHEXACT:
             if (val == searchval)
                MappedMemoryAddMatch(sh, addr, val, searchtype, results, &numresults);
             break;
          case SEARCHLESSTHAN:
             if ((!issigned && val < searchval) || (issigned && (signed)val < (signed)searchval))
                MappedMemoryAddMatch(sh, addr, val, searchtype, results, &numresults);
             break;
          case SEARCHGREATERTHAN:
             if ((!issigned && val > searchval) || (issigned && (signed)val > (signed)searchval))
                MappedMemoryAddMatch(sh, addr, val, searchtype, results, &numresults);
             break;
          default:
             maxresults[0] = 0;
             if (results)
                free(results);
             return NULL;
       }

       addr = newaddr;
   }

   maxresults[0] = numresults;
   return results;
}
