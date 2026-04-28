#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"

MPU6050 mpu;

int16_t axRaw, ayRaw, azRaw;
int16_t gxRaw, gyRaw, gzRaw;

// ==================== MPU6050 参数 ====================
const float ACCEL_SCALE = 16384.0f;  // ±2g, LSB/g
const float GYRO_SCALE  = 131.0f;    // ±250°/s, LSB/(°/s)
const float ACCEL_LPF_ALPHA = 0.15f; // 0~1，越小越平滑
const float COMP_ALPHA      = 0.98f; // 互补滤波，越大越信任陀螺仪

// 滤波后的加速度（g）
float ax = 0, ay = 0, az = 0;

// 姿态角（deg）
float pitch = 0, roll = 0, yaw = 0;

unsigned long lastMicros = 0;

// ==================== PPM 参数（Trainer） ====================
// 默认基于 AVR UNO/Nano（ATmega328P），输出在 D9（PB1）
#define PPM_PIN 9
#define CHANNEL_NUMBER 3
#define PPM_FRAME_LENGTH_US 22500  // 常见 22.5ms
#define PPM_PULSE_LENGTH_US 300    // 脉冲宽度 300us
#define PPM_INVERTED 0             // 0: 正常极性；1: 反相极性

volatile uint16_t ppm[CHANNEL_NUMBER] = {
  1500, 1500, 1500
};

// 角度映射范围（可按你的手感改）
const float ROLL_RANGE_DEG  = 45.0f;  // roll ±45° -> 1000~2000us
const float PITCH_RANGE_DEG = 45.0f;  // pitch ±45° -> 1000~2000us
const float YAW_RANGE_DEG   = 60.0f;  // yaw ±60° -> 1000~2000us

// 死区，防止微抖动导致通道来回跳
const float ANGLE_DEAD_BAND_DEG = 1.0f;

inline uint16_t angleToPpm(float angleDeg, float rangeDeg) {
  if (fabs(angleDeg) < ANGLE_DEAD_BAND_DEG) angleDeg = 0;
  angleDeg = constrain(angleDeg, -rangeDeg, rangeDeg);
  float normalized = angleDeg / rangeDeg;      // -1 ~ 1
  float us = 1500.0f + normalized * 500.0f;    // 1000 ~ 2000
  return (uint16_t)constrain((int)us, 1000, 2000);
}

void setupPPM() {
  pinMode(PPM_PIN, OUTPUT);

#if PPM_INVERTED
  digitalWrite(PPM_PIN, HIGH);
#else
  digitalWrite(PPM_PIN, LOW);
#endif

  cli();

  // Timer1 CTC 模式，分频 8 => 0.5us/tick
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);  // CTC
  TCCR1B |= (1 << CS11);   // prescaler = 8

  // 首次比较值：1ms
  OCR1A = 2000;
  TIMSK1 |= (1 << OCIE1A);

  sei();
}

ISR(TIMER1_COMPA_vect) {
  static boolean state = true;
  static uint8_t cur_chan_numb = 0;
  static uint16_t calc_rest = 0;

  TCNT1 = 0;

  if (state) {
    // 输出固定脉冲段
#if PPM_INVERTED
    digitalWrite(PPM_PIN, LOW);
#else
    digitalWrite(PPM_PIN, HIGH);
#endif
    OCR1A = PPM_PULSE_LENGTH_US * 2; // 0.5us/tick
    state = false;
  } else {
    // 输出通道间隔段（或帧同步段）
#if PPM_INVERTED
    digitalWrite(PPM_PIN, HIGH);
#else
    digitalWrite(PPM_PIN, LOW);
#endif
    state = true;

    if (cur_chan_numb >= CHANNEL_NUMBER) {
      cur_chan_numb = 0;
      calc_rest += PPM_PULSE_LENGTH_US;
      OCR1A = (PPM_FRAME_LENGTH_US - calc_rest) * 2;
      calc_rest = 0;
    } else {
      OCR1A = (ppm[cur_chan_numb] - PPM_PULSE_LENGTH_US) * 2;
      calc_rest += ppm[cur_chan_numb];
      cur_chan_numb++;
    }
  }
}

void setup() {
  Wire.begin();
  Serial.begin(115200);

  mpu.initialize();

  Serial.println("Initializing MPU6050...");
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1) delay(1000);
  }
  Serial.println("MPU6050 connection successful.");

  // 初始读取一次，避免滤波初值突变
  mpu.getMotion6(&axRaw, &ayRaw, &azRaw, &gxRaw, &gyRaw, &gzRaw);
  ax = axRaw / ACCEL_SCALE;
  ay = ayRaw / ACCEL_SCALE;
  az = azRaw / ACCEL_SCALE;

  // 初始姿态
  roll  = atan2(ay, az) * 180.0f / PI;
  pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0f / PI;
  yaw = 0;

  lastMicros = micros();

  setupPPM();

  Serial.println("roll(deg),pitch(deg),yaw(deg),ch1(roll),ch2(pitch),ch3(yaw)");
}

void loop() {
  mpu.getMotion6(&axRaw, &ayRaw, &azRaw, &gxRaw, &gyRaw, &gzRaw);

  // 时间间隔（秒）
  unsigned long now = micros();
  float dt = (now - lastMicros) * 1e-6f;
  lastMicros = now;

  if (dt <= 0 || dt > 0.1f) dt = 0.01f;

  // 原始值转物理量
  float axNow = axRaw / ACCEL_SCALE;
  float ayNow = ayRaw / ACCEL_SCALE;
  float azNow = azRaw / ACCEL_SCALE;

  float gx = gxRaw / GYRO_SCALE;
  float gy = gyRaw / GYRO_SCALE;
  float gz = gzRaw / GYRO_SCALE;

  // 加速度低通滤波
  ax = ACCEL_LPF_ALPHA * axNow + (1.0f - ACCEL_LPF_ALPHA) * ax;
  ay = ACCEL_LPF_ALPHA * ayNow + (1.0f - ACCEL_LPF_ALPHA) * ay;
  az = ACCEL_LPF_ALPHA * azNow + (1.0f - ACCEL_LPF_ALPHA) * az;

  // 加速度姿态
  float rollAcc  = atan2(ay, az) * 180.0f / PI;
  float pitchAcc = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0f / PI;

  // 互补滤波
  roll  = COMP_ALPHA * (roll  + gx * dt) + (1.0f - COMP_ALPHA) * rollAcc;
  pitch = COMP_ALPHA * (pitch + gy * dt) + (1.0f - COMP_ALPHA) * pitchAcc;

  // yaw 仅陀螺积分（会漂移）
  yaw += gz * dt;

  // 映射到 PPM 通道
  uint16_t chRoll  = angleToPpm(roll,  ROLL_RANGE_DEG);  // CH1
  uint16_t chPitch = angleToPpm(pitch, PITCH_RANGE_DEG); // CH2
  uint16_t chYaw   = angleToPpm(yaw,   YAW_RANGE_DEG);   // CH3

  noInterrupts();
  ppm[0] = chRoll;
  ppm[1] = chPitch;
  ppm[2] = chYaw;
  interrupts();

  // 串口调试
  Serial.print(roll, 2);  Serial.print(',');
  Serial.print(pitch, 2); Serial.print(',');
  Serial.print(yaw, 2);   Serial.print(',');
  Serial.print(chRoll);   Serial.print(',');
  Serial.print(chPitch);  Serial.print(',');
  Serial.println(chYaw);

  delay(10); // ~100Hz 更新姿态
}
