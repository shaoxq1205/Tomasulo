#include "stdio.h"
#include "math.h"
#include <string>
#include <sstream>
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include "Instruction.h"
#include "Register.h"
using namespace std;

enum state{IF,	ID,	IS,	EX,	WB};
int totalcycles = 0, totalinstructions = 0;
int S, N;
int entries = 0;
int FetchNums = 0, DispatchNums = 0, SchedulingNums = 0, ExecuteNums = 0;

Instruction *dispatchqueue, *dispatchqueuesave;
Instruction *issuequeue, *issuequeuesave;
Instruction *executequeue, *executequeuesave;
Register reg[128];
Instruction FakeROB[1024];
FILE *fp1;

void FakeRetire();
void Execute();
void Issue();
void Dispatch();
void Fetch();
int Advance_Cycle();

int main(int argc, char *argv[])
{
	//S = atoi(argv[1]);
	//N = atoi(argv[2]);
	//char *filename = argv[3];
	S = 8;
	N = 1;
	char *filename;
	filename = "val_trace_gcc1";
	fp1 = fopen(filename, "r");

	dispatchqueue = (Instruction*)malloc(sizeof(Instruction)* 2 * N);
	issuequeue = (Instruction*)malloc(sizeof(Instruction)*S);
	executequeue = (Instruction*)malloc(sizeof(Instruction)* 5 * N);

dispatchqueuesave=dispatchqueue;
issuequeuesave=issuequeue;
executequeuesave=executequeue;


	if (fp1 == 0)
	{
		cout<<"Cannot open Trace file"<<endl;
		exit(0);
	}

	do{
		FakeRetire();
		Execute();
		Issue();
		Dispatch();
		Fetch();
	} while (Advance_Cycle());
	double ipc=(double)totalinstructions / totalcycles;
	cout<<"CONFIGURATION"<<endl;
	cout<<"superscalar bandwidth (N) = "<<N<<endl;
	cout<<"dispatch queue size (2*N) = "<<2*N<<endl;
	cout<<"schedule queue size (S)   = "<<S<<endl;
	cout<<"RESULTS"<<endl;
	cout<<"number of instructions = "<<totalinstructions<<endl;
	cout<<"number of cycles = "<<totalcycles<<endl;
	printf(" IPC         = %.2f\n", ipc);

}

int Advance_Cycle()
{

	if ((!entries) && (feof(fp1)))
		return 0;
	else
	{
		totalcycles++;
		return 1;
	}
}

void Fetch()
{
	while ((!feof(fp1)) && (DispatchNums < 2*N) && (FetchNums < N) )//not reached end, fetch bandwidth not exceeded, dispatch not full
	{
		//Initialize + set to IF state
		fscanf(fp1, "%x %d %d %d %d \n", &(FakeROB[entries].pc),  &(FakeROB[entries].operation_type) ,&(FakeROB[entries].dest_reg), &(FakeROB[entries].source1_reg), &(FakeROB[entries].source2_reg) );
		FakeROB[entries].tag = totalinstructions;
		FakeROB[entries].state = IF;
		FakeROB[entries].start_if = totalcycles;
		totalinstructions++;
		FetchNums ++;
		// 2) Add to dispatchqueue and reserve an entry
		dispatchqueue[DispatchNums] = FakeROB[entries];
		DispatchNums++;
		entries++;
	}
}

