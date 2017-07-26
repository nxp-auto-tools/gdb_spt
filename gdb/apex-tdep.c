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

#include "apex-tdep.h"

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

#include <inttypes.h>

#include "dis-asm.h"
#include "common/errors.h"
//#include "opcodes/apex-desc.h"
//#include "opcodes/apex-opc.h"


/* The gdbarch_tdep structure.  */

/*! APEX specific per-architecture information.*/
struct gdbarch_tdep
{
  unsigned int  num_matchpoints;	/* Total h/w breakpoints available. */
  unsigned int  num_gpr_regs;		/* Number of general registers.  */
  unsigned int	num_cu_regs;		/* TODO: Number of computational units. Does it needed */
  int           bytes_per_scalar_word; /* self-commented */
  int			bytes_per_vector_word; /* self-commented */
  int           bytes_per_dmem_address; /* ACP memory */
  int			bytes_per_cmem_address;	/* VU memory */
  //CGEN_CPU_DESC gdb_cgen_cpu_desc;
  //TODO: correct struct with appropriate params (must we separate vector/scalar word/addr size?)
};

static enum return_value_convention
apex_return_value (struct gdbarch  *gdbarch,
		   struct value    *functype,
		   struct type     *valtype,
		   struct regcache *regcache,
		   gdb_byte        *readbuf,
		   const gdb_byte  *writebuf)
{

  //TODO: todo


  return RETURN_VALUE_ABI_RETURNS_ADDRESS;
}

static const char *
apex_register_name (struct gdbarch *gdbarch,
		    		int regnum){
  static char *apex_gdb_reg_names[APEX_TOTAL_REG_NUM] =
    {
      "apu1_r0", //Zero register

	  /* general purpose registers */
      "apu1_r1",  "apu1_r2",  "apu1_r3",  "apu1_r4",  "apu1_r5",  "apu1_r6",
	  "apu1_r7",  "apu1_r8",  "apu1_r9",  "apu1_r10", "apu1_r11", "apu1_r12",
	  "apu1_r13", "apu1_r14", "apu1_r15", "apu1_r16", "apu1_r17", "apu1_r18",
	  "apu1_r19", "apu1_r20", "apu1_r21", "apu1_r22", "apu1_r23", "apu1_r24",
	  "apu1_r25", "apu1_r26", "apu1_r27", "apu1_r28",
	  "apu1_r29", //Link Register
	  "apu1_r30", //Vector stack pointer
	  "apu1_r31", //Scalar Stack Pointer

	  /*Link Register, Vector stack pointer, Scalar Stack Pointer */
	  //"lrg", "vsp", "ssp",

	  /* Overflow flag, Program Counter, Loop Flag*/
	  "apu1_ov","apu1_pc","apu1_lf",

	  /*Loop start address*/
	  "apu1_ls0","apu1_ls1","apu1_ls2",

	  /* Loop End address*/
	  "apu1_le0","apu1_le1","apu1_le2",

	  /* Loop count */
	  "apu1_lc0", "apu1_lc1","apu1_lc2",

	  /* Vector registers */
	  "apu1_v0","apu1_v1","apu1_v2","apu1_v3","apu1_v4","apu1_v5","apu1_v6","apu1_v7",

	  "apu1_ovv", //vector overflow flag

	  /* vector conditionals regs */
	  "apu1_vc0","apu1_vc1","apu1_vc2","apu1_vc3",

	  "apu1_vcsptr", // vector conditional stack pointer

	  /*Vector conditional stacks*/
	  "apu1_vcs0",// not used
	  "apu1_vcs1","apu1_vcs2","apu1_vcs3","apu1_vcs4",
	  "apu1_vcs5","apu1_vcs6","apu1_vcs7",


      "apu2_r0", //Zero register

	  /* general purpose registers */
      "apu2_r1",  "apu2_r2",  "apu2_r3",  "apu2_r4",  "apu2_r5",  "apu2_r6",
	  "apu2_r7",  "apu2_r8",  "apu2_r9",  "apu2_r10", "apu2_r11", "apu2_r12",
	  "apu2_r13", "apu2_r14", "apu2_r15", "apu2_r16", "apu2_r17", "apu2_r18",
	  "apu2_r19", "apu2_r20", "apu2_r21", "apu2_r22", "apu2_r23", "apu2_r24",
	  "apu2_r25", "apu2_r26", "apu2_r27", "apu2_r28",
	  "apu2_r29", //Link Register
	  "apu2_r30", //Vector stack pointer
	  "apu2_r31", //Scalar Stack Pointer

	  /*Link Register, Vector stack pointer, Scalar Stack Pointer */
	  //"lrg", "vsp", "ssp",

	  /* Overflow flag, Program Counter, Loop Flag*/
	  "apu2_ov","apu2_pc","apu2_lf",

	  /*Loop start address*/
	  "apu2_ls0","apu2_ls1","apu2_ls2",

	  /* Loop End address*/
	  "apu2_le0","apu2_le1","apu2_le2",

	  /* Loop count */
	  "apu2_lc0", "apu2_lc1","apu2_lc2",

	  /* Vector registers */
	  "apu2_v0","apu2_v1","apu2_v2","apu2_v3","apu2_v4","apu2_v5","apu2_v6","apu2_v7",

	  "apu2_ovv", //vector overflow flag

	  /* vector conditionals regs */
	  "apu2_vc0","apu2_vc1","apu2_vc2","apu2_vc3",

	  "apu2_vcsptr", // vector conditional stack pointer

	  /*Vector conditional stacks*/
	  "apu2_vcs0",// not used
	  "apu2_vcs1","apu2_vcs2","apu2_vcs3","apu2_vcs4",
	  "apu2_vcs5","apu2_vcs6","apu2_vcs7"

    };

  /* If we have a target description, use it */
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
	  return tdesc_register_name (gdbarch, regnum);
  else
	  if (0 <= regnum && regnum < APEX_TOTAL_REG_NUM)
		  return apex_gdb_reg_names[regnum];
	  else
		  return NULL;


}

