/*

Student/Assignment Information

CS 429 Lab 4
pdp8.c

Name: Matthew Diamond
UTEID: med2425
Unique #: 53870

*/

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

/*Includes and defined data types*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef short Boolean;
#define TRUE 1
#define FALSE 0

typedef char* STRING;

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

/*Input file and possible arguments*/

FILE* input; /*input object file*/

Boolean v_option = FALSE; /*verbose mode*/

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

/*Storage Containers for various PDP8 hardware emulations*/

short a_reg; //accumulator
short mD_reg; //memory data register
short mA_reg; //memory address register
short PC; //program counter
short link; //link bit
short mem[4096]; //memory for the emulation
STRING initialPC; //store the EP as hex

short prevPC; //temporary storage for the PC value for verbose mode

long long int time = 0; //number of cycles

short inst; //temporary storage for the data fetched from memory
Boolean halt = FALSE; //continue running while false

typedef enum { //possible opcodes
	AND, TAD, ISZ, DCA, JMS, JMP, IOT, OPER
}
opCodes;

char currentOp[100]; //ops to print for verbose mode

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void parseLine(char* line) {
	STRING memLocation;
	STRING memData;
	char* endPtr;
	int decimal;
	int memLocationDecimal;
	int memDataDecimal;

	if(line[0] == 'E' && line[1] == 'P') { //check for the entry point
		if(PC != -1) {
			fprintf(stderr, "%s\n", "Multiple EPs found. Exiting.");
			exit(0);
		}
		strtok(line, ": ");
		initialPC = strtok(NULL, " ");
		decimal = strtol(initialPC, &endPtr, 16);
		PC = decimal;
	}
	else { //otherwise, read in and store data in specified location
		memLocation = strtok(line, ":");
		memData = strtok(NULL, "\n");
		memLocationDecimal = strtol(memLocation, &endPtr, 16);
		memDataDecimal = strtol(memData, &endPtr, 16);
		mem[memLocationDecimal] = memDataDecimal;
	}
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void parseObj() {
	char line[10]; //temporary storage for a line from the object file

	if(getc(input) == EOF) {
		fprintf(stderr, "%s\n", "Input file is empty. Exiting.");
		exit(0);
	}
	rewind(input);

	while((fgets(line, 10, input)) != NULL) {
		parseLine(line);
	}
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void initializeStorage(){ //set all storage containers to 0 to start
	short i = 0;
	a_reg = 0;
	mD_reg = 0;
	mA_reg = 0;
	PC = -1;
	link = 0;
	for(i = 0; i < 4096; i ++) {
		mem[i] = 0;
	}
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void fetch() {
	if(PC > 4095) {
		PC = PC - 4096;
	}
	inst = mem[PC];
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

int getEffectiveAddress() {
	short page = PC/128;
	int temp = 0;
	int zc = inst & 0x80;
	int id = inst & 0x100;
	if(zc) {
		temp = page * 128;
	}
	temp += inst & 0x7F;
	if(id) {
		temp = mem[temp];
		strcat(currentOp, "I ");
		time ++;
	}
	return temp;
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void group1() {
	Boolean CLA = (inst & 0x80);
	Boolean CLL = (inst & 0x40);
	Boolean CMA = (inst & 0x20);
	Boolean CML = (inst & 0x10);
	Boolean RAR = (inst & 0x8);
	Boolean RAL = (inst & 0x4);
	Boolean rotate1or2 = (inst & 0x2);
	Boolean IAC = (inst & 0x1);
	short numRotates = 1;
	short tempForRotate;
	if(RAR && RAL) {
		fprintf(stderr, "%s\n", "Group 1 RAR and RAL bits set. Fatal error. Exiting.");
		exit(0);
	}
	if(CLA) {
		a_reg = 0;
		strcat(currentOp, "CLA ");
	}
	if(CLL) {
		link = 0;
		strcat(currentOp, "CLL ");
	}
	if(CMA) {
		a_reg = (~a_reg) & 0xFFF;
		strcat(currentOp, "CMA ");
	}
	if(CML) {
		link = (~link) & 0x1;
		strcat(currentOp, "CML ");
	}
	if(IAC) {
		a_reg += 1;
		if(a_reg & 0x1000) {
			link = (~link) & 0x1;
			a_reg = a_reg & 0xFFF;
		}
		strcat(currentOp, "IAC ");
	}
	if(RAR) {
		if(rotate1or2) {
			numRotates = 2;
			strcat(currentOp, "RTR ");
		}
		else {
			strcat(currentOp, "RAR ");
		}
		for(; numRotates > 0; numRotates --) {
			tempForRotate = a_reg & 0x1;
			a_reg = a_reg >> 1;
			a_reg = a_reg | (link << 11);
			link = tempForRotate;
		}
	}
	else if(RAL) {
		if(rotate1or2) {
			numRotates = 2;
			strcat(currentOp, "RTL ");
		}
		else{
			strcat(currentOp, "RAL ");
		}
		for(; numRotates > 0; numRotates --) {
			tempForRotate = a_reg & 0x800;
			a_reg = a_reg << 1;
			a_reg += link;
			link = tempForRotate >> 11;
		}
	}
}

void group2() {
	Boolean CLA = (inst & 0x80);
	Boolean SMA = (inst & 0x40);
	Boolean SZA = (inst & 0x20);
	Boolean SNL = (inst & 0x10);
	Boolean RSS = (inst & 0x8);
	Boolean HLT = (inst & 0x2);
	Boolean error = (inst & 0x1);
	Boolean skipFlag = FALSE;
	if(error) {
		fprintf(stderr, "%s\n", "Group 2 zero bit set. Fatal error. Exiting.");
		exit(0);
	}
	if(SMA) {
		strcat(currentOp, "SMA ");
		if(a_reg & 0x800) {
			skipFlag = TRUE;
		}
	}
	if(SZA) {
		strcat(currentOp, "SZA ");
		if(a_reg == 0) {
			skipFlag = TRUE;
		}
	}
	if(SNL) {
		strcat(currentOp, "SNL ");
		if(link != 0) {
			skipFlag = TRUE;
		}
	}
	if(RSS) {
		if(skipFlag == TRUE) {
			skipFlag = FALSE;
		}
		else {
			skipFlag = TRUE;
		}
		strcat(currentOp, "RSS ");
	}
	if(CLA) {
		a_reg = 0;
		strcat(currentOp, "CLA ");
	}
	if(HLT) {
		strcat(currentOp, "HLT ");
		halt = TRUE;
	}
	if(skipFlag) {
		PC ++;
		PC = PC & 0xFFF;
	}

}

void operate() {
	Boolean group = (inst & 0x100) >> 8; //bit 9
	if(group == FALSE) {
		group1();
	}
	else {
		group2();
	}
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void decodeAndExecute() {
	opCodes opCode = inst >> 9; //get opcode
	int deviceNum;
	int effectiveAddress;
	switch(opCode) {
		case AND: //0
			strcat(currentOp, "AND ");
			time += 2;
			effectiveAddress = getEffectiveAddress();
			a_reg = mem[effectiveAddress] & a_reg;
			PC ++;
			PC = PC & 0xFFF;
			break;
		case TAD: //1
			strcat(currentOp, "TAD ");
			time += 2;
			effectiveAddress = getEffectiveAddress();
			a_reg += mem[effectiveAddress];
			if(a_reg & 0x1000) {
				link = (~link) & 0x1;
				a_reg = a_reg & 0xFFF;
			}
			PC ++;
			PC = PC & 0xFFF;
			break;
		case ISZ: //2
			strcat(currentOp, "ISZ ");
			time += 2;
			effectiveAddress = getEffectiveAddress();
			mem[effectiveAddress] += 1;
			mem[effectiveAddress] = mem[effectiveAddress] & 0xFFF;
			if(mem[effectiveAddress] == 0) {
				PC ++;
			}
			PC ++;
			PC = PC & 0xFFF;
			break;
		case DCA: //3
			strcat(currentOp, "DCA ");
			time += 2;
			effectiveAddress = getEffectiveAddress();
			mem[effectiveAddress] = a_reg;
			a_reg = 0;
			PC ++;
			PC = PC & 0xFFF;
			break;
		case JMS: //4
			strcat(currentOp, "JMS ");
			time += 2;
			effectiveAddress = getEffectiveAddress();
			mem[effectiveAddress] = PC + 1;
			PC = effectiveAddress + 1;
			break;
		case JMP: //5
			strcat(currentOp, "JMP ");
			time += 1;
			effectiveAddress = getEffectiveAddress();
			PC = effectiveAddress;
			break;
		case IOT: //6
			strcat(currentOp, "IOT ");
			deviceNum = ((inst & 0x1F8) >> 3);
			if(deviceNum == 3) {
				a_reg = getchar();
				strcat(currentOp, "3 ");
			}
			else if(deviceNum == 4) {
				putchar(a_reg & 0xFF);
				strcat(currentOp, "4 ");
			}
			else {
				halt = TRUE;
			}
			PC ++;
			PC = PC & 0xFFF;
			time += 1;
			break;
		case OPER: //7
			operate();
			time += 1;
			PC ++;
			PC = PC & 0xFFF;
			break;
	}

}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void printVerbose() {
	currentOp[strlen(currentOp) - 1] = '\0';
	fprintf(stderr, "Time %lld: PC=0x%03X instruction = 0x%03X (%s), rA = 0x%03X, rL = %d\n",
        time, prevPC, inst , currentOp, a_reg, link);
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void DoIt() {
	initializeStorage(); //initialize all storage containers for the emulation
	parseObj(); //parse input file for memory locations and associated data
	if(PC == -1) {
		fprintf(stderr, "%s\n", "No entry point found. Exiting.");
		exit(0);
	}
	while(halt != TRUE) {
		fetch(); //fetch the next instruction
		prevPC = PC;
		decodeAndExecute(); //decode the next instruction
		a_reg = a_reg & 0xFFF;
		if(v_option == TRUE) {
			printVerbose();
		}
		currentOp[0] = '\0';
	}
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

void scanargs(char *s){
	/* check each character of the option list for its meaning. */
	while (*s != '\0') {
		switch (*s++){
		case 'v': /*verbose option*/
			v_option = TRUE;
			break;
		}
	}
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */

int main(int argc, char** argv) {
	// printf("%s", "running\n");
	//catch possible known error cases
	if(argc == 1) { //no input file
		fprintf(stderr, "%s\n", "Too few command line arguments. Exiting.");
		exit(1);
	}
	if(argc > 3) { //more arguments than expected
		fprintf(stderr, "%s\n", "Too many command line arguments. Exiting.");
		exit(1);
	}

	//any case that is not automatically an error case
	while(argc > 1) { //acceptable number of arguments
		argc --;
		argv ++;
		if (**argv == '-'){
  			scanargs(*argv);
		}
	}
	input = fopen(*argv,"r");
	if (input == NULL){
		fprintf (stderr, "Can't open %s. Exiting.\n",*argv);
	}
	else { //run
		// printf("%s", "running\n");
		DoIt();
	}
	exit(0);
}

/* ***************************************************************** */
/*                                                                   */
/*                                                                   */
/* ***************************************************************** */
