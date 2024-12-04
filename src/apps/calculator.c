#include "calculator.h"
#include "../kernel/memory.h"

// Button definitions
static const struct {
    char label[4];
    uint8_t type;
    uint8_t value;
} button_defs[] = {
    {"7", BUTTON_NUMBER, 7},
    {"8", BUTTON_NUMBER, 8},
    {"9", BUTTON_NUMBER, 9},
    {"/", BUTTON_OPERATOR, '/'},
    {"4", BUTTON_NUMBER, 4},
    {"5", BUTTON_NUMBER, 5},
    {"6", BUTTON_NUMBER, 6},
    {"*", BUTTON_OPERATOR, '*'},
    {"1", BUTTON_NUMBER, 1},
    {"2", BUTTON_NUMBER, 2},
    {"3", BUTTON_NUMBER, 3},
    {"-", BUTTON_OPERATOR, '-'},
    {"0", BUTTON_NUMBER, 0},
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
    
    // Create window
    calc->window = create_window(x, y, CALC_WIDTH, CALC_HEIGHT, "Calculator",
        WINDOW_MOVABLE | WINDOW_HAS_TITLE | WINDOW_HAS_CLOSE);
    
    if (!calc->window) {
        kfree(calc);
        return NULL;
    }
    
    // Initialize calculator state
    calc->display_pos = 0;
    calc->display[0] = '0';
    calc->display[1] = '\0';
    calc->current_value = 0;
    calc->stored_value = 0;
    calc->current_operator = 0;
    calc->new_number = true;
    
    // Set up buttons
    int button_x = 10;
    int button_y = CALC_DISPLAY_HEIGHT + 10;
    int button_index = 0;
    
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 4; col++) {
            if (button_index >= sizeof(button_defs) / sizeof(button_defs[0])) break;
            
            calc->buttons[button_index].x = button_x + col * (CALC_BUTTON_WIDTH + 5);
            calc->buttons[button_index].y = button_y + row * (CALC_BUTTON_HEIGHT + 5);
            calc->buttons[button_index].width = CALC_BUTTON_WIDTH;
            calc->buttons[button_index].height = CALC_BUTTON_HEIGHT;
            strcpy(calc->buttons[button_index].label, button_defs[button_index].label);
            calc->buttons[button_index].type = button_defs[button_index].type;
            calc->buttons[button_index].value = button_defs[button_index].value;
            
            button_index++;
        }
    }
    
    // Set window callbacks
    calc->window->on_click = calculator_handle_click;
    calc->window->on_key = calculator_handle_key;
    calc->window->on_draw = calculator_draw;
    
    return calc;
}

// Destroy calculator
void destroy_calculator(calculator_t* calc) {
    if (!calc) return;
    destroy_window(calc->window);
    kfree(calc);
}

// Handle mouse click
void calculator_handle_click(window_t* window, int x, int y) {
    calculator_t* calc = (calculator_t*)window->user_data;
    if (!calc) return;
    
    // Check each button
    for (int i = 0; i < sizeof(button_defs) / sizeof(button_defs[0]); i++) {
        if (x >= calc->buttons[i].x && x < calc->buttons[i].x + calc->buttons[i].width &&
            y >= calc->buttons[i].y && y < calc->buttons[i].y + calc->buttons[i].height) {
            
            // Handle button press
            switch (calc->buttons[i].type) {
                case BUTTON_NUMBER:
                    calculator_add_digit(calc, calc->buttons[i].value);
                    break;
                    
                case BUTTON_OPERATOR:
                    calculator_set_operator(calc, calc->buttons[i].value);
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
            break;
        }
    }
}

// Handle keyboard input
void calculator_handle_key(window_t* window, char key) {
    calculator_t* calc = (calculator_t*)window->user_data;
    if (!calc) return;
    
    if (key >= '0' && key <= '9') {
        calculator_add_digit(calc, key - '0');
    }
    else if (key == '+' || key == '-' || key == '*' || key == '/') {
        calculator_set_operator(calc, key);
    }
    else if (key == '=' || key == '\n') {
        calculator_calculate(calc);
    }
    else if (key == 'c' || key == 'C') {
        calculator_clear(calc);
    }
    else if (key == '.') {
        calculator_add_decimal(calc);
    }
    else if (key == '\b') {
        calculator_backspace(calc);
    }
}

// Draw calculator
void calculator_draw(window_t* window) {
    calculator_t* calc = (calculator_t*)window->user_data;
    if (!calc) return;
    
    // Draw display background
    fill_rect(window->x + 10, window->y + 30, CALC_WIDTH - 20, CALC_DISPLAY_HEIGHT,
             COLOR_WINDOW_BG);
    draw_rect(window->x + 10, window->y + 30, CALC_WIDTH - 20, CALC_DISPLAY_HEIGHT,
             COLOR_WINDOW_FRAME);
    
    // Draw display text
    draw_string_with_bg(window->x + 15, window->y + 45, calc->display,
                       COLOR_TEXT, COLOR_WINDOW_BG);
    
    // Draw buttons
    for (int i = 0; i < sizeof(button_defs) / sizeof(button_defs[0]); i++) {
        int x = window->x + calc->buttons[i].x;
        int y = window->y + calc->buttons[i].y;
        
        // Draw button background
        fill_rect(x, y, calc->buttons[i].width, calc->buttons[i].height,
                 COLOR_WINDOW_FRAME);
        draw_rect(x, y, calc->buttons[i].width, calc->buttons[i].height,
                 COLOR_HIGHLIGHT);
        
        // Draw button label
        int label_x = x + (calc->buttons[i].width - strlen(calc->buttons[i].label) * 8) / 2;
        int label_y = y + (calc->buttons[i].height - 8) / 2;
        draw_string_with_bg(label_x, label_y, calc->buttons[i].label,
                          COLOR_TEXT, COLOR_WINDOW_FRAME);
    }
}

// Add digit to display
void calculator_add_digit(calculator_t* calc, int digit) {
    if (calc->new_number) {
        calc->display_pos = 0;
        calc->new_number = false;
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
        calc->new_number = false;
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
    calc->new_number = true;
}

// Calculate result
void calculator_calculate(calculator_t* calc) {
    if (!calc->current_operator) return;
    
    double current = atof(calc->display);
    double result = 0;
    
    switch (calc->current_operator) {
        case '+':
            result = calc->stored_value + current;
            break;
        case '-':
            result = calc->stored_value - current;
            break;
        case '*':
            result = calc->stored_value * current;
            break;
        case '/':
            if (current != 0) {
                result = calc->stored_value / current;
            } else {
                strcpy(calc->display, "Error");
                calc->new_number = true;
                return;
            }
            break;
    }
    
    // Convert result to string
    sprintf(calc->display, "%.10g", result);
    calc->display_pos = strlen(calc->display);
    calc->current_operator = 0;
    calc->new_number = true;
}

// Clear calculator
void calculator_clear(calculator_t* calc) {
    calc->display[0] = '0';
    calc->display[1] = '\0';
    calc->display_pos = 1;
    calc->current_value = 0;
    calc->stored_value = 0;
    calc->current_operator = 0;
    calc->new_number = true;
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