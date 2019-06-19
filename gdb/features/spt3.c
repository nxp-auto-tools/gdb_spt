/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
 Original: spt3.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_spt3;
static void initialize_tdesc_spt3(void) {
	struct target_desc *result = allocate_target_description();
	struct tdesc_feature *feature;
	struct tdesc_type *field_type;
	struct tdesc_type *type;

	set_tdesc_architecture(result, bfd_scan_arch("spt3"));

	feature = tdesc_create_feature(result, "spt3-dbg-regs");

	tdesc_create_reg(feature, "gbl_ctrl", 0, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_pg_st_addr", 1, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_mode_ctrl", 2, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_wd_status", 3, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_bkpt0_addr", 4, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_bkpt1_addr", 5, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_bkpt2_addr", 6, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_bkpt3_addr", 7, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_jam_inst0", 8, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_jam_inst1", 9, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_jam_inst2", 10, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_jam_inst3", 11, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "pc", 12, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_curr_inst0", 13, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_curr_inst1", 14, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_curr_inst2", 15, 1, NULL, 32, "uint32");
	tdesc_create_reg(feature, "cs_curr_inst3", 16, 1, NULL, 32, "uint32");

	tdesc_spt3 = result;
}
