#include <memory>
#include <stdexcept>

// I'm gonna define those so later it'll be possible to nicely wrap
// dynasm in class
#define Dst       state
#define Dst_DECL  DynAsm *Dst
#define Dst_REF   (state->D)
#define DASM_FDEF static

struct dasm_State;

class DynAsm {
public:
	struct dasm_State *D;
};

#include "dasm_proto.h"
#include "dasm_x86.h"

#include <windows.h>

void jitInit(Dst_DECL, const void *actionlist) {
	dasm_init(Dst, 1);
	dasm_setup(Dst, actionlist);
}

void *jitCode(Dst_DECL) {
	size_t size;
	int dasm_status = dasm_link(Dst, &size);
	if (dasm_status != DASM_S_OK) {
		throw std::runtime_error("link failed");
		return 0;
	}

	void* mem = VirtualAlloc(0, size+sizeof(size_t), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (mem == 0) {
		throw std::bad_alloc();
		return 0;
	}

	*(size_t*)mem = size;
	void *ret = (char*)mem + sizeof(size_t);

	dasm_encode(Dst, ret);
	dasm_free(Dst);

	DWORD oldProt=0;
	BOOL st = VirtualProtect(mem, size, PAGE_EXECUTE_READ, &oldProt);
	if (st != TRUE) {
		VirtualFree(mem, 0, MEM_RELEASE);
		throw std::runtime_error("error while changing memory protection");
		return 0;
	}

	return ret;
}

bool free_jitcode(void *code) {
	void *mem = (char*)code - sizeof(size_t);
	return (0 != VirtualFree(mem, 0, MEM_RELEASE));
}

#include JIT

