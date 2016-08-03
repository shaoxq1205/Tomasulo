
class Instruction
{
public:
	int pc, state, operation_type, tag;
	int dest_reg, source1_reg, source1_ready, source1_tag, source2_reg, source2_ready, source2_tag;
	int latency, finish_time;
	int remove_in_dispatch, remove_in_issue, remove_in_execute, remove_in_rob;
	int start_if, start_id, start_is, start_ex, start_wb, duration_if, duration_id, duration_is, duration_ex, duration_wb;
public:
	Instruction();
	~Instruction();
};

Instruction::Instruction()
	{
		pc = 0; state = -1; operation_type = -1; tag = -1;
		dest_reg = source1_reg = source2_reg = -1; source1_ready = source2_ready = 0;
		source1_tag = source2_tag = -1; remove_in_dispatch = remove_in_issue = remove_in_execute = remove_in_rob = 0;
	}
Instruction::~Instruction(){}
