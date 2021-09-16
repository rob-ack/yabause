/*  Copyright 2015 Theo Berkau

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

#include "scspdsp.h"

s32 float_to_int(u16 f_val);
u16 int_to_float(u32 i_val);

#define sign_x_to_s32(_bits, _value) (((int)((u32)(_value) << (32 - _bits))) >> (32 - _bits))

void ScspDspExec(ScspDsp* const dsp, int const addr, u8 * const sound_ram)
{
  union ScspDspInstruction const inst = { .all = scsp_dsp.mpro[addr] };
  const unsigned TEMPReadAddr = (inst.part.tra + dsp->mdec_ct) & 0x7F;

  if (inst.part.ira & 0x20) {
    if (inst.part.ira & 0x10) {
      if (!(inst.part.ira & 0xE))
        dsp->inputs = dsp->exts[inst.part.ira & 0x1] * 256;
    }else{
      dsp->inputs = dsp->mixs[inst.part.ira & 0xF] * 16;
    }
  }else{
    dsp->inputs = dsp->mems[inst.part.ira & 0x1F];
  }

  const int INPUTS = sign_x_to_s32(24, dsp->inputs);
  const int TEMP = sign_x_to_s32(24, dsp->temp[TEMPReadAddr]);
  const int X_SEL_Inputs[2] = { TEMP, INPUTS };
  const u16 Y_SEL_Inputs[4] = {
    dsp->frc_reg, dsp->coef[inst.part.coef],
    (u16)((dsp->y_reg >> 11) & 0x1FFF),
    (u16)((dsp->y_reg >> 4) & 0x0FFF)
  };
  const u32 SGA_Inputs[2] = { (u32)TEMP, dsp->shift_reg }; // ToDO:?

  if (inst.part.yrl) {
    dsp->y_reg = INPUTS & 0xFFFFFF;
  }

  s32 ShifterOutput = sign_x_to_s32(26, dsp->shift_reg) << (inst.part.shift0 ^ inst.part.shift1);

  if (!inst.part.shift1)
  {
    if(ShifterOutput > 0x7FFFFF)
      ShifterOutput = 0x7FFFFF;
    else if(ShifterOutput < -0x800000)
      ShifterOutput = 0x800000;
  }
  ShifterOutput &= 0xFFFFFF;

  if (inst.part.ewt)
    dsp->efreg[inst.part.ewa] = (ShifterOutput >> 8);

  if (inst.part.twt)
  {
    const unsigned TEMPWriteAddr = (inst.part.twa + dsp->mdec_ct) & 0x7F;
    dsp->temp[TEMPWriteAddr] = ShifterOutput;
  }

  if (inst.part.frcl)
  {
    const unsigned F_SEL_Inputs[2] = { (unsigned)(ShifterOutput >> 11), (unsigned)(ShifterOutput & 0xFFF) };

    dsp->frc_reg = F_SEL_Inputs[inst.part.shift0 & inst.part.shift1];
    //printf("FRCL: 0x%08x\n", DSP.FRC_REG);
  }

  dsp->product = ((s64)sign_x_to_s32(13, Y_SEL_Inputs[inst.part.ysel]) * X_SEL_Inputs[inst.part.xsel]) >> 12;

  u32 SGAOutput = SGA_Inputs[inst.part.bsel];

  if (inst.part.negb)
    SGAOutput = -SGAOutput;

  if (inst.part.zero)
    SGAOutput = 0;

  dsp->shift_reg = (dsp->product + SGAOutput) & 0x3FFFFFF;
  //
  //
  if (inst.part.iwt)
  {
    dsp->mems[inst.part.iwa] = dsp->read_value;
  }

  if (dsp->read_pending)
  {
    u16 const * const sound_ram_16 = (u16 const * const)sound_ram;
    u16 const tmp = sound_ram_16[dsp->io_addr];
    dsp->read_value = (dsp->read_pending == 2) ? (tmp << 8) : float_to_int(tmp);
    dsp->read_pending = 0;
  }
  else if (dsp->write_pending)
  {
    if (!(dsp->io_addr & 0x40000))
    {
      u16* const sound_ram_16 = (u16* const)sound_ram;
      sound_ram_16[dsp->io_addr] = dsp->write_value;
    }
    dsp->write_pending = 0;
  }
  {
    u16 addr = dsp->madrs[inst.part.masa];
    addr += inst.part.nxadr;

    if (inst.part.adreb)
    {
      addr += sign_x_to_s32(12, dsp->adrs_reg);
    }

    if (!inst.part.table)
    {
      addr += dsp->mdec_ct;
      addr &= (0x2000 << dsp->rbl) - 1;
    }

    dsp->io_addr = (addr + (dsp->rbp << 12)) & 0x3FFFF;

    if (inst.part.mrd)
    {
      dsp->read_pending = 1 + inst.part.nofl;
    }
    if (inst.part.mwt)
    {
      dsp->write_pending = 1;
      dsp->write_value = inst.part.nofl ? (ShifterOutput >> 8) : int_to_float(ShifterOutput);
    }
    if (inst.part.adrl)
    {
      const u16 A_SEL_Inputs[2] = { (u16)((INPUTS >> 16) & 0xFFF), (u16)(ShifterOutput >> 12) };

      dsp->adrs_reg = A_SEL_Inputs[inst.part.shift0 & inst.part.shift1];
    }
  }
}


//sign extended to 32 bits instead of 24
s32 float_to_int(u16 f_val)
{
   u32 sign = (f_val >> 15) & 1;
   u32 sign_inverse = (!sign) & 1;
   u32 exponent = (f_val >> 11) & 0xf;
   u32 mantissa = f_val & 0x7FF;

   s32 ret_val = sign << 31;

   if (exponent > 11)
   {
      exponent = 11;
      ret_val |= (sign << 30);
   }
   else
      ret_val |= (sign_inverse << 30);

   ret_val |= mantissa << 19;

   ret_val = ret_val >> (exponent + (1 << 3));

   return ret_val;
}

u16 int_to_float(u32 i_val)
{
   u32 sign = (i_val >> 23) & 1;
   u32 exponent = 0;

   if (sign != 0)
      i_val = (~i_val) & 0x7FFFFF;

   if (i_val <= 0x1FFFF)
   {
      i_val *= 64;
      exponent += 0x3000;
   }

   if (i_val <= 0xFFFFF)
   {
      i_val *= 8;
      exponent += 0x1800;
   }

   if (i_val <= 0x3FFFFF)
   {
      i_val *= 2;
      exponent += 0x800;
   }

   if (i_val <= 0x3FFFFF)
   {
      i_val *= 2;
      exponent += 0x800;
   }

   if (i_val <= 0x3FFFFF)
      exponent += 0x800;

   i_val >>= 11;
   i_val &= 0x7ff;
   i_val |= exponent;

   if (sign != 0)
      i_val ^= (0x7ff | (1 << 15));

   return i_val;
}
