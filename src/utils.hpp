#include <Arduino.h>

const char abc[] = "abcdefghijklmnopqrstuvwxyza";

bool strContainStr(const String& tar, const char* conta)
{
    int l = tar.length();
    int l2 = strlen(conta);
    if (l < l2) {
        return false;
    }
    for (int i = 0; i < l - l2; ++i) {
        bool flag = true;
        for (int j = 0; j < l2; ++j) {
            if (tar[i + j] != conta[j]) {
                flag = false;
                break;
            }
        }
        if (flag) {
            return true;
        }
    }
    return false;
}

String unique_id(uint8_t k = 0)
{
    return String(ESP.getChipId(), HEX) + String(k);
}

String getNodeIdFromTopic(const String& topic)
{
    int l = topic.length();
    int i = topic.lastIndexOf('/');
    if (i == -1) {
        return "";
    }
    return topic.substring(i + 1, l);
}