/*  Copyright 2005-2006 Theo Berkau

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

/*! \file error.c
    \brief Error handling functions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "error.h"
#include "yui.h"

int lastErrorType = 0;

int YabGetLastErrorType() { return lastErrorType; }

//////////////////////////////////////////////////////////////////////////////

static void AllocAmendPrintString(const char *string1, const char *string2)
{
   char *string;
   int length = 0;
   if (string1 != NULL) length += strlen(string1);
   if (string2 != NULL) length += strlen(string2);
   if ((string = (char *)malloc(length + 2)) == NULL)
      return;

   if ((string1 != NULL)&&(string2 != NULL)) sprintf(string, "%s%s", string1, string2);
   if ((string1 != NULL)&&(string2 == NULL)) sprintf(string, "%s", string1);
   if ((string1 == NULL)&&(string2 != NULL)) sprintf(string, "%s", string2);
   YuiErrorMsg(string);

   free(string);
}

//////////////////////////////////////////////////////////////////////////////

void YabSetError(int type, const void *extra)
{
    SH2_struct * sh;
    lastErrorType = type;
   switch (type)
   {
	   case YAB_ERR_FILENOTFOUND:
         AllocAmendPrintString(_("File not found: "), (const char*) extra);
         break;
      case YAB_ERR_MEMORYALLOC:
         YuiErrorMsg(_("Error allocating memory"));
         break;
      case YAB_ERR_FILEREAD:
         AllocAmendPrintString(_("Error reading file: "), (const char*) extra);
         break;
      case YAB_ERR_FILEWRITE:
         AllocAmendPrintString(_("Error writing file: "), (const char*) extra);
         break;
      case YAB_ERR_CANNOTINIT:
         AllocAmendPrintString(_("Cannot initialize "), (const char*) extra);
         break;
      case YAB_ERR_SH2INVALIDOPCODE:
#ifdef DMPHISTORY
        SH2DumpHistory(CurrentSH2);
#endif
		sh = (SH2_struct *)extra;
         SH2GetRegisters(sh, &sh->regs);
         char tempstr[512] = {0};
         sprintf(tempstr, "%s SH2 invalid opcode\n\n"
                          "R0 =  %08lX\tR12 =  %08lX\n"
                          "R1 =  %08lX\tR13 =  %08lX\n"
                          "R2 =  %08lX\tR14 =  %08lX\n"
                          "R3 =  %08lX\tR15 =  %08lX\n"
                          "R4 =  %08lX\tSR =   %08lX\n"
                          "R5 =  %08lX\tGBR =  %08lX\n"
                          "R6 =  %08lX\tVBR =  %08lX\n"
                          "R7 =  %08lX\tMACH = %08lX\n"
                          "R8 =  %08lX\tMACL = %08lX\n"
                          "R9 =  %08lX\tPR =   %08lX\n"
                          "R10 = %08lX\tPC =   %08lX\n"
                          "R11 = %08lX\n", sh->isslave ? "Slave" : "Master",
                          (long)sh->regs.R[0], (long)sh->regs.R[12],
                          (long)sh->regs.R[1], (long)sh->regs.R[13],
                          (long)sh->regs.R[2], (long)sh->regs.R[14],
                          (long)sh->regs.R[3], (long)sh->regs.R[15],
                          (long)sh->regs.R[4], (long)sh->regs.SR.all,
                          (long)sh->regs.R[5], (long)sh->regs.GBR,
                          (long)sh->regs.R[6], (long)sh->regs.VBR,
                          (long)sh->regs.R[7], (long)sh->regs.MACH,
                          (long)sh->regs.R[8], (long)sh->regs.MACL,
                          (long)sh->regs.R[9], (long)sh->regs.PR,
                          (long)sh->regs.R[10], (long)sh->regs.PC,
                          (long)sh->regs.R[11]);
         YuiErrorMsg(tempstr);
         break;
      case YAB_ERR_SH2READ:
         YuiErrorMsg(_("SH2 read error")); // fix me
         break;
      case YAB_ERR_SH2WRITE:
         YuiErrorMsg(_("SH2 write error")); // fix me
         break;
      case YAB_ERR_SDL:
         AllocAmendPrintString(_("SDL Error: "), (const char*) extra);
         break;
      case YAB_ERR_OTHER:
         YuiErrorMsg((const char *)extra);
         break;
      case YAB_ERR_UNKNOWN:
      default:
         YuiErrorMsg(_("Unknown error occurred\n"));
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////

void YabErrorMsg(const char * format, ...) {
    va_list l;
    int n;
    char * buffer;

    va_start(l, format);
    n = vsnprintf(NULL, 0, format, l);
    va_end(l);

    buffer = (char*)malloc(n + 1);

    va_start(l, format);
    vsprintf(buffer, format, l);
    va_end(l);

    YuiErrorMsg(buffer);
    free(buffer);
}

//////////////////////////////////////////////////////////////////////////////
