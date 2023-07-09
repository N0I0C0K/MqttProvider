#include "utils.hpp"
#include <ArduinoMqttClient.h>
#include <ESP8266WiFi.h>

namespace Mqtt {
const char* topic_base = nullptr;

WiFiClient wificlient;
MqttClient mqttclient(wificlient);

enum NodeType {
    BOOL_SENSOR = 1,
    NUM_SENSOR = 2,
    BOOL_CONTROLLER = 11,
    NUM_CONTROLLER = 12
};

enum TopicType {
    ALIVE = 0,
    SENSOR = 1,
    CONTROLLER = 2
};
class MqttNode;

const int max_node_num = 8;
MqttNode* nodes[max_node_num];
uint8_t node_num = 0;

class MqttNode {
private:
public:
    NodeType type;
    const String nodeid;
    const String node_name; // node 的名字，如：温度传感器，台灯
    const String node_pos; // node 的位置，如：客厅
    int (*readSensorData)(void);
    void (*writeControllerData)(int) = nullptr;
    int pre_data = 0;
    unsigned long last_change_time = 0;
    bool is_regist = false;

    /// @brief Construct a new Mqtt Node object
    /// @param _type node 的类型
    /// @param _nodename node 的名字，如：温度传感器，台灯
    /// @param _node_pos node 的位置，如：客厅
    /// @param _readSensorData 节点数据读取函数，需要返回一个int
    /// @param _writeControllerData 节点数据写入函数，需要接受一个int(0-100)，不管是bool类型还是num类型, 如果是传感器则不需要
    /// @param auto_regist 是否自动注册到节点列表
    MqttNode(NodeType _type,
        const String& _nodename,
        const String& _node_pos,
        int (*_readSensorData)(void),
        void (*_writeControllerData)(int) = nullptr,
        bool auto_regist = true)
        : type(_type)
        , nodeid(unique_id(node_num))
        , node_name(_nodename)
        , node_pos(_node_pos)
        , readSensorData(_readSensorData)
        , writeControllerData(_writeControllerData)
    {
        if (auto_regist)
            this->regist();
    }

    void regist(bool initController = true)
    {
        if (is_regist)
            return;
        if (node_num >= max_node_num) {
            return;
        }
        nodes[node_num++] = this;
        const String& nodeid = this->nodeid;
        char tp[50];
        sprintf(tp, "%s/listen/sensor/%s", topic_base, nodeid.c_str());
        mqttclient.subscribe(tp);

        if (this->type == NodeType::BOOL_CONTROLLER || this->type == NodeType::NUM_CONTROLLER) {
            sprintf(tp, "%s/publish/control/%s", topic_base, nodeid.c_str());
            mqttclient.subscribe(tp);

            // init controller
            if (initController) {
                this->control(this->readSensorData());
            }
        }
        Serial.printf("regist node: %s\n", this->alive_string().c_str());
        is_regist = true;
    }

    String alive_string()
    {
        return nodeid + "|" + String(type) + "|" + node_name + "|" + node_pos + "|" + String(readSensorData());
    }
    void sendData()
    {
        String data = nodeid + "|" + String(type) + "|" + String(readSensorData()) + "|" + String(readSensorData());
        char tp[50];
        sprintf(tp, "%s/publish/sensor/%s", topic_base, nodeid.c_str());
        mqttclient.beginMessage(tp);
        mqttclient.print(data);
        mqttclient.endMessage();
    }
    void control(const int& val)
    {
        if (writeControllerData == nullptr)
            return;
        writeControllerData(val);
    }
    bool change()
    {
        int cur_data = readSensorData();
        if (cur_data != pre_data) {
            // Jitter cancellation
            unsigned long now = millis();
            if (this->type == NodeType::NUM_SENSOR || this->type == NodeType::NUM_CONTROLLER) {
                if ((now - last_change_time < 1000) || (now - last_change_time < 5000 && cur_data - pre_data <= 10)) {
                    return false;
                }
            }
            pre_data = cur_data;
            last_change_time = now;
            return true;
        }
        return false;
    }
};

void onMqttMessage(int size);

/// @brief 初始化mqtt
/// @param ssid wifi ssid
/// @param ssid_pwd wifi密码
/// @param _topic_base topic的前缀，所有节点保持一致
/// @param _broker mqtt服务器地址，默认不变
/// @param _borker_port mqtt端口，默认不变
void init(
    const char* ssid,
    const char* ssid_pwd,
    const char* _topic_base,
    const char* _broker = "broker-cn.emqx.io",
    int _borker_port = 1883)
{
    WiFi.begin(ssid, ssid_pwd);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }
    Serial.println("connect net success");
    // log ip info
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (!mqttclient.connect(_broker, _borker_port)) {
        Serial.print("MQTT connection failed! Error code = ");
        Serial.println(mqttclient.connectError());
        while (1) {
        }
    }
    Serial.println("connect to mqtt success!");

