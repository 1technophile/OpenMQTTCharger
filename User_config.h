/*  
  OpenMQTTCharger  - ESP8266 or Arduino program for battery monitoring 

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
/*----------------------------USER PARAMETERS-----------------------------*/
/*-------------DEFINE YOUR MQTT & NETWORK PARAMETERS BELOW----------------*/
//MQTT Parameters definition
#define mqtt_server "192.168.1.17"
#define mqtt_user "your_username" // not compulsory only if your broker needs authentication
#define mqtt_password "your_password" // not compulsory only if your broker needs authentication
#define mqtt_port 1883

// time between voltage and current measurement in ms
#define cycle_time 5000

// Update these with values suitable for your network.
#ifdef ESP8266 // for nodemcu, weemos and esp8266
  #define wifi_ssid "wifi ssid"
  #define wifi_password "wifi password"
#else // for arduino + W5100
  const byte mac[] = {  0xDE, 0xED, 0xBA, 0xFE, 0x54, 0x95 }; //W5100 ethernet shield mac adress
#endif

//Addons management, comment the line if you don't use
//#define ZaddonDHT true

/*----------------------------OTHER PARAMETERS-----------------------------*/
/*-------------------CHANGING THEM IS NOT COMPULSORY-----------------------*/
//MQTT Parameters definition
#define Gateway_Name "OpenMQTTCharger"
#define will_Topic "home/OpenMQTTCharger/LWT"
#define will_QoS 0
#define will_Retain true
#define will_Message "Offline"
#define Gateway_AnnouncementMsg "Online"

const byte ip[] = { 192, 168, 1, 99 }; //ip adress
// Advanced network config (optional) if you want to use these parameters uncomment line 158, 172 and comment line 171  of OpenMQTTGateway.ino
const byte gateway[] = { 192, 168, 1, 1 }; //ip adress
const byte Dns[] = { 192, 168, 1, 1 }; //ip adress
const byte subnet[] = { 255, 255, 255, 0 }; //ip adress

//MQTT definitions
// global MQTT subject listened by the gateway to execute commands (send RF, IR or others)
#define subjectMQTTtoX "home/commands/OpenMQTTCharger/#"

//433Mhz MQTT Subjects and keys
#define subjectVolttoMQTT "home/OpenMQTTCharger/Volt"
#define subjectCurrenttoMQTT "home/OpenMQTTCharger/Current"        
#define subjectPowertoMQTT "home/OpenMQTTCharger/Power"  

//Do we want to see trace for debugging purposes
#define TRACE 1  // 0= trace off 1 = trace on
