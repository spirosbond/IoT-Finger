#include <Arduino.h>

// Import required libraries
#include <ESP8266WiFi.h>
#include <aREST.h>
#include <Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// Create aREST instance
aREST rest = aREST();

// WiFi parameters
const char* ssid     = "BigSunIT";
const char* password = "5p1r05b0nd";

// The port to listen for incoming TCP connections
#define LISTEN_PORT 80
#define HOST_PORT  8080

// Create an instance of the server
WiFiServer server(LISTEN_PORT);
WiFiClient client;
WiFiUDP ntpUDP;

const char* host = "www.socaapp.com";

Servo myServo;
int servoPin = 14;
int servoPos = 0;

// server pool, offset (sec), update interval (msec)
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

// Declare functions to be exposed to the API
int ledControl(String command);
int pushButton(String when);
int setPosition(String posValue);
void stopWiFi();
void startWiFi();

String timeStr = "";

void setup(void)
{
  // Start Serial
  Serial.begin(115200);

  //Red Led for debuging
  pinMode(0, OUTPUT);


  // Function to be exposed
  rest.function("led",ledControl);
  rest.function("pushbutton", pushButton);
  rest.function("setposition", setPosition);

  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("1");
  rest.set_name("IOTbuttonPusher");

  startWiFi();

  timeClient.begin();
}

void loop() {

  handleRest();

  timeClient.update();
  if(!(timeStr == timeClient.getFormattedTime())){
    timeStr = timeClient.getFormattedTime();
    Serial.print("time is: ");
    Serial.println(timeStr);
    Serial.println(myServo.read());
  }
  if(timeClient.getMinutes()%2 == 0){
    //stopWiFi();
  } else{
    //startWiFi();
  }

//getRequest(host, "/Soca/", HOST_PORT);
//deepSleepFor(10);
  //Serial.println(getTime());
}

void handleRest(){
  // Handle REST calls
  client = server.available();
  if (client) {
    while(!client.available()){
      delay(1);
    }
    rest.handle(client);
  }
}

String getRequest(const char* host, String url, int hostPort ){
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, hostPort)) {
    Serial.println("Connection Failed");
    return "Connection Failed";
  }

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
  "Host: " + host + "\r\n" +
  "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return "Client Timeout";
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  String response= "";
  String line = "";
  while(client.available()){
    line = client.readStringUntil('\r');
    //Serial.println(line);
    if(line.indexOf('{')>=0){
      response += line;
    }

  }
  Serial.println(response);

  return response;
}

// Custom function accessible by the API
int ledControl(String command) {

  // Get state from command
  int state = command.toInt();

  Serial.print("Setting led to: ");
  Serial.println(state);

  digitalWrite(0,state);
  return state;
}

int pushButton(String when) {

    Serial.println("Pushing button...");
    Serial.println(when);
    myServo.attach( servoPin );

    servoPos = 20;
    myServo.write(servoPos);
    delay(700);

    servoPos = 110;
    myServo.write(servoPos);
    delay(500);

    servoPos = 20;
    myServo.write(servoPos);

    delay(1000);
    myServo.detach();

    return 1;
}

int setPosition(String posValue) {
    myServo.attach( servoPin );

    servoPos = posValue.toInt();
    myServo.write(servoPos);

    delay(1000);
    myServo.detach();
    return 1;
}

unsigned long secondsUntilTime(int wakeUpHour, int wakeUpMinute, int wakeUpSecond){
    unsigned long seconds=0;
    int nowHour = timeClient.getHours();
    int nowMinute = timeClient.getMinutes();
    int nowSecond = timeClient.getSeconds();

    while(nowSecond!=wakeUpSecond){
        seconds++;
        nowSecond++;
        if(nowSecond==60){
            nowSecond=0;
            nowMinute++;
        }
    }

    while(nowMinute!=wakeUpMinute){
        seconds+=60;
        nowMinute++;
        if(nowMinute==60){
            nowMinute=0;
            nowHour++;
        }
    }

    while(nowHour!=wakeUpHour){
        seconds+=3600;
        nowHour++;
        if(nowHour==24){
            nowHour=0;
        }
    }

    return seconds;
}

void stopWiFi() {

  if(WiFi.status() == WL_CONNECTED){
    Serial.println("stoping WiFi...");
    server.stop();
    client.stop();
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(1);
  }
}

void startWiFi(){
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("starting WiFi...");
    WiFi.forceSleepWake();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(50);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    // Start the server
    server.begin();
    Serial.println("Server started");

    // Print the IP address
    Serial.println(WiFi.localIP());
  }
}

void deepSleepFor(unsigned long seconds){
  ESP.deepSleep(seconds * 1000000, WAKE_RF_DEFAULT);
}
