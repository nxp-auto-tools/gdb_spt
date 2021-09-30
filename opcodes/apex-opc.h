/*
 * Copyright 2018-2021 NXP
 *
 * SPDX-License-Identifier: BSD-3-clause
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OPCODE_APEX_H
#define OPCODE_APEX_H

typedef struct apex_opc_info_t
{
  const char *name;
  unsigned char instr_t; // instruction type (Scalar(0x0), Vector(0x1), Combined(0x2))
  unsigned long opcode;
  unsigned long operands; //operands positions
} apex_opc_info_t;

/* Bit-masks for instruction members separation use "&&" */
/*#define INST_TYPE			0xC0000000;
#define INST_SHORT_OPCODE	0x3E000000;
#define INST_LONG_OPCODE	0x3E0000FF;*/

#define OPERAND_FIRST			0x01F00000U //[20:24] bits
#define OPERAND_SECOND			0x000F8000U //[15:19] bits
#define OPERAND_THIRD			0x00007C00U //[10:14] bits
#define OPERAND_THIRD_EXT_1		0x00007E00U // [9:14] bits
#define OPERAND_FOURTH			0x000003E0U //  [5:9] bits
#define OPERAND_C				0x00007FFFU // [0:14] bits
#define OPERAND_IMM				0x0000FFFFU // [0:15] bits
#define OPERAND_LARGE_IMM		0x01FFFFFFU // [0:24] bits
#define OPERAND_I1				0x01FFE000U //[13:24] bits
#define OPERAND_I2				0x00001FFFU	// [0:12] bits

#define SHIFT_LEFT(v, p) ((v)<<(p)) //v - value; p - number of positions to shift
#define SHIFT_RIGHT(v, p) ((v)>>(p))




#endif // OPCODE_APEX_H
