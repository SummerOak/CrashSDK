#include "DWInstruction.h"
#include "Defines.h"
#include "Log.h"

#define DWARF_CFA_OPCODE_MASK 0xc0
#define DWARF_CFA_OPERAND_MASK   0x3f
#define SETREG(sr,regnum, w, v) \
	(sr)->rs_current.reg[(regnum)].where = (w); \
	(sr)->rs_current.reg[(regnum)].val = (v); \

typedef enum{
   DW_CFA_advance_loc     = 0x40,
   DW_CFA_offset    = 0x80,
   DW_CFA_restore      = 0xc0,
   DW_CFA_nop       = 0x00,
   DW_CFA_set_loc      = 0x01,
   DW_CFA_advance_loc1    = 0x02,
   DW_CFA_advance_loc2    = 0x03,
   DW_CFA_advance_loc4    = 0x04,
   DW_CFA_offset_extended = 0x05,
   DW_CFA_restore_extended   = 0x06,
   DW_CFA_undefined    = 0x07,
   DW_CFA_same_value      = 0x08,
   DW_CFA_register     = 0x09,
   DW_CFA_remember_state  = 0x0a,
   DW_CFA_restore_state   = 0x0b,
   DW_CFA_def_cfa      = 0x0c,
   DW_CFA_def_cfa_register   = 0x0d,
   DW_CFA_def_cfa_offset  = 0x0e,
   DW_CFA_def_cfa_expression = 0x0f,
   DW_CFA_expression      = 0x10,
   DW_CFA_offset_extended_sf = 0x11,
   DW_CFA_def_cfa_sf      = 0x12,
   DW_CFA_def_cfa_offset_sf  = 0x13,
   DW_CFA_lo_user      = 0x1c,
   DW_CFA_MIPS_advance_loc8  = 0x1d,
   DW_CFA_GNU_window_save = 0x2d,
   DW_CFA_GNU_args_size   = 0x2e,
   DW_CFA_GNU_negative_offset_extended   = 0x2f,
   DW_CFA_hi_user      = 0x3c
}dwarf_cfa_t;

const char* DWInstruction::TAG = PTAG("DWInstruction");

int DWInstruction::read_regnum (Context& context, word_t *addr, word_t *valp, void *arg) {
	int ret;

	if ((ret = context.readUleb128 (addr, valp, arg)) < 0){
		return ret;
	}

	if (*valp >= DWARF_NUM_PRESERVED_REGS){
		LOGE (TAG, "Invalid register number %u", (unsigned int) *valp);
		return -1;
	}

	return 0;
}

