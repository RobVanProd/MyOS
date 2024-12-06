#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <graphics.h>
#include <keyboard.h>
#include <stdbool.h>

// Calculator constants
#define CALC_WIDTH 200
#define CALC_HEIGHT 300
#define CALC_BUTTON_WIDTH 40
#define CALC_BUTTON_HEIGHT 40
#define CALC_DISPLAY_HEIGHT 60
#define CALC_MAX_DIGITS 16

// Calculator button types
#define BUTTON_NUMBER    0
#define BUTTON_OPERATOR  1
#define BUTTON_EQUAL     2
#define BUTTON_CLEAR     3
#define BUTTON_DECIMAL   4
#define BUTTON_BACKSPACE 5

// Calculator button structure
typedef struct {
    int x, y;
    int width, height;
    char label[4];
    uint8_t type;
    uint8_t value;
} calc_button_t;

// Calculator structure
typedef struct {
    window_t* window;
    char display[CALC_MAX_DIGITS + 1];
    int display_pos;
    double current_value;
    double stored_value;
    char current_operator;
    bool new_number;
    calc_button_t buttons[20];
} calculator_t;

// Calculator functions
calculator_t* create_calculator(int x, int y);
void destroy_calculator(calculator_t* calc);
void calculator_handle_click(window_t* window, int x, int y);
void calculator_handle_key(window_t* window, char key);
void calculator_draw(window_t* window);

// Calculator operations
void calculator_add_digit(calculator_t* calc, int digit);
void calculator_add_decimal(calculator_t* calc);
void calculator_set_operator(calculator_t* calc, char operator);
void calculator_calculate(calculator_t* calc);
void calculator_clear(calculator_t* calc);
void calculator_backspace(calculator_t* calc);

#endif 