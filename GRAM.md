counter machine JIT
===================

The machine implemented here is actually variant of so called **counter machine**. You can read a lot abot them in following wikipedia articles:
 * http://en.wikipedia.org/wiki/Counter_machine
 * http://en.wikipedia.org/wiki/Counter_machine_reference_model
 * http://en.wikipedia.org/wiki/Random-access_machine

Implementation has following features:
 * program with numbered, consecutive instructions
 * memory cells from 0 to _n_, 0th cell is programs result
 * 4 instructions:
	+ **Z**ero(n) - put zero to _n_-th cell
	+ **S**ucc(n) - increment value in _n_-th cell
	+ **T**ransfer(m,n) - copy value from _m_-th cell to _n_-th cell
	+ **I**f(m,n, x) - if contents of _m_-th and _n_-th cells is equal *jump* to instruction number x

As you can see **T** instruction isn't actually needed, and it could be implemented using *Z*, *S* and *I*.

You can read in mentioned articles, what needs to be done, to make this machine Turing complete.

Before we begin some cool numbers, to grab your attention:
```
compiled for x64
parsing input...............................................................  DONE
value in cell number 0: 165580141
program exited...
 * no-jit  time: 5.72286

parsing input...............................................................  DONE
value in cell number 0: 165580141
program exited...
 *    jit  time: 0.399948
```

So you can see that, although JIT is quite dummy, speed up is over 10 times. 

Interpreter
-----------

First the parser converts file in text form to more convenient *byte-code*. The parser also
recognizes following variant of *If* instruction `I(a,a,x)` which basically is unconditional jump.
So bytecode opcodes are as follow:

```c
// gram_bc.h
enum {
        Op_Zero
        , Op_Increment
        , Op_Transfer
        , Op_Conditional_Jump
        , Op_Jump
};
```

The parser also finds what is the index of 'biggest' cell addressed (`maxMemAccess`).

The interpreter is very simple, you can see some early version of it in this commit:
 * https://github.com/gim913/dynasm-jit-playground/blob/5ed7984bef57772851f149fde74cbe05148ab4ee/src/gRamMachine.cpp#L206

As you can see, `printq` trace instructions, takes more space than instruction handling itself.

I guess you could fit **both** parser and interpreter in about page of python code.

I've changed interpreter later, to use following struct, instead of array of `uint32_t`s:
```c
struct Instr {
	uint32_t opcode;
	uint32_t op[3]; // operands
	uint16_t statsCounter, statsIdx;
};
```

You can see it here:
 * https://github.com/gim913/dynasm-jit-playground/blob/e57d6db06d7564a32078990e731700dcacb2fe21/src/gRamMachine.cpp#L201

Additional fields `statsCounter` and `statsIdx` are used to collect some data during execution of instructions. Particularly `statsCounter` is used to count how many times given instruction has been executed.

`statsIdx` was supposed to be used for nicer tracing (and later during generation), but right now it's not used.

Now the only flow affecting instructions are *Op_Conditional_Jump* and *Op_Jump* instructions.
So, when executing *Jmp* instruction I check if it looks like *hot loop*, in current implementation
it means jump **destination** has been executed 4 times. (4 was choosen *ad-hoc*).

There is also additional check that jump itself was executed less or same number of times than *destination*.

If conditions are ok, I'm trying to generate a code between (*destination* and *jump*)

Which also means there is **WRONG** assumption, that interesting jump goes *back*.
(Maybe I'll fix it later, maybe I won't)

Code generation
---------------

### prelude ###

Let's look at [master](https://github.com/gim913/dynasm-jit-playground/blob/master/src/gRamMachine.cpp#L267), the generation happens in following call:

```c
dynasmGenerator(&da, instructions, gpc, jmpEip, machineMem, maxMemAccess)
```

**da** is small DynAsm wrapper, **gpc** is *PC of destination* and **jmpEip** is *PC of jump*.
(so *gpc* is start, *jmpEip* is end)

Before we look at generator itself, let's see what happens if generation succeeded, instruction at *gpc* is modified in following way:
 * original opcode is replaced with new opcode `Op_Generated`
 * original operands 0,1 are overwritten with a pointer to generated function
 * original operand 2 is overwritten with *length* in instructions, ot generated block

