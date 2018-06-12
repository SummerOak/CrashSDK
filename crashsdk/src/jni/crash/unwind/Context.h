#ifndef Context_h
#define Context_h

#include "Defines.h"
#include "Arch.h"
#include "DataTypes.h"
#include <ucontext.h>
#include <dlfcn.h>

class EHFrame;
class Context{

	friend class EHFrame;

public:
	virtual ~Context(){}

	virtual void setucontext(ucontext_t *uc);
	virtual int reset();
	virtual bool rootFrame();
	virtual void* getCFA();
	virtual void* getIP();
	virtual void* lastLibBase();
	virtual const char* lastLibName();
	virtual uint8_t getAddrWidth(){
		return sizeof(word_t);
	}
	virtual bool isBigEndian(){
		return false;
	}
	
	virtual int restoreFrame() = 0;
	virtual word_t getReg(uint32_t reg) = 0;
	virtual int setupFrame(CFI& cfi) = 0;
	virtual int stepBack(CFI& cfi) = 0;

	virtual int readS8(word_t* addr, int8_t* valp, void* arg = NULL);
	virtual int readS16(word_t* addr, int16_t* valp, void* arg = NULL);
	virtual int readS32(word_t* addr, int32_t* valp, void* arg = NULL);
	virtual int readS64(word_t* addr, int64_t* valp, void* arg = NULL);
	virtual int readU8(word_t* addr, uint8_t* valp, void* arg = NULL);
	virtual int readU16(word_t* addr, uint16_t* valp, void* arg = NULL);
	virtual int readU32(word_t* addr, uint32_t* valp, void* arg = NULL);
	virtual int readU64(word_t* addr, uint64_t* valp, void* arg = NULL);
	virtual int readW(word_t* addr, word_t* valp, void* arg = NULL);
	virtual int readUleb128(word_t* addr, word_t* valp, void* arg = NULL);
	virtual int readSleb128(word_t* addr, word_t* valp, void* arg = NULL);
	virtual int sword (word_t val, sword_t &out);

	CFI mCfi;

protected:
	static const char* TAG;

	ucontext_t* mContext;
	
};

#ifdef __i386__
	#include "Context_x86.h"
#elif defined __arm__
	#include "Context_arm.h"
#endif

#endif