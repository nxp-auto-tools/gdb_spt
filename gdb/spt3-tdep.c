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
#include "spt3-tdep.h"
#include "features/spt3.c"
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
spt3_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR start_pc)
{
  return 0;
}

static const gdb_byte *
spt3_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pc, int *len)
{
  static const gdb_byte break_insn[] = { 0x91 };

  *len = sizeof (break_insn);
  return break_insn;
}

static const char *const spt3_register_names[] = {"gbl_ctrl","cs_pg_st_addr","cs_mode_ctrl","cs_wd_status","cs_bkpt0_addr","cs_bkpt1_addr","cs_bkpt2_addr","cs_bkpt3_addr","cs_jam_inst0","cs_jam_inst1","cs_jam_inst2","cs_jam_inst3","cs_curr_inst_addr","pc","cs_curr_inst0","cs_curr_inst1","cs_curr_inst2","cs_curr_inst3","wr0","wr1","wr2","wr3","wr4","wr5","wr6","wr7","wr8","wr9","wr10","wr11","wr12","wr13","wr14","wr15","wr16","wr17","wr18","wr19","wr20","wr21","wr22","wr23","wr24","wr25","wr26","wr27","wr28","wr29","wr30","wr31","wr32","wr33","wr34","wr35","wr36","wr37","wr38","wr39","wr40","wr41","wr42","wr43","wr44","wr45","wr46","wr47","spr0","spr1","spr2","spr3","spr4","spr5","spr6","spr7","spr8","spr9","hw_spr0","hw_spr1","hw_spr2","hw_spr3","hw_spr4","hw_spr5","evt_spr0","evt_spr1","evt_spr2","evt_spr3","evt_spr4","evt_spr5","evt_spr6","evt_spr7","evt_spr8","evt_spr9","chrp_spr0","chrp_spr1","chrp_spr2","chrp_spr3","chrp_spr4","chrp_spr5","chrp_spr6","chrp_spr7"};

static const char *
spt3_register_name (struct gdbarch *gdbarch, int regnum)
{
  static char t_buf[10];
  if (regnum == SPT3_PC_REGNUM )return "pc";

  //return "wr_0";
  if (regnum <= 64 ){
  	sprintf(t_buf,"wr_%d",regnum-1);
	return t_buf;
  }

  return "unknown";
}

static struct type *
spt3_register_type (struct gdbarch *arch,
		    int             regnum)
{
  //TODO:
  if (regnum <= 16)
	return builtin_type (arch)->builtin_uint32;
  else
	return builtin_type (arch)->builtin_uint64;
}

static void
spt3_registers_info (struct gdbarch    *gdbarch,
		     struct ui_file    *file,
		     struct frame_info *frame,
		     int                regnum,
		     int                all)
{
	//TODO:
  return;
}

static CORE_ADDR
spt3_read_pc (struct regcache *regcache)
{
  ULONGEST pc;
  regcache_raw_read_unsigned (regcache, SPT3_PC_REGNUM, &pc);
  return  pc;
}

static void
spt3_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  /* Keep interrupt enabled state unchanged.  */
  ULONGEST old_pc;

  regcache_cooked_read_unsigned (regcache, SPT3_PC_REGNUM, &old_pc);
  regcache_cooked_write_unsigned (regcache, SPT3_PC_REGNUM,old_pc);
}


static CORE_ADDR
spt3_unwind_pc (struct gdbarch *gdbarch, struct frame_info *this_frame)
{
  CORE_ADDR pc
    = frame_unwind_register_unsigned (this_frame, SPT3_PC_REGNUM);

  return pc;
}


struct spt3_unwind_cache
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
spt3_setup_default (struct spt3_unwind_cache *cache)
{
  /*
  int i;

  for (i = 0; i < APEX_ACP_REGS; i++)
    cache->reg_saved[i] = -1;*/
  cache->base =0;
}

/* Frame base handling.  */
static struct spt3_unwind_cache *
spt3_frame_unwind_cache (struct frame_info *this_frame, void **this_cache)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  struct spt3_unwind_cache *cache;
  CORE_ADDR current_pc;

  if (*this_cache != NULL)
    return (struct spt3_unwind_cache *) *this_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct spt3_unwind_cache);
  (*this_cache) = cache;
  spt3_setup_default (cache);

  cache->pc = get_frame_func (this_frame);
  current_pc = get_frame_pc (this_frame);

  /* Prologue analysis does the rest...  */
 // if ((cache->pc & 0xFFFFFFFF) != 0) apex_analyze_prologue (gdbarch, cache->pc, current_pc, cache, this_frame);

  return cache;
}


static void
spt3_frame_this_id (struct frame_info *this_frame,
			  void **this_cache, struct frame_id *this_id)
{
  struct spt3_unwind_cache *cache =
     spt3_frame_unwind_cache (this_frame, this_cache);

  /* This marks the outermost frame.  */
  if (cache->base == 0)
    return;

  (*this_id) = frame_id_build (cache->cfa, cache->pc);
}


static struct value *
spt3_frame_prev_register (struct frame_info *this_frame,
			  void **this_cache, int regnum)
{
  struct spt3_unwind_cache *cache =
	spt3_frame_unwind_cache (this_frame, this_cache);
  CORE_ADDR noFrame;
  int i;

  /* If we are asked to unwind the PC, then we need to unwind PC ? */
  if (regnum == SPT3_PC_REGNUM)
      //return apex_prev_pc_register(this_frame);
	  return frame_unwind_got_register(this_frame,regnum, regnum);

  return frame_unwind_got_register (this_frame, regnum, regnum);
}

 
 static const struct frame_unwind spt3_frame_unwind =
{
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  spt3_frame_this_id,
  spt3_frame_prev_register,
  NULL,
  default_frame_sniffer
};
 


