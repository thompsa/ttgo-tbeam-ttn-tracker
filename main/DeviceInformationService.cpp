#include "DeviceInformationService.h"
#include <esp_gatt_defs.h>
#include <BLE2902.h>
#include <Arduino.h>

BLEService *createDeviceInfomationService(BLEServer* server) {
    BLEService *m_deviceInfoService = server->createService(BLEUUID((uint16_t) ESP_GATT_UUID_DEVICE_INFO_SVC));

    /*
	 * Mandatory characteristic for device info service?
	 
	BLECharacteristic *m_pnpCharacteristic = m_deviceInfoService->createCharacteristic(ESP_GATT_UUID_PNP_ID, BLECharacteristic::PROPERTY_READ);

    uint8_t sig, uint16_t vid, uint16_t pid, uint16_t version;
	uint8_t pnp[] = { sig, (uint8_t) (vid >> 8), (uint8_t) vid, (uint8_t) (pid >> 8), (uint8_t) pid, (uint8_t) (version >> 8), (uint8_t) version };
	m_pnpCharacteristic->setValue(pnp, sizeof(pnp));
    */

    // m_manufacturerCharacteristic = m_deviceInfoService->createCharacteristic((uint16_t) 0x2a29, BLECharacteristic::PROPERTY_READ);
	// m_manufacturerCharacteristic->setValue(name);

    /* add these later?
    ESP_GATT_UUID_SYSTEM_ID
    ESP_GATT_UUID_SW_VERSION_STR
    */

   // caller must call service->start();
   return m_deviceInfoService;
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

static BLECharacteristic BatteryLevelCharacteristic(BLEUUID((uint16_t) ESP_GATT_UUID_BATTERY_LEVEL), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
static BLEDescriptor BatteryLevelDescriptor(BLEUUID((uint16_t) ESP_GATT_UUID_CHAR_DESCRIPTION));

BLEService *createBatteryService(BLEServer* server) {
    // Create the BLE Service
    BLEService *pBattery = server->createService(BLEUUID((uint16_t)0x180F));

    pBattery->addCharacteristic(&BatteryLevelCharacteristic);
    BatteryLevelDescriptor.setValue("Percentage 0 - 100");
    BatteryLevelCharacteristic.addDescriptor(&BatteryLevelDescriptor);
    BatteryLevelCharacteristic.addDescriptor(new BLE2902());

    return pBattery;
}

void initBLE() {
    BLEDevice::init("KHBT Test");
    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pDevInfo = createDeviceInfomationService(pServer);

    BLEService *pBattery = createBatteryService(pServer);
    pServer->getAdvertising()->addServiceUUID(pBattery->getUUID());

    // start all our services (do this after creating all of them)
    pDevInfo->start();
    pBattery->start();

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