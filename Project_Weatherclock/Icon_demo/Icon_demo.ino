#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "Weather Icon/01.c"
#include "Weather Icon/02.c"
#include "Weather Icon/03.c"
#include "Weather Icon/04.c"
#include "Weather Icon/09.c"
#include "Weather Icon/10.c"
#include "Weather Icon/11.c"
#include "Weather Icon/13.c"
#include "Weather Icon/50.c"

#define I2C_SDA_PIN 32
#define I2C_SCL_PIN 33

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

namespace {

constexpr uint8_t kScreenWidth = 128;
constexpr uint8_t kScreenHeight = 64;
constexpr uint8_t kCaptionBarHeight = 12;
constexpr uint8_t kSourceIconSize = 64;
constexpr uint8_t kDisplayIconSize = 48;
constexpr uint32_t kSwitchIntervalMs = 6000;

struct IconEntry {
  const char* fileName;
  const uint8_t* bitmap;
};

const IconEntry kIcons[] = {
    {"01.c", gImage_01},
    {"02.c", gImage_02},
    {"03.c", gImage_03},
    {"04.c", gImage_04},
    {"09.c", gImage_09},
    {"10.c", gImage_10},
    {"11.c", gImage_11},
    {"13.c", gImage_13},
    {"50.c", gImage_50},
};

size_t currentIconIndex = 0;
unsigned long lastSwitchAt = 0;

bool getBitmapPixel(const uint8_t* bitmap, uint8_t x, uint8_t y) {
  const uint16_t bytesPerRow = kSourceIconSize / 8;
  const uint16_t byteIndex = y * bytesPerRow + (x / 8);
  const uint8_t bitMask = 1U << (x & 0x07);
  return (bitmap[byteIndex] & bitMask) != 0;
}

void drawScaledBitmap(int16_t x, int16_t y, const uint8_t* bitmap) {
  for (uint8_t dy = 0; dy < kDisplayIconSize; ++dy) {
    const uint8_t sy = (uint16_t)dy * kSourceIconSize / kDisplayIconSize;
    for (uint8_t dx = 0; dx < kDisplayIconSize; ++dx) {
      const uint8_t sx = (uint16_t)dx * kSourceIconSize / kDisplayIconSize;
      if (getBitmapPixel(bitmap, sx, sy)) {
        u8g2.drawPixel(x + dx, y + dy);
      }
    }
  }
}

void drawCaptionBar(const char* text) {
  const int16_t barY = kScreenHeight - kCaptionBarHeight;

  u8g2.drawBox(0, barY, kScreenWidth, kCaptionBarHeight);
  u8g2.setDrawColor(0);
  u8g2.setFont(u8g2_font_6x10_tf);

  int16_t textWidth = u8g2.getStrWidth(text);
  int16_t textX = (kScreenWidth - textWidth) / 2;
  if (textX < 0) {
    textX = 0;
  }

  u8g2.drawStr(textX, kScreenHeight - 2, text);
  u8g2.setDrawColor(1);
}

void renderIconPage(size_t iconIndex) {
  const IconEntry& icon = kIcons[iconIndex];
  const int16_t iconX = (kScreenWidth - kDisplayIconSize) / 2;
  const int16_t iconY = 2;

  u8g2.clearBuffer();
  u8g2.drawFrame(iconX - 2, iconY - 2, kDisplayIconSize + 4, kDisplayIconSize + 4);
  drawScaledBitmap(iconX, iconY, icon.bitmap);
  drawCaptionBar(icon.fileName);
  u8g2.sendBuffer();
}

}  // namespace

void setup() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  u8g2.begin();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);

  renderIconPage(currentIconIndex);
  lastSwitchAt = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSwitchAt < kSwitchIntervalMs) {
    return;
  }

  lastSwitchAt = now;
  currentIconIndex = (currentIconIndex + 1) % (sizeof(kIcons) / sizeof(kIcons[0]));
  renderIconPage(currentIconIndex);
}
