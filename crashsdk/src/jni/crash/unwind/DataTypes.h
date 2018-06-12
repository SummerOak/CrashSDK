#ifndef DataTypes_h
#define DataTypes_h

#include "Arch.h"
#include <stdint.h>

typedef struct table_entry{
	int32_t start_ip_offset;
	int32_t fde_offset;
}table_entry;

typedef struct eh_frame_hdr{
	uint8_t version;
	uint8_t eh_frame_ptr_enc;
	uint8_t fde_count_enc;
	uint8_t table_enc;
	/* The rest of the header is variable-length and consists of the
	   following members:

	encoded_t eh_frame_ptr;
	encoded_t fde_count;
	struct
	  {
	    encoded_t start_ip;	// first address covered by this FDE
	    encoded_t fde_addr;	// address of the FDE
	  }
	binary_search_table[fde_count];  */
}eh_frame_hdr_t;

typedef enum{
	DWARF_WHERE_UNDEF,		/* register isn't saved at all */
	DWARF_WHERE_SAME,		/* register has same value as in prev. frame */
	DWARF_WHERE_CFAREL,		/* register saved at CFA-relative address */
	DWARF_WHERE_REG,		/* register saved in another register */
	DWARF_WHERE_EXPR,		/* register saved */
}dwarf_where_t;

typedef struct{
	dwarf_where_t where;	/* how is the register saved? */
	word_t val;		/* where it's saved */
}dwarf_save_loc_t;

#define DWARF_CFA_REG_COLUMN	DWARF_NUM_PRESERVED_REGS
#define DWARF_CFA_OFF_COLUMN	(DWARF_NUM_PRESERVED_REGS + 1)

typedef struct dwarf_reg_state{
	struct dwarf_reg_state *next;	/* for rs_stack */
	dwarf_save_loc_t reg[DWARF_NUM_PRESERVED_REGS + 2];
	word_t ip;		          /* ip this rs is for */
	word_t ret_addr_column;       /* indicates which column in the rule table represents return address */
	unsigned short lru_chain;	  /* used for least-recently-used chain */
	unsigned short coll_chain;	/* used for hash collisions */
	unsigned short hint;	      /* hint for next rs to try (or -1) */
	unsigned short valid : 1;         /* optional machine-dependent signal info */
	unsigned short signal_frame : 1;  /* optional machine-dependent signal info */
}dwarf_reg_state_t;

typedef struct dwarf_state_record{
	unsigned char fde_encoding;
	word_t args_size;

	dwarf_reg_state_t rs_initial;	/* reg-state after CIE instructions */
	dwarf_reg_state_t rs_current;	/* current reg-state */
}dwarf_state_record_t;

typedef struct CallFrameInfo{

	word_t ip;	//current ip of this call frame
	word_t cfa; //current cfa of this call frame

	const char* last_libname; //the name of lib the last ip located in.
    word_t last_libbase; // this load base addr of lib the last ip locate in.

	word_t loc[DWARF_NUM_PRESERVED_REGS];
	
	uint8_t root : 1;
	uint8_t last : 1;
	uint8_t signal_frame : 1;

	//eh_frame info start>>>
	word_t gp = 0;
	uint8_t version;
	word_t code_align;
	word_t data_align;
	word_t handler;
    uint16_t abi;
    uint16_t tag;
    uint8_t fde_encoding;
    uint8_t lsda_encoding;
    
    word_t lsda;

    word_t start_ip;
	word_t end_ip;
    word_t cie_instr_start;	/* start addr. of CIE "initial_instructions" */
    word_t cie_instr_end;	/* end addr. of CIE "initial_instructions" */
    word_t fde_instr_start;	/* start addr. of FDE "instructions" */
    word_t fde_instr_end;	/* end addr. of FDE "instructions" */

	uint8_t sized_augmentation : 1;
    uint8_t have_abi_marker : 1;

	dwarf_state_record_t sr;
    
    word_t ret_addr_column;

	//<<<eh_frame info end

    //exidx info >>>
    word_t exidx_table_start;
    word_t exidx_table_end;
    word_t exidx_entry;
    //<<< exidx info
    

}CFI;

typedef union __attribute__ ((packed)){
	int8_t s8;
	int16_t s16;
	int32_t s32;
	int64_t s64;
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
	void *ptr;
}value_t;

#endif