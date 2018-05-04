#include "memory.h"
#include "chunk.h"
#include "hashtable.h"
#include "compiler.h"
#include "vm.h"

#include <stdint.h>
#include <string.h>

#define REACHED_DEFAULT_SZ 16
#define REACHED_GROW_RATE   2
#define HEAP_GROW_RATE      2

#ifdef DBG_PRINT_GC
#include <stdio.h>
#include <string.h>
#endif

static Obj *newObj(VM *vm, size_t size, ObjType type) {
	Obj *o = ALLOC(vm, size);
	o->type = type;
	o->reached = false;
	o->next = vm->objects;
	vm->objects = o;
	return o;
}

static void garbageCollect(VM *vm);

void *allocate(VM *vm, void *ptr, size_t oldsize, size_t size) {
	vm->allocated += size - oldsize;
	if(size > oldsize && !vm->disableGC) {
#ifdef DBG_STRESS_GC
		garbageCollect(vm);
#endif

		if(vm->allocated > vm->nextGC) {
			garbageCollect(vm);
		}
	}

	return realloc(ptr, size);
}

static uint32_t hashString(const char *str, size_t length);

ObjString *newString(VM *vm, char *cstring, size_t length) {
	ObjString *str = (ObjString*) newObj(vm, sizeof(*str), OBJ_STRING);
	str->length = length;
	str->data = cstring;
	str->hash = hashString(cstring, length);
	return str;
}

ObjFunction *newFunction(VM *vm, ObjModule *module, uint8_t argsCount) {
	ObjFunction *f = (ObjFunction*) newObj(vm, sizeof(*f), OBJ_FUNCTION);
	f->argsCount = argsCount;
	f->module = module;
	f->name = NULL;
	initChunk(&f->chunk);
	return f;
}

ObjNative *newNative(VM *vm, uint8_t argsCount, Native fn) {
	ObjNative *n = (ObjNative*) newObj(vm, sizeof(*n), OBJ_NATIVE);
	n->argsCount = argsCount;
	n->name = NULL;
	n->fn = fn;
	return n;
}

ObjClass *newClass(VM *vm, ObjString *name, ObjClass *superCls) {
	ObjClass *cls = (ObjClass*) newObj(vm, sizeof(*cls), OBJ_CLASS);
	cls->name = name;
	cls->superCls = superCls;
	initHashTable(&cls->methods);
	return cls;
}
ObjInstance *newInstance(VM *vm, ObjClass *cls) {
	ObjInstance *inst = (ObjInstance*) newObj(vm, sizeof(*inst), OBJ_INST);
	inst->cls = cls;
	initHashTable(&inst->fields);
	return inst;
}

ObjModule *newModule(VM *vm, ObjString *name) {
	ObjModule *module = (ObjModule*) newObj(vm, sizeof(*module), OBJ_MODULE);
	module->name = name;
	initHashTable(&module->globals);
	return module;
}

ObjBoundMethod *newBoundMethod(VM *vm, ObjInstance *b, ObjFunction *method) {
	ObjBoundMethod *bound = (ObjBoundMethod*) newObj(vm, sizeof(*bound), OBJ_BOUND_METHOD);
	bound->bound = b;
	bound->method = method;
	return bound;
}

ObjString *copyString(VM *vm, const char *str, size_t length) {
	ObjString *interned = HashTableGetString(&vm->strings, str, length, hashString(str, length));
	if(interned == NULL) {
		char *data = ALLOC(vm, length + 1);
		memcpy(data, str, length);
		data[length] = '\0';

		interned = newString(vm, data, length);
		hashTablePut(&vm->strings, interned, NULL_VAL);
	}
	return interned;
}

ObjString *newStringFromBuf(VM *vm, char *buf, size_t length) {
	ObjString *interned = HashTableGetString(&vm->strings, buf, length, hashString(buf, length));
	if(interned == NULL) {
		interned = newString(vm, buf, length);
		hashTablePut(&vm->strings, interned, NULL_VAL);
		return interned;
	}

	FREEARRAY(vm, char, buf, length + 1);
	return interned;
}

static void freeObject(VM *vm, Obj *o) {
	switch(o->type) {
	case OBJ_STRING: {
		ObjString *s = (ObjString*) o;
		FREEARRAY(vm, char, s->data, s->length + 1);
		FREE(vm, ObjString, s);
		break;
	}
	case OBJ_NATIVE: {
		ObjNative *n = (ObjNative*) o;
		FREE(vm, ObjNative, n);
		break;
	}
	case OBJ_FUNCTION: {
		ObjFunction *f = (ObjFunction*) o;
		freeChunk(&f->chunk);
		FREE(vm, ObjFunction, f);
		break;
	}
	case OBJ_CLASS: {
		ObjClass *cls = (ObjClass*) o;
		freeHashTable(&cls->methods);
		FREE(vm, ObjClass, cls);
		break;
	}
	case OBJ_INST: {
		ObjInstance *i = (ObjInstance*) o;
		freeHashTable(&i->fields);
		FREE(vm, ObjInstance, i);
		break;
	}
	case OBJ_MODULE: {
		ObjModule *m = (ObjModule*) o;
		freeHashTable(&m->globals);
		FREE(vm, ObjModule, m);
		break;
	}
	case OBJ_BOUND_METHOD: {
		ObjBoundMethod *b = (ObjBoundMethod*) o;
		FREE(vm, ObjBoundMethod, b);
		break;
	}
	}
}

