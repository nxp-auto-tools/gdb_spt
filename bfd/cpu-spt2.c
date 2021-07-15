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

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

#define N(BITS_WORD, BITS_ADDR, NUMBER, PRINT, DEFAULT, NEXT) \
  {							\
    BITS_WORD, /* bits in a word */			\
    BITS_ADDR, /* bits in an address */			\
    8,	/* 8 bits in a byte */				\
    bfd_arch_spt2,					\
    NUMBER,						\
    "spt2",						\
    PRINT,						\
    3,							\
    DEFAULT,						\
    bfd_default_compatible, 				\
    bfd_default_scan,					\
    bfd_arch_default_fill,				\
    NEXT,						\
  }

#define NN(index) (&arch_info_struct[index])

/* These exist only so that we can reasonably disassemble PALcode.  */
static const bfd_arch_info_type arch_info_struct[] =
{
  N (32, 32, bfd_mach_spt2, "spt2_32", FALSE, 0),
};

const bfd_arch_info_type bfd_spt2_arch =
  N (32, 32, 0, "spt2", TRUE, NN(0));
