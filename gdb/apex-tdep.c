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

/*! APEX specific per-architecture information. Replaces
    struct_or1k_implementation. A lot of this info comes from the config regs,
    so cannot be put in place until we have the actual target. Up until then
    we have reasonable defaults. */
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

/*----------------------------------------------------------------------------*/
/*!Determine the return convention used for a given type

   ***from or1k***Optionally, fetch or set the return value via "readbuf" or "writebuf"
   respectively using "regcache" for the register values.***

   The APEX returns scalar values via R1 and (for 64 bit values on
   32 bit architectures) R2. Structs and unions are returned by reference,
   with the address in R1

   ***from or1k***The result returned is independent of the function type, so we ignore that.***

   Throughout use read_memory(), not target_read_memory(), since the address
   may be invalid and we want an error reported (read_memory() is
   target_read_memory() with error reporting).

   @todo This implementation is labelled OR1K, but in fact is just for the 32
         bit version, OR1K. This should be made explicit

   @param[in]  gdbarch   The GDB architecture being used
   @param[in]  functype  The type of the function to be called (may be NULL)
   @param[in]  valtype   The type of the entity to be returned
   @param[in]  regcache  The register cache
   @param[in]  readbuf   Buffer into which the return value should be written
   @param[out] writebuf  Buffer from which the return value should be written

   @return  The type of return value */
/*---------------------------------------------------------------------------*/

static enum return_value_convention
apex_return_value (struct gdbarch  *gdbarch,
		   struct value    *functype,
		   struct type     *valtype,
		   struct regcache *regcache,
		   gdb_byte        *readbuf,
		   const gdb_byte  *writebuf)
{
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  enum type_code rv_type = TYPE_CODE (valtype);
  unsigned int rv_size = TYPE_LENGTH (valtype);
  unsigned int bpsw = (gdbarch_tdep (gdbarch))->bytes_per_scalar_word;
  unsigned int bpvw = (gdbarch_tdep (gdbarch))->bytes_per_vector_word;

  /* ACP (scalar) return value processing */

  if ((TYPE_CODE_STRUCT == rv_type) || (TYPE_CODE_UNION == rv_type) ||
      ((TYPE_CODE_ARRAY == rv_type) && rv_size > bpsw) ||
      (rv_size > 2*bpsw)){

      if (readbuf){
		  ULONGEST        tmp;
		  regcache_cooked_read_unsigned (regcache, APEX_ACP_REG_RV_LOW, &tmp);
		  read_memory (tmp, readbuf, rv_size);
      }
      if (writebuf){
		  ULONGEST        tmp;
		  regcache_cooked_read_unsigned (regcache, APEX_ACP_REG_RV_LOW, &tmp);
		  write_memory (tmp, writebuf, rv_size);
      }
      return RETURN_VALUE_ABI_RETURNS_ADDRESS;
  }
  if (rv_size <= bpsw){
        /* up to one word scalars are returned in R11 */
        if (readbuf){
        	ULONGEST        tmp;
        	regcache_cooked_read_unsigned (regcache, APEX_ACP_REG_RV_LOW, &tmp);
        	store_unsigned_integer (readbuf, rv_size, byte_order, tmp);
		}
        if (writebuf){
			gdb_byte buf[4];
			memset (buf, 0, sizeof (buf));
			if (BFD_ENDIAN_LITTLE == byte_order){
				memcpy (buf,writebuf, rv_size);
			}else{
				internal_error (__FILE__, __LINE__,
						  _("apex_return_value: it seems to be that gdb decided that BIG_ENDIAN"));
				memcpy (buf + sizeof (buf) - rv_size, writebuf, rv_size);
			}
			  regcache_cooked_write (regcache, APEX_ACP_REG_RV_LOW, buf);
        }
  }
  //TODO: todo


  return RETURN_VALUE_ABI_RETURNS_ADDRESS;
}	/* apex_return_value() */

/*----------------------------------------------------------------------------*/
/*!Return the register name for the APEX architecture

   This version converted to ANSI C, made static and incorporates the static
   table of register names (this is the only place it is referenced).

   @todo The floating point and vector registers ought to be done as
         pseudo-registers.

   @param[in] gdbarch  The GDB architecture being used
   @param[in] regnum    The register number

   @return  The textual name of the register */
/*---------------------------------------------------------------------------*/

