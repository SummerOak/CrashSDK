#include "EHFrame.h"
#include "Defines.h"
#include "Log.h"
#include "DWInstruction.h"
#include "DWExpression.h"
#include <link.h>

const char* EHFrame::TAG = PTAG("EHFrame");

int EHFrame::restoreFrame(Context& context){

	CFI& cfi = context.mCfi;

	LOGD(TAG,"restoreFrame ip=0x%x",cfi.ip);

	if(dl_iterate_phdr (EHFrame::checklib, &context) == 1){

		LOGD(TAG,"find target fde&cie success, restore reg states...");

		dwarf_state_record_t& sr = cfi.sr;
		memset (&sr, 0, sizeof (sr));
		for (int i = 0; i < DWARF_NUM_PRESERVED_REGS + 2; ++i){
			(sr).rs_current.reg[i].where = DWARF_WHERE_SAME;
			(sr).rs_current.reg[i].val = 0;
		}

		word_t start = cfi.cie_instr_start;
		word_t end = cfi.cie_instr_end;
		if(DWInstruction::runCFIInstructions(context, cfi, ~(word_t)0, &start, end, &sr) < 0){
			LOGE(TAG,"run cie instructions failed.");
			return 1;
		}

		memcpy (&sr.rs_initial, &sr.rs_current, sizeof (sr.rs_initial));

		start = cfi.fde_instr_start;
		end = cfi.fde_instr_end;

		if(DWInstruction::runCFIInstructions(context, cfi, cfi.ip, &start, end, &sr) < 0){
			LOGE(TAG,"run fde instructions failed.");
			return 1;
		}

		if(applyInstr(context, cfi)){
			LOGE(TAG,"applyInstr failed");
			return 1;
		}

		return 0;
	}

	LOGE(TAG,"restoreFrame(0x%lx) failed, cant find host lib", (long)cfi.ip);

	return 1;
}

