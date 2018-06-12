#include "Arch.h"
#include "Context.h"
#include "Log.h"
#include "EHFrame.h"

const char* Context::TAG = PTAG("Context");

void Context::setucontext(ucontext_t *uc){
	mContext = uc;
}

int Context::reset(){
	LOGD(TAG,"sizeof CFI = %d", sizeof(mCfi));
	memset(&mCfi, 0, sizeof(mCfi));
	mCfi.last = 1;

	return setupFrame(mCfi);
}

bool Context::rootFrame(){
	return mCfi.root;
}

void* Context::getCFA(){
	return (void*)mCfi.cfa;
}

void* Context::getIP(){
	return (void*)mCfi.ip;
}

void* Context::lastLibBase(){
	return (void*)mCfi.last_libbase;
}

const char* Context::lastLibName(){
	return mCfi.last_libname;
}

int Context::readS8( word_t* addr, int8_t* valp, void* arg){
	uint8_t uval;
	int ret;

	if ((ret = readU8 (addr, &uval, arg)) < 0){
		return ret;
	}

	*valp = (int8_t) uval;
	return 0;
}


int Context::readS16( word_t* addr, int16_t* valp, void* arg){

	uint16_t uval;
	int ret;

	if ((ret = readU16 (addr, &uval, arg)) < 0){
		return ret;
	}

	*valp = (int16_t) uval;
	return 0;
}


int Context::readS32( word_t* addr, int32_t* valp, void* arg){
	uint32_t uval;
	int ret;

	if ((ret = readU32 (addr, &uval, arg)) < 0){
		return ret;
	}

	*valp = (int32_t) uval;
	return 0;
}


int Context::readS64( word_t* addr, int64_t* valp, void* arg){
	uint64_t uval;
	int ret;

	if ((ret = readU64 (addr, &uval, arg)) < 0){
		return ret;
	}

	*valp = (int64_t) uval;
	return 0;
}

int Context::readU8( word_t* addr, uint8_t* valp, void* arg){
	*valp = ((value_t *)(uintptr_t)*addr)->u8;
	*addr += sizeof (*valp);
	return 0;
}


int Context::readU16( word_t* addr, uint16_t* valp, void* arg){

	*valp = ((value_t *)(uintptr_t)*addr)->u16;
	*addr += sizeof (*valp);
	return 0;
}


int Context::readU32( word_t* addr, uint32_t* valp, void* arg){
	*valp = ((value_t *)(uintptr_t)*addr)->u32;
	*addr += sizeof (*valp);
	return 0;
}


int Context::readU64( word_t* addr, uint64_t* valp, void* arg){
	*valp = ((value_t *)(uintptr_t)*addr)->u64;
	*addr += sizeof (*valp);
	return 0;
}


int Context::readW( word_t* addr, word_t* valp, void* arg){
	uint32_t u32;
	uint64_t u64;
	int ret;

	switch (sizeof(word_t)){
		case 4:
			if((ret = readU32 (addr, &u32, arg)) < 0){
				return ret;
			}

			*valp = u32;
			return ret;

		case 8:
			if((ret = readU64 (addr, &u64, arg)) < 0){
				return ret;
			}

			*valp = u64;
			return ret;

		default:
			LOGE(TAG,"fault address size");
			return -1;
	}
}


int Context::readUleb128( word_t* addr, word_t* valp, void* arg){
	word_t val = 0, shift = 0;
	uint8_t byte;
	int ret;

	do{
		if ((ret = readU8 (addr, &byte, arg)) < 0){
			return ret;
		}

		val |= ((word_t) byte & 0x7f) << shift;
		shift += 7;
	}while (byte & 0x80);

	*valp = val;
	return 0;
}


int Context::readSleb128( word_t* addr, word_t* valp, void* arg){
	word_t val = 0, shift = 0;
	uint8_t byte;
	int ret;

	do{
		if ((ret = readU8 (addr, &byte, arg)) < 0){
			return ret;
		}

		val |= ((word_t) byte & 0x7f) << shift;
		shift += 7;
	}while (byte & 0x80);

	if (shift < 8 * sizeof (word_t) && (byte & 0x40) != 0){
		/* sign-extend negative value */
		val |= ((word_t) -1) << shift;
	}
		
	*valp = val;
	return 0;
}

int Context::sword (word_t val, sword_t &out){
	switch (getAddrWidth()){
		case 1: out = (int8_t) val; return 0;
		case 2: out = (int16_t) val; return 0;
		case 4: out = (int32_t) val; return 0;
		case 8: out = (int64_t) val; return 0;
		default: LOGE(TAG,"unknown address width %d", getAddrWidth());
	}

	return -1;
}

