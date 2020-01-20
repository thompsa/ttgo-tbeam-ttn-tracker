#pragma once

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

BLEService *createDeviceInfomationService(BLEServer* server, uint8_t sig, uint16_t vid, uint16_t pid, uint16_t version);

