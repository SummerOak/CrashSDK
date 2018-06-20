#include "Utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "Log.h"
#include "Defines.h"

template <typename T>
inline int vpnlib::compareArr(T a1[],int l1,T a2[],int l2){
	for(int i=0;i<l1&&i<l2;i++){
		if(a1[i] > a2[i]){
			return 1;
		}else if(a1[i] < a2[i]){
			return -1;
		}
	}

	return l1>l2?1:(l1<l2?-1:0);
}

char* vpnlib::bytes2hex(uint8_t* data,char* out,size_t len){

	for(int i=0;i<len;i++){
		sprintf(&out[i<<1], "%02X", data[i]);
	}

	out[len<<1] = '\0';
	return out;
}

uint8_t vpnlib::char2hex(const char c) {
    if (c >= '0' && c <= '9') return (uint8_t) (c - '0');
    if (c >= 'a' && c <= 'f') return (uint8_t) ((c - 'a') + 10);
    if (c >= 'A' && c <= 'F') return (uint8_t) ((c - 'A') + 10);
    return 255;
}

void vpnlib::hex2bytes(const char *hex, uint8_t *out) {
    size_t len = strlen(hex);
    for (int i = 0; i<len; i+=2){
        out[i>>1] = (char2hex(hex[i]) << 4) | char2hex(hex[i+1]);
    }
    return;
}

void vpnlib::printMem(uint32_t s, uint32_t e){
	while(s <= e){

		LOGE(PTAG(""), "%x: %08x", s, *(uint32_t*)s);

		s += 4;
	}
}