int EHFrame::checklib (struct dl_phdr_info *info, size_t size, void *ptr){

	Context& context = *((Context*)ptr);
	CFI& cfi = context.mCfi;

	if (size < OFFSET(struct dl_phdr_info, dlpi_phnum) + sizeof (info->dlpi_phnum)){
		LOGE(TAG,"size not match while iterating libs, continue");
		return 0;
	}

	// LOGD(TAG,"checking %s (0x%x)", info->dlpi_name, info->dlpi_addr);
	// return 0;

	word_t ip = cfi.ip;
	int ret;
	const ElfW(Phdr) *phdr = info->dlpi_phdr;
	const ElfW(Phdr) *p_eh_hdr = NULL;
	const ElfW(Phdr) *p_dynamic =NULL;
	const ElfW(Phdr) *p_text = NULL;
	ElfW(Addr) libbase = info->dlpi_addr;
	ElfW(Addr) max_load_addr = 0;

	for (int n = info->dlpi_phnum; --n >= 0; phdr++){
		if (phdr->p_type == PT_LOAD){
			ElfW(Addr) vaddr = phdr->p_vaddr + libbase;

			if (ip >= vaddr && ip < vaddr + phdr->p_memsz){
				// LOGD(TAG,"find p_text %p in %s", phdr, info->dlpi_name);
				p_text = phdr;
			}

			if (vaddr + phdr->p_filesz > max_load_addr){
				max_load_addr = vaddr + phdr->p_filesz;
			}
		} else if (phdr->p_type == PT_GNU_EH_FRAME){
			// LOGD(TAG,"find p_eh_hdr %p in %s", phdr, info->dlpi_name);
			p_eh_hdr = phdr;
		} else if (phdr->p_type == PT_DYNAMIC){
			p_dynamic = phdr;
		}
    }

    if (!p_text){
    	// LOGE(TAG,"p_text = %p, p_eh_hdr = (%p)", p_text, p_eh_hdr);
    	return 0;
    }

    if(!p_eh_hdr){
    	LOGE(TAG,"not found eh_frame, in %s", info->dlpi_name);
    	return 0;
    }

    LOGD(TAG,"found ip(0x%x) fall in to lib(%s) (0x%lx)", ip, info->dlpi_name, (long)libbase);

    cfi.last_libname = info->dlpi_name;
    cfi.last_libbase = libbase;
    cfi.gp = 0;

    if (p_dynamic){
		/* For dynamicly linked executables and shared libraries,
		 DT_PLTGOT is the value that data-relative addresses are
		 relative to for that object.  We call this the "gp".  */
		ElfW(Dyn) *dyn = (ElfW(Dyn) *)(p_dynamic->p_vaddr + libbase);
		for (; dyn->d_tag != DT_NULL; ++dyn){
			if (dyn->d_tag == DT_PLTGOT){
				/* Assume that _DYNAMIC is writable and GLIBC has
				   relocated it (true for x86 at least).  */
				cfi.gp = dyn->d_un.d_ptr;
				break;
			}
		}
	}

	eh_frame_hdr_t *eh_frame_hdr = (eh_frame_hdr_t*)(p_eh_hdr->p_vaddr + libbase);
	if(eh_frame_hdr->version != EH_VERSION){
		LOGE(TAG,"not support eh version %d",eh_frame_hdr->version);
		return -1;
	}

	word_t addr, eh_frame_start, eh_frame_end, fde_count;
	addr = (word_t) (uintptr_t) (eh_frame_hdr + 1);

	/* (Optionally) read eh_frame_ptr: */
	if ((ret = readEncodedPointer(context, &addr, eh_frame_hdr->eh_frame_ptr_enc, &eh_frame_start)) < 0){
		LOGE(TAG,"read eh_frame_start failed.");
		return ret;
	}

	/* (Optionally) read fde_count: */
	if ((ret = readEncodedPointer(context, &addr, eh_frame_hdr->fde_count_enc, &fde_count)) < 0){
		LOGE(TAG,"read fde_count failed");
		return ret;
	}

	LOGD(TAG,"eh_frame_start(%p) table_enc(0x%x), eh_frame_ptr_enc(0x%x) fde_count(%d) fde_count_enc(0x%x)", 
		eh_frame_start, eh_frame_hdr->table_enc, eh_frame_hdr->eh_frame_ptr_enc, fde_count , eh_frame_hdr->fde_count_enc);

	if (eh_frame_hdr->table_enc != (DW_EH_PE_datarel | DW_EH_PE_sdata4)){
		LOGD(TAG,"table_enc not support now.");
	}else{
		word_t segbase = (word_t)(uintptr_t)eh_frame_hdr;
		int32_t relIP = cfi.ip - segbase;
		table_entry* entry = findEntry((table_entry*)addr,fde_count,relIP);
		if(entry == NULL){
			LOGE(TAG,"find fde entry failed.");
			return -1;
		}

		word_t fde_addr = entry->fde_offset + segbase;
		if(extractCFI(context, &fde_addr) == 0){
			LOGD(TAG,"find cfi success, ip(0x%x - 0x%x), cie_instr(0x%x - 0x%x), fde_instr(0x%x - 0x%x)", 
	    		cfi.start_ip, cfi.end_ip, cfi.cie_instr_start, cfi.cie_instr_end, cfi.fde_instr_start, cfi.fde_instr_end);

	    	return 1;
		}else{
			LOGE(TAG,"extractCFI failed.");
		}
	}

    LOGE(TAG,"parse eh_frame failed");
    
    return 2;
}

table_entry* EHFrame::findEntry(table_entry* start, uint64_t size, int32_t rel_ip){
	struct table_entry *e = NULL;
	uint64_t lo, hi, mid;

	/* do a binary search for right entry: */
	for (lo = 0, hi = size; lo < hi;){
		mid = (lo + hi) / 2;
		e = start + mid;
		// LOGD(TAG, "e->start_ip_offset = 0x%lx\n", (long) e->start_ip_offset);
		if (rel_ip < e->start_ip_offset){
			hi = mid;
		}else{
			lo = mid + 1;
		}
	}

	if (hi <= 0){
		return NULL;
	}

	e = start + hi - 1;
	return e;
}

