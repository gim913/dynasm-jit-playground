#pragma once

// I'm gonna define those so later it'll be possible to nicely wrap
// dynasm in class
#define Dst       state
#define Dst_DECL  DynAsm *Dst
#define Dst_REF   (state->m_state)
// commented as dasm_put and Co. will reside in dynasm-helper.cpp now
//#define DASM_FDEF static

struct dasm_State;

class DynAsm {
	DynAsm(const DynAsm&);
	DynAsm& operator=(const DynAsm&);
public:
	DynAsm(const void *actionlist, size_t globCount = 0);
	~DynAsm();

	// allocate executable memory, build code
	void *build();

	// destroy code
	bool destroy(void *code);

	int getPc(size_t index);
	void growPc(size_t nVal);

	struct dasm_State *m_state;
	size_t m_globCount;
	void* m_glob;
};

#include "dasm_proto.h"

