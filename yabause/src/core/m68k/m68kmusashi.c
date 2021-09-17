/*  Copyright 2007 Guillaume Duhamel

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

/*! \file m68kmusashi.c
\brief Musashi 68000 interface.
*/

#include "m68kmusashi.h"
#include "musashi/m68k.h"
#include "m68kcore.h"
#include "musashi/m68kcpu.h"

struct ReadWriteFuncs
{
   M68K_READ  *r_8;
   M68K_READ  *r_16;
   M68K_WRITE *w_8;
   M68K_WRITE *w_16;
}rw_funcs;

static int M68KMusashiInit(void) {

   m68k_init();
   m68k_set_reset_instr_callback(m68k_pulse_reset);
   m68k_set_cpu_type(M68K_CPU_TYPE_68000);

   return 0;
}

static void M68KMusashiDeInit(void) {
}

static void M68KMusashiReset(void) {
   m68k_pulse_reset();
}

static s32 FASTCALL M68KMusashiExec(s32 cycle) {
   return m68k_execute(cycle);
}

static void M68KMusashiSync(void) {
}

static u32 M68KMusashiGetDReg(u32 num) {
   return (u32)m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + num));
}

static u32 M68KMusashiGetAReg(u32 num) {
   return (u32)m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_A0 + num));
}

static u32 M68KMusashiGetPC(void) {
   return (u32)m68k_get_reg(NULL, (m68k_register_t)M68K_REG_PC);
}

static u32 M68KMusashiGetSR(void) {
   return (u32)m68k_get_reg(NULL, (m68k_register_t)M68K_REG_SR);
}

static u32 M68KMusashiGetUSP(void) {
   return (u32)m68k_get_reg(NULL, (m68k_register_t)M68K_REG_USP);
}

static u32 M68KMusashiGetMSP(void) {
   return (u32)m68k_get_reg(NULL, (m68k_register_t)M68K_REG_MSP);
}

static void M68KMusashiSetDReg(u32 num, u32 val) {
   m68k_set_reg((m68k_register_t)(M68K_REG_D0 + num), val);
}

static void M68KMusashiSetAReg(u32 num, u32 val) {
   m68k_set_reg((m68k_register_t)(M68K_REG_A0 + num), val);
}

static void M68KMusashiSetPC(u32 val) {
   m68k_set_reg((m68k_register_t)M68K_REG_PC, val);
}

static void M68KMusashiSetSR(u32 val) {
   m68k_set_reg((m68k_register_t)M68K_REG_SR, val);
}

static void M68KMusashiSetUSP(u32 val) {
   m68k_set_reg((m68k_register_t)M68K_REG_USP, val);
}

static void M68KMusashiSetMSP(u32 val) {
   m68k_set_reg((m68k_register_t)M68K_REG_MSP, val);
}

static void M68KMusashiSetFetch(u32 low_adr, u32 high_adr, pointer fetch_adr) {
}

static void FASTCALL M68KMusashiSetIRQ(s32 level) {
   if (level > 0)
      m68k_set_irq(level);
}

static void FASTCALL M68KMusashiWriteNotify(u32 address, u32 size) {
}

unsigned int  m68k_read_memory_8(unsigned int address)
{
   return rw_funcs.r_8(address);
}

unsigned int  m68k_read_memory_16(unsigned int address)
{
   return rw_funcs.r_16(address);
}

unsigned int m68k_read_disassembler_16(unsigned int address)
{
	return m68k_read_memory_16(address);
}

unsigned int  m68k_read_memory_32(unsigned int address)
{
   u16 const val1 = (u16)rw_funcs.r_16(address);
   return (((u32)val1 << 16) | rw_funcs.r_16(address + 2));
}

unsigned int m68k_read_disassembler_32(unsigned int address)
{
	return m68k_read_memory_32(address);
}

void m68k_write_memory_8(unsigned int address, unsigned int value)
{
   rw_funcs.w_8(address, value);
}

void m68k_write_memory_16(unsigned int address, unsigned int value)
{
   rw_funcs.w_16(address, value);
}

void m68k_write_memory_32(unsigned int address, unsigned int value)
{
   rw_funcs.w_16(address, value >> 16 );
   rw_funcs.w_16(address + 2, value & 0xffff);
}

static void M68KMusashiSetReadB(M68K_READ *Func) {
   rw_funcs.r_8 = Func;
}

static void M68KMusashiSetReadW(M68K_READ *Func) {
   rw_funcs.r_16 = Func;
}

static void M68KMusashiSetWriteB(M68K_WRITE *Func) {
   rw_funcs.w_8 = Func;
}

static void M68KMusashiSetWriteW(M68K_WRITE *Func) {
   rw_funcs.w_16 = Func;
}

static struct {
	uint16 sr;
	int stopped;
	int halted;
} m68k_substate;

