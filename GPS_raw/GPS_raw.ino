#include <Wire.h>
#include <SoftwareSerial.h>
#include <stdlib.h>
#include <U8x8lib.h>

// GPS: Nano D4 <- GPS TXD, D3 -> GPS RXD(可选)
SoftwareSerial gpsSerial(4, 3);  // RX, TX

// ⚠️ 你的是 SH1106 128x64 I2C
U8X8_SH1106_128X64_NONAME_HW_I2C oled(U8X8_PIN_NONE);

char lineBuf[96];
uint8_t linePos = 0;

bool locValid = false;
int fixQuality = -1;
int satellites = 0;
float speedKmph = 0.0;
float latDec = 0.0;
float lonDec = 0.0;
float altitudeM = 0.0;
float headingDeg = 0.0;
int utcHour = -1;
int utcMin = -1;
int utcSec = -1;

unsigned long lastScreenMs = 0;
unsigned long lastDebugMs = 0;

float nmeaToDecimal(const char* s) {
  if (!s || !*s) return 0.0;
  float v = atof(s);
  int deg = (int)(v / 100);
  float minutes = v - deg * 100;
  return deg + minutes / 60.0;
}

void splitCSV(char* str, char* fields[], int maxFields, int& count) {
  count = 0;
  fields[count++] = str;
  for (char* p = str; *p && count < maxFields; ++p) {
    if (*p == ',') {
      *p = '\0';
      fields[count++] = p + 1;
    } else if (*p == '*') {
      *p = '\0';
      break;
    }
  }
}

const char* headingText(float deg) {
  while (deg < 0) deg += 360.0f;
  while (deg >= 360.0f) deg -= 360.0f;

  if (deg >= 337.5f || deg < 22.5f) return "N";
  if (deg < 67.5f) return "NE";
  if (deg < 112.5f) return "E";
  if (deg < 157.5f) return "SE";
  if (deg < 202.5f) return "S";
  if (deg < 247.5f) return "SW";
  if (deg < 292.5f) return "W";
  return "NW";
}

void parseRMC(char* s) {
  char* f[20];
  int n = 0;
  splitCSV(s, f, 20, n);
  if (n < 8) return;

  locValid = (f[2][0] == 'A');

  if (f[3][0] && f[4][0] && f[5][0] && f[6][0]) {
    float la = nmeaToDecimal(f[3]);
    float lo = nmeaToDecimal(f[5]);
    if (f[4][0] == 'S') la = -la;
    if (f[6][0] == 'W') lo = -lo;
    latDec = la;
    lonDec = lo;
  }

  if (f[1][0] && strlen(f[1]) >= 6) {
    int h = (f[1][0] - '0') * 10 + (f[1][1] - '0');
    int m = (f[1][2] - '0') * 10 + (f[1][3] - '0');
    int s2 = (f[1][4] - '0') * 10 + (f[1][5] - '0');
    h = (h + 8) % 24;
    utcHour = h;
    utcMin = m;
    utcSec = s2;
  }

  if (f[7][0]) {
    float knots = atof(f[7]);
    speedKmph = knots * 1.852f;
  }

  if (f[8][0]) {
    headingDeg = atof(f[8]);
  }
}

void parseGGA(char* s) {
  char* f[20];
  int n = 0;
  splitCSV(s, f, 20, n);
  if (n < 10) return;

  if (f[6][0]) fixQuality = atoi(f[6]);
  if (f[7][0]) satellites = atoi(f[7]);
  if (f[9][0]) altitudeM = atof(f[9]);

  if (fixQuality > 0) locValid = true;

  if (f[2][0] && f[3][0] && f[4][0] && f[5][0]) {
    float la = nmeaToDecimal(f[2]);
    float lo = nmeaToDecimal(f[4]);
    if (f[3][0] == 'S') la = -la;
    if (f[5][0] == 'W') lo = -lo;
    latDec = la;
    lonDec = lo;
  }
}

const char* fixText(int q) {
  switch (q) {
    case 0: return "NO FIX";
    case 1: return "GPS";
    case 2: return "DGPS";
    case 4: return "RTK FIX";
    case 5: return "RTK FLT";
    default: return "WAIT";
  }
}

void formatFloat(char* out, size_t outSize, float value, uint8_t width, uint8_t prec) {
  char tmp[20];
  dtostrf(value, width, prec, tmp);

  char* p = tmp;
  while (*p == ' ') p++;

  strncpy(out, p, outSize - 1);
  out[outSize - 1] = '\0';
}

