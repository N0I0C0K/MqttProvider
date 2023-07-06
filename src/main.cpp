#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <MqttProvider.hpp>

namespace RLED {

const int led_pin = D5;
const int btn_pin = D3;

int led_stat = LOW;
int last_btn_stat = HIGH;

void set_rtrm_led(int stat)
{
    led_stat = stat ? HIGH : LOW;
    digitalWrite(led_pin, led_stat);
}

int get_rtrm_led_stat()
{
    return led_stat;
}

void rled_setup()
{
    pinMode(led_pin, OUTPUT);
    pinMode(btn_pin, INPUT_PULLUP);
}

void rled_loop()
{
    int btn_stat = digitalRead(btn_pin);
    if (btn_stat != last_btn_stat) {
        last_btn_stat = btn_stat;
        if (btn_stat == LOW) {
            Serial.println('D');
            set_rtrm_led(!led_stat);
        }
    }
}
}
namespace RWater {
int waterSensor = A0; // 水位传感器模拟输入引脚
int Buzzer = D1; // LED数字输出引脚
int led = D4;

int water_stat = 0;
unsigned long last_change_time = 0;
double temp, data;

int get_rtrm_water_stat()
{
    return water_stat;
}
void rwater_setup()
{
    pinMode(waterSensor, INPUT);
    pinMode(Buzzer, OUTPUT);
    pinMode(led, OUTPUT);
}
void rwater_loop()
{
    double temp = analogRead(waterSensor); // 读取水位传感器的值
    data = (temp / 650) * 4;
    if (data > 1) { // 当水位高于设定值时，蜂鸣器报警，LED频闪红灯
        digitalWrite(Buzzer, LOW);
        water_stat = 1;
        unsigned int t = millis();
        if (t - last_change_time < 100) {
            digitalWrite(led, HIGH);
        } else if (t - last_change_time < 200) {
            digitalWrite(led, LOW);
        } else {
            last_change_time = t;
        }
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
    RLED::rled_setup();
    RWater::rwater_setup();

    Mqtt::init("YH HUAWEI PHONE", "yuanhao123@@@", "acddb");
    Mqtt::MqttNode* led_node = new Mqtt::MqttNode(Mqtt::BOOL_CONTROLLER, "灯光", "卫生间", RLED::get_rtrm_led_stat, RLED::set_rtrm_led);
    Mqtt::MqttNode* water_node = new Mqtt::MqttNode(Mqtt::BOOL_SENSOR, "水位传感", "卫生间", RWater::get_rtrm_water_stat);
    Mqtt::send_alive();
}

void loop()
{
    RLED::rled_loop();
    RWater::rwater_loop();
    Mqtt::loop();
}