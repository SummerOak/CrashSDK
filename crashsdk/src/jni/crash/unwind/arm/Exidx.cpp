#include "Exidx.h"
#include "Log.h"
#include "Defines.h"
#include <link.h>

const char* Exidx::TAG = PTAG("Exidx");

int Exidx::restoreFrame(Context& context){
	CFI& cfi = context.mCfi;

	LOGD(TAG,"restoreFrame ip=0x%x",cfi.ip);

#ifdef __arm__
	word_t ip = cfi.ip;
	int pcount = 0;
	 _Unwind_Ptr exidxBase =  dl_unwind_find_exidx(ip, &pcount);
	 LOGE(TAG, "find exidx ip 0x%x , exidx base = 0x%x, pcount %d", ip, exidxBase, pcount);
	 if(exidxBase != 0){
	 	cfi.exidx_table_start = exidxBase;
	 	cfi.exidx_table_end = cfi.exidx_table_start + (pcount<<3);
	 	if(retriveEntry(context, cfi.exidx_table_start, cfi.exidx_table_end) != 0){
			LOGE(TAG, "retrive exidx entry failed.");
			return 2;
		}

		LOGD(TAG,"find exidx section success, ip(0x%x - 0x%x), exidx(0x%x - 0x%x)", 
	    		cfi.start_ip, cfi.end_ip, cfi.exidx_table_start, cfi.exidx_table_end);

		uint8_t instr[32];
		int instrLen = readUnwindInstr(context, cfi.exidx_entry, instr);
		if(instrLen <= 0){
			LOGE(TAG,"read instr failed.");
			return 1;
		}

		if(runExidxInstr(instr, instrLen, cfi) != 0){
			LOGE(TAG,"run exidx instr failed.");
			return 1;
		}

		return 0;

	 }else{
	 	LOGE(TAG, "exidxBase not found");
	 }
#else

	int ret = dl_iterate_phdr (Exidx::checklib, &context);
	if(ret == 1){

		LOGD(TAG,"find target exidx success, restore reg states...");

		uint8_t instr[32];
		int instrLen = readUnwindInstr(context, cfi.exidx_entry, instr);
		if(instrLen <= 0){
			LOGE(TAG,"read instr failed.");
			return 1;
		}

		if(runExidxInstr(instr, instrLen, cfi) != 0){
			LOGE(TAG,"run exidx instr failed.");
			return 1;
		}

		LOGD(TAG,"restoreFrame(0x%lx) success.", (long)cfi.ip);
		return 0;
	}else if(ret == 0){
		LOGE(TAG,"restoreFrame(0x%lx) failed after search all libs.", (long)cfi.ip);
	}else{
		LOGE(TAG,"restoreFrame(0x%lx) failed.", (long)cfi.ip);
	}
	
#endif

	return 1;

}

int Exidx::checklib (struct dl_phdr_info *info, size_t size, void *ptr){

	if (size < OFFSET(struct dl_phdr_info, dlpi_phnum) + sizeof (info->dlpi_phnum)){
		LOGE(TAG,"size not match while iterating libs, continue");
		return 0;
	}

	// LOGD(TAG,"checking %s (0x%x)", info->dlpi_name, info->dlpi_addr);

	Context& context = (*(Context*)ptr);
	CFI& cfi = context.mCfi;

	const ElfW(Phdr) *phdr, *p_arm_exidx, *p_text;
	ElfW(Addr) libbase;
	phdr = info->dlpi_phdr;
	libbase = info->dlpi_addr;
	p_text = NULL;
	p_arm_exidx = NULL;
	word_t ip = cfi.ip;

	for (int n = info->dlpi_phnum; --n >= 0; phdr++){
		if (phdr->p_type == PT_LOAD){
			ElfW(Addr) vaddr = phdr->p_vaddr + libbase;

			// LOGD(TAG,"(0x%x  0x%x) 0x%x",vaddr,vaddr + phdr->p_memsz, ip);

			if (ip >= vaddr && ip < vaddr + phdr->p_memsz){
				p_text = phdr;
			}

		} else if (phdr->p_type == PT_ARM_EXIDX){
			p_arm_exidx = phdr;
		}
    }

    if(p_text == NULL || p_arm_exidx == NULL){
    	return 0;
    }

    LOGD(TAG,"found ip(0x%x) fall in to lib(%s) (0x%lx)", ip, info->dlpi_name, (long)libbase);

    cfi.last_libname = info->dlpi_name;
    cfi.last_libbase = libbase;
	cfi.start_ip = p_text->p_vaddr + libbase;
	cfi.end_ip = cfi.start_ip + p_text->p_memsz;
	cfi.exidx_table_start = p_arm_exidx->p_vaddr + libbase;
	cfi.exidx_table_end = cfi.exidx_table_start + p_arm_exidx->p_memsz;

	if(retriveEntry(context, cfi.exidx_table_start, cfi.exidx_table_end) != 0){
		LOGE(TAG, "retrive exidx entry failed.");
		return 2;
	}

	LOGD(TAG,"find exidx section success, ip(0x%x - 0x%x), exidx(0x%x - 0x%x)", 
    		cfi.start_ip, cfi.end_ip, cfi.exidx_table_start, cfi.exidx_table_end);

	return 1;
}

