#include "dynasm-helper.h"

#ifdef _WIN64
#include "examp2_x64.h"
#elif _WIN32
#include "examp2_x86.h"
#endif

#include "timer.h"

#include <iostream>
#include <stdint.h>

uint32_t crcTab[4*256];

void crcInit() {
	for (unsigned int i = 0; i <= 0xFF; i++) {
		uint32_t crc = i;
		for (unsigned int j = 0; j < 8; j++) {
			crc = (crc >> 1) ^ ((crc & 1) * 0x04C11DB7);
		}
		crcTab[0 + i] = crc;
	}

	for (unsigned int i = 0; i <= 0xFF; i++) {
	  crcTab[1*256 + i] = (crcTab[0*256 + i] >> 8) ^ crcTab[0 + (crcTab[0*256 + i] & 0xFF)];
	  crcTab[2*256 + i] = (crcTab[1*256 + i] >> 8) ^ crcTab[0 + (crcTab[1*256 + i] & 0xFF)];
	  crcTab[3*256 + i] = (crcTab[2*256 + i] >> 8) ^ crcTab[0 + (crcTab[2*256 + i] & 0xFF)];
	}
}

uint32_t crcSlicing4(const void* data, size_t length, uint32_t previousCrc = 0)
{
	uint32_t* current = (uint32_t*) data;
	uint32_t crc = ~previousCrc;

	while (length >= 4)
	{
		crc ^= *current++;
		crc  =
			crcTab[3*256 + (crc & 0xFF)] ^
			crcTab[2*256 + ((crc>>8) & 0xFF)] ^
			crcTab[1*256 + ((crc>>16) & 0xFF)] ^
			crcTab[0*256 + (crc>>24)];
		length -= 4;
	}
	return ~crc;
}

int main(int argc, char *argv[]) {
	crcInit();

#ifdef _WIN64
	std::cout << "compiled for x64" << std::endl;
#elif _WIN32
	std::cout << "compiled for x86" << std::endl;
#endif

	const size_t dataSize = 100*1024*1024;
	void* dataBlock=malloc(dataSize);
	char* temp=(char*)dataBlock;
	uint64_t x = 0x416947;
	for (size_t i=0; i<0x100000; ++i) {
		x = (x*0x7FFFFFED + 0x7FFFFFC3) % (0x7fffffff);
		*temp++ = x&0xff;
	}

	try {
		DynAsm da(actions, GLOB__MAX);

		dynasmGenerator(&da);
  
#ifdef _WIN64
		uint32_t (__fastcall * fptr)(void*,size_t,void*) = reinterpret_cast<uint32_t(__fastcall *)(void*, size_t,void*)>( da.build() );
#elif _WIN32
		uint32_t (__fastcall * fptr)(void*,size_t) = reinterpret_cast<uint32_t(__fastcall *)(void*, size_t)>( da.build() );
#endif

		Timer::init();

		auto t1 = Timer::tick();
		uint32_t real = crcSlicing4(dataBlock, dataSize);
		auto t2 = Timer::tick();
#ifdef _WIN64
		uint32_t ret = fptr(dataBlock, dataSize, crcTab);
#elif _WIN32
		uint32_t ret = fptr(dataBlock, dataSize);
#endif
		auto t3 = Timer::tick();
	
		t3 -= t2;
		t2 -= t1;
		std::cout << "jit returend value: " << ret << " time: " << t3 << " spd: " << (dataSize/t3 / 1024 / 1024) << " Mb/s" << std::endl;
		std::cout << "c   returend value: " << real << " time: " << t2 << " spd: " << (dataSize/t2 / 1024 / 1024) << " Mb/s" << std::endl;
		std::cout << "last pc val: " << da.getPc(0) << std::endl;
		da.destroy(fptr);

	} catch(std::exception& e) {
		std::cout << "exception occured" << std::endl;
		std::cout << e.what() << std::endl;
	}

	return 0;
}
