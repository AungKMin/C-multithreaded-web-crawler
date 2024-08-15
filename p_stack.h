#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct p_stack_struct
{
    // max capacity of the stack
    size_t size;
    // position of the last item pushed onto the stack
    size_t pos;
    // memory for stack
    void **items;
} PSTACK;

#define PSTACK_RESIZE_FACTOR 2

int init_pstack(PSTACK *p, size_t PSTACK_size);
bool is_full_pstack(PSTACK *p);
bool is_empty_pstack(PSTACK *p);
int push_pstack(PSTACK *p, void *item);
int pop_pstack(PSTACK *p, void **p_item);
int resize_pstack(PSTACK *p);
size_t num_elements_pstack(PSTACK *p);
int cleanup_pstack(PSTACK *p);