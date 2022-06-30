#include <stdio.h>

#include "animation_stack.h"

/*
 * THIS IS WORK IN PROGRESS AND NOT YET USED
 */

int MAX_SIZE = 8;
string stack[8];
int top = -1;

bool isEmpty() {
    if (top == -1) {
        return true;
    }
    return false;
}

bool isFull() {
    if (top == MAX_SIZE) {
        return 1;
    }
    return 0;
}

string peek() {
    return stack[top];
}

string pop() {
    if (!isEmpty()) {
        top = top - 1;
        return stack[top];;
    }

    printf("[ERROR] Could not retrieve data. Stack is empty.\n");
    string s;
    s.capacity = -1;
    return s;
}

bool push(string data) {
    if (!isFull()) {
        top = top + 1;
        stack[top] = data;
        return true;
    }

    printf("[ERROR] Could not insert data. Stack is full.\n");
    return false;
}