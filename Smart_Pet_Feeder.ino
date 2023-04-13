#include <BlynkSimpleEsp32.h> //Blynk
#include <TimeLib.h>

#include <TridentTD_LineNotify.h> //Line

#include <WiFi.h>
#include <HTTPClient.h>
#include <ThingSpeak.h> //ThingSpeak

#include <DS1302.h> //DS1302
#include <Wire.h>

#include <DHT.h> //DHT22

//Servo S90
#include <ESP32Servo.h>
// lcd
#include <LiquidCrystal_I2C.h>

#define BLYNK_TEMPLATE_ID "YOUR TEMPLATE ID"
#define BLYNK_TEMPLATE_NAME "YOUR NAME TEMPLATE"
#define BLYNK_AUTH_TOKEN "YOUR TOKEN KEY"
#define LINE_TOKEN  "YOUR TOKEN KEY"
#define BLYNK_PRINT Serial



//delcare variable
Servo myservo;
DHT dht;
DS1302 rtc(2, 4, 5);  // DS1302 rtc([CE/RST], [I/O], [CLOCK]);


// Wi-Fi credentials
// Set password to "" for open networks.
const char* ssid = "YOUR NAME WIFI";
const char* pass = "YOUR PASSWORD WIFI";
// for HC-SR04
const int pingPin = 17;    //tx
int inPin = 16; //        rx

char auth[] = BLYNK_AUTH_TOKEN;

// ThingSpeak settings
const char* server = "api.thingspeak.com";
const char* apiKey = "YOUR API KEY";
const int httpPort = 80;
const char* httpEndpoint = "/update";

// set the LCD address and size
LiquidCrystal_I2C lcd(0x27, 16, 2);  

Time t_now; // DS1302Time

BlynkTimer timer; // BlynkTimer

void setup()
{
  myservo.attach(13); //  set servo pin
  Serial.begin(115200);
  Wire.begin(); //i2c
  dht.setup(4); //set dht22 pin
  setDateTime();

  Blynk.begin(auth, ssid, pass);
  Blynk.syncAll();
  // Setup timer to read sensor and send data to ThingSpeak every 5 seconds
  timer.setInterval(5000L, sendDataToThingSpeak);
}

BLYNK_WRITE(V1){
  int pinValue = param.asInt();

  if (pinValue == 1) {
    Serial.println(pinValue);
    doservo();
    // linenotify();
  }
}

long microsecondsToCentimeters(long microseconds)
{
// The speed of sound is 340 m/s or 29 microseconds per centimeter.
// The ping travels out and back, so to find the distance of the
// object we take half of the distance travelled.
return microseconds / 29 / 2;
}

void sendDataToThingSpeak() {
  // Get temperature readings from DHT22
  float temperature = dht.getTemperature();
  t_now = rtc.getTime(); //Get datetime reading from DS1302
  Serial.print("Temperature: ");
  Serial.println(temperature);

  // Ultrasonic sensor reading
  long duration, cm;
  pinMode(pingPin,OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pingPin, LOW);
  pinMode(inPin, INPUT);
  duration = pulseIn(inPin, HIGH);
  cm = microsecondsToCentimeters(duration);
  Serial.print("Distance: ");
  Serial.println(cm);

  // Construct ThingSpeak update URL
  String url = "http://";
  url += server;
  url += httpEndpoint;
  url += "?api_key=";
  url += apiKey;
  url += "&field1=";
  url += String(temperature);
  url += "&field2=";
  url += String(cm);

  // Send update to ThingSpeak
  HTTPClient http;
  http.begin(url);
  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
    Serial.println("Update sent to ThingSpeak"); // when success
  } else {
    Serial.print("Error sending update: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void doservo(){
  myservo.write(0); // 
  delay(1000);
  myservo.write(120);
  delay(3000);
  myservo.write(0);
  delay(1000);
  linenotify();
  displaylcd();
}

void linenotify() {
  Serial.begin(115200); Serial.println();
  Serial.println(LINE.getVersion());
  // กำหนด Line Token
  LINE.setToken(LINE_TOKEN);
  // sent message to Line 
  t_now = rtc.getTime();
  String datetime_str = String(t_now.date, DEC) + "-" + String(t_now.mon, DEC) + "-" + String(t_now.year, DEC) + " " + String(t_now.hour, DEC) + ":" + String(t_now.min, DEC);
  String temp_notify = "เจ้ามนุษย์ข้าได้รับอาหารแล้ว  อุณหภูมิ ณ ";
  temp_notify += datetime_str;
  
  float temperature = dht.getTemperature();
  temp_notify += " = "+String(temperature)+" °C";

  // ส่งข้อความ
  LINE.notify(temp_notify);
}


void displaylcd() {
  Wire.begin();
  lcd.begin();  // initialize the LCD
  
  t_now = rtc.getTime();
  String datetime_str = String(t_now.date, DEC) + "-" + String(t_now.mon, DEC) + "-" + String(t_now.year, DEC) + " " + String(t_now.hour, DEC) + ":" + String(t_now.min, DEC);
  lcd.backlight();  // turn on the backlight
  lcd.setCursor(1, 0);  // set the cursor to the top-left corner
  lcd.print("Last feeding at");
  lcd.setCursor(0, 1);
  lcd.print(datetime_str); //DD-MM-YYYY HH:MM

}

void setDateTime() {
  rtc.halt(false);
  rtc.writeProtect(false);
  rtc.setDOW(THURSDAY); // Set Day-of-Week 
  rtc.setTime(0, 32, 0); // Set the time to 12:00:00 (24hr format)
  rtc.setDate(14, 4, 2023); // Set the date
}

void loop()
{
  Blynk.run();
  timer.run();
}
