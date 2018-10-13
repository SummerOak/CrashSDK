#include "DWExpression.h"
#include "Defines.h"
#include "Log.h"

/* The "pick" operator provides an index range of 0..255 indicating
   that the stack could at least have a depth of up to 256 elements,
   but the GCC unwinder restricts the depth to 64, which seems
   reasonable so we use the same value here.  */
#define MAX_EXPR_STACK_SIZE   64

#define NUM_OPERANDS(signature)  (((signature) >> 6) & 0x3)
#define OPND1_TYPE(signature) (((signature) >> 3) & 0x7)
#define OPND2_TYPE(signature) (((signature) >> 0) & 0x7)

#define OPND_SIGNATURE(n, t1, t2) (((n) << 6) | ((t1) << 3) | ((t2) << 0))
#define OPND1(t1)    OPND_SIGNATURE(1, t1, 0)
#define OPND2(t1, t2)      OPND_SIGNATURE(2, t1, t2)

#define VAL8   0x0
#define VAL16  0x1
#define VAL32  0x2
#define VAL64  0x3
#define ULEB128   0x4
#define SLEB128   0x5
#define OFFSET_VAL 0x6   /* 32-bit offset for 32-bit DWARF, 64-bit otherwise */
#define ADDR   0x7   /* Machine address.  */

typedef enum {
   DW_OP_addr       = 0x03,
   DW_OP_deref         = 0x06,
   DW_OP_const1u    = 0x08,
   DW_OP_const1s    = 0x09,
   DW_OP_const2u    = 0x0a,
   DW_OP_const2s    = 0x0b,
   DW_OP_const4u    = 0x0c,
   DW_OP_const4s    = 0x0d,
   DW_OP_const8u    = 0x0e,
   DW_OP_const8s    = 0x0f,
   DW_OP_constu     = 0x10,
   DW_OP_consts     = 0x11,
   DW_OP_dup        = 0x12,
   DW_OP_drop       = 0x13,
   DW_OP_over       = 0x14,
   DW_OP_pick       = 0x15,
   DW_OP_swap       = 0x16,
   DW_OP_rot        = 0x17,
   DW_OP_xderef     = 0x18,
   DW_OP_abs        = 0x19,
   DW_OP_and        = 0x1a,
   DW_OP_div        = 0x1b,
   DW_OP_minus         = 0x1c,
   DW_OP_mod        = 0x1d,
   DW_OP_mul        = 0x1e,
   DW_OP_neg        = 0x1f,
   DW_OP_not        = 0x20,
   DW_OP_or         = 0x21,
   DW_OP_plus       = 0x22,
   DW_OP_plus_uconst      = 0x23,
   DW_OP_shl        = 0x24,
   DW_OP_shr        = 0x25,
   DW_OP_shra       = 0x26,
   DW_OP_xor        = 0x27,
   DW_OP_skip       = 0x2f,
   DW_OP_bra        = 0x28,
   DW_OP_eq         = 0x29,
   DW_OP_ge         = 0x2a,
   DW_OP_gt         = 0x2b,
   DW_OP_le         = 0x2c,
   DW_OP_lt         = 0x2d,
   DW_OP_ne         = 0x2e,
   DW_OP_lit0       = 0x30,
   DW_OP_lit1,  DW_OP_lit2,  DW_OP_lit3,  DW_OP_lit4,  DW_OP_lit5,
   DW_OP_lit6,  DW_OP_lit7,  DW_OP_lit8,  DW_OP_lit9,  DW_OP_lit10,
   DW_OP_lit11, DW_OP_lit12, DW_OP_lit13, DW_OP_lit14, DW_OP_lit15,
   DW_OP_lit16, DW_OP_lit17, DW_OP_lit18, DW_OP_lit19, DW_OP_lit20,
   DW_OP_lit21, DW_OP_lit22, DW_OP_lit23, DW_OP_lit24, DW_OP_lit25,
   DW_OP_lit26, DW_OP_lit27, DW_OP_lit28, DW_OP_lit29, DW_OP_lit30,
   DW_OP_lit31,
   DW_OP_reg0       = 0x50,
   DW_OP_reg1,  DW_OP_reg2,  DW_OP_reg3,  DW_OP_reg4,  DW_OP_reg5,
   DW_OP_reg6,  DW_OP_reg7,  DW_OP_reg8,  DW_OP_reg9,  DW_OP_reg10,
   DW_OP_reg11, DW_OP_reg12, DW_OP_reg13, DW_OP_reg14, DW_OP_reg15,
   DW_OP_reg16, DW_OP_reg17, DW_OP_reg18, DW_OP_reg19, DW_OP_reg20,
   DW_OP_reg21, DW_OP_reg22, DW_OP_reg23, DW_OP_reg24, DW_OP_reg25,
   DW_OP_reg26, DW_OP_reg27, DW_OP_reg28, DW_OP_reg29, DW_OP_reg30,
   DW_OP_reg31,
   DW_OP_breg0         = 0x70,
   DW_OP_breg1,  DW_OP_breg2,  DW_OP_breg3,  DW_OP_breg4,  DW_OP_breg5,
   DW_OP_breg6,  DW_OP_breg7,  DW_OP_breg8,  DW_OP_breg9,  DW_OP_breg10,
   DW_OP_breg11, DW_OP_breg12, DW_OP_breg13, DW_OP_breg14, DW_OP_breg15,
   DW_OP_breg16, DW_OP_breg17, DW_OP_breg18, DW_OP_breg19, DW_OP_breg20,
   DW_OP_breg21, DW_OP_breg22, DW_OP_breg23, DW_OP_breg24, DW_OP_breg25,
   DW_OP_breg26, DW_OP_breg27, DW_OP_breg28, DW_OP_breg29, DW_OP_breg30,
   DW_OP_breg31,
   DW_OP_regx       = 0x90,
   DW_OP_fbreg         = 0x91,
   DW_OP_bregx         = 0x92,
   DW_OP_piece         = 0x93,
   DW_OP_deref_size    = 0x94,
   DW_OP_xderef_size      = 0x95,
   DW_OP_nop        = 0x96,
   DW_OP_push_object_address = 0x97,
   DW_OP_call2         = 0x98,
   DW_OP_call4         = 0x99,
   DW_OP_call_ref      = 0x9a,
   DW_OP_lo_user    = 0xe0,
   DW_OP_hi_user    = 0xff
}dwarf_expr_op_t;

