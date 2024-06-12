/*
 * MIT License
 *
 * Copyright (c) 2024 esp32beans@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*
 * For ESP32 S3 Box and ESP32 S3 Box 3
 * Modify ESP_Panel_conf.h to switch between the original Box and the Box 3.
 * Convert capacitive touch screen (x,y) to USB joystick (x,y).
 * The USB joystick works with the Xbox Adaptive Controller as well as Windows.
 */

#include <Arduino.h>
#include <ESP_Panel_Library.h>
#include "USB.h"
#include "ESP32_flight_stick.h"
ESP32_flight_stick FSJoy;

#define EXAMPLE_TOUCH_READ_POINTS_NUM         (5)

ESP_Panel *panel = nullptr;
ESP_PanelTouch *touch = nullptr;
ESP_PanelBacklight *backlight = nullptr;

const int JOY_AXIS_MIN = 0;
const int JOY_AXIS_MAX = 1<<10;
const int JOY_AXIS_MID = JOY_AXIS_MAX / 2;
const int SCREEN_X_MAX = 319;
const int SCREEN_Y_MAX = 239;

void setup() {
  panel = new ESP_Panel();

  /* Initialize bus and device of panel */
  panel->init();
  /* Start panel */
  panel->begin();

  touch = panel->getTouch();
  backlight = panel->getBacklight();

  /* Reduce power consumption by turning screen backlight off */
  if (backlight != nullptr) {
    Serial.println("Turn off the backlight");
    backlight->off();
  } else {
    Serial.println("Backlight is not available");
  }

  if (touch == nullptr) {
    Serial.println("Touch is not available");
    while (1) delay(10);
  }

  FSJoy.begin();
  USB.begin();
}

void touch2joy(ESP_PanelTouchPoint *point) {
  static int x_min = 32;
  static int x_max = SCREEN_X_MAX - 32;
  static int y_min = 32;
  static int y_max = SCREEN_Y_MAX - 32;
  static int x_axis_last = -2;
  static int y_axis_last = -2;
  int x_axis;
  int y_axis;

  if (point != nullptr) {
    if (point->x > x_max) x_max = point->x;
    if (point->x < x_min) x_min = point->x;
    x_axis = map(point->x, x_min, x_max, JOY_AXIS_MIN, JOY_AXIS_MAX);
    if (point->y > y_max) y_max = point->y;
    if (point->y < y_min) y_min = point->y;
    y_axis = map(point->y, y_min, y_max, JOY_AXIS_MIN, JOY_AXIS_MAX);
  } else {
    x_axis = JOY_AXIS_MID;
    y_axis = JOY_AXIS_MID;
  }
  if ((x_axis_last != x_axis) || (y_axis_last != y_axis)) {
    FSJoy.xAxis(x_axis);
    FSJoy.yAxis(y_axis);
    FSJoy.loop();
    x_axis_last = x_axis;
    y_axis_last = y_axis;
  }
}

void loop() {
  ESP_PanelTouchPoint point[EXAMPLE_TOUCH_READ_POINTS_NUM];
  int read_touch_result = touch->readPoints(point, EXAMPLE_TOUCH_READ_POINTS_NUM, -1);

  if (read_touch_result > 0) {
    touch2joy(&point[0]);
  } else if (read_touch_result == 0) {
    touch2joy(nullptr);
  } else {
    Serial.println("Read touch point failed");
    touch2joy(nullptr);
  }
  // Delay for a while to avoid reading too frequently if the interrupt is not enabled
  if (!touch->isInterruptEnabled()) {
    delay(30);
  }
}
