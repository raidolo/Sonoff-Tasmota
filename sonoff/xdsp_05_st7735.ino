/*
  xdsp_04_ST7735.ino - Display Tft ST7735 support for Sonoff-Tasmota

  Copyright (C) 2018  Theo Arends and Adafruit

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_SPI
#ifdef USE_DISPLAY
#ifdef USE_DISPLAY_ST7735

#define XDSP_04                4

#define TFT_TOP                16
#define TFT_BOTTOM             16
#define TFT_FONT_WIDTH         6
#define TFT_FONT_HEIGTH        8     // Adafruit minimal font heigth pixels

#define TFT_RST   -1

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>        // TFT 320x240 and 480x320

Adafruit_ST7735 *tft;

uint16_t tft_scroll;

/*********************************************************************************************/

void ST7735InitMode()
{
  tft->setRotation(Settings.display_rotate);  // 0
  tft->invertDisplay(0);
  tft->fillScreen(ST7735_BLACK);
  tft->setTextWrap(false);         // Allow text to run off edges
  tft->cp437(true);
  if (!Settings.display_mode) {
    tft->setCursor(0, 0);
    tft->setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft->setTextSize(1);
  } else {
    tft->setScrollMargins(TFT_TOP, TFT_BOTTOM);  // NON PRESENTE SU ST7735
    tft->setCursor(0, 0);
    tft->setTextColor(ST7735_YELLOW, ST7735_BLACK);
    tft->setTextSize(2);
//    tft->println("HEADER");

    tft_scroll = TFT_TOP;
  }
}

void ST7735Init(uint8_t mode)
{
  switch(mode) {
    case DISPLAY_INIT_MODE:
      ST7735InitMode();
#ifdef USE_DISPLAY_MODES1TO5
      if (Settings.display_rotate) {
        DisplayClearScreenBuffer();
      }
#endif  // USE_DISPLAY_MODES1TO5
      break;
    case DISPLAY_INIT_PARTIAL:
    case DISPLAY_INIT_FULL:
      break;
  }
}

void ST7735InitDriver()
{
  if (!Settings.display_model) {
    Settings.display_model = XDSP_04;
  }

  if (XDSP_04 == Settings.display_model) {
    tft = new Adafruit_ST7735(pin[GPIO_SPI_CS], pin[GPIO_SPI_DC], TFT_RST);
    tft->initR(INITR_BLACKTAB);

#ifdef USE_DISPLAY_MODES1TO5
    if (Settings.display_rotate) {
      DisplayAllocScreenBuffer();
    }
#endif  // USE_DISPLAY_MODES1TO5

    ST7735InitMode();
  }
}

void ST7735Clear()
{
  tft->fillScreen(ST7735_BLACK);
  tft->setCursor(0, 0);
}

void ST7735DrawStringAt(uint16_t x, uint16_t y, char *str, uint16_t color, uint8_t flag)
{
  uint16_t active_color = ST7735_WHITE;

  tft->setTextSize(Settings.display_size);
  if (!flag) {
    tft->setCursor(x, y);
  } else {
    tft->setCursor((x-1) * TFT_FONT_WIDTH * Settings.display_size, (y-1) * TFT_FONT_HEIGTH * Settings.display_size);
  }
  if (color) { active_color = color; }
  tft->setTextColor(active_color, ST7735_BLACK);
  tft->println(str);
}

void ST7735DisplayOnOff(uint8_t on)
{
//  tft->showDisplay(on);
//  tft->invertDisplay(on);
  if (pin[GPIO_BACKLIGHT] < 99) {
    pinMode(pin[GPIO_BACKLIGHT], OUTPUT);
    digitalWrite(pin[GPIO_BACKLIGHT], on);
  }
}

void ST7735OnOff()
{
  ST7735DisplayOnOff(disp_power);

}

/*********************************************************************************************/

#ifdef USE_DISPLAY_MODES1TO5

void ST7735PrintLog()
{
  disp_refresh--;
  if (!disp_refresh) {
    disp_refresh = Settings.display_refresh;
    if (Settings.display_rotate) {
      if (!disp_screen_buffer_cols) { DisplayAllocScreenBuffer(); }
    }

    char* txt = DisplayLogBuffer('\370');

    if (txt != NULL) {
      byte size = Settings.display_size;
      uint16_t theight = size * TFT_FONT_HEIGTH;

      tft->setTextSize(size);
      tft->setTextColor(ST7735_CYAN, ST7735_BLACK);  // Add background color to solve flicker
      if (!Settings.display_rotate) {  // Use hardware scroll
        tft->setCursor(0, tft_scroll);
        tft->fillRect(0, tft_scroll, tft->width(), theight, ST7735_GREEN);  // Erase line
        tft->print(txt);
        tft_scroll += theight;
        if (tft_scroll >= (tft->height() - TFT_BOTTOM)) {
          tft_scroll = TFT_TOP;
        }
        tft->scrollTo(tft_scroll);   //FUNZIONE Aggiunta SU ST7735
      } else {
        uint8_t last_row = Settings.display_rows -1;

        tft_scroll = theight;  // Start below header
        tft->setCursor(0, tft_scroll);
        for (byte i = 0; i < last_row; i++) {
          strlcpy(disp_screen_buffer[i], disp_screen_buffer[i +1], disp_screen_buffer_cols);
//          tft->fillRect(0, tft_scroll, tft->width(), theight, ST7735_BLACK);  // Erase line
          tft->print(disp_screen_buffer[i]);
          tft_scroll += theight;
          tft->setCursor(0, tft_scroll);
        }
        strlcpy(disp_screen_buffer[last_row], txt, disp_screen_buffer_cols);
        DisplayFillScreen(last_row);
        tft->print(disp_screen_buffer[last_row]);
      }
      snprintf_P(log_data, sizeof(log_data), PSTR(D_LOG_APPLICATION "[%s]"), txt);
      AddLog(LOG_LEVEL_DEBUG);
    }
  }
}

