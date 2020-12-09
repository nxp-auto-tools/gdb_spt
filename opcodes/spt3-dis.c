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
#include "sysdep.h"
#include <ansidecl.h> //for ATTRIBUTE_UNUSED
#include "dis-asm.h"
//#include "apex-opc.h"
#include "elf-bfd.h"
//#include "sptInstruction.h"



#include "spt3-dis.h"
#include <string.h>
#include <stdio.h>

typedef unsigned char byte;
typedef unsigned long long   uint64_t;

#define BUF_SZ 200
#define ATTRIBUTE_UNUSED
#define INSN_ unsigned x0, unsigned x1, unsigned x2, unsigned x3
#define INSN_F insn[0], insn[1], insn[2], insn[3]
#define SP " "

static unsigned int  insn[4];
	char disasmBuffer[20];
	char outbuff[BUF_SZ];
	static int bigendian;
	static void setEndian(int value){
		bigendian = value;
	};

	static const int size = 16;
	

char * getBIns(INSN_, char * insName){
	static	char  buff[20];
	memset(buff, 0, sizeof(buff));
	if (((x0 >> 24) & 0xfc) >> 4 == 3)sprintf(buff, "%sb", insName);
	else sprintf(buff, "%s", insName);
	return buff;
}
	
	

char *  getTwOvs(INSN_){
	static char buff[10];
	unsigned char ovs = ((x3 & 0xf));
	if (ovs == 0)sprintf(buff, ".noovs");
	else  sprintf(buff, ".ovs%d", 1 << ovs);
	return buff;
}

char * getFftRnd(INSN_){
	static	char  buff[10];
	memset(buff, 0, sizeof(buff));
	int fftrnd = ((x0 & 0x700000) >> 20);
	sprintf(buff, ".round%d", fftrnd);
	return buff;
}


char * get_in_dattyp_s(INSN_){
	static	char  buff[10];
	char val[][10] = { ".real", ".cmplx", ".log2" };
	memset(buff, 0, sizeof(buff));
	int in_dattyp = ((x0 & 0x2000000) >> 25);
	sprintf(buff, val[in_dattyp]);
	return buff;
}

unsigned char getInd(INSN_){
	return ((x0 & 0x2000) >> 13);
}

char * getModVal(INSN_){
	static char buff[10];
	memset(buff, 0, sizeof(buff));
	int modulo_val = ((x0 & 0x1f));
	sprintf(buff, ".mod%d", modulo_val);
	return buff;
}



char * get_shift_val(INSN_){
	static char buff[10];
	memset(buff, 0, sizeof(buff));
	int shft_val = ((x3 & 0x70) >> 4);
	if (shft_val == 5)shft_val = 8;
	if (shft_val == 0) sprintf(buff, ".nosft");
	else sprintf(buff, ".shift%d", shft_val);
	return buff;
}

unsigned char  getShift(INSN_){
	return ((x0 & 0x1000000) >> 24);
}

 char * getShift_s(INSN_){
	 static char buff[10];
	 if (getShift(x0,x1,x2,x3)) sprintf(buff, "%s", ".shift");
	 else sprintf(buff, "%s", ".noshift");
	return buff;
}


unsigned char  getSrc(INSN_){
	return ((x0 & 0x2000000) >> 25);
}


char * getSrcAddWr(INSN_){
	static char buff[10];
	memset(buff, 0, sizeof(buff));
	int src_add_wr = ((x1 & 0xffff0000) >> 16);
	sprintf(buff, "WR_%d", src_add_wr);
	return buff;
}

char * getSrc2Add(INSN_){
	static char buffa[10];
	memset(buffa, 0, sizeof(buffa));
	int bank = ((x2 & 0x30000000) >> 28);
	int slice = ((x2 & 0xfff0000) >> 16);
	if (bank)sprintf(buffa, "spr(0,0,%d)",  slice);
	else sprintf(buffa, "wr(0,0,%d)", slice);
	return buffa;
}

char * getImmDat(INSN_){
	static char buff[50];
	uint64_t imm_dat = (((uint64_t)((x2 & 0xffff) >> 0) << 32)) | (((uint64_t)((x3 & 0xffffffff) >> 0) << 0));
	memset(buff, 0, sizeof(buff));
	sprintf(buff, "0x%llx", (long long unsigned)imm_dat);
	return buff;
}


char *  getMcaMode(INSN_){
	static char buff[10];
	unsigned char mode = ((x3&0xf0000000)>>28);
	if(mode==0)sprintf(buff, ".mod0");
	else  sprintf(buff, ".mod%d",1<<mode);
	return buff;
}


void convertAddr(char *buff,int  mode, int bank, int col, int slice){
		slice += ((col & 1) << 3);
		col = ((bank & 1) << 8) + (col >> 1);
		bank = bank >> 1;
	
	switch (mode) {
	  case 0:
			if (bank)sprintf(buff, "spr(0,0,%d)", (col << 3) + slice);
			else sprintf(buff, "wr(0,0,%d)", (col << 3) + slice);
		break;
	case 1:
		sprintf(buff, "tr(%d,%d,%d)", bank, col, slice);
		break;
	case 2:
		 sprintf(buff, "or(%d,%d,%d)", bank, col, slice);
		break;
	case 3:		
		sprintf(buff, "or(%d,%d,%d)", 2+bank, col, slice);

		break;
	}
}





char *  getShftOffset(INSN_){
	static char buff[20];
	char values[][20] = { ".bits_0_5", ".bits_6_11", ".bits_12_17", ".bits_18_23",
		".bits_24_29", ".bits_30_35", ".bits_36_41", ".bits_42_47" };
	unsigned char offset = ((x3 & 0xe000000) >> 25);
	sprintf(buff,"%s", values[offset]);
	return buff;
}
				 

char *  getMultCoefAdd(INSN_){
	static char buff[20];
	int mult_coef_mode = ((x2 & 0xc0000000) >> 30);
	int mult_coef_bank = ((x2 & 0x30000000) >> 28);;
	int mult_coef_col = ((x2 & 0xff80000) >> 19);
	int mult_coef_slice = ((x2 & 0x70000) >> 16);
	memset(buff, 0, sizeof(buff));
	convertAddr(buff, mult_coef_mode, mult_coef_bank, mult_coef_col, mult_coef_slice);
	return buff;
}


char *  getDestAdd(INSN_){
	static char buff[20];
	int dest_add_mode = ((x1 & 0xc000) >> 14);
	int dest_add_bank = ((x1 & 0x3000) >> 12);
	int dest_add_col = ((x1 & 0xff8) >> 3);
	int dest_add_slice = ((x1 & 0x7));
	memset(buff, 0, sizeof(buff));
	convertAddr(buff, dest_add_mode, dest_add_bank, dest_add_col, dest_add_slice);
	return buff;
}


char *  getSrcAdd(INSN_){
	static char buff[20];
	 int src_add_mode = ((x1 & 0xc0000000) >> 30);
	 int src_add_bank = ((x1 & 0x30000000) >> 28);
	 int src_add_col = ((x1 & 0xff80000) >> 19);
	 int src_add_slice = ((x1 & 0x70000) >> 16);
	memset(buff, 0, sizeof(buff));
	convertAddr(buff, src_add_mode, src_add_bank, src_add_col, src_add_slice);
	return buff;
}

char *  getSrc1Add(INSN_){
	static char buff[20];
	int src1_add_mode = ((x3 & 0xc0000000) >> 30);
	int src1_add_bank = ((x3 & 0x30000000) >> 28);
	int src1_add_col = ((x3 & 0xff80000) >> 19);
	int src1_add_slice = ((x3 & 0x70000) >> 16);
	memset(buff, 0, sizeof(buff));
	convertAddr(buff, src1_add_mode, src1_add_bank, src1_add_col, src1_add_slice);
	return buff;
}

