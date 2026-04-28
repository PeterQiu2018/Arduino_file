#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include <pgmspace.h>

#include "Weather Icon/01.c"   // 晴
#include "Weather Icon/02.c"   // 晴间多云
#include "Weather Icon/03.c"   // 少云
#include "Weather Icon/04.c"   // 多云
#include "Weather Icon/09.c"   // 雨
#include "Weather Icon/10.c"   // 太阳雨
#include "Weather Icon/11.c"   // 雷雨
#include "Weather Icon/13.c"   // 雪
#include "Weather Icon/50.c"   // 雾/霾/沙尘

extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;

inline bool iconGetPixel(const unsigned char* bitmap, uint8_t x, uint8_t y) {
  const uint16_t bytesPerRow = 64 / 8;
  const uint16_t byteIndex = y * bytesPerRow + (x / 8);
  const uint8_t b = pgm_read_byte(bitmap + byteIndex);
  const uint8_t bitMask = 1U << (x & 0x07);
  return (b & bitMask) != 0;
}

inline void drawIconScaled(int16_t x, int16_t y, const unsigned char* bitmap, uint8_t dstSize = 48) {
  for (uint8_t dy = 0; dy < dstSize; ++dy) {
    // 顺时针旋转90°：inverse map -> srcX = dy, srcY = 63 - dx
    const uint8_t sx = (uint16_t)dy * 64 / dstSize;
    for (uint8_t dx = 0; dx < dstSize; ++dx) {
      const uint8_t syRaw = (uint16_t)dx * 64 / dstSize;
      const uint8_t sy = 63 - syRaw;
      if (iconGetPixel(bitmap, sx, sy)) {
        u8g2.drawPixel(x + dx, y + dy);
      }
    }
  }
}

inline const unsigned char* weatherCodeToBitmap(int code) {
  if (code == 100 || code == 150 || code == 900) return gImage_01;
  if (code == 103 || code == 153) return gImage_02;
  if (code == 102 || code == 152) return gImage_03;
  if (code == 101 || code == 104 || code == 151 || code == 999) return gImage_04;

  if (code == 309) return gImage_10;
  if (code >= 302 && code <= 304) return gImage_11;
  if (code == 901 || (code >= 400 && code <= 499)) return gImage_13;
  if (code >= 500 && code <= 515) return gImage_50;

  if (code == 300 || code == 301) return gImage_09;
  if (code >= 305 && code <= 308) return gImage_09;
  if (code >= 310 && code <= 399) return gImage_09;

  return gImage_04;  // 其他未知兜底
}

inline void drawWeatherIcon(int x, int y, int code) {
  const unsigned char* bmp = weatherCodeToBitmap(code);
  drawIconScaled(x, y, bmp, 48);
}
