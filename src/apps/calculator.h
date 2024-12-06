#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <window.h>

// Maximum input length
#define CALC_MAX_INPUT 32
#define CALC_MAX_DIGITS 16

// Button types
#define BUTTON_NUMBER    0
#define BUTTON_OPERATOR  1
#define BUTTON_EQUAL     2
#define BUTTON_CLEAR     3
#define BUTTON_DECIMAL   4
#define BUTTON_BACKSPACE 5

// Button structure
typedef struct {
    char label[4];     // Button label
    int type;          // Button type
    char value;        // Button value
    int x, y;          // Button position
    int width, height; // Button dimensions
} calculator_button_t;

// Calculator structure
typedef struct {
    window_t* window;       // Window handle
    char display[CALC_MAX_DIGITS + 1];  // Display buffer
    int display_pos;        // Current display position
    double current_value;   // Current value
    double stored_value;    // Stored value
    char current_operator;  // Current operator
    int new_number;        // New number flag
    calculator_button_t buttons[20];  // Button array
} calculator_t;

// Calculator functions
calculator_t* create_calculator(int x, int y);
void destroy_calculator(calculator_t* calc);
void calculator_handle_key(window_t* window, char key);
void calculator_handle_click(window_t* window, int x, int y);
void calculator_draw(window_t* window);

// Internal calculator functions
void calculator_add_digit(calculator_t* calc, int digit);
void calculator_add_decimal(calculator_t* calc);
void calculator_set_operator(calculator_t* calc, char operator);
void calculator_calculate(calculator_t* calc);
void calculator_clear(calculator_t* calc);
void calculator_backspace(calculator_t* calc);

#endif
