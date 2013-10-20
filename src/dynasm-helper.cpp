#include "dynasm-helper.h"
#include "dasm_x86.h"

#include <memory>
#include <stdexcept>
#include <windows.h>

DynAsm::DynAsm(const void *actionlist, size_t globCount /* = 0 */)
		: m_globCount(globCount)
		, m_actionList(actionlist)
{
	dasm_init(this, 1);
	
}

DynAsm::~DynAsm() {
	dasm_free(this);
}


void DynAsm::prepare()
{
	m_glob = 0;
	dasm_setupglobal(this, &m_glob, m_globCount); 
	dasm_setup(this, m_actionList);
	growPc(1);
}

void *DynAsm::build() {
	size_t size;
	int dasm_status = dasm_link(this, &size);
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

	dasm_encode(this, ret);

	//FlushInstructionCache(GetCurrentProcess(), ret, size);

	DWORD oldProt=0;
	BOOL st = VirtualProtect(mem, size, PAGE_EXECUTE_READ, &oldProt);
	if (st != TRUE) {
		VirtualFree(mem, 0, MEM_RELEASE);
		throw std::runtime_error("error while changing memory protection");
		return 0;
	}

	return ret;
}

bool DynAsm::destroy(void *code) {
	void *mem = (char*)code - sizeof(size_t);
	VirtualFree(mem, 0, MEM_RELEASE);
	return true;
}

int DynAsm::getPc(size_t index) {
	return dasm_getpclabel(this, static_cast<unsigned int>(index));
}

void DynAsm::growPc(size_t newMaxPc) {
	dasm_growpc(this, static_cast<unsigned int>(newMaxPc));
}

bool Timer::init() {
#if !defined(LINUX)
	ULONGLONG u64freq;
	if(! QueryPerformanceFrequency((LARGE_INTEGER*)&u64freq))
		return false;
	freq = 1.0 / u64freq;
	return true;
#else
	return false;
#endif
}
Timer::TickType Timer::tick() {
#if !defined(LINUX)
	DWORD_PTR prev = SetThreadAffinityMask(GetCurrentThread(), 0);
	ULONGLONG v;
	QueryPerformanceCounter((LARGE_INTEGER*)&v);
	SetThreadAffinityMask(GetCurrentThread(), prev);
	return v*freq;
#else
	return 0;
#endif;
}
Timer::TickType Timer::freq = 0;

