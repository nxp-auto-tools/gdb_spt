/* Copyright (C) 2018-2021 Free Software Foundation, Inc.
   Contributed by NXP, SPT support.

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

#include "defs.h"
#include <string.h>
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "value.h"
#include "gdbcmd.h"
#include "language.h"
#include "gdbcore.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "target.h"
#include "regcache.h"
#include "gdbarch.h"
#include "gdbserver/tdesc.h"
#include "spt-tdep.h"
#include "features/spt.c"
#include "safe-ctype.h"
#include "block.h"
#include "reggroups.h"
#include "arch-utils.h"
#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "dwarf2-frame.h"
#include "trad-frame.h"
#include "regset.h"
#include "remote.h"
#include "target-descriptions.h"
#include "bfd-in2.h"

#include <inttypes.h>

#include "dis-asm.h"
#include "common/errors.h"



/* The gdbarch_tdep structure.  */

static CORE_ADDR
spt_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  return 0;
}

static const gdb_byte *
spt_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pc, int *len)
{
  static const gdb_byte break_insn[] = { 0x91 };

  *len = sizeof (break_insn);
  return break_insn;
}


static const char *
spt_register_name (struct gdbarch *gdbarch, int regnum)
{
  /*
  if (regnum >= 0 && regnum < SPARC32_NUM_REGS)
    return sparc32_register_names[regnum];

  if (regnum < SPARC32_NUM_REGS + SPARC32_NUM_PSEUDO_REGS)
    return sparc32_pseudo_register_names[regnum - SPARC32_NUM_REGS];*/

  return "pc";
}

static struct type *
spt_register_type (struct gdbarch *arch,
		    int             regnum)
{
  //TODO:
  return builtin_type (arch)->builtin_uint32;
}




static void
spt_registers_info (struct gdbarch    *gdbarch,
		     struct ui_file    *file,
		     struct frame_info *frame,
		     int                regnum,
		     int                all)
{
	//TODO:
  return;
}


/* Software single-stepping support.  */
/*
static int
spu_software_single_step (struct frame_info *frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct address_space *aspace = get_frame_address_space (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR pc, next_pc;
  unsigned int insn;
  int offset, reg;
  gdb_byte buf[4];
  ULONGEST lslr;

  pc = get_frame_pc (frame);

  if (target_read_memory (pc, buf, 4))
    throw_error (MEMORY_ERROR, _("Could not read instruction at %s."),
		 paddress (gdbarch, pc));

  insn = extract_unsigned_integer (buf, 4, byte_order);

  // Get local store limit.  
  lslr = get_frame_register_unsigned (frame, SPU_LSLR_REGNUM);
  if (!lslr)
    lslr = (ULONGEST) -1;


  if ((insn & 0xffffff00) == 0x00002100)
    next_pc = (SPUADDR_ADDR (pc) + 8) & lslr;
  else
    next_pc = (SPUADDR_ADDR (pc) + 4) & lslr;

  insert_single_step_breakpoint (gdbarch,
				 aspace, SPUADDR (SPUADDR_SPU (pc), next_pc));

  if (is_branch (insn, &offset, &reg))
    {
      CORE_ADDR target = offset;

      if (reg == SPU_PC_REGNUM)
	target += SPUADDR_ADDR (pc);
      else if (reg != -1)
	{
	  int optim, unavail;

	  if (get_frame_register_bytes (frame, reg, 0, 4, buf,
					 &optim, &unavail))
	    target += extract_unsigned_integer (buf, 4, byte_order) & -4;
	  else
	    {
	      if (optim)
		throw_error (OPTIMIZED_OUT_ERROR,
			     _("Could not determine address of "
			       "single-step breakpoint."));
	      if (unavail)
		throw_error (NOT_AVAILABLE_ERROR,
			     _("Could not determine address of "
			       "single-step breakpoint."));
	    }
	}

      target = target & lslr;
      if (target != next_pc)
	insert_single_step_breakpoint (gdbarch, aspace,
				       SPUADDR (SPUADDR_SPU (pc), target));
    }

  return 1;
}

*/

static CORE_ADDR
spt_read_pc (struct regcache *regcache)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (get_regcache_arch (regcache));
  ULONGEST pc;
  regcache_cooked_read_unsigned (regcache, SPT_PC_REGNUM, &pc);
  /* Mask off interrupt enable bit.  */
  return  pc;
}

static void
spt_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  /* Keep interrupt enabled state unchanged.  */
  ULONGEST old_pc;

  regcache_cooked_read_unsigned (regcache, SPT_PC_REGNUM, &old_pc);
  regcache_cooked_write_unsigned (regcache, SPT_PC_REGNUM,old_pc);
}


static CORE_ADDR
spt_unwind_pc (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  CORE_ADDR pc
    = frame_unwind_register_unsigned (this_frame, SPT_PC_REGNUM);

  return pc;
}


struct spt_unwind_cache
{
  /* The frame's base, optionally used by the high-level debug info.  */
  CORE_ADDR base;
  /* The address of the first instruction in this function */
  CORE_ADDR pc;
  /* The offset of register saved on stack.  If register is not saved, the
     corresponding element is -1.  */
   CORE_ADDR prev_pc;
   CORE_ADDR cfa;
  //CORE_ADDR reg_saved[2];
};


static void
spt_setup_default (struct spt_unwind_cache *cache)
{
  /*
  int i;

  for (i = 0; i < APEX_ACP_REGS; i++)
    cache->reg_saved[i] = -1;*/
  cache->base =0;
}