static const char *
apex_register_name (struct gdbarch *gdbarch,
		    		int regnum){
  static char *or1k_gdb_reg_names[APEX_TOTAL_REG_NUM] =
    {
      "r0", //Zero register

	  /* general purpose registers */
      "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
      "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
      "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
      "r24", "r25", "r26", "r27", "r28",

	  /*Link Register, Vector stack pointer, Scalar Stack Pointer */
	  "lrg", "vsp", "ssp",

	  /* Overflow flag, Program Counter, Loop Flag*/
	  "ov","pc","lf",

	  /*Loop start address*/
	  "ls0","ls1","ls2",

	  /* Loop End address*/
	  "le0","le1","le2",

	  /* Loop count */
	  "lc0", "lc1","lc2",

	  /* Vector registers */
	  "v0","v1","v2","v3","v4","v5","v6","v7",

	  "ovv", //vector overflow flag

	  /* vector conditionals regs */
	  "vc0","vc1","vc2","vc3",

	  "vcsptr", // vector conditional stack pointer

	  /*Vector conditional stacks*/
	  "vc0",// not used
	  "vc1","vc2","vc3","vc4","vc5","vc6","vc7"

    };

  /* If we have a target description, use it */
  if (tdesc_has_registers (gdbarch_target_desc (gdbarch)))
    return tdesc_register_name (gdbarch, regnum);
  else
    {
      if (0 <= regnum && regnum < APEX_TOTAL_REG_NUM)
	{
	  return or1k_gdb_reg_names[regnum];
	}
      else
	return NULL;
    }

}	/* APEX_register_name() */

/*----------------------------------------------------------------------------*/
/*!Identify the type of a register

   @todo I don't fully understand exactly what this does, but I think this
         makes sense!

   @param[in] arch     The GDB architecture to consider
   @param[in] regnum   The register to identify

   @return  The type of the register */
/*---------------------------------------------------------------------------*/

static struct type *
apex_register_type (struct gdbarch *arch,
		    int             regnum)
{
  static struct type *void_func_ptr = NULL;
  static struct type *void_ptr      = NULL;

  /* Set up the static pointers once, the first time*/
  if (NULL == void_func_ptr)
    {
      struct type *void_type = builtin_type (arch)->builtin_void;

      void_ptr      = lookup_pointer_type (void_type);
      void_func_ptr = lookup_pointer_type (lookup_function_type (void_type));
    }

  if((regnum >= 0) && (regnum < APEX_TOTAL_REG_NUM))
    {
      switch (regnum)
		{
		case APEX_CONTROL_REG_PC:
		  return void_func_ptr;			/* Pointer to code */

		/*case OR1K_SP_REGNUM:
		case OR1K_FP_REGNUM:
		  return void_ptr;	*/			/* Pointer to data */

		default:
		  return builtin_type (arch)->builtin_uint32;	/* Data */
		}
    }

  internal_error (__FILE__, __LINE__,
		  _("apex_register_type: illegal register number %d"), regnum);
  return builtin_type (arch)->builtin_uint32;
}	/* apex_register_type() */

/*----------------------------------------------------------------------------*/
/*!Handle the "info register" command

   Print the identified register, unless it is -1, in which case print all
   the registers. If all is 1 means all registers, otherwise only the core
   GPRs.

   @todo At present all registers are printed with the default method. Should
         there be something special for FP registers?

   @param[in] gdbarch  The GDB architecture being used
   @param[in] file     File handle for use with any custom I/O
   @param[in] frame    Frame info for use with custom output
   @param[in] regnum   Register of interest, or -1 if all registers
   @param[in] all      1 if all means all, 0 if all means just GPRs

   @return  The aligned stack frame address */
/*---------------------------------------------------------------------------*/

static void
apex_registers_info (struct gdbarch    *gdbarch,
		     struct ui_file    *file,
		     struct frame_info *frame,
		     int                regnum,
		     int                all)
{
  struct regcache *regcache = get_current_regcache ();

  if (-1 == regnum)
    {
      /* Do all (valid) registers */
      unsigned int  lim = APEX_TOTAL_REG_NUM;//all ? OR1K_NUM_REGS_CACHED : OR1K_MAX_GPR_REGS;

      for (regnum = 0; regnum < lim; regnum++) {
	if ('\0' != *(apex_register_name (gdbarch, regnum)))
	  {
	    apex_registers_info (gdbarch, file, frame, regnum, all);
	  }
      }
    }
  else
    {
      /* Do one specified register - if it is part of this architecture */
      if ((regnum < APEX_TOTAL_REG_NUM)
	  && ('\0' == *(apex_register_name (gdbarch, regnum))))
	{
	  error ("Not a valid register for the current processor type");
	}
      else
	{
	  /* If the register is not in the g/G packet, fetch it from the
	   * target with a p/P packet.
	   */
	  if (regnum >= APEX_TOTAL_REG_NUM)
	    target_fetch_registers (regcache, regnum);

	  default_print_registers_info (gdbarch, file, frame, regnum, all);
	}
    }
}	/* apex_registers_info() */
/* -------------------------------------------------------------------------- */
/*!Architecture initialization for APEX

   Looks for a candidate architecture in the list of architectures supplied
   using the info supplied. If none match, create a new architecture.

   @param[in] info    Information about the target architecture
   @param[in] arches  The list of currently know architectures

   @return  A structure describing the target architecture                    */
