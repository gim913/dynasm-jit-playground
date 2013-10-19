#include "dynasm-helper.h"

#include "gram_x64.h"

#include "gram_bc.h"

#include <iostream>
#include <iomanip>
#include <list>

#include <stdint.h>
#include <stdio.h>

#define DELN(x) do {\
	if (x[strlen(x)-1] == '\n') x[strlen(x)-1]='\0'; \
	if (x[strlen(x)-1] == '\r') x[strlen(x)-1]='\0'; \
} while(0)


#define fatal(xxx) do { \
	std::cerr << "\n\nG_RAM MACHINE 2.0:\n\tERROR: " << std::endl; \
	std::cerr << xxx << std::endl; \
	exit (0); \
} while(0)

size_t n; // number of instructions
size_t maxMemAccess;
uint32_t lineNo;
uint64_t* machineMem;


int gatoi(char **buf, unsigned long *a) {
	char *x;
	unsigned long t;
	t=strtoul(*buf, &x, 10);
	if (x == *buf) return (1);
	*a=t;
	*buf = x;
	return (0);
}

// instruction type, op1, op2, op3, stats
//space for bytecode
Instr *ginstructions;

int parseInstruction(Instr* instructions, char *s) {
	char *k;
	char comm[10] = "zZsStTjJ";
	int pars[4] = { 1, 1, 2, 3 }, i;

#define SKIPBLANKS \
	do {\
		while (strchr("\t ", *k)) { \
			if (!*k) {\
				fatal("unexpected EOL " << lineNo << ":" << (k-s)); \
			} else { \
					k++; \
			} \
		} \
	} while(0)


#define set_instruction(instType) do {\
	instructions[n].opcode = instType; \
	} while (0)


#define set_operand(idx,operand) do {\
	instructions[n].op[idx-1] = operand; \
	} while (0)

#define push_instruction() n++

	lineNo++;

	if (!s)
		return 0;

	k=strchr(s, ':');
	k=k?k+1:s;
	if (!k || !strlen(k))
		return 0;

	SKIPBLANKS;
	if (!*k)
		return 0;

	if (*k == 'i' || *k=='I') *k='j';
	if (!strchr(comm, *k)) {
		fatal("while parsing line " << lineNo << (k-s) << " sign " << (*k));
	}

	int curInstr = (int)((strchr(comm, *k)-comm)/2);
	set_instruction(curInstr);
	k++;
	if (!*k) {
		fatal("unexpected EOL " << lineNo << ":" << (k-s));
	}
	SKIPBLANKS;
	if (*k != '(') {
		fatal("'(' expected  " << lineNo << ":" << (k-s) << "but '" << (*k) << "' found");
	}
	k++;
	if (!*k) {
		fatal("unexpected EOL " << lineNo << ":" << (k-s));
	}

	for (i=0; i<pars[curInstr]; i++)
	{
		unsigned long operVal;
		if (gatoi(&k, &operVal)) {
			fatal("while parsing line " << lineNo << ":" << (k-s) << " sign" << (*k) << ",  bad or missing argument");
		}
		set_operand(i+1, operVal);
		maxMemAccess = std::max<size_t>((size_t)operVal, maxMemAccess);

		SKIPBLANKS;
		if (i != pars[curInstr]-1) {
			if (*k != ',') {
				fatal("',' expected  " << lineNo << ":" << (k-s) << "but '" << (*k) << "' found");

			} else {
				k++;
			}
		}
	}

	SKIPBLANKS;
	if (*k != ')') {
		fatal("')' expected  " << lineNo << ":" << (k-s) << "but '" << (*k) << "' found");
	}

	// fix unconditional jump:
	// CJ x x y  ->  J y
	if (curInstr == Op_Conditional_Jump && GETOP(1,n) == GETOP(2, n))
	{
		std::cout << "patching jump" << std::endl;
		set_instruction(Op_Jump);
		set_operand(1, GETOP(3, n));
		set_operand(2, 0);
		set_operand(3, 0);
	}

	k++;
	std::cerr << ".."; 
	push_instruction();
	return 0;
}

void parseInput(char * fileName) {
	FILE* fp;
	char buf[0x400];

	if ((fp = fopen(fileName, "r")) == NULL) {
		fatal("Cannot open input file: %s\n", fileName);
	}

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		n++;
	}

	fseek(fp, 0L, SEEK_SET);

	ginstructions = new Instr[n * sizeof(Instr)]();

	std::cerr << "parsing input...";
	n=0;
	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
		DELN(buf);
		parseInstruction(ginstructions, buf);
	}
	std::cerr << "  DONE\n\n";

	fclose(fp);

	// here 'n' has REAL instruction count
}

