/*  
  OpenMQTTCharger  - ESP8266 program for battery monitoring 

   Act as a voltage and current sensor transmitting data by MQTT
 
  From the orgiginal work of Matthias Busse http://shelvin.de/ein-batteriemonitor-fuer-strom-und-spannung-mit-dem-ina226-und-dem-arduino-uno/
  MQTT add - 1technophile
  
Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
//
// Pinout
// INA226 - Uno - Mega - NODEMCU
// SCL    - A5  - 21 - D1
// SDA    - A4  - 20 - D2
//

#include <Wire.h>
#include "User_config.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

//adding this to bypass the problem of the arduino builder issue 50
void callback(char*topic, byte* payload,unsigned int length);

WiFiClient eClient;

// client parameters
PubSubClient client(mqtt_server, mqtt_port, callback, eClient);

//MQTT last attemps reconnection date
int lastReconnectAttempt = 0;

boolean reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    trc(F("Attempting MQTT connection...")); //F function enable to decrease sram usage
    #ifdef mqtt_user
      if (client.connect(Gateway_Name, mqtt_user, mqtt_password, will_Topic, will_QoS, will_Retain, will_Message)) { // if an mqtt user is defined we connect to the broker with authentication
      trc(F("connected with authentication"));
    #else
      if (client.connect(Gateway_Name, will_Topic, will_QoS, will_Retain, will_Message)) {
      trc(F("connected without authentication"));
    #endif
    // Once connected, publish an announcement...
      client.publish(will_Topic,Gateway_AnnouncementMsg);
      //Subscribing to topic
      if (client.subscribe(subjectMQTTtoX)) {
        trc(F("subscription OK to"));
        trc(subjectMQTTtoX);
      }
      } else {
      trc(F("failed, rc="));
      trc(String(client.state()));
      trc(F(" try again in 5 seconds"));
      // Wait x milli seconds before retrying
      delay(cycle_time);
    }
  }
  return client.connected();
}

// Callback function, when the gateway receive an MQTT value on the topics subscribed this function is called
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.
  trc(F("Hey I got a callback "));
  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length + 1);
  // Copy the payload to the new buffer
  memcpy(p,payload,length);
  // Conversion to a printable string
  p[length] = '\0';
  //launch the function to treat received data
  receivingMQTT(topic,(char *) p);
  // Free the memory
  free(p);
}

float rShunt=0.1; // Shunt Widerstand festlegen, hier 0.1 Ohm
const int INA226_ADDR = 0x40; // A0 und A1 auf GND > Adresse 40 Hex auf Seite 18 im Datenblatt

void setup() {
  Wire.begin();
  //Launch serial for debugging purposes
  Serial.begin(115200);
  
  //Begining wifi connection in case of ESP8266
  setup_wifi();
  
  delay(1500);
  
  lastReconnectAttempt = 0;
    
  // Configuration Register Standard Einstellung 0x4127, hier aber 16 Werte Mitteln > 0x4427
  writeRegister(0x00, 0x4427); // 1.1ms Volt und Strom A/D-Wandlung, Shunt und VBus continous
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  trc(F("Connecting to "));
  trc(wifi_ssid);
  IPAddress ip_adress(ip);
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress dns_adress(Dns);
  WiFi.begin(wifi_ssid, wifi_password);
  WiFi.config(ip_adress,gateway_adress,subnet_adress); //Uncomment this line if you want to use advanced network config
  trc("OpenMQTTGateway ip adress: ");   
  Serial.println(WiFi.localIP());
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    trc(F("."));
  }
  trc(F("WiFi connected"));
}

void loop(){
  
 //MQTT client connexion management
  if (!client.connected()) { // not connected
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      trc(F("client mqtt not connected, trying to connect"));
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else { //connected
    // MQTT loop
    client.loop();
    // Receive loop, if data received by RF433 or IR send it by MQTT
      boolean result = sendValuebyMQTT();
      if(result)
        trc(F("value successfully sent by MQTT"));
    }
}


static void writeRegister(byte reg, word value) {
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);
  Wire.write((value >> 8) & 0xFF);
  Wire.write(value & 0xFF);
  Wire.endTransmission();
}

static word readRegister(byte reg) {
  word res = 0x0000;
  Wire.beginTransmission(INA226_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission() == 0) {
    if (Wire.requestFrom(INA226_ADDR, 2) >= 2) {
      res  = Wire.read() * 256;
      res += Wire.read();
    }
  }
  return res;
}


boolean sendValuebyMQTT(){
// Topic on which we will send data
trc(F("Receiving electrical data"));
  // Bus Spannung, read-only, 16Bit, 0...40.96V max., LSB 1.25mV
  Serial.print(" Volt: ");
  float volt = readRegister(0x02) * 0.00125; 
  Serial.print(volt,3);
  Serial.print(" V, Current: ");
  // Seite 24: Shunt Spannung +- 81,92mV mit 16 Bit, LSB 2,5uV
  int shuntvolt = readRegister(0x01);
  if (shuntvolt && 0x8000) {// eine negative Zahl? Dann 2er Komplement bilden
    shuntvolt = ~shuntvolt; // alle Bits invertieren
    shuntvolt += 1;         // 1 dazuzählen
    shuntvolt *= -1 ;       // negativ machen
  }
  float current = shuntvolt * 0.0000025 / rShunt; // * LSB / R
  Serial.print(current,3);
  Serial.print(" A, Power: ");
  float power=abs(volt*current);
  Serial.print(power,3);
  Serial.print(" W");
  Serial.println();

  char volt_c[7];
  char current_c[7];
  char power_c[7];
  dtostrf(volt,6,3,volt_c);
  dtostrf(current,6,3,current_c);
  dtostrf(power,6,3,power_c);

  client.publish(subjectVolttoMQTT,volt_c);
  client.publish(subjectCurrenttoMQTT,current_c);          
  client.publish(subjectPowertoMQTT,power_c);  
  
delay(5000);
return true;
}

void receivingMQTT(char * topicOri, char * datacallback) {
  
  String topic = topicOri;

  trc(F("Receiving data by MQTT"));
  trc(topic);  
  trc(F("Callback value"));
  trc(String(datacallback));
  unsigned long data = strtoul(datacallback, NULL, 10); // we will not be able to pass values > 4294967295
  trc(F("Converted value to unsigned long"));
  trc(String(data));
}

//trace
void trc(String msg){
  if (TRACE) {
  Serial.println(msg);
  }
}