static struct type *
apex_register_type (struct gdbarch *arch,
		    int             regnum)
{
  //TODO:
  return builtin_type (arch)->builtin_uint32;
}




static void
apex_registers_info (struct gdbarch    *gdbarch,
		     struct ui_file    *file,
		     struct frame_info *frame,
		     int                regnum,
		     int                all)
{
	//TODO:
  return;
}



static CORE_ADDR
apex_skip_prologue (struct gdbarch *gdbarch,
		    CORE_ADDR       pc)
{
	//TODO: edit this function
  CORE_ADDR     start_pc;
  CORE_ADDR     addr;

  addr=pc;
  return pc;
}



static const gdb_byte *
apex_breakpoint_from_pc (struct gdbarch *gdbarch,
			 CORE_ADDR      *bp_addr,
			 int            *bp_size)
{
  static const gdb_byte breakpoint[] = {0};
  *bp_size = 1;
  return breakpoint;

}




static struct gdbarch *
apex_gdbarch_init (struct gdbarch_info  info,
		   struct gdbarch_list *arches)
{
	static struct frame_base     apex_frame_base;
	struct        gdbarch       *gdbarch;
	struct        gdbarch_tdep  *tdep;
	const struct  bfd_arch_info *binfo;
	struct tdesc_arch_data      *tdesc_data = NULL;
	const struct target_desc *tdesc = info.target_desc;

	//gdb_assert (tdesc_has_registers (tdesc));

	if (tdesc_has_registers (tdesc)){

		static const char *const gprs[] = {
				"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
				"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
				"r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
				"r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
		};

		static const char *const ctlrs[] = {
			/* Overflow flag, Program Counter, Loop Flag*/
				"ov","pc","lf",

			/*Loop start address*/
				"ls0","ls1","ls2",

			/* Loop End address*/
				"le0","le1","le2",

			/* Loop counters */
				"lc0", "lc1","lc2",

		};

		const struct tdesc_feature *feature = tdesc_find_feature (tdesc, "org.gnu.gdb.apex.apu1");
		if (feature == NULL)
			return NULL;
		tdesc_data = tdesc_data_alloc ();

		int valid_p = 1;


	}




	/* None found, create a new architecture from the information
	     provided. Can't initialize all the target dependencies until we actually
	     know which target we are talking to, but put in some defaults for now. */

