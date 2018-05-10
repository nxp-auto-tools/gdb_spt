/*
 * Copyrights
 */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

#define N(BITS_WORD, BITS_ADDR, NUMBER, PRINT, DEFAULT, NEXT) \
  {							\
    BITS_WORD, /* bits in a word */			\
    BITS_ADDR, /* bits in an address */			\
    8,	/* 8 bits in a byte */				\
    bfd_arch_spt3,					\
    NUMBER,						\
    "spt3",						\
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
  N (32, 32, bfd_mach_spt3, "spt3_64", FALSE, 0),
};

const bfd_arch_info_type bfd_spt3_arch =
  N (32, 32, 0, "spt3", TRUE, NN(0));
