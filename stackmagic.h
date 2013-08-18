#include <stdlib.h>
#include <setjmp.h>

typedef void (*poutine_function)(void *);

struct poutine_stackmagic
{
	jmp_buf env;
	void *stack;
};

void poutine_stackmagic_initialize (struct poutine_stackmagic *sm,
	poutine_function function, void *arg, size_t stacksize);
void poutine_stackmagic_switch (struct poutine_stackmagic *next, struct poutine_stackmagic *prev);
void poutine_stackmagic_finalize (struct poutine_stackmagic *sm);