int EHFrame::extractCIE(Context& context, word_t* pCIEAddr){
	CFI& cfi = context.mCfi;

	word_t addr = *pCIEAddr;
	LOGD(TAG, "extractCIE at address 0x%lx", addr);

	uint8_t version, ch, augstr[5], fde_encoding, handler_encoding;
	word_t len, cie_end_addr, aug_size;
	uint32_t u32val;
	uint64_t u64val;
	size_t i;
	int ret;

	/* Pick appropriate default for FDE-encoding.  DWARF spec says
	start-IP (initial_location) and the code-size (address_range) are
	"address-unit sized constants".  The `R' augmentation can be used
	to override but by default, we pick an address-sized unit
	for fde_encoding.  */
	switch (context.getAddrWidth()){
		case 4:	fde_encoding = DW_EH_PE_udata4; break;
		case 8:	fde_encoding = DW_EH_PE_udata8; break;
		default:	fde_encoding = DW_EH_PE_omit; break;
	}

	cfi.lsda_encoding = DW_EH_PE_omit;
	cfi.handler = 0;

	if ((ret = context.readU32 (&addr, &u32val)) < 0){
		return ret;
	}

	if (u32val != 0xffffffff){
		/* the CIE is in the 32-bit DWARF format */
		uint32_t cie_id;
		/* DWARF says CIE id should be 0xffffffff, but in .eh_frame, it's 0 */
		const uint32_t expected_id = 0;

		len = u32val;
		cie_end_addr = addr + len;
		if ((ret = context.readU32 (&addr, &cie_id)) < 0){
			return ret;
		}
		if (cie_id != expected_id) {
			LOGE (TAG, "Unexpected CIE id 0x%x", cie_id);
			return -1;
		}
	} else {
		/* the CIE is in the 64-bit DWARF format */
		uint64_t cie_id;
		/* DWARF says CIE id should be 0xffffffffffffffff, but in
		.eh_frame, it's 0 */
		const uint64_t expected_id = 0;

		if ((ret = context.readU64 (&addr, &u64val)) < 0){
			return ret;
		}

		len = u64val;
		cie_end_addr = addr + len;
		if ((ret = context.readU64 (&addr, &cie_id)) < 0){
			return ret;
		}

		if (cie_id != expected_id){
			LOGE (TAG, "Unexpected CIE id 0x%llx", cie_id);
			return -1;
		}
	}

	cfi.cie_instr_end = cie_end_addr;

	if ((ret = context.readU8 (&addr, &version)) < 0){
		return ret;
	}

	if (version != 1 && version != 3 && version != 4) {
		LOGE (TAG, "Got CIE version %u, expected version 1, 3 or 4; ", version);
		return -1;
	}

	/* read and parse the augmentation string: */
	memset (augstr, 0, sizeof (augstr));
	for (i = 0;;) {
		if ((ret = context.readU8 (&addr, &ch	)) < 0){
			return ret;
		}

		if (!ch){
			break;	/* end of augmentation string */
		}

		if (i < sizeof (augstr) - 1){
			augstr[i++] = ch;
		}
	}

	if (version == 4) {
		uint8_t address_size;
		if ((ret = context.readU8(&addr, &address_size)) < 0) {
			return ret;
		}
		if (address_size != sizeof(word_t)) {
			return -1;
		}
		uint8_t segment_size;
		if ((ret = context.readU8(&addr, &segment_size)) < 0) {
			return ret;
		}
		// We don't support non-zero segment size.
		if (segment_size != 0) {
			LOGE(TAG,"segment_size is not 0 (0x%x)",segment_size);
			return -1;
		}
	}


	if ((ret = context.readUleb128 (&addr, &cfi.code_align)) < 0
	|| (ret = context.readSleb128 (&addr, &cfi.data_align)) < 0){
		return ret;
	}

	/* Read the return-address column either as a u8 or as a uleb128.  */
	if (version == 1){
		if ((ret = context.readU8 (&addr, &ch)) < 0){
			return ret;
		}
		cfi.ret_addr_column = ch;
	} else if ((ret = context.readUleb128 (&addr, &cfi.ret_addr_column)) < 0){
		return ret;
	}

	i = 0;
	if (augstr[0] == 'z'){
		cfi.sized_augmentation = 1;
		if ((ret = context.readUleb128 (&addr, &aug_size)) < 0){
			return ret;
		}

		i++;
	}

	for (; i < sizeof (augstr) && augstr[i]; ++i){
		switch (augstr[i]){
			case 'L':
				/* read the LSDA pointer-encoding format.  */
				if ((ret = context.readU8 (&addr, &ch)) < 0){
					return ret;
				}
				cfi.lsda_encoding = ch;
				break;

			case 'R':
				/* read the FDE pointer-encoding format.  */
				if ((ret = context.readU8(&addr, &fde_encoding)) < 0){
					return ret;
				}
				break;

			case 'P':
				/* read the personality-routine pointer-encoding format.  */
				if ((ret = context.readU8 (&addr, &handler_encoding)) < 0){
					return ret;
				}

				if ((ret = readEncodedPointer (context, &addr, handler_encoding, &cfi.handler)) < 0){
					return ret;
				}
				break;

			case 'S':
				/* This is a signal frame. */
				cfi.signal_frame = 1;
				cfi.have_abi_marker = 1;
				break;

			default:
				LOGE (TAG, "Unexpected augmentation string `%s'\n", augstr);
				if (!cfi.sized_augmentation){
					return -1;
				}
		}
	}

	cfi.fde_encoding = fde_encoding;
	cfi.cie_instr_start = addr;
	LOGD (TAG, "CIE parsed OK, augmentation = \"%s\", handler=0x%lx, cie_instr_start(0x%lx) cie_instr_end(0x%lx) data_align(0x%lx) code_align(0x%lx)", 
		augstr, (long) cfi.handler, (long)cfi.cie_instr_start, (long)cfi.cie_instr_end, (long)(cfi.data_align), (long)(cfi.code_align));
	return 0;
}

