#include "scu_debug.h"
#include "scu.h"
#include "sh2core.h"

extern scudspregs_struct* ScuDsp;
scubp_struct ScuBP;

void ScuDspStep(void) {
    if (ScuDsp)
        ScuExec(1);
}

void ScuDspSetBreakpointCallBack(void (*func)(u32)) {
    ScuBP.BreakpointCallBack = func;
}

int ScuDspAddCodeBreakpoint(u32 addr) {
    int i;

    if (ScuBP.numcodebreakpoints < MAX_BREAKPOINTS) {
        // Make sure it isn't already on the list
        for (i = 0; i < ScuBP.numcodebreakpoints; i++)
        {
            if (addr == ScuBP.codebreakpoint[i].addr)
                return -1;
        }

        ScuBP.codebreakpoint[ScuBP.numcodebreakpoints].addr = addr;
        ScuBP.numcodebreakpoints++;

        return 0;
    }

    return -1;
}

static void ScuDspSortCodeBreakpoints(void) {
    int i, i2;
    u32 tmp;

    for (i = 0; i < (MAX_BREAKPOINTS - 1); i++)
    {
        for (i2 = i + 1; i2 < MAX_BREAKPOINTS; i2++)
        {
            if (ScuBP.codebreakpoint[i].addr == 0xFFFFFFFF &&
                ScuBP.codebreakpoint[i2].addr != 0xFFFFFFFF)
            {
                tmp = ScuBP.codebreakpoint[i].addr;
                ScuBP.codebreakpoint[i].addr = ScuBP.codebreakpoint[i2].addr;
                ScuBP.codebreakpoint[i2].addr = tmp;
            }
        }
    }
}

int ScuDspDelCodeBreakpoint(u32 addr) {
    int i;

    if (ScuBP.numcodebreakpoints > 0) {
        for (i = 0; i < ScuBP.numcodebreakpoints; i++) {
            if (ScuBP.codebreakpoint[i].addr == addr)
            {
                ScuBP.codebreakpoint[i].addr = 0xFFFFFFFF;
                ScuDspSortCodeBreakpoints();
                ScuBP.numcodebreakpoints--;
                return 0;
            }
        }
    }

    return -1;
}

scucodebreakpoint_struct* ScuDspGetBreakpointList(void) {
    return ScuBP.codebreakpoint;
}

void ScuDspClearCodeBreakpoints(void) {
    int i;
    for (i = 0; i < MAX_BREAKPOINTS; i++)
        ScuBP.codebreakpoint[i].addr = 0xFFFFFFFF;

    ScuBP.numcodebreakpoints = 0;
}

void ScuCheckBreakpoints()
{
    for (int i = 0; i < ScuBP.numcodebreakpoints; i++) {
        if ((ScuDsp->PC == ScuBP.codebreakpoint[i].addr) && ScuBP.inbreakpoint == 0) {
            ScuBP.inbreakpoint = 1;
            if (ScuBP.BreakpointCallBack) ScuBP.BreakpointCallBack(ScuBP.codebreakpoint[i].addr);
            ScuBP.inbreakpoint = 0;
        }
    }
}

static char const* const disd1bussrc(u8 num)
{
    switch (num) {
    case 0x0:
        return "M0";
    case 0x1:
        return "M1";
    case 0x2:
        return "M2";
    case 0x3:
        return "M3";
    case 0x4:
        return "MC0";
    case 0x5:
        return "MC1";
    case 0x6:
        return "MC2";
    case 0x7:
        return "MC3";
    case 0x9:
        return "ALL";
    case 0xA:
        return "ALH";
    default: break;
    }

    return "??";
}

//////////////////////////////////////////////////////////////////////////////

static char const* const disd1busdest(u8 num)
{
    switch (num) {
    case 0x0:
        return "MC0";
    case 0x1:
        return "MC1";
    case 0x2:
        return "MC2";
    case 0x3:
        return "MC3";
    case 0x4:
        return "RX";
    case 0x5:
        return "PL";
    case 0x6:
        return "RA0";
    case 0x7:
        return "WA0";
    case 0xA:
        return "LOP";
    case 0xB:
        return "TOP";
    case 0xC:
        return "CT0";
    case 0xD:
        return "CT1";
    case 0xE:
        return "CT2";
    case 0xF:
        return "CT3";
    default: break;
    }

    return "??";
}