static struct gdbarch *
spt3_gdbarch_init (struct gdbarch_info info,
		   struct gdbarch_list *arches)
{
      
  struct gdbarch       *gdbarch;

  /* If there is already a candidate, use it.  */
   arches = gdbarch_list_lookup_by_info (arches, &info);
   if (arches != NULL)
     return arches->gdbarch;

  struct tdesc_arch_data *tdesc_data = NULL;
  const struct target_desc *tdesc = info.target_desc;

const struct tdesc_feature *feature;

  int i;
  int valid_p = 1;


  /* Ensure we always have a target descriptor.  */
  if (!tdesc_has_registers (tdesc)){
    //warning("tdesc has NO registers");
    tdesc = tdesc_spt3;
  }
  gdb_assert (tdesc);

	if (tdesc_has_registers(tdesc)) {
		tdesc_data = tdesc_data_alloc();
		feature = tdesc_find_feature(tdesc, "spt3-dbg-regs");

		if (feature == NULL) {
			error("spt3_gdbarch_init: no feature spt3-dbg-regs");
			return NULL;
		}

		for (i = 0; i < SPT3_DBG_REGNUM; i++) {
			valid_p &= tdesc_numbered_register(feature, tdesc_data, i,
					spt3_register_names[i]);
		}

		if (!valid_p) {
			tdesc_data_cleanup(tdesc_data);
			return NULL;
		}

		feature = tdesc_find_feature(tdesc, "spt3-work-regs");
		if (feature != NULL) {
			valid_p = 1;
			for (i = SPT3_DBG_REGNUM; i < SPT3_DBG_REGNUM + SPT3_WORK_REGNUM; i++)
				valid_p &= tdesc_numbered_register(feature, tdesc_data, i,
						spt3_register_names[i]);
			if (!valid_p) {
				tdesc_data_cleanup(tdesc_data);
				return NULL;
			}
		}

		feature = tdesc_find_feature(tdesc, "spt3-special-regs");
		if (feature != NULL) {
			valid_p = 1;
			for (i = SPT3_DBG_REGNUM + SPT3_WORK_REGNUM; i < SPT3_REGNUM; i++)
				valid_p &= tdesc_numbered_register(feature, tdesc_data, i,
						spt3_register_names[i]);
			if (!valid_p) {
				tdesc_data_cleanup(tdesc_data);
				return NULL;
			}
		}

	}

  info.byte_order_for_code = BFD_ENDIAN_LITTLE;
/* Allocate space for the new architecture.  */
  gdbarch = gdbarch_alloc (&info, NULL);


  /* Target data types.  */
  set_gdbarch_short_bit             (gdbarch, 16);
  set_gdbarch_int_bit               (gdbarch, 32);
  set_gdbarch_long_bit              (gdbarch, 32);
  set_gdbarch_long_long_bit         (gdbarch, 64);

    /* Information about the target architecture */
 set_gdbarch_bits_big_endian 	    (gdbarch, BFD_ENDIAN_LITTLE);

  set_gdbarch_num_regs (gdbarch, SPT3_REGNUM);

    /* Internal <-> external register number maps.  */

  /* Functions to supply register information */
  set_gdbarch_register_name         (gdbarch, spt3_register_name);
  set_gdbarch_register_type         (gdbarch, spt3_register_type);
  //set_gdbarch_print_registers_info  (gdbarch, spt3_registers_info);

  /* Frame handling.  */
  set_gdbarch_unwind_pc (gdbarch, spt3_unwind_pc);
  frame_unwind_append_unwinder (gdbarch, &spt3_frame_unwind);

  /* Functions to analyse frames */
  set_gdbarch_skip_prologue         (gdbarch, spt3_skip_prologue);
  set_gdbarch_inner_than            (gdbarch, core_addr_lessthan);
  set_gdbarch_breakpoint_from_pc (gdbarch, spt3_breakpoint_from_pc);
  set_gdbarch_pc_regnum (gdbarch, SPT3_PC_REGNUM);
  
  set_gdbarch_read_pc (gdbarch, spt3_read_pc);
  set_gdbarch_write_pc (gdbarch, spt3_write_pc);

  /*Associates registers description with arch*/
  tdesc_use_registers (gdbarch, tdesc, tdesc_data);
 
 
//set_gdbarch_software_single_step (gdbarch, spu_software_single_step);



  /* instruction set printer */
  set_gdbarch_print_insn (gdbarch, print_insn_spt3);

  return gdbarch;
} /* _gdbarch_init() */



/*----------------------------------------------------------------------------*/
/*!Dump the target specific data for this architecture

   @param[in] gdbarch  The architecture of interest
   @param[in] file     Where to dump the data */
/*---------------------------------------------------------------------------*/
static void
spt3_dump_tdep (struct gdbarch *gdbarch,
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

extern initialize_file_ftype _initialize_spt3_tdep; /* -Wmissing-prototypes */

void
_initialize_spt3_tdep (void)
{
	  gdbarch_register (bfd_arch_spt3, spt3_gdbarch_init, spt3_dump_tdep);
	  initialize_tdesc_spt3();
	  /* Tell remote stub that we support XML target description.  */
	  register_remote_support_xml ("spt3");//really need it ???


} /* _initialize_spt3_tdep() */

