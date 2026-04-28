#include "Wire.h"
#include "I2Cdev.h"
#include "MPU6050.h"

MPU6050 mpu;

int16_t axRaw, ayRaw, azRaw;
int16_t gxRaw, gyRaw, gzRaw;

// 量程缩放（默认配置：±2g, ±250°/s）
const float ACCEL_SCALE = 16384.0f;  // LSB/g
const float GYRO_SCALE  = 131.0f;    // LSB/(°/s)

// 一阶低通滤波参数（0~1，越小越平滑）
const float ACCEL_LPF_ALPHA = 0.15f;

// 互补滤波参数（越接近1越信任陀螺仪）
const float COMP_ALPHA = 0.98f;

// 滤波后的加速度（单位：g）
float ax = 0, ay = 0, az = 0;

// 姿态角（单位：deg）
float pitch = 0, roll = 0, yaw = 0;

unsigned long lastMicros = 0;

void setup() {
  Wire.begin();
  Serial.begin(115200);

  mpu.initialize();

  Serial.println("Initializing MPU6050...");
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("MPU6050 connection successful.");

  // 初始读取一次，避免滤波初值突变
  mpu.getMotion6(&axRaw, &ayRaw, &azRaw, &gxRaw, &gyRaw, &gzRaw);
  ax = axRaw / ACCEL_SCALE;
  ay = ayRaw / ACCEL_SCALE;
  az = azRaw / ACCEL_SCALE;

  // 用加速度估计初始姿态
  roll  = atan2(ay, az) * 180.0f / PI;
  pitch = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0f / PI;
  yaw = 0;

  lastMicros = micros();

  Serial.println("ax(g),ay(g),az(g),pitch(deg),yaw(deg),roll(deg)");
}

void loop() {
  mpu.getMotion6(&axRaw, &ayRaw, &azRaw, &gxRaw, &gyRaw, &gzRaw);

  // 时间间隔（秒）
  unsigned long now = micros();
  float dt = (now - lastMicros) * 1e-6f;
  lastMicros = now;

  // 防止异常 dt
  if (dt <= 0 || dt > 0.1f) {
    dt = 0.01f;
  }

  // 原始值转物理量
  float axNow = axRaw / ACCEL_SCALE;
  float ayNow = ayRaw / ACCEL_SCALE;
  float azNow = azRaw / ACCEL_SCALE;

  float gx = gxRaw / GYRO_SCALE; // deg/s
  float gy = gyRaw / GYRO_SCALE;
  float gz = gzRaw / GYRO_SCALE;

  // 加速度低通滤波（抑制抖动）
  ax = ACCEL_LPF_ALPHA * axNow + (1.0f - ACCEL_LPF_ALPHA) * ax;
  ay = ACCEL_LPF_ALPHA * ayNow + (1.0f - ACCEL_LPF_ALPHA) * ay;
  az = ACCEL_LPF_ALPHA * azNow + (1.0f - ACCEL_LPF_ALPHA) * az;

  // 加速度估计姿态角
  float rollAcc  = atan2(ay, az) * 180.0f / PI;
  float pitchAcc = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0f / PI;

  // 互补滤波融合陀螺仪 + 加速度
  roll  = COMP_ALPHA * (roll  + gx * dt) + (1.0f - COMP_ALPHA) * rollAcc;
  pitch = COMP_ALPHA * (pitch + gy * dt) + (1.0f - COMP_ALPHA) * pitchAcc;

  // yaw 只能靠陀螺仪积分，长期会漂移（无磁力计无法绝对校正）
  yaw += gz * dt;

  // 串口输出
  Serial.print(ax, 4);    Serial.print(',');
  Serial.print(ay, 4);    Serial.print(',');
  Serial.print(az, 4);    Serial.print(',');
  Serial.print(pitch, 2); Serial.print(',');
  Serial.print(yaw, 2);   Serial.print(',');
  Serial.println(roll, 2);

  delay(10); // ~100Hz
}
