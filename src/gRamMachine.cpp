#include "dynasm-helper.h"

#include "gram_x64.h"

#include "gram_bc.h"

#include <algorithm>
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

// instruction type, op1, op2, op3, stats
//space for bytecode
Instr *ginstructions;
size_t instructionsCount; // number of instructions
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
	instructions[instructionsCount].opcode = instType; \
	} while (0)


#define set_operand(idx,operand) do {\
	instructions[instructionsCount].op[idx-1] = operand; \
	} while (0)

#define push_instruction() instructionsCount++

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
	// CJ x x y  ->  J 0 0 y
	if (curInstr == Op_Conditional_Jump && GETOP(1,instructionsCount) == GETOP(2, instructionsCount))
	{
		set_instruction(Op_Jump);
		set_operand(1, 0);
		set_operand(2, 0);
		// leave jump destination untouched
		//set_operand(3, );
	}

	k++;
	std::cerr << "..";
	push_instruction();
	return 0;
}

void parseInput(const char * fileName) {
	FILE* fp;
	char buf[0x400];

	if ((fp = fopen(fileName, "r")) == NULL) {
		fatal("Cannot open input file: " << fileName);
	}

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		instructionsCount++;
	}

	fseek(fp, 0L, SEEK_SET);

	ginstructions = new Instr[instructionsCount * sizeof(Instr)]();

	std::cerr << "parsing input...";
	instructionsCount=0;
	while (fgets(buf, sizeof(buf), fp) != NULL)
	{
		DELN(buf);
		parseInstruction(ginstructions, buf);
	}
	std::cerr << "  DONE\n";

	fclose(fp);

	// here 'n' has REAL instruction count
}

void dumpInstructions(Instr* instructions, int start /* = 0 */, int end /* = 0*/) {
	// zero, successor(inc), transfer(move), conditional jump, unconditional jump, native
#define mnem(x) ("ZSTJgn"[x])

	size_t i = start;
	while (i < (end?end:instructionsCount)) {
		std::cerr << std::setw(3) << i << ": ";
		std::cerr << std::setw(3) << mnem(instructions[i].opcode) << " ";
		std::cerr << std::setw(5) << instructions[i].op[0] << " ";
		std::cerr << std::setw(5) << instructions[i].op[1] << " ";
		std::cerr << std::setw(5) << instructions[i].op[2] << "  | ";
		std::cerr << std::setw(5) << instructions[i].statsCounter << " " << instructions[i].statsIdx;
		std::cerr << std::endl;

		if (instructions[i].opcode == Op_Generated) {
			i = instructions[i].op[2];
		} else {
			i++;
		}
	}
}

static const int verbose = 0;

#define printq(xxx) do { if(verbose) std::cerr << xxx; } while(0)

#define MAXRUN (1000*1000*1000)