int DWInstruction::runCFIInstructions(Context& context, CFI& cfi, word_t ip, word_t* addr, word_t end_addr, dwarf_state_record_t *sr){
	
	word_t curr_ip, operand = 0, regnum, val, len, fde_encoding;
	dwarf_reg_state_t *rs_stack = NULL, *new_rs, *old_rs;
	uint8_t u8, op;
	uint16_t u16;
	uint32_t u32;
	void *arg;
	int ret;

	curr_ip = cfi.start_ip;

	LOGD(TAG,"run cfi instructions(0x%x - 0x%x), ip(0x%x - 0x%x)", *addr, end_addr, curr_ip, ip);

	/* Process everything up to and including the current 'ip',
	including all the DW_CFA_advance_loc instructions.  See
	'c->use_prev_instr' use in 'fetch_proc_info' for details. */
	while (curr_ip <= ip && *addr < end_addr){
		if ((ret = context.readU8 (addr, &op, arg)) < 0){
			LOGE(TAG,"read op failed(%d)",ret);
			return ret;
		}

		if (op & DWARF_CFA_OPCODE_MASK){
			operand = op & DWARF_CFA_OPERAND_MASK;
			op &= ~DWARF_CFA_OPERAND_MASK;
		}

		LOGD(TAG,"excute op(%x)", op);

		switch ((dwarf_cfa_t) op){
			case DW_CFA_advance_loc:
				curr_ip += operand * cfi.code_align;
				LOGD (TAG, "CFA_advance_loc to 0x%lx", (long) curr_ip);
				break;

			case DW_CFA_advance_loc1:
				if ((ret = context.readU8 (addr, &u8, arg)) < 0){
					goto fail;
				}
				curr_ip += u8 * cfi.code_align;
				LOGD (TAG, "CFA_advance_loc1 to 0x%lx", (long) curr_ip);
				break;

			case DW_CFA_advance_loc2:
				if ((ret = context.readU16 (addr, &u16, arg)) < 0){
					goto fail;
				}
				curr_ip += u16 * cfi.code_align;
				LOGD (TAG, "CFA_advance_loc2 to 0x%lx", (long) curr_ip);
				break;

			case DW_CFA_advance_loc4:
				if ((ret = context.readU32 (addr, &u32, arg)) < 0){
					goto fail;
				}
				curr_ip += u32 * cfi.code_align;
				LOGD (TAG, "CFA_advance_loc4 to 0x%lx", (long) curr_ip);
				break;

			case DW_CFA_MIPS_advance_loc8:
				#ifdef __mips__
					uint64_t u64;
					if ((ret = context.readU64 (addr, &u64, arg)) < 0){
						goto fail;
					}
					curr_ip += u64 * cfi.code_align;
					LOGD (TAG, "CFA_MIPS_advance_loc8");
					break;
				#else
					LOGD (TAG, "DW_CFA_MIPS_advance_loc8 on non-MIPS target");
					ret = -1;
					goto fail;
				#endif

			case DW_CFA_offset:
				regnum = operand;
				if (regnum >= DWARF_NUM_PRESERVED_REGS){
					LOGE (TAG, "Invalid register number %u in DW_cfa_OFFSET", (unsigned int) regnum);
					ret = -1;
					goto fail;
				}
				if ((ret = context.readUleb128 (addr, &val, arg)) < 0){
					goto fail;
				}

				SETREG (sr, regnum, DWARF_WHERE_CFAREL, val * cfi.data_align);
				LOGD(TAG, "CFA_offset r%lu at cfa+0x%lx",(long) regnum, (long) (val * cfi.data_align));
				break;

			case DW_CFA_offset_extended:
				if (((ret = read_regnum (context, addr, &regnum, arg)) < 0)
				|| ((ret = context.readUleb128 (addr, &val, arg)) < 0)){
					goto fail;
				}

				SETREG (sr, regnum, DWARF_WHERE_CFAREL, val * cfi.data_align);
				LOGD(TAG, "CFA_offset_extended r%lu at cf+0x%lx", (long) regnum, (long) (val * cfi.data_align));
				break;

			case DW_CFA_offset_extended_sf:
				if (((ret = read_regnum (context, addr, &regnum, arg)) < 0)
				|| ((ret = context.readSleb128 (addr, &val, arg)) < 0)){
					goto fail;
				}

				SETREG (sr, regnum, DWARF_WHERE_CFAREL, val * cfi.data_align);
				LOGD(TAG, "CFA_offset_extended_sf r%lu at cf+0x%lx", (long) regnum, (long) (val * cfi.data_align));
				break;

			case DW_CFA_restore:
				regnum = operand;
				if (regnum >= DWARF_NUM_PRESERVED_REGS){
					LOGE(TAG, "Invalid register number %u in DW_CFA_restore", (unsigned int) regnum);
					ret = -1;
					goto fail;
				}

				sr->rs_current.reg[regnum] = sr->rs_initial.reg[regnum];
				LOGD(TAG, "CFA_restore r%lu", (long) regnum);
				break;

			case DW_CFA_restore_extended:
				if ((ret = context.readUleb128 (addr, &regnum, arg)) < 0){
					goto fail;
				}

				if (regnum >= DWARF_NUM_PRESERVED_REGS){
					LOGE(TAG, "Invalid register number %u in DW_CFA_restore_extended", (unsigned int) regnum);
					ret = -1;
					goto fail;
				}

				sr->rs_current.reg[regnum] = sr->rs_initial.reg[regnum];
				LOGD(TAG, "CFA_restore_extended r%lu", (long) regnum);
				break;

			case DW_CFA_nop:
				break;

			case DW_CFA_set_loc:
				fde_encoding = cfi.fde_encoding;
				if ((ret = EHFrame::readEncodedPointer (context, addr, fde_encoding, &curr_ip)) < 0){
					goto fail;
				}

				LOGD(TAG, "CFA_set_loc to 0x%lx", (long) curr_ip);
				break;

			case DW_CFA_undefined:
				if ((ret = read_regnum (context, addr, &regnum, arg)) < 0){
					goto fail;
				}

				SETREG (sr, regnum, DWARF_WHERE_UNDEF, 0);
				LOGD(TAG, "CFA_undefined r%lu", (long) regnum);
				break;

			case DW_CFA_same_value:
				if ((ret = read_regnum (context, addr, &regnum, arg)) < 0){
					goto fail;
				}

				SETREG (sr, regnum, DWARF_WHERE_SAME, 0);
				LOGD(TAG, "CFA_same_value r%lu", (long) regnum);
				break;

			case DW_CFA_register:
				if (((ret = read_regnum (context, addr, &regnum, arg)) < 0)
				|| ((ret = context.readUleb128 (addr, &val, arg)) < 0)){
					goto fail;
				}

				SETREG (sr, regnum, DWARF_WHERE_REG, val);
				LOGD(TAG, "CFA_register r%lu to r%lu", (long) regnum, (long) val);
				break;

			case DW_CFA_remember_state:
				new_rs = new dwarf_reg_state();
				if (!new_rs) {
					LOGE(TAG, "Out of memory in DW_CFA_remember_state");
					ret = -1;
					goto fail;
				}

				memcpy (new_rs->reg, sr->rs_current.reg, sizeof (new_rs->reg));
				new_rs->next = rs_stack;
				rs_stack = new_rs;
				LOGD(TAG, "CFA_remember_state");
				break;

			case DW_CFA_restore_state:
				if (!rs_stack){
					LOGD(TAG, "register-state stack underflow");
					ret = -1;
					goto fail;
				}

				memcpy (&sr->rs_current.reg, &rs_stack->reg, sizeof (rs_stack->reg));
				old_rs = rs_stack;
				rs_stack = rs_stack->next;
				SAFE_DELETE (old_rs);
				LOGD(TAG, "CFA_restore_state");
				break;

			case DW_CFA_def_cfa:
				if (((ret = read_regnum (context, addr, &regnum, arg)) < 0)
				|| ((ret = context.readUleb128 (addr, &val, arg)) < 0)){
					goto fail;
				}

				SETREG (sr, DWARF_CFA_REG_COLUMN, DWARF_WHERE_REG, regnum);
				SETREG (sr, DWARF_CFA_OFF_COLUMN, (dwarf_where_t)0, val);	/* NOT factored! */
				LOGD(TAG, "CFA_def_cfa r%lu+0x%lx", (long) regnum, (long) val);
				break;

			case DW_CFA_def_cfa_sf:
				if (((ret = read_regnum (context, addr, &regnum, arg)) < 0)
				|| ((ret = context.readSleb128 (addr, &val, arg)) < 0)){
					goto fail;
				}

				SETREG (sr, DWARF_CFA_REG_COLUMN, DWARF_WHERE_REG, regnum);
				SETREG (sr, DWARF_CFA_OFF_COLUMN, (dwarf_where_t)0, val * cfi.data_align);		/* factored! */
				LOGD(TAG, "CFA_def_cfa_sf r%lu+0x%lx",(long) regnum, (long) (val * cfi.data_align));
				break;

			case DW_CFA_def_cfa_register:
				if ((ret = read_regnum (context, addr, &regnum, arg)) < 0){
					goto fail;
				}

				SETREG (sr, DWARF_CFA_REG_COLUMN, DWARF_WHERE_REG, regnum);
				LOGD(TAG, "CFA_def_cfa_register r%lu", (long) regnum);
				break;

			case DW_CFA_def_cfa_offset:
				if ((ret = context.readUleb128 (addr, &val, arg)) < 0){
					goto fail;
				}
				SETREG (sr, DWARF_CFA_OFF_COLUMN, (dwarf_where_t)0, val);	/* NOT factored! */
				LOGD(TAG, "CFA_def_cfa_offset 0x%lx", (long) val);
				break;

			case DW_CFA_def_cfa_offset_sf:
				if ((ret = context.readSleb128 (addr, &val, arg)) < 0){
					goto fail;
				}

				SETREG (sr, DWARF_CFA_OFF_COLUMN, (dwarf_where_t)0, val * cfi.data_align);	/* factored! */
				LOGD(TAG, "CFA_def_cfa_offset_sf 0x%lx", (long) (val * cfi.data_align));
				break;

			case DW_CFA_def_cfa_expression:
				/* Save the address of the DW_FORM_block for later evaluation. */
				SETREG (sr, DWARF_CFA_REG_COLUMN, DWARF_WHERE_EXPR, *addr);

				if ((ret = context.readUleb128 (addr, &len, arg)) < 0){
					goto fail;
				}

				LOGD(TAG, "CFA_def_cfa_expr @ 0x%lx [%lu bytes]", (long) *addr, (long) len);
				*addr += len;
				break;

			case DW_CFA_expression:
				if ((ret = read_regnum (context, addr, &regnum, arg)) < 0){
					goto fail;
				}

				/* Save the address of the DW_FORM_block for later evaluation. */
				SETREG (sr, regnum, DWARF_WHERE_EXPR, *addr);

				if ((ret = context.readUleb128 (addr, &len, arg)) < 0){
					goto fail;
				}

				LOGD(TAG, "CFA_expression r%lu @ 0x%lx [%lu bytes]", (long) regnum, (long) addr, (long) len);
				*addr += len;
				break;

			case DW_CFA_GNU_args_size:
				if ((ret = context.readUleb128 (addr, &val, arg)) < 0){
					goto fail;
				}

				sr->args_size = val;
				LOGD(TAG, "CFA_GNU_args_size %lu", (long) val);
				break;

			case DW_CFA_GNU_negative_offset_extended:
				/* A comment in GCC says that this is obsoleted by
				DW_CFA_offset_extended_sf, but that it's used by older
				PowerPC code.  */
				if (((ret = read_regnum (context, addr, &regnum, arg)) < 0)
				|| ((ret = context.readUleb128 (addr, &val, arg)) < 0)){
					goto fail;
				}

				SETREG (sr, regnum, DWARF_WHERE_CFAREL, -(val * cfi.data_align));
				LOGD(TAG, "CFA_GNU_negative_offset_extended cfa+0x%lx", (long) -(val * cfi.data_align));
				break;

			case DW_CFA_GNU_window_save:
			case DW_CFA_lo_user:
			case DW_CFA_hi_user:
			default:
				LOGD(TAG, "Unexpected CFA opcode 0x%x", op);
				ret = -1;
				goto fail;
		}
	}

	ret = 0;

	fail:
	if(ret != 0){
		 LOGE(TAG,"runCFIInstructions failed.");
	}
	
	/* Free the register-state stack, if not empty already.  */
	while (rs_stack) {
		old_rs = rs_stack;
		rs_stack = rs_stack->next;
		SAFE_DELETE(old_rs);
	}
	return ret;
}