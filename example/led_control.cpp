#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <MqttProvider.hpp>

namespace LED_1 {

const int led_pin = D5;
const int btn_pin = D6;

uint8_t led_stat = LOW;
uint8_t last_btn_stat = HIGH;

void led_setup()
{
    pinMode(led_pin, OUTPUT);
    pinMode(btn_pin, INPUT_PULLUP);
}

void set_led(int stat)
{
    led_stat = stat ? HIGH : LOW;
    digitalWrite(led_pin, led_stat);
}

int get_led_stat()
{
    return led_stat;
}

void loop()
{
    uint8_t btn_stat = digitalRead(btn_pin);
    if (btn_stat != last_btn_stat) {
        last_btn_stat = btn_stat;
        if (btn_stat == LOW) {
            Serial.println('D');
            set_led(!led_stat);
        }
    }
}

} // namespace LED_1

void setup()
{
    Serial.begin(115200);
    Mqtt::init("不醒人室", "5050505050", "acddb");
    Mqtt::MqttNode* led_node = new Mqtt::MqttNode(Mqtt::BOOL_CONTROLLER, "台灯", "客厅", LED_1::get_led_stat, LED_1::set_led);
    Mqtt::send_alive();

    LED_1::led_setup();
}

void loop()
{
    Mqtt::loop();
    LED_1::loop();
}