int Exidx::retriveEntry(Context& context, word_t s, word_t e){
	/* The .ARM.exidx section contains a sorted list of key-value pairs -
	the unwind entries.  The 'key' is a prel31 offset to the start of a
	function.  We binary search this section in order to find the
	appropriate unwind entry.  */
	word_t first = s;
	word_t last = e - 8;
	word_t entry, val;
	CFI& cfi = context.mCfi;
	word_t ip = cfi.ip;

	if (prel31_to_addr (first, &val) < 0 || ip < val){
		LOGE(TAG,"init first failed or ip < val");
		return -1;
	}

	if (prel31_to_addr (last, &val) < 0){
		LOGE(TAG,"init last failed");
		return -1;
	}

	if (ip >= val){
		LOGE(TAG,"ip exceed exidx range.");
		entry = last;
		if (prel31_to_addr (last, &cfi.start_ip) < 0){
			return -1;
		}
		--cfi.end_ip;
	} else {
		while (first < last - 8){
			entry = first + (((last - first) / 8 + 1) >> 1) * 8;

			if (prel31_to_addr (entry, &val) < 0){
				return -1;
			}

			if (ip < val){
				last = entry;
			} else{
				first = entry;
			}
		}

		entry = first;

		if (prel31_to_addr (entry, &cfi.start_ip) < 0
			|| prel31_to_addr (entry + 8, &cfi.end_ip) < 0){
			return -1;
		}

		--cfi.end_ip;
	}

	cfi.exidx_entry = entry;

	return 0;
}

int Exidx::readUnwindInstr(Context& context, word_t entry, uint8_t* instr){
	int nbuf = 0;
	word_t addr, temp;
	uint32_t data;

	/* An ARM unwind entry consists of a prel31 offset to the start of a
	function followed by 31bits of data: 
	* if set to 0x1: the function cannot be unwound (EXIDX_CANTUNWIND)
	* if bit 31 is one: this is a table entry itself (ARM_EXIDX_COMPACT)
	* if bit 31 is zero: this is a prel31 offset of the start of the
	table entry for this function  */
	if (prel31_to_addr(entry, &addr) < 0){
		return -1;
	}

	temp = entry+4;
	if (context.readU32(&temp, &data) < 0){
		return -1;
	}

	if (data == ARM_EXIDX_CANT_UNWIND) {
		LOGE (TAG, "ARM_EXIDX_CANT_UNWIND(%x)", data);
		nbuf = -1;
	} else if (data & ARM_EXIDX_COMPACT){
		LOGD (TAG, "%p compact model %d [0x%x]", (void *)addr, (data >> 24) & 0x7f, data);
		instr[nbuf++] = data >> 16;
		instr[nbuf++] = data >> 8;
		instr[nbuf++] = data;
	} else {
		word_t extbl_data;
		uint8_t n_table_words = 0;

		if (prel31_to_addr(entry + 4, &extbl_data) < 0){
			return -1;
		}

		temp = extbl_data;
		if (context.readU32(&temp, &data) < 0){
			return -1;
		}

		if (data & ARM_EXIDX_COMPACT){
			int pers = (data >> 24) & 0x0f;
			LOGD (TAG, "%p compact model %d [%8.8x]", (void *)addr, pers, data);
			if (pers == 1 || pers == 2) {
				n_table_words = (data >> 16) & 0xff;
				extbl_data += 4;
			}else{
				instr[nbuf++] = data >> 16;
			}

			instr[nbuf++] = data >> 8;
			instr[nbuf++] = data;
		} else {
			word_t pers;
			if (prel31_to_addr (extbl_data, &pers) < 0){
				return -1;
			}

			LOGD (TAG, "%p Personality routine: %8p", (void *)addr, (void *)pers);

			temp = extbl_data+4;
			if (context.readU32(&temp, &data) < 0){
				return -1;
			}

			n_table_words = data >> 24;
			instr[nbuf++] = data >> 16;
			instr[nbuf++] = data >> 8;
			instr[nbuf++] = data;
			extbl_data += 8;
		}

		if(n_table_words > 5){
			LOGE(TAG,"table too large %u", n_table_words);
			return -1;
		}

		for (uint8_t j = 0; j < n_table_words; j++){
			temp = extbl_data;
			if (context.readU32(&temp, &data) < 0){
				return -1;
			}

			instr[nbuf++] = data >> 24;
			instr[nbuf++] = data >> 16;
			instr[nbuf++] = data >> 8;
			instr[nbuf++] = data >> 0;
		}
	}

	if (nbuf > 0 && instr[nbuf - 1] != ARM_EXTBL_OP_FINISH){
		instr[nbuf++] = ARM_EXTBL_OP_FINISH;
	}

	return nbuf;
}