	//TODO: fill the struct with appropriate params
	// binfo                        = info.bfd_arch_info;
	  tdep                         = xmalloc (sizeof *tdep);
	  tdep->num_matchpoints        = APEX_MATCHPOINTS_NUM;
	  tdep->num_gpr_regs      	   = APEX_ACP_REG_GP_NUM;
	  tdep->num_cu_regs			   = APEX_CU_REG_NUM;
	  tdep->bytes_per_scalar_word  = APEX_BITS_PER_WORD/APEX_BITS_PER_BYTE;//binfo->bits_per_word / binfo->bits_per_byte;
	  tdep->bytes_per_dmem_address = APEX_BITS_PER_DMEM_ADDR/APEX_BITS_PER_BYTE;//binfo->bits_per_address / binfo->bits_per_byte;
	  tdep->bytes_per_vector_word  = APEX_BITS_PER_WORD/APEX_BITS_PER_BYTE;
	  tdep->bytes_per_cmem_address = APEX_BITS_PER_CMEM_ADDR/APEX_BITS_PER_BYTE;
	  gdbarch = gdbarch_alloc (&info, tdep);


	  /* Target data types.  */
	  set_gdbarch_short_bit             (gdbarch, 16);
	  set_gdbarch_int_bit               (gdbarch, 32);
	  set_gdbarch_long_bit              (gdbarch, 32);
	  set_gdbarch_long_long_bit         (gdbarch, 64);

	  //TODO: add necessary target date types
	 // Remember that address ranges different for ACU(24bits) and VU(16bit)



	   /* Information about the target architecture */
	  set_gdbarch_return_value          (gdbarch, apex_return_value);
	  set_gdbarch_breakpoint_from_pc    (gdbarch, apex_breakpoint_from_pc);

	  set_gdbarch_print_insn 	       (gdbarch, print_insn_apex);

	  /* Register architecture */
	  set_gdbarch_num_regs              (gdbarch, APEX_TOTAL_REG_NUM);


	  /* Functions to supply register information */
	  set_gdbarch_register_name         (gdbarch, apex_register_name);
	  set_gdbarch_register_type         (gdbarch, apex_register_type);
	  set_gdbarch_print_registers_info  (gdbarch, apex_registers_info);

	  /* Functions to analyse frames */
	  set_gdbarch_skip_prologue         (gdbarch, apex_skip_prologue);
	  set_gdbarch_inner_than            (gdbarch, core_addr_lessthan);




	return gdbarch;
} /* apex_gdbarch_init() */



/*----------------------------------------------------------------------------*/
/*!Dump the target specific data for this architecture

   @param[in] gdbarch  The architecture of interest
   @param[in] file     Where to dump the data */
/*---------------------------------------------------------------------------*/
static void
apex_dump_tdep (struct gdbarch *gdbarch,
		struct ui_file *file)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  if (NULL == tdep)
    {
      return;			/* Nothing to report */
    }

  fprintf_unfiltered (file, "apex_dump_tdep: %d matchpoints available\n",
		      tdep->num_matchpoints);
  fprintf_unfiltered (file, "apex_dump_tdep: %d general purpose registers\n",
		      tdep->num_gpr_regs);
  fprintf_unfiltered (file, "apex_dump_tdep: %d bytes per word\n",
		      tdep->bytes_per_scalar_word);
  fprintf_unfiltered (file, "apex_dump_tdep: %d bytes per address\n",
		      tdep->bytes_per_dmem_address);

}


/*----------------------------------------------------------------------------*/
/*!Main entry point for target architecture initialization

   In this version initializes the architecture via
   registers_gdbarch_init(). Add a command to set and show special purpose
   registers. */
/*---------------------------------------------------------------------------*/

extern initialize_file_ftype _initialize_apex_tdep; /* -Wmissing-prototypes */

void
_initialize_apex_tdep (void)
{
	  gdbarch_register (bfd_arch_apex, apex_gdbarch_init, apex_dump_tdep);

	  /* Tell remote stub that we support XML target description.  */
	  register_remote_support_xml ("apex");


} /* _initialize_apex_tdep() */
