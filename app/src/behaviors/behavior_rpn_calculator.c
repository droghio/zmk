/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_rpn_calculator

#include <device.h>
#include <drivers/behavior.h>
#include <logging/log.h>

#include <zmk/rpn_calculator.h>
#include <zmk/event_manager.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/behavior.h>

#define RPN_LINE_WIDTH 10

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct behavior_rpn_calculator_data {
    // The text form input of the current line.
    char current_line[RPN_LINE_WIDTH];
    // Internal state of the RPN calculator.
    struct calculator_t calculator_state;
} local_data;

static int behavior_rpn_calculator_init(const struct device *dev) { return 0; };

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    struct behavior_rpn_calculator_data *data = dev->data;
    push_on_stack(&data->calculator_state, 1);
    double output = perform_operation(&data->calculator_state, CALC_OP_ADD);
    LOG_DBG("position %d keycode 0x%02X %0.3f", event.position, binding->param1, output);
    return 0;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    LOG_DBG("position %d keycode 0x%02X", event.position, binding->param1);
    return 0;
}

static const struct behavior_driver_api behavior_rpn_calculator_driver_api = {
    .binding_pressed = on_keymap_binding_pressed, .binding_released = on_keymap_binding_released};

    DEVICE_DT_INST_DEFINE(0, behavior_rpn_calculator_init, NULL, &local_data,    \
                          NULL, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                  \
                          &behavior_rpn_calculator_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