int EHFrame::extractCFI(Context& context, word_t* addrp){
	LOGD(TAG,"extractCFI from 0x%x", *addrp);

	CFI& cfi = context.mCfi;

	word_t fde_end_addr, cie_addr, cie_offset_addr, aug_end_addr = 0;
	word_t start_ip, ip_range, aug_size, addr = *addrp;
	int ret, ip_range_encoding;
	uint64_t u64val;
	uint32_t u32val;

	if ((ret = context.readU32 (&addr, &u32val)) < 0){
		return ret;
	}

	if (u32val != 0xffffffff){
		int32_t cie_offset;

		/* In some configurations, an FDE with a 0 length indicates the
		end of the FDE-table.  */
		if (u32val == 0){
			LOGE(TAG,"end of FDE-table");
			return -1;
		}

		/* the FDE is in the 32-bit DWARF format */

		*addrp = fde_end_addr = addr + u32val;
		cie_offset_addr = addr;

		if ((ret = context.readS32(&addr, &cie_offset)) < 0){
			return ret;
		}

		if (cie_offset == 0){
			LOGE(TAG,"cie pointer is 0");
			return -1;
		}

		cie_addr = cie_offset_addr - cie_offset;
	} else {
		int64_t cie_offset;

		/* the FDE is in the 64-bit DWARF format */
		if ((ret = context.readU64 (&addr, &u64val)) < 0){
			return -1;
		}

		*addrp = fde_end_addr = addr + u64val;
		cie_offset_addr = addr;

		if ((ret = context.readS64 (&addr, &cie_offset)) < 0){
			return -1;
		}

		if (cie_offset == 0){
			LOGE(TAG,"cie pointer is 0");
			return -1;
		}

		cie_addr = cie_offset_addr - cie_offset;
	}

	if ((ret = extractCIE (context, &cie_addr)) < 0){
		return -1;
	}

	/* IP-range has same encoding as FDE pointers, except that it's
	always an absolute value: */
	ip_range_encoding = cfi.fde_encoding & DW_EH_PE_FORMAT_MASK;

	if ((ret = readEncodedPointer (context, &addr, cfi.fde_encoding, &start_ip)) < 0
		|| (ret = readEncodedPointer (context, &addr, ip_range_encoding, &ip_range)) < 0){
		return -1;
	}

	cfi.start_ip = start_ip;
	cfi.end_ip = start_ip + ip_range;

	if (cfi.sized_augmentation){
		if ((ret = context.readUleb128 (&addr, &aug_size)) < 0){
			LOGE(TAG,"read aug size failed.");
			return -1;
		}

		aug_end_addr = addr + aug_size;
	}

	if(cfi.lsda_encoding != DW_EH_PE_omit){
		if ((ret = readEncodedPointer (context, &addr, cfi.lsda_encoding, &cfi.lsda)) < 0){
			LOGE(TAG,"read lsda failed");
			return -1;
		}
	}

	LOGD(TAG,"FDE covers IP 0x%lx-0x%lx, LSDA=0x%lx\n", (long) cfi.start_ip, (long) cfi.end_ip, (long) cfi.lsda);

	if (cfi.have_abi_marker){
		if ((ret = context.readU16 (&addr, &cfi.abi)) < 0
		|| (ret = context.readU16 (&addr, &cfi.tag)) < 0){
			return -1;
		}
		
		LOGE(TAG, "Found ABI marker = (abi=%u, tag=%u)\n", cfi.abi, cfi.tag);
	}

	if (cfi.sized_augmentation){
		cfi.fde_instr_start = aug_end_addr;
	}else{
		cfi.fde_instr_start = addr;
	}

	cfi.fde_instr_end = fde_end_addr;
	
	return 0;
}

