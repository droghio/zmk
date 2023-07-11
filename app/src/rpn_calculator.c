/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
// TODO(droghio): Correct naming to meet ZMK standards.

#include <string.h>
#include <math.h>

#include <zmk/rpn_calculator.h>

// Internal Functions

// Drops the `frames_to_prune` oldest records from the calculator buffer.
static void prune_oldest_frames(struct calculator_t *calculator, int frames_to_prune) {
    memmove(calculator->buffer_stack, calculator->buffer_stack + frames_to_prune,
            sizeof(calculator->buffer_stack) - frames_to_prune);
    calculator->stack_depth -= frames_to_prune;
}

// Copies over the specified number of arguments from the stack to the given
// buffer. If there are not enough arguments on the stack zeros are appended
// to the output buffer. If number of arguments to copy is smaller than the
// output buffer unused elements in the buffer will be zeroed.
static void copy_arguments(const struct calculator_t *calculator, int number_arguments,
                           double output_buffer[CALCULATOR_MAX_ARGUMENTS]) {
    memset(output_buffer, 0, sizeof(output_buffer[0]) * CALCULATOR_MAX_ARGUMENTS);

    int number_of_arguments_to_copy =
        number_arguments <= calculator->stack_depth ? number_arguments : calculator->stack_depth;
    memcpy(output_buffer,
           calculator->buffer_stack + calculator->stack_depth - number_of_arguments_to_copy,
           sizeof(output_buffer[0]) * number_of_arguments_to_copy);
};

// Taken from the upstream zephyr codebase. Upped the number of iterations on
// the Newton method to reduce error.
static double sqrt(double value) {
    if (value <= 0) {
        return 0;
    }
    // Generate an initial guess and improve it using Newton's method.
    double sqrt = value / 3;
    for (int i = 0; i < 16; i++) {
        sqrt = (sqrt + value / sqrt) / 2;
    }
    return sqrt;
}

// Public Functions

double push_on_stack(struct calculator_t *calculator, double value) {
    while (calculator->stack_depth > CALCULATOR_STACK_DEPTH - 1) {
        prune_oldest_frames(calculator, 1);
    }
    calculator->buffer_stack[calculator->stack_depth] = value;
    calculator->stack_depth++;
    return calculator->buffer_stack[calculator->stack_depth - 1];
}

double perform_operation(struct calculator_t *calculator, enum calculator_operation_t operation) {
    double arguments[CALCULATOR_MAX_ARGUMENTS];
    copy_arguments(calculator, kArityOfOperation[operation], arguments);

    switch (operation) {
    case CALC_OP_DROP:
        push_on_stack(calculator, arguments[0]);
        break;
    case CALC_OP_ADD:
        push_on_stack(calculator, arguments[0] + arguments[1]);
        break;
    case CALC_OP_SUBTRACT:
        push_on_stack(calculator, arguments[0] - arguments[1]);
        break;
    case CALC_OP_MULTIPLY:
        push_on_stack(calculator, arguments[0] * arguments[1]);
        break;
    case CALC_OP_DIVIDE:
        push_on_stack(calculator, arguments[0] / arguments[1]);
        break;
    case CALC_OP_NEGATE:
        push_on_stack(calculator, -arguments[0]);
        break;
    case CALC_OP_INVERT:
        push_on_stack(calculator, 1 / arguments[0]);
        break;
    case CALC_OP_SQROOT:
        push_on_stack(calculator, sqrt(arguments[0]));
        break;
    }
    return calculator->buffer_stack[calculator->stack_depth - 1];
}
