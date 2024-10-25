#include "CPU.h"

instruction::instruction() {
	//initalize to 0
	instr = 0x0;
	opcode = 0x0;
	func3 = 0x0;
	rs1 = 0x0;
	rs2 = 0x0;
	rd = 0x0;
	imm = 0x0;
}

void instruction::setinstruction(uint32_t fetch) {
	instr = fetch;
	opcode = instr & 0x7f;
	//depending on opcode, parse out rd, func3, rs1, rs2
	if (opcode == RTYPE) {
		rd = (instr >> 7) & 0x1F;
		func3 = (instr >> 12) & 0x7;
		rs1 = (instr >> 15) & 0x1F;
		rs2 = (instr >> 20) & 0x1F;
	}
	else if (opcode == ITYPE || opcode == LOAD) {
		rd = (instr >> 7) & 0x1F;
		func3 = (instr >> 12) & 0x7;
		rs1 = (instr >> 15) & 0x1F;
	}
	else if (opcode == STORE) {
		func3 = (instr >> 12) & 0x7;
		rs1 = (instr >> 15) & 0x1F;
		rs2 = (instr >> 20) & 0x1F;
	}
	else if (opcode == BRANCH) {
		func3 = (instr >> 12) & 0x7;
		rs1 = (instr >> 15) & 0x1F;
		rs2 = (instr >> 20) & 0x1F;
	}
	else if (opcode == UPPER) {
		rd = (instr >> 7) & 0x1F;
	}
	else if (opcode == JUMP) {
		rd = (instr >> 7) & 0x1F;
	}
	else {
		//if opcode is neither any of the types of instructions
		opcode = 0;
		func3 = 0; 
		rs1 = 0;
		rs2 = 0;
		rd = 0;
		imm = 0;
	}
}


CPU::CPU(uint8_t* instructionMemory)
{
	PC = 0; //set PC to 0
	for (int i = 0; i < 4096; i++) //copy instrMEM and initalize data memory
	{
		instMem[i] = instructionMemory[i];
		dmemory[i] = 0;
	}
	for (int i = 0; i < 32; i++) {
		//set all registers to 0
		registerFile[i] = 0;
	}
}

int32_t CPU::readregisterFile(int32_t a) {
	return registerFile[a];
}

unsigned long CPU::readPC()
{
	return PC;
}
void CPU::incPC(int a)
{
	PC += a;
}

int32_t CPU::readOpcode() {
	return currentInstruction.opcode;
}

void CPU::instructionMemory() {
	if (PC + 3 < 4096) {
		//check PC is not out of range
		uint32_t byte1 = instMem[PC];
		uint32_t byte2 = instMem[PC + 1];
		uint32_t byte3 = instMem[PC + 2];
		uint32_t byte4 = instMem[PC + 3];

		uint32_t concatenated = (byte4 << 24) + (byte3 << 16) + (byte2 << 8) + byte1;
		//generate all of instruction's components after concatenation
		currentInstruction.setinstruction(concatenated);
	}
	return;
}

void CPU::immGen() {
	int32_t instr = currentInstruction.instr;
	//every instruction's immediate is stored differently
	if (currentInstruction.opcode == ITYPE || currentInstruction.opcode == LOAD) {
		//int32_t already sign extends
		currentInstruction.imm = (instr >> 20);
	}
	else if (currentInstruction.opcode == STORE) {
		currentInstruction.imm = ((instr & 0xfe000000) + ((instr & 0xf80) << 13)) >> 20;
	}
	else if (currentInstruction.opcode == BRANCH) {
		int32_t imm12 = (instr >> 31) & 0x1;       // bit 12
		int32_t imm10_5 = (instr >> 25) & 0x3F;    // bits 10-5
		int32_t imm4_1 = (instr >> 8) & 0xF;        // bits 4-1
		int32_t imm11 = (instr >> 7) & 0x1;          // bit 11
		currentInstruction.imm = (imm12 << 11) | (imm10_5 << 4) | (imm4_1) | (imm11 << 10);
		if (currentInstruction.imm & 0x800) { // Check if bit 12 (sign bit) is set
			currentInstruction.imm |= 0xFFFFF000; // Sign-extend to 32 bits
		}
		//imm has not been shifted left 1 yet, will happen during nextPC function
	}
	else if (currentInstruction.opcode == UPPER) {
		currentInstruction.imm = (instr >> 12);
	}
	else if (currentInstruction.opcode == JUMP) {
		int32_t imm20 = (instr >> 31) & 0x1;          // bit 31
		int32_t imm10_1 = (instr >> 21) & 0x3FF;      // bits 30-21
		int32_t imm11 = (instr >> 20) & 0x1;           // bit 20
		int32_t imm19_12 = (instr >> 12) & 0xFF;       // bits 19-12
		currentInstruction.imm = (imm20 << 19) | (imm19_12 << 11) | (imm11 << 10) | imm10_1;
		if (currentInstruction.imm & 0x80000) { // Check if bit 20 (sign bit) is set
			currentInstruction.imm |= 0xFFF00000; // Extend sign to 32 bits
		}
	}
	else {
		currentInstruction.imm = 0;
	}
	
	return;
}

void CPU::registers() {
	readData1 = registerFile[currentInstruction.rs1];
	readData2 = registerFile[currentInstruction.rs2];
	
	return;
}

