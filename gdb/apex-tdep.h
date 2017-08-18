/*
 * Copyrights
 */

#ifndef APEX_TDEP__H
#define APEX_TDEP__H

#ifndef TARGET_APEX
#define TARGET_APEX
#endif





enum {
  /* ACP registers */
  APEX_R0_REGNUM = 0,
  APEX_LR_REGNUM = APEX_R0_REGNUM + 29,
  APEX_VSP_REGNUM,
  APEX_SP_REGNUM,
  APEX_OV_REGNUM,
  APEX_PC_REGNUM,
  
  /* vector unit registers */
  //APEX_VU_REGNUM,
  //LAST_APEX_VU_REGNUM = APEX_VU_REGNUM + (32/*cu*/*22/*per each cu*/),
  
  APEX_NUM_REGS,
};


/***** TRUE OFFSETS ******/

#define APEX_ACP_REG_GP 	0
#define APEX_ACP_REG_CTL 	32
#define APEX_VCU_REG_GP		44
#define APEX_VCU_REG_CTL	52

#endif
