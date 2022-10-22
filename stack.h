#ifndef TASBOT_EYES_STACK_H
#define TASBOT_EYES_STACK_H

#include <stdbool.h>

#define MAX_SIZE    128     //Maximum amount of items, stack can hold

extern void* stack[MAX_SIZE];
extern int top;

bool isEmpty();
void* peek();
void* pop();
bool push(void* _item);

#endif //TASBOT_EYES_STACK_H
