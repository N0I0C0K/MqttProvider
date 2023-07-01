#include "utils.hpp"
#include <ArduinoMqttClient.h>
#include <ESP8266WiFi.h>

namespace Mqtt {

const char broker[] = "test.mosquitto.org";
const int borker_port = 1883;
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

class MqttNode {
private:
public:
    NodeType type;
    const String nodeid;
    int (*readSensorData)(void);
    void (*writeControllerData)(int);
    int pre_data = 0;
    MqttNode(NodeType _type, const String& _nodeid, int (*_readSensorData)(void), void (*_writeControllerData)(int))
        : type(_type)
        , nodeid(_nodeid)
        , readSensorData(_readSensorData)
        , writeControllerData(_writeControllerData)
    {
    }
    String to_string()
    {
        return nodeid + "|" + String(type);
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
        writeControllerData(val);
    }
    bool change()
    {
        int cur_data = readSensorData();
        if (cur_data != pre_data) {
            pre_data = cur_data;
            return true;
        }
        return false;
    }
};

const int max_node_num = 8;
MqttNode* nodes[max_node_num];
int node_num = 0;

void onMqttMessage(int size);

void init(const char* ssid, const char* ssid_pwd, const char* _topic_base)
{
    WiFi.begin(ssid, ssid_pwd);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }
    Serial.println("connect net success");

    if (!mqttclient.connect(broker, borker_port)) {
        Serial.print("MQTT connection failed! Error code = ");
        Serial.println(mqttclient.connectError());
        while (1) { }
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
    } else if (strContainStr(topic, "controller")) {
        return TopicType::CONTROLLER;
    }
    return TopicType::ALIVE;
}

void send_alive()
{
    char tp[50];
    sprintf(tp, "%s/listen/center/alive", topic_base);
    mqttclient.beginMessage(tp);
    for (int i = 0; i < node_num; ++i) {
        mqttclient.print(nodes[i]->to_string());
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

/// @brief 注册节点
/// @param type 节点类型
/// @param readSensorData 节点数据读取函数，需要返回一个int
/// @param writeControllerData 节点数据写入函数，需要接受一个int(0-100)，不管是bool类型还是num类型
/// @param initController 是否初始化控制器，如果是控制器节点，是否初始化控制器
void regist_node(NodeType type, int (*readSensorData)(void), void (*writeControllerData)(int), bool initController = true)
{
    if (node_num >= max_node_num) {
        return;
    }
    String nodeid = random_id();
    MqttNode* node = new MqttNode(type, nodeid, readSensorData, writeControllerData);
    nodes[node_num++] = node;

    char tp[50];
    sprintf(tp, "%s/listen/sensor/%s", topic_base, nodeid.c_str());
    mqttclient.subscribe(tp);

    if (type == NodeType::BOOL_CONTROLLER || type == NodeType::NUM_CONTROLLER) {
        sprintf(tp, "%s/publish/controller/%s", topic_base, nodeid.c_str());
        mqttclient.subscribe(tp);

        // init controller
        if (initController) {
            writeControllerData(readSensorData());
        }
    }

    Serial.printf("regist node: %s\n", node->to_string().c_str());
}

void checkSensorDataChange()
{
    for (int i = 0; i < node_num; ++i) {
        if (nodes[i]->change()) {
            nodes[i]->sendData();
        }
    }
}

void loop()
{
    mqttclient.poll();
    checkSensorDataChange();
}

}
