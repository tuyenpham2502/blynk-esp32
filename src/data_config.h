#ifndef _MY_CONFIG_H_
#define _MY_CONFIG_H_

const char* ssidAP     = "ESP32_IOT";     // Tên wifi AP Mode
const char* passwordAP = "";          // Mật khẩu AP Mode

String  Essid   = "";                 // EEPROM tên wifi nhà bạn
String  Epass   = "";                 // EEPROM mật khẩu wifi nhà bạn
String  Etoken = "";                  // EEPROM mã token blynk
int  EtempThreshold1 = 0;             // ngưỡng nhiệt độ 1   
int  EtempThreshold2 = 0;             // ngưỡng nhiệt độ 2 
int  EhumiThreshold1 = 0;             // ngưỡng độ ẩm 1
int  EhumiThreshold2 = 0;             // ngưỡng độ ẩm 2
int  EdustThreshold1 = 0;             // ngưỡng bụi 1
int  EdustThreshold2 = 0;             // ngưỡng bụi 2

String snTemp[4] = {"", "Nhiệt độ thấp ","Nhiệt độ phù hợp ","Nhiệt độ cao "};
String snHumi[4] = {"", "Độ ẩm thấp ","Độ ẩm phù hợp ","Độ ẩm cao "};
String snDust[4] = {"", "Không khí tốt ","Không khí trung bình ", "Không khí nguy hiểm độc hại "};

#endif