void freeObjects(VM *vm) {
	Obj **head = &vm->objects;
	while(*head != NULL) {
		if(!(*head)->reached)  {
			Obj *u = *head;
			*head = u->next;

#ifdef DBG_PRINT_GC
			printf("FREE: unreached object %p type: %s\n", (void*)u, typeName[u->type]);
#endif

			freeObject(vm, u);
		} else {
			(*head)->reached = false;
			head = &(*head)->next;
		}
	}
}

void disableGC(VM *vm , bool disable) {
	vm->disableGC = disable;
}

static void growReached(VM *vm) {
	vm->reachedCapacity *= REACHED_GROW_RATE;
	vm->reachedStack = realloc(vm->reachedStack, vm->reachedCapacity);
}

static void addReachedObject(VM *vm, Obj *o) {
	if(vm->reachedCount + 1 > vm->reachedCapacity) {
		growReached(vm);
	}
	vm->reachedStack[vm->reachedCount++] = o;
}

void reachObject(VM *vm, Obj *o) {
	if(o == NULL || o->reached) return;

#ifdef DBG_PRINT_GC
	printf("REACHED: Object %p type: %s repr: ", (void*)o, typeName[o->type]);
	printObj(o);
	printf("\n");
#endif

	o->reached = true;
	addReachedObject(vm, o);
}

void reachValue(VM *vm, Value v) {
	if(IS_OBJ(v)) reachObject(vm, AS_OBJ(v));
}

static void reachValueArray(VM *vm, ValueArray *a) {
	for(size_t i = 0; i < a->count; i++) {
		reachValue(vm, a->arr[i]);
	}
}

static void reachHashTable(VM *vm, HashTable *t) {
	for(size_t i = 0; i < t->size; i++) {
		Entry *buckHead = t->entries[i];
		while(buckHead != NULL) {
			reachObject(vm, (Obj*) buckHead->key);
			reachValue(vm, buckHead->value);
			buckHead = buckHead->next;
		}
	}
}

static void recursevelyReach(VM *vm, Obj *o) {
#ifdef DBG_PRINT_GC
	printf("Recursevely exploring object %p...\n", (void*)o);
#endif

	switch(o->type) {
	case OBJ_NATIVE:
		reachObject(vm, (Obj*)((ObjNative*)o)->name);
		break;
	case OBJ_FUNCTION: {
		ObjFunction *func = (ObjFunction*) o;
		reachObject(vm, (Obj*) func->name);
		reachObject(vm, (Obj*) func->module);
		reachValueArray(vm, &func->chunk.consts);
		break;
	}
	case OBJ_CLASS: {
		ObjClass *cls = (ObjClass*) o;
		reachObject(vm, (Obj*) cls->name);
		reachObject(vm, (Obj*) cls->superCls);
		reachHashTable(vm, &cls->methods);
		break;
	}
	case OBJ_INST: {
		ObjInstance *i = (ObjInstance*) o;
		reachObject(vm, (Obj*) i->cls);
		reachHashTable(vm, &i->fields);
		break;
	}
	case OBJ_MODULE: {
		ObjModule *m = (ObjModule*) o;
		reachObject(vm, (Obj*) m->name);
		reachHashTable(vm, &m->globals);
		break;
	}
	case OBJ_BOUND_METHOD: {
		ObjBoundMethod *b = (ObjBoundMethod*) o;
		reachObject(vm, (Obj*) b->bound);
		reachObject(vm, (Obj*) b->method);
		break;
	}
	case OBJ_STRING: break;
	}
}

static void garbageCollect(VM *vm) {
#ifdef DBG_PRINT_GC
	size_t prevAlloc = vm->allocated;
	puts("*--- Starting GC ---*");
#endif

	//init reached object stack
	vm->reachedStack = malloc(sizeof(Obj*) * REACHED_DEFAULT_SZ);
	vm->reachedCapacity = REACHED_DEFAULT_SZ;

	reachObject(vm, (Obj*) vm->ctor);
	//reach vm global vars
	reachHashTable(vm, &vm->modules);
	//reach elemnts on the stack
	for(Value *v = vm->stack; v < vm->sp; v++) {
		reachValue(vm, *v);
	}
	//reach elements on the frame stack
	for(int i = 0; i < vm->frameCount; i++) {
		reachObject(vm, (Obj*) vm->frames[i].fn);
	}

	//reach the compiler objects
	reachCompilerRoots(vm, vm->currCompiler);

	//recursevely reach objs held by other reached objs
	while(vm->reachedCount != 0) {
		recursevelyReach(vm, vm->reachedStack[--vm->reachedCount]);
	}

	//remove unused strings
	removeUnreachedStrings(&vm->strings);

	//free the garbage
	freeObjects(vm);

	//free the reached objects stack
	free(vm->reachedStack);
	vm->reachedStack = NULL;
	vm->reachedCapacity = 0;
	vm->reachedCount = 0;

	vm->nextGC = vm->allocated * HEAP_GROW_RATE;

#ifdef DBG_PRINT_GC
	size_t curr = prevAlloc - vm->allocated;
	printf("Completed GC, prev allocated: %lu, curr allocated "
	       "%lu, freed: %lu bytes of memory, next GC: %lu.\n",
		   prevAlloc, vm->allocated, curr, vm->nextGC);
	puts("*--- End  of  GC ---*\n");
#endif
}

static uint32_t hashString(const char *str, size_t length) {
	uint32_t hash = 5381;

	for(size_t i = 0; i < length; i++) {
		hash = ((hash << 5) + hash) + str[i];
	}

	return hash;
}