#include "calculator.h"
#include <terminal.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <heap.h>

// Simple string to float conversion
static double atof(const char* str) {
    double result = 0.0;
    double fraction = 0.0;
    int decimal_seen = 0;
    double decimal_places = 1.0;
    int negative = 0;

    // Handle negative numbers
    if (*str == '-') {
        negative = 1;
        str++;
    }

    while (*str) {
        if (*str == '.') {
            decimal_seen = 1;
            str++;
            continue;
        }

        if (*str >= '0' && *str <= '9') {
            if (!decimal_seen) {
                result = result * 10.0 + (*str - '0');
            } else {
                decimal_places *= 10.0;
                fraction = fraction * 10.0 + (*str - '0');
            }
        }
        str++;
    }

    result += fraction / decimal_places;
    return negative ? -result : result;
}

// Simple snprintf implementation for floats
static int snprintf(char* str, size_t size, const char* format, double value) {
    if (!str || size == 0) return 0;

    // Handle negative numbers
    int len = 0;
    if (value < 0) {
        if (len < size - 1) {
            str[len++] = '-';
            value = -value;
        } else {
            return len;
        }
    }

    // Extract integer and decimal parts
    int32_t integer_part = (int32_t)value;
    double decimal_part = value - integer_part;

    // Convert integer part
    char int_str[32];
    int_to_string(integer_part, int_str);
    
    // Copy integer part to output
    int i = 0;
    while (int_str[i] && len < size - 1) {
        str[len++] = int_str[i++];
    }

    // Handle decimal part if present
    if (decimal_part > 0.000001 && len < size - 2) {
        str[len++] = '.';
        for (i = 0; i < 6 && len < size - 1; i++) {
            decimal_part *= 10;
            int digit = (int)decimal_part;
            str[len++] = '0' + digit;
            decimal_part -= digit;
        }
    }

    // Null terminate
    if (len < size) {
        str[len] = '\0';
    } else {
        str[size - 1] = '\0';
        len = size - 1;
    }

    return len;
}

// Button definitions
static const calc_button_t default_buttons[] = {
    {0, 0, CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '7', false},
    {CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING, 0, CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '8', false},
    {2 * (CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING), 0, CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '9', false},
    {3 * (CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING), 0, CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '/', false},
    
    {0, CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING, CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '4', false},
    {CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING, CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING, CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '5', false},
    {2 * (CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING), CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING, CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '6', false},
    {3 * (CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING), CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING, CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '*', false},
    
    {0, 2 * (CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING), CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '1', false},
    {CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING, 2 * (CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING), CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '2', false},
    {2 * (CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING), 2 * (CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING), CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '3', false},
    {3 * (CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING), 2 * (CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING), CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '-', false},
    
    {0, 3 * (CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING), CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '0', false},
    {CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING, 3 * (CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING), CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '.', false},
    {2 * (CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING), 3 * (CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING), CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '=', false},
    {3 * (CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING), 3 * (CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING), CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '+', false},
    
    {0, 4 * (CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING), CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, 'C', false},
    {CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING, 4 * (CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING), CALC_BUTTON_WIDTH, CALC_BUTTON_HEIGHT, '\b', false}
};

calculator_t* create_calculator(int x, int y) {
    calculator_t* calc = heap_alloc(sizeof(calculator_t));
    if (!calc) return NULL;

    calc->x = x;
    calc->y = y;
    calc->width = 4 * (CALC_BUTTON_WIDTH + CALC_BUTTON_SPACING);
    calc->height = 5 * (CALC_BUTTON_HEIGHT + CALC_BUTTON_SPACING);
    calc->display_length = 0;
    calc->current_value = 0;
    calc->stored_value = 0;
    calc->operator = 0;
    calc->decimal_used = false;
    calc->new_number = true;

    memset(calc->display, 0, CALC_MAX_DIGITS);
    memcpy(calc->buttons, default_buttons, sizeof(default_buttons));
    calc->num_buttons = sizeof(default_buttons) / sizeof(default_buttons[0]);

    return calc;
}

void destroy_calculator(calculator_t* calc) {
    if (calc) {
        heap_free(calc);
    }
}

