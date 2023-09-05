#define BOUNCE_WITH_PROMPT_DETECTION  // 启用带有立即检测的弹跳
// 需要安装以下四个库
#include <ESP32Encoder.h>  // 用于编码器的库 https://github.com/madhephaestus/ESP32Encoder
#include <Bounce2.h>       // 弹跳处理库 https://github.com/thomasfredericks/Bounce2
#include <BleGamepad.h>    // 用于创建BLE游戏手柄的库 https://github.com/lemmingDev/ESP32-BLE-Gamepad
// NimBLE

ESP32Encoder encoder1;       // 创建编码器1对象
ESP32Encoder encoder2;       // 创建编码器2对象
int32_t encoder1Count = 0;   // 用于存储编码器1的当前计数值
int32_t encoder2Count = 0;   // 用于存储编码器2的当前计数值
#define ENC1_LeftButton 5    // 定义编码器1左侧按钮的标识
#define ENC1_RightButton 6   // 定义编码器1右侧按钮的标识
#define ENC2_LeftButton 15   // 定义编码器2左侧按钮的标识
#define ENC2_RightButton 16  // 定义编码器2右侧按钮的标识
#define numOfButtons 16      // 定义按钮的总数

Bounce debouncers[numOfButtons];                                                                   // 创建弹跳对象数组，用于处理按钮弹跳
BleGamepad bleGamepad("TiHUB", "Tinyboxxx", 100);                                                  // 创建BLE游戏手柄对象，可以自定义设备名称、制造商名称和初始电池电量
byte buttonPins[numOfButtons] = { 33, 25, 14, 39, 32, 34, 35, 36, 2, 5, 17, 18, 21, 19, 22, 23 };  // 每个按钮所连接的实际IO引脚
// 编码器1连接到引脚26和27，编码器2连接到引脚4和16
byte physicalButtons[numOfButtons] = { 1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20 };  // 在计算机上虚拟显示的按钮顺序，按钮5、6、15、16代表旋转编码器
int counter_for_battery_ADC = 0;                                                                   // 用于计数ADC转换的计数器
int battery_level = 100;                                                                           // 存储电池电量的变量，默认为100%
int battery_voltage_ADC = 0;                                                                       // 存储电池电压的ADC值的变量
float battery_voltage = 0;                                                                         // 存储电池电压的变量，以伏特为单位

void setup() {
  ESP32Encoder::useInternalWeakPullResistors = UP;  // 启用ESP32的弱上拉电阻
  encoder1.attachHalfQuad(26, 27);                  // 将编码器1连接到引脚26和27
  encoder2.attachHalfQuad(4, 16);                   // 将编码器2连接到引脚4和16
  encoder1.clearCount();                            // 清除编码器1的计数值
  encoder2.clearCount();                            // 清除编码器2的计数值
  encoder1Count = encoder1.getCount();              // 初始化编码器1的计数值
  encoder2Count = encoder2.getCount();              // 初始化编码器2的计数值

  for (byte currentPinIndex = 0; currentPinIndex < numOfButtons; currentPinIndex++) {
    pinMode(buttonPins[currentPinIndex], INPUT_PULLUP);  // 配置按钮引脚为输入模式，并启用上拉电阻
    debouncers[currentPinIndex] = Bounce();
    debouncers[currentPinIndex].attach(buttonPins[currentPinIndex]);  // 设置Bounce实例的引脚
    debouncers[currentPinIndex].interval(5);                          // 设置按钮弹跳的间隔时间
  }

  BleGamepadConfiguration bleGamepadConfig;
  bleGamepadConfig.setAutoReport(false);
  bleGamepadConfig.setButtonCount(20);  //20是虚拟出来的数量，和前面的不一样
  bleGamepadConfig.setWhichAxes(false, false, false, false, false, false, false, false);
  bleGamepad.begin(&bleGamepadConfig);  // 使用BleGamepadConfiguration对象来初始化BLE游戏手柄
}

void EncodersUpdate()  // 更新编码器的函数
{
  int32_t tempEncoderCount1 = encoder1.getCount();  // 获取编码器1的当前计数值
  int32_t tempEncoderCount2 = encoder2.getCount();  // 获取编码器2的当前计数值
  bool sendReport = false;                          // 标志是否需要发送游戏手柄报告

  if (tempEncoderCount1 != encoder1Count) {
    sendReport = true;
    if (tempEncoderCount1 > encoder1Count)
      bleGamepad.press(ENC1_RightButton);  // 如果编码器1计数增加，则按下右侧按钮
    else
      bleGamepad.press(ENC1_LeftButton);  // 如果编码器1计数减少，则按下左侧按钮
  }

  if (tempEncoderCount2 != encoder2Count) {
    sendReport = true;
    if (tempEncoderCount2 > encoder2Count)
      bleGamepad.press(ENC2_RightButton);  // 如果编码器2计数增加，则按下右侧按钮
    else
      bleGamepad.press(ENC2_LeftButton);  // 如果编码器2计数减少，则按下左侧按钮
  }

  if (sendReport) {
    bleGamepad.sendReport();  // 发送游戏手柄报告
    delay(150);
    bleGamepad.release(ENC1_LeftButton);
    bleGamepad.release(ENC1_RightButton);
    bleGamepad.release(ENC2_LeftButton);
    bleGamepad.release(ENC2_RightButton);
    bleGamepad.sendReport();
    delay(50);

    encoder1Count = encoder1.getCount();  // 更新编码器1的计数值
    encoder2Count = encoder2.getCount();  // 更新编码器2的计数值
  }
}

void loop() {

  if (bleGamepad.isConnected()) {
    EncodersUpdate();         // 更新编码器状态
    bool sendReport = false;  // 标志是否需要发送游戏手柄报告
    for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++) {
      debouncers[currentIndex].update();    // 更新按钮弹跳状态
      if (debouncers[currentIndex].fell())  // 如果按钮按下
      {
        bleGamepad.press(physicalButtons[currentIndex]);  // 按下虚拟按钮
        sendReport = true;
      } else if (debouncers[currentIndex].rose())  // 如果按钮释放
      {
        bleGamepad.release(physicalButtons[currentIndex]);  // 释放虚拟按钮
        sendReport = true;
      }
    }

    if (sendReport) {
      bleGamepad.sendReport();  // 发送游戏手柄报告
    }
    delay(10);  // (取消)注释以添加/删除循环之间的延迟

    counter_for_battery_ADC++;
    if (counter_for_battery_ADC > 500) {
      battery_voltage_ADC = analogRead(15);                  // 读取电池电压的ADC值，15是电池电压的ADC引脚
      battery_voltage = battery_voltage_ADC * 6.6 / 4096.0;  // 将ADC值转换为电压
      battery_level = battery_voltage * 142.857 - 500;       // 估算电池电量，仅供参考
      bleGamepad.setBatteryLevel(battery_level);             // 设置游戏手柄的电池电量
      counter_for_battery_ADC = 0;
    }
  }
}
