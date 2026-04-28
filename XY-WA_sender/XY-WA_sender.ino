#include <SPI.h>

// ===== Nano <-> XY module wiring =====
// D13 -> SCK (divider)
// D11 -> MOSI (divider)
// D12 <- MISO (direct)
// D10 -> CS   (divider)
// D9  -> RST  (divider)
// D2  <- PKT  (direct)

static const uint8_t PIN_CS  = 10;
static const uint8_t PIN_RST = 9;
static const uint8_t PIN_PKT = 2;

uint8_t RegH = 0, RegL = 0;
uint8_t txBuf[2] = {0x55, 0x66};
uint32_t txCount = 0;
uint32_t ackCount = 0;
uint32_t timeoutCount = 0;

// LT8920 register write: addr + 2 bytes
void writeReg(uint8_t addr, uint8_t hi, uint8_t lo) {
  digitalWrite(PIN_CS, LOW);
  delayMicroseconds(2);
  SPI.transfer(addr);
  SPI.transfer(hi);
  SPI.transfer(lo);
  delayMicroseconds(2);
  digitalWrite(PIN_CS, HIGH);
  delayMicroseconds(2);
}

// LT8920 register read: (addr|0x80) then read 2 bytes
void readReg(uint8_t addr) {
  digitalWrite(PIN_CS, LOW);
  delayMicroseconds(2);
  SPI.transfer(addr | 0x80);
  RegH = SPI.transfer(0xFF);
  RegL = SPI.transfer(0xFF);
  delayMicroseconds(2);
  digitalWrite(PIN_CS, HIGH);
  delayMicroseconds(2);
}

void lt8920Init() {
  // hardware reset
  digitalWrite(PIN_RST, LOW);
  delay(2);
  digitalWrite(PIN_RST, HIGH);
  delay(5);

  // init sequence migrated from vendor STM8 demo (LT8920.c)
  writeReg(0,  0x6f, 0xef);
  writeReg(1,  0x56, 0x81);
  writeReg(2,  0x66, 0x17);
  writeReg(4,  0x9c, 0xc9);
  writeReg(5,  0x66, 0x37);
  writeReg(7,  0x00, 0x00); // channel 2402MHz
  writeReg(8,  0x6c, 0x90);
  writeReg(9,  0x48, 0x00);
  writeReg(10, 0x7f, 0xfd);
  writeReg(11, 0x00, 0x08);
  writeReg(12, 0x00, 0x00);
  writeReg(13, 0x48, 0xbd);
  writeReg(22, 0x00, 0xff);
  writeReg(23, 0x80, 0x05);
  writeReg(24, 0x00, 0x67);
  writeReg(25, 0x16, 0x59);
  writeReg(26, 0x19, 0xe0);
  writeReg(27, 0x13, 0x00);
  writeReg(28, 0x18, 0x00);
  writeReg(32, 0x48, 0x00);
  writeReg(33, 0x3f, 0xc7);
  writeReg(34, 0x20, 0x00);
  writeReg(35, 0x05, 0x00);
  writeReg(36, 0x03, 0x80);
  writeReg(37, 0x03, 0x80);
  writeReg(38, 0x5a, 0x5a);
  writeReg(39, 0x03, 0x80);
  writeReg(40, 0x44, 0x02); // demo checks this reg
  writeReg(41, 0xb8, 0x00);
  writeReg(42, 0xfd, 0xb0);
  writeReg(43, 0x00, 0x0f);
  writeReg(44, 0x10, 0x00);
  writeReg(45, 0x05, 0x52);
  writeReg(50, 0x00, 0x00);

  delay(100);
}

void sendOnePacket() {
  // idle/standby
  writeReg(7,  0x00, 0x00);
  // clear RX/TX FIFO status
  writeReg(52, 0x80, 0x80);

  // FIFO write: length then payload
  writeReg(50, 2, 0);
  writeReg(50, txBuf[0], txBuf[1]);

  // TX mode
  writeReg(7, 0x01, 0x00);

  // wait packet done (PKT goes high in vendor demo)
  bool done = false;
  for (uint16_t i = 0; i < 10000; i++) {
    if (digitalRead(PIN_PKT) == HIGH) {
      done = true;
      break;
    }
    delayMicroseconds(10);
  }

  txCount++;
  if (!done) {
    timeoutCount++;
    return;
  }

  // check auto-ack status
  readReg(52);
  if ((RegH & 0x3F) == 0) {
    ackCount++;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_RST, OUTPUT);
  pinMode(PIN_PKT, INPUT);

  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_RST, HIGH);

  SPI.begin();
  // If no response, try SPI_MODE3 and/or lower speed
  SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));

  lt8920Init();

  // chip presence check from vendor demo
  readReg(40);
  Serial.print(F("Reg40 = 0x"));
  if (RegH < 16) Serial.print('0');
  Serial.print(RegH, HEX);
  if (RegL < 16) Serial.print('0');
  Serial.println(RegL, HEX);

  if (RegH == 0x44 && RegL == 0x02) {
    Serial.println(F("LT8920 init OK"));
  } else {
    Serial.println(F("LT8920 check failed. Verify wiring / SPI mode / divider."));
  }
}

void loop() {
  sendOnePacket();

  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    Serial.print(F("TX=")); Serial.print(txCount);
    Serial.print(F(" ACK=")); Serial.print(ackCount);
    Serial.print(F(" TIMEOUT=")); Serial.println(timeoutCount);
  }

  delay(20); // ~50Hz
}
