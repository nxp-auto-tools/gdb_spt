/*
* Copyright (c) 2020, NXP.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of NXP, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