/* -------------------------------------------------------------------------- */

/*----------------------------------------------------------------------------*/
/*!Skip a function prolog

   If the input address, PC, is in a function prologue, return the address of
   the end of the prologue, otherwise return the input  address.

   @see For details of the stack frame, see the function
        or1k_frame_cache().

   @note The old version of this function used to use skip_prologue_using_sal
         to skip the prologue without checking if it had actually worked. It
         doesn't for STABS, so we had better check for a valid result.

   This function reuses the helper functions from or1k_frame_cache() to
   locate the various parts of the prolog, any or all of which may be missing.

   @param[in] gdbarch  The GDB architecture being used
   @param[in] pc       Current program counter

   @return  The address of the end of the prolog if the PC is in a function
            prologue, otherwise the input  address.                           */
/*----------------------------------------------------------------------------*/
static CORE_ADDR
apex_skip_prologue (struct gdbarch *gdbarch,
		    CORE_ADDR       pc)
{
	//TODO: edit this function
  CORE_ADDR     start_pc;
  CORE_ADDR     addr;

  addr=pc;
  return pc;
}	/* apex_skip_prologue() */

/*----------------------------------------------------------------------------*/
/*!Determine the instruction to use for a breakpoint.

   Given the address at which to insert a breakpoint (bp_addr), what will
   that breakpoint be?

   For or1k, we have a breakpoint instruction. Since all or1k instructions
   are 32 bits, this is all we need, regardless of address.

   @param[in]  gdbarch  The GDB architecture being used
   @param[in]  bp_addr  The breakpoint address in question
   @param[out] bp_size  The size of instruction selected

   @return  The chosen breakpoint instruction */
/*---------------------------------------------------------------------------*/

static const gdb_byte *
apex_breakpoint_from_pc (struct gdbarch *gdbarch,
			 CORE_ADDR      *bp_addr,
			 int            *bp_size)
{
  static const gdb_byte breakpoint[] = {0x16,1,0x0000004};//{16’h0, `DBG_APUn, `DBG_ADDR_REQ};//OR1K_BRK_INSTR_STRUCT;

  *bp_size = 3;//OR1K_INSTLEN;
  return breakpoint;

}	/* apex_breakpoint_from_pc() */

static struct gdbarch *
apex_gdbarch_init (struct gdbarch_info  info,
		   struct gdbarch_list *arches)
{
	static struct frame_base     apex_frame_base;
	struct        gdbarch       *gdbarch;
	struct        gdbarch_tdep  *tdep;
	const struct  bfd_arch_info *binfo;
	struct tdesc_arch_data      *tdesc_data = NULL;

	/* None found, create a new architecture from the information
	     provided. Can't initialize all the target dependencies until we actually
	     know which target we are talking to, but put in some defaults for now. */

	//TODO: fill the struct with appropriate params
	  binfo                   = info.bfd_arch_info;
	  tdep                    = xmalloc (sizeof *tdep);
	  tdep->num_matchpoints   = APEX_MATCHPOINTS_NUM;
	  tdep->num_gpr_regs      = APEX_ACP_REG_GP_NUM;
	  tdep->num_cu_regs			= APEX_CU_REG_NUM;
	  tdep->bytes_per_scalar_word = APEX_BITS_PER_WORD/APEX_BITS_PER_BYTE;//binfo->bits_per_word / binfo->bits_per_byte;
	  tdep->bytes_per_dmem_address = APEX_BITS_PER_DMEM_ADDR/APEX_BITS_PER_BYTE;//binfo->bits_per_address / binfo->bits_per_byte;
	  tdep->bytes_per_vector_word = APEX_BITS_PER_WORD/APEX_BITS_PER_BYTE;
	  tdep->bytes_per_cmem_address = APEX_BITS_PER_CMEM_ADDR/APEX_BITS_PER_BYTE;
	  gdbarch = gdbarch_alloc (&info, tdep);