//////////////////////////////////////////////////////////////////////////////

static char const* const disloadimdest(u8 num)
{
    switch (num) {
    case 0x0:
        return "MC0";
    case 0x1:
        return "MC1";
    case 0x2:
        return "MC2";
    case 0x3:
        return "MC3";
    case 0x4:
        return "RX";
    case 0x5:
        return "PL";
    case 0x6:
        return "RA0";
    case 0x7:
        return "WA0";
    case 0xA:
        return "LOP";
    case 0xC:
        return "PC";
    default: break;
    }

    return "??";
}

//////////////////////////////////////////////////////////////////////////////

static char const* const disdmaram(u8 num)
{
    switch (num)
    {
    case 0x0: // MC0
        return "MC0";
    case 0x1: // MC1
        return "MC1";
    case 0x2: // MC2
        return "MC2";
    case 0x3: // MC3
        return "MC3";
    case 0x4: // Program Ram
        return "PRG";
    default: break;
    }

    return "??";
}

//////////////////////////////////////////////////////////////////////////////

void ScuDspDisasm(u8 addr, char* outstring) {
    u32 instruction;
    u8 counter = 0;
    u8 filllength = 0;

    instruction = ScuDsp->ProgramRam[addr];

    sprintf(outstring, "%02X: ", addr);
    outstring += strlen(outstring);

    if (instruction == 0)
    {
        sprintf(outstring, "NOP");
        return;
    }

    // Handle ALU commands
    switch (instruction >> 26)
    {
    case 0x0: // NOP
        break;
    case 0x1: // AND
        sprintf(outstring, "AND");
        counter = (u8)strlen(outstring);
        outstring += (u8)strlen(outstring);
        break;
    case 0x2: // OR
        sprintf(outstring, "OR");
        counter = (u8)strlen(outstring);
        outstring += (u8)strlen(outstring);
        break;
    case 0x3: // XOR
        sprintf(outstring, "XOR");
        counter = (u8)strlen(outstring);
        outstring += (u8)strlen(outstring);
        break;
    case 0x4: // ADD
        sprintf(outstring, "ADD");
        counter = (u8)strlen(outstring);
        outstring += (u8)strlen(outstring);
        break;
    case 0x5: // SUB
        sprintf(outstring, "SUB");
        counter = (u8)strlen(outstring);
        outstring += (u8)strlen(outstring);
        break;
    case 0x6: // AD2
        sprintf(outstring, "AD2");
        counter = (u8)strlen(outstring);
        outstring += (u8)strlen(outstring);
        break;
    case 0x8: // SR
        sprintf(outstring, "SR");
        counter = (u8)strlen(outstring);
        outstring += (u8)strlen(outstring);
        break;
    case 0x9: // RR
        sprintf(outstring, "RR");
        counter = (u8)strlen(outstring);
        outstring += (u8)strlen(outstring);
        break;
    case 0xA: // SL
        sprintf(outstring, "SL");
        counter = (u8)strlen(outstring);
        outstring += (u8)strlen(outstring);
        break;
    case 0xB: // RL
        sprintf(outstring, "RL");
        counter = (u8)strlen(outstring);
        outstring += (u8)strlen(outstring);
        break;
    case 0xF: // RL8
        sprintf(outstring, "RL8");
        counter = (u8)strlen(outstring);
        outstring += (u8)strlen(outstring);
        break;
    default: break;
    }

    switch (instruction >> 30) {
    case 0x00: // Operation Commands
        filllength = 5 - counter;
        memset((void*)outstring, 0x20, filllength);
        counter += filllength;
        outstring += filllength;

        if ((instruction >> 23) & 0x4)
        {
            sprintf(outstring, "MOV %s, X", disd1bussrc((instruction >> 20) & 0x7));
            counter += (u8)strlen(outstring);
            outstring += (u8)strlen(outstring);
        }

        filllength = 16 - counter;
        memset((void*)outstring, 0x20, filllength);
        counter += filllength;
        outstring += filllength;

        switch ((instruction >> 23) & 0x3)
        {
        case 2:
            sprintf(outstring, "MOV MUL, P");
            counter += (u8)strlen(outstring);
            outstring += (u8)strlen(outstring);
            break;
        case 3:
            sprintf(outstring, "MOV %s, P", disd1bussrc((instruction >> 20) & 0x7));
            counter += (u8)strlen(outstring);
            outstring += (u8)strlen(outstring);
            break;
        default: break;
        }

        filllength = 27 - counter;
        memset((void*)outstring, 0x20, filllength);
        counter += filllength;
        outstring += filllength;

        // Y-bus
        if ((instruction >> 17) & 0x4)
        {
            sprintf(outstring, "MOV %s, Y", disd1bussrc((instruction >> 14) & 0x7));
            counter += (u8)strlen(outstring);
            outstring += (u8)strlen(outstring);
        }

        filllength = 38 - counter;
        memset((void*)outstring, 0x20, filllength);
        counter += filllength;
        outstring += filllength;

        switch ((instruction >> 17) & 0x3)
        {
        case 1:
            sprintf(outstring, "CLR A");
            counter += (u8)strlen(outstring);
            outstring += (u8)strlen(outstring);
            break;
        case 2:
            sprintf(outstring, "MOV ALU, A");
            counter += (u8)strlen(outstring);
            outstring += (u8)strlen(outstring);
            break;
        case 3:
            sprintf(outstring, "MOV %s, A", disd1bussrc((instruction >> 14) & 0x7));
            counter += (u8)strlen(outstring);
            outstring += (u8)strlen(outstring);
            break;
        default: break;
        }

        filllength = 50 - counter;
        memset((void*)outstring, 0x20, filllength);
        counter += filllength;
        outstring += filllength;

        // D1-bus
        switch ((instruction >> 12) & 0x3)
        {
        case 1:
            sprintf(outstring, "MOV #$%02X, %s", (unsigned int)instruction & 0xFF, disd1busdest((instruction >> 8) & 0xF));
            outstring += (u8)strlen(outstring);
            break;
        case 3:
            sprintf(outstring, "MOV %s, %s", disd1bussrc(instruction & 0xF), disd1busdest((instruction >> 8) & 0xF));
            outstring += (u8)strlen(outstring);
            break;
        default:
            outstring[0] = 0x00;
            break;
        }

        break;
    case 0x02: // Load Immediate Commands
        if ((instruction >> 25) & 1)
        {
            switch ((instruction >> 19) & 0x3F) {
            case 0x01:
                sprintf(outstring, "MVI #$%05X,%s,NZ", (unsigned int)instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                break;
            case 0x02:
                sprintf(outstring, "MVI #$%05X,%s,NS", (unsigned int)instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                break;
            case 0x03:
                sprintf(outstring, "MVI #$%05X,%s,NZS", (unsigned int)instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                break;
            case 0x04:
                sprintf(outstring, "MVI #$%05X,%s,NC", (unsigned int)instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                break;
            case 0x08:
                sprintf(outstring, "MVI #$%05X,%s,NT0", (unsigned int)instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                break;
            case 0x21:
                sprintf(outstring, "MVI #$%05X,%s,Z", (unsigned int)instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                break;
            case 0x22:
                sprintf(outstring, "MVI #$%05X,%s,S", (unsigned int)instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                break;
            case 0x23:
                sprintf(outstring, "MVI #$%05X,%s,ZS", (unsigned int)instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                break;
            case 0x24:
                sprintf(outstring, "MVI #$%05X,%s,C", (unsigned int)instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                break;
            case 0x28:
                sprintf(outstring, "MVI #$%05X,%s,T0", (unsigned int)instruction & 0x7FFFF, disloadimdest((instruction >> 26) & 0xF));
                break;
            default: break;
            }
        }
        else
        {
            //sprintf(outstring, "MVI #$%08X,%s", (instruction & 0xFFFFFF) | ((instruction & 0x1000000) ? 0xFF000000 : 0x00000000), disloadimdest((instruction >> 26) & 0xF));
            sprintf(outstring, "MVI #$%08X,%s", (instruction & 0x1FFFFFF) << 2, disloadimdest((instruction >> 26) & 0xF));
        }

        break;
    case 0x03: // Other
        switch ((instruction >> 28) & 0x3) {
        case 0x00: // DMA Commands
        {
            int addressAdd;

            if (instruction & 0x1000)
                addressAdd = (instruction >> 15) & 0x7;
            else
                addressAdd = (instruction >> 15) & 0x1;

            switch (addressAdd)
            {
            case 0: // Add 0
                addressAdd = 0;
                break;
            case 1: // Add 1
                addressAdd = 1;
                break;
            case 2: // Add 2
                addressAdd = 2;
                break;
            case 3: // Add 4
                addressAdd = 4;
                break;
            case 4: // Add 8
                addressAdd = 8;
                break;
            case 5: // Add 16
                addressAdd = 16;
                break;
            case 6: // Add 32
                addressAdd = 32;
                break;
            case 7: // Add 64
                addressAdd = 64;
                break;
            default:
                addressAdd = 0;
                break;
            }

            // Write Command name
            sprintf(outstring, "DMA");
            outstring += (u8)strlen(outstring);

            // Is h bit set?
            if (instruction & 0x4000)
            {
                outstring[0] = 'H';
                outstring++;
            }

            sprintf(outstring, "%d ", addressAdd);
            outstring += (u8)strlen(outstring);

            if (instruction & 0x2000)
            {
                // Command Format 2
                if (instruction & 0x1000)
                    sprintf(outstring, "%s, D0, %s", disdmaram((instruction >> 8) & 0x7), disd1bussrc(instruction & 0x7));
                else
                    sprintf(outstring, "D0, %s, %s", disdmaram((instruction >> 8) & 0x7), disd1bussrc(instruction & 0x7));
            }
            else
            {
                // Command Format 1
                if (instruction & 0x1000)
                    sprintf(outstring, "%s, D0, #$%02X", disdmaram((instruction >> 8) & 0x7), (int)(instruction & 0xFF));
                else
                    sprintf(outstring, "D0, %s, #$%02X", disdmaram((instruction >> 8) & 0x7), (int)(instruction & 0xFF));
            }

            break;
        }
        case 0x01: // Jump Commands
            switch ((instruction >> 19) & 0x7F) {
            case 0x00:
                sprintf(outstring, "JMP $%02X", (unsigned int)instruction & 0xFF);
                break;
            case 0x41:
                sprintf(outstring, "JMP NZ,$%02X", (unsigned int)instruction & 0xFF);
                break;
            case 0x42:
                sprintf(outstring, "JMP NS,$%02X", (unsigned int)instruction & 0xFF);
                break;
            case 0x43:
                sprintf(outstring, "JMP NZS,$%02X", (unsigned int)instruction & 0xFF);
                break;
            case 0x44:
                sprintf(outstring, "JMP NC,$%02X", (unsigned int)instruction & 0xFF);
                break;
            case 0x48:
                sprintf(outstring, "JMP NT0,$%02X", (unsigned int)instruction & 0xFF);
                break;
            case 0x61:
                sprintf(outstring, "JMP Z,$%02X", (unsigned int)instruction & 0xFF);
                break;
            case 0x62:
                sprintf(outstring, "JMP S,$%02X", (unsigned int)instruction & 0xFF);
                break;
            case 0x63:
                sprintf(outstring, "JMP ZS,$%02X", (unsigned int)instruction & 0xFF);
                break;
            case 0x64:
                sprintf(outstring, "JMP C,$%02X", (unsigned int)instruction & 0xFF);
                break;
            case 0x68:
                sprintf(outstring, "JMP T0,$%02X", (unsigned int)instruction & 0xFF);
                break;
            default:
                sprintf(outstring, "Unknown JMP");
                break;
            }
            break;
        case 0x02: // Loop bottom Commands
            if (instruction & 0x8000000)
                sprintf(outstring, "LPS");
            else
                sprintf(outstring, "BTM");

            break;
        case 0x03: // End Commands
            if (instruction & 0x8000000)
                sprintf(outstring, "ENDI");
            else
                sprintf(outstring, "END");

            break;
        default: break;
        }
        break;
    default:
        sprintf(outstring, "Invalid opcode");
        break;
    }
}