const uint8_t DWExpression::OPERANDS[256] = {
   [DW_OP_addr] =    OPND1 (ADDR),
   [DW_OP_const1u] =    OPND1 (VAL8),
   [DW_OP_const1s] =    OPND1 (VAL8),
   [DW_OP_const2u] =    OPND1 (VAL16),
   [DW_OP_const2s] =    OPND1 (VAL16),
   [DW_OP_const4u] =    OPND1 (VAL32),
   [DW_OP_const4s] =    OPND1 (VAL32),
   [DW_OP_const8u] =    OPND1 (VAL64),
   [DW_OP_const8s] =    OPND1 (VAL64),
   [DW_OP_pick] =    OPND1 (VAL8),
   [DW_OP_plus_uconst] =   OPND1 (ULEB128),
   [DW_OP_skip] =    OPND1 (VAL16),
   [DW_OP_bra] =     OPND1 (VAL16),
   [DW_OP_breg0 +  0] = OPND1 (SLEB128),
   [DW_OP_breg0 +  1] = OPND1 (SLEB128),
   [DW_OP_breg0 +  2] = OPND1 (SLEB128),
   [DW_OP_breg0 +  3] = OPND1 (SLEB128),
   [DW_OP_breg0 +  4] = OPND1 (SLEB128),
   [DW_OP_breg0 +  5] = OPND1 (SLEB128),
   [DW_OP_breg0 +  6] = OPND1 (SLEB128),
   [DW_OP_breg0 +  7] = OPND1 (SLEB128),
   [DW_OP_breg0 +  8] = OPND1 (SLEB128),
   [DW_OP_breg0 +  9] = OPND1 (SLEB128),
   [DW_OP_breg0 + 10] = OPND1 (SLEB128),
   [DW_OP_breg0 + 11] = OPND1 (SLEB128),
   [DW_OP_breg0 + 12] = OPND1 (SLEB128),
   [DW_OP_breg0 + 13] = OPND1 (SLEB128),
   [DW_OP_breg0 + 14] = OPND1 (SLEB128),
   [DW_OP_breg0 + 15] = OPND1 (SLEB128),
   [DW_OP_breg0 + 16] = OPND1 (SLEB128),
   [DW_OP_breg0 + 17] = OPND1 (SLEB128),
   [DW_OP_breg0 + 18] = OPND1 (SLEB128),
   [DW_OP_breg0 + 19] = OPND1 (SLEB128),
   [DW_OP_breg0 + 20] = OPND1 (SLEB128),
   [DW_OP_breg0 + 21] = OPND1 (SLEB128),
   [DW_OP_breg0 + 22] = OPND1 (SLEB128),
   [DW_OP_breg0 + 23] = OPND1 (SLEB128),
   [DW_OP_breg0 + 24] = OPND1 (SLEB128),
   [DW_OP_breg0 + 25] = OPND1 (SLEB128),
   [DW_OP_breg0 + 26] = OPND1 (SLEB128),
   [DW_OP_breg0 + 27] = OPND1 (SLEB128),
   [DW_OP_breg0 + 28] = OPND1 (SLEB128),
   [DW_OP_breg0 + 29] = OPND1 (SLEB128),
   [DW_OP_breg0 + 30] = OPND1 (SLEB128),
   [DW_OP_breg0 + 31] = OPND1 (SLEB128),
   [DW_OP_regx] =    OPND1 (ULEB128),
   [DW_OP_fbreg] =      OPND1 (SLEB128),
   [DW_OP_bregx] =      OPND2 (ULEB128, SLEB128),
   [DW_OP_piece] =      OPND1 (ULEB128),
   [DW_OP_deref_size] = OPND1 (VAL8),
   [DW_OP_xderef_size] =   OPND1 (VAL8),
   [DW_OP_call2] =      OPND1 (VAL16),
   [DW_OP_call4] =      OPND1 (VAL32),
   [DW_OP_call_ref] =      OPND1 (OFFSET_VAL)
};