template<bool Use_Jit>
void executeInstructions(Instr* instructions, size_t n, uint64_t* machineMem, size_t maxMemAccess) {
	DynAsm da(actions, GLOB__MAX);
	std::list< void* > generatedFunctions;

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
			STATSCNT(gpc)++;
			STATSIDX(gpc, lc);
			gpc++; break;
		case Op_Increment:
			machineMem[GETOP(1, gpc)]++;
			printq("EIP("<< gpc <<") S("<< GETOP(1, gpc) <<") = " << machineMem[GETOP(1, gpc)]<<"\n");
			STATSCNT(gpc)++;
			STATSIDX(gpc, lc);
			gpc++; break;
		case Op_Transfer:
			machineMem[GETOP(2, gpc)]=machineMem[GETOP(1, gpc)];
			printq("EIP("<< gpc <<") T("<< GETOP(1, gpc) <<") -> (" << GETOP(2, gpc) << ") = " << machineMem[GETOP(2, gpc)]<<"\n");
			STATSCNT(gpc)++;
			STATSIDX(gpc, lc);
			gpc++; break;
		case Op_Jump:
		case Op_Conditional_Jump:
			if (GETCD(gpc) == Op_Jump) {
				printq("EIP("<< gpc <<") goto ("<<GETOP(3, gpc)<<"\n");
			} else {
				printq("EIP("<< gpc <<") (");
				printq(GETOP(1, gpc) <<")"<<machineMem[GETOP(1, gpc)]<<" == (");
				printq(GETOP(2, gpc) <<")"<<machineMem[GETOP(2, gpc)]<<" then goto ");
				printq(GETOP(3, gpc)<<"\n");
			}
			STATSCNT(gpc)++;
			STATSIDX(gpc, lc);
			if (GETCD(gpc) == Op_Jump || machineMem[GETOP(2, gpc)] == machineMem[GETOP(1, gpc)]) {
				size_t jmpEip = gpc;
				gpc = GETOP(3, gpc);

				// it's sooooo hot here...
				if (Use_Jit && STATSCNT(gpc) == 4) {
					// if we were at 'jump destination' more times than at jump itself
					// (so hopefully there's a path in code from GPC -> JMPEIP)
					if (STATSCNT(jmpEip) <= STATSCNT(gpc)) {
						printq("let's generate hoties! " << gpc << " c("<<STATSCNT(gpc)<< ") to " << jmpEip << " c("<<STATSCNT(jmpEip)<<")\n");
						da.prepare();

						size_t lesser = std::min<size_t>(gpc, jmpEip);
						size_t bigger = std::max<size_t>(gpc, jmpEip);
						if ( dynasmGenerator(&da, instructions, instructionsCount, lesser, bigger, machineMem, maxMemAccess) ) {
							void* fptr = da.build();
							generatedFunctions.push_back( fptr );

							if (jmpEip > gpc) {
								GETCD(gpc) = Op_Generated;
								// now this one is ugly and note to good girls: you shouldn't do it,
								// but I'm too lazy, to make union :p
								*((size_t*)instructions[gpc].op) = (size_t)fptr;

								// my operands are 32b only, so let's cast this
								GETOP(3, gpc) = (uint32_t)(jmpEip+1);

							} else {
								// I could merge if+else block, but keep in mind there's a subtle difference:
								// in both cases, NEXT executed (by interpreter!) instruction will be at 'gpc', but
								// in 'if', I've just put Op_Generated, there and that's what'll be executed
								// in 'else' block it's put at 'jmpEip', but you actually don't know, if we'll ever reach that point
								//     (so there's a chance, that the code we have just generated won't be executed even a single time)
								GETCD(jmpEip) = Op_Generated;
								*((size_t*)instructions[jmpEip].op) = (size_t)fptr;
								GETOP(3, jmpEip) = (uint32_t)(gpc+1);
							}
						}

					} else {
						// ignore for now?
					}
				}

			} else {
				gpc++;
			}
			break;
		case Op_Generated:
			size_t (*fptr)() = *((size_t (**)())instructions[gpc].op);

			printq("EIP("<< gpc <<") (");
			printq(std::hex << fptr<<") then goto ");
			printq(std::dec << GETOP(3, gpc)<<"\n");

			size_t endOfExec = fptr();
			if (endOfExec >= n) {
				gpc = endOfExec;

			} else {
				gpc = GETOP(3, gpc);
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

	auto it = generatedFunctions.begin();
	auto _it = generatedFunctions.end();
	for(; it != _it; ++it) {
		da.destroy((*it));
	}
}

template<bool Use_Jit>
void performTest(const char* mesg, size_t argc, const char **argv)
{
	const char* inputFile = argv[1];
	parseInput(inputFile);

	//dumpInstructions(ginstructions);

	maxMemAccess += 1;
	machineMem = new uint64_t[maxMemAccess]();

	for (size_t curArg=2; curArg < argc; ++curArg)
	{
		char* x = 0;
		unsigned long v=strtoul(argv[curArg], &x, 10);
		if (x == argv[curArg]) {
			std::cout << "argument: " << argv[curArg] << " has not been recognized as a number\n";
			return;
		}

		machineMem[curArg - 1] = v;
	}

	try {

		Timer::init();

		auto t1 = Timer::tick();
		executeInstructions<Use_Jit>(ginstructions, instructionsCount, machineMem, maxMemAccess);
		auto t2 = Timer::tick();
		std::cout << mesg << " time: " << (t2-t1) << "\n\n";

	} catch(std::exception& e) {
		std::cout << "exception occurred" << std::endl;
		std::cout << e.what() << std::endl;
	}

	delete[] machineMem;
	//dumpInstructions(ginstructions);

}

int main(int argc, const char *argv[])
{
	std::ios_base::sync_with_stdio(false);
	std::cout << "compiled for x64" << std::endl;

	if (argc < 2) {
		fatal("syntax: gram <input file>");
	}

	performTest<false>(" * no-jit ", argc, argv);

	performTest<true >(" *    jit ", argc, argv);

	return 0;
}
