#ifndef VM_H
#define VM_H

#include "value.h"
#include "object.h"
#include "compiler.h"
#include "hashtable.h"

#include <stdlib.h>

#define FRAME_SZ 1000                 // Max stack depth
#define STACK_SZ FRAME_SZ * UINT8_MAX // We have at most UINT8_MAX local var per stack

typedef enum {
	VM_EVAL_SUCCSESS,
	VM_SYNTAX_ERR,
	VM_COMPILE_ERR,
	VM_RUNTIME_ERR,
	VM_GENERIC_ERR
} EvalResult;

typedef struct Frame {
	uint8_t *ip;       // Instruction pointer
	Value *stack;      // Base of stack for current frame
	ObjFunction *fn;   // The function associated with the frame
} Frame;

typedef struct VM {
	// Current VM compiler
	Compiler *currCompiler;

	// VM program stack
	Value *stack, *sp;

	Frame *frames;
	int frameCount;

	// Globals and constant strings pool
	HashTable globals;
	HashTable strings;

	// Memory management
	Obj *objects;

	bool disableGC;
	size_t allocated;
	size_t nextGC;

	Obj **reachedStack;
	size_t reachedCapacity, reachedCount;
} VM;

void initVM(VM *vm);
void freeVM(VM *vm);

EvalResult evaluate(VM *vm, const char *src);

#define push(vm, v)  (*(vm)->sp++ = (v))
#define pop(vm)      (*(--(vm)->sp))
#define peek(vm)     ((vm)->sp[-1])
#define peek2(vm)    ((vm)->sp[-2])
#define peekn(vm, n) ((vm)->sp[-(n + 1)])

#endif
