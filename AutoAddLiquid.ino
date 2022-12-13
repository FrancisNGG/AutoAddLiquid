/* *****************************************************************
 *
 * Download latest Blinker library here:
 * https://github.com/blinker-iot/blinker-library/archive/master.zip
 *
 *
 * Blinker is a cross-hardware, cross-platform solution for the IoT.
 * It provides APP, device and server support,
 * and uses public cloud services for data transmission and storage.
 * It can be used in smart home, data monitoring and other fields
 * to help users build Internet of Things projects better and faster.
 *
 * Make sure installed 2.7.4 or later ESP8266/Arduino package,
 * if use ESP8266 with Blinker.
 * https://github.com/esp8266/Arduino/releases
 *
 * Make sure installed 1.0.5 or later ESP32/Arduino package,
 * if use ESP32 with Blinker.
 * https://github.com/espressif/arduino-esp32/releases
 *
 * Docs: https://diandeng.tech/doc
 *
 *
 * *****************************************************************
 *
 * Blinker 库下载地址:
 * https://github.com/blinker-iot/blinker-library/archive/master.zip
 *
 * Blinker 是一套跨硬件、跨平台的物联网解决方案，提供APP端、设备端、
 * 服务器端支持，使用公有云服务进行数据传输存储。可用于智能家居、
 * 数据监测等领域，可以帮助用户更好更快地搭建物联网项目。
 *
 * 如果使用 ESP8266 接入 Blinker,
 * 请确保安装了 2.7.4 或更新的 ESP8266/Arduino 支持包。
 * https://github.com/esp8266/Arduino/releases
 *
 * 如果使用 ESP32 接入 Blinker,
 * 请确保安装了 1.0.5 或更新的 ESP32/Arduino 支持包。
 * https://github.com/espressif/arduino-esp32/releases
 *
 * 文档: https://diandeng.tech/doc
 *
 *
 * D3 --- GPIO0 --- FLASH
 * D4 --- GPIO2 --- TXD1 --- LED_BUILTIN
 * D2 --- GPIO4 --- SDA
 * D1 --- GPIO5 --- SCL
 * D6 --- GPIO12 --- MISO
 * D7 --- GPIO13 --- MOSI --- RXD2
 * D5 --- GPIO14 --- CLK
 * D8 --- GPIO15 --- CS --- TXD2
 * D0 --- GPIO16 --- WAKE
 *
 * *****************************************************************/

#define BLINKER_WIFI
// #define BLINKER_APCONFIG

#include <Blinker.h>
#include <Ticker.h>

char auth[] = "Blinker Key";
char ssid[] = "ssid";
char pswd[] = "password";

// #define RUN_BTN_PIN 4    //按键启动脚
#define LIQUID_OUT_PIN 5 //水泵电机控制脚

#define DELAY_TIME_ADDR 3000
#define LIQUID_TIME_ADDR 3010

// 新建组件对象
BlinkerSlider delayRunTimeSlider("ran-rze");
BlinkerSlider LiquidOutTimeSlider("ran-9b3");
BlinkerText wifiIP("tex-5fw");
BlinkerNumber wifiRssi("num-ca6");
BlinkerButton runNowButton("btn-r4z");
BlinkerNumber LiquidOutTimeNum("num-f8d");
BlinkerNumber delayRunTimeNum("num-t7a");

Ticker refresh_ticker;
Ticker runNow_ticker;
Ticker liquidOut_ticker;
Ticker intoDeepSleep_ticker;

uint8_t delayRunTime;
uint8_t LiquidOutTime;

uint8_t runNowStatus = 0;

/* 写参数到 Flash */
void configWirte(uint16_t addr, uint8_t val)
{
    if (addr < 3000) // Blinker 官方文档指出 0-2447 已被 Blinker 库使用
    {
        BLINKER_LOG("Address is too forward!");
        return;
    }

    EEPROM.begin(4096);
    EEPROM.write(addr, val);
    EEPROM.end();
}

void intoDeepSleep_callback()
{
    ESP.deepSleep(0);//0表示不会自动唤醒
}

/* Blinker 按钮控件回调，可以在app操作控件进行启动 */
void runNowButton_callback(const String &state)
{
    if (state == "on")
    {
        //开始启动，delayRunTime换算成秒，到期后执行一次回调
        runNow_ticker.once_scheduled(delayRunTime * 60, runNowTicker_callback);
        runNowStatus = 1;
    }
    else
    {
        runNowStatus = 0;
    }
}

