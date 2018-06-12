#ifndef Exidx_h
#define Exidx_h

#include "Context.h"

#define ARM_EXBUF_START(x)	(((x) >> 4) & 0x0f)
#define ARM_EXBUF_COUNT(x)	((x) & 0x0f)
#define ARM_EXBUF_END(x)	(ARM_EXBUF_START(x) + ARM_EXBUF_COUNT(x))

#define ARM_EXIDX_CANT_UNWIND	0x00000001
#define ARM_EXIDX_COMPACT	0x80000000

#define ARM_EXTBL_OP_FINISH	0xb0

enum arm_exbuf_cmd_flags {
  ARM_EXIDX_VFP_SHIFT_16 = 1 << 16,
  ARM_EXIDX_VFP_DOUBLE = 1 << 17,
};

typedef enum arm_exbuf_cmd {
	ARM_EXIDX_CMD_FINISH,
	ARM_EXIDX_CMD_DATA_PUSH,
	ARM_EXIDX_CMD_DATA_POP,
	ARM_EXIDX_CMD_REG_POP,
	ARM_EXIDX_CMD_REG_TO_SP,
	ARM_EXIDX_CMD_VFP_POP,
	ARM_EXIDX_CMD_WREG_POP,
	ARM_EXIDX_CMD_WCGR_POP,
	ARM_EXIDX_CMD_RESERVED,
	ARM_EXIDX_CMD_REFUSED,
} arm_exbuf_cmd_t;

typedef struct arm_exbuf_data{
	arm_exbuf_cmd_t cmd;
	uint32_t data;
}arm_exbuf_data;

class Exidx{

public:

	static int restoreFrame(Context& context);
	static int checklib (struct dl_phdr_info *info, size_t size, void *ptr);
	static int retriveEntry(Context& context, word_t s, word_t e);
	static int readUnwindInstr(Context& context, word_t entry, uint8_t* instr);
	static int runExidxInstr(const uint8_t* instr, uint8_t len, CFI& cfi);
	static int prel31_to_addr(word_t prel31, word_t *val);

private:
	static const char* TAG;

	static int applyInstr(arm_exbuf_data& edata, CFI& cfi);

};

#endif