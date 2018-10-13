#ifndef DWExpression_h
#define DWExpression_h

#include "Context.h"
#include "EHFrame.h"

class DWExpression{

public:

	static int evaluate(Context& context, CFI& cfi, word_t* addr, word_t& val, int& is_register);
	static int evaluate(Context& context, CFI& cfi, word_t* addr, word_t len, word_t& val, int& is_register);
	static int read_operand (Context& context, word_t *addr, int operand_type, word_t *val, void *arg = NULL);

private:
	static const char* TAG;
	static const uint8_t OPERANDS[256];

};

#endif