class Register
{
public:
	int ready_flag, tag;

public:
	Register();
	~Register();
};

Register::Register()
	{
		ready_flag = 1;
		tag = -1;
	}
Register::~Register(){}