void ST7735Refresh()  // Every second
{
  if (Settings.display_mode) {  // Mode 0 is User text
    char tftdt[Settings.display_cols[0] +1];
    char date4[11];  // 24-04-2017
    char space[Settings.display_cols[0] - 17];
    char time[9];    // 13:45:43

    tft->setTextSize(1);
    tft->setTextColor(ST7735_YELLOW, ST7735_RED);   // Add background color to solve flicker
    tft->setCursor(0, 0);

    snprintf_P(date4, sizeof(date4), PSTR("%02d" D_MONTH_DAY_SEPARATOR "%02d" D_YEAR_MONTH_SEPARATOR "%04d"), RtcTime.day_of_month, RtcTime.month, RtcTime.year);
    memset(space, 0x20, sizeof(space));
    space[sizeof(space) -1] = '\0';
    snprintf_P(time, sizeof(time), PSTR("%02d" D_HOUR_MINUTE_SEPARATOR "%02d" D_MINUTE_SECOND_SEPARATOR "%02d"), RtcTime.hour, RtcTime.minute, RtcTime.second);
    snprintf_P(tftdt, sizeof(tftdt), PSTR("%s%s%s"), date4, space, time);

    tft->print(tftdt);

    switch (Settings.display_mode) {
      case 1:  // Text
      case 2:  // Local
      case 3:  // Local
      case 4:  // Mqtt
      case 5:  // Mqtt
        ST7735PrintLog();
        break;
    }
  }
}

#endif  // USE_DISPLAY_MODES1TO5

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

boolean Xdsp04(byte function)
{
  boolean result = false;

  if (spi_flg) {
    if (FUNC_DISPLAY_INIT_DRIVER == function) {
      ST7735InitDriver();
    }
    else if (XDSP_04 == Settings.display_model) {

      if (!dsp_color) { dsp_color = ST7735_WHITE; }

      switch (function) {
        case FUNC_DISPLAY_MODEL:
          result = true;
          break;
        case FUNC_DISPLAY_INIT:
          ST7735Init(dsp_init);
          break;
        case FUNC_DISPLAY_POWER:
          ST7735OnOff();
          break;
        case FUNC_DISPLAY_CLEAR:
          ST7735Clear();
          break;
        case FUNC_DISPLAY_DRAW_HLINE:
          tft->writeFastHLine(dsp_x, dsp_y, dsp_len, dsp_color);
          Serial.println("drawHline " + String(dsp_len) + "color " + String(dsp_color) );
          break;
        case FUNC_DISPLAY_DRAW_VLINE:
          tft->writeFastVLine(dsp_x, dsp_y, dsp_len, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_LINE:
          tft->writeLine(dsp_x, dsp_y, dsp_x2, dsp_y2, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_CIRCLE:
          tft->drawCircle(dsp_x, dsp_y, dsp_rad, dsp_color);
          break;
        case FUNC_DISPLAY_FILL_CIRCLE:
          tft->fillCircle(dsp_x, dsp_y, dsp_rad, dsp_color);
          break;
        case FUNC_DISPLAY_DRAW_RECTANGLE:
          tft->drawRect(dsp_x, dsp_y, dsp_x2, dsp_y2, dsp_color);
          break;
        case FUNC_DISPLAY_FILL_RECTANGLE:
          tft->fillRect(dsp_x, dsp_y, dsp_x2, dsp_y2, dsp_color);
          break;
//        case FUNC_DISPLAY_DRAW_FRAME:
//          oled->display();
//          break;
        case FUNC_DISPLAY_TEXT_SIZE:
          tft->setTextSize(Settings.display_size);
          break;
        case FUNC_DISPLAY_FONT_SIZE:
//          tft->setTextSize(Settings.display_font);
          break;
        case FUNC_DISPLAY_DRAW_STRING:
          ST7735DrawStringAt(dsp_x, dsp_y, dsp_str, dsp_color, dsp_flag);
          break;
        case FUNC_DISPLAY_ONOFF:
          ST7735DisplayOnOff(dsp_on);
          break;
        case FUNC_DISPLAY_ROTATION:
          tft->setRotation(Settings.display_rotate);
          break;
#ifdef USE_DISPLAY_MODES1TO5
        case FUNC_DISPLAY_EVERY_SECOND:
          ST7735Refresh();
          break;
#endif  // USE_DISPLAY_MODES1TO5
      }
    }
  }
  return result;
}

#endif  // USE_DISPLAY_ST7735
#endif  // USE_DISPLAY
#endif  // USE_SPI
