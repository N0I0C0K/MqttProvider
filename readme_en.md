# Mqtt Provider

Mqtt Provider is an MQTT wrapper library that allows you to connect a node to remote control with just a few lines of code. You can use your own interface or utilize the provided [interface](https://github.com/N0I0C0K/smart-home-front).

## How to Connect

### 1. Sensor Nodes should support the following functions

- `int get()` function, which returns an array from 0 to 100 representing the node's detection value. If it's a boolean type, it only needs to return 0 or 1.
- Controllers should also support `void set(int val)` function, which accepts a value (0-100).

**Example:**

```c++
namespace Led {
int led_pin = D5;
int led_state = LOW;
int btn_pin = D6;

int get()
{
    return led_state;
}

void set(int val)
{
    led_state = val ? HIGH : LOW;
    digitalWrite(led_pin, led_state);
}

void setup()
{
    pinMode(led_pin, OUTPUT);
    pinMode(btn_pin, INPUT_PULLUP);
    digitalWrite(led_pin, led_state);
}

void loop()
{
    if(btn_click(btn_pin)){
        led_state = led_state ? LOW : HIGH;
        set(led_state);
    }
}
}
```

### 2. Connecting Nodes

```c++
#include <MqttProvider.hpp>
namespace Led {
    ...........// Code for the LED mentioned above
}

void setup()
{
    Serial.begin(115200);   // Initialize Serial
    Led::setup();           // Initialize the LED
    Mqtt::init("WiFi SSID", "password", "Secret key");  // 1. Initialize MQTT
    Mqtt::MqttNode* led_node = new Mqtt::MqttNode(Mqtt::BOOL_CONTROLLER, "Light", "Kitchen", Led::get, Led::set);  // 2. Add a node
    Mqtt::send_alive();   // 3. Ensure message synchronization after adding the node
}

void loop()
{
    Led::loop();
    Mqtt::loop();   // 4. Must include this to enable data synchronization and other important functionalities
}
```

## Supported Features

- Remote control
- Automatic data synchronization (physical switches are also synchronized remotely)
- Alive checking

For detailed MQTT topic design, please refer to [this document](./mqtt%20design.md).