void CPU::controller() {
	//based off of controller table in report
	switch (currentInstruction.opcode) {
		case RTYPE:
			RegWrite = 1;
			ALUOp = 1;
			break;
		case ITYPE:
			RegWrite = 1;
			ALUSrc = 1;
			ALUOp = 2;
			break;
		case LOAD:
			RegWrite = 1;
			ALUSrc = 1;
			ALUOp = 3;
			MemtoReg = 1;
			MemRead = 1;
			break;
		case STORE:
			ALUSrc = 1;
			MemWrite = 1;
			ALUOp = 4;
			break;
		case BRANCH:
			ALUOp = 5;
			Branch = 1;
			break;
		case UPPER:
			RegWrite = 1;
			ALUOp = 6;
			LUI = 1;
			break;
		case JUMP:
			RegWrite = 1;
			ALUOp = 7;
			jal = 1;
			break;
	}
	return;
}

void CPU::aluControl() {
	//set flag if SB or LB
	if ((ALUOp == 3 && currentInstruction.func3 == 0) || (ALUOp == 4 && currentInstruction.func3 == 0)) { 
		WorB = 1; 
	}
	if (ALUOp == 3 || ALUOp == 4 || (ALUOp == 1 && currentInstruction.func3 == 0)) {
		//desired action: add
		inputForALU = 1;
	}
	else if (ALUOp == 5) {
		//desired action: subtract
		inputForALU = 2;
	}
	else if ((ALUOp == 2 && currentInstruction.func3 == 5) || ALUOp == 6) {
		//desired action: shift
		inputForALU = 3;
	}
	else if (ALUOp == 1 && currentInstruction.func3 == 4) {
		//desired action: xor
		inputForALU = 4;
	}
	else if (ALUOp == 2 && currentInstruction.func3 == 6) {
		//desired action: or
		inputForALU = 5;
	}
	else if (ALUOp == 7) {
		//desired action: get pc+4 for jal to store in rd
		inputForALU = 6;
	}

	return;
}

void CPU::alu() {
	if (inputForALU == 1) {
		//ALUSrc controls to read from rs2 or imm
		if (ALUSrc == 0) {
			ALUResult = readData1 + readData2;
		}
		else {
			ALUResult = readData1 + currentInstruction.imm;
		}
	}
	else if (inputForALU == 2) { 
		if ((readData1 - readData2) == 0) { zero = 1; }
		ALUResult = readData1 - readData2;
	}
	else if (inputForALU == 3) {
		if (LUI == 0) {
			ALUResult = readData1 >> currentInstruction.imm;
		}
		else {
			ALUResult = (currentInstruction.imm << 12) & 0xFFFFF000;
		}
	}
	else if (inputForALU == 4) {
		ALUResult = readData1 ^ readData2;
	}
	else if (inputForALU == 5) {
		ALUResult = readData1 | currentInstruction.imm;
	}
	else if (inputForALU == 6) {
		ALUResult = PC + 4;
	}
	
	return;
}

void CPU::dataMemory() {
	int32_t lByte1, lByte2, lByte3, lByte4;
	uint8_t sByte1, sByte2, sByte3, sByte4;

	//store instr
	if (MemWrite == 1) {
		//SW
		if (WorB == 0) {
			// Separate bytes of readData2
			sByte1 = (readData2 >> 24) & 0xff;
			sByte2 = (readData2 >> 16) & 0xff;
			sByte3 = (readData2 >> 8) & 0xff;
			sByte4 = readData2 & 0xff;

			// Store in dmemory in little endian form
			dmemory[ALUResult] = sByte4;
			dmemory[ALUResult + 1] = sByte3;
			dmemory[ALUResult + 2] = sByte2;
			dmemory[ALUResult + 3] = sByte1;
		}
		//SB
		else {
			sByte4 = readData2 & 0xff;
			dmemory[ALUResult] = sByte4;
		}
	}
	//load instruction
	else if (MemRead == 1) {
		//LW
		if (WorB == 0) {
			// Fetch 4 bytes from the data memory in little endian form.
			lByte1 = dmemory[ALUResult];
			lByte2 = dmemory[ALUResult + 1];
			lByte3 = dmemory[ALUResult + 2];
			lByte4 = dmemory[ALUResult + 3];

			// Convert and store as readData_dmemory
			readData_dmemory = (lByte4 << 24) + (lByte3 << 16) + (lByte2 << 8) + lByte1;
		}
		//LB
		else {
			lByte1 = dmemory[ALUResult];
			readData_dmemory = lByte1;
		}
	}
	return;
}

void CPU::writeBack() {
	//Load instruction
	if (MemtoReg == 1 && RegWrite == 1) {
		registerFile[currentInstruction.rd] = readData_dmemory;
	}
	//MemtoReg == 0 which means ALUResult is used instead of readData_dmemory
	else if (RegWrite == 1){
		registerFile[currentInstruction.rd] = ALUResult;
	}
	//x0 cannot be written to
	registerFile[0] = 0;
	return;
}

void CPU::nextPC() {
	//need to jump
	//shift left by 1 for alignment
	if ((zero & Branch) || jal) {
		incPC((currentInstruction.imm) << 1);
	}
	//PC + 4
	else {
		incPC(4);
	}
	
	return;
}

void CPU::resetControl() {
	//make sure previous instruction doesn't interfere with next instruction
	RegWrite = 0;
	ALUSrc = 0;
	MemWrite = 0;
	ALUOp = 0;
	MemtoReg = 0;
	MemRead = 0;
	Branch = 0;
	LUI = 0;
	jal = 0;
	WorB = 0; //word or byte signal
	inputForALU = 0;
	zero = 0;
	ALUResult = 0;
	readData_dmemory = 0;
}
// Add other functions here ... 