char *  getSrc3Add(INSN_){
	static char buff[20];
	int src3_add_mode = ((x2 & 0xc000) >> 14);
	int src3_add_bank = ((x2 & 0x3000) >> 12);
	int src3_add_col = ((x2 & 0xff8) >> 3);
	int src3_add_slice = ((x2 & 0x7));
	memset(buff, 0, sizeof(buff));
	convertAddr(buff, src3_add_mode, src3_add_bank, src3_add_col, src3_add_slice);
	return buff;
}


char * sptAddInstruction_dis()
{
	char *mod_val, *shift;
	 
	int Immed = !getSrc(INSN_F);
	mod_val = getModVal( INSN_F);
	shift = getShift_s(INSN_F);
	if (Immed){
		sprintf(outbuff, "add %s %s %s, #%s, %s", shift, mod_val, getSrcAdd(INSN_F), getImmDat(INSN_F), getDestAdd(INSN_F));
	}
	else{
		sprintf(outbuff, "add %s %s %s, %s, %s", shift, mod_val, getSrcAdd(INSN_F), getSrc2Add(INSN_F), getDestAdd(INSN_F));
	}

	return  outbuff;
}


char * sptCmpInstruction_dis()
{
	 
	int Immed = !getSrc(INSN_F);
	if (Immed) {
		sprintf(outbuff, "cmp.immed %s, %s, %s", getSrcAdd(INSN_F), getImmDat(INSN_F), getDestAdd(INSN_F));
	}
	else sprintf(outbuff, "cmp %s, %s, %s", getSrcAdd(INSN_F), getSrc2Add(INSN_F), getDestAdd(INSN_F));
		
	return  outbuff;
}


char * sptCopyInstruction_dis(bfd_boolean isBlockVersion)
{
	char cp_type_lst[][20] = { ".simple_copy", ".threshold_ge", ".threshold_lt", ".transpose_copy", ".copy_real_pack", ".copy_imag_pack",
							".copy_unpack", ".partial_copy_real", ".partial_copy_imag", ".partial_copy_r2i", ".partial_copy_i2r", ".copy_clear",
							".copy_shift", ".transpose_fwd", ".transpose_bck" };
	char rst_n_keep_lst[][20] = { ".keep_orig", ".reset_values" };
	char preproc_lst[][15] = { ".no_pre", ".abs_abs_proc", ".abs_mag_proc" };
	//char preproc_lst[][15] = { ".no_pre", ".abs_abs_proc", ".abs_mag_proc" };
	char ind[] = ".ind";
	
	char *src_add, *dest_add, *shft_val, *in_dat_type, *mca_mode, *shft_offs, *fft_rnd;
	int Ind = getInd(INSN_F);
	int cp_type = ((insn[0] & 0xf0000) >> 16);
	int rst_n_keep = ((insn[0] & 0x8000) >> 15);
	int preproc = ((insn[0] & 0xc00000) >> 22);
	int vec_sz = ((insn[0] & 0x1fff));

	int src_add_inc = ((insn[2] & 0xff00) >> 8);
	int dest_add_inc = ((insn[2] & 0xff));

	int blk_src_inc = ((insn[2] & 0xff000000) >> 24);
	int blk_dest_inc = ((insn[2] & 0x7f0000) >> 16);
	int mask = ((insn[3] & 0xffff0000) >> 16);

	in_dat_type = get_in_dattyp_s(INSN_F);
	dest_add = getDestAdd(INSN_F);
	src_add = getSrcAdd(INSN_F);
	/*
	int quad_ext = ((insn[0] & 0x10000) >> 16);
	int cc_im = ((insn[2] & 0xffff0000) >> 16);
	int cc_re = ((insn[3] & 0xffff0000) >> 16);
	int mca_inc = ((insn[3] & 0x1ff0000) >> 16);
	int shft_src = ((insn[3] & 0x80) >> 7);
	int repeat = ((insn[0] & 0xc000) >> 14);
	int adaptv = ((insn[0] & 0x40000) >> 18);
	int adaptv_shft = ((insn[0] & 0x20000) >> 17);
	mca_mode = getMcaMode(INSN_F);
	*/

	int shft_wr = ((insn[3] & 0xfc00) >> 10);
	//case indirect- no dest
	if (getInd(INSN_F)){
		*dest_add = 0;
	}
	else {
		ind[0] = 0;
		strcat(dest_add, ",");
	}

	if(isBlockVersion)
		sprintf(outbuff, "copyb%s %s %s  %s %d %s, %s %d, %d, %d, %d, %d", ind, in_dat_type, cp_type_lst[cp_type], rst_n_keep_lst[rst_n_keep], vec_sz, src_add, dest_add, blk_src_inc, blk_dest_inc, src_add_inc, dest_add_inc, mask);
	else
		sprintf(outbuff, "copy%s %s %s  %s %d %s, %s %d, %d, %d, %d, %d", ind, in_dat_type, cp_type_lst[cp_type], rst_n_keep_lst[rst_n_keep], vec_sz, src_add, dest_add, blk_src_inc, blk_dest_inc, src_add_inc, dest_add_inc, mask);

	
	return  outbuff;
}



char * sptDummyInstruction_dis()
{
 
	sprintf(outbuff, "Unknown instruction code i0 - 0x%x, 0x%x, 0x%x, 0x%x",insn[0],insn[1],insn[2],insn[3]);
	return outbuff;
}

char * sptEvtInstruction_dis()
{
	char buff[10];
	 
	char * ev_lvl_lst[] = { ".low", ".high"};
	char * evt_ev_lst[] = { ".lsb", ".bit1", ".bit2", ".bit3", ".bit4", ".bit5", ".bit6", ".msb" };
	
	int ev_lvl = ((insn[0] & 0x800000) >> 23);
	int evt_ev = ((insn[0]& 0x70000) >> 16);
	sprintf(outbuff,"evt %s %s",ev_lvl_lst[ev_lvl],evt_ev_lst[evt_ev]);	
	return  outbuff;
}



