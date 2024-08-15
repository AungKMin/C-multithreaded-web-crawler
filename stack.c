/*
A dynamic stack holding strings
- note: this current implementation only resizes stack to increase size, never to decrease size
*/

#include "stack.h"

/**
 * @brief initialize stack with an initial size (capacity)
 * @param p STACK*: a pointer to uninitialized memory
 * @param stack_size size_t: initial capacity of the stack to be initialized
 * @return 0 on success; 1 otherwise
 */
int init_stack(STACK *p, size_t stack_size)
{
    if (p == NULL || stack_size == 0)
    {
        return 1;
    }

    p->size = stack_size;
    p->pos = -1;
    p->items = (char **)malloc(stack_size * sizeof(char *));

    memset(p->items, 0, stack_size * sizeof(char *));

    return 0;
}

/**
 * @brief push an item onto the stack; if the stack is full, resize the stack
 * @param p STACK*: (pointer to) the stack the function will push item onto
 * @param item char*: string to push onto stack
 * @return 0 on success; 1 otherwise
 */
int push_stack(STACK *p, char *item)
{
    if (p == NULL)
    {
        return 1;
    }

    if (is_full_stack(p))
    {
        resize_stack(p);
    }

    ++(p->pos);
    // strlen(item) + 1 for end of string character
    p->items[p->pos] = malloc((strlen(item) + 1) * sizeof(char));
    memset(p->items[p->pos], 0, (strlen(item) + 1) * sizeof(char));
    strncpy(p->items[p->pos], item, strlen(item));

    return 0;
}

/**
 * @brief pop from the stack
 * @param p STACK*: (pointer to) the stack the function will pop from
 * @param p_item char**: pointer that will be populated with popped element
 * @return 0 on success; 1 otherwise
 * @note the caller is responsible for deallocating memory assigned to p_item
 */
int pop_stack(STACK *p, char **p_item)
{
    if ((p == NULL) || is_empty_stack(p))
    {
        return 1;
    }

    *p_item = malloc(sizeof(char) * (strlen(p->items[p->pos]) + 1));
    memset(*p_item, 0, sizeof(char) * (strlen(p->items[p->pos]) + 1));
    strncpy(*p_item, p->items[p->pos], strlen(p->items[p->pos]));
    free(p->items[p->pos]);
    p->items[p->pos] = NULL;
    (p->pos)--;
    return 0;
}

/**
 * @brief check if the stack is full
 * @param p STACK*: (pointer to) the stack to check
 * @return true if full; false otherwise
 */
bool is_full_stack(STACK *p)
{
    if (p == NULL)
    {
        return 0;
    }
    return (p->pos == (p->size - 1));
}

/**
 * @brief check if the stack is empty
 * @param p STACK*: (pointer to) the stack to check
 * @return true if empty; false otherwise
 */
bool is_empty_stack(STACK *p)
{
    if (p == NULL)
    {
        return 0;
    }
    return (p->pos == -1);
}

/**
 * @brief resize stack to have greater capacity; maintain existing elements
 * @param p STACK*: (pointer to) the stack to resize
 * @return 0 on success; 1 otherwise
 */
int resize_stack(STACK *p)
{
    size_t old_size = p->size;
    char **old_items = p->items;
    p->size = (p->size) * STACK_RESIZE_FACTOR;
    p->items = (char **)malloc((p->size) * sizeof(char *));
    if (p->items == NULL)
    {
        return 1;
    }

    for (size_t i = 0; i < old_size; ++i)
    {
        p->items[i] = old_items[i];
    }
    for (size_t i = old_size; i < p->size; ++i)
    {
        p->items[i] = NULL;
    }

    free(old_items);
    old_items = NULL;

    return 0;
}

/**
 * @brief returns number of elements currently in the stack
 * @param p STACK*: (pointer to) the stack
 * @return number of elements in the stack
 */
size_t num_elements_stack(STACK *p)
{
    return p->pos + 1;
}

/**
 * @brief deconstruct stack: free all allocated memory
 * @param p STACK*: (pointer to) the stack to deconstruct
 * @return 0 on success; 1 otherwise
 */
int cleanup_stack(STACK *p)
{
    if (p == NULL || p->items == NULL)
    {
        return 0;
    }
    for (size_t i = 0; i < p->size; ++i)
    {
        if (p->items[i] != NULL)
        {
            free(p->items[i]);
            p->items[i] = NULL;
        }
    }
    free(p->items);
    p->items = NULL;
    return 0;
}