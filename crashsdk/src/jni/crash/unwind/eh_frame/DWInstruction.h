#ifndef DWInstruction_H
#define DWInstruction_H

#include "Context.h"
#include "EHFrame.h"

class DWInstruction{

public:

	static int runCFIInstructions(Context& context, CFI& cfi, word_t ip, word_t* addr, word_t end_addr, dwarf_state_record_t *sr);

private:
	static int read_regnum (Context& context, word_t *addr, word_t *valp, void *arg);

	static const char* TAG;

};

#endif