void drawLinePadded(uint8_t row, const char* text) {
  char buf[17];
  snprintf(buf, sizeof(buf), "%-16s", text);  // 固定16列，避免残影
  oled.drawString(0, row, buf);
}

void drawScreen() {
  char b[24];
  char num[20];

  snprintf(b, sizeof(b), "FIX:%s S:%d", fixText(fixQuality), satellites);
  drawLinePadded(0, b);

  formatFloat(num, sizeof(num), speedKmph, 0, 1);
  snprintf(b, sizeof(b), "SPD:%skm/h", num);
  drawLinePadded(1, b);

  if (fixQuality > 0) {
    formatFloat(num, sizeof(num), latDec, 0, 6);
    snprintf(b, sizeof(b), "LAT:%s", num);
    drawLinePadded(2, b);

    formatFloat(num, sizeof(num), lonDec, 0, 6);
    snprintf(b, sizeof(b), "LON:%s", num);
    drawLinePadded(3, b);

    formatFloat(num, sizeof(num), altitudeM, 0, 1);
    snprintf(b, sizeof(b), "ALT:%sm", num);
    drawLinePadded(4, b);
  } else {
    drawLinePadded(2, "LAT: ----");
    drawLinePadded(3, "LON: ----");
    drawLinePadded(4, "ALT: ----");
  }

  snprintf(b, sizeof(b), "HEADING:%s %d", headingText(headingDeg), (int)(headingDeg + 0.5f));
  drawLinePadded(5, b);

  if (utcHour >= 0) {
    snprintf(b, sizeof(b), "TIME:%02d:%02d:%02d", utcHour, utcMin, utcSec);
  } else {
    snprintf(b, sizeof(b), "TIME:--:--:--");
  }
  drawLinePadded(6, b);

  drawLinePadded(7, "");
}

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);

  Wire.begin();
  Wire.setClock(100000);
  delay(80);

  oled.begin();
  oled.setI2CAddress(0x3C << 1);
  oled.setPowerSave(0);
  oled.setFont(u8x8_font_chroma48medium8_r);

  drawLinePadded(0, "Bike Computer");
  drawLinePadded(1, "SH1106 Ready");
  drawLinePadded(2, "Waiting NMEA...");

  Serial.println("GPS bike computer started (SH1106)");
}

void loop() {
  while (gpsSerial.available()) {
    char c = (char)gpsSerial.read();
    Serial.write(c);

    if (c == '\n' || c == '\r') {
      if (linePos > 0) {
        lineBuf[linePos] = '\0';
        if (strncmp(lineBuf, "$GNRMC", 6) == 0 || strncmp(lineBuf, "$GPRMC", 6) == 0) {
          parseRMC(lineBuf);
        } else if (strncmp(lineBuf, "$GNGGA", 6) == 0 || strncmp(lineBuf, "$GPGGA", 6) == 0) {
          parseGGA(lineBuf);
        }
        linePos = 0;
      }
    } else {
      if (linePos < sizeof(lineBuf) - 1) lineBuf[linePos++] = c;
      else linePos = 0;
    }
  }

  // 降刷新频率，减少闪烁
  if (millis() - lastScreenMs >= 1000) {
    lastScreenMs = millis();
    drawScreen();
  }

  if (millis() - lastDebugMs >= 1000) {
    lastDebugMs = millis();
    Serial.print("FIX=");
    Serial.print(locValid ? "A" : "V");
    Serial.print(" Q=");
    Serial.print(fixQuality);
    Serial.print(" SAT=");
    Serial.print(satellites);
    Serial.print(" SPD=");
    Serial.print(speedKmph, 1);
    Serial.print(" LAT=");
    Serial.print(latDec, 6);
    Serial.print(" LON=");
    Serial.print(lonDec, 6);
    Serial.print(" ALT=");
    Serial.print(altitudeM, 1);
    Serial.print(" HEADING=");
    Serial.print(headingText(headingDeg));
    Serial.print(' ');
    Serial.print(headingDeg, 1);
    Serial.print(" TIME=");
    if (utcHour >= 0) {
      if (utcHour < 10) Serial.print('0');
      Serial.print(utcHour);
      Serial.print(':');
      if (utcMin < 10) Serial.print('0');
      Serial.print(utcMin);
      Serial.print(':');
      if (utcSec < 10) Serial.print('0');
      Serial.println(utcSec);
    } else {
      Serial.println("--:--:--");
    }
  }
}
