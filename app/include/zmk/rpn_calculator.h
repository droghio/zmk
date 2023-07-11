/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
// TODO(droghio): Correct naming to meet ZMK standards.

#ifndef RPN_CALCULATOR_H_
#define RPN_CALCULATOR_H_

#include <complex.h>

#define CALCULATOR_STACK_DEPTH 256
#define CALCULATOR_MAX_ARGUMENTS 5

// List of supported operations.
enum calculator_operation_t {
    CALC_OP_DROP = 0,
    CALC_OP_ADD,
    CALC_OP_SUBTRACT,
    CALC_OP_MULTIPLY,
    CALC_OP_DIVIDE,
    CALC_OP_NEGATE,
    CALC_OP_INVERT,
    CALC_OP_SQROOT,
    CALC_OP_LOG10,
    CALC_OP_POWER,
    CALC_OP_SINE,
    CALC_OP_ARCSINE,
    CALC_OP_EXTRACT_REAL,
    CALC_OP_EXTRACT_IMAGINARY,
};

// Number of arguments necessary for each operation.
static const int kArityOfOperation[] = {
    [CALC_OP_DROP] = 1,         [CALC_OP_ADD] = 2,
    [CALC_OP_SUBTRACT] = 2,     [CALC_OP_MULTIPLY] = 2,
    [CALC_OP_DIVIDE] = 2,       [CALC_OP_NEGATE] = 1,
    [CALC_OP_INVERT] = 1,       [CALC_OP_SQROOT] = 1,
    [CALC_OP_LOG10] = 1,        [CALC_OP_POWER] = 2,
    [CALC_OP_SINE] = 1,         [CALC_OP_ARCSINE] = 1,
    [CALC_OP_EXTRACT_REAL] = 1, [CALC_OP_EXTRACT_IMAGINARY] = 1,
};

struct calculator_t {
    // The stack of previous values.
    double complex buffer_stack[CALCULATOR_STACK_DEPTH];
    // Currently used frames of the stack.
    int stack_depth;
};

// Adds a value to the stack. If adding the value would overflow the stack first
// pop the oldest frame to bound the stack size. Returns the value pushed.
double complex push_on_stack(struct calculator_t *calculator, double complex value);

// Executes the provided `operation` and appends the result to the stack.
// Returns this value.
double complex perform_operation(struct calculator_t *calculator,
                                 enum calculator_operation_t operation);

#endif // RPN_CALCULATOR_H_
