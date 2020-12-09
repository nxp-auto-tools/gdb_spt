/*
* Copyright (c) 2020, NXP.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of NXP, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
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



#endif // _SPT_DIS_H_
