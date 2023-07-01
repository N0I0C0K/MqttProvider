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

String random_id()
{
    char id[7];
    for (uint8_t i = 0; i < 6; ++i) {
        id[i] = abc[random(0, 26)];
    }
    id[6] = '\0';
    return String(id);
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