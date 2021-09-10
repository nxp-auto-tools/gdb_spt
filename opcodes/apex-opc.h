/* Copyright (C) 2018-2021 Free Software Foundation, Inc.
   Contributed by NXP, APEX support.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

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
