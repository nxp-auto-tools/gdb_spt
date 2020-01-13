#ifndef _SPT_DIS_H_
#define _SPT_DIS_H_

#define INSTR_LEN 8
#define LONG_INSTR_LEN 8


#define OPCODE_SET 1
#define OPCODE_GET 2
#define OPCODE_ADD 3
#define OPCODE_STOP 4
#define OPCODE_LOOP 5
#define OPCODE_NEXT 6
#define OPCODE_SYNC 7
#define OPCODE_WAIT 8
#define OPCODE_EVT 9
#define OPCODE_WATCHDOG 0x0A
#define OPCODE_SUB 0x0B
#define OPCODE_CMP 0x0C
#define OPCODE_JUMP 0x0D
#define OPCODE_SEL 0x0E

#define OPCODE_REPEAT 0x0F
#define OPCODE_THREAD 0x10

#define OPCODE_WIN 0x21
#define OPCODE_RDX4 0x22
#define OPCODE_RDX2 0x23
#define OPCODE_HIST 0x24
#define OPCODE_COPY 0x25
#define OPCODE_VMT 0x26
#define OPCODE_MAXS 0x27
#define OPCODE_PDMA 0x28
#define OPCODE_FIR 0x29
#define OPCODE_SCP 0x2A
#define OPCODE_IRDX4 0x2B
#define OPCODE_IRDX2 0x2C
//SPT3 instructions
#define OPCODE_SORT 0x2D
#define OPCODE_DSP 0x2E
#define OPCODE_WINB 0x31
#define OPCODE_RDX4B 0x32
#define OPCODE_RDX2B 0x33
#define OPCODE_COPYB 0x35
#define OPCODE_MAXSB 0x37
#define OPCODE_FIRB 0x39
#define OPCODE_SCPB 0x3A
#define OPCODE_IRDX4B 0x3B
#define OPCODE_IRDX2B 0x3C
#define OPCODE_SORTB 0x3D



#endif // _SPT_DIS_H_
