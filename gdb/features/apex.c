/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: apex.rml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_apex;
static void
initialize_tdesc_apex_apu (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;
  struct tdesc_type *field_type;
  struct tdesc_type *type;
  const char* gp_group = "General purposes";
  const char* ctl_group = "ACP control";
  const char* vector_group = "Vector";
  const char* vector_ctl_group = "Vector control";

  set_tdesc_architecture (result, bfd_scan_arch ("apex"));

  feature = tdesc_create_feature (result, "org.gnu.gdb.apex.apu");

  //TODO: here was some flags from aarch64.c
  //need to adjust arch description with flags and vectors
  //and describe new data types like vint, uint1, uint2, etc.

  /* General purposes registers */
  tdesc_create_reg (feature, "r0", 	 0, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r1",	 1, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r2", 	 2, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r3", 	 3, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r4", 	 4, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r5", 	 5, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r6", 	 6, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r7", 	 7, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r8", 	 8, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r9", 	 9, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r10", 10, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r11", 11, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r12", 12, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r13", 13, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r14", 14, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r15", 15, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r16", 16, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r17", 17, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r18", 18, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r19", 19, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r20", 20, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r21", 21, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r22", 22, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r23", 23, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r24", 24, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r25", 25, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r26", 26, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r27", 27, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r28", 28, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r29", 29, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r30", 30, 1, gp_group, 32, "uint32");
  tdesc_create_reg (feature, "r31", 31, 1, gp_group, 32, "uint32");

  // TODO: below described rest of registers. Some are vectores
  // and control regs. Must to bring them to the right form of description

  /* ACP control registers */
  tdesc_create_reg (feature, "ov", 	32, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "pc", 	33, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "lf", 	34, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "ls0", 35, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "ls1", 36, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "ls2", 37, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "le0", 38, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "le1", 39, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "le2", 40, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "lc0", 41, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "lc1", 42, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "lc2", 43, 1, ctl_group, 32, "uint32");

  /* VCU GP registers */
  tdesc_create_reg (feature, "v0", 44, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "v1", 45, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "v2", 46, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "v3", 47, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "v4", 48, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "v5", 49, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "v6", 50, 1, ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "v7", 51, 1, ctl_group, 32, "uint32");

   /* VCU control registers */
  tdesc_create_reg (feature, "ovv", 	52, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vc0", 	53, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vc1", 	54, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vc2", 	55, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vc3", 	56, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vcsptr", 	57, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vcs0", 	58, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vcs1", 	59, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vcs2", 	60, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vcs3", 	61, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vcs4", 	62, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vcs5", 	63, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vcs6",	64, 1, vector_ctl_group, 32, "uint32");
  tdesc_create_reg (feature, "vcs7", 	65, 1, vector_ctl_group, 32, "uint32");

   tdesc_apex = result;
}