Now in [interpeter part](https://github.com/gim913/dynasm-jit-playground/blob/master/src/gRamMachine.cpp#L289) if we stumble upon `Op_Generated` opcode, I do exactly what's needed:
 * get the pointer and execute generated code
 * change current instruction pointer accordingly

### generator ###

Now we can head to generator itself which is in [gram_x64.dasc](https://github.com/gim913/dynasm-jit-playground/blob/master/src/gram_x64.dasc).

Generator works in two passes:
 * first pass:
	+ checks if all jumps within currently generated block points to the block itself
		if not, I won't be generating code for such block
	+ since all jumps are **direct**, I collect all the jumps *destinations*
 * second pass - code generation itself

Between first and second pass, I set identifiers, for all possible *destinations*, they will be used in dasm's so called *PC labels*. You can read more about different type of labels here: [Jumping with DynAsm - by Corsix](http://www.corsix.org/content/jumping-dynasm) and here: [lua-l mailing list, Josh Haberman](http://lua-users.org/lists/lua-l/2011-04/msg01025.html)


Then goes generation, which is pretty straightforward. `rdi` register is used to keep pointer to *machine's memory*

Since all instructions use *direct* addressing, I calculate proper cell index during generation, and will use that value in generated code, here's example:

```
case Op_Transfer:
	op1 = GETOP(1, locGpc);
	op2 = GETOP(2, locGpc);
	| mov eax, op1
	| mov rcx, aword [rdi + rax*8]
	| mov eax, op2
	| mov aword [rdi + rax*8], rcx
```

So `op1, op2` will be calculated during generation, and will be put into proper place with dasm_put directives later, so for example if during generation `op1==3 && op2==5` following **NATIVE code** will be generated later:

```asm
000000300A040038 B8 03 00 00 00       mov         eax,3  
000000300A04003D 48 8B 0C C7          mov         rcx,qword ptr [rdi+rax*8]  
000000300A040041 B8 05 00 00 00       mov         eax,5  
000000300A040046 48 89 0C C7          mov         qword ptr [rdi+rax*8],rcx  
```


Last thing is treatment of jumps, but first consider following piece of code, for which generator will be called (with start==9, end==12):
```
 9: +-> I(2, 6, 13) -+
10: |   S(5)         |
11: |   S(6)         |
12: +-- I(1, 1, 9)   |
13: ...            <-+
```

As you can see using my leet ascii skillz I've drawn how the jumps goes,
inside the generator there are checks like:
```
if (dest == end+1) {
	| jmp >3

} else {
	| je => labels[dest]
}
```

Reasoning is as follows:
 1. I know that jump at *end* goes **UP** (I've written about it at the end of previous section)
 2. I'm NOT generating code for pieces, that have jumps outside of (start, end+1)
 3. so, if there's jump to `end+1` somewhere from within the code it means it's "end-of-loop"

So in case of jump to "end-of-loop", I simply generate jmp to *end of generated function*,
and otherwise, I generate *PC jump*, to one of the labels, collected during **pass 1**.

There's difference in handling of `Op_Generated`, which loows like this:

```
| mov rax, aword [rsi + locGpc*sizeof(Instr) + 4]
| call rax

if (dest == end+1) {
	| jmp >3

} else {
	// ...
}
locGpc = dest;
```

So basically if after call there is some more code, we do not need to jump to it, as it will
be placed directly after *native* call instruction, e.g:

```
; this is code generated for Op_Generated
000000656E1B004A 48 8B 86 B8 00 00 00 mov         rax,qword ptr [rsi+0B8h]  
000000656E1B0051 FF D0                call        rax
; and right after *native* call, there's a code generated for:
; Op_transfer, op[0]==(3), op[1]==(2)
000000656E1B0053 B8 03 00 00 00       mov         eax,3  
000000656E1B0058 48 8B 0C C7          mov         rcx,qword ptr [rdi+rax*8]  
000000656E1B005C B8 02 00 00 00       mov         eax,2  
000000656E1B0061 48 89 0C C7          mov         qword ptr [rdi+rax*8],rcx  
```

### epiogue ###

If you'd like to see how interpreter and generator works, inside interpreter there is global
`verbose` variable, and inside generator, you'd have to change, following define:
```
printgen(xxx) do { if(0) std::cerr << xxx; } while(0)
```
(and use `if(1)` instead of `if(0)`)

Also if you'd like to debug generated code, "`| int3`" is your friend.

I think that would be all for now.
