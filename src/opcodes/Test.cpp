/*-------------------------------------*\
|
|  File Name: Test.cpp
|
|  Creation Date: 06-10-2012
|
|  Last Modified: Tue, Oct  9, 2012  4:54:40 PM
|
|  Created By: Robert Nelson
|
\*-------------------------------------*/

#include "Test.hpp"
#include "../ModrmOperand.hpp"
#include "../ImmediateOperand.hpp"
#include "../RegisterOperand.hpp"
#include "../Processor.hpp"

Test::Test(Prefix* pre, std::string text, std::string inst, int op) {
	mPrefix = pre;
	mText = text;
	mInst = inst;
	mOpcode = op;
	mValid = true;
}

Instruction* Test::CreateInstruction(unsigned char* memLoc, Processor* proc) {
	unsigned char* opLoc = memLoc;
	char buf[65];
	std::string inst;

	Prefix* pre = Prefix::GetPrefix(memLoc);
	unsigned int preSize = 0;
	Instruction* newTest = 0;

	if(pre) {
		opLoc += preSize = pre->GetLength();
	}

	switch(*opLoc) {
		case TEST_AL_IMM8:
		case TEST_AX_IMM16:
		{
			unsigned int size = (*opLoc == TEST_AL_IMM8 ? 1 : 2);

			unsigned int val = (int)*(opLoc + 1);
			if(size == 2) {
				val += (int)*(opLoc + 2) << 8;
			}

			GETINST(preSize + 1 + size);

			Operand* src = new ImmediateOperand(val, size);
			Operand* dst = new RegisterOperand(*opLoc == TEST_AL_IMM8 ? REG_AL : REG_AX, proc);

			newTest = new Test(pre, buf, inst, (unsigned char)*opLoc);
			newTest->SetOperand(Operand::SRC, src);
			newTest->SetOperand(Operand::DST, dst);
			break;
		}
		case TEST_MOD8_IMM8:
		case TEST_MOD16_IMM16:
		{
			unsigned int size = (*opLoc == TEST_MOD8_IMM8 ? 1 : 2);

			unsigned int val = (int)*(opLoc + 1);
			if(size == 2) {
				val += (int)*(opLoc + 2) << 8;
			}


			Operand* src = new ImmediateOperand(val, size);
			Operand* dst = ModrmOperand::GetModrmOperand(proc, opLoc, ModrmOperand::MOD, size);

			GETINST(preSize + 1 + size + dst->GetBytecodeLen());
			newTest = new Test(pre, buf, inst, (unsigned char)*opLoc);
			newTest->SetOperand(Operand::SRC, src);
			newTest->SetOperand(Operand::DST, dst);
			break;
		}
		case TEST_MOD8_REG8:
		case TEST_MOD16_REG16:
		{
			unsigned int size = (*opLoc == TEST_MOD8_REG8 ? 1 : 2);
			
			Operand* src = ModrmOperand::GetModrmOperand(proc, opLoc, ModrmOperand::REG, size);
			Operand* dst = ModrmOperand::GetModrmOperand(proc, opLoc, ModrmOperand::MOD, size);

			GETINST(preSize + 2 + dst->GetBytecodeLen() + src->GetBytecodeLen());
			newTest = new Test(pre, buf, inst, (unsigned char)*opLoc);
			newTest->SetOperand(Operand::SRC, src);
			newTest->SetOperand(Operand::DST, dst);
			break;
		}

	}

	return newTest;

}

int Test::Execute(Processor* proc) {

	if(mOperands[Operand::SRC] == 0 || mOperands[Operand::DST] == 0)
		return -1;

	unsigned int val = mOperands[Operand::SRC]->GetValue() & mOperands[Operand::DST]->GetValue();
	unsigned int sign = mOperands[Operand::DST]->GetBitmask() == 0xFF ? 0x80 : 0x8000;

	unsigned int parity = val;
	parity ^= parity >> 16;
	parity ^= parity >> 8;
	parity ^= parity >> 4;
	parity &= 0x0f;
	
	proc->SetFlag(FLAGS_PF, (0x6996 >> parity) & 1);
	proc->SetFlag(FLAGS_ZF, val == 0);
	proc->SetFlag(FLAGS_SF, val >= sign);

	return 0;
}
