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

I wouldn't call this example *jit*, it simply uses dasm instead of Visual's builtin asm
(which won't work for x64, and you'd have to create separate asm files)

There are few caveats:
 * I've made precomputed CRC table 1D, to make access easiers from asm
 * I've dropped _end part_ from crc calculation, so buffer size must be multiple of 4
 * it has 'relative local labels', there can be up to 10 labels numbered from 0-9
   + when using them, you need to specify whether you're jumping backward (<) or forward (>)
   + if you use them, you need to call `dasm_setupglobal`, in my case it's in ctor of DynAsm object

 * ~~I'm not sure if it's bug or not, but `dasm_put` for both x86 and x64 uses `va_arg(*, int)`, which totally screws up pointers in x64 (and probably any 64-bit values for that matter).~~
   + ~~so in x64 version I had to pass pointer to _crc table_ as another argument~~
 * I'a probably never found it out by myself, there's `mov64` instruction, that does what I've needed, see here:
   + http://www.corsix.org/content/calling-external-functions-x64-dynasm

Example results from my laptop
```
jitdemo> bin\examp2_x86.exe
compiled for x86
jit returend value: 4281082894 time: 0.111525 spd: 896.658 Mb/s
c   returend value: 4281082894 time: 0.138894 spd: 719.976 Mb/s

jitdemo> bin\examp2_x64.exe
compiled for x64
jit returend value: 4281082894 time: 0.111745 spd: 894.891 Mb/s
c   returend value: 4281082894 time: 0.144186 spd: 693.548 Mb/s
```

Example 3.
----------

Is implementation of *counter machine* and is much more complicated.
You can find detailed description in [GRAM.md](https://github.com/gim913/dynasm-jit-playground/blob/master/GRAM.md)

Some cool numbers while testing fibonacci:
```
compiled for x64
value in cell number 0: 165580141
 * no-jit  time: 1.70075

value in cell number 0: 165580141
 *    jit  time: 0.399607
```