const char* DWExpression::TAG = PTAG("DWExpression");

int DWExpression::evaluate(Context& context, CFI& cfi, word_t* addr, word_t& val, int& is_register){

	int ret;
	word_t len;

	/* read the length of the expression: */
	if ((ret = context.readUleb128(addr, &len)) < 0){
		return ret;
	}

	if((ret = evaluate(context, cfi, addr, len, val, is_register)) != 0){
		LOGE(TAG,"evaluate expression val failed");
		return ret;
	}

	LOGD(TAG,"evaluate DWExpression result = 0x%lx",(long)val);

	if (is_register){
		if(val >= DWARF_NUM_PRESERVED_REGS){
			LOGE(TAG,"regNum out of range.");
			return -1;
		}

		val = cfi.loc[val];
	}else{
		val = *(word_t*)val;
	}

	return 0;
}


int DWExpression::evaluate(Context& context, CFI& cfi, word_t* addr, word_t len, word_t& val, int& is_register){

	LOGD(TAG,"evaluate DWExpression %u",len);
	
	word_t operand1 = 0, operand2 = 0, tmp1, tmp2, tmp3, end_addr;
	sword_t stmp1,stmp2;
	uint8_t opcode, operands_signature, u8;
	word_t stack[MAX_EXPR_STACK_SIZE];
	unsigned int tos = 0;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
	int ret, regNum;


	# define pop() \
		({\
			if ((tos - 1) >= MAX_EXPR_STACK_SIZE){\
				LOGE (TAG, "Stack underflow");\
				return -1;\
			}\
			stack[--tos];\
		})

	# define push(x) \
		do{\
			if (tos >= MAX_EXPR_STACK_SIZE){\
				LOGE (TAG, "Stack overflow");\
				return -1;\
			}\
			stack[tos++] = (x);\
		} while (0)

	# define pick(n)\
	({\
		unsigned int _index = tos - 1 - (n);\
		if (_index >= MAX_EXPR_STACK_SIZE){\
			LOGE (TAG, "Out-of-stack pick");\
			return -1;\
		}\
		stack[_index];\
	})

	end_addr = *addr + len;

	LOGD (TAG, "len=%lu, pushing cfa=0x%lx",(unsigned long) len, (unsigned long) cfi.cfa);

	push (cfi.cfa);	/* push current CFA as required by DWARF spec */

	while (*addr < end_addr){
		if ((ret = context.readU8(addr,&opcode)) < 0){
			return ret;
		}

		operands_signature = OPERANDS[opcode];

		if (NUM_OPERANDS (operands_signature) > 0){
			if ((ret = read_operand (context, addr, OPND1_TYPE (operands_signature),&operand1)) < 0){
				return ret;
			}

			if (NUM_OPERANDS (operands_signature) > 1){
				if ((ret = read_operand (context, addr,OPND2_TYPE (operands_signature),&operand2)) < 0){
					return ret;
				}
			}
		}

		switch ((dwarf_expr_op_t) opcode){
			case DW_OP_lit0:  case DW_OP_lit1:  case DW_OP_lit2:
			case DW_OP_lit3:  case DW_OP_lit4:  case DW_OP_lit5:
			case DW_OP_lit6:  case DW_OP_lit7:  case DW_OP_lit8:
			case DW_OP_lit9:  case DW_OP_lit10: case DW_OP_lit11:
			case DW_OP_lit12: case DW_OP_lit13: case DW_OP_lit14:
			case DW_OP_lit15: case DW_OP_lit16: case DW_OP_lit17:
			case DW_OP_lit18: case DW_OP_lit19: case DW_OP_lit20:
			case DW_OP_lit21: case DW_OP_lit22: case DW_OP_lit23:
			case DW_OP_lit24: case DW_OP_lit25: case DW_OP_lit26:
			case DW_OP_lit27: case DW_OP_lit28: case DW_OP_lit29:
			case DW_OP_lit30: case DW_OP_lit31:
				LOGD (TAG, "OP_lit(%d)", (int) opcode - DW_OP_lit0);
				push (opcode - DW_OP_lit0);
				break;

			case DW_OP_breg0:  case DW_OP_breg1:  case DW_OP_breg2:
			case DW_OP_breg3:  case DW_OP_breg4:  case DW_OP_breg5:
			case DW_OP_breg6:  case DW_OP_breg7:  case DW_OP_breg8:
			case DW_OP_breg9:  case DW_OP_breg10: case DW_OP_breg11:
			case DW_OP_breg12: case DW_OP_breg13: case DW_OP_breg14:
			case DW_OP_breg15: case DW_OP_breg16: case DW_OP_breg17:
			case DW_OP_breg18: case DW_OP_breg19: case DW_OP_breg20:
			case DW_OP_breg21: case DW_OP_breg22: case DW_OP_breg23:
			case DW_OP_breg24: case DW_OP_breg25: case DW_OP_breg26:
			case DW_OP_breg27: case DW_OP_breg28: case DW_OP_breg29:
			case DW_OP_breg30: case DW_OP_breg31:
				regNum = opcode - DW_OP_breg0;
				LOGD (TAG, "OP_breg(r%d,0x%lx)",regNum, (unsigned long) operand1);
				if(0 > regNum || regNum >= DWARF_NUM_PRESERVED_REGS){
					LOGE(TAG,"regNum out of range");
					return -1;
				}

				tmp1 = cfi.loc[regNum];
				push (tmp1 + operand1);
				break;

			case DW_OP_bregx:
				regNum = (int)operand1;
				LOGD (TAG, "OP_bregx(r%d,0x%lx)", regNum, (unsigned long) operand2);
				
				if(0 > regNum || regNum >= DWARF_NUM_PRESERVED_REGS){
					LOGE(TAG,"regNum out of range");
					return -1;
				}

				tmp1 = cfi.loc[regNum];
				push (tmp1 + operand2);
				break;

			case DW_OP_reg0:  case DW_OP_reg1:  case DW_OP_reg2:
			case DW_OP_reg3:  case DW_OP_reg4:  case DW_OP_reg5:
			case DW_OP_reg6:  case DW_OP_reg7:  case DW_OP_reg8:
			case DW_OP_reg9:  case DW_OP_reg10: case DW_OP_reg11:
			case DW_OP_reg12: case DW_OP_reg13: case DW_OP_reg14:
			case DW_OP_reg15: case DW_OP_reg16: case DW_OP_reg17:
			case DW_OP_reg18: case DW_OP_reg19: case DW_OP_reg20:
			case DW_OP_reg21: case DW_OP_reg22: case DW_OP_reg23:
			case DW_OP_reg24: case DW_OP_reg25: case DW_OP_reg26:
			case DW_OP_reg27: case DW_OP_reg28: case DW_OP_reg29:
			case DW_OP_reg30: case DW_OP_reg31:
				LOGD (TAG, "OP_reg(r%d)", (int) opcode - DW_OP_reg0);
				val = (opcode - DW_OP_reg0);
				is_register = 1;
				return 0;

			case DW_OP_regx:
				LOGD (TAG, "OP_regx(r%d)", (int) operand1);
				val = (operand1);
				is_register = 1;
				return 0;

			case DW_OP_addr:
			case DW_OP_const1u:
			case DW_OP_const2u:
			case DW_OP_const4u:
			case DW_OP_const8u:
			case DW_OP_constu:
			case DW_OP_const8s:
			case DW_OP_consts:
				LOGD (TAG, "OP_const(0x%lx)", (unsigned long) operand1);
				push (operand1);
				break;

			case DW_OP_const1s:
				if (operand1 & 0x80){
					operand1 |= ((word_t) -1) << 8;
				}

				LOGD (TAG, "OP_const1s(%ld)", (long) operand1);
				push (operand1);
				break;

			case DW_OP_const2s:
				if (operand1 & 0x8000){
					operand1 |= ((word_t) -1) << 16;
				}
				LOGD (TAG, "OP_const2s(%ld)", (long) operand1);
				push (operand1);
				break;

			case DW_OP_const4s:
				if (operand1 & 0x80000000){
					operand1 |= (((word_t) -1) << 16) << 16;
				}
				LOGD(TAG, "OP_const4s(%ld)", (long) operand1);
				push (operand1);
				break;

			case DW_OP_deref:
				LOGD(TAG, "OP_deref");
				tmp1 = pop ();
				if ((ret = context.readW (&tmp1, &tmp2)) < 0){
					return ret;
				}
				push (tmp2);
				break;

			case DW_OP_deref_size:
				LOGD(TAG, "OP_deref_size(%d)", (int) operand1);
				tmp1 = pop ();
				switch (operand1){
					case 1:
						if ((ret = context.readU8 (&tmp1, &u8)) < 0){
							return ret;
						}
						tmp2 = u8;
						break;

					case 2:
						if ((ret = context.readU16 (&tmp1, &u16)) < 0){
							return ret;
						}
						tmp2 = u16;
						break;

					case 3:
					case 4:
						if ((ret = context.readU32 (&tmp1, &u32)) < 0){
							return ret;
						}
						tmp2 = u32;
						if (operand1 == 3){
							if (context.isBigEndian()){
								tmp2 >>= 8;
							}else{
								tmp2 &= 0xffffff;
							}
						}
						break;
					case 5:
					case 6:
					case 7:
					case 8:
						if ((ret = context.readU64 (&tmp1, &u64)) < 0){
							return ret;
						}
						tmp2 = u64;
						if (operand1 != 8){
							if (context.isBigEndian()){
								tmp2 >>= 64 - 8 * operand1;
							} else{
								tmp2 &= (~ (word_t) 0) << (8 * operand1);
							}
						}
						break;

					default: 
							LOGD (TAG, "Unexpected DW_OP_deref_size size %d", (int) operand1);
							return -1;
				}

				push (tmp2);
				break;

			case DW_OP_dup:
				LOGD(TAG, "OP_dup");
				push (pick (0));
				break;

			case DW_OP_drop:
				LOGD(TAG, "OP_drop");
				(void) pop ();
				break;

			case DW_OP_pick:
				LOGD(TAG, "OP_pick(%d)", (int) operand1);
				push (pick (operand1));
				break;

			case DW_OP_over:
				LOGD(TAG, "OP_over");
				push (pick (1));
				break;

			case DW_OP_swap:
				LOGD(TAG, "OP_swap");
				tmp1 = pop ();
				tmp2 = pop ();
				push (tmp1);
				push (tmp2);
				break;

			case DW_OP_rot:
				LOGD(TAG, "OP_rot");
				tmp1 = pop ();
				tmp2 = pop ();
				tmp3 = pop ();
				push (tmp1);
				push (tmp3);
				push (tmp2);
				break;

			case DW_OP_abs:
				LOGD(TAG, "OP_abs");
				tmp1 = pop ();
				if (tmp1 & ((word_t) 1 << (8 * context.getAddrWidth() - 1))){
					tmp1 = -tmp1;
				}
				push (tmp1);
				break;

			case DW_OP_and:
				LOGD(TAG, "OP_and");
				tmp1 = pop ();
				tmp2 = pop ();
				push (tmp1 & tmp2);
				break;

			case DW_OP_div:
				LOGD(TAG, "OP_div");
				tmp1 = pop ();
				tmp2 = pop ();
				if (tmp1){
					if(context.sword(tmp2,stmp2) == 0 && context.sword(tmp1,stmp1) == 0){
						tmp1 = stmp2/stmp1;
					}else{
						LOGE(TAG,"change word_t to sword_t failed.");
						return -1;
					}
				}
				push (tmp1);
				break;

			case DW_OP_minus:
				LOGD(TAG, "OP_minus");
				tmp1 = pop ();
				tmp2 = pop ();
				tmp1 = tmp2 - tmp1;
				push (tmp1);
				break;

			case DW_OP_mod:
				LOGD(TAG, "OP_mod");
				tmp1 = pop ();
				tmp2 = pop ();
				if (tmp1){
					tmp1 = tmp2 % tmp1;
				}
				push (tmp1);
				break;

			case DW_OP_mul:
				LOGD(TAG, "OP_mul");
				tmp1 = pop ();
				tmp2 = pop ();
				if (tmp1){
					tmp1 = tmp2 * tmp1;
				}
				push (tmp1);
				break;

			case DW_OP_neg:
				LOGD(TAG, "OP_neg");
				push (-pop ());
				break;

			case DW_OP_not:
				LOGD(TAG, "OP_not");
				push (~pop ());
				break;

			case DW_OP_or:
				LOGD(TAG, "OP_or");
				tmp1 = pop ();
				tmp2 = pop ();
				push (tmp1 | tmp2);
				break;

			case DW_OP_plus:
				LOGD(TAG, "OP_plus");
				tmp1 = pop ();
				tmp2 = pop ();
				push (tmp1 + tmp2);
				break;

			case DW_OP_plus_uconst:
				LOGD(TAG, "OP_plus_uconst(%lu)", (unsigned long) operand1);
				tmp1 = pop ();
				push (tmp1 + operand1);
				break;

			case DW_OP_shl:
				LOGD(TAG, "OP_shl");
				tmp1 = pop ();
				tmp2 = pop ();
				push (tmp2 << tmp1);
				break;

			case DW_OP_shr:
				LOGD(TAG, "OP_shr");
				tmp1 = pop ();
				tmp2 = pop ();
				push (tmp2 >> tmp1);
				break;

			case DW_OP_shra:
				LOGD(TAG, "OP_shra");
				tmp1 = pop ();
				tmp2 = pop ();
				if(context.sword(tmp2,stmp2)){
					LOGE(TAG,"signed change failed.");
					return -1;
				}
				push (stmp2 >> tmp1);
				break;

			case DW_OP_xor:
				LOGD(TAG, "OP_xor");
				tmp1 = pop ();
				tmp2 = pop ();
				push (tmp1 ^ tmp2);
				break;

			case DW_OP_le:
				LOGD(TAG, "OP_le");
				tmp1 = pop ();
				tmp2 = pop ();
				if(context.sword(tmp1,stmp1) == 0 && context.sword(tmp2,stmp2) == 0){
					push (stmp2 <= stmp1);
				}else{
					LOGE(TAG,"sword failed.");
					return -1;
				}
				
				break;

			case DW_OP_ge:
				LOGD(TAG, "OP_ge");
				tmp1 = pop ();
				tmp2 = pop ();
				if(context.sword(tmp1, stmp1) == 0 && context.sword(tmp2, stmp2) == 0){
					push (stmp2 >= stmp1);
				}else{
					LOGE(TAG,"sword failed.");
					return -1;
				}
				break;

			case DW_OP_eq:
			LOGD(TAG, "OP_eq");
			tmp1 = pop ();
			tmp2 = pop ();
			if(context.sword(tmp1, stmp1) == 0 && context.sword(tmp2, stmp2) == 0){
				push (stmp2 == stmp1);
			}else{
				LOGE(TAG,"sword failed.");
				return -1;
			}
			break;

			case DW_OP_lt:
			LOGD(TAG, "OP_lt");
			tmp1 = pop ();
			tmp2 = pop ();

			if(context.sword(tmp1, stmp1) == 0 && context.sword(tmp2, stmp2) == 0){
				push (stmp2 < stmp1);
			}else{
				LOGE(TAG,"sword failed.");
				return -1;
			}
			break;

			case DW_OP_gt:
			LOGD(TAG, "OP_gt");
			tmp1 = pop ();
			tmp2 = pop ();

			if(context.sword(tmp1, stmp1) == 0 && context.sword(tmp2, stmp2) == 0){
				push (stmp2 > stmp1);
			}else{
				LOGE(TAG,"sword failed.");
				return -1;
			}
			break;

			case DW_OP_ne:
			LOGD(TAG, "OP_ne");
			tmp1 = pop ();
			tmp2 = pop ();

			if(context.sword(tmp1, stmp1) == 0 && context.sword(tmp2, stmp2) == 0){
				push (stmp2 != stmp1);
			}else{
				LOGE(TAG,"sword failed.");
				return -1;
			}
			break;

			case DW_OP_skip:
				LOGD(TAG, "OP_skip(%d)", (int16_t) operand1);
				*addr += (int16_t) operand1;
				break;

			case DW_OP_bra:
				LOGD(TAG, "OP_skip(%d)", (int16_t) operand1);
				tmp1 = pop ();
				if (tmp1)
				*addr += (int16_t) operand1;
				break;

			case DW_OP_nop:
				LOGD(TAG, "OP_nop");
				break;

			case DW_OP_call2:
			case DW_OP_call4:
			case DW_OP_call_ref:
			case DW_OP_fbreg:
			case DW_OP_piece:
			case DW_OP_push_object_address:
			case DW_OP_xderef:
			case DW_OP_xderef_size:
			default:
				LOGE (TAG, "Unexpected opcode 0x%x", opcode);
				return -1;
		}
	}

	val = pop ();
	LOGD (TAG, "final value = 0x%lx", (unsigned long)val);

	return 0;
}

