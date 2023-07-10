/*
 * Copyright (c) 2022
 *
 * SPDX-License-Identifier: Apache-2.0
 * NOTICE: API derived from Zephyr Project's Grove LCD Driver:
 *      zephyr/include/zephyr/drivers/misc/grove_lcd/grove_lcd.h
 */

#define DT_DRV_COMPAT hit_hd44780

#include "lcd_hd44780_i2c.h"

#include <device.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <sys/util.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(lcd_hd44780, 1);

//---------------------------------------
// Random Animation
//---------------------------------------

/* animation sleep time in milliseconds */
#define ANIMATION_SLEEPTIME 10
static char str[2][LCD_NUM_COLS] = {
    {' ', LCD_CHAR_SQRT, '(', '-', '1', ')', '*', LCD_CHAR_PI, LCD_CHAR_INV,
     LCD_CHAR_DIV, LCD_CHAR_MU, '=', '2', LCD_CHAR_IMAG, '+', LCD_CHAR_INF},
    {' ', LCD_CHAR_SQRT, '(', '-', '1', ')', '*', LCD_CHAR_PI, LCD_CHAR_INV,
     LCD_CHAR_DIV, LCD_CHAR_MU, '=', '2', LCD_CHAR_IMAG, '+', LCD_CHAR_INF}};

static uint8_t *rotate(uint8_t *input, uint8_t length, bool direction_right) {
  static uint8_t buffer[256] = {};
  if (direction_right) {
    buffer[0] = input[length - 1];
    memcpy(buffer + 1, input, length - 1);
  } else {
    buffer[length - 1] = input[0];
    memcpy(buffer, input + 1, length - 1);
  }
  return buffer;
}

static int animation_frames = 0;
static void my_work_handler(struct k_work *work) {
    const struct device *lcd = DEVICE_DT_GET(DT_NODELABEL(lcd));
    if (!device_is_ready(lcd)) {
        LOG_ERR("LCD: Device not ready: %d\n", lcd->state->init_res);
        return;
  }
  int i = animation_frames;
  memcpy(str[0], rotate(str[0], sizeof(str[0]), i < 80), sizeof(str[0]));
  memcpy(str[1], rotate(str[1], sizeof(str[1]), i > 80), sizeof(str[1]));
  lcd_cursor_pos_set(lcd, 0, 0);
  lcd_print(lcd, str[0], sizeof(str[0]));
  lcd_cursor_pos_set(lcd, 1, 0);
  lcd_print(lcd, str[1], sizeof(str[1]));
  lcd_cursor_pos_set(lcd, 0, LCD_NUM_COLS - 1);
  animation_frames++;
  if (animation_frames > 170) {
    animation_frames = 0;
  }
}

K_WORK_DEFINE(my_work, my_work_handler);

// Work queue for executing the draw calls.
static struct k_work_q display_work_q;
void my_timer_handler(struct k_timer *unused) { k_work_submit_to_queue(&display_work_q, &my_work); }

K_THREAD_STACK_DEFINE(display_work_stack_area, 2048);
K_TIMER_DEFINE(my_timer, my_timer_handler, NULL);

// Structure for persistent data across LCD transactions.
struct lcd_data {
  // Whether the backlight should be enabled or not.
  uint8_t backlight_enabled : 1;
};

struct lcd_config {
  struct i2c_dt_spec bus;
};

//---------------------------------------
// Command Defines
//---------------------------------------

#define LCD_START_SLEEP_TIME_MS 50
#define LCD_CMD_CLEAR_TIME_MS 5
#define LCD_CMD_SLEEP_TIME_US 50 // Spec sheet says 37us for most commands.
#define LCD_EN_STROBE_TIME_US 1 // Spec sheet says minimum 450ns.

// Display commands (from data sheet).
#define LCD_CMD_SCREEN_CLEAR (1 << 0)
#define LCD_CMD_CURSOR_RETURN (1 << 1)
#define LCD_CMD_INPUT_SET (1 << 2)
#define LCD_CMD_DISPLAY_SWITCH (1 << 3)
#define LCD_CMD_CURSOR_SHIFT (1 << 4)
#define LCD_CMD_FUNCTION_SET (1 << 5)
#define LCD_CMD_SET_CGRAM_ADDR (1 << 6)
#define LCD_CMD_SET_DDRAM_ADDR (1 << 7)

// The backlight, enable, read/write and register selects.
#define LCD_BL (1 << 3)
#define LCD_EN (1 << 2)
#define LCD_RW (1 << 1)
#define LCD_RS (1 << 0)
#define LCD_BUSY (1 << 7)
#define LCD_DEFAULT_OPTIONS 0

// Internal helper call to LCD module. Writes one nibble to the device and
// strobes the enable bit.
static void lcd_write_nibble(const struct device *dev, uint8_t data) {
  uint8_t buf = data;
  const struct lcd_config *config = dev->config;
  i2c_write_dt(&config->bus, &buf, sizeof(buf));
  k_usleep(LCD_EN_STROBE_TIME_US);

  // Strobe the enable.
  buf |= LCD_EN;
  i2c_write_dt(&config->bus, &buf, sizeof(buf));
  k_usleep(LCD_EN_STROBE_TIME_US);

  // Wait for the command to execute.
  buf = data & ~LCD_EN;
  i2c_write_dt(&config->bus, &buf, sizeof(buf));
  k_usleep(LCD_CMD_SLEEP_TIME_US);
}

