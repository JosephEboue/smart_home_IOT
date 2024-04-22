/*  ___   ___  ___  _   _  ___   ___   ____ ___  ____  
 * / _ \ /___)/ _ \| | | |/ _ \ / _ \ / ___) _ \|    \ 
 *| |_| |___ | |_| | |_| | |_| | |_| ( (__| |_| | | | |
 * \___/(___/ \___/ \__  |\___/ \___(_)____)___/|_|_|_|
 *                  (____/ 
 * Use browser and OSOYOO MEGA-IoT extension shield to detect remote intruder(motion sensor)
 * Tutorial URL http://osoyoo.com/?p=28872
 * CopyRight www.osoyoo.com
 */
#include <Servo.h>
Servo head;
#define SERVO_PIN 3
#define redLED 11
#define greenLED 12
#define buzzer 5
#define motion_sensor 4
#define light_sensor A0
#define Button 7
  
int lightStatus=0;
String lightStr;
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 16, 2);  // set the LCD address from 0x27 to 0x3f if i2c address is 0x3f

#include "Wire.h"
#include <SPI.h>
#include <RFID.h>
unsigned char my_rfid[] = { 42, 50, 98, 191, 197 };
RFID rfid(48, 49);
#include "WiFiEsp.h"
#include "SoftwareSerial.h"
SoftwareSerial softserial(A9, A8);  // A9 to ESP_TX, A8 to ESP_RX by default

int gasStatus = 0;
String gasStr;
char ssid[] = "Joe Joe";   // replace ****** with your network SSID (name)
char pass[] = "0#123Joe";  // replace ****** with your network password
int status = WL_IDLE_STATUS;
int ledStatus = LOW;
bool intruderPrinted = false;  // Flag to keep track of whether the intruder message has been printed
WiFiEspServer server(80);
RingBuffer buf(8);
// char packetBuffer[5];
int DoorStatus = LOW;
WiFiEspClient client;
//Keypad
#include "Keypad.h"
const byte ROWS = 4;  //four rows
const byte COLS = 4;  //four columns
char hexaKeys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte rowPins[ROWS] = { 33, 35, 37, 39 };
byte colPins[COLS] = { 41, 43, 45, 47 };
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
const String password = "1234";  // change your password here
String input_password;
// Think Speak
const char* serverApi = "api.thingspeak.com";
// const int channelID = 2511593;
// Think Speak function
void WriteToThingSpeak(int fieldNum, int data) {
  char url[100];
  sprintf(url, "/update?api_key=%s&field%d=%d", "3MN0OLQNL3XWEN2D", fieldNum, data);
  if (client.connect(serverApi, 80)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + serverApi + "\r\n" + "Connection: close\r\n\r\n");
    client.stop();
  }
}

void setup() {
  pinMode(redLED, OUTPUT);        // initialize digital pin Red LED as an output.
  pinMode(greenLED, OUTPUT);      // initialize digital pin Red LED as an output.
  pinMode(motion_sensor, INPUT);  // initialize gas sensor pin input.
  pinMode(buzzer, OUTPUT);        // initialize digital pin Red LED as an output.
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(light_sensor, INPUT); 
  SPI.begin();
  rfid.init();
  lcd.init();
  lcd.backlight();
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);  // initialize serial for debugging
  softserial.begin(115200);
  softserial.write("AT+CIOBAUD=9600\r\n");
  softserial.write("AT+RST\r\n");
  softserial.begin(9600);  // initialize serial for ESP module
  WiFi.init(&softserial);  // initialize ESP module

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true)
      ;
  }

  // attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");
  // printWifiStatus();

  // start the web server on port 80
  server.begin();
}

void loop() {
  char customKey = customKeypad.getKey();
  gasStatus = digitalRead(motion_sensor);
  lightStatus=digitalRead(light_sensor);

  // motion sensor
  if (gasStatus == 0 && !intruderPrinted) {
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
    digitalWrite(buzzer, LOW);
    lcd.clear();
    lcd.print("Home");
    intruderPrinted = true;
  } else if (gasStatus == 1 && intruderPrinted) {
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW);
    digitalWrite(buzzer, HIGH);
    lcd.clear();
    lcd.print("Alert Intruder !");
    WriteToThingSpeak(1, gasStatus);
    intruderPrinted = false;
  }

  // RFID start
  if (rfid.isCard()) {
    Serial.println("Find the card!");
    //read serial number
    if (rfid.readCardSerial()) {
      Serial.println("id=");
      for (int i = 0; i < 5; i++) {
        Serial.print(rfid.serNum[i]);
        Serial.print(' ');
      }
      Serial.println();
      if (compare_rfid(rfid.serNum, my_rfid)) {
        Serial.println("Access Granted");
        open_door();
        success();
        lcd.clear();
        lcd.print("Access Granted");
      } else {
        WriteToThingSpeak(2, 2);
        Serial.println("Access Denied");
        close_door();
        denied();
        lcd.clear();
        lcd.print("Access Denied");
      }
    }
    rfid.selectTag(rfid.serNum);
  }
  rfid.halt();

  // KeyPad here
  if (customKey) {
    Serial.println(customKey);

    if (customKey == '*') {
      input_password = "";  // clear input password
    } else if (customKey == '#') {
      if (password == input_password) {
        Serial.println("Access Granted");
        lcd.clear();
        lcd.print("Welcome");
        success();
        head.attach(3);
        delay(300);
        head.write(180);  //servo goes to 180 degrees
        delay(400);
        head.detach();  // save current of servo
        digitalWrite(3, LOW);

      } else {
        WriteToThingSpeak(3, 3);
        Serial.println("Permission Denied");
        lcd.clear();
        lcd.print("Unauthorized !");
        denied();
        head.attach(3);
        delay(300);
        head.write(0);  //servo goes to 0 degrees
        delay(400);
        head.detach();  // save current of servo
        digitalWrite(3, LOW);
      }

      input_password = "";  // clear input password
    } else {
      input_password += customKey;  // append new character to input password string
    }
  }
}
boolean compare_rfid(unsigned char x[],unsigned char y[])
{
  for (int i=0;i<5;i++)
  {
    if(x[i]!=y[i]) return false;
  }
  return true;
}
void open_door() {
    head.attach(SERVO_PIN); 
    delay(300);
    head.write(180);//servo goes to 0 degrees 
     delay(400);
    head.detach(); // save current of servo
    digitalWrite(SERVO_PIN,LOW);
}
void close_door() {
    head.attach(SERVO_PIN); 
    delay(300);
    head.write(0);//servo goes to 0 degrees 
     delay(400);
    head.detach(); // save current of servo
    digitalWrite(SERVO_PIN,LOW);
}
void success(){
    digitalWrite(redLED,LOW);
    digitalWrite(greenLED,HIGH);
    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
}
void denied(){
    digitalWrite(greenLED,LOW);
    digitalWrite(redLED,HIGH);
    digitalWrite(buzzer, HIGH);
    delay(1000);
    digitalWrite(buzzer, LOW);
}