int DWExpression::read_operand (Context& context, word_t *addr, int operand_type, word_t *val, void *arg){
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
	int ret;

	if (operand_type == ADDR){
		switch (context.getAddrWidth()){
			case 1: operand_type = VAL8; break;
			case 2: operand_type = VAL16; break;
			case 4: operand_type = VAL32; break;
			case 8: operand_type = VAL64; break;
			default: 
				LOGE(TAG,"error addr width");
				return -1;
		}

		switch (operand_type){
			case VAL8:
				if((ret = context.readU8 (addr, &u8, arg)) < 0){
					return ret;
				}

				*val = u8;
				break;

			case VAL16:
				if((ret = context.readU16 (addr, &u16, arg)) < 0){
					return ret;
				}

				*val = u16;
				break;

			case VAL32:
				if((ret = context.readU32 (addr, &u32, arg)) < 0){
					return ret;
				}

				*val = u32;
				break;

			case VAL64:
				if((ret = context.readU64 (addr, &u64, arg)) < 0){
					return ret;
				}

				*val = u64;
				break;

			case ULEB128:
				if((ret = context.readUleb128 (addr, val, arg)) < 0){
					return ret;
				}

				break;

			case SLEB128:
				if((ret = context.readSleb128 (addr, val, arg)) < 0){
					return ret;
				}

				break;

			case OFFSET_VAL: /* only used by DW_OP_call_ref, which we don't implement */
			default:
				LOGE(TAG, "Unexpected operand type %d\n", operand_type);
				ret = -1;
		}
	}
		
	return ret;
}

