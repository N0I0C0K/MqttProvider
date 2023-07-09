#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <MqttProvider.hpp>

namespace KSmoke {
int kitc_smoke = 0;

int Smoke = D0; // MQ-2模拟输出引脚
int Buzzer = D1; // 蜂鸣器引脚
int Led = D2; // LED灯引脚

int get_kitc_smoke_stat()
{
    return kitc_smoke;
}

void ksmoke_setup()
{
    pinMode(Buzzer, OUTPUT); // 烟雾
    pinMode(Led, OUTPUT);
    pinMode(Smoke, INPUT);
}

void ksmoke_loop()
{
    int smokeValue = analogRead(Smoke); // 烟雾传感器
    if (smokeValue > 300) { // 烟雾浓度超过300时，LED灯闪烁，蜂鸣器响起
        digitalWrite(Buzzer, LOW);
        digitalWrite(Led, HIGH);
        kitc_smoke = 1;
    } else {
        digitalWrite(Buzzer, HIGH);
        digitalWrite(Led, LOW);
        kitc_smoke = 0;
        delay(1000);
    }
}
}
namespace KFlame {
bool kitc_falme = 0;

const int flamePin = D6; // 火焰传感器引脚
const int ledPin = D2;
int Buzzer = D1; // LED引脚

int get_kitc_flame_stat()
{
    return kitc_falme;
}

void kflame_setup()
{
    pinMode(flamePin, INPUT); // 火焰
    pinMode(ledPin, OUTPUT);
    pinMode(Buzzer, OUTPUT);
}

void kflame_loop()
{
    int flameValue = digitalRead(flamePin); // 火焰传感器

    if (flameValue == LOW) {
        digitalWrite(ledPin, HIGH);
        digitalWrite(Buzzer, LOW);
        kitc_falme = 1;
    } else {
        digitalWrite(ledPin, LOW);
        digitalWrite(Buzzer, HIGH);
        kitc_falme = 0;
    }
}
}
namespace KLED {

const int led_pin = D2;
const int btn_pin = D3;

int led_stat = LOW;
int last_btn_stat = HIGH;

void set_kitc_led(int stat)
{
    led_stat = stat ? HIGH : LOW;
    digitalWrite(led_pin, led_stat);
}

int get_kitc_led_stat()
{
    return led_stat;
}

void kled_setup()
{
    pinMode(led_pin, OUTPUT);
    pinMode(btn_pin, INPUT_PULLUP);
}

void kled_loop()
{
    uint8_t btn_stat = digitalRead(btn_pin);
    if (btn_stat != last_btn_stat) {
        last_btn_stat = btn_stat;
        if (btn_stat == LOW) {
            Serial.println('D');
            set_kitc_led(!led_stat);
        }
    }
}
}
namespace KWater {
int waterSensor = A0; // 水位传感器模拟输入引脚
int Buzzer = D1; // LED数字输出引脚
int led = D2;

int water_stat = 0;
double temp, data;

int get_kitc_water_stat()
{
    return water_stat;
}
void kwater_setup()
{
    pinMode(waterSensor, INPUT);
    pinMode(Buzzer, OUTPUT);
    pinMode(led, OUTPUT);
}
void kwater_loop()
{
    double temp = analogRead(waterSensor); // 读取水位传感器的值
    data = (temp / 650) * 4;
    if (data > 1) { // 当水位高于设定值时，蜂鸣器报警，LED频闪红灯
        digitalWrite(Buzzer, LOW);
        digitalWrite(led, HIGH);
        delay(100);
        digitalWrite(led, LOW);
        delay(100);
        water_stat = 1;
    } else { // 当水位高于设定值时，关闭蜂鸣器和LED
        digitalWrite(Buzzer, HIGH);
        digitalWrite(led, LOW);
        water_stat = 0;
    }
}
}
void setup()
{
    Serial.begin(115200);
    KLED::kled_setup();
    KFlame::kflame_setup();
    KSmoke::ksmoke_setup();
    KWater::kwater_setup();

    Mqtt::init("WiFi SSID", "password", "Secret key");
    Mqtt::MqttNode* led_node = new Mqtt::MqttNode(Mqtt::BOOL_CONTROLLER, "灯光", "厨房", KLED::get_kitc_led_stat, KLED::set_kitc_led);
    Mqtt::MqttNode* flame_node = new Mqtt::MqttNode(Mqtt::BOOL_SENSOR, "火焰传感器", "厨房", KFlame::get_kitc_flame_stat);
    Mqtt::MqttNode* smoke_node = new Mqtt::MqttNode(Mqtt::BOOL_SENSOR, "烟雾传感器", "厨房", KSmoke::get_kitc_smoke_stat);
    Mqtt::MqttNode* water_node = new Mqtt::MqttNode(Mqtt::BOOL_SENSOR, "水位传感", "厨房", KWater::get_kitc_water_stat);
    Mqtt::send_alive();
}

void loop()
{
    KLED::kled_loop();
    KFlame::kflame_loop();
    KSmoke::ksmoke_loop();
    KWater::kwater_loop();
    Mqtt::loop();
}
