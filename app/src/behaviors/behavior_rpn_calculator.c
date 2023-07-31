/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_rpn_calculator

#include <stdio.h>
#include <stdlib.h>
#include <device.h>
#include <drivers/behavior.h>
#include <drivers/lcd_hd44780_i2c.h>
#include <logging/log.h>

#include <zmk/keys.h>
#include <zmk/rpn_calculator.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/behavior.h>

#include <sys/cbprintf.h>

#define RPN_LINE_WIDTH 17

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_rpn_calculator_data {
    // The text form input of the current line.
    char current_line[RPN_LINE_WIDTH];
    // Internal state of the RPN calculator.
    struct calculator_t calculator_state;
} local_data;

// Reflects the current RPN state to the configured display.
static void update_display(const struct behavior_rpn_calculator_data *rpn_data) {
    int buffer_depth = rpn_data->calculator_state.stack_depth;
    double complex current_frame =
        rpn_data->calculator_state.buffer_stack[buffer_depth ? buffer_depth - 1 : 0];
    const struct device *lcd = DEVICE_DT_GET(DT_NODELABEL(lcd));
    if (!device_is_ready(lcd)) {
        LOG_ERR("LCD: Device not ready: %d\n", lcd->state->init_res);
        return;
    }
    lcd_clear(lcd);
    lcd_cursor_pos_set(lcd, 0, 0);
    char output_buffer[LCD_NUM_COLS + 1] = "";
    snprintfcb(output_buffer, sizeof(output_buffer), "%0.3g%+0.3gi |%d", creal(current_frame),
            cimag(current_frame), buffer_depth);
    LOG_INF("%s", output_buffer);
    lcd_print(lcd, output_buffer, strlen(output_buffer));
    lcd_cursor_pos_set(lcd, 1, 0);
    LOG_INF("%s", rpn_data->current_line);
    lcd_print(lcd, rpn_data->current_line, strlen(rpn_data->current_line));
    lcd_cursor_pos_set(lcd, 1, strlen(rpn_data->current_line));
};

// Update the current entry buffer with the selected digit. Returns false if input was not valid.
static bool handle_key_input(struct behavior_rpn_calculator_data *rpn_data, zmk_key_t key_pressed) {
    int line_length = strlen(rpn_data->current_line);
    switch (key_pressed) {
    case KP_NUMBER_1:
    case KP_NUMBER_2:
    case KP_NUMBER_3:
    case KP_NUMBER_4:
    case KP_NUMBER_5:
    case KP_NUMBER_6:
    case KP_NUMBER_7:
    case KP_NUMBER_8:
    case KP_NUMBER_9:
        if (line_length >= sizeof(rpn_data->current_line) - 1) {
            return false;
        }
        rpn_data->current_line[line_length] = key_pressed - KP_NUMBER_1 + '1';
        rpn_data->current_line[line_length + 1] = '\0';
        return true;
    // Special handling because zero comes after nine in the HID tables.
    case KP_NUMBER_0:
        if (line_length >= sizeof(rpn_data->current_line) - 1) {
            return false;
        }
        rpn_data->current_line[line_length] = '0';
        rpn_data->current_line[line_length + 1] = '\0';
        return true;
    case KP_DOT:
        if (line_length >= sizeof(rpn_data->current_line) - 1) {
            return false;
        }
        // Do not allow multiple periods in the current line.
        if (memchr(rpn_data->current_line, '.', sizeof(rpn_data->current_line)) != NULL) {
            return false;
        }
        rpn_data->current_line[line_length] = '.';
        rpn_data->current_line[line_length + 1] = '\0';
        return true;
    case BACKSPACE:
        if (line_length == 0) {
            return false;
        }
        if (line_length >= sizeof(rpn_data->current_line)) {
            line_length = sizeof(rpn_data->current_line) - 1;
        }
        rpn_data->current_line[line_length - 1] = '\0';
        return true;
    case KP_ENTER:
        if (strlen(rpn_data->current_line) == 0) {
            return false;
        }
        // TODO(droghio): Add error checking for invalid input.
        push_on_stack(&rpn_data->calculator_state, strtod(rpn_data->current_line, NULL));
        rpn_data->current_line[0] = '\0';
        return true;
    }
    return false;
}

static int behavior_rpn_calculator_init(const struct device *dev) {
    struct behavior_rpn_calculator_data *rpn_data = dev->data;
    memset(rpn_data, sizeof(struct behavior_rpn_calculator_data), 0);
    return 0;
};

static int on_rpn_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                         struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d keycode 0x%02X", event.position, binding->param1);
    struct behavior_rpn_calculator_data *rpn_data = device_get_binding(binding->behavior_dev)->data;
    // Any entry in the undefined block is assumed to be a RPN command.
    if (binding->param1 < ZMK_HID_USAGE(HID_USAGE_GD, HID_USAGE_GD_UNDEFINED)) {
        // TODO(droghio): Actually break this out into a proper helper method.
        handle_key_input(rpn_data, KP_ENTER);
        perform_operation(&rpn_data->calculator_state, binding->param1);
    } else if (handle_key_input(rpn_data, binding->param1) == false) {
        LOG_WRN("invalid input received: %d", binding->param1);
    }
    update_display(rpn_data);
    return 0;
}

static int on_rpn_keymap_binding_released(struct zmk_behavior_binding *binding,
                                          struct zmk_behavior_binding_event event) {
    // TODO(droghio): Is this necessary?
    return 0;
}

static const struct behavior_driver_api behavior_rpn_calculator_driver_api = {
    .binding_pressed = on_rpn_keymap_binding_pressed,
    .binding_released = on_rpn_keymap_binding_released};

DEVICE_DT_INST_DEFINE(0, behavior_rpn_calculator_init, NULL, &local_data, NULL, APPLICATION,
                      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_rpn_calculator_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