/* Frame base handling.  */
static struct spt_unwind_cache *
spt_frame_unwind_cache (struct frame_info *this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct spt_unwind_cache *cache;
  CORE_ADDR current_pc;

  if (*this_cache != NULL)
    return (struct spt_unwind_cache *) *this_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct spt_unwind_cache);
  (*this_cache) = cache;
  spt_setup_default (cache);

  cache->pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);

  /* Prologue analysis does the rest...  */
 // if ((cache->pc & 0xFFFFFFFF) != 0) apex_analyze_prologue (gdbarch, cache->pc, current_pc, cache, this_frame);

  return cache;
}


static void
spt_frame_this_id (struct frame_info *this_frame,
			  void **this_cache, struct frame_id *this_id)
{
  struct spt_unwind_cache *cache =
     spt_frame_unwind_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  (*this_id) = frame_id_build (cache->cfa, cache->pc);
}


static struct value *
spt_frame_prev_register (struct frame_info *this_frame,
			  void **this_cache, int regnum)
{
  struct spt_unwind_cache *cache =
	spt_frame_unwind_cache (this_frame, this_cache);
  CORE_ADDR noFrame;
  int i;

  /* If we are asked to unwind the PC, then we need to unwind PC ? */
  if (regnum == SPT_PC_REGNUM)
      //return apex_prev_pc_register(this_frame);
	  return frame_unwind_got_register(this_frame,regnum, regnum);

  /* If we've worked out where a register is stored then load it from
     there.  */
 /* if (regnum < APEX_ACP_REGS && cache->reg_saved[regnum] != -1)
    return frame_unwind_got_memory (this_frame, regnum,
				    cache->reg_saved[regnum]);*/

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

 
 static const struct frame_unwind spt_frame_unwind =
{
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  spt_frame_this_id,
  spt_frame_prev_register,
  NULL,
  default_frame_sniffer
};
 


static struct gdbarch *
spt_gdbarch_init (struct gdbarch_info info,
		   struct gdbarch_list *arches)
{
      
  struct gdbarch       *gdbarch;


   /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

info.byte_order_for_code = BFD_ENDIAN_LITTLE;
/* Allocate space for the new architecture.  */
  gdbarch = gdbarch_alloc (&info, NULL);


  /* Target data types.  */
  set_gdbarch_short_bit             (gdbarch, 16);
  set_gdbarch_int_bit               (gdbarch, 32);
  set_gdbarch_long_bit              (gdbarch, 32);
  set_gdbarch_long_long_bit         (gdbarch, 64);

    /* Information about the target architecture */
  //set_gdbarch_return_value          (gdbarch, apex_return_value);
 // set_gdbarch_breakpoint_from_pc    (gdbarch, apex_breakpoint_from_pc);
 //set_gdbarch_bits_big_endian 	    (gdbarch, BFD_ENDIAN_LITTLE);

  set_gdbarch_num_regs (gdbarch, 1);

    /* Internal <-> external register number maps.  */
  //set_gdbarch_dwarf2_reg_to_regnum (gdbarch, apex_dwarf_reg_to_regnum);

  /* Functions to supply register information */
  set_gdbarch_register_name         (gdbarch, spt_register_name);
  set_gdbarch_register_type         (gdbarch, spt_register_type);
  set_gdbarch_print_registers_info  (gdbarch, spt_registers_info);

  /* Frame handling.  */
  set_gdbarch_unwind_pc (gdbarch, spt_unwind_pc);
 // set_gdbarch_unwind_sp (gdbarch, apex_unwind_sp);  
  frame_unwind_append_unwinder (gdbarch, &spt_frame_unwind);

  /* Functions to analyse frames */
  set_gdbarch_skip_prologue         (gdbarch, spt_skip_prologue);
  set_gdbarch_inner_than            (gdbarch, core_addr_lessthan);
  set_gdbarch_breakpoint_from_pc (gdbarch, spt_breakpoint_from_pc);
  set_gdbarch_pc_regnum (gdbarch, SPT_PC_REGNUM);
  
  set_gdbarch_read_pc (gdbarch, spt_read_pc);
  set_gdbarch_write_pc (gdbarch, spt_write_pc);

  /*Associates registers description with arch*/
 // tdesc_use_registers (gdbarch, tdesc, tdesc_data);
 
 
//set_gdbarch_software_single_step (gdbarch, spu_software_single_step);



  /* instruction set printer */
  set_gdbarch_print_insn (gdbarch, print_insn_spt2);


  return gdbarch;
} /* _gdbarch_init() */



/*----------------------------------------------------------------------------*/
/*!Dump the target specific data for this architecture

   @param[in] gdbarch  The architecture of interest
   @param[in] file     Where to dump the data */
/*---------------------------------------------------------------------------*/
static void
spt_dump_tdep (struct gdbarch *gdbarch,
		struct ui_file *file)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (NULL == tdep)
    {
      return;			/* Nothing to report */
    }

  fprintf_unfiltered (file, "Dummy spt_dump_tdep:\n");

}


/*----------------------------------------------------------------------------*/
/*!Main entry point for target architecture initialization

   In this version initializes the architecture via
   registers_gdbarch_init(). Add a command to set and show special purpose
   registers. */
/*---------------------------------------------------------------------------*/

extern initialize_file_ftype _initialize_spt_tdep; /* -Wmissing-prototypes */

void
_initialize_spt_tdep (void)
{
	  gdbarch_register (bfd_arch_spt, spt_gdbarch_init, spt_dump_tdep);
	  /* Tell remote stub that we support XML target description.  */
	  //register_remote_support_xml ("spt");//really need it ???


} /* _initialize_spt_tdep() */

