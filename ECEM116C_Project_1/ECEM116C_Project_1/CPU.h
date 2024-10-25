#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstdint>
using namespace std;

#define RTYPE 0x33
#define ITYPE 0x13
#define LOAD 0x3
#define STORE 0x23
#define BRANCH 0x63
#define UPPER 0x37
#define JUMP 0x6F

//breaks up the instruction into all its separate parts
class instruction { // optional
public:
 	int32_t instr;//instruction
 	instruction(); // constructor
	void setinstruction(uint32_t fetch);
	int32_t opcode;
	int32_t func3;
	int32_t rs1;
	int32_t rs2;
	int32_t rd;
	int32_t imm;
};

class CPU {
private:
	//byte addressable data memory
	int8_t dmemory[4096];
	unsigned long PC; //pc 
	int32_t registerFile[32];
	uint32_t instMem[4096];
	instruction currentInstruction;

public:
	CPU(uint8_t* instructionMemory);
	unsigned long readPC();
	void incPC(int a);
	int32_t readregisterFile(int32_t a);
	int32_t readOpcode();

	int32_t readData1 = 0;
	int32_t readData2 = 0;

	//control signals
	int32_t RegWrite = 0;
	int32_t ALUSrc = 0;
	int32_t MemWrite = 0;
	int32_t ALUOp = 0;
	int32_t MemtoReg = 0;
	int32_t MemRead = 0;
	int32_t Branch = 0;
	int32_t LUI = 0;
	int32_t jal = 0;
	int32_t WorB = 0; //word or byte signal
	int32_t inputForALU = 0;
	int32_t zero = 0;
	int32_t ALUResult = 0;
	int32_t readData_dmemory = 0;

	void instructionMemory();
	void immGen();
	void registers();
	void controller();
	void aluControl();
	void alu();
	void dataMemory();
	void writeBack();
	void nextPC();

	void resetControl();
};

// add other functions and objects here
