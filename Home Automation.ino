/*Blynk and ESP8266WiFi(NODEMCU)*/

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
BlynkTimer timer;

/*Temperature and Humidity Sensor(DTH11)*/

#include "dht.h"
#define dht_apin D0 
dht DHT;

#define pirPin D7                // Input for HC-S501
int pirValue;                   // Place to store read PIR Value
int pinValue; 
int pinValue1;

#include <Servo.h>
Servo servo_1;


char auth[] = "8efeb6f586e646caac830654f9464121"; //Blynk app Auth Key i.e sent to ur email
char pass[] = ""; //wifi password
char ssid[] = ""; //wifi name


BLYNK_WRITE(V0)
{
 pinValue = param.asInt();    
} 
BLYNK_WRITE(V1)
{
 pinValue1 = param.asInt();    
}

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define MQTT_SERV "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_NAME "suryawanshi"
#define MQTT_PASS "e3e16463eab84eabbaa4dba11f98908c"

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERV, MQTT_PORT, MQTT_NAME, MQTT_PASS);

Adafruit_MQTT_Subscribe onoff = Adafruit_MQTT_Subscribe(&mqtt, MQTT_NAME "/f/ServoMotor"); 
Adafruit_MQTT_Publish LightsStatus = Adafruit_MQTT_Publish(&mqtt, MQTT_NAME "/f/ServoMotorStatus");

void setup ()
{
  Blynk.begin(auth, ssid, pass);
  
  pinMode(pirPin, INPUT);
  
  timer.setInterval(1000L,sendSensor);
  
  servo_1.attach(5); // Attaching Servo to D1 of Nodemcu corresponds to pin 5 of arduino uno
  
  Serial.begin(9600);

  // Connect to WiFi
  Serial.print("\n\nConnecting Wifi....");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("...");
    delay(1000);
  }

  Serial.println("OK!");

  //Subscribe to the onoff topic
  mqtt.subscribe(&onoff);
}
void loop()
{
  Blynk.run();
  
  sendSensor(); //calling the fn of temp and humdity
  if (pinValue == HIGH)    //pinValue is the status of Notification Button that we have created in Blynk App(to stop sending notification mutilple times)
  {                         //If Notification Button is HIGH(ON)then it send notification otherwise not for Motion Detector
     getPirValue(); //calling the fn of PIR
  }
  
  /*ADARFRUIT.IO*/
  
  // Connect/Reconnect to MQTT
  MQTT_connect();

  //Read from our subscription queue until we run out, or
  //wait up to 5 seconds for subscription to update
  
  Adafruit_MQTT_Subscribe * subscription;
  while ((subscription = mqtt.readSubscription(5000)))
  {
    //If we're in here, a subscription updated...
    if (subscription == &onoff)
    {
      //Print the new value to the serial monitor
      Serial.print("onoff: ");
      Serial.println((char*) onoff.lastread);

      //If the new value is  "ON", turn the light on.
      //Otherwise, turn it off.
      if (!strcmp((char*) onoff.lastread, "ON"))  //For Garage OPEN/CLOSE
      {
         servo_1.write(0); 
         delay(4000); 
         // Make servo go to 90 degrees 
         servo_1.write(90); 
         delay(1000); 
         
         LightsStatus.publish("ON");       //Update the Status of Servo on adafruit.io in ServoStatus
      }
      else
      {
        LightsStatus.publish("ERROR");
      }
    }
    else
    {
      LightsStatus.publish("ERROR");
    }
  }
}

void sendSensor()
{
  DHT.read11(dht_apin);
  
  float h = DHT.humidity;
  float t = DHT.temperature;
  
  Serial.print("Temp : ");
  Serial.print(t);
  Serial.print("\tHumdity : ");
  Serial.println(h); 

  Blynk.virtualWrite(V5, h);  //Send the data of humidity to Virtual pin V5 of Blynk App
  Blynk.virtualWrite(V6, t);  //Send the data of temperature to Virtual pin V6 of Blynk App

  if (pinValue1 == HIGH)    //pinValue1 is the status of Notification Button that we have created in Blynk App(to stop sending notification mutilple times)
  {                         //If Notification Button is HIGH(ON)then it send notification otherwise not
    if(h>=54)
    {
      Serial.print("Humidity is High *********************************\n");
      Blynk.notify("Alert : Please Turn ON the Fan \nTemp = " + String(t) + "\tHumidity = "+String(h)); //Notification msg
    }
  }
} 

void getPirValue()      
{
    pirValue = digitalRead(pirPin); 
    if (pirValue)   
    { 
       Serial.println("Motion detected");
       Blynk.notify("Motion detected");  //Notification msg
    }
}

void MQTT_connect()
{

  // Stop if already connected
  if (mqtt.connected() && mqtt.ping())
  {
    //    mqtt.disconnect();
    return;
  }

  int8_t ret;

  mqtt.disconnect();

  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) // connect will return 0 for connected
  {
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0)
    {
      ESP.reset();
    }
  }
  Serial.println("MQTT Connected!");
}