int Exidx::runExidxInstr(const uint8_t* instr, uint8_t len, CFI& cfi){
	LOGD(TAG,"runExidxInstr instr[%p +%u]", instr, len);
	if(instr == NULL || len <= 0){
		return 0;
	}

	#define READ_OP() *instr++
	const uint8_t *end = instr + len;
	int ret;
	arm_exbuf_data edata;

	while (instr < end){
		uint8_t op = READ_OP ();
		if ((op & 0xc0) == 0x00) {
			edata.cmd = ARM_EXIDX_CMD_DATA_POP;
			edata.data = (((int)op & 0x3f) << 2) + 4;
		} else if ((op & 0xc0) == 0x40){
			edata.cmd = ARM_EXIDX_CMD_DATA_PUSH;
			edata.data = (((int)op & 0x3f) << 2) + 4;
		} else if ((op & 0xf0) == 0x80) {
			uint8_t op2 = READ_OP ();
			if (op == 0x80 && op2 == 0x00){
				edata.cmd = ARM_EXIDX_CMD_REFUSED;
			} else {
				edata.cmd = ARM_EXIDX_CMD_REG_POP;
				edata.data = ((op & 0xf) << 8) | op2;
				edata.data = edata.data << 4;
			}
		} else if ((op & 0xf0) == 0x90) {
			if (op == 0x9d || op == 0x9f){
				edata.cmd = ARM_EXIDX_CMD_RESERVED;
			} else {
				edata.cmd = ARM_EXIDX_CMD_REG_TO_SP;
				edata.data = op & 0x0f;
			}
		} else if ((op & 0xf0) == 0xa0) {
			unsigned end = (op & 0x07);
			edata.data = (1 << (end + 1)) - 1;
			edata.data = edata.data << 4;
			if (op & 0x08)
			edata.data |= 1 << 14;
			edata.cmd = ARM_EXIDX_CMD_REG_POP;
		} else if (op == ARM_EXTBL_OP_FINISH) {
			edata.cmd = ARM_EXIDX_CMD_FINISH;
			instr = end;
		} else if (op == 0xb1) {
			uint8_t op2 = READ_OP ();
			if (op2 == 0 || (op2 & 0xf0)){
				edata.cmd = ARM_EXIDX_CMD_RESERVED;
			} else {
				edata.cmd = ARM_EXIDX_CMD_REG_POP;
				edata.data = op2 & 0x0f;
			}
		} else if (op == 0xb2) {
			uint32_t offset = 0;
			uint8_t byte, shift = 0;
			do{
				byte = READ_OP ();
				offset |= (byte & 0x7f) << shift;
				shift += 7;
			}while (byte & 0x80);
			edata.data = offset * 4 + 0x204;
			edata.cmd = ARM_EXIDX_CMD_DATA_POP;
		}else if (op == 0xb3 || op == 0xc8 || op == 0xc9) {
			edata.cmd = ARM_EXIDX_CMD_VFP_POP;
			edata.data = READ_OP ();
			if (op == 0xc8){
				edata.data |= ARM_EXIDX_VFP_SHIFT_16;
			}
			if (op != 0xb3){
				edata.data |= ARM_EXIDX_VFP_DOUBLE;
			}
		} else if ((op & 0xf8) == 0xb8 || (op & 0xf8) == 0xd0){
			edata.cmd = ARM_EXIDX_CMD_VFP_POP;
			edata.data = 0x80 | (op & 0x07);
			if ((op & 0xf8) == 0xd0){
				edata.data |= ARM_EXIDX_VFP_DOUBLE;
			}
		} else if (op >= 0xc0 && op <= 0xc5) {
			edata.cmd = ARM_EXIDX_CMD_WREG_POP;
			edata.data = 0xa0 | (op & 0x07);
		} else if (op == 0xc6) {
			edata.cmd = ARM_EXIDX_CMD_WREG_POP;
			edata.data = READ_OP ();
		} else if (op == 0xc7) {
			uint8_t op2 = READ_OP ();
			if (op2 == 0 || (op2 & 0xf0)){
				edata.cmd = ARM_EXIDX_CMD_RESERVED;
			} else {
				edata.cmd = ARM_EXIDX_CMD_WCGR_POP;
				edata.data = op2 & 0x0f;
			}
		} else{
			edata.cmd = ARM_EXIDX_CMD_RESERVED;
		}

		if ((ret = applyInstr(edata, cfi)) != 0){
			return ret;
		}
	}

	return 0;
}