void m68k_save_context(void ** stream){
	int i;
	uint32 regd[8];
	uint32 rega[8];
	uint32 val;

	m68k_substate.sr = m68ki_get_sr();
	m68k_substate.stopped = (CPU_STOPPED & STOP_LEVEL_STOP) != 0;
	m68k_substate.halted  = (CPU_STOPPED & STOP_LEVEL_HALT) != 0;

	for (i = 0; i<8; i++) {
		regd[i]=m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + i));
	}
	MemStateWrite((void *)&regd, sizeof(uint32), 8, stream );
	for (i = 0; i<8; i++) {
		rega[i]=m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_A0 + i));
	}
	MemStateWrite((void *)&rega, sizeof(uint32), 8, stream );

	val = m68k_get_reg(NULL, M68K_REG_PPC);
	MemStateWrite((void *)&val, sizeof(uint32), 1, stream );

	val = m68k_get_reg(NULL, M68K_REG_PC);
	MemStateWrite((void *)&val, sizeof(uint32), 1, stream );

	val = m68k_get_reg(NULL, M68K_REG_USP);
	MemStateWrite((void *)&val, sizeof(uint32), 1, stream );

	val = m68k_get_reg(NULL, M68K_REG_ISP);
	MemStateWrite((void *)&val, sizeof(uint32), 1, stream );

	val = m68k_get_reg(NULL, M68K_REG_MSP);
	MemStateWrite((void *)&val, sizeof(uint32), 1, stream );

	val = m68k_get_reg(NULL, M68K_REG_VBR);
	MemStateWrite((void *)&val, sizeof(uint32), 1, stream );

	val = m68k_get_reg(NULL, M68K_REG_SFC);
	MemStateWrite((void *)&val, sizeof(uint32), 1, stream );

	val = m68k_get_reg(NULL, M68K_REG_DFC);
	MemStateWrite((void *)&val, sizeof(uint32), 1, stream );

	val = m68k_get_reg(NULL, M68K_REG_CACR);
	MemStateWrite((void *)&val, sizeof(uint32), 1, stream );

	val = m68k_get_reg(NULL, M68K_REG_CAAR);
	MemStateWrite((void *)&val, sizeof(uint32), 1, stream );

	val = m68k_get_reg(NULL, M68K_REG_SR);
	MemStateWrite((void *)&m68k_substate.sr, sizeof(uint16), 1, stream );

	MemStateWrite((void *)&CPU_INT_LEVEL, sizeof(uint32), 1, stream );

	MemStateWrite((void *)&CPU_INT_CYCLES, sizeof(uint32), 1, stream );

	MemStateWrite((void *)&m68k_substate.stopped, sizeof(int), 1, stream );
	MemStateWrite((void *)&m68k_substate.halted, sizeof(int), 1, stream );
	MemStateWrite((void *)&CPU_PREF_ADDR, sizeof(uint32), 1, stream );
	MemStateWrite((void *)&CPU_PREF_DATA, sizeof(uint32), 1, stream );

	#define FLAG_T1          m68ki_cpu.t1_flag
	#define FLAG_T0          m68ki_cpu.t0_flag
	#define FLAG_S           m68ki_cpu.s_flag
	#define FLAG_M           m68ki_cpu.m_flag
	#define FLAG_X           m68ki_cpu.x_flag
	#define FLAG_N           m68ki_cpu.n_flag
	#define FLAG_Z           m68ki_cpu.not_z_flag
	#define FLAG_V           m68ki_cpu.v_flag
	#define FLAG_C           m68ki_cpu.c_flag
	#define FLAG_INT_MASK    m68ki_cpu.int_mask

	#define CPU_INT_LEVEL    m68ki_cpu.int_level /* ASG: changed from CPU_INTS_PENDING */
	#define CPU_INT_CYCLES   m68ki_cpu.int_cycles /* ASG */
	#define CPU_STOPPED      m68ki_cpu.stopped
	#define CPU_PREF_ADDR    m68ki_cpu.pref_addr
	#define CPU_PREF_DATA    m68ki_cpu.pref_data
	#define CPU_ADDRESS_MASK m68ki_cpu.address_mask
	#define CPU_SR_MASK      m68ki_cpu.sr_mask
	#define CPU_INSTR_MODE   m68ki_cpu.instr_mode
	#define CPU_RUN_MODE     m68ki_cpu.run_mode

	#define CYC_INSTRUCTION  m68ki_cpu.cyc_instruction
	#define CYC_EXCEPTION    m68ki_cpu.cyc_exception
	#define CYC_BCC_NOTAKE_B m68ki_cpu.cyc_bcc_notake_b
	#define CYC_BCC_NOTAKE_W m68ki_cpu.cyc_bcc_notake_w
	#define CYC_DBCC_F_NOEXP m68ki_cpu.cyc_dbcc_f_noexp
	#define CYC_DBCC_F_EXP   m68ki_cpu.cyc_dbcc_f_exp
	#define CYC_SCC_R_TRUE   m68ki_cpu.cyc_scc_r_true
	#define CYC_MOVEM_W      m68ki_cpu.cyc_movem_w
	#define CYC_MOVEM_L      m68ki_cpu.cyc_movem_l
	#define CYC_SHIFT        m68ki_cpu.cyc_shift
	#define CYC_RESET        m68ki_cpu.cyc_reset
}