char * sptFirInstruction_dis(bfd_boolean isBlockVersion)
{
	
	char win_type_lst[][20] = { ".cmplx_win", ".cmplx_win", ".real_win_im_tram", ".real_win_real_tram" };
	char init_lst[][20] = { ".zero_init", ".const_init" };
	char ind[] = ".ind";
	
	char *src_add, *dest_add, *shft_val, *in_dat_type, *shft_offs;
	int Ind = getInd(INSN_F);

	int mult_mod = (insn[3] & 0x300) >> 8;//0-none
	int vec_sz = ((insn[0] & 0x1fff));
	int win_type = ((insn[0] & 0x1800000) >> 23);//SPT2!!!!!
	int src_add_inc = ((insn[2] & 0xff00) >> 8);
	int dest_add_inc = ((insn[2] & 0xff));
	int shft_src = ((insn[3] & 0x80) >> 7);
	int no_of_taps = ((insn[0] & 0xf0000) >> 16);
	int init = ((insn[0] & 0x100000) >> 20);
	in_dat_type = get_in_dattyp_s(INSN_F);
	shft_val = get_shift_val(INSN_F);
	dest_add = getDestAdd(INSN_F);
	src_add = getSrcAdd(INSN_F);
	shft_offs = getShftOffset(INSN_F);
	int shft_wr = ((insn[3] & 0xfc00) >> 10);
	//case indirect- no dest
	if (getInd(INSN_F)){
		*dest_add = 0;
	}
	else {
		ind[0] = 0;
		strcat(dest_add, ",");
	}

	char *instr = isBlockVersion? "firb" : "fir";

	if (!shft_src){	
		sprintf(outbuff, "%s%s %s %s %s %s %d %d %s, %s %s, %d, %d", instr, ind, in_dat_type, win_type_lst[win_type], init_lst[init], shft_val, no_of_taps, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
	}
	else
	{
		sprintf(outbuff, "%s.shift_wr%s %s %s %s %s %d %d %d %s, %s %s, %d, %d", instr, ind, in_dat_type, win_type_lst[win_type], init_lst[init], shft_offs, shft_wr, no_of_taps, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
	}
	
	return  outbuff;
}


char * sptGetInstruction_dis()
{

	char *src_add, *dest_add;
	 

	dest_add = getDestAdd( INSN_F);
	src_add = getSrcAdd( INSN_F);
	if (getInd(INSN_F)){
		sprintf(outbuff, "get.ind %s", src_add);
	}	else sprintf(outbuff, "get %s, %s", src_add, dest_add);	
	return  outbuff;
}



char * sptHistInstruction_dis()
{
	 
	char preproc_lst[][15] = { ".no_pre", ".abs_abs_proc", ".abs_mag_proc" };
	char hist_mode_lst[][15] = { ".read_store", ".read", ".acc", ".store" };
	char bin_sz_lst[][12] = { ".16_24bins", ".32bins", ".64bins" };
	char pack_en_lst[][20] = { ".no_pack_real", ".no_pack_im",".pack" };
	char dat_sz_lst[][20] = { ".16bits", ".24bits" };
	char ind[] = ".ind";
	
	char *src_add, *dest_add, *in_dat_type;
	int Ind = getInd(INSN_F);

	int loc = ((insn[0] & 0x80000) >> 19);
	int preproc = ((insn[0] & 0xc00000) >> 22);
	int hist_mode = ((insn[0] & 0x300000) >> 20);
	int bin_sz = ((insn[0] & 0xc0000) >> 18);
	int pack_en = ((insn[0] & 0xc000) >> 14);
	int dat_sz = ((insn[3] & 0x1));
	int vec_sz = ((insn[0] & 0x1fff));
	int src_add_inc = ((insn[2] & 0xff00) >> 8);
	int dest_add_inc = ((insn[2] & 0xff));
	int thld_add = ((insn[3] & 0xffff));

	in_dat_type = get_in_dattyp_s(INSN_F);
	dest_add = getDestAdd(INSN_F);
	src_add = getSrcAdd(INSN_F);

	//case indirect- no dest
	if (getInd(INSN_F)){
		*dest_add = 0;
	}
	else {
		ind[0] = 0;
		strcat(dest_add, ",");
	}
	sprintf(outbuff, "hist%s %s %s %s %s %s %s %d %s, %s %d, %d, 0x%x", ind, in_dat_type, preproc_lst[preproc], hist_mode_lst[hist_mode], bin_sz_lst[bin_sz], pack_en_lst[pack_en], dat_sz_lst[dat_sz], vec_sz, src_add, dest_add, src_add_inc, dest_add_inc, thld_add);
	return  outbuff;
}


char * sptIrdx2Instruction_dis(bfd_boolean isBlockVersion)
{
	 
	char quad_ext_lst[][10] = { ".noqext", ".qext" };
	char repeat_lst[][12] = { ".no_repeat",".fft8", ".fft16", ".fft32" };
	char adaptv_shft_lst[][10] = { ".15down",".23down" };
	char ind[] = ".ind";	
	char *src_add, *dest_add, *shft_val, *in_dat_type, *mca_mode, *shft_offs,*fft_rnd;
	int Ind = getInd(INSN_F);	
	int mult_mod = (insn[3] & 0x300) >> 8;//0-none
	int vec_sz = ((insn[0] & 0x1fff));
	int quad_ext = ((insn[0] & 0x10000) >> 16);
	int src_add_inc = ((insn[2] & 0xff00) >> 8);
	int dest_add_inc = ((insn[2] & 0xff));
	int repeat = ((insn[0] & 0xc000) >> 14);
	int adaptv = ((insn[0]&0x40000)>>18);
	int adaptv_shft = ((insn[0] & 0x20000) >> 17);
	int shft_src = ((insn[3] & 0x80) >> 7);
	fft_rnd = getFftRnd(INSN_F);
	mca_mode = getMcaMode(INSN_F);
	in_dat_type = get_in_dattyp_s(INSN_F);
	shft_val = get_shift_val(INSN_F);
	dest_add = getDestAdd(INSN_F);	
	src_add = getSrcAdd(INSN_F);
	shft_offs = getShftOffset(INSN_F);
	int shft_wr = ((insn[3] & 0xfc00) >> 10);
	//case indirect- no dest
	if (getInd(INSN_F)){
		*dest_add = 0;
	}
	else {
		ind[0] = 0;
		strcat(dest_add, ",");
	}

	char* instr = isBlockVersion? "irdx2b" : "irdx2";

	if (adaptv){
		if (!shft_src){
			sprintf(outbuff, "%s.adaptv%s %s %s %s %s %s %s  %s %s %d %d %s, %s %s, %d, %d", instr, ind, in_dat_type, quad_ext_lst[quad_ext], getTwOvs(INSN_F), fft_rnd, repeat_lst[repeat], shft_val, adaptv_shft_lst[adaptv_shft], getShftOffset(INSN_F), shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
		}
		else sprintf(outbuff, "%s.adaptv.shift_wr%s %s %s %s %s %s %s  %s %s %d %d %s, %s %s, %d, %d", instr, ind, in_dat_type, quad_ext_lst[quad_ext], getTwOvs(INSN_F), fft_rnd, repeat_lst[repeat], shft_val, adaptv_shft_lst[adaptv_shft], getShftOffset(INSN_F), shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
	}
	else{
		if (!shft_src){
			sprintf(outbuff, "%s%s %s %s %s %s %s %s %d %s, %s %s, %d, %d",instr , ind, in_dat_type, quad_ext_lst[quad_ext], getTwOvs(INSN_F), fft_rnd, repeat_lst[repeat], shft_val, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
		}
		else
		{
			sprintf(outbuff, "%s.shift_wr %s %s %s %s %s %s %d %d %s, %s, %s, %d, %d", instr, in_dat_type, quad_ext_lst[quad_ext], getTwOvs(INSN_F), fft_rnd, repeat_lst[repeat], shft_offs, shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
		}
	}	
	return  outbuff;
}



char * sptIrdx4Instruction_dis(bfd_boolean isBlockVersion)
{
	 
	char quad_ext_lst[][10] = { ".noqext", ".qext" };
	char repeat_lst[][12] = { ".no_repeat", ".fft8", ".fft16", ".fft32" };
	char adaptv_shft_lst[][10] = { ".15down", ".23down" };
	char win_type_lst[][20] = { ".no_win", ".cmplx_win", ".real_win_im_tram", ".real_win_real_tram" };
	char mult_mod_lst[][10] = { ".immed", ".const", ".caddr" };
	char ind[] = ".ind";
	
	char *src_add, *dest_add, *shft_val, *in_dat_type, *mca_mode, *shft_offs, *fft_rnd;
	int Ind = getInd(INSN_F);
	
	int mult_mod = (insn[3] & 0x300) >> 8;//0-none
	int vec_sz = ((insn[0] & 0x1fff));
	int win_type = ((insn[0] & 0x1800000) >> 23);//SPT2!!!!!
	int src_add_inc = ((insn[2] & 0xff00) >> 8);
	int dest_add_inc = ((insn[2] & 0xff));
	int quad_ext = ((insn[0] & 0x10000) >> 16);
	int cc_im = ((insn[2] & 0xffff0000) >> 16);
	int cc_re = ((insn[3] & 0xffff0000) >> 16);
	int mca_inc = ((insn[3] & 0x1ff0000) >> 16);
	int shft_src = ((insn[3] & 0x80) >> 7);
	int repeat = ((insn[0] & 0xc000) >> 14);
	int adaptv = ((insn[0] & 0x40000) >> 18);
	int adaptv_shft = ((insn[0] & 0x20000) >> 17);
	mca_mode = getMcaMode(INSN_F);
	in_dat_type = get_in_dattyp_s(INSN_F);
	shft_val = get_shift_val(INSN_F);
	dest_add = getDestAdd(INSN_F);
	src_add = getSrcAdd(INSN_F);
	shft_offs = getShftOffset(INSN_F);
	fft_rnd = getFftRnd(INSN_F);
	int shft_wr = ((insn[3] & 0xfc00) >> 10);
	//case indirect- no dest
	if (getInd(INSN_F)){
		*dest_add = 0;
	}
	else {
		ind[0] = 0;
		strcat(dest_add, ",");
	}

	char* instr = isBlockVersion ? "irdx4b" : "irdx4";

	if (adaptv){
		sprintf(outbuff, "%s.adaptv%s %s %s %s %s %s %s %s %s %d %d %s, %s %s, %d, %d, %s, %d", instr, ind, mult_mod_lst[mult_mod], in_dat_type, win_type_lst[win_type], repeat_lst[repeat], quad_ext_lst[quad_ext], getTwOvs(INSN_F), adaptv_shft_lst[adaptv_shft], shft_offs, shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc, mca_mode, mca_inc);
	}
	else
	{
	if (!shft_src){
		if (mult_mod == 0)
		{
			sprintf(outbuff, "%s%s %s %s %s %s %s %s %s %d %s, %s %d, %d, %d, %d", instr, ind, in_dat_type, win_type_lst[win_type], repeat_lst[repeat], quad_ext_lst[quad_ext], getTwOvs(INSN_F), fft_rnd, shft_val, vec_sz, src_add, dest_add, src_add_inc, dest_add_inc, cc_im, cc_re);
		}
		else//selected
		{
			sprintf(outbuff, "%s%s %s %s %s %s %s %s %s %d %s, %s %d, %d, %s, %d", instr, ind, mult_mod_lst[mult_mod], in_dat_type, win_type_lst[win_type], repeat_lst[repeat], quad_ext_lst[quad_ext], getTwOvs(INSN_F), fft_rnd, vec_sz, src_add, dest_add, src_add_inc, dest_add_inc, mca_mode, mca_inc);
		}
	}
	else
	{
		sprintf(outbuff, "%s.shift_wr%s %s %s %s %s %s %s %s %d %d %s, %s %s, %d, %d, %s, %d", instr, ind, mult_mod_lst[mult_mod], in_dat_type, win_type_lst[win_type], repeat_lst[repeat], quad_ext_lst[quad_ext], getTwOvs(INSN_F), shft_offs, shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc, mca_mode, mca_inc);
	}
}
	
	return  outbuff;
}



char * sptJumpInstruction_dis()
{

	char *src_add, *dest_add;
	 
	int jmp_type = ((insn[0] & 0x2000000) >> 25);
	int jmp_sel = ((insn[0] & 0x1e00000) >> 21);
	int	jmp_wr = ((insn[1] & 0x1f80000) >> 19);
	int jmp_addr = ((insn[2] & 0xffffffff));
	if ((jmp_type == 0x0) && (jmp_sel == 0x0) && (jmp_wr == 0x0)) {
		sprintf(outbuff, "jumpa  %d", jmp_addr);
	}
	else{
		sprintf(outbuff, "jumpc .bit%d, WR_%d %d", jmp_sel, jmp_wr, jmp_addr);
	}

	return  outbuff;
}


char * sptLoopInstruction_dis()
{
	 	
	sprintf(outbuff, "loop %d", (insn[0] & 0xffff));
	return  outbuff;
}


char * sptNextInstruction_dis()
{  
sprintf(outbuff, "next");
	return  "next";
}


char * sptMaxsInstruction_dis(bfd_boolean isBlockVersion)
{
	 
	char preproc_lst[][15] = { ".no_pre",  ".abs_abs_proc",".abs_mag_proc" };
	char thld_cmp_lst[][15] = { ".no_thld_cmp", ".thld_cmp" };
	char in_tag_lst[][12] = { ".no_tag_in", ".tag_in" };
	char tag_n_lst[][20] = { ".packed_bitfld", ".tagged_vect" };
	char cyc_extn_lst[][15] = { ".no_cyc_ext", ".cyc_ext"};
	char in_pack_lst[][15] = { ".in_24real", ".in_24im", ".in_48packed" };
	char maxsn_sel_lst[][10] = { ".no_maxsn", ".maxsn_16", ".maxsn_8",".maxsn_4" };

	char ind[] = ".ind";
	
	char *src_add, *dest_add, *in_dat_type;
	int Ind = getInd(INSN_F);
	
	int loc = ((insn[0] & 0x80000) >> 19);
	int preproc = ((insn[0] & 0xc00000) >> 22);
	int thld_cmp = ((insn[0] & 0x200000) >> 21);
	int in_tag = ((insn[0] & 0x100000) >> 20);
	int tag_n = ((insn[0] & 0x40000) >> 18);
	int cyc_extn = ((insn[0] & 0x20000) >> 17);
	int in_pack = ((insn[0] & 0xc000) >> 14);
	int maxsn_sel = ((insn[3] & 0x30000) >> 16);
	int vec_sz = ((insn[0] & 0x1fff));
	int src_add_inc = ((insn[2] & 0xff00) >> 8);
	int dest_add_inc = ((insn[2] & 0xff));
	int thld_add = ((insn[3] & 0xffff));


	in_dat_type = get_in_dattyp_s(INSN_F);

	dest_add = getDestAdd(INSN_F);
	src_add = getSrcAdd(INSN_F);

	//case indirect- no dest
	if (getInd(INSN_F)){
		*dest_add = 0;
	}
	else {
		ind[0] = 0;
		strcat(dest_add, ",");
	}

	char *instr = isBlockVersion? "maxsb" : "maxs";

	if (loc)
			{
				sprintf(outbuff, "%s%s %s %s %s %s %s %s %s %s %d %s, %s %d, %d, 0x%x", instr, ind, in_dat_type, preproc_lst[preproc], thld_cmp_lst[thld_cmp], in_tag_lst[in_tag], tag_n_lst[tag_n], cyc_extn_lst[cyc_extn], in_pack_lst[in_pack], maxsn_sel_lst[maxsn_sel], vec_sz, src_add, dest_add, src_add_inc, dest_add_inc, thld_add);
			}
			else//selected
			{
				sprintf(outbuff, "%s.gbl%s %s %s %s %s %d %s, %s %d, %d, 0x%x", instr, ind, in_dat_type, preproc_lst[preproc], in_pack_lst[in_pack], maxsn_sel_lst[maxsn_sel], vec_sz, src_add, dest_add, src_add_inc, dest_add_inc, thld_add);
			}
	return  outbuff;
}


char * sptPdmaInstruction_dis()
{
	 
	char se_lst[][10] = { ".zeropad", ".signext", };
	char pdma_tag_lst[][10] = { ".notag", ".tag"};
	char data_packing_lst[][15] = { ".16cmplx", ".24cmplx", ".24real", ".16real", ".48bin", ".16swap", ".16clubbing", ".cp4", ".cp6",".cp8", ".cp16", ".cp4d",
		".abs", ".idx", ".idxsum", ".cp4fmta", ".cp4dfmta", ".cp4fmtb", ".cp4dfmtb", ".cp8fmtb", ".cp16fmtb" };
	char trans_type_lst[][15] = { ".sysram2opram", ".opram2sysram" };
	char sync_async_lst[][10] = { ".async", ".sync" };
	int se = ((insn[0] & 0x1000000) >> 24);
	int pdma_tag = ((insn[0] & 0x2000000) >> 25);
	int data_packing = ((insn[0] & 0x7c0000) >> 18);
	int trans_type = ((insn[0] & 0x20000) >> 17);
	int sync_async = ((insn[0] & 0x10000) >> 16);
	int sysram_mem_start_addr = ((insn[1] & 0xffffffff));
	int opram_skip_addr = ((insn[2] & 0xfff0) >> 4);
	int opram_cont_addr = ((insn[2] & 0xf) << 8) | ((insn[3] & 0xff000000) >> 24);
	int sysram_skip_addr = ((insn[3] & 0xfff000) >> 12);
	int sysram_cont_addr = ((insn[3] & 0xfff));
	int vec_sz = ((insn[0] & 0x1fff));
	int ram_wr = ((insn[3] & 0xfc00) >> 10);

	if (!getInd(INSN_F)){
		sprintf(outbuff, "pdma %s %s %s %s %s %d %d, %s, %d, %d, %d, %d", se_lst[se], pdma_tag_lst[pdma_tag], data_packing_lst[data_packing], trans_type_lst[trans_type], 
			sync_async_lst[sync_async], vec_sz, sysram_mem_start_addr, getMultCoefAdd(INSN_F), opram_skip_addr, opram_cont_addr, sysram_skip_addr, sysram_cont_addr);
	}
	else {
		sprintf(outbuff, "pdma.ind %s %s %s %s %s %d %d, %d, %d, %d, %d, %d,", se_lst[se], pdma_tag_lst[pdma_tag], data_packing_lst[data_packing], trans_type_lst[trans_type],
			sync_async_lst[sync_async], vec_sz, sysram_mem_start_addr, ram_wr, opram_skip_addr, opram_cont_addr, sysram_skip_addr, sysram_cont_addr);
	}		
	return  outbuff;
}



char * sptRdx2Instruction_dis(bfd_boolean isBlockVersion)
{
	 
	char quad_ext_lst[][10] = { ".noqext", ".qext" };
	char repeat_lst[][12] = { ".no_repeat", ".fft8", ".fft16", ".fft32" };
	char adaptv_shft_lst[][10] = { ".15down", ".23down" };
	char real_fft_lst[][10] = { ".nosplit",".opsplit" };

	char ind[] = ".ind";
	char *src_add, *dest_add, *shft_val, *in_dat_type, *mca_mode, *shft_offs, *fft_rnd;
	int Ind = getInd(INSN_F);
	int real_fft = ((insn[0] & 0x80000) >> 19);
	int vec_sz = ((insn[0] & 0x1fff));
	int src_add_inc = ((insn[2] & 0xff00) >> 8);
	int dest_add_inc = ((insn[2] & 0xff));
	int quad_ext = ((insn[0] & 0x10000) >> 16);
	int mca_inc = ((insn[3] & 0x1ff0000) >> 16);
	int shft_src = ((insn[3] & 0x80) >> 7);
	int repeat = ((insn[0] & 0xc000) >> 14);
	int adaptv = ((insn[0] & 0x40000) >> 18);
	int adaptv_shft = ((insn[0] & 0x20000) >> 17);
	mca_mode = getMcaMode(INSN_F);
	in_dat_type = get_in_dattyp_s(INSN_F);
	shft_val = get_shift_val(INSN_F);
	dest_add = getDestAdd(INSN_F);
	src_add = getSrcAdd(INSN_F);
	shft_offs = getShftOffset(INSN_F);
	fft_rnd = getFftRnd(INSN_F);
	int shft_wr = ((insn[3] & 0xfc00) >> 10);
	//case indirect- no dest
	if (getInd(INSN_F)){
		*dest_add = 0;
	}
	else {
		ind[0] = 0;
		strcat(dest_add, ",");
	}

	char *instr = isBlockVersion? "rdx2b" : "rdx2";

	if (adaptv){
		if (!shft_src){
			sprintf(outbuff, "%s.adaptv%s %s %s %s %s %s %s %s %s %s %d %d %s, %s %s %d, %d", instr, ind, in_dat_type, quad_ext_lst[quad_ext], getTwOvs(INSN_F), fft_rnd, repeat_lst[repeat], real_fft_lst[real_fft], shft_val, adaptv_shft_lst[adaptv_shft], getShftOffset(INSN_F), shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
		}
		else
		{//shft_wr
			sprintf(outbuff, "%s.adaptv.shift_wr%s %s %s %s %s %s %s %s %d %d %s, %s %s %d, %d", instr, ind, in_dat_type, quad_ext_lst[quad_ext], getTwOvs(INSN_F), repeat_lst[repeat], real_fft_lst[real_fft],  adaptv_shft_lst[adaptv_shft], getShftOffset(INSN_F), shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
		}
	}
	else
	{
		if (!shft_src){

			sprintf(outbuff, "%s%s %s %s %s %s %s %s %s %d %s, %s %s %d, %d", instr, ind, in_dat_type, quad_ext_lst[quad_ext], getTwOvs(INSN_F), fft_rnd, repeat_lst[repeat], real_fft_lst[real_fft], shft_val, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
		}
		else//selected
		{
			sprintf(outbuff, "%s.shift_wr%s %s %s %s %s %s %s %s %d %d %s, %s %s %d, %d", instr, ind, in_dat_type, quad_ext_lst[quad_ext], getTwOvs(INSN_F), repeat_lst[repeat], real_fft_lst[real_fft], adaptv_shft_lst[adaptv_shft], getShftOffset(INSN_F), shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
		}
	}
	return  outbuff;
}



char * sptRdx4Instruction_dis(bfd_boolean isBlockVersion)
{
	 
	char quad_ext_lst[][10] = { ".noqext", ".qext" };
	char repeat_lst[][12] = { ".no_repeat", ".fft8", ".fft16", ".fft32" };
	char adaptv_shft_lst[][10] = { ".15down", ".23down" };
	char win_type_lst[][20] = { ".no_win", ".cmplx_win", ".real_win_im_tram", ".real_win_real_tram" };
	char mult_mod_lst[][10] = { ".immed", ".const", ".caddr" };
	char ind[] = ".ind";

	char *src_add, *dest_add, *shft_val, *in_dat_type, *mca_mode, *shft_offs, *fft_rnd;
	int Ind = getInd(INSN_F);
	int mult_mod = (insn[3] & 0x300) >> 8;//0-none
	int vec_sz = ((insn[0] & 0x1fff));
	int win_type = ((insn[0] & 0x1800000) >> 23);//SPT2!!!!!
	int src_add_inc = ((insn[2] & 0xff00) >> 8);
	int dest_add_inc = ((insn[2] & 0xff));
	int quad_ext = ((insn[0] & 0x10000) >> 16);
	int cc_im = ((insn[2] & 0xffff0000) >> 16);
	int cc_re = ((insn[3] & 0xffff0000) >> 16);
	int mca_inc = ((insn[3] & 0x1ff0000) >> 16);
	int shft_src = ((insn[3] & 0x80) >> 7);
	int repeat = ((insn[0] & 0xc000) >> 14);
	int adaptv = ((insn[0] & 0x40000) >> 18);
	int adaptv_shft = ((insn[0] & 0x20000) >> 17);
	mca_mode = getMcaMode(INSN_F);
	in_dat_type = get_in_dattyp_s(INSN_F);
	shft_val = get_shift_val(INSN_F);
	dest_add = getDestAdd(INSN_F);
	src_add = getSrcAdd(INSN_F);
	shft_offs = getShftOffset(INSN_F);
	fft_rnd = getFftRnd(INSN_F);
	int shft_wr = ((insn[3] & 0xfc00) >> 10);
	//case indirect- no dest
	if (getInd(INSN_F)){
		*dest_add = 0;
	}
	else {
		ind[0] = 0;
		strcat(dest_add, ",");
	}

	char *instr = isBlockVersion ? "rdx4b" : "rdx4";

	if (adaptv){
		if (!shft_src){
			sprintf(outbuff, "%s.adaptv%s %s %s %s %s %s %s %s %s %d %d %s, %s %s, %d, %d, %s, %d", instr, ind, mult_mod_lst[mult_mod], in_dat_type, win_type_lst[win_type], repeat_lst[repeat], quad_ext_lst[quad_ext], getTwOvs(INSN_F), adaptv_shft_lst[adaptv_shft], shft_offs, shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc, mca_mode, mca_inc);
		}
		else{
			sprintf(outbuff, "%s.adaptv.shift_wr%s %s %s %s %s %s %s %s %s %d %d %s, %s %s, %d, %d, %s, %d", instr, ind, mult_mod_lst[mult_mod], in_dat_type, win_type_lst[win_type], repeat_lst[repeat], quad_ext_lst[quad_ext], getTwOvs(INSN_F), adaptv_shft_lst[adaptv_shft], shft_offs, shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc, mca_mode, mca_inc);

		}
	}
	else
	{
		if (!shft_src){
			if (mult_mod == 0)
			{
				sprintf(outbuff, "%s%s %s %s %s %s %s %s %s %d %s, %s %d, %d, %d, %d", instr, ind, in_dat_type, win_type_lst[win_type], repeat_lst[repeat], quad_ext_lst[quad_ext], getTwOvs(INSN_F), fft_rnd, shft_val, vec_sz, src_add, dest_add, src_add_inc, dest_add_inc, cc_im, cc_re);
			}
			else//selected
			{
				sprintf(outbuff, "%s%s %s %s %s %s %s %s %s %d %s, %s %d, %d, %s, %d", instr, ind, mult_mod_lst[mult_mod], in_dat_type, win_type_lst[win_type], repeat_lst[repeat], quad_ext_lst[quad_ext], getTwOvs(INSN_F), fft_rnd, vec_sz, src_add, dest_add, src_add_inc, dest_add_inc, mca_mode, mca_inc);
			}
		}
		else
		{
			sprintf(outbuff, "%s.shift_wr%s %s %s %s %s %s %s %s %d %d %s, %s %s, %d, %d, %s, %d", instr, ind, mult_mod_lst[mult_mod], in_dat_type, win_type_lst[win_type], repeat_lst[repeat], quad_ext_lst[quad_ext], getTwOvs(INSN_F), shft_offs, shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc, mca_mode, mca_inc);
		}
	}	
	return  outbuff;
}



char * sptScpInstruction_dis(bfd_boolean isBlockVersion)
{
	 
	char re_im_coeff_lst[][20] = { ".coef_cmplx", ".coef_cmplx", ".coef_im", ".coef_re" };
	char ind[] = ".ind";
	char *src_add, *dest_add, *shft_val, *in_dat_type, *shft_offs;
	int re_im_coeff = ((insn[0] & 0x1800000) >> 23);
	int vec_sz = ((insn[0] & 0x1fff));
	int src_add_inc = ((insn[2] & 0xff00) >> 8);
	int dest_add_inc = ((insn[2] & 0xff));
	int shft_src = ((insn[3] & 0x80) >> 7);
	int num_of_taps=((insn[0] & 0xf0000) >> 16);
	
	in_dat_type = get_in_dattyp_s(INSN_F);
	shft_val = get_shift_val(INSN_F);
	dest_add = getDestAdd(INSN_F);
	src_add = getSrcAdd(INSN_F);
	shft_offs = getShftOffset(INSN_F);
	int shft_wr = ((insn[3] & 0xfc00) >> 10);
	//case indirect- no dest
	if (getInd(INSN_F)){
		*dest_add = 0;
	}
	else {
		ind[0] = 0;
		strcat(dest_add, ",");
	}

	char* instr = isBlockVersion ? "scpb" : "scp";

	if (!shft_src){
		sprintf(outbuff, "%s%s %s  %s %s %d %d %s, %s %s, %d, %d", instr, ind, in_dat_type, re_im_coeff_lst[re_im_coeff], shft_val, num_of_taps,
			vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
	}
	else
	{
		sprintf(outbuff, "%s.shift_wr%s %s  %s %s %d %d %d %s, %s %s, %d, %d", instr, ind, in_dat_type, re_im_coeff_lst[re_im_coeff], shft_offs, shft_wr, num_of_taps,
			vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
	}
	return  outbuff;
}



char * sptSelInstruction_dis()
{	
	char *mod_val, *shift;
	 
	char re_im_lst[][10] = { ".im",".re" };
	int re_im = ((insn[0] & 0x2000000) >> 25);
	int sel_wr = ((insn[1] & 0x3f0000) >> 16);
	mod_val = getModVal(INSN_F);
	shift = getShift_s(INSN_F);
	sprintf(outbuff, "sel %s WR_%d  %s %s, %s, %s", re_im_lst[re_im], sel_wr, getDestAdd(INSN_F), getSrc1Add(INSN_F), getSrc2Add(INSN_F), getSrc3Add(INSN_F));
	return  outbuff;

}


char * sptSetInstruction_dis()
{
		 
	if (getInd(INSN_F)){
		sprintf(outbuff, "set.ind %s", getSrcAddWr(INSN_F));
	}
	if (!getSrc(INSN_F)){
		sprintf(outbuff, "set.immed %s, %s", getImmDat(INSN_F), getDestAdd(INSN_F));
	}
	else
	{
		sprintf(outbuff, "set %s, %s", getSrcAddWr(INSN_F), getDestAdd(INSN_F));
	}
	return  outbuff;
}
	

char * sptStopInstruction_dis()
{
	// 
	//sprintf(outbuff,"stop");
	return "stop";
}
	


char * sptSubInstruction_dis()
{	
	char *mod_val, *shift;
	 
	int Immed = !getSrc(INSN_F);
	mod_val = getModVal(INSN_F);
	shift = getShift_s(INSN_F);
	if (Immed){
		sprintf(outbuff, "sub %s %s %s, #%s, %s", shift, mod_val, getSrcAdd(INSN_F), getImmDat(INSN_F), getDestAdd(INSN_F));
	}
	else{
		sprintf(outbuff, "sub %s %s %s, %s, %s", shift, mod_val, getSrcAdd(INSN_F), getSrc2Add(INSN_F), getDestAdd(INSN_F));
	}
	return  outbuff;
}



char * sptWaitInstruction_dis()
{
	
	 
	char * ev_tr_names[4] = { ".l0", ".l1", ".pos", ".neg" };
	int ev_tr = ((insn[0] & 0x3000000) >> 24);
	int ev = ((insn[0] & 0x1f0000) >> 16);
	sprintf(outbuff, "wait %s %d " , ev_tr_names[ev_tr], ev );
	return  outbuff;
}


char * sptSyncInstruction_dis()
{
	int sync_type=((insn[0] & 0x3000000) >> 24);
	int cs_id = ((insn[3] & 0x1f));
	switch (sync_type){
	case 0: sprintf(outbuff, "sync");
		  break;
	case 1:sprintf(outbuff, "sync.pdma %d ", cs_id);
		  break;
	case 2:sprintf(outbuff, "sync.thread %d ", cs_id);
		break;
	case 3:sprintf(outbuff, "sync.dsp");
		 break;
	}
	return  outbuff;

}

char * sptThreadInstruction_dis()
{
	int th_id = (insn[3] & 0xf) >> 1;
	if (th_id > 2) th_id = 3;
	sprintf(outbuff, "thread .thd_scs%d", th_id);
	return outbuff;
}


char * sptVmtInstruction_dis()
{
	 
	char  rst_acc_lst[][15] = { ".no_rst", ".rst_sum" };
	char ip_pack_lst[][15] = { ".in_24real", ".in_24im", ".in_48" };
	char op_pack_lst[][12] = { ".op_off", ".op_on" };
	char opsq1_lst[][20] = { ".no_sq1", ".sq1" };
	char opsq2s1_lst[][15] = { ".no_sq2s1", ".abs_sq2s1", ".mag_sq2s1", ".conj_sq2s1" };
	char opsq2s2_lst[][15] = { ".no_sq2s2", ".shft_sq2s2", ".wr_off_sq2s2", ".imm_off_sqs2", ".vec_sq2s2" };
	char opsq2s3_lst[][15] = { ".no_sq2s3", ".sum_sq2s3"};
	char ind[] = ".ind";
	
	char *src_add, *dest_add, *in_dat_type;
	int rst_acc = ((insn[0] & 0x2000000) >> 25);
	int  ip_pack = ((insn[0] & 0xc000) >> 14);
	int op_pack = ((insn[0] & 0x10000) >> 16);
	int opsq1 = ((insn[0] & 0x800000) >> 23);
	int opsq2s1 = ((insn[0] & 0x600000) >> 21);
	int opsq2s2 = ((insn[0] & 0x1c0000) >> 18);
	int opsq2s3 = ((insn[0] & 0x20000) >> 17);
	int vec_sz = ((insn[0] & 0x1fff));
	int src_add_inc = ((insn[2] & 0xff00) >> 8);
	int dest_add_inc = ((insn[2] & 0xff)); 
	int imdt_offset_val_im = ((insn[3] & 0xffff0000) >> 16);
	int extra_offset_val_im = ((insn[3] & 0xffff));
	in_dat_type = get_in_dattyp_s(INSN_F);
	dest_add = getDestAdd(INSN_F);
	src_add = getSrcAdd(INSN_F);

	//case indirect- no dest
	if (getInd(INSN_F)){
		*dest_add = 0;
	}
	else {
		ind[0] = 0;
		strcat(dest_add, ",");
	}
	switch (opsq2s2){

	case 0:
		sprintf(outbuff, "vmt%s %s %s %s %s %s %s %s %d %s, %s %s, %d, %d", ind, in_dat_type, rst_acc_lst[rst_acc], ip_pack_lst[ip_pack], op_pack_lst[op_pack], opsq1_lst[opsq1],
			opsq2s1_lst[opsq2s1], opsq2s3_lst[opsq2s3], vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc);
		break;
	case 1:
		sprintf(outbuff, "vmt.shift_sq2s2%s %s %s %s %s %s %s %s %d %s, %s %s, %d, %d, %d", ind, in_dat_type, rst_acc_lst[rst_acc], ip_pack_lst[ip_pack], op_pack_lst[op_pack], opsq1_lst[opsq1],
			opsq2s1_lst[opsq2s1], opsq2s3_lst[opsq2s3], vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc, extra_offset_val_im);
		break;
	case 2:
		sprintf(outbuff, "vmt.wr_off_sq2s2%s %s %s %s %s %s %s %s %d %s, %s %s, %d, %d, %d", ind, in_dat_type, rst_acc_lst[rst_acc], ip_pack_lst[ip_pack], op_pack_lst[op_pack], opsq1_lst[opsq1],
			opsq2s1_lst[opsq2s1], opsq2s3_lst[opsq2s3], vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc, extra_offset_val_im);
		break;
	case 3:
		sprintf(outbuff, "vmt.imm_off_sq2s2%s %s %s %s %s %s %s %s %d %s, %s %s, %d, %d, %d, %d", ind, in_dat_type, rst_acc_lst[rst_acc], ip_pack_lst[ip_pack], op_pack_lst[op_pack], opsq1_lst[opsq1],
			opsq2s1_lst[opsq2s1], opsq2s3_lst[opsq2s3], vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc, imdt_offset_val_im, extra_offset_val_im);
		break;
	case 4:
		sprintf(outbuff, "vmt.vec_sq2s2%s %s %s %s %s %s %s %s %d %s, %s %s, %d, %d, %d", ind, in_dat_type, rst_acc_lst[rst_acc], ip_pack_lst[ip_pack], op_pack_lst[op_pack], opsq1_lst[opsq1],
			opsq2s1_lst[opsq2s1], opsq2s3_lst[opsq2s3], vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc, extra_offset_val_im);
		break;

	}
	return  outbuff;
}



char * sptWatchdogInstruction_dis()
{
	 
	char ev_tr_lst[][10] = { ".l0", ".l1", ".pos", ".neg" };
	char op_lst[][10] = { ".start", ".stop", ".reset", ".event" };
	char ev_lst[][10] = { ".ev0", ".ev1", ".ev2", ".ev3", ".hsync", ".vsync" };
	int ev_tr = ((insn[0] & 0x3000000) >> 24);
	int op = ((insn[0] & 0x300000) >> 20);
	int ev = ((insn[0] & 0x70000) >> 16);
	int cnt_in_val = ((insn[3] & 0xffffff));

	sprintf(outbuff, "watchdog %s %s %s %d", ev_tr_lst[ev_tr], op_lst[op], ev_lst[ev], cnt_in_val);
	return  outbuff;
}


char * sptWinInstruction_dis(bfd_boolean isBlockVersion)
{
	 
	char win_type_lst[][20] = { ".cmplx_win", ".cmplx_win", ".real_win_im_tram", ".real_win_real_tram" };
	char mult_mod_lst[][10] = { ".immed", ".const", ".caddr" };
	char ind[] = ".ind";
	char *src_add, *dest_add, *shft_val, *in_dat_type, *mca_mode, *shft_offs;
	int Ind = getInd(INSN_F);
	int mult_mod = (insn[3] & 0x300)>>8;//0-none
	int vec_sz = ((insn[0] & 0x1fff));
	int win_type = ((insn[0] & 0x1800000) >> 23);//SPT2!!!!!
	int src_add_inc = ((insn[2] & 0xff00) >> 8);
	int dest_add_inc = ((insn[2] & 0xff));
	int cc_im = ((insn[2] & 0xffff0000) >> 16);
	int cc_re = ((insn[3] & 0xffff0000) >> 16);
	int mca_inc = ((insn[3] & 0x1ff0000) >> 16);
	int shft_src = ((insn[3] & 0x80) >> 7);
	mca_mode = getMcaMode(INSN_F);
	in_dat_type = get_in_dattyp_s(INSN_F);
	shft_val = get_shift_val( INSN_F);
	dest_add = getDestAdd( INSN_F);
	src_add = getSrcAdd( INSN_F);
	shft_offs = getShftOffset(INSN_F);
	int shft_wr = ((insn[3] & 0xfc00) >> 10);
	//case indirect- no dest
	if (getInd(INSN_F)){
		*dest_add = 0;
	}
	else {
		ind[0] = 0;
		strcat(dest_add, ",");
	}

	char *instr = isBlockVersion? "winb" : "win";

	if (!shft_src){
		if (mult_mod == 0)
		{
		 sprintf(outbuff, "%s%s %s %s %s %d %s, %s %d, %d, %d, %d",instr, ind, in_dat_type, win_type_lst[win_type], shft_val, vec_sz, src_add, dest_add, src_add_inc, dest_add_inc, cc_im, cc_re);
		}
		else//selected
		{
		 sprintf(outbuff, "%s%s %s %s %s %s %d %s, %s, %s, %d, %d, %s, %d",instr, ind, mult_mod_lst[mult_mod], in_dat_type, win_type_lst[win_type], shft_val, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc, mca_mode, mca_inc);
		}
	}
	else
	{
		 sprintf(outbuff, "%s.shift_wr%s %s %s %s %s %d %d %s, %s %s, %d, %d, %s, %d",instr, ind, mult_mod_lst[mult_mod], in_dat_type, win_type_lst[win_type], shft_offs, shft_wr, vec_sz, src_add, dest_add, getMultCoefAdd(INSN_F), src_add_inc, dest_add_inc, mca_mode, mca_inc);
	}
	return  outbuff;
}

char * sptDspInstruction_dis()
{
	char* isBlocking = ((insn[0] >> 25) & 0x1) ? ".blocking" : ".non-blocking";
	sprintf(outbuff, "dsp %s 0x%x 0x%x 0x%x 0x%x",isBlocking, (insn[0] & 0x1FFFFFF), insn[1], insn[2], insn[3]);
	return outbuff;
}

char * sptRepeatInstruction_dis()
{
	int rptCnt = insn[0] & 0x1FFF;
	sprintf(outbuff, "repeat 0x%x 0x%x 0x%x 0x%x 0x%x",rptCnt, (insn[2] >> 16) & 0xFFFF, insn[2] & 0xFFFF, (insn[3] >> 16) & 0xFFFF, insn[3] & 0xFFFF);
	return outbuff;
}

char * sptSortInstruction_dis(bfd_boolean isBlockVersion)
{	char *reserved = "";
	char *instr = isBlockVersion? "sortb" : "sort";
	char *in_dattyp[4] = { ".real", ".cmplx", ".log2", reserved};
	char *preproc[4] = {".no_preproc",".abs_proc",".mag_proc",reserved};
	char *in_pack[3] = {".in_24real",".in_24im",".in_48packed"};
	char *set_size[4] = {".ss4",".ss8",".ss16",reserved};

	int in_dattyp_flag = (insn[0] >> 24) & 0x3;
	int preproc_flag = (insn[0] >> 22) & 0x3;
	int in_pack_flag = (insn[0] >> 14) & 0x3;
	int ima_flag = (insn[0] >> 13) & 0x3;
	int vec_sz = insn[0] & 0x1FFF;
	int src_add_inc = (insn[2] >> 8) & 0xFF;
	int dest_add_inc = insn[2] & 0xFF;
	int sort_rank_flag = (insn[3] >> 18) & 0xF;
	int set_size_flag = (insn[3] >> 16) & 0x3;

	if(ima_flag){
		sprintf(outbuff, "%s.ind %s %s %s %s,%d,%s,%d,%d", instr, in_dattyp[in_dattyp_flag], preproc[preproc_flag], set_size[set_size_flag], in_pack[in_pack_flag], vec_sz, getSrcAddWr(INSN_F), src_add_inc, dest_add_inc);
	}else{
		sprintf(outbuff, "%s %s %s %s %s,%d,%s,%s,%d,%d", instr, in_dattyp[in_dattyp_flag], preproc[preproc_flag], set_size[set_size_flag], in_pack[in_pack_flag], vec_sz, getSrcAdd(INSN_F), getDestAdd(INSN_F), src_add_inc, dest_add_inc);
	}

	return outbuff;
}
	
char * sptDisassemle(){
/*int temp_val;
     for(int i =0;i<4;i++)
	{
	insn[i] = *ins;
	ins++;
	}*/

	byte opcode = ((insn[0]>>24) & 0xfc) >> 2;
	switch (opcode)
	{
	case OPCODE_SET:
		return sptSetInstruction_dis();
		break;
	case OPCODE_GET:
		return sptGetInstruction_dis();
		break;
	case OPCODE_EVT:
		return sptEvtInstruction_dis();
		break;
	case OPCODE_JUMP:
		return sptJumpInstruction_dis();
		break;
	case OPCODE_ADD:
		return sptAddInstruction_dis();
		break;
	case OPCODE_COPY:
		return sptCopyInstruction_dis(FALSE);
		break;
	case OPCODE_COPYB:
		return sptCopyInstruction_dis(TRUE);
		break;
	case OPCODE_STOP:
		return  sptStopInstruction_dis();
		break;
	case OPCODE_LOOP:
		return  sptLoopInstruction_dis();
		break;
	case OPCODE_NEXT:
		return  sptNextInstruction_dis();
		break;
	case OPCODE_SYNC:
		return  sptSyncInstruction_dis();
		break;
	case OPCODE_WAIT:
		return  sptWaitInstruction_dis();
		break;
	case OPCODE_SUB:
		return  sptSubInstruction_dis();
		break;
	case OPCODE_CMP:
		return  sptCmpInstruction_dis();
		break;
	case OPCODE_FIR:
		return  sptFirInstruction_dis(FALSE);
		break;
	case OPCODE_FIRB:
		return  sptFirInstruction_dis(TRUE);
		break;
	case OPCODE_HIST:
		return  sptHistInstruction_dis();
		break;
	case OPCODE_IRDX2:
		return  sptIrdx2Instruction_dis(FALSE);
		break;
	case OPCODE_IRDX2B:
		return  sptIrdx2Instruction_dis(TRUE);
		break;
	case OPCODE_IRDX4:
		return  sptIrdx4Instruction_dis(FALSE);
		break;
	case OPCODE_IRDX4B:
		return  sptIrdx4Instruction_dis(TRUE);
		break;
	case OPCODE_MAXS:
		return  sptMaxsInstruction_dis(FALSE);
		break;
	case OPCODE_MAXSB:
		return  sptMaxsInstruction_dis(TRUE);
		break;
	case OPCODE_PDMA:
		return  sptPdmaInstruction_dis();
		break;
	case OPCODE_SCP:
		return  sptScpInstruction_dis(FALSE);
		break;
	case OPCODE_SCPB:
		return  sptScpInstruction_dis(TRUE);
		break;
	case OPCODE_SEL:
		return  sptSelInstruction_dis();
		break;
	case OPCODE_RDX2:
		return  sptRdx2Instruction_dis(FALSE);
		break;
	case OPCODE_RDX2B:
		return  sptRdx2Instruction_dis(TRUE);
		break;
	case OPCODE_RDX4:
		return  sptRdx4Instruction_dis(FALSE);
		break;
	case OPCODE_RDX4B:
		return  sptRdx4Instruction_dis(TRUE);
		break;
	case OPCODE_THREAD:
		return  sptThreadInstruction_dis();
		break;		
	case OPCODE_VMT:
		return  sptVmtInstruction_dis();
		break;
	case OPCODE_WATCHDOG:
		return  sptWatchdogInstruction_dis();
		break;
	case OPCODE_WIN:
		return  sptWinInstruction_dis(FALSE);
		break;
	case OPCODE_WINB:
		return  sptWinInstruction_dis(TRUE);
		break;
	case OPCODE_DSP:
		return sptDspInstruction_dis();
		break;
	case OPCODE_REPEAT:
		return sptRepeatInstruction_dis();
		break;
	case OPCODE_SORT:
		return sptSortInstruction_dis(FALSE);
		break;
	case OPCODE_SORTB:
		return sptSortInstruction_dis(TRUE);
		break;

	default:
		return  sptDummyInstruction_dis();
		break;
		
	}	

	return 0;
}




int
print_insn_spt3(bfd_vma pc  ATTRIBUTE_UNUSED, disassemble_info *info  ATTRIBUTE_UNUSED){
int bigendian =0;
  bfd_byte buffer[16];
 // unsigned  insn[4];
  
  int status;
  int buf_size = 16;
  if (info->buffer_length) {
    if (pc+buf_size > (info->buffer_vma+info->buffer_length)) {
      buf_size = (info->buffer_vma+info->buffer_length)-pc;
      memset(buffer,0,16);
    }
  }
  status = (*info->read_memory_func) (pc, buffer, buf_size, info);
  if (status != 0) {
    (*info->memory_error_func) (status, pc, info);
    return -1;
  }
  if (bigendian) {
    insn[0] = bfd_getb32 (buffer +0*4);
    insn[1] = bfd_getb32 (buffer +1*4);
    insn[2] = bfd_getb32 (buffer +2*4);
    insn[3] = bfd_getb32 (buffer +3*4);
  } else {
    insn[3] = bfd_getl32 (buffer +0*4);
    insn[2] = bfd_getl32 (buffer +1*4);
    insn[1] = bfd_getl32 (buffer +2*4);
    insn[0] = bfd_getl32 (buffer +3*4);
  }
  int sz = 16;
  (*info->fprintf_func) (info->stream, (sptDisassemle()) );
  return sz;  
}