void Dispatch()
{

	int count = DispatchNums;
	int readynumber = 0;
	int k = 0;
	int deletenumber = 0;
	Instruction **tempID, **tempIDsave;
	tempID = (Instruction**)malloc(sizeof(Instruction*)*count);
tempIDsave=tempID;

	//Find all the instrs in state ID
	for (int i = 0; i < count; i++)
	{
		if (dispatchqueue[i].state == ID)
		{
			tempID[readynumber] = &dispatchqueue[i];
			readynumber++;
		}
	}
	//Sort ID instrs in asending order by tag
	Instruction *tempID0;
	for (int j = 0; j < readynumber - 1; j++)
	{
		for (int i = 0; i < readynumber - 1 - j; i++)
		{
			if (tempID[i]->tag > tempID[i + 1]->tag)
			{
				tempID0 = tempID[i];
				tempID[i] = tempID[i + 1];
				tempID[i + 1] = tempID0;
			}
		}
	}

	while ((k<readynumber)&&(SchedulingNums < S))
	{
		//also update FakeROB
		for (int i = 0; i < entries; i++)
		{
			if (FakeROB[i].tag == tempID[k]->tag)
			{
				//update parameters
				FakeROB[i].state = IS;
				FakeROB[i].start_is = totalcycles;
				FakeROB[i].duration_id = totalcycles - FakeROB[i].start_id;
				break;
			}
		}

		issuequeue[SchedulingNums] = *tempID[k];
		tempID[k]->remove_in_dispatch = 1;
		tempID[k]->state = IS;
		deletenumber++;

		//update source and destination reg
		if (issuequeue[SchedulingNums].source1_reg == -1)
			issuequeue[SchedulingNums].source1_ready = 1;
		else
		{
			if (reg[issuequeue[SchedulingNums].source1_reg].ready_flag == 1) issuequeue[SchedulingNums].source1_ready = 1;
			if (reg[issuequeue[SchedulingNums].source1_reg].ready_flag == 0)  issuequeue[SchedulingNums].source1_tag = reg[issuequeue[SchedulingNums].source1_reg].tag;
		}
		if (issuequeue[SchedulingNums].source2_reg == -1)
			issuequeue[SchedulingNums].source2_ready = 1;
		else
		{
			if (reg[issuequeue[SchedulingNums].source2_reg].ready_flag == 1) issuequeue[SchedulingNums].source2_ready = 1;
			if (reg[issuequeue[SchedulingNums].source2_reg].ready_flag == 0) issuequeue[SchedulingNums].source2_tag = reg[issuequeue[SchedulingNums].source2_reg].tag;
		}
		if (issuequeue[SchedulingNums].dest_reg != -1)
		{
			reg[issuequeue[SchedulingNums].dest_reg].ready_flag = 0;
			reg[issuequeue[SchedulingNums].dest_reg].tag = issuequeue[SchedulingNums].tag;
		}

		DispatchNums--;
		SchedulingNums++;
		k++;
	}


	// For instructions in the dispatchqueue that are in the IF state, unconditionally transition to the ID state
	for (int i = 0; i < count; i++)
	{
		if (dispatchqueue[i].state == IF)
		{
			dispatchqueue[i].state = ID;
			for (int j = 0; j < entries; j++)
			{
				if (FakeROB[j].tag == dispatchqueue[i].tag)
				{
					//update parameters
					FakeROB[j].state = ID;
					FakeROB[j].start_id = totalcycles;
					FakeROB[j].duration_if = totalcycles - FakeROB[j].start_if;
					break;
				}
			}
			FetchNums--;
		}
	}
	//remove
	if(deletenumber)
	{
		for (int k = 0; k < deletenumber; k++)
		{
			for (int i = 0; i < count; i++)
			{
				if (dispatchqueue[i].remove_in_dispatch == 1)
				{
					for (int j = i; j < (count - 1); j++)
						dispatchqueue[j] = dispatchqueue[j + 1];
					break;
				}
			}
			count--;
		}
	}
//	free(tempIDsave);
}


