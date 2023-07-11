/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

// For simplicity the module accepts standard KP codes for other inputs. Since these contain values
// that correspond to HID page codes (and page zero is explicitly undefined) there should be no
// conflict.
#define RPN_DROP 0
#define RPN_ADD 1
#define RPN_SUBTRACT 2
#define RPN_MULTIPLY 3
#define RPN_DIVIDE 4
#define RPN_NEGATE 5
#define RPN_INVERT 6
#define RPN_SQROOT 7
#define RPN_LOG10 8
#define RPN_POWER 9
#define RPN_SINE 10
#define RPN_ARCSINE 11
#define RPN_EXTRACT_REAL 12
#define RPN_EXTRACT_IMAGINARY 13
