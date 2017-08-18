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
#include "apex-tdep.h"
#include "features/apex.c"
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
//#include "opcodes/apex-desc.h"
//#include "opcodes/apex-opc.h"


/* The gdbarch_tdep structure.  */

/*! APEX specific per-architecture information.*/
struct gdbarch_tdep
{
  unsigned int  num_matchpoints;	/* Total h/w breakpoints available. */
  unsigned int  scalar_gp_regs_num;		/* Number of general registers.  */
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

static const char *const acp_register_names[] = {
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
  "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",
  "ov","pc"
};

static const char *const vcu_gp_regs[] = {
	"v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7"
};

static const char *const vcu_ctl_regs[] = {
	"ovv","vc0","vc1","vc2","vc3","vcsptr",
	"vcs0","vcs1","vcs2","vcs3","vcs4","vcs5",
	"vcs6","vcs7"
};


static const char *
apex_register_name (struct gdbarch *gdbarch,
		    		int regnum){

  if (regnum >= ARRAY_SIZE (acp_register_names))
    /* These registers are only supported on targets which supply
       an XML description.  */
    return "";

  return acp_register_names[regnum];
  
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
	//TODO:
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
apex_gdbarch_init (struct gdbarch_info info,
		   struct gdbarch_list *arches)
{
  static const char *const apex_sp_names[] = { "r31", "sp", NULL };
  static const char *const apex_vsp_names[] = { "r30", "vsp", NULL };
  static const char *const apex_lr_names[] = { "r29", "lr", NULL };
      
  struct gdbarch       *gdbarch;
  struct gdbarch_tdep  *tdep;

  struct tdesc_arch_data *tdesc_data = NULL;
  const struct target_desc *tdesc=info.target_desc;
  const struct tdesc_feature *feature;

  int i;
  int valid_p = 1;
  unsigned int regs_num = 0;


  /* Ensure we always have a target descriptor.  */
  if (!tdesc_has_registers (tdesc)){
    //warning("tdesc has NO registers");
    tdesc = tdesc_apex;
  }
  gdb_assert (tdesc);

  
  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.apex.apu.acp");

  if (feature == NULL){
    error ("apex_gdbarch_init: no feature org.gnu.gdb.apex.apu.acp");
    return NULL;
  }

  tdesc_data = tdesc_data_alloc ();


  for (i = 0; i < APEX_LR_REGNUM; i++){
    valid_p &= tdesc_numbered_register (feature, tdesc_data, i,
                                        acp_register_names[i]);
  }

  valid_p &= tdesc_numbered_register_choices (feature, tdesc_data, APEX_SP_REGNUM,
                                              apex_sp_names);
  i++;

  valid_p &= tdesc_numbered_register_choices (feature, tdesc_data, APEX_VSP_REGNUM,
                                              apex_vsp_names);
  i++;
  
  valid_p &= tdesc_numbered_register_choices (feature, tdesc_data, APEX_LR_REGNUM,
                                              apex_lr_names);
  i++;


  if (!valid_p)
    {
      tdesc_data_cleanup (tdesc_data);
      return NULL;
    }
  else
    {
      regs_num += i;
    }  

    
  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.apex.apu.vu");    
    
  if (feature != NULL)
    {
      //TODO: check VU registers
      
      
      if (!valid_p)
      {
        tdesc_data_cleanup (tdesc_data);
        return NULL;
      }
    }


  tdep = XCNEW (struct gdbarch_tdep);
  gdbarch = gdbarch_alloc (&info, tdep);



  /* Target data types.  */
  set_gdbarch_short_bit             (gdbarch, 16);
  set_gdbarch_int_bit               (gdbarch, 32);
  set_gdbarch_long_bit              (gdbarch, 32);
  set_gdbarch_long_long_bit         (gdbarch, 64);


  set_gdbarch_pc_regnum (gdbarch, APEX_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, APEX_SP_REGNUM);

    /* Information about the target architecture */
  set_gdbarch_return_value          (gdbarch, apex_return_value);
  set_gdbarch_breakpoint_from_pc    (gdbarch, apex_breakpoint_from_pc);

  /* Register architecture */
  set_gdbarch_num_regs              (gdbarch, APEX_TOTAL_REG_NUM_PER_APU);


  /* Functions to supply register information */
  set_gdbarch_register_name         (gdbarch, apex_register_name);
  set_gdbarch_register_type         (gdbarch, apex_register_type);
  set_gdbarch_print_registers_info  (gdbarch, apex_registers_info);

  /* Functions to analyse frames */
  set_gdbarch_skip_prologue         (gdbarch, apex_skip_prologue);
  set_gdbarch_inner_than            (gdbarch, core_addr_lessthan);

  /*Associates registers description with arch*/
  tdesc_use_registers (gdbarch, tdesc, tdesc_data);

  /* instruction set printer */
  set_gdbarch_print_insn (gdbarch, print_insn_apex);


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
		      tdep->scalar_gp_regs_num);
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

	  initialize_tdesc_apex_apu();
	  /* Tell remote stub that we support XML target description.  */
	  register_remote_support_xml ("apex");


} /* _initialize_apex_tdep() */
