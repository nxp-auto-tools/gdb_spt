/* Copyright (C) 2018-2021 Free Software Foundation, Inc.
   Contributed by NXP, APEX support.

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
  APEX_ACP_REGS,
  /* vector unit registers */
  //APEX_VU_REGNUM,
  //LAST_APEX_VU_REGNUM = APEX_VU_REGNUM + (32/*cu*/*22/*per each cu*/),
  
  APEX_NUM_REGS = APEX_ACP_REGS,
};


/***** TRUE OFFSETS ******/

#define APEX_ACP_REG_GP 	0
#define APEX_ACP_REG_CTL 	32
#define APEX_VCU_REG_GP		44
#define APEX_VCU_REG_CTL	52

#endif
