/*
   It shows the received values in Serial and is also able to switch notificaton on the sensor on and off (using BLE2902)

   Copyright <2017> <Andreas Spiess>

  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.

   Based on Neil Kolban's example file: https://github.com/nkolban/ESP32_BLE_Arduino
 * ***********************************************************************
   DFRobot's SHT20 Humidity And Temperature Sensor Module
   For ESP32 LOLIN32 Pro
   RED - VCC = 3.3V
   GREEN - GND = GND
   BLUE (BLACK) - SDA = IO21 (use inline 330 ohm resistor if your board is 5V)
   YELLOW (WHITE) - SCL = IO22(use inline 330 ohm resistor if your board is 5V)
 * ***********************************************************************
*/

#include "BLEDevice.h"
//#include "BLEScan.h"
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include "DFRobot_SHT20.h"

#define uS_TO_S_FACTOR 1000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  20        /* Time ESP32 will go to sleep (in seconds) */

DFRobot_SHT20    sht20; // for the Temp and Humd sensor

// for client
// The remote service we wish to connect to.
static BLEUUID SERVICE_UUID1 ("0000181a-0000-1000-8000-00805f9b34fb");
// The characteristic of the remote service we are interested in.
static BLEUUID CHARACTERISTIC_UUID ("0000181a-0000-1000-8000-00805f9b34fb");

// for server
//#define SERVICE_UUID2          "0000181a-0000-1000-8000-00805f9b34fb"
static BLEUUID SERVICE_UUID2          ("0000182a-0000-1000-8000-00805f9b34fb"); // the service UUID for Client to connect
//#define CHARACTERISTIC_UUID_TX "0000181a-0000-1000-8000-00805f9b34fb"
static BLEUUID  CHARACTERISTIC_UUID_TX ("0000182a-0000-1000-8000-00805f9b34fb"); // the Characteristic UUID for Client to access the data

/**
   client variables declearation
*/
static BLEClient*  pClient  = BLEDevice::createClient(); // create a new client
static boolean doConnect = false; // for client
static boolean connected = false; // for client
static BLERemoteCharacteristic* pRemoteCharacteristic; // for client
static BLEAddress *pServerAddress; // for client
/**
   server variables declearation
*/
BLECharacteristic *pCharacteristic; // for server
bool deviceConnected = false; // for server
/**
   other variables declearation
*/
int i = 0; // for data receive times counting
float humd = 0; // humidity
float temp = 0; // temperature
std::string rxData; // device received data
std::string detector = ""; // to detect if the device has received data
std::string Sample; // to store received data and pass to next device
char array[100];
/**
   functions for client
*/
void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {

  Serial.println(" CHEN ");
}

// Disconnection from the sever
void disconnectToServer()
{
  pClient->disconnect();
  Serial.println(" - disConnected to server");
}

// connect to the sever that the device before it
bool connectToServer(BLEAddress pAddress)
{
  Serial.print("Forming a connection to ");
  Serial.println(pAddress.toString().c_str());

  // BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID1);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(SERVICE_UUID1.toString().c_str());
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr)
  {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(CHARACTERISTIC_UUID.toString().c_str());
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.

  pRemoteCharacteristic->registerForNotify(notifyCallback);
}

/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());

      // We have found a device, let us now see if it contains the service we are looking for.
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(SERVICE_UUID1))
      {
        //
        Serial.print("Found our device!  address: ");
        advertisedDevice.getScan()->stop();
        pServerAddress = new BLEAddress(advertisedDevice.getAddress());
        // Serial.print(pServerAddress->toString().c_str());
        doConnect = true;
        //Serial.print(BLEAddress);
      } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks

/**
    main function to receive data
*/
void runRx()
{
  Serial.println("===================================================================");
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("Sensor3");
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  // scan for servers around
  BLEScan* pBLEScan = BLEDevice::getScan();
  // once the scanner find a server, it will check if the server is it wants
  // get ready for connection 
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  // activate the scanner 
  pBLEScan->setActiveScan(true);
  // scan start 
  pBLEScan->start(120);
}

