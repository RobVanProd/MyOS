#include "calculator.h"
#include <memory.h>
#include <graphics.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Button definitions
static const struct {
    char label[4];
    int type;
    char value;
} button_defs[] = {
    {"7", BUTTON_NUMBER, '7'},
    {"8", BUTTON_NUMBER, '8'},
    {"9", BUTTON_NUMBER, '9'},
    {"/", BUTTON_OPERATOR, '/'},
    {"4", BUTTON_NUMBER, '4'},
    {"5", BUTTON_NUMBER, '5'},
    {"6", BUTTON_NUMBER, '6'},
    {"*", BUTTON_OPERATOR, '*'},
    {"1", BUTTON_NUMBER, '1'},
    {"2", BUTTON_NUMBER, '2'},
    {"3", BUTTON_NUMBER, '3'},
    {"-", BUTTON_OPERATOR, '-'},
    {"0", BUTTON_NUMBER, '0'},
    {".", BUTTON_DECIMAL, '.'},
    {"=", BUTTON_EQUAL, '='},
    {"+", BUTTON_OPERATOR, '+'},
    {"C", BUTTON_CLEAR, 'C'},
    {"⌫", BUTTON_BACKSPACE, '\b'}
};

// Create calculator
calculator_t* create_calculator(int x, int y) {
    calculator_t* calc = kmalloc(sizeof(calculator_t));
    if (!calc) return NULL;
    
    // Initialize calculator data
    calc->display[0] = '0';
    calc->display[1] = '\0';
    calc->display_pos = 1;
    calc->current_value = 0;
    calc->stored_value = 0;
    calc->current_operator = 0;
    calc->new_number = 1;
    
    // Create window
    calc->window = create_window(x, y, 200, 250, "Calculator", 
        WINDOW_MOVABLE | WINDOW_HAS_TITLE | WINDOW_HAS_CLOSE);
    
    if (!calc->window) {
        kfree(calc);
        return NULL;
    }
    
    // Initialize buttons
    int button_width = (200 - 50) / 4;
    int button_height = 30;
    int start_x = 10;
    int start_y = 75;
    size_t num_buttons = sizeof(button_defs) / sizeof(button_defs[0]);
    
    for (size_t i = 0; i < num_buttons; i++) {
        strcpy(calc->buttons[i].label, button_defs[i].label);
        calc->buttons[i].type = button_defs[i].type;
        calc->buttons[i].value = button_defs[i].value;
        calc->buttons[i].x = start_x + (i % 4) * (button_width + 10);
        calc->buttons[i].y = start_y + (i / 4) * (button_height + 10);
        calc->buttons[i].width = button_width;
        calc->buttons[i].height = button_height;
    }
    
    // Set window callbacks
    calc->window->on_key = calculator_handle_key;
    calc->window->on_click = calculator_handle_click;
    calc->window->on_draw = calculator_draw;
    calc->window->data = calc;
    
    return calc;
}

void destroy_calculator(calculator_t* calc) {
    if (!calc) return;
    destroy_window(calc->window);
    kfree(calc);
}

void calculator_handle_key(window_t* window, char key) {
    calculator_t* calc = (calculator_t*)window->data;
    
    // Handle backspace
    if (key == '\b') {
        if (calc->display_pos > 0) {
            calc->display[--calc->display_pos] = '\0';
        }
    }
    // Handle numbers and operators
    else if ((key >= '0' && key <= '9') || 
             key == '+' || key == '-' || 
             key == '*' || key == '/' || 
             key == '.' || key == '=') {
        if (calc->display_pos < CALC_MAX_DIGITS) {
            calc->display[calc->display_pos++] = key;
            calc->display[calc->display_pos] = '\0';
            
            // Calculate result if equals pressed
            if (key == '=') {
                calculator_calculate(calc);
            }
        }
    }
    
    // Request redraw
    window_invalidate(window);
}

void calculator_handle_click(window_t* window, int x, int y) {
    calculator_t* calc = (calculator_t*)window->data;
    
    // Convert to window-relative coordinates
    x -= window->x;
    y -= window->y;
    
    // Check each button
    size_t num_buttons = sizeof(button_defs) / sizeof(button_defs[0]);
    for (size_t i = 0; i < num_buttons; i++) {
        if (x >= calc->buttons[i].x && x < calc->buttons[i].x + calc->buttons[i].width &&
            y >= calc->buttons[i].y && y < calc->buttons[i].y + calc->buttons[i].height) {
            
            // Handle button press
            switch (calc->buttons[i].type) {
                case BUTTON_NUMBER:
                    calculator_add_digit(calc, calc->buttons[i].value - '0');
                    break;
                    
                case BUTTON_OPERATOR:
                    char op = calc->buttons[i].value;
                    size_t len = strlen(calc->display);
                    
                    // Don't add operator if display is empty or ends with operator
                    if (len == 0 || strchr("+-*/", calc->display[len-1])) {
                        continue;
                    }
                    
                    // Add space before operator
                    if (len > 0 && calc->display[len-1] != ' ') {
                        calc->display[len++] = ' ';
                    }
                    calc->display[len++] = op;
                    calc->display[len++] = ' ';
                    calc->display[len] = '\0';
                    calc->display_pos = len;
                    break;
                    
                case BUTTON_EQUAL:
                    calculator_calculate(calc);
                    break;
                    
                case BUTTON_CLEAR:
                    calculator_clear(calc);
                    break;
                    
                case BUTTON_DECIMAL:
                    calculator_add_decimal(calc);
                    break;
                    
                case BUTTON_BACKSPACE:
                    calculator_backspace(calc);
                    break;
            }
            
            // Request redraw
            window_invalidate(window);
            break;
        }
    }
}