	  /* Target data types.  */
	  set_gdbarch_short_bit             (gdbarch, 16);
	  set_gdbarch_int_bit               (gdbarch, 32);
	  set_gdbarch_long_bit              (gdbarch, 32);
	  set_gdbarch_long_long_bit         (gdbarch, 64);

	  //TODO: add necessary target date types
	 // Remember that address ranges different for ACU(24bits) and VU(16bit)

	/*set_gdbarch_float_bit             (gdbarch, 32);
	  set_gdbarch_float_format          (gdbarch, floatformats_ieee_single);
	  set_gdbarch_double_bit            (gdbarch, 64);
	  set_gdbarch_double_format         (gdbarch, floatformats_ieee_double);
	  set_gdbarch_long_double_bit       (gdbarch, 64);
	  set_gdbarch_long_double_format    (gdbarch, floatformats_ieee_double);
	  set_gdbarch_ptr_bit               (gdbarch, binfo->bits_per_address);
	  set_gdbarch_addr_bit              (gdbarch, binfo->bits_per_address);
	  set_gdbarch_char_signed           (gdbarch, 1);*/

	   /* Information about the target architecture */
	  set_gdbarch_return_value          (gdbarch, apex_return_value);
	  set_gdbarch_breakpoint_from_pc    (gdbarch, apex_breakpoint_from_pc);
	//  set_gdbarch_have_nonsteppable_watchpoint(gdbarch, 1);

	  set_gdbarch_print_insn 	       (gdbarch, print_insn_apex);

	  /* Register architecture */
	  set_gdbarch_num_regs              (gdbarch, APEX_TOTAL_REG_NUM);

	  /*set_gdbarch_pseudo_register_read  (gdbarch, or1k_pseudo_register_read);
	  set_gdbarch_pseudo_register_write (gdbarch, or1k_pseudo_register_write);
	  set_gdbarch_num_pseudo_regs       (gdbarch, OR1K_NUM_PSEUDO_REGS);
	  set_gdbarch_sp_regnum             (gdbarch, OR1K_SP_REGNUM);
	  set_gdbarch_pc_regnum             (gdbarch, OR1K_NPC_REGNUM);
	  set_gdbarch_ps_regnum             (gdbarch, OR1K_SR_REGNUM);
	  set_gdbarch_deprecated_fp_regnum  (gdbarch, OR1K_FP_REGNUM);*/

	  /* Functions to supply register information */
	  set_gdbarch_register_name         (gdbarch, apex_register_name);
	  set_gdbarch_register_type         (gdbarch, apex_register_type);
	  set_gdbarch_print_registers_info  (gdbarch, apex_registers_info);
	  //set_gdbarch_register_reggroup_p   (gdbarch, apex_register_reggroup_p);

	  /* Functions to analyse frames */
	  set_gdbarch_skip_prologue         (gdbarch, apex_skip_prologue);
	  set_gdbarch_inner_than            (gdbarch, core_addr_lessthan);
	  /*set_gdbarch_frame_align           (gdbarch, or1k_frame_align);
	  set_gdbarch_frame_red_zone_size   (gdbarch, OR1K_FRAME_RED_ZONE_SIZE);*/

	  /* Functions to access frame data */
	 /* set_gdbarch_unwind_pc             (gdbarch, or1k_unwind_pc);
	  set_gdbarch_unwind_sp             (gdbarch, or1k_unwind_sp);*/

	  /* Functions handling dummy frames */
	  /*set_gdbarch_call_dummy_location   (gdbarch, ON_STACK);
	  set_gdbarch_push_dummy_code       (gdbarch, or1k_push_dummy_code);
	  set_gdbarch_push_dummy_call       (gdbarch, or1k_push_dummy_call);
	  set_gdbarch_dummy_id              (gdbarch, or1k_dummy_id);*/

	#if 0
	  /* Set up sniffers for the frame base. Use DWARF debug info if available,
		 otherwise use our own sniffer. */
	  frame_base_append_sniffer (gdbarch, dwarf2_frame_base_sniffer);
	  //frame_base_append_sniffer (gdbarch, apex_frame_base_sniffer);
	#endif

	  /* Handle core files */
	 // set_gdbarch_iterate_over_regset_sections (gdbarch, or1k_iterate_over_regset_sections);

	  /* Frame unwinders. Use DWARF debug info if available, otherwise use our
		 own unwinder. */
	 // dwarf2_append_unwinders (gdbarch);
	 // frame_unwind_append_unwinder (gdbarch, &or1k_frame_unwind);

	  /* TODO: Does a CGEN CPU descriptor for this architecture needed?  */

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

}	/* or1k_dump_tdep() */


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
