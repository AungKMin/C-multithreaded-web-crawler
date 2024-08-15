/*
A dynamic stack holding strings
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct stack
{
    // max capacity of the stack
    size_t size;
    // position of the last item pushed onto the stack
    size_t pos;
    // memory for stack
    char **items;
} STACK;

#define STACK_RESIZE_FACTOR 2

int init_stack(STACK *p, size_t stack_size);
bool is_full_stack(STACK *p);
bool is_empty_stack(STACK *p);
int push_stack(STACK *p, char *item);
int pop_stack(STACK *p, char **p_item);
int resize_stack(STACK *p);
size_t num_elements_stack(STACK *p);
int cleanup_stack(STACK *p);