// Main write call to the LCD. Breaks down the command byte into the two nibble
// transactions necessary for the four bit interface.
static void lcd_write(const struct device *dev, uint8_t data,
               uint8_t options_or_mask) {
  const struct lcd_data *state = dev->data;
  uint8_t or_mask = (state->backlight_enabled ? LCD_BL : LCD_DEFAULT_OPTIONS) |
                    options_or_mask;
  lcd_write_nibble(dev, (data & 0xF0) | or_mask);
  lcd_write_nibble(dev, ((data << 4) & 0xF0) | or_mask);
}

// Defines for the function selection.
#define LCD_FS_4BIT_MODE (0 << 4)
#define LCD_FS_8BIT_MODE (1 << 4)
// Two row mode requires more power than one row (lower duty cycle per
// character). Recommend >3.7V.
#define LCD_FS_ROWS_2 (1 << 3)
#define LCD_FS_ROWS_1 (0 << 3)
// Big font is not compatible with 2 row mode, 2 row will take precidence.
#define LCD_FS_DOT_SIZE_BIG (1 << 2)
#define LCD_FS_DOT_SIZE_LITTLE (0 << 2)

// Initial function configuration, only to be called once as the first command
// during chip boot to set the transaction size.
void lcd_initial_function_set(const struct device *dev, uint8_t opt) {
  lcd_write_nibble(dev, opt | LCD_CMD_FUNCTION_SET);
}

//---------------------------------------
// API Definitions
//---------------------------------------

void lcd_print(const struct device *dev, char *data, uint32_t size) {
  for (int i = 0; i < size; i++) {
    // From the manual, setting RS to high is required for this command.
    lcd_write(dev, data[i], LCD_DEFAULT_OPTIONS | LCD_RS);
  }
}

void lcd_cursor_pos_set(const struct device *dev, uint8_t row, uint8_t col) {
  uint8_t buf = col;
  if (row == 0) {
    buf |= 0x80;
  } else {
    buf |= 0xC0;
  }
  lcd_write(dev, LCD_CMD_SET_DDRAM_ADDR | buf, LCD_DEFAULT_OPTIONS);
}

void lcd_clear(const struct device *dev) {
  lcd_write(dev, LCD_CMD_SCREEN_CLEAR, LCD_DEFAULT_OPTIONS);
  k_msleep(LCD_CMD_CLEAR_TIME_MS);
}

void lcd_display_state_set(const struct device *dev, uint8_t opt) {
  lcd_write(dev, opt | LCD_CMD_DISPLAY_SWITCH, LCD_DEFAULT_OPTIONS);
}

void lcd_input_state_set(const struct device *dev, uint8_t opt) {
  lcd_write(dev, opt | LCD_CMD_INPUT_SET, LCD_DEFAULT_OPTIONS);
}

static int lcd_initialize(const struct device *dev) {
  const struct lcd_config *config = dev->config;
  struct lcd_data *data = dev->data;
  if (!device_is_ready(config->bus.bus)) {
    return -ENODEV;
  }
  k_msleep(LCD_START_SLEEP_TIME_MS);

  // Configure everything for the display function first.
  // 4bit is necessary so we can send the control flags as well.
  // From the man page this can take up to 37us and needs to be sent twice.
  lcd_initial_function_set(dev, LCD_CMD_FUNCTION_SET | LCD_FS_4BIT_MODE);

  // After 4bit is enabled add the lower bits.
  lcd_write(dev, LCD_CMD_FUNCTION_SET | LCD_FS_4BIT_MODE | LCD_FS_ROWS_2 |
                     LCD_FS_DOT_SIZE_LITTLE,
            LCD_DEFAULT_OPTIONS);

  // Now let's configure the display.
  data->backlight_enabled = true;
  lcd_display_state_set(dev, LCD_DS_DISPLAY_ON | LCD_DS_CURSOR_OFF |
                                 LCD_DS_BLINK_OFF);

  // And random animation because I can!
  k_work_queue_start(&display_work_q, display_work_stack_area,
                    K_THREAD_STACK_SIZEOF(display_work_stack_area),
                    6, NULL);
  k_timer_start(&my_timer, K_SECONDS(1), K_MSEC(ANIMATION_SLEEPTIME));
  return 0;
}

//---------------------------------------
// Module Definition
//---------------------------------------
static const struct lcd_config lcd_shared_config = {
  // TODO: See if we can load the right I2C instance from the parent of the LCD
  // via the device tree verse picking only the first one.
    .bus = I2C_DT_SPEC_INST_GET(0),
};
static struct lcd_data lcd_shared_data;

// Must configure this module after the I2C bus (since we are an I2C device),
// so selecting POST_KERNEL initialization order.
DEVICE_DT_INST_DEFINE(0, lcd_initialize, NULL, &lcd_shared_data,
                      &lcd_shared_config, POST_KERNEL,
                      CONFIG_APPLICATION_INIT_PRIORITY, NULL);
