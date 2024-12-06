#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define CALC_MAX_DIGITS 32
#define CALC_MAX_BUTTONS 20
#define CALC_BUTTON_WIDTH 40
#define CALC_BUTTON_HEIGHT 30
#define CALC_BUTTON_SPACING 5

// Calculator button structure
typedef struct {
    int x;
    int y;
    int width;
    int height;
    char value;
    bool pressed;
} calc_button_t;

// Calculator structure
typedef struct {
    int x;
    int y;
    int width;
    int height;
    char display[CALC_MAX_DIGITS];
    int display_length;
    double current_value;
    double stored_value;
    char operator;
    bool decimal_used;
    bool new_number;
    calc_button_t buttons[CALC_MAX_BUTTONS];
    int num_buttons;
} calculator_t;

// Function declarations
calculator_t* create_calculator(int x, int y);
void destroy_calculator(calculator_t* calc);
void calculator_draw(calculator_t* calc);
void calculator_handle_key(calculator_t* calc, char key);
void calculator_handle_click(calculator_t* calc, int x, int y);
void calculator_add_digit(calculator_t* calc, int digit);
void calculator_add_decimal(calculator_t* calc);
void calculator_set_operator(calculator_t* calc, char operator);
void calculator_calculate(calculator_t* calc);
void calculator_clear(calculator_t* calc);
void calculator_backspace(calculator_t* calc);
void calculator_text_mode(void);

#endif
