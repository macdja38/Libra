/*-------------------------------------*\
|
|  File Name: Processor.cpp
|
|  Creation Date: 26-09-2012
|
|  Last Modified: Thu, Oct 18, 2012 10:57:12 PM
|
|  Created By: Robert Nelson
|
\*-------------------------------------*/

#include "Processor.hpp"
#include "peripherals/AllPeripherals.hpp"

#include <iostream>
#include <cstring>
#include <cstdio>

Processor::Processor(unsigned char* mem): mMem(mem), mLastPort(0xFFFFFFFF), mLastDevice(0) {

}

//Allows the starting address to be changed
int Processor::Initialize(unsigned int startAddr) {


	if(startAddr >= 0x10000) {
		return PROC_ERR_INV_ADDR;
	}

	SetRegister(REG_IP, startAddr);

	_InitializeDevices();

	return PROC_SUCCESS;

}

void Processor::_InitializeDevices() {

	mDevices.push_back(new Screen());
}

//Execute a single instruction
int Processor::Step() {
    int retVal = PROC_SUCCESS;

	//Fetch
	Instruction* inst = Instruction::ReadInstruction(mMem + GetRegister(REG_IP), this);

	//Ensure it exists and is valid 
	if(inst && inst->IsValid()) {
		//Increment IP
		SetRegister(REG_IP, GetRegister(REG_IP) + inst->GetLength());

#ifdef DEBUG
		inst->AddLengthToDisasm();
#endif

		std::cout << inst->GetDisasm() << std::endl;

		//Execute
        if((retVal = inst->Execute(this)) < 0) {
			return PROC_ERR_INST;
		}

	} else {
		return PROC_ERR_INV_INST;
	}

    return retVal;

}

bool Processor::GetFlag(eFlags flag) const {
	return (GetRegister(REG_FLAGS) & (1 << flag)) != 0;
}

void Processor::SetFlag(eFlags flag, bool val) {
	if(val)
		SetRegister(REG_FLAGS, GetRegister(REG_FLAGS) | (1 << flag));
	else
		SetRegister(REG_FLAGS, GetRegister(REG_FLAGS) & ~(1 << flag));
}


unsigned int Processor::GetRegister(eRegisters reg) const {
	if(reg == NumRegisters || reg == HighRegisters || reg == LowRegisters) {
		return 0;
	}

	if(reg < NumRegisters) {
		return mRegisters[reg].GetValue();
	}
	else if(reg < LowRegisters) {
		return GetRegisterLow((eRegisters)(reg - NumRegisters - 1));
	}
	else if(reg < HighRegisters) {
		return GetRegisterHigh((eRegisters)(reg - LowRegisters - 1));
	}
	return 0;
}

const char* Processor::GetRegisterHex(eRegisters reg) const {
    if(reg == NumRegisters || reg == HighRegisters || reg == LowRegisters) {
        return 0;
    }
    if(reg < NumRegisters) {
        return mRegisters[reg].GetHex();
    }
    return 0;
}

void Processor::SetRegister(eRegisters reg, unsigned int val) {
	if(reg == NumRegisters) {
		return;
	}

	if(reg < NumRegisters) {
		mRegisters[reg].SetValue(val);
	} else if(reg < LowRegisters) {
		SetRegisterLow((eRegisters)(reg - NumRegisters - 1), val);
	} else if(reg < HighRegisters) {
		SetRegisterHigh((eRegisters)(reg - LowRegisters - 1), val);
	}

}

unsigned int Processor::GetRegisterLow(eRegisters reg) const {
	if(reg == NumRegisters) {
		return 0;
	}

	return mRegisters[reg].GetValue() & 0xFF;
}

unsigned int Processor::GetRegisterHigh(eRegisters reg) const {
	if(reg == NumRegisters) {
		return 0;
	}

	return (mRegisters[reg].GetValue() & 0xFF00) >> 8;
};

