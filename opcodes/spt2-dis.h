/* Copyright (C) 1992-2021 Free Software Foundation, Inc.
   Contributed by NXP, SPT support.

   This file is part of BFD, the Binary File Descriptor library.

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
