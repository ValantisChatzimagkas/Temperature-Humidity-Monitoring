/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE"
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.
 * ***********************************************************************
   DFRobot's SHT20 Humidity And Temperature Sensor Module
   For ESP32 LOLIN32 Pro
   RED - VCC = 3.3V
   GREEN - GND = GND
   BLUE (BLACK) - SDA = IO21 (use inline 330 ohm resistor if your board is 5V)
   YELLOW (WHITE) - SCL = IO22(use inline 330 ohm resistor if your board is 5V)
 * ***********************************************************************
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include "DFRobot_SHT20.h"

DFRobot_SHT20    sht20;

#define SERVICE_UUID            "0000180a-0000-1000-8000-00805f9b34fb" // the service UUID for Client to connect
#define CHARACTERISTIC_UUID_TX  "0000180a-0000-1000-8000-00805f9b34fb" // the Characteristic UUID for Client to access the data

int i = 0; // for labeling the data 
float humd = 0;// store humidity measurement  
float temp = 0;// store temperature measurement

bool deviceConnected = false; // a flag for connection
BLECharacteristic *pCharacteristic;
std::string Sample; // store the measurement data
char array[100]; // pack measurement data for transmission

    /*
      detect if the two modules still connected 
      if connected, it prints Connected
      if disconnected, it prints DisConnected
    */

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Connected :) ");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("DisConnected :( ");
    }
};
    /*
      the main function of server 
    */
void runTx() {

  // Create the BLE Device
  BLEDevice::init("Sensor1"); // Give it a name
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_READ
                    );
  pCharacteristic->addDescriptor(new BLE2902()); // for notification
  // Start the service
  pService->start();
  // Start advertising
  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID); //add UUID for advertising
  pServer->getAdvertising()->start(); // start to advertise
  
  Serial.println("========================================================================");
  Serial.println("SEVER START");
  Serial.println("Waiting a client connection to notify...");
}




void setup() {
  Serial.begin(115200);
  runTx(); // server function
  Serial.println("Temp&Humi Sensor Data: ");
  sht20.initSHT20();                                  // Init SHT20 Sensor
  delay(100);
  sht20.checkSHT20();                                 // Check SHT20 Sensor
  Serial.println("*********************");

}



void loop() {

  if (deviceConnected == true) {
    i++;


     humd = sht20.readHumidity();                  // Read Humidity
     temp = sht20.readTemperature();               // Read Temperature

    Serial.print(" Temperature:");
    Serial.print(temp, 1);
    Serial.print("C");
    Serial.print(" Humidity:");
    Serial.print(humd, 1);
    Serial.print("% ***");
    Serial.println();

    sprintf(array, "DS: %d\n", i); // to convert number to standard string data type with formatting 
    Sample = std::string(array); // store the data to Sample
  
    /*
      to detect the frozen temperature
      if the temperature is below the frozen temperature, the measurement data will be labeled [!] 
    */
    if (temp > 0) {
      sprintf(array, "S1-T:%05.2fH:%05.2f\n",temp, humd);
      Sample += std::string(array);
    }
    if (temp <= 0) {
      sprintf(array, "S1-[!]T:%05.2fH:%05.2f\n", temp, humd);
      Sample += std::string(array);
    }

    pCharacteristic->setValue(Sample); // get the data ready for sending
    
    Serial.println();
    Serial.println("*** Sent DATA *** ");
    Serial.print(Sample.c_str());
    Serial.println("***************** ");
    Serial.println();
   /*
      the loop to continually send data until next device got the data
    */ 
    while (deviceConnected == true) {
      pCharacteristic->notify(); // Send the value to the app!
      delay(1000); // delay here is important for ESP32 have time to send data
    }
    delay(2000);
    //================================================
    // ESP.restart();

    // pCharacteristic->~BLECharacteristic();
    //================================================
  }
  delay(2000);
} // end loop