void dumpInstructions(Instr* instructions) {

#define mnem(x) ("ZSTJg"[x])

	for (size_t i = 0; i < n; ++i) {
		std::cerr << std::setw(3) << i << ": ";
		std::cerr << std::setw(3) << mnem(instructions[i].opcode) << " ";
		std::cerr << std::setw(5) << instructions[i].op[0] << " ";
		std::cerr << std::setw(5) << instructions[i].op[1] << " ";
		std::cerr << std::setw(5) << instructions[i].op[2] << "  | ";
		std::cerr << std::setw(5) << instructions[i].statsCounter << " " << instructions[i].statsIdx;
		std::cerr << std::endl;
	}
}

int verbose = 0;

#define printq(xxx) do { if(verbose) std::cerr << xxx; } while(0)

#define MAXRUN (1000*1000*10)

void executeInstructions(Instr* instructions, size_t n, uint64_t* machineMem, size_t maxMemAccess) {
	DynAsm da(actions, GLOB__MAX);

	size_t gpc=0; // program counter
	unsigned long lc=0;
	while (1)
	{
		//if (GETOP(1, gpc) > maxMemAccess)
		//	fatal("");
		//if ((GETCD(gpc) == Op_Transfer || GETCD(gpc) == Op_Conditional_Jump) && (GETOP(2, gpc) > maxMemAccess))
		//	fatal("");

		printq(std::setw(9) << lc << " ");
		switch (GETCD(gpc))
		{
		case Op_Zero:
			printq("EIP("<< gpc <<") Z("<< GETOP(1, gpc) <<")\n");
			machineMem[GETOP(1, gpc)] = 0;
			INCSTATS(gpc);
			gpc++; break;
		case Op_Increment:
			machineMem[GETOP(1, gpc)]++;
			printq("EIP("<< gpc <<") S("<< GETOP(1, gpc) <<") = " << machineMem[GETOP(1, gpc)]<<"\n");
			INCSTATS(gpc);
			gpc++; break;
		case Op_Transfer:
			machineMem[GETOP(2, gpc)]=machineMem[GETOP(1, gpc)];
			printq("EIP("<< gpc <<") T("<< GETOP(1, gpc) <<") -> (" << GETOP(2, gpc) << ") = " << machineMem[GETOP(2, gpc)]<<"\n");
			INCSTATS(gpc);
			gpc++; break;
		case Op_Jump:
			printq("EIP("<< gpc <<") goto ("<<GETOP(3, gpc)<<"\n");
			INCSTATS(gpc);
			gpc = GETOP(1, gpc);
			break;
		case Op_Conditional_Jump:
			printq("EIP("<< gpc <<") (");
			printq(GETOP(1, gpc) <<")"<<machineMem[GETOP(1, gpc)]<<" == (");
			printq(GETOP(2, gpc) <<")"<<machineMem[GETOP(2, gpc)]<<" then goto ");
			printq(GETOP(3, gpc)<<"\n");
			INCSTATS(gpc);
			if (machineMem[GETOP(2, gpc)] == machineMem[GETOP(1, gpc)]) {
				size_t jmpEip = gpc;
				gpc = GETOP(3, gpc);
				// it's sooooo hot here...
				if (GETOP(4, gpc) == 4) {
					// if we were at 'jump destination' more times than at jump itself
					// (so hopefully there's a path in code from GPC -> JMPEIP)
					if (GETOP(4, jmpEip) <= GETOP(4, gpc)) {
						printq("let's generate hoties!" << gpc << "c("<<GETOP(4, gpc)<< ") to " << jmpEip << " c("<<GETOP(4, jmpEip)<<")\n");
						dynasmGenerator(&da, instructions, gpc, jmpEip, machineMem, maxMemAccess);


						
					} else {
						// ignore for now?
					}
				}
			} else {
				gpc++;
			}
			break;
		}
		if (gpc>=n)
			break;
		lc++;
		if (lc == MAXRUN)
			break;
	}
	printq("\n");

	if (lc == MAXRUN) {
		std::cerr << "program exceeded maximum number of instruction executions\n";

	} else {
		std::cerr << "value in cell number 0: " << machineMem[0] << "\n";
		std::cerr << "program exited...\n";
	}
}


int main(int argc, char *argv[])
{
	std::ios_base::sync_with_stdio(false);
	std::cout << "compiled for x64" << std::endl;

	if (argc < 2) {
		fatal("syntax: gram <input file>");
	}

	parseInput(argv[1]);
	dumpInstructions(ginstructions);

	maxMemAccess += 1;
	machineMem = new uint64_t[maxMemAccess]();

	machineMem[1] = 3;

	try {
		executeInstructions(ginstructions, n, machineMem, maxMemAccess);

	} catch(std::exception& e) {
		std::cout << "exception occurred" << std::endl;
		std::cout << e.what() << std::endl;
	}
	dumpInstructions(ginstructions);

	return 0;
}
