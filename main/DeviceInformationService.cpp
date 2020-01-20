#include "DeviceInformationService.h"
#include <esp_gatt_defs.h>
#include <BLE2902.h>
#include <Arduino.h>

static BLECharacteristic SWVersionCharacteristic(BLEUUID((uint16_t) ESP_GATT_UUID_SW_VERSION_STR), BLECharacteristic::PROPERTY_READ);
static BLECharacteristic ManufacturerCharacteristic(BLEUUID((uint16_t) ESP_GATT_UUID_MANU_NAME), BLECharacteristic::PROPERTY_READ);
static BLECharacteristic SerialNumberCharacteristic(BLEUUID((uint16_t) ESP_GATT_UUID_SERIAL_NUMBER_STR), BLECharacteristic::PROPERTY_READ);

BLEService *createDeviceInfomationService(BLEServer* server) {
    BLEService *deviceInfoService = server->createService(BLEUUID((uint16_t) ESP_GATT_UUID_DEVICE_INFO_SVC));

    /*
	 * Mandatory characteristic for device info service?
	 
	BLECharacteristic *m_pnpCharacteristic = m_deviceInfoService->createCharacteristic(ESP_GATT_UUID_PNP_ID, BLECharacteristic::PROPERTY_READ);

    uint8_t sig, uint16_t vid, uint16_t pid, uint16_t version;
	uint8_t pnp[] = { sig, (uint8_t) (vid >> 8), (uint8_t) vid, (uint8_t) (pid >> 8), (uint8_t) pid, (uint8_t) (version >> 8), (uint8_t) version };
	m_pnpCharacteristic->setValue(pnp, sizeof(pnp));
    */
    SWVersionCharacteristic.setValue("0.1");
    deviceInfoService->addCharacteristic(&SWVersionCharacteristic);
    ManufacturerCharacteristic.setValue("TTGO");
    deviceInfoService->addCharacteristic(&ManufacturerCharacteristic);   
    SerialNumberCharacteristic.setValue("FIXME");
    deviceInfoService->addCharacteristic(&SerialNumberCharacteristic); 

    // m_manufacturerCharacteristic = m_deviceInfoService->createCharacteristic((uint16_t) 0x2a29, BLECharacteristic::PROPERTY_READ);
	// m_manufacturerCharacteristic->setValue(name);


    /* add these later?
    ESP_GATT_UUID_SYSTEM_ID
    */

   // caller must call service->start();
   return deviceInfoService;
}

bool _BLEClientConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      _BLEClientConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      _BLEClientConnected = false;
    }
};

// Help routine to add a description to any BLECharacteristic and add it to the service
static void addWithDesc(BLEService *service, BLECharacteristic *c, const char *description) {
  BLEDescriptor *desc = new BLEDescriptor(BLEUUID((uint16_t) ESP_GATT_UUID_CHAR_DESCRIPTION), strlen(description) + 1);
  desc->setValue(description);
  c->addDescriptor(desc);
  service->addCharacteristic(c);
}

static BLECharacteristic BatteryLevelCharacteristic(BLEUUID((uint16_t) ESP_GATT_UUID_BATTERY_LEVEL), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

BLEService *createBatteryService(BLEServer* server) {
    // Create the BLE Service
    BLEService *pBattery = server->createService(BLEUUID((uint16_t)0x180F));

    addWithDesc(pBattery, &BatteryLevelCharacteristic, "Percentage 0 - 100");
    BatteryLevelCharacteristic.addDescriptor(new BLE2902()); // Needed so clients can request notification

    return pBattery;
}

static BLECharacteristic swUpdateTotalSizeCharacteristic("e74dd9c0-a301-4a6f-95a1-f0e1dbea8e1e", BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ);
static BLECharacteristic swUpdateDataCharacteristic("e272ebac-d463-4b98-bc84-5cc1a39ee517", BLECharacteristic::PROPERTY_WRITE);
static BLECharacteristic swUpdateCRC32Characteristic("4826129c-c22a-43a3-b066-ce8f0d5bacc6", BLECharacteristic::PROPERTY_WRITE);
static BLECharacteristic swUpdateResultCharacteristic("5e134862-7411-4424-ac4a-210937432c77", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

class UpdateCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};

UpdateCallbacks updateCb;

/*
SoftwareUpdateService UUID cb0b9a0b-a84c-4c0d-bdbb-442e3144ee30

Characteristics

UUID                                 properties          description
e74dd9c0-a301-4a6f-95a1-f0e1dbea8e1e write|read          total image size, 32 bit, write this first, then read read back to see if it was acceptable (0 mean not accepted)
e272ebac-d463-4b98-bc84-5cc1a39ee517 write               data, variable sized, recommended 512 bytes, write one for each block of file
4826129c-c22a-43a3-b066-ce8f0d5bacc6 write               crc32, write last - writing this will complete the OTA operation, now you can read result
5e134862-7411-4424-ac4a-210937432c77 read|notify         result code, readable but will notify when the OTA operation completes
 */
BLEService *createUpdateService(BLEServer* server) {
    // Create the BLE Service
    BLEService *service = server->createService("cb0b9a0b-a84c-4c0d-bdbb-442e3144ee30");

    addWithDesc(service, &swUpdateTotalSizeCharacteristic, "total image size, 32 bit, write this first, then read read back to see if it was acceptable (0 mean not accepted)");
    addWithDesc(service, &swUpdateDataCharacteristic, "data, variable sized, recommended 512 bytes, write one for each block of file");
    addWithDesc(service, &swUpdateCRC32Characteristic, "crc32, write last - writing this will complete the OTA operation, now you can read result");
    addWithDesc(service, &swUpdateResultCharacteristic, "result code, readable but will notify when the OTA operation completes");

    swUpdateTotalSizeCharacteristic.setCallbacks(&updateCb);
    swUpdateDataCharacteristic.setCallbacks(&updateCb);
    swUpdateCRC32Characteristic.setCallbacks(&updateCb);
    
    swUpdateResultCharacteristic.addDescriptor(new BLE2902()); // Needed so clients can request notification

    return service;
}


void initBLE() {
    BLEDevice::init("KHBT Test");
    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pDevInfo = createDeviceInfomationService(pServer);

    BLEService *pBattery = createBatteryService(pServer);
    pServer->getAdvertising()->addServiceUUID(pBattery->getUUID());

    BLEService *pUpdate = createUpdateService(pServer);

    // start all our services (do this after creating all of them)
    pDevInfo->start();
    pBattery->start();
    pUpdate->start();

    // Start advertising
    pServer->getAdvertising()->start();
}

// Called from loop
void loopBLE() {
    static uint8_t level = 31;

    BatteryLevelCharacteristic.setValue(&level, 1);
    BatteryLevelCharacteristic.notify();
    delay(5000);

    level = (level + 1) % 100;
}