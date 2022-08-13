#include "memory_sh2.h"
#include "memory.h"
#include "sh2core.h"

u8** MemoryBuffer[0x1000] = {};

#define LOG(...)

void CacheInvalidate(SH2_struct* context, u32 addr) {
#ifdef USE_CACHE
    if (yabsys.usecache == 0) return;
    u8 line = (addr >> 4) & 0x3F;
    u32 tag = (addr >> 10) & 0x7FFFF;
    u8 way = context->tagWay[line][tag];
    context->tagWay[line][tag] = 0x4;
    if (way <= 0x3) context->cacheTagArray[line][way] = 0x0;
    context->cacheLRU[line] = 0;
#endif
}

static u8 FASTCALL UnhandledMemoryReadByte(SH2_struct* context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
    LOG("Unhandled byte read %08X\n", (unsigned int)addr);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u16 FASTCALL UnhandledMemoryReadWord(SH2_struct* context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
    LOG("Unhandled word read %08X\n", (unsigned int)addr);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static u32 FASTCALL UnhandledMemoryReadLong(SH2_struct* context, UNUSED u8* memory, USED_IF_DEBUG u32 addr)
{
    LOG("Unhandled long read %08X\n", (unsigned int)addr);
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteByte(SH2_struct* context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u8 val)
{
    LOG("Unhandled byte write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteWord(SH2_struct* context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u16 val)
{
    LOG("Unhandled word write %08X\n", (unsigned int)addr);
}

//////////////////////////////////////////////////////////////////////////////

static void FASTCALL UnhandledMemoryWriteLong(SH2_struct* context, UNUSED u8* memory, USED_IF_DEBUG u32 addr, UNUSED u32 val)
{
    LOG("Unhandled long write %08X\n", (unsigned int)addr);
}

u8 FASTCALL SH2MappedMemoryReadByte(SH2_struct* context, u32 addr) {
    CACHE_LOG("rb %x %x\n", addr, addr >> 29);
    switch (addr >> 29)
    {
    case 0x4:
    case 0x0:
    case 0x1:
    {
        return ReadByteList[(addr >> 16) & 0xFFF](addr);
    }
//    case 0x0:
//    case 0x4:
//    {
//        return CacheReadByteList[(addr >> 16) & 0xFFF](addr);
//    }
    case 0x2:
        return 0xFF;
        //case 0x4:
    case 0x6:
        // Data Array
        LOG("DAta Byte R %x\n", addr);
        return DataArrayReadByte(addr);
    case 0x7:
    {
        if (addr >= 0xFFFFFE00)
        {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadByte(addr);
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

u16 FASTCALL SH2MappedMemoryReadWord(SH2_struct* context, u32 addr)
{
    int id = addr >> 29;
//    if (context == NULL) id = 1;
    switch (id)
    {
    case 0x0: //0x0 cache
    case 0x1:
    {
        return ReadWordList[(addr >> 16) & 0xFFF](addr);
    }
//    case 0x0: //0x0 cache
//        return CacheReadWordList[(addr >> 16) & 0xFFF](addr);
    case 0x2:
    case 0x5:
        return 0xFFFF;
        //case 0x4:
    case 0x6:
        // Data Array
        CACHE_LOG("Data Word R %x\n", addr);
        return DataArrayReadWord(addr);
    case 0x7:
    {
        if ((addr >= 0xFFFFFE00) /* && (context != NULL)*/)
        {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadWord(addr);
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

u32 FASTCALL SH2MappedMemoryReadLong(SH2_struct* context, u32 addr)
{
    int id = addr >> 29;
//    if (context == NULL) id = 1;
    switch (id)
    {
    case 0x0:
    case 0x1: //0x0 no cache
    {
        return ReadLongList[(addr >> 16) & 0xFFF](addr);
    }
//    case 0x0:
//    {
//        return CacheReadLongList[(addr >> 16) & 0xFFF](addr);
//    }
    case 0x2:
    case 0x5:
        return 0xFFFFFFFF;
    case 0x3:
    {
        // Address Array
        return AddressArrayReadLong(addr);
    }
    //case 0x4:
    case 0x6:
        // Data Array
        CACHE_LOG("Data Long R %x\n", addr);
        return DataArrayReadLong(addr);
    case 0x7:
    {
        if (addr >= 0xFFFFFE00)
        {
            // Onchip modules
            addr &= 0x1FF;
            return OnchipReadLong(addr);
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
        LOG("Hunandled SH2 Long R %x %d\n", addr, (addr >> 29));
        return UnhandledMemoryReadLong(context, *(MemoryBuffer[(addr >> 16) & 0xFFF]), addr);
    }
    }
    return 0;
}

void FASTCALL SH2MappedMemoryWriteByte(SH2_struct* context, u32 addr, u8 val)
{
    int id = addr >> 29;
//    if (context == NULL) id = 1;
    SH2WriteNotify(CurrentSH2, addr, 1);
    switch (id)
    {
    case 0x0:
    case 0x1:
    {
        WriteByteList[(addr >> 16) & 0xFFF](addr, val);
        return;
    }
//    case 0x0:
//    {
//        CACHE_LOG("wb %x %x\n", addr, addr >> 29);
//        CacheWriteByteList[(addr >> 16) & 0xFFF](addr, val);
//        return;
//    }

    case 0x2:
    {
        // Purge Area
        CacheInvalidate(context, addr);
        return;
    }

    //case 0x4:
    case 0x6:
        // Data Array
        DataArrayWriteByte(addr, val);
        return;
    case 0x7:
    {
        if (addr >= 0xFFFFFE00)
        {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteByte(addr, val);
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

void FASTCALL SH2MappedMemoryWriteWord(SH2_struct* context, u32 addr, u16 val)
{
    int id = addr >> 29;
//    if (context == NULL) id = 1;
    SH2WriteNotify(CurrentSH2, addr, 2);
    switch (id)
    {
    case 0x0:
    case 0x1:
    {
        WriteWordList[(addr >> 16) & 0xFFF](addr, val);
        return;
    }
//    case 0x0:
//    {
//        CACHE_LOG("ww %x %x\n", addr, addr >> 29);
//        // Cache/Non-Cached
//        CacheWriteWordList[(addr >> 16) & 0xFFF](addr, val);
//        return;
//    }

    case 0x2:
    {
        // Purge Area
        CacheInvalidate(context, addr);
        return;
    }

    //case 0x4:
    case 0x6:
        // Data Array

        DataArrayWriteWord(addr, val);
        return;
    case 0x7:
    {
        if ((addr >= 0xFFFFFE00) /* && (context != NULL)*/)
        {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteWord(addr, val);
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

void FASTCALL SH2MappedMemoryWriteLong(SH2_struct* context, u32 addr, u32 val)
{
    int id = addr >> 29;
//    if (context == NULL) id = 1;
    SH2WriteNotify(CurrentSH2, addr, 4);
    switch (id)
    {
    case 0x0:
    case 0x1:
    {
        WriteLongList[(addr >> 16) & 0xFFF](addr, val);
        return;
    }
//    case 0x0:
//    {
//        CACHE_LOG("wl %x %x\n", addr, addr >> 29);
//        // Cache/Non-Cached
//        CacheWriteLongList[(addr >> 16) & 0xFFF](addr, val);
//        return;
//    }
    case 0x2:
    {
        // Purge Area
        CacheInvalidate(context, addr);
        return;
    }
    case 0x3:
    {
        // Address Array
        AddressArrayWriteLong(addr, val);
        return;
    }
    //case 0x4:
    case 0x6:

        // Data Array
        DataArrayWriteLong(addr, val);
        return;
    case 0x7:
    {
        if (addr >= 0xFFFFFE00)
        {
            // Onchip modules
            addr &= 0x1FF;
            OnchipWriteLong(addr, val);
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
