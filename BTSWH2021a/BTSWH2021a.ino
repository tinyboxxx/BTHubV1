#define BOUNCE_WITH_PROMPT_DETECTION // Make button state changes available immediately
//！需要安装以下三个库
#include <ESP32Encoder.h> // https://github.com/madhephaestus/ESP32Encoder
#include <Bounce2.h>      // https://github.com/thomasfredericks/Bounce2
#include <BleGamepad.h>   // https://github.com/lemmingDev/ESP32-BLE-Gamepad

ESP32Encoder encoder1; // Create encoder objects
ESP32Encoder encoder2;
int32_t encoder1Count = 0; // Variables to store the current encoder count
int32_t encoder2Count = 0;
#define ENC1_LeftButton 5
#define ENC1_RightButton 6
#define ENC2_LeftButton 15
#define ENC2_RightButton 16
#define numOfButtons 16

Bounce debouncers[numOfButtons];
BleGamepad bleGamepad("SteeringWheel Wireless Hub", "Tinyboxxx", 100); //Shows how you can customize the device name, manufacturer name and initial battery level
byte buttonPins[numOfButtons] = {33, 25, 14, 39, 32, 34, 35, 36, 2, 5, 17, 18, 21, 19, 22, 23}; //每个按钮所在的实际IO位置
//4,16, encoder2
//26,27, encoder1
byte physicalButtons[numOfButtons] = {1, 2, 3, 4, 7, 8, 9, 10, 11, 12, 13, 14, 17, 18, 19, 20}; //虚拟出来电脑里显示的按钮顺序。5、6、15、16为旋转编码器
int counter_for_battery_ADC = 0;
int battery_level = 100;
int battery_voltage_ADC = 0;
float battery_voltage = 0;

void setup()
{
  ESP32Encoder::useInternalWeakPullResistors = UP; // Enable the weak pull up resistors
  encoder1.attachHalfQuad(26, 27);                 // Attach pins to encoders
  encoder2.attachHalfQuad(4, 16);
  encoder1.clearCount(); // Clear encoder counts
  encoder2.clearCount();
  encoder1Count = encoder1.getCount(); // Initialise encoder counts
  encoder2Count = encoder2.getCount();

  for (byte currentPinIndex = 0; currentPinIndex < numOfButtons; currentPinIndex++)
  {
    pinMode(buttonPins[currentPinIndex], INPUT_PULLUP);
    debouncers[currentPinIndex] = Bounce();
    debouncers[currentPinIndex].attach(buttonPins[currentPinIndex]); // After setting up the button, setup the Bounce instance
    debouncers[currentPinIndex].interval(5);
  }
  bleGamepad.begin(20, 0, false, false, false, false, false, false, false, false, false, false, false, false, false); //后面的false是关闭默认开启的轴输入
  bleGamepad.setAutoReport(false);
}

void EncodersUpdate() //更新编码器
{
  int32_t tempEncoderCount1 = encoder1.getCount();
  int32_t tempEncoderCount2 = encoder2.getCount();
  bool sendReport = false;

  if (tempEncoderCount1 != encoder1Count)
  {
    sendReport = true;
    if (tempEncoderCount1 > encoder1Count)
      bleGamepad.press(ENC1_RightButton);
    else
      bleGamepad.press(ENC1_LeftButton);
  }

  if (tempEncoderCount2 != encoder2Count)
  {
    sendReport = true;
    if (tempEncoderCount2 > encoder2Count)
      bleGamepad.press(ENC2_RightButton);
    else
      bleGamepad.press(ENC2_LeftButton);
  }

  if (sendReport)
  {
    bleGamepad.sendReport();
    delay(150);
    bleGamepad.release(ENC1_LeftButton);
    bleGamepad.release(ENC1_RightButton);
    bleGamepad.release(ENC2_LeftButton);
    bleGamepad.release(ENC2_RightButton);
    bleGamepad.sendReport();
    delay(50);

    encoder1Count = encoder1.getCount();
    encoder2Count = encoder2.getCount();
  }
}

void loop()
{

  if (bleGamepad.isConnected())
  {
    EncodersUpdate();
    bool sendReport = false;
    for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++)
    {
      debouncers[currentIndex].update();
      if (debouncers[currentIndex].fell())
      {
        bleGamepad.press(physicalButtons[currentIndex]);
        sendReport = true;
      }
      else if (debouncers[currentIndex].rose())
      {
        bleGamepad.release(physicalButtons[currentIndex]);
        sendReport = true;
      }
    }

    if (sendReport)
    {
      bleGamepad.sendReport();
    }
    delay(10); // (Un)comment to remove/add delay between loops

    counter_for_battery_ADC++;
    if (counter_for_battery_ADC > 500)
    {
      battery_voltage_ADC = analogRead(15); //15 is battery voltage ADC pin
      battery_voltage = battery_voltage_ADC * 6.6 / 4096.0;
      battery_level = battery_voltage * 142.857 - 500; //目前电量估测不准，仅供参考
      bleGamepad.setBatteryLevel(battery_level);
      counter_for_battery_ADC = 0;
    }
  }
}
