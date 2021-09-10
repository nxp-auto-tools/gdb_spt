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

#include "sysdep.h"
#include <ansidecl.h> //for ATTRIBUTE_UNUSED
#include "dis-asm.h"
#include "apex-opc.h"
#include "elf-bfd.h"

int
print_insn_apex(bfd_vma pc  ATTRIBUTE_UNUSED, disassemble_info *info  ATTRIBUTE_UNUSED){

	return 0;
}
