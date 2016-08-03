#include "Instruction.h"

Instruction::Instruction()
	{
		pc = 0; state = -1; operation_type = -1; tag = -1; 
		dest_reg = source1_reg = source2_reg = -1; source1_ready = source2_ready = 0;
		source1_tag = source2_tag = -1; remove_in_dispatch = remove_in_issue = remove_in_execute = remove_in_rob = 0;
	}
Instruction::~Instruction(){}