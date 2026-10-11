#include "stack.h"
#include "types.h"

void stackPush(Stack *stack, u16 value)
{
    stack->arr[stack->index] = value;
    stack->index++;
}

u16 stackPop(Stack *stack)
{
    stack->index--;
    return stack->arr[stack->index];
}