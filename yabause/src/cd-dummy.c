#include "cdbase.h"

//////////////////////////////////////////////////////////////////////////////

// Contains the Dummy and ISO CD Interfaces

static int DummyCDInit(const char*);
static void DummyCDDeInit(void);
static int DummyCDGetStatus(void);
static s32 DummyCDReadTOC(u32*);
static int DummyCDReadSectorFAD(u32, void*);
static void DummyCDReadAheadFAD(u32);
static void DummyCDSetStatus(int status);

CDInterface DummyCD = {
    CDCORE_DUMMY,
    "Dummy CD Drive",
    DummyCDInit,
    DummyCDDeInit,
    DummyCDGetStatus,
    DummyCDReadTOC,
    DummyCDReadSectorFAD,
    DummyCDReadAheadFAD,
    DummyCDSetStatus,
};

static int dmy_status = 2;
//////////////////////////////////////////////////////////////////////////////
// Dummy Interface
//////////////////////////////////////////////////////////////////////////////

static int DummyCDInit(UNUSED const char* cdrom_name)
{
	// Initialization function. cdrom_name can be whatever you want it to
	// be. Obviously with some ports(e.g. the dreamcast port) you probably
	// won't even use it.
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

static void DummyCDDeInit(void)
{
	// Cleanup function. Enough said.
}

//////////////////////////////////////////////////////////////////////////////

static int DummyCDGetStatus(void)
{
	// This function is called periodically to see what the status of the
	// drive is.
	//
	// Should return one of the following values:
	// 0 - CD Present, disc spinning
	// 1 - CD Present, disc not spinning
	// 2 - CD not present
	// 3 - Tray open
	//
	// If you really don't want to bother too much with this function, just
	// return status 0. Though it is kind of nice when the bios's cd
	// player, etc. recognizes when you've ejected the tray and popped in
	// another disc.

	return dmy_status;
}

static void DummyCDSetStatus(int status) {
	dmy_status = status;
	if (dmy_status != 3) {
		dmy_status = 2;
	}
	return;
}

//////////////////////////////////////////////////////////////////////////////

static s32 DummyCDReadTOC(UNUSED u32* TOC)
{
	// The format of TOC is as follows:
	// TOC[0] - TOC[98] are meant for tracks 1-99. Each entry has the
	// following format:
	// bits 0 - 23: track FAD address
	// bits 24 - 27: track addr
	// bits 28 - 31: track ctrl
	//
	// Any Unused tracks should be set to 0xFFFFFFFF
	//
	// TOC[99] - Point A0 information
	// Uses the following format:
	// bits 0 - 7: PFRAME(should always be 0)
	// bits 7 - 15: PSEC(Program area format: 0x00 - CDDA or CDROM,
	//                   0x10 - CDI, 0x20 - CDROM-XA)
	// bits 16 - 23: PMIN(first track's number)
	// bits 24 - 27: first track's addr
	// bits 28 - 31: first track's ctrl
	//
	// TOC[100] - Point A1 information
	// Uses the following format:
	// bits 0 - 7: PFRAME(should always be 0)
	// bits 7 - 15: PSEC(should always be 0)
	// bits 16 - 23: PMIN(last track's number)
	// bits 24 - 27: last track's addr
	// bits 28 - 31: last track's ctrl
	//
	// TOC[101] - Point A2 information
	// Uses the following format:
	// bits 0 - 23: leadout FAD address
	// bits 24 - 27: leadout's addr
	// bits 28 - 31: leadout's ctrl
	//
	// Special Note: To convert from LBA/LSN to FAD, add 150.

	return 0;
}

//////////////////////////////////////////////////////////////////////////////

static int DummyCDReadSectorFAD(UNUSED u32 FAD, void* buffer)
{
	// This function is supposed to read exactly 1 -RAW- 2352-byte sector
	// at the specified FAD address to buffer. Should return true if
	// successful, false if there was an error.
	//
	// Special Note: To convert from FAD to LBA/LSN, minus 150.
	//
	// The whole process needed to be changed since I need more control
	// over sector detection, etc. Not to mention it means less work for
	// the porter since they only have to implement raw sector reading as
	// opposed to implementing mode 1, mode 2 form1/form2, -and- raw
	// sector reading.

	memset(buffer, 0, 2352);

	return 1;
}

//////////////////////////////////////////////////////////////////////////////

static void DummyCDReadAheadFAD(UNUSED u32 FAD)
{
	// This function is called to tell the driver which sector (FAD
	// address) is expected to be read next. If the driver supports
	// read-ahead, it should start reading the given sector in the
	// background while the emulation continues, so that when the
	// sector is actually read with ReadSectorFAD() it'll be available
	// immediately. (Note that there's no guarantee this sector will
	// actually be requested--the emulated CD might be stopped before
	// the sector is read, for example.)
	//
	// This function should NOT block. If the driver can't perform
	// asynchronous reads (or you just don't want to bother handling
	// them), make this function a no-op and just read sectors
	// normally.
}
