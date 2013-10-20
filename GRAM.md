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
	+ **I**f*(m,n, x) - if contents of _m_-th and _n_-th cells is equal *jump* to instruction number x

As you can see **T** instruction isn't actually needed, and it could be implemented using *Z*, *S* and *I*.

You can read in mentioned articles, what needs to be done, to make this machine Turing complete.

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
 * 5ed7984bef57772851f149fde74cbe05148ab4ee/src/gRamMachine.cpp#L206

As you can see, `printq` trace instructions, takes more space than instruction handling itself.

I've changed interpreter later, to use following struct, instead of array of `uint32_t`s:

```c
struct Instr {
	uint32_t opcode;
	uint32_t op[3]; // operands
	uint16_t statsCounter, statsIdx;
};
```
You can see it here:
 * e57d6db06d7564a32078990e731700dcacb2fe21/src/gRamMachine.cpp#L201

