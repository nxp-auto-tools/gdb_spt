/*
 * Copyrights
 */

#ifndef APEX_TDEP__H
#define APEX_TDEP__H

#ifndef TARGET_APEX
#define TARGET_APEX
#endif

/* APEX arch generic sizes */

#define APEX_BITS_PER_BYTE			8	//for both scalar and vector
#define APEX_BITS_PER_HALFWORD		16	//for both scalar and vector
#define APEX_BITS_PER_WORD			32	//for both scalar and vector
#define APEX_BITS_PER_DOUBLEWORD	64	//only for scalar

#define APEX_BITS_PER_DMEM_ADDR		24
#define APEX_BITS_PER_CMEM_ADDR		16

/*
 * Array Control Processor registers. Size = 32 bits
 * An overflow flag (OV) is also provided.
 * There are five registers that are given for control purposes only: PC, LF, LS, LE, and LC.
 * The Loop registers: LF, LS, LE, and LC, are not accessible by the debugger.
 */
#define APEX_CONTROL_REG_PC	0
#define APEX_CONTROL_REG_LF	1
#define APEX_CONTROL_REG_LS	2
#define APEX_CONTROL_REG_LE	3
#define APEX_CONTROL_REG_LC	4

#define APEX_ACP_REG_SIZE			32 // ACP register size in bits

/* ACP registers offsets */
#define APEX_ACP_REG_ZERO	0
#define APEX_ACP_REG_GP		1	//	general purposes registers offset begin
#define APEX_ACP_REG_GP_NUM 28	//	general purposes registers offset and total number
#define APEX_ACP_REG_LINK	29
#define APEX_ACP_REG_VSP	30	//vector stack pointer
#define APEX_ACP_REG_SSP	31	//scalar stack pointer

#define APEX_ACP_REG_OV 	0	//TODO: overflow flag uint1 (Some bit-mask or separate register)

#define APEX_ACP_REG_RV_LOW		1 	// return 32-bit value
#define APEX_ACP_REG_RV_HIGH	2	// return 64-bit value

/*
 * Computational Unit registers
 */

#define APEX_CU_REG_SIZE	16 // CU register size in bits
#define APEX_CU_REG_NUM		32 // total number of CU registers
/*
 * Vector Unit registers.
 * Each vector unit has 8 16-bit general purpose registers (V) and an overflow flag (OVV).
 * 4 Vector conditional registers are also provided to store the results of comparison operations.
 * A vector conditional stack is used to turn ON/OFF the vector units.
 * The vector conditional stack pointer (VCSPTR) is used to keep track of the top of the stack.
 */

#define APEX_VU_REG_V_SIZE 	8	// number of CU registers per vector
#define APEX_VU_REG_V_NUM 	4	// total number of vectors
#define APEX_VU_REG_VC_NUM	4	// total number of vector conditional registers
#define APEX_VU_REG_VCSPTR		//TODO: vector conditional stack pointer (size uint3)
#define APEX_VU_REG_VCS_NUM	8 	// total number of vector conditional stacks (size vuint1)

/* Vector Unit regs offsets */
#define APEX_VU_REG_V_1		0
#define APEX_VU_REG_V_2		1
#define APEX_VU_REG_V_3		2
#define APEX_VU_REG_V_4		3

/* Vector Unit vector conditional regs offsets*/
#define APEX_VU_REG_VC_1	0
#define APEX_VU_REG_VC_2	1
#define APEX_VU_REG_VC_3	2
#define APEX_VU_REG_VC_4	3


#define APEX_MATCHPOINTS_NUM	4	//number of hardware breakpoints

#define APEX_TOTAL_REG_NUM 67

#endif