int EHFrame::applyInstr(Context& context, CFI& cfi){
	word_t regnum, addr;
	int ret = 0, isRegister = 0;
	dwarf_reg_state* rs = &cfi.sr.rs_current;
	word_t old_ip = cfi.ip;
	word_t old_cfa = cfi.cfa;

	/* Evaluate the CFA first, because it may be referred to by other
	expressions.  */
	if (rs->reg[DWARF_CFA_REG_COLUMN].where == DWARF_WHERE_REG){
		/* CFA is equal to [reg] + offset: */

		/* As a special-case, if the stack-pointer is the CFA and the
		stack-pointer wasn't saved, popping the CFA implicitly pops
		the stack-pointer as well.  */
		if ((rs->reg[DWARF_CFA_REG_COLUMN].val == UNW_TDEP_SP)
			&& (UNW_TDEP_SP < ARRAY_SIZE(rs->reg))
			&& (rs->reg[UNW_TDEP_SP].where == DWARF_WHERE_SAME)){
			LOGE(TAG,"same cfa!");
		} else {
			word_t dwarfReg = rs->reg[DWARF_CFA_REG_COLUMN].val;
			if(dwarfReg >= DWARF_NUM_PRESERVED_REGS){
				LOGE(TAG,"dwarfReg out of range: %lu", dwarfReg);
				return -1;
			}

			cfi.cfa = cfi.loc[dwarfReg] + (rs->reg[DWARF_CFA_OFF_COLUMN].val);
			LOGD(TAG,"new cfa = 0x%lx, loc[%d] + offset(%d)", (long)cfi.cfa, dwarfReg, (rs->reg[DWARF_CFA_OFF_COLUMN].val));
		}
	} else {
		/* CFA is equal to EXPR: */
		if(rs->reg[DWARF_CFA_REG_COLUMN].where != DWARF_WHERE_EXPR){
			LOGE(TAG,"it should be expression type");
			return -1;
		}

		addr = rs->reg[DWARF_CFA_REG_COLUMN].val;

		if ((ret = DWExpression::evaluate(context, cfi, &addr, cfi.cfa, isRegister)) < 0){
			LOGE(TAG,"caculate expression failed.");
			return ret;
		}

		if(isRegister){
			LOGE(TAG,"cfa cannot be in register");
			return -1;
		}
	}

	bool ret_addr_column_undefined = false;
	for (int i = 0; i < DWARF_NUM_PRESERVED_REGS; ++i){
		switch ((dwarf_where_t) rs->reg[i].where){
		case DWARF_WHERE_UNDEF:
			LOGE(TAG,"undef reg(%d)", i);
			if(i == cfi.ret_addr_column){
				LOGE(TAG,"return addr cannot undefined");
				cfi.root = 1;
			}
			break;

		case DWARF_WHERE_SAME:
			break;

		case DWARF_WHERE_CFAREL:
			cfi.loc[i] = *(word_t*)(cfi.cfa + rs->reg[i].val);
			LOGD(TAG,"loc[%d] = cfa[0x%lx] offset %d , 0x%lx", i, (long)cfi.cfa, rs->reg[i].val, (long)cfi.loc[i]);
			break;

		case DWARF_WHERE_REG:
			regnum = rs->reg[i].val;
			if(regnum >= DWARF_NUM_PRESERVED_REGS){
				LOGE(TAG,"regnum out of range.");
				return -1;
			}

			cfi.loc[i] = cfi.loc[regnum];
			LOGD(TAG,"loc[%d] = loc[%d] = 0x%lx", i, regnum, cfi.loc[i]);
			break;

		case DWARF_WHERE_EXPR:
			addr = rs->reg[i].val;
			if ((ret = DWExpression::evaluate (context, cfi, &addr, cfi.loc[i], isRegister)) < 0){
				LOGE(TAG, "evaluate DWExpression failed %d", ret);
				return ret;
			}
			LOGD(TAG,"loc[%d] = 0x%lx", i, cfi.loc[i]);
			break;
		}
	}

	LOGD(TAG,"ret_addr_column = %d", cfi.ret_addr_column);
	/* DWARF spec says undefined return address location means end of stack. */
	if (cfi.ret_addr_column < DWARF_NUM_PRESERVED_REGS) {
		cfi.ip = cfi.loc[cfi.ret_addr_column] - 1;
	}

	LOGD(TAG,"after apply ip(0x%lx) old ip(0x%lx) cfa(0x%lx) old cfa(0x%lx) ",
		(long)cfi.ip, (long)old_ip, (long)cfi.cfa, (long) old_cfa);

	/* XXX: check for ip to be code_aligned */
	if (cfi.ip == old_ip && cfi.cfa == old_cfa){
		cfi.root = 1;
		LOGE(TAG, "ip and cfa unchanged; stopping here (ip=0x%lx)", (long) cfi.ip);
	}

	return 0;
}