void Issue()
{

	int count = SchedulingNums;
	int tempID_count = 0;
	Instruction **tempID1, **tempIDsave1;
	tempID1 = (Instruction**)malloc(sizeof(Instruction*)*count);
tempIDsave1=tempID1;

	for (int i = 0; i < count; i++)
	{
		//find source ready instrs
		if ((issuequeue[i].source1_ready == 1) && (issuequeue[i].source2_ready == 1))
		{
			tempID1[tempID_count] = &issuequeue[i];
			tempID_count++;
		}
	}
	 //Sort in ascending order of tags
	Instruction *tempID0;
	for (int j = 0; j < tempID_count - 1; j++)
	{
		for (int i = 0; i < tempID_count - 1 - j; i++)
		{
			if (tempID1[i]->tag > tempID1[i + 1]->tag)
			{
				tempID0 = tempID1[i];
				tempID1[i] = tempID1[i + 1];
				tempID1[i + 1] = tempID0;
			}
		}
	}

	int k = 0;
	int deletenumber = 0;
	// issue up to N
	while ((k < tempID_count) && (k < N))
	{
		//also update in FakeROB
		tempID1[k]->state = EX;
		for (int i = 0; i < entries; i++)
		{
			if (FakeROB[i].tag == tempID1[k]->tag)
			{
				//update parameters
				FakeROB[i].state = EX;
				FakeROB[i].start_ex = totalcycles;
				FakeROB[i].duration_is = totalcycles - FakeROB[i].start_is;
				break;
			}
		}
		//move from issue to exe status
		executequeue[ExecuteNums] = *tempID1[k];
		tempID1[k]->remove_in_issue = 1;
		deletenumber++;

		SchedulingNums--;
		//Simulate delay
		switch ( executequeue[ExecuteNums].operation_type) {
		case 0:
			executequeue[ExecuteNums].finish_time = totalcycles + 1;
			break;
		case 1:
			executequeue[ExecuteNums].finish_time = totalcycles + 3;
			break;
		case 2:
			executequeue[ExecuteNums].finish_time = totalcycles + 7;
		break;
		default:
		cout<<"Incorrect operation type happened!"<<endl;
		  break;
		}
		ExecuteNums++;
		k++;
	}
	//remove
	for (int i = 0; i < deletenumber; i++)
	{
		for (int j = 0; j < count; j++)
		{
			if (issuequeue[j].remove_in_issue == 1)
			{
				for (int k = j; k < (count - 1); k++)
					issuequeue[k] = issuequeue[k + 1];
				break;
			}
		}
		count--;
	}

//	free(tempIDsave1);
}

void Execute()
{

	int count = ExecuteNums;
	int deletenumber = 0;
	for (int i = 0; i < count; i++)
	{
		if (executequeue[i].finish_time == totalcycles)
		{

			executequeue[i].remove_in_execute = 1;
			deletenumber++;
			ExecuteNums--;

			executequeue[i].state = WB;

			for (int k = 0; k < entries; k++)
			{
				if (FakeROB[k].tag == executequeue[i].tag)
				{
					FakeROB[k].state = WB;
					FakeROB[k].start_wb = totalcycles;
					FakeROB[k].duration_ex = totalcycles - FakeROB[k].start_ex;
					break;
				}

			}

			if (executequeue[i].dest_reg != -1)
			{
				if (executequeue[i].tag == reg[executequeue[i].dest_reg].tag)
					reg[executequeue[i].dest_reg].ready_flag = 1;
			}

			for (int k = 0; k < SchedulingNums; k++)
			{
				if (issuequeue[k].source1_tag == executequeue[i].tag) issuequeue[k].source1_ready = 1;
				if (issuequeue[k].source2_tag == executequeue[i].tag) issuequeue[k].source2_ready = 1;
			}
		}
	}

	for (int k = 0; k < deletenumber; k++)
	{
		for (int i = 0; i < count; i++)
		{
			if (executequeue[i].remove_in_execute == 1)
			{
				for (int j = i; j < (count - 1); j++)
					executequeue[j] = executequeue[j + 1];
				break;
			}
		}
		count--;
	}
}

void FakeRetire()
{

	int deletenumber = 0;
	int count = entries;
	for (int i = 0; i < count; i++)
	{
		if (FakeROB[i].state == WB)
		{
			FakeROB[i].remove_in_rob = 1;
			deletenumber++;
			entries--;
			printf("%d fu{%d} src{%d,%d} dst{%d} IF{%d,%d} ID{%d,%d} IS{%d,%d} EX{%d,%d} WB{%d,%d}\n", FakeROB[i].tag, FakeROB[i].operation_type,
				FakeROB[i].source1_reg, FakeROB[i].source2_reg, FakeROB[i].dest_reg, FakeROB[i].start_if, FakeROB[i].duration_if,
				FakeROB[i].start_id,FakeROB[i].duration_id, FakeROB[i].start_is, FakeROB[i].duration_is, FakeROB[i].start_ex,
				FakeROB[i].duration_ex, FakeROB[i].start_wb,1);
		}
		else
			break;
	}

	for (int i = 0; i < entries; i++)
	{
		FakeROB[i] = FakeROB[i + deletenumber];
	}


}