/**
   functions for server
*/

class MyServerCallbacks: public BLEServerCallbacks
{
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Connected :) ");
    };

    void onDisconnect(BLEServer* pServer)
    {
      deviceConnected = false;
      Serial.println("DisConnected :( ");
    }
};

void setup()
{
  Serial.println("wake up in 2 minutes");
  delay(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  Serial.begin(115200);
  runRx();
  Serial.println("Temp&Humi Sensor Data: ");
  sht20.initSHT20();                                  // Init SHT20 Sensor
  delay(100);
  sht20.checkSHT20();                                 // Check SHT20 Sensor
  Serial.println("*********************");

} // End of setup.



void loop()
{

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  /**
     Run as a Client to receive data
  */
  if (doConnect == true)
  {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server.");
      connected = true;
    } else
    {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
      connected = false;
    }
    doConnect = false;
  }

  // If it is connected to a peer BLE Server, update the characteristic each time 

  if (connected == true)
  {
    /*
      the loop to continually send data until next device got the data
    */
    while (rxData == detector) {
      delay(1000);
      rxData = pRemoteCharacteristic->readValue();
    }
    Serial.println();
    Serial.println("*** Received DATA ***");
    Serial.print(rxData.c_str());
    Serial.println("*********************");
    Serial.println();
    
    disconnectToServer(); //disconnect client from server
    pClient->~BLEClient(); // destroy the client created before

    /*
      run as a sever to forward the retrieved value
    */

    // Create the BLE Device
    // BLEDevice::init("ESP32_Oddie"); // Give it a name

    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID2);
    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_READ
                      );
    // Create a BLE Descriptor
    pCharacteristic->addDescriptor(new BLE2902());
    // Start the service
    pService->start();
    // Start advertising
    pServer->getAdvertising()->addServiceUUID(SERVICE_UUID2); //add UUID for advertising
    pServer->getAdvertising()->start();
    Serial.println("========================================================================");
    Serial.println("SEVER START");
    Serial.println("Waiting a client connection to notify...");

    while (deviceConnected == false)
    {
      delay(1000);
    }

    if (deviceConnected == true)
    {
      i++;

      humd = sht20.readHumidity();                  // Read Humidity
      //temp = sht20.readTemperature();               // Read Temperature
      
      temp = -1.32;               // for demo temp below the freezing temp
      
      Serial.print(" Temperature:");
      Serial.print(temp, 1);
      Serial.print("C");
      Serial.print(" Humidity:");
      Serial.print(humd, 1);
      Serial.print("% ***");
      Serial.println();
      Sample = rxData;
      
    /*
      to detect the frozen temperature
      if the temperature is below the frozen temperature, the measurement data will be labeled [!] 
    */
    
      if (temp > 0) {
        sprintf(array, "S3-T:%05.2fH:%05.2f\n", temp, humd);
        Sample += std::string(array);
      }
      if (temp <= 0) {
        sprintf(array, "S3-[!]T:%05.2fH:%05.2f\n", temp, humd);
        Sample += std::string(array);
      }

      pCharacteristic->setValue(Sample);
      Serial.println();
      Serial.println("*** Sent DATA *** ");
      Serial.print(Sample.c_str()); // print out revceived data
      Serial.println("***************** ");
      Serial.println();
      while (deviceConnected == true) {
        delay(1000);
        pCharacteristic->notify(); // Send the value to the app!
      }
      delay(2000);

      /*
        the loop is for the device to make sure the device have reveived data
        it continually send data untill disconnected
      */ 
      
      while (deviceConnected)
      {
        Serial.println("Still connected to server");
        delay(2000);
      }
      // distroy server class
      // pServer->~BLEServer();//Valantis added this
      //=================================================
      Serial.println("Going to sleep now");
      //pServer->getAdvertising()->stop();
      //pCharacteristic->~BLECharacteristic();
      //runRx();
      //=================================================
      ESP.restart();//restart the device back to receive data
    }
  }
  delay(2000);
} // End of loop
