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
