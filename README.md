dynasm jit playground
=====================

Playing around with dynasm 

Example 1.
----------
examp1 is really basic, just to show how .dasc files look like

Example 2.
----------

LIttle more complicated, it is implementation of CRC using Intel's
Slicing-by-4 technique.

There are 3 implementations, one directly in C, and two others in dasm: x86 and x64 version.

There are few caveats:
 * I've made precomputed CRC table 1D, to make access easiers from asm
 * I've dropped _end part_ from crc calculation, so buffer size must be multiple of 4
 * it has 'relative local labels', there can be up to 10 labels numbered from 0-9
   + when using them, you need to specify whether you're jumping backward (<) or forward (>)
   + if you use them, you need to call `dasm_setupglobal`, in my case it's in ctor of DynAsm object

 * I'm not sure if it's bug or not, but `dasm_put` for both x86 and x64 uses `va_arg(*, int)`, which totally screws up pointers in x64 (and probably any 64-bit values for that matter).
   + so in x64 version I had to pass pointer to _crc table_ as another argument

