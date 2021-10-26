#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
//Wi-Fi情報
const char* ssid = "bld2-guest";
const char* pass = "MatsumotoWay";
//const char* ssid = "IODATA-127068-2G";
//const char* pass = "7LLAH61238701";
//unixtimeあわせ
const char* ntp_server = "ntp.nict.jp";
unsigned long last_sync_time=0;
//ディスプレイ
#define OLED_RESET 2
Adafruit_SSD1306 display(OLED_RESET);

//IOTサーバホスト名とポート番号、通信用文字バッファ
const char* host = "iot.hongo.wide.ad.jp";
const int port = 10017; // ** 割り当てられたものを使用せよ**
char send_buf[100];

int prev_stat = LOW;
void setup() {
  Serial.begin(115200);
  pinMode(14, OUTPUT);
  digitalWrite(14, LOW);
  pinMode(2, INPUT);
  pinMode(12, INPUT);
  pinMode(13, INPUT);
  pinMode(16, INPUT);
  delay(10);

  //ディスプレイ
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Connecting to ");
  display.println(ssid);
  display.display();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
    display.display(); 
  }
  display.println();
  display.display(); 
  
  display.println("WiFi connected");
  display.println("IP address: ");
  display.println(WiFi.localIP());
  display.display();
  boolean status = syncNTPTime(ntp_server);
  display.print("sync to ");
  display.print(ntp_server);
  display.println();
  if(status == true){
    display.println("success");
  }else {
    display.println("failed, exiting...");
    delay(2000);
    exit(0);  
  }
  display.display();
  last_sync_time = now();
  delay(5000);
  unsigned long t=now();
  char str_time[30];
  sprintf(str_time,"%04d-%02d-%02dT%02d:%02d:%02d",
  year(t),month(t),day(t),
  hour(t),minute(t),second(t));
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(str_time);
  display.display();
}



void loop() {

 unsigned long t=now();
 if(t/10!=last_sync_time/10){
   display.clearDisplay();
   display.setCursor(0,0);
  //DIPスイッチ状態
   int ID = getDIPSWStatus();
   display.print("ID:");
   display.println(ID);
  //時刻
   unsigned long t=now();
   char str_time[30];
   sprintf(str_time,"%04d-%02d-%02dT%02d:%02d:%02d",
   year(t),month(t),day(t),
   hour(t),minute(t),second(t));

   display.println(str_time);

    //照度
    int lx = getIlluminance();
    display.print("Illuminance:");
    display.println(lx);
    
    //人感センサの状態
    boolean mdstat = getMDStatus();
    display.print("Motion Detected:");
    display.println(mdstat);
    display.display();
    syncNTPTime(ntp_server);
    last_sync_time=t;
  
    //TCP通信でサーバに置きに行く
    WiFiClient client;
     //通信できなかった場合のエラー処理
    if (!client.connect(host, port)) {
     display.println("...ERR, exiting...");
     display.display();
     delay(2000);
     exit(0);//XXX
    }
    sprintf(send_buf,"%d,%s,%d,%d\n",ID, str_time, lx, mdstat);
 
    client.print(send_buf);
    delay(3000);
    int bytes = 0;
    bytes = client.available();
    Serial.print("waiting...");
    Serial.print(bytes);
    Serial.print("bytes");
    String messages = "";
    
    if (bytes != 0){
      for (int ii = 0; ii < bytes; ii++){
        char tmp1 = client.read();
        //Serial.print("received");
        //Serial.print(tmp1);
        messages.concat(tmp1);
      }
      Serial.println(messages);
      if (messages.compareTo("OK\n") == 0){
        display.println("...OK"); 
        display.display();
        setBZ(true);
      }
      else if (messages.compareTo("ERROR\n") == 0){
        display.println("...NG");
        display.display();
        setBZ(true);
      }
      else if (messages.compareTo("XOK\n") == 0){
        display.println("...XOK"); 
        display.display();
        setBZ(false);
      }
      else if (messages.compareTo("XERROR\n") == 0){
        display.println("...XNG");
        display.display();
        setBZ(false);
      }
    }


    //client.available とclient.readで撮ってくるっぽい。
    if(!client.connected()){
      client.stop();
      display.println("...ERR, Exiting");
      display.display();
      delay(2000);//XXX
      exit(0);//XXX
    }
    
    client.stop();
  }
 
}

void setBZ(boolean on){
  if (on == true){
    digitalWrite(14, HIGH);  
  }
  if (on == false){
    digitalWrite(14, LOW);  
  }
  return;
}

boolean getPushSWStatus(){
  int stat = digitalRead(2);
  if (stat == LOW){
    return true;  
  }  
  if (stat == HIGH){
    return false;  
  }
}

boolean detectPushSWON(){
 int stat=digitalRead(2);
 int ret = 1;
 if(stat==LOW && prev_stat==HIGH){
  ret = 1;
 }
 else{
  ret = 0;
 }
 prev_stat = stat;
 delay(1);
 if (ret == 1){
  return true;
 }
 if (ret == 0){
  return false;
 }
}

int getDIPSWStatus(){
 int stat=0;
 int bit1=digitalRead(12);
 int bit0=digitalRead(13);
 if(bit0==LOW){
 stat|=0x01;
 }
 if(bit1==LOW){
 stat|=0x02;
 }
  
  return stat;
}

boolean getMDStatus(){
  int stat = digitalRead(16);
  if (stat == HIGH){
    return true;
  }else if (stat == LOW){
    return false;
  }
}

int getIlluminance(){  
  int lx = 3200*analogRead(A0)/3072;
  return lx;
}

unsigned long getNTPTime(const char* ntp_server){
 WiFiUDP udp;
 udp.begin(8888);
 unsigned long unix_time=0UL;
 byte packet[48];
 memset(packet, 0, 48);
 packet[0] = 0b11100011;
 packet[1] = 0;
 packet[2] = 6;
 packet[3] = 0xEC;
 packet[12] = 49;
 packet[13] = 0x4E;
 packet[14] = 49;
 packet[15] = 52;
 udp.beginPacket(ntp_server, 123); 
 udp.write(packet, 48);
 udp.endPacket();
 for(int i=0;i<10;i++){
 delay(500);
 if(udp.parsePacket()){
 udp.read(packet, 48);
 unsigned long highWord = word(packet[40], packet[41]);
 unsigned long lowWord = word(packet[42], packet[43]);
 unsigned long secsSince1900 = highWord << 16 | lowWord;
 const unsigned long seventyYears = 2208988800UL;
 unix_time = secsSince1900 - seventyYears + 32400UL; // 32400 = 9 hours (JST)
 break;
 } 
 }
 udp.stop();
 return unix_time;
}

boolean syncNTPTime(const char* ntp_server){
 unsigned long unix_time=getNTPTime(ntp_server);
 if(unix_time>0){
 setTime(unix_time);
 return true;
 }
 return false;
  
}
