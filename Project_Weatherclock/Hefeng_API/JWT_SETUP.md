# QWeather JWT 动态更新说明

已按参数配置：
- `kid = TFWG39JNJ8`
- `sub = 2D88DP9BBU`
- 私钥文件：`auth/ed25519-private.pem`

## 1) 依赖安装（Windows / WSL）
```bash
pip install pyjwt cryptography
```

## 2) 先手动生成一次 token
在 `Hefeng_API` 目录执行：
```bash
python3 scripts/gen_qweather_jwt.py \
  --key auth/ed25519-private.pem \
  --kid TFWG39JNJ8 \
  --sub 2D88DP9BBU \
  --ttl 3000 \
  --out-header qweather_token.h \
  --out-token auth/qweather_token.txt
```

生成后：
- `qweather_token.h`：供 `Hefeng_API.ino` 编译时引用
- `auth/qweather_token.txt`：便于调试查看

## 3) 长时间运行：定时刷新模块
### 方式A：前台循环（简单）
```bash
bash scripts/run_refresh_loop.sh
```
默认每 45 分钟刷新一次，token 有效期 50 分钟。

### 方式B：系统定时任务
可用 Windows 任务计划程序或 Linux cron，每 30~45 分钟运行一次 `gen_qweather_jwt.py`。

## 4) Arduino 代码已接入
`Hefeng_API.ino` 已包含：
```cpp
#include "qweather_token.h"
```
并使用 `Authorization: Bearer <token>`。

> 注意：`qweather_token.h` 是编译期文件。若你刷新了 token，要让设备使用新 token，需要重新编译上传。
> 如果你要“设备在线自动换 token 且不重刷固件”，需要额外做：
> - 设备端本地 Ed25519 签名，或
> - 局域网 token 服务，设备运行时拉取。
