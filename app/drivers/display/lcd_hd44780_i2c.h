/*
 * Copyright (c) 2022
 *
 * SPDX-License-Identifier: Apache-2.0
 * NOTICE: API derived from Zephyr Project's Grove LCD Driver:
 *      zephyr/include/zephyr/drivers/misc/grove_lcd/grove_lcd.h
 */

#ifndef LCD_HD44780_H_
#define LCD_HD44780_H_

#include <stdint.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_NUM_COLS 16

/**
 * @brief Display Drivers
 * @defgroup display_interfaces Display Drivers
 * @{
 * @}
 */

/**
 * @brief HD44780 display APIs
 * @defgroup HD44780_display HD44780 display APIs
 * @ingroup display_interfaces
 * @{
 */

/* Defines for custom characters. */
#define LCD_CHAR_MU 0xE4
#define LCD_CHAR_IMAG 0xEA
#define LCD_CHAR_SQRT 0xE8
#define LCD_CHAR_INV 0xE9
#define LCD_CHAR_INF 0xF3
#define LCD_CHAR_PI 0xF7
#define LCD_CHAR_DIV 0xFD
/**
 *  @brief Send text to the screen
 *
 *  @param dev Pointer to device structure for driver instance.
 *  @param data the ASCII text to display
 *  @param size the length of the text in bytes
 */
void lcd_print(const struct device *dev, char *data, uint32_t size);

/**
 *  @brief Set text cursor position for next additions
 *
 *  @param dev Pointer to device structure for driver instance.
 *  @param row the row it should be moved to (0 or 1)
 *  @param col the column for the cursor to be moved to (0-15)
 */
void lcd_cursor_pos_set(const struct device *dev, uint8_t row, uint8_t col);

/**
 *  @brief Clear the current display
 *
 *  @param dev Pointer to device structure for driver instance.
 */
void lcd_clear(const struct device *dev);

/* Defines for the LCD_CMD_DISPLAY_SWITCH options */
#define LCD_DS_DISPLAY_ON		(1 << 2)
#define LCD_DS_DISPLAY_OFF		(0 << 2)
#define LCD_DS_CURSOR_ON		(1 << 1)
#define LCD_DS_CURSOR_OFF		(0 << 1)
#define LCD_DS_BLINK_ON		    (1 << 0)
#define LCD_DS_BLINK_OFF		(0 << 0)
/**
 *  @brief Function to change the display state.
 *  @details This function provides the user the ability to change the state
 *  of the display as per needed. Controlling things like powering on or off
 *  the screen, the option to display the cursor or not, and the ability to
 *  blink the cursor.
 *
 *  @param dev Pointer to device structure for driver instance.
 *  @param opt An 8bit bitmask of LCD_DS_* options.
 *
 */
void lcd_display_state_set(const struct device *dev, uint8_t opt);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* LCD_HD44780_H_ */
