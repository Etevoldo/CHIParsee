#include <stdlib.h>
#include <stdio.h>
#include "list.h"

List list_create(int capacity)
{
    List list = (List) { .length = 0, .capacity = capacity};
    if ((list.arr = (int *) malloc(capacity * sizeof(list.arr))) == NULL) {
        exit(EXIT_FAILURE);
    }
    return list;
}

bool list_contains(List *list, int value)
{
    for (int i = 0; i < list->length; i++) {
        if (list->arr[i] == value) return true;
    }
    return false;
}

void list_push(List *list, int value)
{
    list->arr[list->length] = value;
    if (list->length < list->capacity) list->length++;
}

void list_print(List *list)
{
    for (int i = 0; i < list->length; i++) {
        printf("%d ", list->arr[i]);
    }
    printf("\n");
}

void list_remove(List *list, int index)
{
    for (int i = index + 1; i < list->length; i++) {
        list->arr[i - 1] = list->arr[i];
    }
    list->length--;
}

void list_dealloc(List *list)
{
    free(list->arr);
}