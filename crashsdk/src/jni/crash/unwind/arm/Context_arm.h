#ifndef Context_arm_h
#define Context_arm_h

#include "Context.h"
#include "Exidx.h"

typedef enum{
	UNW_ARM_R0,
	UNW_ARM_R1,
	UNW_ARM_R2,
	UNW_ARM_R3,
	UNW_ARM_R4,
	UNW_ARM_R5,
	UNW_ARM_R6,
	UNW_ARM_R7,
	UNW_ARM_R8,
	UNW_ARM_R9,
	UNW_ARM_R10,
	UNW_ARM_R11,
	UNW_ARM_R12,
	UNW_ARM_R13,
	UNW_ARM_R14,
	UNW_ARM_R15,

	UNW_TDEP_IP = UNW_ARM_R14,  /* A little white lie.  */
	UNW_TDEP_SP = UNW_ARM_R13,

}arm_regnum_t;

class Context_arm: public Context{

	friend class Exidx;

public:
	Context_arm(){
	}

	~Context_arm(){
		mContext = NULL;
	}

	virtual int setupFrame(CFI& cfi);
	virtual word_t getReg(uint32_t reg);
	virtual int restoreFrame();
	virtual int stepBack(CFI& cfi);
	void adjustIP(word_t& ip);
	
	static int callback (struct dl_phdr_info *info, size_t size, void *ptr);
	
private:

	static const char* TAG;


};

#endif