void m68k_load_context(const void * stream){
	int i;
	uint32 regd[8];
	uint32 rega[8];
	uint32 val;

	MemStateRead((void *)&regd, sizeof(uint32), 8, stream );
	for (i = 0; i<8; i++) {
		m68k_set_reg((m68k_register_t)(M68K_REG_D0 + i), regd[i]);
	}

	MemStateRead((void *)&rega, sizeof(uint32), 8, stream );
	for (i = 0; i<8; i++) {
		m68k_set_reg((m68k_register_t)(M68K_REG_A0 + i), rega[i]);
	}

	MemStateRead((void *)&val, sizeof(uint32), 1, stream );
	m68k_set_reg((m68k_register_t)M68K_REG_PPC, val);

	MemStateRead((void *)&val, sizeof(uint32), 1, stream );
	m68k_set_reg((m68k_register_t)M68K_REG_PC, val);

	MemStateRead((void *)&val, sizeof(uint32), 1, stream );
	m68k_set_reg((m68k_register_t)M68K_REG_USP, val);

	MemStateRead((void *)&val, sizeof(uint32), 1, stream );
	m68k_set_reg((m68k_register_t)M68K_REG_ISP, val);

	MemStateRead((void *)&val, sizeof(uint32), 1, stream );
	m68k_set_reg((m68k_register_t)M68K_REG_MSP, val);

	MemStateRead((void *)&val, sizeof(uint32), 1, stream );
	m68k_set_reg((m68k_register_t)M68K_REG_VBR, val);

	MemStateRead((void *)&val, sizeof(uint32), 1, stream );
	m68k_set_reg((m68k_register_t)M68K_REG_SFC, val);

	MemStateRead((void *)&val, sizeof(uint32), 1, stream );
	m68k_set_reg((m68k_register_t)M68K_REG_DFC, val);

	MemStateRead((void *)&val, sizeof(uint32), 1, stream );
	m68k_set_reg((m68k_register_t)M68K_REG_CACR, val);

	MemStateRead((void *)&val, sizeof(uint32), 1, stream );
	m68k_set_reg((m68k_register_t)M68K_REG_CAAR, val);

	MemStateRead((void *)&m68k_substate.sr, sizeof(uint16), 1, stream );
	m68ki_set_sr_noint_nosp(val);

	MemStateRead((void *)&CPU_INT_LEVEL, sizeof(uint32), 1, stream );

	MemStateRead((void *)&CPU_INT_CYCLES, sizeof(uint32), 1, stream );

	MemStateRead((void *)&m68k_substate.stopped, sizeof(int), 1, stream );
	MemStateRead((void *)&m68k_substate.halted, sizeof(int), 1, stream );
	MemStateRead((void *)&CPU_PREF_ADDR, sizeof(uint32), 1, stream );
	MemStateRead((void *)&CPU_PREF_DATA, sizeof(uint32), 1, stream );

	m68ki_set_sr_noint_nosp(m68k_substate.sr);
	CPU_STOPPED = m68k_substate.stopped ? STOP_LEVEL_STOP : 0
						| m68k_substate.halted  ? STOP_LEVEL_HALT : 0;
	m68ki_jump(REG_PC);
}

static void M68KMusashiSaveState(void ** stream) {

  m68k_save_context(stream);

}

static void M68KMusashiLoadState(const void * stream) {

  m68k_load_context(stream);
}

M68K_struct M68KMusashi = {
   3,
   "Musashi Interface",
   M68KMusashiInit,
   M68KMusashiDeInit,
   M68KMusashiReset,
   M68KMusashiExec,
   M68KMusashiSync,
   M68KMusashiGetDReg,
   M68KMusashiGetAReg,
   M68KMusashiGetPC,
   M68KMusashiGetSR,
   M68KMusashiGetUSP,
   M68KMusashiGetMSP,
   M68KMusashiSetDReg,
   M68KMusashiSetAReg,
   M68KMusashiSetPC,
   M68KMusashiSetSR,
   M68KMusashiSetUSP,
   M68KMusashiSetMSP,
   M68KMusashiSetFetch,
   M68KMusashiSetIRQ,
   M68KMusashiWriteNotify,
   M68KMusashiSetReadB,
   M68KMusashiSetReadW,
   M68KMusashiSetWriteB,
   M68KMusashiSetWriteW,
   M68KMusashiSaveState,
   M68KMusashiLoadState
};