void Processor::SetRegisterLow(eRegisters reg, unsigned int val) {
	if(reg == NumRegisters)
		return;

	mRegisters[reg].SetValue((mRegisters[reg].GetValue() & 0xFF00) | (val & 0xFF));
}

void Processor::SetRegisterHigh(eRegisters reg, unsigned int val) {
	if(reg == NumRegisters)
		return;

	mRegisters[reg].SetValue((mRegisters[reg].GetValue() & 0xFF) | ((val & 0xFF) << 8));
}

unsigned int Processor::GetMemory(unsigned int addr, unsigned int size) {
	unsigned int temp = 0;
	memcpy(&temp, mMem + (addr % 0x10000), size);
	return temp;
}

void Processor::SetMemory(unsigned int addr, unsigned int size, unsigned int val) {

	memcpy(mMem + (addr % 0x10000), &val, size);
}

void Processor::PushRegister(eRegisters reg) {
	SetRegister(REG_SP, (GetRegister(REG_SP) - 2) & 0xFFFF);
	SetMemory(GetRegister(REG_SP) + (GetRegister(REG_SS) << 4), 2, GetRegister(reg));
}

void Processor::PushValue(unsigned int val) {
	SetRegister(REG_SP, (GetRegister(REG_SP) - 2) & 0xFFFF);
	SetMemory(GetRegister(REG_SP) + (GetRegister(REG_SS) << 4), 2, val & 0xFFFF);
}

void Processor::PopRegister(eRegisters reg) {
	SetRegister(reg, GetMemory(GetRegister(REG_SP) + (GetRegister(REG_SS) << 4), 2));
	SetRegister(REG_SP, (GetRegister(REG_SP) + 2) & 0xFFFF);
}

void Processor::PopSize(unsigned int size) {
	SetRegister(REG_SP, (GetRegister(REG_SP) + size) & 0xFFFF);
}

unsigned int Processor::PopValue() {
	unsigned int val = GetMemory(GetRegister(REG_SP), 2) & 0xFFFF;
	SetRegister(REG_SP, (GetRegister(REG_SP) + 2) & 0xFFFF);
	return val;
}

void Processor::Outb(unsigned int port, unsigned char data) {
	if(mLastPort == port && mLastDevice) {
		mLastDevice->Put8(port, data);
		return;
	}
	for(unsigned int i = 0; i < mDevices.size(); i++) {
		if(mDevices[i]->Put8(port, data)){
			mLastDevice = mDevices[i];
			mLastPort = port;
			break;
		}
	}
}

void Processor::Outw(unsigned int port, unsigned short data) {
	if(mLastPort == port && mLastDevice) {
		mLastDevice->Put16(port, data);
		return;
	}
	for(unsigned int i = 0; i < mDevices.size(); i++) {
		if(mDevices[i]->Put16(port, data)){
			mLastDevice = mDevices[i];
			mLastPort = port;
			return;
		}
	}
}

unsigned char Processor::Inb(unsigned int port) {
	if(mLastPort == port && mLastDevice) {
		return mLastDevice->Get8(port);
	}
	unsigned int tmpRet = 0;
	for(unsigned int i = 0; i < mDevices.size(); i++) {
		if((tmpRet = mDevices[i]->Get8(port)) != 0xFFFFFFFF) {
			mLastPort = port;
			mLastDevice = mDevices[i];
			return tmpRet;
		}
	}
	return 0x00;
}