void calculator_draw(calculator_t* calc) {
    // Clear the display area
    terminal_writestring("\n Calculator\n");
    terminal_writestring(" -----------\n");
    terminal_writestring(" |");
    terminal_writestring(calc->display);
    terminal_writestring("|\n");
    terminal_writestring(" -----------\n");

    // Draw buttons
    for (int i = 0; i < calc->num_buttons; i++) {
        terminal_putchar(' ');
        terminal_putchar('[');
        terminal_putchar(calc->buttons[i].value);
        terminal_putchar(']');
        if ((i + 1) % 4 == 0) {
            terminal_writestring("\n");
        }
    }
    terminal_writestring("\n");
}

void calculator_handle_key(calculator_t* calc, char key) {
    if (key >= '0' && key <= '9') {
        calculator_add_digit(calc, key - '0');
    } else if (key == '+' || key == '-' || key == '*' || key == '/') {
        calculator_set_operator(calc, key);
    } else if (key == '=' || key == '\n') {
        calculator_calculate(calc);
    } else if (key == 'c' || key == 'C') {
        calculator_clear(calc);
    } else if (key == '.') {
        calculator_add_decimal(calc);
    } else if (key == '\b') {
        calculator_backspace(calc);
    }
}

void calculator_handle_click(calculator_t* calc, int x, int y) {
    for (int i = 0; i < calc->num_buttons; i++) {
        calc_button_t* btn = &calc->buttons[i];
        if (x >= btn->x && x < btn->x + btn->width &&
            y >= btn->y && y < btn->y + btn->height) {
            calculator_handle_key(calc, btn->value);
            break;
        }
    }
}

void calculator_add_digit(calculator_t* calc, int digit) {
    if (calc->display_length >= CALC_MAX_DIGITS - 1) return;
    
    if (calc->new_number) {
        calc->display_length = 0;
        calc->new_number = false;
    }
    
    calc->display[calc->display_length++] = digit + '0';
    calc->display[calc->display_length] = '\0';
}

void calculator_add_decimal(calculator_t* calc) {
    if (calc->decimal_used) return;
    if (calc->display_length >= CALC_MAX_DIGITS - 1) return;
    
    if (calc->new_number) {
        calc->display_length = 0;
        calc->display[calc->display_length++] = '0';
        calc->new_number = false;
    }
    
    calc->display[calc->display_length++] = '.';
    calc->display[calc->display_length] = '\0';
    calc->decimal_used = true;
}

void calculator_set_operator(calculator_t* calc, char op) {
    calculator_calculate(calc);
    calc->operator = op;
    calc->stored_value = calc->current_value;
    calc->new_number = true;
    calc->decimal_used = false;
}

void calculator_calculate(calculator_t* calc) {
    if (!calc->operator) {
        calc->current_value = atof(calc->display);
        return;
    }
    
    double operand = atof(calc->display);
    
    switch (calc->operator) {
        case '+':
            calc->current_value = calc->stored_value + operand;
            break;
        case '-':
            calc->current_value = calc->stored_value - operand;
            break;
        case '*':
            calc->current_value = calc->stored_value * operand;
            break;
        case '/':
            if (operand != 0) {
                calc->current_value = calc->stored_value / operand;
            } else {
                terminal_writestring("Error: Division by zero\n");
                calculator_clear(calc);
                return;
            }
            break;
    }
    
    // Convert result to string
    snprintf(calc->display, CALC_MAX_DIGITS, "%.6g", calc->current_value);
    calc->display_length = strlen(calc->display);
    calc->operator = 0;
    calc->new_number = true;
    calc->decimal_used = false;
}

void calculator_clear(calculator_t* calc) {
    calc->display_length = 0;
    calc->display[0] = '\0';
    calc->current_value = 0;
    calc->stored_value = 0;
    calc->operator = 0;
    calc->decimal_used = false;
    calc->new_number = true;
}

void calculator_backspace(calculator_t* calc) {
    if (calc->display_length > 0) {
        if (calc->display[calc->display_length - 1] == '.') {
            calc->decimal_used = false;
        }
        calc->display[--calc->display_length] = '\0';
    }
}

void calculator_text_mode(void) {
    calculator_t* calc = create_calculator(0, 0);
    if (!calc) {
        terminal_writestring("Error: Failed to create calculator\n");
        return;
    }

    terminal_writestring("\nCalculator Text Mode\n");
    terminal_writestring("Commands: number, +, -, *, /, =, c (clear), q (quit)\n");

    while (1) {
        calculator_draw(calc);
        terminal_writestring("> ");
        
        char key = terminal_getchar();
        if (key == 'q' || key == 'Q') break;
        
        calculator_handle_key(calc, key);
    }

    destroy_calculator(calc);
}