int EHFrame::readEncodedPointer(Context& context, word_t *addr, uint8_t encoding, word_t *valp){
	LOGD(TAG,"readEncodedPointer encoding(0x%x) from addr(%p)",encoding, addr);

	CFI& cfi = context.mCfi;
	word_t val, initial_addr = *addr;
	uint16_t uval16;
	uint32_t uval32;
	uint64_t uval64;
	int16_t sval16;
	int32_t sval32;
	int64_t sval64;
	int ret;

	int ws = sizeof(word_t);

	/* DW_EH_PE_omit and DW_EH_PE_aligned don't follow the normal
	format/application encoding.  Handle them first.  */
	if (encoding == DW_EH_PE_omit){
		*valp = 0;
		return -1;
	}else if (encoding == DW_EH_PE_aligned){
		*addr = (initial_addr + ws - 1) & -ws;
		return context.readW(addr, valp);
	}

	LOGD(TAG,"encoding & DW_EH_PE_FORMAT_MASK 0x%x, addr(%p)", encoding & DW_EH_PE_FORMAT_MASK, *addr);

	switch (encoding & DW_EH_PE_FORMAT_MASK){
		case DW_EH_PE_ptr:
			if ((ret = context.readW(addr, &val)) < 0) return ret;
			break;

		case DW_EH_PE_uleb128:
			if ((ret = context.readUleb128(addr, &val)) < 0) return ret;
			break;

		case DW_EH_PE_udata2:
			if ((ret = context.readU16(addr, &uval16)) < 0) return ret;
			val = uval16;
			break;

		case DW_EH_PE_udata4:
			if ((ret = context.readU32(addr, &uval32)) < 0) return ret;
			val = uval32;
			break;

		case DW_EH_PE_udata8:
			if ((ret = context.readU64(addr, &uval64)) < 0) return ret;
			val = uval64;
			break;

		case DW_EH_PE_sleb128:
			if ((ret = context.readUleb128(addr, &val)) < 0) return ret;
			break;

		case DW_EH_PE_sdata2:
			if ((ret = context.readS16(addr, &sval16)) < 0) return ret;
			val = sval16;
			break;

		case DW_EH_PE_sdata4:
			if ((ret = context.readS32(addr, &sval32)) < 0) return ret;
			val = sval32;
			break;

		case DW_EH_PE_sdata8:
			if ((ret = context.readS64(addr, &sval64)) < 0) return ret;
			val = sval64;
			break;

		default:
			LOGE (TAG, "unexpected encoding format 0x%x\n", encoding & DW_EH_PE_FORMAT_MASK);
			return -1;
	}

	if (val == 0){
		/* 0 is a special value and always absolute.  */
		*valp = 0;
		return 0;
	}

	LOGD(TAG,"val(0x%x) encoding & DW_EH_PE_APPL_MASK 0x%x ,addr(%p)", val, encoding & DW_EH_PE_APPL_MASK, *addr);

	switch (encoding & DW_EH_PE_APPL_MASK){
		case DW_EH_PE_absptr:
			break;

		case DW_EH_PE_pcrel:
			val += initial_addr;
			break;

		case DW_EH_PE_datarel:
			/* XXX For now, assume that data-relative addresses are relative
			to the global pointer.  */
			val += cfi.gp;
			break;

		case DW_EH_PE_funcrel:
			val += cfi.start_ip;
			break;

		case DW_EH_PE_textrel:
			/* XXX For now we don't support text-rel values.  If there is a
			platform which needs this, we probably would have to add a
			"segbase" member to unw_proc_info_t.  */
		default:
			LOGE(TAG, "unexpected application type 0x%x\n", encoding & DW_EH_PE_APPL_MASK);
			return -1;
	}

	/* Trim off any extra bits.  Assume that sign extension isn't
	required; the only place it is needed is MIPS kernel space
	addresses.  */
	if (sizeof (val) > ws){
		if(ws != 4){
			LOGE(TAG,"word size not 4!");
			return -1;
		}
		val = (uint32_t) val;
	}

	if (encoding & DW_EH_PE_indirect){
		word_t indirect_addr = val;
		if ((ret = context.readW(&indirect_addr, &val)) < 0){
			return ret;
		}
	}

	*valp = val;
	return 0;
}