/* 延迟启动时长滑块回调函数，设置延迟启动时长 */
void delayRunTime_callback(int32_t value)
{
    delayRunTime = value;
    configWirte(DELAY_TIME_ADDR, delayRunTime);
}

/* 出液持续时长滑块回调函数，设置出液持续时长 */
void LiquidOutTime_callback(int32_t value)
{
    LiquidOutTime = value;
    configWirte(LIQUID_TIME_ADDR, LiquidOutTime);
}

// 如果未绑定的组件被触发，则会执行其中内容
void dataRead(const String &data)
{
    BLINKER_LOG("Blinker readString: ", 123);
    // counter++;
    // Number1.print("123");
}

void heartbeat_callback()
{
    BLINKER_LOG("heartbeat_callback");
}

void runNowTicker_callback()
{
    digitalWrite(LIQUID_OUT_PIN, HIGH);
    liquidOut_ticker.once_ms_scheduled(LiquidOutTime * 100, liquidOutTicker_callback);
}

/* 出液完成回调，在这里设置休眠时间 */
void liquidOutTicker_callback()
{
    digitalWrite(LIQUID_OUT_PIN, LOW);
    runNowStatus = 0;
    intoDeepSleep_ticker.once_scheduled(10, intoDeepSleep_callback);// 10s后进入深度睡眠
}

void refreshTicker_callback()
{
    // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delayRunTimeSlider.print(delayRunTime);
    LiquidOutTimeSlider.print(LiquidOutTime);
    LiquidOutTimeNum.print(LiquidOutTime * 100);
    delayRunTimeNum.print(delayRunTime);
    wifiIP.print("IP : " + WiFi.localIP().toString());
    wifiRssi.print(WiFi.RSSI());

    if (WiFi.status() == WL_CONNECTED)
    {
        digitalWrite(LED_BUILTIN, LOW);
    }

    if (runNowStatus == 1)
    {
        runNowButton.print("on");
    }
    else
    {
        runNowButton.print("off");
    }
}

/* IO 中断函数，如果跟 WiFi 一起使用，则必须使用 ICACHE_RAM_ATTR 修饰放到 RAM 执行 */
// ICACHE_RAM_ATTR void IO_Interrupt_callback()
// {
//     runNowStatus = !runNowStatus;
//     if (runNowStatus == 1)
//     {
//         runNowStatus = 1;
//         // runNow_ticker.once_scheduled(delayRunTime * 60, runNowTicker_callback);
//         runNow_ticker.once_scheduled(5, runNowTicker_callback);
//     }
// }

void setup()
{
    // 初始化串口
    Serial.begin(115200);
    BLINKER_DEBUG.stream(Serial);
    BLINKER_DEBUG.debugAll();

    // 初始化 IO
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    pinMode(LIQUID_OUT_PIN, OUTPUT);
    digitalWrite(LIQUID_OUT_PIN, LOW);

    // pinMode(RUN_BTN_PIN, INPUT_PULLUP);
    // attachInterrupt(digitalPinToInterrupt(RUN_BTN_PIN), IO_Interrupt_callback, RISING);

    /* 启动 Blinker */
    Blinker.begin(auth, ssid, pswd);
    // Blinker.begin(auth);
    Blinker.attachData(dataRead);
    Blinker.attachHeartbeat(heartbeat_callback);

    /* 初始化 Blinker 控件 */
    refresh_ticker.attach_ms_scheduled(300, refreshTicker_callback);
    delayRunTimeSlider.attach(delayRunTime_callback);
    LiquidOutTimeSlider.attach(LiquidOutTime_callback);
    runNowButton.attach(runNowButton_callback);

    /* 读取 flash 保存的配置 */
    EEPROM.begin(4096);
    delayRunTime = EEPROM.read(DELAY_TIME_ADDR);
    LiquidOutTime = EEPROM.read(LIQUID_TIME_ADDR);
    EEPROM.end();

    /* 执行一次 */
    runNow_ticker.once_scheduled(delayRunTime * 60, runNowTicker_callback);
    runNowStatus = 1;
}

void loop()
{
    Blinker.run();
}