    topic_base = _topic_base;

    mqttclient.onMessage(onMqttMessage);

    char tp[50];
    sprintf(tp, "%s/publish/center/alive", topic_base);
    mqttclient.subscribe(tp);
}

TopicType getTopicType(const String& topic)
{
    if (strContainStr(topic, "alive")) {
        return TopicType::ALIVE;
    } else if (strContainStr(topic, "sensor")) {
        return TopicType::SENSOR;
    } else if (strContainStr(topic, "control")) {
        return TopicType::CONTROLLER;
    }
    return TopicType::ALIVE;
}

void send_test()
{
    char tp[50];
    sprintf(tp, "%s/test/message", topic_base);
    mqttclient.beginMessage(tp);
    mqttclient.print("test");
    mqttclient.endMessage();
}

void send_alive()
{
    char tp[50];
    sprintf(tp, "%s/listen/center/alive", topic_base);
    mqttclient.beginMessage(tp);
    for (int i = 0; i < node_num; ++i) {
        mqttclient.print(nodes[i]->alive_string());
        if (i != node_num - 1)
            mqttclient.print('|');
    }
    mqttclient.endMessage();
}

void send_sensor_data(const String& nodeid)
{
    if (nodeid.length() == 0)
        return;
    for (int i = 0; i < node_num; ++i) {
        if (nodes[i]->nodeid == nodeid) {
            nodes[i]->sendData();
            break;
        }
    }
}

void control_sensor_data(const String& nodeid, const String& data)
{
    if (nodeid.length() == 0)
        return;

    int idx = data.lastIndexOf('|');
    if (idx == -1)
        return;
    int val = data.substring(idx + 1).toInt();

    for (int i = 0; i < node_num; ++i) {
        if (nodes[i]->nodeid == nodeid) {
            nodes[i]->control(val);
            break;
        }
    }
}

void onMqttMessage(int size)
{
    String topic = mqttclient.messageTopic();
    TopicType type = getTopicType(topic);
    String data = mqttclient.readString();
    Serial.printf("recive topic: %s, type: %d, message: %s\n", topic.c_str(), type, data.c_str());
    switch (type) {
    case TopicType::ALIVE:
        Serial.println("recive alive");
        send_alive();
        break;
    case TopicType::SENSOR:
        Serial.println("sensor");
        send_sensor_data(getNodeIdFromTopic(topic));
        break;
    case TopicType::CONTROLLER:
        Serial.println("controller");
        control_sensor_data(getNodeIdFromTopic(topic), data);
        break;
    default:
        Serial.println("unknown command");
        break;
    }
}

void checkSensorDataChange()
{
    for (int i = 0; i < node_num; ++i) {
        if (nodes[i]->change()) {
            nodes[i]->sendData();
        }
    }
}

unsigned long last_alive_time = 0;

void loop()
{
    mqttclient.poll();
    checkSensorDataChange();
    // send alive every 30s
    if (millis() - last_alive_time > 30000) {
        send_alive();
        last_alive_time = millis();
    }
}

}