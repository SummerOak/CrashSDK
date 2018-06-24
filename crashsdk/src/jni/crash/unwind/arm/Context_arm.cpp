#include "Context_arm.h"
#include "Log.h"
#include "Utils.h"
#include "EHFrame.h"
#include <asm/sigcontext.h>

const char* Context_arm::TAG = PTAG("Context_arm");

int Context_arm::setupFrame(CFI& cfi){

	cfi.loc[UNW_ARM_R0] = getReg(UNW_ARM_R0);
	cfi.loc[UNW_ARM_R1] = getReg(UNW_ARM_R1);
	cfi.loc[UNW_ARM_R2] = getReg(UNW_ARM_R2);
	cfi.loc[UNW_ARM_R3] = getReg(UNW_ARM_R3);
	cfi.loc[UNW_ARM_R4] = getReg(UNW_ARM_R4);
	cfi.loc[UNW_ARM_R5] = getReg(UNW_ARM_R5);
	cfi.loc[UNW_ARM_R6] = getReg(UNW_ARM_R6);
	cfi.loc[UNW_ARM_R7] = getReg(UNW_ARM_R7);
	cfi.loc[UNW_ARM_R8] = getReg(UNW_ARM_R8);
	cfi.loc[UNW_ARM_R9] = getReg(UNW_ARM_R9);
	cfi.loc[UNW_ARM_R10] = getReg(UNW_ARM_R10);
	cfi.loc[UNW_ARM_R11] = getReg(UNW_ARM_R11);
	cfi.loc[UNW_ARM_R12] = getReg(UNW_ARM_R12);
	cfi.loc[UNW_ARM_R13] = getReg(UNW_ARM_R13);
	cfi.loc[UNW_ARM_R14] = getReg(UNW_ARM_R14);
	cfi.loc[UNW_ARM_R15] = getReg(UNW_ARM_R15);

	cfi.ip = cfi.loc[UNW_ARM_R15];
	cfi.cfa = cfi.loc[UNW_ARM_R13];

	for(int i=0;i<=UNW_ARM_R15;i++){
		LOGD(TAG,"r%d = 0x%lx", i, (long)cfi.loc[i]);
	}
	
	return 0;
}

word_t Context_arm::getReg(uint32_t reg){
	void* addr;

	struct sigcontext* sig_ctx = &mContext->uc_mcontext;
	switch (reg){
		case UNW_ARM_R0:	addr = &sig_ctx->arm_r0; break;
		case UNW_ARM_R1:  	addr = &sig_ctx->arm_r1; break;
		case UNW_ARM_R2:  	addr = &sig_ctx->arm_r2; break;
		case UNW_ARM_R3:  	addr = &sig_ctx->arm_r3; break;
		case UNW_ARM_R4: 	addr = &sig_ctx->arm_r4; break;
		case UNW_ARM_R5: 	addr = &sig_ctx->arm_r5; break;
		case UNW_ARM_R6:	addr = &sig_ctx->arm_r6; break;
		case UNW_ARM_R7: 	addr = &sig_ctx->arm_r7; break;
		case UNW_ARM_R8: 	addr = &sig_ctx->arm_r8; break;
		case UNW_ARM_R9: 	addr = &sig_ctx->arm_r9; break;
		case UNW_ARM_R10: 	addr = &sig_ctx->arm_r10; break;
		case UNW_ARM_R11: 	addr = &sig_ctx->arm_fp; break;
		case UNW_ARM_R12: 	addr = &sig_ctx->arm_ip; break;
		case UNW_ARM_R13:  	addr = &sig_ctx->arm_sp; break;
		case UNW_ARM_R14:  	addr = &sig_ctx->arm_lr; break;
		case UNW_ARM_R15:  	addr = &sig_ctx->arm_pc; break;

		default:
		LOGE(TAG,"unknown reg %d", reg);
		addr = NULL;
	}

	if(addr != NULL){
		return *(word_t*)addr;
	}

	return 0;
}

void Context_arm::adjustIP(word_t& ip){
	if (ip){
		int adjust = 4;
		if (ip & 1){
			/* Thumb instructions, the currently executing instruction could be
			* 2 or 4 bytes, so adjust appropriately.
			*/

			word_t addr = ip - 5;
			word_t value;
			if (ip < 5 || readW(&addr, &value) < 0 || (value & 0xe000f000) != 0xe000f000){
				adjust = 2;
			}
		}

		ip -= adjust;
	}
}

int Context_arm::restoreFrame(){
	if(rootFrame()){
		LOGE(TAG,"restoreFrame failed , it's root frame.");
		return 1;
	}

	LOGD(TAG,"restoreFrame ip(0x%lx)", (long)mCfi.ip);
	int ret = 0;

	word_t ip = mCfi.ip;
	word_t cfa = mCfi.cfa;

	if((ret = EHFrame::restoreFrame(*this)) != 0){
		LOGE(TAG,"eh_frame parse failed.");

		if((ret = Exidx::restoreFrame(*this)) != 0){
			LOGE(TAG,"restoreFrame(0x%lx) from exidx failed, try call routine.", (long)mCfi.ip);
			if((ret = stepBack(mCfi)) != 0){
				LOGE(TAG,"try call routine failed.");
				return ret;
			}
		}
	}

	adjustIP(mCfi.ip);

	if(ip == mCfi.ip && cfa == mCfi.cfa){
		LOGE(TAG,"reach root, stop here.");
		mCfi.root = 1;
		vpnlib::printMem(cfa, cfa + (50<<2));

	}else{
		vpnlib::printMem(cfa, mCfi.cfa);
	}

	mCfi.loc[UNW_ARM_R13] = mCfi.cfa;

	return ret;

}

int Context_arm::stepBack(CFI& cfi){

	cfi.ip = cfi.loc[UNW_ARM_R15] = cfi.loc[UNW_ARM_R14]; //set pc to last frame

	LOGD(TAG,"stepBack ip(0x%lx)", cfi.ip);
	return 0;
}

