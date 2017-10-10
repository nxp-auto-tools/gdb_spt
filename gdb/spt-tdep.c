/*
 * Copyrights
 */
//#include "demangle.h"
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

  return "no reg";
}



static struct gdbarch *
spt_gdbarch_init (struct gdbarch_info info,
		   struct gdbarch_list *arches)
{
      
  struct gdbarch       *gdbarch;


   /* If there is already a candidate, use it.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;


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
  set_gdbarch_bits_big_endian 	    (gdbarch, BFD_ENDIAN_LITTLE);
  set_gdbarch_num_regs (gdbarch, 0);

    /* Internal <-> external register number maps.  */
  //set_gdbarch_dwarf2_reg_to_regnum (gdbarch, apex_dwarf_reg_to_regnum);

  /* Functions to supply register information */
  set_gdbarch_register_name         (gdbarch, spt_register_name);
 // set_gdbarch_register_type         (gdbarch, apex_register_type);
 // set_gdbarch_print_registers_info  (gdbarch, apex_registers_info);

  /* Frame handling.  */
 // set_gdbarch_unwind_pc (gdbarch, apex_unwind_pc);
 // set_gdbarch_unwind_sp (gdbarch, apex_unwind_sp);  
  //frame_unwind_append_unwinder (gdbarch, &apex_frame_unwind);

  /* Functions to analyse frames */
  set_gdbarch_skip_prologue         (gdbarch, spt_skip_prologue);
  set_gdbarch_inner_than            (gdbarch, core_addr_lessthan);
  set_gdbarch_breakpoint_from_pc (gdbarch, spt_breakpoint_from_pc);

  /*Associates registers description with arch*/
 // tdesc_use_registers (gdbarch, tdesc, tdesc_data);

  /* instruction set printer */
  set_gdbarch_print_insn (gdbarch, print_insn_spt);


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
	  register_remote_support_xml ("spt");//really need it ???


} /* _initialize_spt_tdep() */