void calculator_draw(window_t* window) {
    calculator_t* calc = (calculator_t*)window->data;
    
    // Clear window
    draw_rect(window->x, window->y + 25, window->width, window->height - 25, COLOR_WINDOW_BG);
    
    // Draw display area
    draw_rect(window->x + 10, window->y + 35, window->width - 20, 30, COLOR_WHITE);
    draw_string(window->x + 15, window->y + 42, calc->display, COLOR_BLACK);
    
    // Draw buttons
    size_t num_buttons = sizeof(button_defs) / sizeof(button_defs[0]);
    for (size_t i = 0; i < num_buttons; i++) {
        int x = window->x + calc->buttons[i].x;
        int y = window->y + calc->buttons[i].y;
        
        // Draw button
        draw_rect(x, y, calc->buttons[i].width, calc->buttons[i].height, COLOR_GRAY);
        draw_string(x + (calc->buttons[i].width - strlen(calc->buttons[i].label) * 8) / 2,
                   y + (calc->buttons[i].height - 16) / 2,
                   calc->buttons[i].label, COLOR_BLACK);
    }
}

// Add digit to display
void calculator_add_digit(calculator_t* calc, int digit) {
    if (calc->new_number) {
        calc->display_pos = 0;
        calc->new_number = 0;
    }
    
    if (calc->display_pos < CALC_MAX_DIGITS) {
        if (calc->display_pos == 0 && digit == 0 && !strchr(calc->display, '.')) {
            return; // Don't add leading zeros
        }
        calc->display[calc->display_pos++] = '0' + digit;
        calc->display[calc->display_pos] = '\0';
    }
}

// Add decimal point
void calculator_add_decimal(calculator_t* calc) {
    if (calc->new_number) {
        calc->display_pos = 0;
        calc->display[calc->display_pos++] = '0';
        calc->new_number = 0;
    }
    
    // Check if decimal already exists
    if (!strchr(calc->display, '.') && calc->display_pos < CALC_MAX_DIGITS) {
        calc->display[calc->display_pos++] = '.';
        calc->display[calc->display_pos] = '\0';
    }
}

// Set operator
void calculator_set_operator(calculator_t* calc, char operator) {
    calculator_calculate(calc);
    calc->current_operator = operator;
    calc->stored_value = atof(calc->display);
    calc->new_number = 1;
}

// Calculate result
void calculator_calculate(calculator_t* calc) {
    double num1 = 0;
    double num2 = 0;
    char operator = '\0';
    char* token;
    char temp[CALC_MAX_DIGITS + 1];
    
    // Copy display to temp for tokenizing
    strncpy(temp, calc->display, CALC_MAX_DIGITS);
    temp[CALC_MAX_DIGITS] = '\0';
    
    // Get first number
    token = temp;
    while (*token && (*token >= '0' && *token <= '9' || *token == '.')) token++;
    char save = *token;
    *token = '\0';
    num1 = atof(temp);
    *token = save;
    
    // Get operator
    while (*token && (*token == ' ' || *token == '\t')) token++;
    operator = *token++;
    
    // Get second number
    while (*token && (*token == ' ' || *token == '\t')) token++;
    num2 = atof(token);
    
    // Perform calculation
    double result = 0;
    switch (operator) {
        case '+': result = num1 + num2; break;
        case '-': result = num1 - num2; break;
        case '*': result = num1 * num2; break;
        case '/': 
            if (num2 != 0) {
                result = num1 / num2;
            } else {
                strcpy(calc->display, "Error: Div by 0");
                calc->display_pos = strlen(calc->display);
                return;
            }
            break;
        default:
            strcpy(calc->display, "Error");
            calc->display_pos = strlen(calc->display);
            return;
    }
    
    // Convert result to string
    sprintf(calc->display, "%.6g", result);
    calc->display_pos = strlen(calc->display);
}

// Clear calculator
void calculator_clear(calculator_t* calc) {
    calc->display[0] = '0';
    calc->display[1] = '\0';
    calc->display_pos = 1;
    calc->current_value = 0;
    calc->stored_value = 0;
    calc->current_operator = 0;
    calc->new_number = 1;
}

// Handle backspace
void calculator_backspace(calculator_t* calc) {
    if (calc->display_pos > 0) {
        calc->display[--calc->display_pos] = '\0';
        if (calc->display_pos == 0) {
            calc->display[0] = '0';
            calc->display[1] = '\0';
            calc->display_pos = 1;
        }
    }
}