int Exidx::applyInstr(arm_exbuf_data& edata, CFI& cfi){
	int ret = 0;
	unsigned i;

	LOGD(TAG,"applyInstr cmd(%d)", edata.cmd);
	switch (edata.cmd){
		case ARM_EXIDX_CMD_FINISH:
			/* Set LR to PC if not set already.  */
			// if (cfi.loc[UNW_ARM_R15] == 0){
				LOGD(TAG,"set r15(0x%lx) to r14(0x%lx)", cfi.loc[UNW_ARM_R15], cfi.loc[UNW_ARM_R14]);
				cfi.loc[UNW_ARM_R15] = cfi.loc[UNW_ARM_R14];
			// }

			LOGD(TAG,"set ip(0x%lx) to 0x%lx", cfi.ip, cfi.loc[UNW_ARM_R15]);
			/* Set IP.  */
			cfi.ip = cfi.loc[UNW_ARM_R15];
			break;
		case ARM_EXIDX_CMD_DATA_PUSH:
			LOGD (TAG, "cfa(0x%lx) - %d", cfi.cfa, edata.data);
			cfi.cfa -= edata.data;
			break;
		case ARM_EXIDX_CMD_DATA_POP:
			LOGD (TAG, "cfa(0x%lx) + %d", cfi.cfa, edata.data);
			cfi.cfa += edata.data;
			break;
		case ARM_EXIDX_CMD_REG_POP:
			for (i = 0; i < 16; i++) if (edata.data & (1 << i)) {
				LOGD(TAG, "pop {r%d}, 0x%lx", i, *(word_t*)cfi.cfa);
				cfi.loc[UNW_ARM_R0 + i] = *(word_t*)cfi.cfa;
				cfi.cfa += 4;
			}
			/* Set cfa in case the SP got popped. */
			if (edata.data & (1 << 13)){
				LOGD(TAG,"set cfa(0x%lx) = 0x%lx", cfi.cfa, cfi.loc[UNW_ARM_R13]);
				cfi.cfa = cfi.loc[UNW_ARM_R13];
			}
			break;
		case ARM_EXIDX_CMD_REG_TO_SP:
			if(edata.data >= 16){
				LOGE(TAG,"register indx must in [0,15], %d", edata.data);
				return -1;
			}

			LOGD(TAG, "cfa = r%d 0x%lx", edata.data, cfi.cfa);
			cfi.loc[UNW_ARM_R13] = cfi.loc[UNW_ARM_R0 + edata.data];
			cfi.cfa = cfi.loc[UNW_ARM_R13];
			break;
		case ARM_EXIDX_CMD_VFP_POP:
			/* Skip VFP registers, but be sure to adjust stack */
			for (i = ARM_EXBUF_START (edata.data); i <= ARM_EXBUF_END (edata.data); i++){
				LOGD(TAG,"pop, cfa += 8");
				cfi.cfa += 8;
			}
			if (!(edata.data & ARM_EXIDX_VFP_DOUBLE)){
				LOGD(TAG,"pop, cfa += 4");
				cfi.cfa += 4;
			}
			break;
		case ARM_EXIDX_CMD_WREG_POP:
			for (i = ARM_EXBUF_START (edata.data); i <= ARM_EXBUF_END (edata.data); i++){
				LOGD(TAG,"pop, cfa += 8");
				cfi.cfa += 8;
			}
			break;
		case ARM_EXIDX_CMD_WCGR_POP:
			for (i = 0; i < 4; i++) if (edata.data & (1 << i)){
				LOGD(TAG,"pop, cfa += 4");
				cfi.cfa += 4;
			}
			break;
		case ARM_EXIDX_CMD_REFUSED:
		case ARM_EXIDX_CMD_RESERVED:
			ret = -1;
			break;
	}

	return ret;
}

int Exidx::prel31_to_addr(word_t prel31, word_t *val){
	word_t offset = *(word_t*)prel31;
	offset = ((long)offset << 1) >> 1;
	*val = prel31 + offset;

	return 0;
}
