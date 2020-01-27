#include "BluetoothUtil.h"
#include "MeshBluetoothService.h"
#include <esp_gatt_defs.h>
#include <BLE2902.h>
#include <Arduino.h>


class BluetoothMeshCallbacks : public BLECharacteristicCallbacks
{
    void onRead(BLECharacteristic *pCharacteristic) {
        BLECharacteristicCallbacks::onRead(pCharacteristic);
        Serial.println("Got on read");
    }

    void onWrite(BLECharacteristic *pCharacteristic)
    {
        // dumpCharacteristic(pCharacteristic);
    /*
        if (pCharacteristic == &swUpdateTotalSizeCharacteristic)
        {
            // Check if there is enough to OTA Update
            uint32_t len = getValue32(pCharacteristic, 0);
            crc.reset();
            bool canBegin = Update.begin(len);
            Serial.printf("Setting update size %u, result %d\n", len, canBegin);
            if(!canBegin)
                // Indicate failure by forcing the size to 0
                pCharacteristic->setValue(0UL);
        }
    */
    }
};

static BluetoothMeshCallbacks btMeshCb;

static BLECharacteristic meshFromRadioCharacteristic("8ba2bcc2-ee02-4a55-a531-c525c5e454d5", BLECharacteristic::PROPERTY_READ);
static BLECharacteristic meshToRadioCharacteristic("f75c76d2-129e-4dad-a1dd-7866124401e7", BLECharacteristic::PROPERTY_WRITE);
static BLECharacteristic meshFromNumCharacteristic("ed9da18c-a800-4f66-a670-aa7547e34453", BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

/*
MeshBluetoothService UUID 6ba1b218-15a8-461f-9fa8-5dcae273eafd

FIXME - notify vs indiciation for fromradio output.  Using notify for now, not sure if that is best
FIXME - in the esp32 mesh managment code, occasionally mirror the current net db to flash, so that if we reboot we still have a good guess of users who are out there.
FIXME - make sure this protocol is guaranteed robust and won't drop packets

"According to the BLE specification the notification length can be max ATT_MTU - 3. The 3 bytes subtracted is the 3-byte header(OP-code (operation, 1 byte) and the attribute handle (2 bytes)).
In BLE 4.1 the ATT_MTU is 23 bytes (20 bytes for payload), but in BLE 4.2 the ATT_MTU can be negotiated up to 247 bytes."

MAXPACKET is 256? look into what the lora lib uses. FIXME

Characteristics:
UUID                                 
properties          
description

8ba2bcc2-ee02-4a55-a531-c525c5e454d5                                 
read                
fromradio - contains a newly received packet destined towards the phone (up to MAXPACKET bytes? per packet).
After reading the esp32 will put the next packet in this mailbox.  If the FIFO is empty it will put an empty packet in this
mailbox.

f75c76d2-129e-4dad-a1dd-7866124401e7                             
write               
toradio - write ToRadio protobufs to this charstic to send them (up to MAXPACKET len)

ed9da18c-a800-4f66-a670-aa7547e34453                                  
read|notify|write         
fromnum - the current packet # in the message waiting inside fromradio, if the phone sees this notify it should read messages
until it catches up with this number.
  The phone can write to this register to go backwards up to FIXME packets, to handle the rare case of a fromradio packet was dropped after the esp32 
callback was called, but before it arrives at the phone.  If the phone writes to this register the esp32 will discard older packets and put the next packet >= fromnum in fromradio.
When the esp32 advances fromnum, it will delay doing the notify by 100ms, in the hopes that the notify will never actally need to be sent if the phone is already pulling from fromradio.
  Note: that if the phone ever sees this number decrease, it means the esp32 has rebooted.

Re: queue management
Not all messages are kept in the fromradio queue (filtered based on SubPacket):
* only the most recent Position and User messages for a particular node are kept
* all Data SubPackets are kept
* No WantNodeNum / DenyNodeNum messages are kept
A variable keepAllPackets, if set to true will suppress this behavior and instead keep everything for forwarding to the phone (for debugging)

 */
BLEService *createMeshBluetoothService(BLEServer* server) {
    // Create the BLE Service
    BLEService *service = server->createService("6ba1b218-15a8-461f-9fa8-5dcae273eafd");

    addWithDesc(service, &meshFromRadioCharacteristic, "fromRadio");
    addWithDesc(service, &meshToRadioCharacteristic, "toRadio");
    addWithDesc(service, &meshFromNumCharacteristic, "fromNum");

    meshFromRadioCharacteristic.setCallbacks(&btMeshCb);
    meshToRadioCharacteristic.setCallbacks(&btMeshCb);
    meshFromNumCharacteristic.setCallbacks(&btMeshCb);
    
    meshFromNumCharacteristic.addDescriptor(new BLE2902()); // Needed so clients can request notification

    return service;
}

