#include <stdint.h>
#include <stdio.h>
#include "stackmagic.h"

#define STACK_SAFETY_MARGIN (4096)

static struct poutine_stackmagic stackmagic_main = {
	.stack = NULL,
};

static enum {
	STACK_DIRECTION_UNKNOWN,
	STACK_DIRECTION_UP,
	STACK_DIRECTION_DOWN,
} stack_direction = STACK_DIRECTION_UNKNOWN;

static void detect_stack_direction_inner_impl (void *);
static void (* const volatile detect_stack_direction_inner)(void *) =
	detect_stack_direction_inner_impl;
static void detect_stack_direction_inner_impl (void *previous)
{
	int dummy;

	uintptr_t prev = (uintptr_t) previous;
	uintptr_t next = (uintptr_t) &dummy;

	if (prev != next)
		stack_direction = next < prev ? STACK_DIRECTION_DOWN : STACK_DIRECTION_UP;
	else // wtf
		abort ();
}

static void detect_stack_direction ()
{
	if (stack_direction == STACK_DIRECTION_UNKNOWN)
	{
		int dummy;
		detect_stack_direction_inner (&dummy);
	}
}

static void relocate_and_run_impl (struct poutine_stackmagic *, poutine_function, void *, jmp_buf, size_t);
static void (* const volatile relocate_and_run)(struct poutine_stackmagic *, poutine_function, void *, jmp_buf, size_t) =
	relocate_and_run_impl;
static void relocate_and_run_impl (struct poutine_stackmagic *sm, poutine_function function, void *arg,
	jmp_buf env, size_t relocsize)
{
	int8_t reloc[relocsize];

	static volatile void *dummy;
	dummy = reloc;
	(void) dummy;

	if (!setjmp (sm->env))
		longjmp (env, 1);

	function (arg);

	// should not exit
	abort ();
}

void poutine_stackmagic_initialize (struct poutine_stackmagic *sm,
	poutine_function function, void *arg, size_t stacksize)
{
	void *stack = malloc (stacksize);
	sm->stack = stack;

	detect_stack_direction ();

	uintptr_t source = (uintptr_t) &stack;
	uintptr_t target = (uintptr_t) stack;

	size_t relocsize;
	if (stack_direction == STACK_DIRECTION_DOWN)
		relocsize = source - (target + stacksize) + STACK_SAFETY_MARGIN;
	else
		relocsize = target - source + STACK_SAFETY_MARGIN;

	jmp_buf tmp;
	if (!setjmp (tmp))
		relocate_and_run (sm, function, arg, tmp, relocsize);
}

void poutine_stackmagic_switch (struct poutine_stackmagic *next, struct poutine_stackmagic *prev)
{
	if (!prev)
		prev = &stackmagic_main;
	if (!next)
		next = &stackmagic_main;
	if (!setjmp (prev->env))
		longjmp (next->env, 1);
}

void poutine_stackmagic_finalize (struct poutine_stackmagic *sm)
{
	free(sm->stack);
}
