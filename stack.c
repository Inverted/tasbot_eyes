#include <stdio.h>

#include "stack.h"

int top = -1;
void* stack[MAX_SIZE];

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

void* peek() {
    if (!isEmpty()) {
        return stack[top];
    }

    printf("[ERROR] Could not retrieve data. Stack is empty.\n");
    return NULL; //TODO: Check if returned item is not NULL
}

void* pop() {
    if (!isEmpty()) {
        void* item = stack[top];
        top--;
        printf("[INFO] Stack size is now %d\n", top);
        return item;
    }

    printf("[ERROR] Could not retrieve data. Stack is empty.\n");
    return NULL; //TODO: Check if returned item is not NULL
}

bool push(void* _item) {
    if (!isFull()) {
        top++;
        printf("[INFO] Stack size is now %d\n", top);
        stack[top] = _item; //not the same object

        return true;
    }

    printf("[ERROR] Could not insert data. Stack is full.\n");
    return false;
}