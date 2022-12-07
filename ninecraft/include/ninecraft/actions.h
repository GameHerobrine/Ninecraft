#pragma once

typedef struct {
    int action;
    int keyCode;
} keyboard_action_t;

typedef struct {
    short x;
    short y;
    short dx;
    short dy;
    char button;
    char type;
    char pointer_id;
} mouse_action_0_6_t;

typedef struct {
    short x;
    short y;
    char button;
    char type;
    char pointer_id;
} mouse_action_0_5_t;