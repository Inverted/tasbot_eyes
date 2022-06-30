#ifndef TASBOT_EYES_ANIMATION_STACK_H
#define TASBOT_EYES_ANIMATION_STACK_H

#include <stdbool.h>

#include "utils.h"

/*
 * THIS IS WORK IN PROGRESS AND NOT YET USED
 */

bool isEmpty();
bool isFull();
string peek();
string pop();
bool push(string _animPath);

#endif //TASBOT_EYES_ANIMATION_STACK_H
