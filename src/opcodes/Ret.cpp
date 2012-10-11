/*-------------------------------------*\
|
|  File Name: Ret.cpp
|
|  Creation Date: 09-10-2012
|
|  Last Modified: Tue, Oct  9, 2012  4:47:21 PM
|
|  Created By: Robert Nelson
|
\*-------------------------------------*/

#include "Ret.hpp"
#include "../ImmediateOperand.hpp"
#include "../Processor.hpp"

#include <cstdio>

Ret::Ret(Prefix* pre, std::string text, std::string inst, int op) : Instruction(pre,text,inst,op) {}

Instruction* Ret::CreateInstruction(unsigned char* memLoc, Processor* proc) {
	unsigned char* opLoc = memLoc;
	char buf[65];
	std::string inst;

	Prefix* pre = Prefix::GetPrefix(memLoc);
	unsigned int preSize = 0;

	Instruction* newRet = 0;

	if(pre) {
		opLoc += preSize = pre->GetLength();
	}

	switch(*opLoc) {
		case RET_NEAR:
		case RET_FAR:
			GETINST(preSize + 1);
			sprintf(buf, "RET");
			newRet = new Ret(pre, buf, inst, (unsigned int)*opLoc);
			break;
		case RET_NEAR_POP:
		case RET_FAR_POP:
		{
			unsigned int val = (unsigned int)*(opLoc + 1) + ((unsigned int)*(opLoc + 2) << 8);
			Operand* dst = new ImmediateOperand(val, 2);
			sprintf(buf, "RET %s", "");
			GETINST(preSize + 1 + dst->GetBytecodeLen());
			newRet = new Ret(pre, buf, inst, (unsigned int)*opLoc);
			newRet->SetOperand(Operand::DST, dst);
			break;
		}
	}
	return newRet;
}

int Ret::Execute(Processor* proc) {
	Operand* dst = mOperands[Operand::DST];


	switch(mOpcode) {
		case RET_NEAR:
			proc->PopRegister(REG_IP);
			return 0;
			break;
		case RET_FAR:
			proc->PopRegister(REG_IP);
			proc->PopRegister(REG_CS);
			return 0;
			break;
		case RET_NEAR_POP:
			if(dst == 0) {
				return -1;
			}
			proc->PopRegister(REG_IP);
			proc->PopSize(dst->GetValue());
			return 0;
			break;
		case RET_FAR_POP:
			if(dst == 0) {
				return -1;
			}
			proc->PopRegister(REG_IP);
			proc->PopRegister(REG_CS);
			proc->PopSize(dst->GetValue());
			return 0;
			break;
	}
	return -1;

}