unsigned short Processor::Inw(unsigned int port) {
	if(mLastPort == port && mLastDevice) {
		return mLastDevice->Get16(port);
	}
	unsigned int tmpRet = 0;
	for(unsigned int i = 0; i < mDevices.size(); i++) {
		if((tmpRet = mDevices[i]->Get16(port)) != 0xFFFFFFFF) {
			mLastPort = port;
			mLastDevice = mDevices[i];
			return tmpRet;
		}
	}
	return 0x00;
}
void Processor::ProcDump() {

	std::cout << "++++++++++++++++++++++++ BEGIN DUMP ++++++++++++++++++++++++" << std::endl; 

	std::cout << "Registers:" << std::endl;
	std::cout << std::hex;
	printf( "\tAX = 0x%04X" , mRegisters[REG_AX].GetValue());
	printf( "\tBX = 0x%04X\n" , mRegisters[REG_BX].GetValue());
	printf( "\tCX = 0x%04X" , mRegisters[REG_CX].GetValue());
	printf( "\tDX = 0x%04X\n" , mRegisters[REG_DX].GetValue());
	printf( "\tSI = 0x%04X" , mRegisters[REG_SI].GetValue());
	printf( "\tDI = 0x%04X\n" , mRegisters[REG_DI].GetValue());
	printf( "\tBP = 0x%04X" , mRegisters[REG_BP].GetValue());
	printf( "\tSP = 0x%04X\n" , mRegisters[REG_SP].GetValue());

	printf( "\tCS = 0x%04X" , mRegisters[REG_CS].GetValue());
	printf( "\tDS = 0x%04X\n" , mRegisters[REG_DS].GetValue());
	printf( "\tSS = 0x%04X" , mRegisters[REG_SS].GetValue());
	printf( "\tES = 0x%04X\n" , mRegisters[REG_ES].GetValue());
	printf( "\tIP = 0x%04X" , mRegisters[REG_IP].GetValue());
	printf( "\tFLAGS = 0x%04X\n" , mRegisters[REG_FLAGS].GetValue());

	std::cout << std::endl << "Flags:" << std::endl;
	std::cout << "\tCF = " << GetFlag(FLAGS_CF);
	std::cout << "\tPF = " << GetFlag(FLAGS_PF) << std::endl;
	std::cout << "\tAF = " << GetFlag(FLAGS_AF);
	std::cout << "\tZF = " << GetFlag(FLAGS_ZF) << std::endl;
	std::cout << "\tSF = " << GetFlag(FLAGS_SF);
	std::cout << "\tTF = " << GetFlag(FLAGS_TF) << std::endl;
	std::cout << "\tIF = " << GetFlag(FLAGS_IF);
	std::cout << "\tDF = " << GetFlag(FLAGS_DF) << std::endl;
	std::cout << "\tOF = " << GetFlag(FLAGS_OF) << std::endl;

	std::cout << std::dec;
	std::cout << std::endl;


	std::cout << "++++++++++++++++++++++++  END DUMP  ++++++++++++++++++++++++" << std::endl; 

}

void Processor::MemDump() {
	std::cout << "++++++++++++++++++++++ BEGIN MEM DUMP ++++++++++++++++++++++" << std::endl;
	std::cout << std::hex;
	int lastSame = 0;
	for(unsigned int i = 0; i < (0x10000 / 0x10); i++) {
		bool same = true;
		if(i != (0x10000 / 0x10) - 1) {
			for(int j = 0; j < 0x10; j++) {
				if (mMem[i * 0x10 + j] != mMem[(i + 1) * 0x10 + j]) {
					same = false;
					break;
				}
			}
		}
		//skip if same, not first, not last
		if(same && (i != 0) && (i != (0x10000 / 0x10) - 1)) {
			if(lastSame == 0) {
				printf("0x%04X: ", i * 0x10);
				for(int j = 0; j < 0x10; j++) {
					printf("%02X ", mMem[i*0x10 + j]);
				}
				std::cout << std::endl;
				lastSame++;
			} else if(lastSame == 1) {
				std::cout << "..." << std::endl;
				lastSame++;
			}
		} else {
			lastSame = 0;
			printf("0x%04X: ", i * 0x10);
			for(int j = 0; j < 0x10; j++) {
				printf("%02X ", mMem[i*0x10 + j]);
			}
			std::cout << std::endl;
		}
	}
	std::cout << "++++++++++++++++++++++  END MEM DUMP  ++++++++++++++++++++++" << std::endl;
}

void Processor::DeviceDump() {
	for(unsigned int i = 0; i < mDevices.size(); i++) {
		mDevices[i]->Dump();
	}
}
