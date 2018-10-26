#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    char *name;
} MenuItem;

typedef struct {
    int num_items;
    int selected_item;
    MenuItem items[10];
    char value[16];
    void (*draw)(void*, int);
} Menu;



void ui_init(void);
void ui_update(void);
void draw_menu(Menu menu);

extern float seq_note_input;

