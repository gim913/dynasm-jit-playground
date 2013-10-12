#include "examp1_x86.h"

#include <iostream>

int main(int argc, char *argv[]) {
	if (argc <= 1) {
		std::cout << "syntax: examp1 <integer>" << std::endl;
		return -1;
	}
	int num = atoi(argv[1]);

	DynAsm state_buf;
	DynAsm *state = &state_buf;
	try {
		jitInit(state, actions);

		dynasmGenerator(state, num);
  
		int (*fptr)() = reinterpret_cast<int(*)()>( jitCode(state) );

		// Call the JIT-ted function.
		int ret = fptr();
	
		std::cout << "code returend value: " << ret << std::endl;
		free_jitcode(fptr);

	} catch(std::exception& e) {
		std::cout << "exception occured" << std::endl;
		std::cout << e.what() << std::endl;
	}

	return 0;
}
